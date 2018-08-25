//
// Created by liu meng on 2018/8/20.
//
#include <pthread.h>
#include "common.h"
#include "Dalvik.h"
#ifndef CUSTOMAPPVMP_HASH_H
#define CUSTOMAPPVMP_HASH_H
typedef void (*HashFreeFunc)(void* ptr);
struct HashEntry {
    u4 hashValue;
    void* data;
};
#define HASH_TOMBSTONE ((void*) 0xcbcacccd)
struct HashTable {
    int         tableSize;          /* must be power of 2 */
    int         numEntries;         /* current #of "live" entries */
    int         numDeadEntries;     /* current #of tombstone entries */
    HashEntry*  pEntries;           /* array on heap */
    HashFreeFunc freeFunc;
    pthread_mutex_t lock;
};
#define LOAD_NUMER  5       // 62.5%
#define LOAD_DENOM  8
typedef int (*HashCompareFunc)(const void* tableItem, const void* looseItem);
size_t dvmHashSize(size_t size) {
    return (size * LOAD_DENOM) / LOAD_NUMER +1;
}
#ifndef NDEBUG
/*
 * Count up the number of tombstone entries in the hash table.
 */
static int countTombStones(HashTable* pHashTable)
{
    int i, count;

    for (count = i = 0; i < pHashTable->tableSize; i++) {
        if (pHashTable->pEntries[i].data == HASH_TOMBSTONE)
            count++;
    }
    return count;
}
#endif

static bool resizeHash(HashTable* pHashTable, int newSize)
{
    HashEntry* pNewEntries;
    int i;

    assert(countTombStones(pHashTable) == pHashTable->numDeadEntries);
    //ALOGI("before: dead=%d", pHashTable->numDeadEntries);

    pNewEntries = (HashEntry*) calloc(newSize, sizeof(HashEntry));
    if (pNewEntries == NULL)
        return false;

    for (i = 0; i < pHashTable->tableSize; i++) {
        void* data = pHashTable->pEntries[i].data;
        if (data != NULL && data != HASH_TOMBSTONE) {
            int hashValue = pHashTable->pEntries[i].hashValue;
            int newIdx;

            /* probe for new spot, wrapping around */
            newIdx = hashValue & (newSize-1);
            while (pNewEntries[newIdx].data != NULL)
                newIdx = (newIdx + 1) & (newSize-1);

            pNewEntries[newIdx].hashValue = hashValue;
            pNewEntries[newIdx].data = data;
        }
    }

    free(pHashTable->pEntries);
    pHashTable->pEntries = pNewEntries;
    pHashTable->tableSize = newSize;
    pHashTable->numDeadEntries = 0;

    assert(countTombStones(pHashTable) == 0);
    return true;
}

void* dvmHashTableLookup(HashTable* pHashTable, u4 itemHash, void* item,
                         HashCompareFunc cmpFunc, bool doAdd)
{
    HashEntry* pEntry;
    HashEntry* pEnd;
    void* result = NULL;

    assert(pHashTable->tableSize > 0);
    assert(item != HASH_TOMBSTONE);
    assert(item != NULL);

    /* jump to the first entry and probe for a match */
    pEntry = &pHashTable->pEntries[itemHash & (pHashTable->tableSize-1)];
    pEnd = &pHashTable->pEntries[pHashTable->tableSize];
    while (pEntry->data != NULL) {
        if (pEntry->data != HASH_TOMBSTONE &&
            pEntry->hashValue == itemHash &&
            (*cmpFunc)(pEntry->data, item) == 0)
        {
            /* match */
            //ALOGD("+++ match on entry %d", pEntry - pHashTable->pEntries);
            break;
        }

        pEntry++;
        if (pEntry == pEnd) {     /* wrap around to start */
            if (pHashTable->tableSize == 1)
                break;      /* edge case - single-entry table */
            pEntry = pHashTable->pEntries;
        }

        //ALOGI("+++ look probing %d...", pEntry - pHashTable->pEntries);
    }

    if (pEntry->data == NULL) {
        if (doAdd) {
            pEntry->hashValue = itemHash;
            pEntry->data = item;
            pHashTable->numEntries++;

            /*
             * We've added an entry.  See if this brings us too close to full.
             */
            if ((pHashTable->numEntries+pHashTable->numDeadEntries) * LOAD_DENOM
                > pHashTable->tableSize * LOAD_NUMER)
            {
                if (!resizeHash(pHashTable, pHashTable->tableSize * 2)) {
                    /* don't really have a way to indicate failure */
                    ALOGE("Dalvik hash resize failure");
                    dvmAbort();
                }
                /* note "pEntry" is now invalid */
            } else {
                //ALOGW("okay %d/%d/%d",
                //    pHashTable->numEntries, pHashTable->tableSize,
                //    (pHashTable->tableSize * LOAD_NUMER) / LOAD_DENOM);
            }

            /* full table is bad -- search for nonexistent never halts */
            assert(pHashTable->numEntries < pHashTable->tableSize);
            result = item;
        } else {
            assert(result == NULL);
        }
    } else {
        result = pEntry->data;
    }

    return result;
}
bool dvmHashTableRemove(HashTable* pHashTable, u4 itemHash, void* item)
{
    HashEntry* pEntry;
    HashEntry* pEnd;

    assert(pHashTable->tableSize > 0);

    /* jump to the first entry and probe for a match */
    pEntry = &pHashTable->pEntries[itemHash & (pHashTable->tableSize-1)];
    pEnd = &pHashTable->pEntries[pHashTable->tableSize];
    while (pEntry->data != NULL) {
        if (pEntry->data == item) {
            //ALOGI("+++ stepping on entry %d", pEntry - pHashTable->pEntries);
            pEntry->data = HASH_TOMBSTONE;
            pHashTable->numEntries--;
            pHashTable->numDeadEntries++;
            return true;
        }

        pEntry++;
        if (pEntry == pEnd) {     /* wrap around to start */
            if (pHashTable->tableSize == 1)
                break;      /* edge case - single-entry table */
            pEntry = pHashTable->pEntries;
        }

        //ALOGI("+++ del probing %d...", pEntry - pHashTable->pEntries);
    }

    return false;
}
#endif //CUSTOMAPPVMP_HASH_H
