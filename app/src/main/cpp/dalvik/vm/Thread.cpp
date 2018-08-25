//
// Created by liu meng on 2018/8/21.
//
#include "Thread.h"
#include "Globals.h"
#include "atomic-arm.h"
#include "log.h"
#include "trace.h"
#define LOG_THREAD  LOGVV
Thread* dvmThreadSelf()
{
    return (Thread*) pthread_getspecific(gDvm.pthreadKeySelf);
}
static inline void unlockThreadSuspendCount()
{
    dvmUnlockMutex(&gDvm.threadSuspendCountLock);
}

static inline void lockThreadSuspendCount()
{
    /*
     * Don't try to change to VMWAIT here.  When we change back to RUNNING
     * we have to check for a pending suspend, which results in grabbing
     * this lock recursively.  Doesn't work with "fast" pthread mutexes.
     *
     * This lock is always held for very brief periods, so as long as
     * mutex ordering is respected we shouldn't stall.
     */
    dvmLockMutex(&gDvm.threadSuspendCountLock);
}

static bool fullSuspendCheck(Thread* self)
{
    assert(self != NULL);
    assert(self->suspendCount >= 0);

    /*
     * Grab gDvm.threadSuspendCountLock.  This gives us exclusive write
     * access to self->suspendCount.
     */
    lockThreadSuspendCount();   /* grab gDvm.threadSuspendCountLock */

    bool needSuspend = (self->suspendCount != 0);
    if (needSuspend) {
        LOG_THREAD("threadid=%d: self-suspending", self->threadId);
        ThreadStatus oldStatus =(ThreadStatus) self->status;      /* should be RUNNING */
        self->status = (volatile ThreadStatus)THREAD_SUSPENDED;

        while (self->suspendCount != 0) {
            /*
             * Wait for wakeup signal, releasing lock.  The act of releasing
             * and re-acquiring the lock provides the memory barriers we
             * need for correct behavior on SMP.
             */
            dvmWaitCond(&gDvm.threadSuspendCountCond,
                        &gDvm.threadSuspendCountLock);
        }
        assert(self->suspendCount == 0 && self->dbgSuspendCount == 0);
        self->status = (volatile ThreadStatus)oldStatus;
        LOG_THREAD("threadid=%d: self-reviving, status=%d",
                   self->threadId, self->status);
    }

    unlockThreadSuspendCount();

    return needSuspend;
}

ThreadStatus dvmChangeStatus(Thread* self, ThreadStatus newStatus)
{
     ThreadStatus oldStatus;

    if (self == NULL)
        self = dvmThreadSelf();

    LOGVV("threadid=%d: (status %d -> %d)",
          self->threadId, self->status, newStatus);

    oldStatus = (ThreadStatus)self->status;
    if (oldStatus == newStatus)
        return oldStatus;

    if (newStatus == THREAD_RUNNING) {
        /*
         * Change our status to THREAD_RUNNING.  The transition requires
         * that we check for pending suspension, because the VM considers
         * us to be "asleep" in all other states, and another thread could
         * be performing a GC now.
         *
         * The order of operations is very significant here.  One way to
         * do this wrong is:
         *
         *   GCing thread                   Our thread (in NATIVE)
         *   ------------                   ----------------------
         *                                  check suspend count (== 0)
         *   dvmSuspendAllThreads()
         *   grab suspend-count lock
         *   increment all suspend counts
         *   release suspend-count lock
         *   check thread state (== NATIVE)
         *   all are suspended, begin GC
         *                                  set state to RUNNING
         *                                  (continue executing)
         *
         * We can correct this by grabbing the suspend-count lock and
         * performing both of our operations (check suspend count, set
         * state) while holding it, now we need to grab a mutex on every
         * transition to RUNNING.
         *
         * What we do instead is change the order of operations so that
         * the transition to RUNNING happens first.  If we then detect
         * that the suspend count is nonzero, we switch to SUSPENDED.
         *
         * Appropriate compiler and memory barriers are required to ensure
         * that the operations are observed in the expected order.
         *
         * This does create a small window of opportunity where a GC in
         * progress could observe what appears to be a running thread (if
         * it happens to look between when we set to RUNNING and when we
         * switch to SUSPENDED).  At worst this only affects assertions
         * and thread logging.  (We could work around it with some sort
         * of intermediate "pre-running" state that is generally treated
         * as equivalent to running, but that doesn't seem worthwhile.)
         *
         * We can also solve this by combining the "status" and "suspend
         * count" fields into a single 32-bit value.  This trades the
         * store/load barrier on transition to RUNNING for an atomic RMW
         * op on all transitions and all suspend count updates (also, all
         * accesses to status or the thread count require bit-fiddling).
         * It also eliminates the brief transition through RUNNING when
         * the thread is supposed to be suspended.  This is possibly faster
         * on SMP and slightly more correct, but less convenient.
         */
        volatile void* raw = reinterpret_cast<volatile void*>(&self->status);
        volatile int32_t* addr = reinterpret_cast<volatile int32_t*>(raw);
        android_atomic_acquire_store(newStatus, addr);
        if (self->suspendCount != 0) {
            fullSuspendCheck(self);
        }
    } else {
        /*
         * Not changing to THREAD_RUNNING.  No additional work required.
         *
         * We use a releasing store to ensure that, if we were RUNNING,
         * any updates we previously made to objects on the managed heap
         * will be observed before the state change.
         */
        assert(newStatus != THREAD_SUSPENDED);
        volatile void* raw = reinterpret_cast<volatile void*>(&self->status);
        volatile int32_t* addr = reinterpret_cast<volatile int32_t*>(raw);
        android_atomic_release_store(newStatus, addr);
    }

    return oldStatus;
}
void dvmDumpThread(Thread* thread, bool isRunning)
{
    DebugOutputTarget target;

    dvmCreateLogOutputTarget(&target, ANDROID_LOG_INFO, LOG_TAG);
    dvmDumpThreadEx(&target, thread, isRunning);
}
