//
// Created by liu meng on 2018/9/2.
//

#ifndef CUSTOMAPPVMP_ATOMICCACHE_H
#define CUSTOMAPPVMP_ATOMICCACHE_H

#include "stdafx.h"
#include "Interp.h"
struct AtomicCacheEntry {
    u4          key1;
    u4          key2;
    u4          value;
    volatile u4 version;    /* version and lock flag */
};
struct AtomicCache {
    AtomicCacheEntry*   entries;        /* array of entries */
    int         numEntries;             /* #of entries, must be power of 2 */

    void*       entryAlloc;             /* memory allocated for entries */

    /* cache stats; note we don't guarantee atomic increments for these */
    int         trivial;                /* cache access not required */
    int         fail;                   /* contention failure */
    int         hits;                   /* found entry in cache */
    int         misses;                 /* entry was for other keys */
    int         fills;                  /* entry was empty */
};
#define BOOL_TO_INT(x)  (x)
//#define BOOL_TO_INT(x)  ((x) ? 1 : 0)

#define CPU_CACHE_WIDTH         32
#define CPU_CACHE_WIDTH_1       (CPU_CACHE_WIDTH-1)

#define ATOMIC_LOCK_FLAG        (1 << 31)

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
        dvmAbortHook();
    }
}

#define CALC_CACHE_STATS 0
#if CALC_CACHE_STATS > 0
# define CACHE_XARG(_value) ,_value
#else
# define CACHE_XARG(_value)
#endif
#define ATOMIC_CACHE_LOOKUP(_cache, _cacheSize, _key1, _key2) ({            \
    AtomicCacheEntry* pEntry;                                               \
    int hash;                                                               \
    u4 firstVersion, secondVersion;                                         \
    u4 value;                                                               \
                                                                            \
    /* simple hash function */                                              \
    hash = (((u4)(_key1) >> 2) ^ (u4)(_key2)) & ((_cacheSize)-1);           \
    pEntry = (_cache)->entries + hash;                                      \
                                                                            \
    firstVersion = android_atomic_acquire_load((int32_t*)&pEntry->version); \
                                                                            \
    if (pEntry->key1 == (u4)(_key1) && pEntry->key2 == (u4)(_key2)) {       \
        /*                                                                  \
         * The fields match.  Get the value, then read the version a        \
         * second time to verify that we didn't catch a partial update.     \
         * We're also hosed if "firstVersion" was odd, indicating that      \
         * an update was in progress before we got here (unlikely).         \
         */                                                                 \
        value = android_atomic_acquire_load((int32_t*) &pEntry->value);     \
        secondVersion = pEntry->version;                                    \
                                                                            \
        if ((firstVersion & 0x01) != 0 || firstVersion != secondVersion) {  \
            /*                                                              \
             * We clashed with another thread.  Instead of sitting and      \
             * spinning, which might not complete if we're a high priority  \
             * thread, just do the regular computation.                     \
             */                                                             \
            if (CALC_CACHE_STATS)                                           \
                (_cache)->fail++;                                           \
            value = (u4) ATOMIC_CACHE_CALC;                                 \
        } else {                                                            \
            /* all good */                                                  \
            if (CALC_CACHE_STATS)                                           \
                (_cache)->hits++;                                           \
        }                                                                   \
    } else {                                                                \
        /*                                                                  \
         * Compute the result and update the cache.  We really want this    \
         * to happen in a different method -- it makes the ARM frame        \
         * setup for this method simpler, which gives us a ~10% speed       \
         * boost.                                                           \
         */                                                                 \
        value = (u4) ATOMIC_CACHE_CALC;                                     \
        if (value == 0 && ATOMIC_CACHE_NULL_ALLOWED) { \
            dvmUpdateAtomicCache((u4) (_key1), (u4) (_key2), value, pEntry, \
                        firstVersion CACHE_XARG(_cache) ); \
        } \
    }                                                                       \
    value;                                                                  \
})

#endif //CUSTOMAPPVMP_ATOMICCACHE_H
