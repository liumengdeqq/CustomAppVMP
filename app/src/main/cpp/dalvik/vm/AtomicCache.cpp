//
// Created by liu meng on 2018/8/25.
//
#include "AtomicCache.h"
void dvmUpdateAtomicCache(u4 key1, u4 key2, u4 value, AtomicCacheEntry* pEntry,
                          u4 firstVersion
#if CALC_CACHE_STATS > 0
        , AtomicCache* pCache
#endif
)
{
    /*
     * The fields don't match, so we want to update them.  There is a risk
     * that another thread is also trying to update them, so we grab an
     * ownership flag to lock out other threads.
     *
     * If the lock flag was already set in "firstVersion", somebody else
     * was in mid-update, and we don't want to continue here.  (This means
     * that using "firstVersion" as the "before" argument to the CAS would
     * succeed when it shouldn't and vice-versa -- we could also just pass
     * in (firstVersion & ~ATOMIC_LOCK_FLAG) as the first argument.)
     *
     * NOTE: we don't deal with the situation where we overflow the version
     * counter and trample the ATOMIC_LOCK_FLAG (at 2^31).  Probably not
     * a real concern.
     */
    if ((firstVersion & ATOMIC_LOCK_FLAG) != 0 ||
        android_atomic_release_cas(
                firstVersion, firstVersion | ATOMIC_LOCK_FLAG,
                (volatile s4*) &pEntry->version) != 0)
    {
        /*
         * We couldn't get the write lock.  Return without updating the table.
         */
#if CALC_CACHE_STATS > 0
        pCache->fail++;
#endif
        return;
    }

    /* must be even-valued on entry */
    assert((firstVersion & 0x01) == 0);

#if CALC_CACHE_STATS > 0
    /* for stats, assume a key value of zero indicates an empty entry */
    if (pEntry->key1 == 0)
        pCache->fills++;
    else
        pCache->misses++;
#endif

    /*
     * We have the write lock, but somebody could be reading this entry
     * while we work.  We use memory barriers to ensure that the state
     * is always consistent when the version number is even.
     */
    u4 newVersion = (firstVersion | ATOMIC_LOCK_FLAG) + 1;
    assert((newVersion & 0x01) == 1);

    pEntry->version = newVersion;

    android_atomic_release_store(key1, (int32_t*) &pEntry->key1);
    pEntry->key2 = key2;
    pEntry->value = value;

    newVersion++;
    android_atomic_release_store(newVersion, (int32_t*) &pEntry->version);

    /*
     * Clear the lock flag.  Nobody else should have been able to modify
     * pEntry->version, so if this fails the world is broken.
     */
    assert(newVersion == ((firstVersion + 2) | ATOMIC_LOCK_FLAG));
    if (android_atomic_release_cas(
            newVersion, newVersion & ~ATOMIC_LOCK_FLAG,
            (volatile s4*) &pEntry->version) != 0)
    {
        //ALOGE("unable to reset the instanceof cache ownership");
        dvmAbort();
    }
}
