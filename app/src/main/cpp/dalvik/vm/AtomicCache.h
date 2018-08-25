//
// Created by liu meng on 2018/8/25.
//
#include "common.h"
#include "atomic-arm.h"
#include "Dalvik.h"
#include "atomic-arm.h"
#ifndef CUSTOMAPPVMP_ATOMICCACHE_H
#define CUSTOMAPPVMP_ATOMICCACHE_H
#define CPU_CACHE_WIDTH         32
#define CPU_CACHE_WIDTH_1       (CPU_CACHE_WIDTH-1)


#define CALC_CACHE_STATS 0

#define ATOMIC_LOCK_FLAG        (1 << 31)
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

void dvmUpdateAtomicCache(u4 key1, u4 key2, u4 value, AtomicCacheEntry* pEntry,
                          u4 firstVersion
#if CALC_CACHE_STATS > 0
        , AtomicCache* pCache
#endif
);
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
