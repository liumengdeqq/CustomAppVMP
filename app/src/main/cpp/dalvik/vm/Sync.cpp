//
// Created by liu meng on 2018/8/21.
//
#include "Sync.h"
#include "Globals.h"
#include "log.h"
#include <stdlib.h>
#include "Dalvik.h"
#include "atomic-arm.h"
#include "Misc.h"
#include "Stack.h"
Monitor* dvmCreateMonitor(Object* obj)
{
    Monitor* mon;

    mon = (Monitor*) calloc(1, sizeof(Monitor));
    if (mon == NULL) {
        ALOGE("Unable to allocate monitor");
        dvmAbort();
    }
    mon->obj = obj;
    dvmInitMutex(&mon->lock);

    /* replace the head of the list with the new monitor */
    do {
        mon->next = gDvm.monitorList;
    } while (android_atomic_release_cas((int32_t)mon->next, (int32_t)mon,
                                        (int32_t*)(void*)&gDvm.monitorList) != 0);

    return mon;
}
static void lockMonitor(Thread* self, Monitor* mon)
{
    ThreadStatus oldStatus;
    u4 waitThreshold, samplePercent;
    u8 waitStart, waitEnd, waitMs;

    if (mon->owner == self) {
        mon->lockCount++;
        return;
    }
    if (dvmTryLockMutex(&mon->lock) != 0) {
        oldStatus = dvmChangeStatus(self, THREAD_MONITOR);
        waitThreshold = gDvm.lockProfThreshold;
        if (waitThreshold) {
            waitStart = dvmGetRelativeTimeUsec();
        }

        const Method* currentOwnerMethod = mon->ownerMethod;
        u4 currentOwnerPc = mon->ownerPc;

        dvmLockMutex(&mon->lock);
        if (waitThreshold) {
            waitEnd = dvmGetRelativeTimeUsec();
        }
        dvmChangeStatus(self, oldStatus);
        if (waitThreshold) {
            waitMs = (waitEnd - waitStart) / 1000;
            if (waitMs >= waitThreshold) {
                samplePercent = 100;
            } else {
                samplePercent = 100 * waitMs / waitThreshold;
            }
            if (samplePercent != 0 && ((u4)rand() % 100 < samplePercent)) {
                const char* currentOwnerFileName = "no_method";
                u4 currentOwnerLineNumber = 0;
                if (currentOwnerMethod != NULL) {
                    currentOwnerFileName = dvmGetMethodSourceFile(currentOwnerMethod);
                    if (currentOwnerFileName == NULL) {
                        currentOwnerFileName = "no_method_file";
                    }
                    currentOwnerLineNumber = dvmLineNumFromPC(currentOwnerMethod, currentOwnerPc);
                }
                logContentionEvent(self, waitMs, samplePercent,
                                   currentOwnerFileName, currentOwnerLineNumber);
            }
        }
    }
    mon->owner = self;
    assert(mon->lockCount == 0);

    // When debugging, save the current monitor holder for future
    // acquisition failures to use in sampled logging.
    if (gDvm.lockProfThreshold > 0) {
        mon->ownerMethod = NULL;
        mon->ownerPc = 0;
        if (self->interpSave.curFrame == NULL) {
            return;
        }
        const StackSaveArea* saveArea = SAVEAREA_FROM_FP(self->interpSave.curFrame);
        if (saveArea == NULL) {
            return;
        }
        mon->ownerMethod = saveArea->method;
        mon->ownerPc = (saveArea->xtra.currentPc - saveArea->method->insns);
    }
}

static void inflateMonitor(Thread *self, Object *obj)
{
    Monitor *mon;
    u4 thin;

    assert(self != NULL);
    assert(obj != NULL);
    assert(LW_SHAPE(obj->lock) == LW_SHAPE_THIN);
    assert(LW_LOCK_OWNER(obj->lock) == self->threadId);
    /* Allocate and acquire a new monitor. */
    mon = dvmCreateMonitor(obj);
    lockMonitor(self, mon);
    /* Propagate the lock state. */
    thin = obj->lock;
    mon->lockCount = LW_LOCK_COUNT(thin);
    thin &= LW_HASH_STATE_MASK << LW_HASH_STATE_SHIFT;
    thin |= (u4)mon | LW_SHAPE_FAT;
    /* Publish the updated lock word. */
    android_atomic_release_store(thin, (int32_t *)&obj->lock);
}


void dvmLockObject(Thread* self, Object *obj)
{
    volatile u4 *thinp;
    ThreadStatus oldStatus;
    struct timespec tm;
    long sleepDelayNs;
    long minSleepDelayNs = 1000000;  /* 1 millisecond */
    long maxSleepDelayNs = 1000000000;  /* 1 second */
    u4 thin, newThin, threadId;

    assert(self != NULL);
    assert(obj != NULL);
    threadId = self->threadId;
    thinp = &obj->lock;
    retry:
    thin = *thinp;
    if (LW_SHAPE(thin) == LW_SHAPE_THIN) {
        /*
         * The lock is a thin lock.  The owner field is used to
         * determine the acquire method, ordered by cost.
         */
        if (LW_LOCK_OWNER(thin) == threadId) {
            /*
             * The calling thread owns the lock.  Increment the
             * value of the recursion count field.
             */
            obj->lock += 1 << LW_LOCK_COUNT_SHIFT;
            if (LW_LOCK_COUNT(obj->lock) == LW_LOCK_COUNT_MASK) {
                /*
                 * The reacquisition limit has been reached.  Inflate
                 * the lock so the next acquire will not overflow the
                 * recursion count field.
                 */
                inflateMonitor(self, obj);
            }
        } else if (LW_LOCK_OWNER(thin) == 0) {
            /*
             * The lock is unowned.  Install the thread id of the
             * calling thread into the owner field.  This is the
             * common case.  In performance critical code the JIT
             * will have tried this before calling out to the VM.
             */
            newThin = thin | (threadId << LW_LOCK_OWNER_SHIFT);
            if (android_atomic_acquire_cas(thin, newThin,
                                           (int32_t*)thinp) != 0) {
                /*
                 * The acquire failed.  Try again.
                 */
                goto retry;
            }
        } else {
            ALOGV("(%d) spin on lock %p: %#x (%#x) %#x",
                  threadId, &obj->lock, 0, *thinp, thin);
            /*
             * The lock is owned by another thread.  Notify the VM
             * that we are about to wait.
             */
            oldStatus = dvmChangeStatus(self, THREAD_MONITOR);
            /*
             * Spin until the thin lock is released or inflated.
             */
            sleepDelayNs = 0;
            for (;;) {
                thin = *thinp;
                /*
                 * Check the shape of the lock word.  Another thread
                 * may have inflated the lock while we were waiting.
                 */
                if (LW_SHAPE(thin) == LW_SHAPE_THIN) {
                    if (LW_LOCK_OWNER(thin) == 0) {
                        /*
                         * The lock has been released.  Install the
                         * thread id of the calling thread into the
                         * owner field.
                         */
                        newThin = thin | (threadId << LW_LOCK_OWNER_SHIFT);
                        if (android_atomic_acquire_cas(thin, newThin,
                                                       (int32_t *)thinp) == 0) {
                            /*
                             * The acquire succeed.  Break out of the
                             * loop and proceed to inflate the lock.
                             */
                            break;
                        }
                    } else {
                        /*
                         * The lock has not been released.  Yield so
                         * the owning thread can run.
                         */
                        if (sleepDelayNs == 0) {
                            sched_yield();
                            sleepDelayNs = minSleepDelayNs;
                        } else {
                            tm.tv_sec = 0;
                            tm.tv_nsec = sleepDelayNs;
                            nanosleep(&tm, NULL);
                            /*
                             * Prepare the next delay value.  Wrap to
                             * avoid once a second polls for eternity.
                             */
                            if (sleepDelayNs < maxSleepDelayNs / 2) {
                                sleepDelayNs *= 2;
                            } else {
                                sleepDelayNs = minSleepDelayNs;
                            }
                        }
                    }
                } else {
                    /*
                     * The thin lock was inflated by another thread.
                     * Let the VM know we are no longer waiting and
                     * try again.
                     */
                    ALOGV("(%d) lock %p surprise-fattened",
                          threadId, &obj->lock);
                    dvmChangeStatus(self, oldStatus);
                    goto retry;
                }
            }
            ALOGV("(%d) spin on lock done %p: %#x (%#x) %#x",
                  threadId, &obj->lock, 0, *thinp, thin);
            /*
             * We have acquired the thin lock.  Let the VM know that
             * we are no longer waiting.
             */
            dvmChangeStatus(self, oldStatus);
            /*
             * Fatten the lock.
             */
            inflateMonitor(self, obj);
            ALOGV("(%d) lock %p fattened", threadId, &obj->lock);
        }
    } else {
        /*
         * The lock is a fat lock.
         */
        assert(LW_MONITOR(obj->lock) != NULL);
        lockMonitor(self, LW_MONITOR(obj->lock));
    }
}
