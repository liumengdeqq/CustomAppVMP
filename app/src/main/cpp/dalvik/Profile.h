//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_PROFILE_H
#define CUSTOMAPPVMP_PROFILE_H

#include "base.h"
struct AllocProfState {
    bool    enabled;            // is allocation tracking enabled?

    int     allocCount;         // #of objects allocated
    int     allocSize;          // cumulative size of objects

    int     failedAllocCount;   // #of times an allocation failed
    int     failedAllocSize;    // cumulative size of failed allocations

    int     freeCount;          // #of objects freed
    int     freeSize;           // cumulative size of freed objects

    int     gcCount;            // #of times an allocation triggered a GC

    int     classInitCount;     // #of initialized classes
    u8      classInitTime;      // cumulative time spent in class init (nsec)
};

#endif //CUSTOMAPPVMP_PROFILE_H
