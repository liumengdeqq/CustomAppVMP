/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Mutex-free cache.  Each entry has two 32-bit keys, one 32-bit value,
 * and a 32-bit version.
 */
#include "Dalvik.h"

#include <stdlib.h>

/*
 * I think modern C mandates that the results of a boolean expression are
 * 0 or 1.  If not, or we suddenly turn into C++ and bool != int, use this.
 */
#define BOOL_TO_INT(x)  (x)
//#define BOOL_TO_INT(x)  ((x) ? 1 : 0)

#define CPU_CACHE_WIDTH         32
#define CPU_CACHE_WIDTH_1       (CPU_CACHE_WIDTH-1)

#define ATOMIC_LOCK_FLAG        (1 << 31)

/*
 * Allocate cache.
 */
AtomicCache* dvmAllocAtomicCache(int numEntries)
{
    AtomicCache* newCache;

    newCache = (AtomicCache*) calloc(1, sizeof(AtomicCache));
    if (newCache == NULL)
        return NULL;

    newCache->numEntries = numEntries;

    newCache->entryAlloc = calloc(1,
        sizeof(AtomicCacheEntry) * numEntries + CPU_CACHE_WIDTH);
    if (newCache->entryAlloc == NULL) {
        free(newCache);
        return NULL;
    }

    /*
     * Adjust storage to align on a 32-byte boundary.  Each entry is 16 bytes
     * wide.  This ensures that each cache entry sits on a single CPU cache
     * line.
     */
    assert(sizeof(AtomicCacheEntry) == 16);
    newCache->entries = (AtomicCacheEntry*)
        (((int) newCache->entryAlloc + CPU_CACHE_WIDTH_1) & ~CPU_CACHE_WIDTH_1);

    return newCache;
}

/*
 * Free cache.
 */
void dvmFreeAtomicCache(AtomicCache* cache)
{
    if (cache != NULL) {
        free(cache->entryAlloc);
        free(cache);
    }
}


/*
 * Update a cache entry.
 *
 * In the event of a collision with another thread, the update may be skipped.
 *
 * We only need "pCache" for stats.
 */

int dvmFprintf(FILE* fp, const char* format, ...)
{
    va_list args;
    int result;

    va_start(args, format);
    if (gDvm.vfprintfHook != NULL)
        result = (*gDvm.vfprintfHook)(fp, format, args);
    else
        result = vfprintf(fp, format, args);
    va_end(args);

    return result;
}

/*
 * Dump the "instanceof" cache stats.
 */
void dvmDumpAtomicCacheStats(const AtomicCache* pCache)
{
    if (pCache == NULL)
        return;
    dvmFprintf(stdout,
        "Cache stats: trv=%d fai=%d hit=%d mis=%d fil=%d %d%% (size=%d)\n",
        pCache->trivial, pCache->fail, pCache->hits,
        pCache->misses, pCache->fills,
        (pCache->hits == 0) ? 0 :
            pCache->hits * 100 /
                (pCache->fail + pCache->hits + pCache->misses + pCache->fills),
        pCache->numEntries);
}
