//
// Created by liu meng on 2018/8/20.
//
#include "Class.h"
#include "Globals.h"
#include "log.h"
bool dvmInitClass(ClassObject* clazz)
{
    u8 startWhen = 0;

#if LOG_CLASS_LOADING
    bool initializedByUs = false;
#endif

    Thread* self = dvmThreadSelf();
    const Method* method;

    dvmLockObject(self, (Object*) clazz);
    assert(dvmIsClassLinked(clazz) || clazz->status == CLASS_ERROR);

    /*
     * If the class hasn't been verified yet, do so now.
     */
    if (clazz->status < CLASS_VERIFIED) {
        /*
         * If we're in an "erroneous" state, throw an exception and bail.
         */
        if (clazz->status == CLASS_ERROR) {
            throwEarlierClassFailure(clazz);
            goto bail_unlock;
        }

        assert(clazz->status == CLASS_RESOLVED);
        assert(!IS_CLASS_FLAG_SET(clazz, CLASS_ISPREVERIFIED));

        if (gDvm.classVerifyMode == VERIFY_MODE_NONE ||
            (gDvm.classVerifyMode == VERIFY_MODE_REMOTE &&
             clazz->classLoader == NULL))
        {
            /* advance to "verified" state */
            ALOGW("+++ not verifying class %s (cl=%p)",
                  clazz->descriptor, clazz->classLoader);
            clazz->status = CLASS_VERIFIED;
            goto noverify;
        }

        if (!gDvm.optimizing)
                ALOGW("+++ late verify on %s", clazz->descriptor);

        /*
         * We're not supposed to optimize an unverified class, but during
         * development this mode was useful.  We can't verify an optimized
         * class because the optimization process discards information.
         */
        if (IS_CLASS_FLAG_SET(clazz, CLASS_ISOPTIMIZED)) {
            ALOGW("Class '%s' was optimized without verification; "
                          "not verifying now",
                  clazz->descriptor);
            ALOGW("  ('rm /data/dalvik-cache/*' and restart to fix this)");
            goto verify_failed;
        }

        clazz->status = CLASS_VERIFYING;
        if (!dvmVerifyClass(clazz)) {
            verify_failed:
            dvmThrowVerifyError(clazz->descriptor);
            dvmSetFieldObject((Object*) clazz,
                              OFFSETOF_MEMBER(ClassObject, verifyErrorClass),
                              (Object*) dvmGetException(self)->clazz);
            clazz->status = CLASS_ERROR;
            goto bail_unlock;
        }

        clazz->status = CLASS_VERIFIED;
    }
    noverify:

    /*
     * We need to ensure that certain instructions, notably accesses to
     * volatile fields, are replaced before any code is executed.  This
     * must happen even if DEX optimizations are disabled.
     *
     * The only exception to this rule is that we don't want to do this
     * during dexopt.  We don't generally initialize classes at all
     * during dexopt, but because we're loading classes we need Class and
     * Object (and possibly some Throwable stuff if a class isn't found).
     * If optimizations are disabled, we don't want to output optimized
     * instructions at this time.  This means we will be executing <clinit>
     * code with un-fixed volatiles, but we're only doing it for a few
     * system classes, and dexopt runs single-threaded.
     */
    if (!IS_CLASS_FLAG_SET(clazz, CLASS_ISOPTIMIZED) && !gDvm.optimizing) {
        ALOGV("+++ late optimize on %s (pv=%d)",
              clazz->descriptor, IS_CLASS_FLAG_SET(clazz, CLASS_ISPREVERIFIED));
        bool essentialOnly = (gDvm.dexOptMode != OPTIMIZE_MODE_FULL);
        dvmOptimizeClass(clazz, essentialOnly);
        SET_CLASS_FLAG(clazz, CLASS_ISOPTIMIZED);
    }

    /* update instruction stream now that verification + optimization is done */
    dvmFlushBreakpoints(clazz);

    if (clazz->status == CLASS_INITIALIZED)
        goto bail_unlock;

    while (clazz->status == CLASS_INITIALIZING) {
        /* we caught somebody else in the act; was it us? */
        if (clazz->initThreadId == self->threadId) {
            //ALOGV("HEY: found a recursive <clinit>");
            goto bail_unlock;
        }

        if (dvmCheckException(self)) {
            ALOGW("GLITCH: exception pending at start of class init");
            dvmAbort();
        }

        /*
         * Wait for the other thread to finish initialization.  We pass
         * "false" for the "interruptShouldThrow" arg so it doesn't throw
         * an exception on interrupt.
         */
        dvmObjectWait(self, (Object*) clazz, 0, 0, false);

        /*
         * When we wake up, repeat the test for init-in-progress.  If there's
         * an exception pending (only possible if "interruptShouldThrow"
         * was set), bail out.
         */
        if (dvmCheckException(self)) {
            ALOGI("Class init of '%s' failing with wait() exception",
                  clazz->descriptor);
            /*
             * TODO: this is bogus, because it means the two threads have a
             * different idea of the class status.  We need to flag the
             * class as bad and ensure that the initializer thread respects
             * our notice.  If we get lucky and wake up after the class has
             * finished initialization but before being woken, we have to
             * swallow the exception, perhaps raising thread->interrupted
             * to preserve semantics.
             *
             * Since we're not currently allowing interrupts, this should
             * never happen and we don't need to fix this.
             */
            assert(false);
            dvmThrowExceptionInInitializerError();
            clazz->status = CLASS_ERROR;
            goto bail_unlock;
        }
        if (clazz->status == CLASS_INITIALIZING) {
            ALOGI("Waiting again for class init");
            continue;
        }
        assert(clazz->status == CLASS_INITIALIZED ||
               clazz->status == CLASS_ERROR);
        if (clazz->status == CLASS_ERROR) {
            /*
             * The caller wants an exception, but it was thrown in a
             * different thread.  Synthesize one here.
             */
            dvmThrowUnsatisfiedLinkError(
                    "(<clinit> failed, see exception in other thread)");
        }
        goto bail_unlock;
    }

    /* see if we failed previously */
    if (clazz->status == CLASS_ERROR) {
        // might be wise to unlock before throwing; depends on which class
        // it is that we have locked
        dvmUnlockObject(self, (Object*) clazz);
        throwEarlierClassFailure(clazz);
        return false;
    }

    if (gDvm.allocProf.enabled) {
        startWhen = dvmGetRelativeTimeNsec();
    }

    /*
     * We're ready to go, and have exclusive access to the class.
     *
     * Before we start initialization, we need to do one extra bit of
     * validation: make sure that the methods declared here match up
     * with our superclass and interfaces.  We know that the UTF-8
     * descriptors match, but classes from different class loaders can
     * have the same name.
     *
     * We do this now, rather than at load/link time, for the same reason
     * that we defer verification.
     *
     * It's unfortunate that we need to do this at all, but we risk
     * mixing reference types with identical names (see Dalvik test 068).
     */
    if (!validateSuperDescriptors(clazz)) {
        assert(dvmCheckException(self));
        clazz->status = CLASS_ERROR;
        goto bail_unlock;
    }

    /*
     * Let's initialize this thing.
     *
     * We unlock the object so that other threads can politely sleep on
     * our mutex with Object.wait(), instead of hanging or spinning trying
     * to grab our mutex.
     */
    assert(clazz->status < CLASS_INITIALIZING);

#if LOG_CLASS_LOADING
    // We started initializing.
    logClassLoad('+', clazz);
    initializedByUs = true;
#endif

    /* order matters here, esp. interaction with dvmIsClassInitializing */
    clazz->initThreadId = self->threadId;
    android_atomic_release_store(CLASS_INITIALIZING,
                                 (int32_t*)(void*)&clazz->status);
    dvmUnlockObject(self, (Object*) clazz);

    /* init our superclass */
    if (clazz->super != NULL && clazz->super->status != CLASS_INITIALIZED) {
        assert(!dvmIsInterfaceClass(clazz));
        if (!dvmInitClass(clazz->super)) {
            assert(dvmCheckException(self));
            clazz->status = CLASS_ERROR;
            /* wake up anybody who started waiting while we were unlocked */
            dvmLockObject(self, (Object*) clazz);
            goto bail_notify;
        }
    }

    /* Initialize any static fields whose values are
     * stored in the Dex file.  This should include all of the
     * simple "final static" fields, which are required to
     * be initialized first. (vmspec 2 sec 2.17.5 item 8)
     * More-complicated final static fields should be set
     * at the beginning of <clinit>;  all we can do is trust
     * that the compiler did the right thing.
     */
    initSFields(clazz);

    /* Execute any static initialization code.
     */
    method = dvmFindDirectMethodByDescriptor(clazz, "<clinit>", "()V");
    if (method == NULL) {
        LOGVV("No <clinit> found for %s", clazz->descriptor);
    } else {
        LOGVV("Invoking %s.<clinit>", clazz->descriptor);
        JValue unused;
        dvmCallMethod(self, method, NULL, &unused);
    }

    if (dvmCheckException(self)) {
        /*
         * We've had an exception thrown during static initialization.  We
         * need to throw an ExceptionInInitializerError, but we want to
         * tuck the original exception into the "cause" field.
         */
        ALOGW("Exception %s thrown while initializing %s",
              (dvmGetException(self)->clazz)->descriptor, clazz->descriptor);
        dvmThrowExceptionInInitializerError();
        //ALOGW("+++ replaced");

        dvmLockObject(self, (Object*) clazz);
        clazz->status = CLASS_ERROR;
    } else {
        /* success! */
        dvmLockObject(self, (Object*) clazz);
        clazz->status = CLASS_INITIALIZED;
        LOGVV("Initialized class: %s", clazz->descriptor);

        /*
         * Update alloc counters.  TODO: guard with mutex.
         */
        if (gDvm.allocProf.enabled && startWhen != 0) {
            u8 initDuration = dvmGetRelativeTimeNsec() - startWhen;
            gDvm.allocProf.classInitTime += initDuration;
            self->allocProf.classInitTime += initDuration;
            gDvm.allocProf.classInitCount++;
            self->allocProf.classInitCount++;
        }
    }

    bail_notify:
    /*
     * Notify anybody waiting on the object.
     */
    dvmObjectNotifyAll(self, (Object*) clazz);

    bail_unlock:

#if LOG_CLASS_LOADING
    if (initializedByUs) {
        // We finished initializing.
        logClassLoad('-', clazz);
    }
#endif

    dvmUnlockObject(self, (Object*) clazz);

    return (clazz->status != CLASS_ERROR);
}
