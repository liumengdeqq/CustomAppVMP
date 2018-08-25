//
// Created by liu meng on 2018/8/20.
//
#include "Class.h"
#include "Globals.h"
#include "log.h"
#include "Thread.h"
#include "Sync.h"
#include "Exception.h"
#include "ObjectInlines.h"
#include "Misc.h"
#include "Interp.h"
#include "Reflect.h"
#include "Annotation.h"
static void throwEarlierClassFailure(ClassObject* clazz);
static void throwEarlierClassFailure(ClassObject* clazz)
{
    ALOGI("Rejecting re-init on previously-failed class %s v=%p",
          clazz->descriptor, clazz->verifyErrorClass);

    if (clazz->verifyErrorClass == NULL) {
        dvmThrowNoClassDefFoundError(clazz->descriptor);
    } else {
        dvmThrowExceptionWithClassMessage(clazz->verifyErrorClass,
                                          clazz->descriptor);
    }
}
void dvmAddInitiatingLoader(ClassObject* clazz, Object* loader)
{
    if (loader != clazz->classLoader) {
        assert(loader != NULL);

        LOGVV("Adding %p to '%s' init list", loader, clazz->descriptor);
        dvmHashTableLock(gDvm.loadedClasses);

        /*
         * Make sure nobody snuck in.  The penalty for adding twice is
         * pretty minor, and probably outweighs the O(n^2) hit for
         * checking before every add, so we may not want to do this.
         */
        //if (dvmLoaderInInitiatingList(clazz, loader)) {
        //    ALOGW("WOW: simultaneous add of initiating class loader");
        //    goto bail_unlock;
        //}

        /*
         * The list never shrinks, so we just keep a count of the
         * number of elements in it, and reallocate the buffer when
         * we run off the end.
         *
         * The pointer is initially NULL, so we *do* want to call realloc
         * when count==0.
         */
        InitiatingLoaderList *loaderList = dvmGetInitiatingLoaderList(clazz);
        if ((loaderList->initiatingLoaderCount & (kInitLoaderInc-1)) == 0) {
            Object** newList;

            newList = (Object**) realloc(loaderList->initiatingLoaders,
                                         (loaderList->initiatingLoaderCount + kInitLoaderInc)
                                         * sizeof(Object*));
            if (newList == NULL) {
                /* this is mainly a cache, so it's not the EotW */
                assert(false);
                goto bail_unlock;
            }
            loaderList->initiatingLoaders = newList;

            //ALOGI("Expanded init list to %d (%s)",
            //    loaderList->initiatingLoaderCount+kInitLoaderInc,
            //    clazz->descriptor);
        }
        loaderList->initiatingLoaders[loaderList->initiatingLoaderCount++] =
                loader;

        bail_unlock:
        dvmHashTableUnlock(gDvm.loadedClasses);
    }
}
static ClassObject* findClassFromLoaderNoInit(const char* descriptor,
                                              Object* loader)
{
    //ALOGI("##### findClassFromLoaderNoInit (%s,%p)",
    //        descriptor, loader);

    Thread* self = dvmThreadSelf();

    assert(loader != NULL);

    /*
     * Do we already have it?
     *
     * The class loader code does the "is it already loaded" check as
     * well.  However, this call is much faster than calling through
     * interpreted code.  Doing this does mean that in the common case
     * (365 out of 420 calls booting the sim) we're doing the
     * lookup-by-descriptor twice.  It appears this is still a win, so
     * I'm keeping it in.
     */
    ClassObject* clazz = dvmLookupClass(descriptor, loader, false);
    if (clazz != NULL) {
        LOGVV("Already loaded: %s %p", descriptor, loader);
        return clazz;
    } else {
        LOGVV("Not already loaded: %s %p", descriptor, loader);
    }

    char* dotName = NULL;
    StringObject* nameObj = NULL;

    /* convert "Landroid/debug/Stuff;" to "android.debug.Stuff" */
    dotName = dvmDescriptorToDot(descriptor);
    if (dotName == NULL) {
        dvmThrowOutOfMemoryError(NULL);
        return NULL;
    }
    nameObj = dvmCreateStringFromCstr(dotName);
    if (nameObj == NULL) {
        assert(dvmCheckException(self));
        goto bail;
    }

    dvmMethodTraceClassPrepBegin();

    /*
     * Invoke loadClass().  This will probably result in a couple of
     * exceptions being thrown, because the ClassLoader.loadClass()
     * implementation eventually calls VMClassLoader.loadClass to see if
     * the bootstrap class loader can find it before doing its own load.
     */
    LOGVV("--- Invoking loadClass(%s, %p)", dotName, loader);
    {
        const Method* loadClass =
                loader->clazz->vtable[gDvm.voffJavaLangClassLoader_loadClass];
        JValue result;
        dvmCallMethod(self, loadClass, loader, &result, nameObj);
        clazz = (ClassObject*) result.l;

        dvmMethodTraceClassPrepEnd();
        Object* excep = dvmGetException(self);
        if (excep != NULL) {
#if DVM_SHOW_EXCEPTION >= 2
            ALOGD("NOTE: loadClass '%s' %p threw exception %s",
                 dotName, loader, excep->clazz->descriptor);
#endif
            dvmAddTrackedAlloc(excep, self);
            dvmClearException(self);
            dvmThrowChainedNoClassDefFoundError(descriptor, excep);
            dvmReleaseTrackedAlloc(excep, self);
            clazz = NULL;
            goto bail;
        } else if (clazz == NULL) {
            ALOGW("ClassLoader returned NULL w/o exception pending");
            dvmThrowNullPointerException("ClassLoader returned null");
            goto bail;
        }
    }

    /* not adding clazz to tracked-alloc list, because it's a ClassObject */

    dvmAddInitiatingLoader(clazz, loader);

    LOGVV("--- Successfully loaded %s %p (thisldr=%p clazz=%p)",
          descriptor, clazz->classLoader, loader, clazz);

    bail:
    dvmReleaseTrackedAlloc((Object*)nameObj, NULL);
    free(dotName);
    return clazz;
}
static ClassObject* findClassNoInit(const char* descriptor, Object* loader,
                                    DvmDex* pDvmDex)
{
    Thread* self = dvmThreadSelf();
    ClassObject* clazz;
    bool profilerNotified = false;

    if (loader != NULL) {
        LOGVV("#### findClassNoInit(%s,%p,%p)", descriptor, loader,
              pDvmDex->pDexFile);
    }

    /*
     * We don't expect an exception to be raised at this point.  The
     * exception handling code is good about managing this.  This *can*
     * happen if a JNI lookup fails and the JNI code doesn't do any
     * error checking before doing another class lookup, so we may just
     * want to clear this and restore it on exit.  If we don't, some kinds
     * of failures can't be detected without rearranging other stuff.
     *
     * Most often when we hit this situation it means that something is
     * broken in the VM or in JNI code, so I'm keeping it in place (and
     * making it an informative abort rather than an assert).
     */
    if (dvmCheckException(self)) {
        ALOGE("Class lookup %s attempted with exception pending", descriptor);
        ALOGW("Pending exception is:");
        dvmLogExceptionStackTrace();
        dvmDumpAllThreads(false);
        dvmAbort();
    }

    clazz = dvmLookupClass(descriptor, loader, true);
    if (clazz == NULL) {
        const DexClassDef* pClassDef;

        dvmMethodTraceClassPrepBegin();
        profilerNotified = true;

#if LOG_CLASS_LOADING
        u8 startTime = dvmGetThreadCpuTimeNsec();
#endif

        if (pDvmDex == NULL) {
            assert(loader == NULL);     /* shouldn't be here otherwise */
            pDvmDex = searchBootPathForClass(descriptor, &pClassDef);
        } else {
            pClassDef = dexFindClass(pDvmDex->pDexFile, descriptor);
        }

        if (pDvmDex == NULL || pClassDef == NULL) {
            if (gDvm.noClassDefFoundErrorObj != NULL) {
                /* usual case -- use prefabricated object */
                dvmSetException(self, gDvm.noClassDefFoundErrorObj);
            } else {
                /* dexopt case -- can't guarantee prefab (core.jar) */
                dvmThrowNoClassDefFoundError(descriptor);
            }
            goto bail;
        }

        /* found a match, try to load it */
        clazz = loadClassFromDex(pDvmDex, pClassDef, loader);
        if (dvmCheckException(self)) {
            /* class was found but had issues */
            if (clazz != NULL) {
                dvmFreeClassInnards(clazz);
                dvmReleaseTrackedAlloc((Object*) clazz, NULL);
            }
            goto bail;
        }

        /*
         * Lock the class while we link it so other threads must wait for us
         * to finish.  Set the "initThreadId" so we can identify recursive
         * invocation.  (Note all accesses to initThreadId here are
         * guarded by the class object's lock.)
         */
        dvmLockObject(self, (Object*) clazz);
        clazz->initThreadId = self->threadId;

        /*
         * Add to hash table so lookups succeed.
         *
         * [Are circular references possible when linking a class?]
         */
        assert(clazz->classLoader == loader);
        if (!dvmAddClassToHash(clazz)) {
            /*
             * Another thread must have loaded the class after we
             * started but before we finished.  Discard what we've
             * done and leave some hints for the GC.
             *
             * (Yes, this happens.)
             */
            //ALOGW("WOW: somebody loaded %s simultaneously", descriptor);
            clazz->initThreadId = 0;
            dvmUnlockObject(self, (Object*) clazz);

            /* Let the GC free the class.
             */
            dvmFreeClassInnards(clazz);
            dvmReleaseTrackedAlloc((Object*) clazz, NULL);

            /* Grab the winning class.
             */
            clazz = dvmLookupClass(descriptor, loader, true);
            assert(clazz != NULL);
            goto got_class;
        }
        dvmReleaseTrackedAlloc((Object*) clazz, NULL);

#if LOG_CLASS_LOADING
        logClassLoadWithTime('>', clazz, startTime);
#endif
        /*
         * Prepare and resolve.
         */
        if (!dvmLinkClass(clazz)) {
            assert(dvmCheckException(self));

            /* Make note of the error and clean up the class.
             */
            removeClassFromHash(clazz);
            clazz->status = CLASS_ERROR;
            dvmFreeClassInnards(clazz);

            /* Let any waiters know.
             */
            clazz->initThreadId = 0;
            dvmObjectNotifyAll(self, (Object*) clazz);
            dvmUnlockObject(self, (Object*) clazz);

#if LOG_CLASS_LOADING
            ALOG(LOG_INFO, "DVMLINK FAILED FOR CLASS ", "%s in %s",
                clazz->descriptor, get_process_name());

            /*
             * TODO: It would probably be better to use a new type code here (instead of '<') to
             * indicate the failure.  This change would require a matching change in the parser
             * and analysis code in frameworks/base/tools/preload.
             */
            logClassLoad('<', clazz);
#endif
            clazz = NULL;
            if (gDvm.optimizing) {
                /* happens with "external" libs */
                ALOGV("Link of class '%s' failed", descriptor);
            } else {
                ALOGW("Link of class '%s' failed", descriptor);
            }
            goto bail;
        }
        dvmObjectNotifyAll(self, (Object*) clazz);
        dvmUnlockObject(self, (Object*) clazz);

        /*
         * Add class stats to global counters.
         *
         * TODO: these should probably be atomic ops.
         */
        gDvm.numLoadedClasses++;
        gDvm.numDeclaredMethods +=
                clazz->virtualMethodCount + clazz->directMethodCount;
        gDvm.numDeclaredInstFields += clazz->ifieldCount;
        gDvm.numDeclaredStaticFields += clazz->sfieldCount;

        /*
         * Cache pointers to basic classes.  We want to use these in
         * various places, and it's easiest to initialize them on first
         * use rather than trying to force them to initialize (startup
         * ordering makes it weird).
         */
        if (gDvm.classJavaLangObject == NULL &&
            strcmp(descriptor, "Ljava/lang/Object;") == 0)
        {
            /* It should be impossible to get here with anything
             * but the bootclasspath loader.
             */
            assert(loader == NULL);
            gDvm.classJavaLangObject = clazz;
        }

#if LOG_CLASS_LOADING
        logClassLoad('<', clazz);
#endif

    } else {
        got_class:
        if (!dvmIsClassLinked(clazz) && clazz->status != CLASS_ERROR) {
            /*
             * We can race with other threads for class linking.  We should
             * never get here recursively; doing so indicates that two
             * classes have circular dependencies.
             *
             * One exception: we force discovery of java.lang.Class in
             * dvmLinkClass(), and Class has Object as its superclass.  So
             * if the first thing we ever load is Object, we will init
             * Object->Class->Object.  The easiest way to avoid this is to
             * ensure that Object is never the first thing we look up, so
             * we get Foo->Class->Object instead.
             */
            dvmLockObject(self, (Object*) clazz);
            if (!dvmIsClassLinked(clazz) &&
                clazz->initThreadId == self->threadId)
            {
                ALOGW("Recursive link on class %s", clazz->descriptor);
                dvmUnlockObject(self, (Object*) clazz);
                dvmThrowClassCircularityError(clazz->descriptor);
                clazz = NULL;
                goto bail;
            }
            //ALOGI("WAITING  for '%s' (owner=%d)",
            //    clazz->descriptor, clazz->initThreadId);
            while (!dvmIsClassLinked(clazz) && clazz->status != CLASS_ERROR) {
                dvmObjectWait(self, (Object*) clazz, 0, 0, false);
            }
            dvmUnlockObject(self, (Object*) clazz);
        }
        if (clazz->status == CLASS_ERROR) {
            /*
             * Somebody else tried to load this and failed.  We need to raise
             * an exception and report failure.
             */
            throwEarlierClassFailure(clazz);
            clazz = NULL;
            goto bail;
        }
    }

    /* check some invariants */
    assert(dvmIsClassLinked(clazz));
    assert(gDvm.classJavaLangClass != NULL);
    assert(clazz->clazz == gDvm.classJavaLangClass);
    assert(dvmIsClassObject(clazz));
    assert(clazz == gDvm.classJavaLangObject || clazz->super != NULL);
    if (!dvmIsInterfaceClass(clazz)) {
        //ALOGI("class=%s vtableCount=%d, virtualMeth=%d",
        //    clazz->descriptor, clazz->vtableCount,
        //    clazz->virtualMethodCount);
        assert(clazz->vtableCount >= clazz->virtualMethodCount);
    }

    bail:
    if (profilerNotified)
        dvmMethodTraceClassPrepEnd();
    assert(clazz != NULL || dvmCheckException(self));
    return clazz;
}

ClassObject* dvmFindSystemClassNoInit(const char* descriptor)
{
    return findClassNoInit(descriptor, NULL, NULL);
}

ClassObject* dvmFindClassNoInit(const char* descriptor,
                                Object* loader)
{
    assert(descriptor != NULL);
    //assert(loader != NULL);

    LOGVV("FindClassNoInit '%s' %p", descriptor, loader);

    if (*descriptor == '[') {
        /*
         * Array class.  Find in table, generate if not found.
         */
        return dvmFindArrayClass(descriptor, loader);
    } else {
        /*
         * Regular class.  Find in table, load if not found.
         */
        if (loader != NULL) {
            return findClassFromLoaderNoInit(descriptor, loader);
        } else {
            return dvmFindSystemClassNoInit(descriptor);
        }
    }
}

static bool compareDescriptorClasses(const char* descriptor,
                                     const ClassObject* clazz1, const ClassObject* clazz2)
{
    ClassObject* result1;
    ClassObject* result2;

    /*
     * Do the first lookup by name.
     */
    result1 = dvmFindClassNoInit(descriptor, clazz1->classLoader);

    /*
     * We can skip a second lookup by name if the second class loader is
     * in the initiating loader list of the class object we retrieved.
     * (This means that somebody already did a lookup of this class through
     * the second loader, and it resolved to the same class.)  If it's not
     * there, we may simply not have had an opportunity to add it yet, so
     * we do the full lookup.
     *
     * The initiating loader test should catch the majority of cases
     * (in particular, the zillions of references to String/Object).
     *
     * Unfortunately we're still stuck grabbing a mutex to do the lookup.
     *
     * For this to work, the superclass/interface should be the first
     * argument, so that way if it's from the bootstrap loader this test
     * will work.  (The bootstrap loader, by definition, never shows up
     * as the initiating loader of a class defined by some other loader.)
     */
    dvmHashTableLock(gDvm.loadedClasses);
    bool isInit = dvmLoaderInInitiatingList(result1, clazz2->classLoader);
    dvmHashTableUnlock(gDvm.loadedClasses);

    if (isInit) {
        //printf("%s(obj=%p) / %s(cl=%p): initiating\n",
        //    result1->descriptor, result1,
        //    clazz2->descriptor, clazz2->classLoader);
        return true;
    } else {
        //printf("%s(obj=%p) / %s(cl=%p): RAW\n",
        //    result1->descriptor, result1,
        //    clazz2->descriptor, clazz2->classLoader);
        result2 = dvmFindClassNoInit(descriptor, clazz2->classLoader);
    }

    if (result1 == NULL || result2 == NULL) {
        dvmClearException(dvmThreadSelf());
        if (result1 == result2) {
            /*
             * Neither class loader could find this class.  Apparently it
             * doesn't exist.
             *
             * We can either throw some sort of exception now, or just
             * assume that it'll fail later when something actually tries
             * to use the class.  For strict handling we should throw now,
             * because a "tricky" class loader could start returning
             * something later, and a pair of "tricky" loaders could set
             * us up for confusion.
             *
             * I'm not sure if we're allowed to complain about nonexistent
             * classes in method signatures during class init, so for now
             * this will just return "true" and let nature take its course.
             */
            return true;
        } else {
            /* only one was found, so clearly they're not the same */
            return false;
        }
    }

    return result1 == result2;
}
static bool checkMethodDescriptorClasses(const Method* meth,
                                         const ClassObject* clazz1, const ClassObject* clazz2)
{
    DexParameterIterator iterator;
    const char* descriptor;

    /* walk through the list of parameters */
    dexParameterIteratorInit(&iterator, &meth->prototype);
    while (true) {
        descriptor = dexParameterIteratorNextDescriptor(&iterator);

        if (descriptor == NULL)
            break;

        if (descriptor[0] == 'L' || descriptor[0] == '[') {
            /* non-primitive type */
            if (!compareDescriptorClasses(descriptor, clazz1, clazz2))
                return false;
        }
    }

    /* check the return type */
    descriptor = dexProtoGetReturnType(&meth->prototype);
    if (descriptor[0] == 'L' || descriptor[0] == '[') {
        if (!compareDescriptorClasses(descriptor, clazz1, clazz2))
            return false;
    }
    return true;
}

static bool validateSuperDescriptors(const ClassObject* clazz)
{
    int i;

    if (dvmIsInterfaceClass(clazz))
        return true;

    /*
     * Start with the superclass-declared methods.
     */
    if (clazz->super != NULL &&
        clazz->classLoader != clazz->super->classLoader)
    {
        /*
         * Walk through every overridden method and compare resolved
         * descriptor components.  We pull the Method structs out of
         * the vtable.  It doesn't matter whether we get the struct from
         * the parent or child, since we just need the UTF-8 descriptor,
         * which must match.
         *
         * We need to do this even for the stuff inherited from Object,
         * because it's possible that the new class loader has redefined
         * a basic class like String.
         *
         * We don't need to check stuff defined in a superclass because
         * it was checked when the superclass was loaded.
         */
        const Method* meth;

        //printf("Checking %s %p vs %s %p\n",
        //    clazz->descriptor, clazz->classLoader,
        //    clazz->super->descriptor, clazz->super->classLoader);
        for (i = clazz->super->vtableCount - 1; i >= 0; i--) {
            meth = clazz->vtable[i];
            if (meth != clazz->super->vtable[i] &&
                !checkMethodDescriptorClasses(meth, clazz->super, clazz))
            {
                ALOGW("Method mismatch: %s in %s (cl=%p) and super %s (cl=%p)",
                      meth->name, clazz->descriptor, clazz->classLoader,
                      clazz->super->descriptor, clazz->super->classLoader);
                dvmThrowLinkageError(
                        "Classes resolve differently in superclass");
                return false;
            }
        }
    }

    /*
     * Check the methods defined by this class against the interfaces it
     * implements.  If we inherited the implementation from a superclass,
     * we have to check it against the superclass (which might be in a
     * different class loader).  If the superclass also implements the
     * interface, we could skip the check since by definition it was
     * performed when the class was loaded.
     */
    for (i = 0; i < clazz->iftableCount; i++) {
        const InterfaceEntry* iftable = &clazz->iftable[i];

        if (clazz->classLoader != iftable->clazz->classLoader) {
            const ClassObject* iface = iftable->clazz;
            int j;

            for (j = 0; j < iface->virtualMethodCount; j++) {
                const Method* meth;
                int vtableIndex;

                vtableIndex = iftable->methodIndexArray[j];
                meth = clazz->vtable[vtableIndex];

                if (!checkMethodDescriptorClasses(meth, iface, meth->clazz)) {
                    ALOGW("Method mismatch: %s in %s (cl=%p) and "
                                  "iface %s (cl=%p)",
                          meth->name, clazz->descriptor, clazz->classLoader,
                          iface->descriptor, iface->classLoader);
                    dvmThrowLinkageError(
                            "Classes resolve differently in interface");
                    return false;
                }
            }
        }
    }

    return true;
}
static void initSFields(ClassObject* clazz)
{
    Thread* self = dvmThreadSelf(); /* for dvmReleaseTrackedAlloc() */
    DexFile* pDexFile;
    const DexClassDef* pClassDef;
    const DexEncodedArray* pValueList;
    EncodedArrayIterator iterator;
    int i;

    if (clazz->sfieldCount == 0) {
        return;
    }
    if (clazz->pDvmDex == NULL) {
        /* generated class; any static fields should already be set up */
        ALOGV("Not initializing static fields in %s", clazz->descriptor);
        return;
    }
    pDexFile = clazz->pDvmDex->pDexFile;

    pClassDef = dexFindClass(pDexFile, clazz->descriptor);
    assert(pClassDef != NULL);

    pValueList = dexGetStaticValuesList(pDexFile, pClassDef);
    if (pValueList == NULL) {
        return;
    }

    dvmEncodedArrayIteratorInitialize(&iterator, pValueList, clazz);

    /*
     * Iterate over the initial values array, setting the corresponding
     * static field for each array element.
     */

    for (i = 0; dvmEncodedArrayIteratorHasNext(&iterator); i++) {
        AnnotationValue value;
        bool parsed = dvmEncodedArrayIteratorGetNext(&iterator, &value);
        StaticField* sfield = &clazz->sfields[i];
        const char* descriptor = sfield->signature;
        bool isObj = false;

        if (! parsed) {
            /*
             * TODO: Eventually verification should attempt to ensure
             * that this can't happen at least due to a data integrity
             * problem.
             */
            ALOGE("Static initializer parse failed for %s at index %d",
                  clazz->descriptor, i);
            dvmAbort();
        }

        /* Verify that the value we got was of a valid type. */

        switch (descriptor[0]) {
            case 'Z': parsed = (value.type == kDexAnnotationBoolean); break;
            case 'B': parsed = (value.type == kDexAnnotationByte);    break;
            case 'C': parsed = (value.type == kDexAnnotationChar);    break;
            case 'S': parsed = (value.type == kDexAnnotationShort);   break;
            case 'I': parsed = (value.type == kDexAnnotationInt);     break;
            case 'J': parsed = (value.type == kDexAnnotationLong);    break;
            case 'F': parsed = (value.type == kDexAnnotationFloat);   break;
            case 'D': parsed = (value.type == kDexAnnotationDouble);  break;
            case '[': parsed = (value.type == kDexAnnotationNull);    break;
            case 'L': {
                switch (value.type) {
                    case kDexAnnotationNull: {
                        /* No need for further tests. */
                        break;
                    }
                    case kDexAnnotationString: {
                        parsed =
                                (strcmp(descriptor, "Ljava/lang/String;") == 0);
                        isObj = true;
                        break;
                    }
                    case kDexAnnotationType: {
                        parsed =
                                (strcmp(descriptor, "Ljava/lang/Class;") == 0);
                        isObj = true;
                        break;
                    }
                    default: {
                        parsed = false;
                        break;
                    }
                }
                break;
            }
            default: {
                parsed = false;
                break;
            }
        }

        if (parsed) {
            /*
             * All's well, so store the value.
             */
            if (isObj) {
                dvmSetStaticFieldObject(sfield, (Object*)value.value.l);
                dvmReleaseTrackedAlloc((Object*)value.value.l, self);
            } else {
                /*
                 * Note: This always stores the full width of a
                 * JValue, even though most of the time only the first
                 * word is needed.
                 */
                sfield->value = value.value;
            }
        } else {
            /*
             * Something up above had a problem. TODO: See comment
             * above the switch about verfication.
             */
            ALOGE("Bogus static initialization: value type %d in field type "
                          "%s for %s at index %d",
                  value.type, descriptor, clazz->descriptor, i);
            dvmAbort();
        }
    }
}

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
ClassObject* dvmFindPrimitiveClass(char type)
{
    PrimitiveType primitiveType = dexGetPrimitiveTypeFromDescriptorChar(type);

    switch (primitiveType) {
        case PRIM_VOID:    return gDvm.typeVoid;
        case PRIM_BOOLEAN: return gDvm.typeBoolean;
        case PRIM_BYTE:    return gDvm.typeByte;
        case PRIM_SHORT:   return gDvm.typeShort;
        case PRIM_CHAR:    return gDvm.typeChar;
        case PRIM_INT:     return gDvm.typeInt;
        case PRIM_LONG:    return gDvm.typeLong;
        case PRIM_FLOAT:   return gDvm.typeFloat;
        case PRIM_DOUBLE:  return gDvm.typeDouble;
        default: {
            ALOGW("Unknown primitive type '%c'", type);
            return NULL;
        }
    }
}