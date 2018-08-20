//
// Created by liu meng on 2018/8/20.
//
#include <pthread.h>
#include "common.h"
#ifndef CUSTOMAPPVMP_HASH_H
#define CUSTOMAPPVMP_HASH_H
typedef void (*HashFreeFunc)(void* ptr);
struct HashEntry {
    u4 hashValue;
    void* data;
};

struct HashTable {
    int         tableSize;          /* must be power of 2 */
    int         numEntries;         /* current #of "live" entries */
    int         numDeadEntries;     /* current #of tombstone entries */
    HashEntry*  pEntries;           /* array on heap */
    HashFreeFunc freeFunc;
    pthread_mutex_t lock;
};

#endif //CUSTOMAPPVMP_HASH_H
