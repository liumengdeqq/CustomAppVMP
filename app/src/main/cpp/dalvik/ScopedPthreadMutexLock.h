#ifndef SCOPED_PTHREAD_MUTEX_LOCK_H_included
#define SCOPED_PTHREAD_MUTEX_LOCK_H_included

#include <pthread.h>

/**
 * Locks and unlocks a pthread_mutex_t as it goes in and out of scope.
 */
class ScopedPthreadMutexLock {
public:
    explicit ScopedPthreadMutexLock(pthread_mutex_t* mutex) : mMutexPtr(mutex) {
        pthread_mutex_lock(mMutexPtr);
    }

    ~ScopedPthreadMutexLock() {
        pthread_mutex_unlock(mMutexPtr);
    }

private:
    pthread_mutex_t* mMutexPtr;

    // Disallow copy and assignment.
    ScopedPthreadMutexLock(const ScopedPthreadMutexLock&);
    void operator=(const ScopedPthreadMutexLock&);
};

#endif  // SCOPED_PTHREAD_MUTEX_LOCK_H_included
