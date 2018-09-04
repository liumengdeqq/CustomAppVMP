//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_ALLOC_H
#define CUSTOMAPPVMP_ALLOC_H

#include "Thread.h"
enum {
    ALLOC_DEFAULT = 0x00,
    ALLOC_DONT_TRACK = 0x01,  /* don't add to internal tracking list */
    ALLOC_NON_MOVING = 0x02,
};
typedef Object* (*dvmAllocObject_func)(ClassObject* clazz, int flags);
dvmAllocObject_func dvmAllocObjectHook;
typedef void (*dvmAddTrackedAlloc_func)(Object* obj, Thread* self);
dvmAddTrackedAlloc_func dvmAddTrackedAllocHook;
typedef void (*dvmReleaseTrackedAlloc_func)(Object* obj, Thread* self);
dvmReleaseTrackedAlloc_func dvmReleaseTrackedAllocHook;
static bool initAllocFuction(void *dvm_hand,int apilevel){

    if (dvm_hand) {
        dvmAllocObjectHook = (dvmAllocObject_func)dlsym(dvm_hand,"dvmAllocObject");
        if (!dvmAllocObjectHook) {
            return JNI_FALSE;
        }
        dvmAddTrackedAllocHook = (dvmAddTrackedAlloc_func)dlsym(dvm_hand,"dvmAddTrackedAlloc");
        if (!dvmAddTrackedAllocHook) {
            return JNI_FALSE;
        }
        dvmReleaseTrackedAllocHook= (dvmReleaseTrackedAlloc_func)dlsym(dvm_hand,"dvmReleaseTrackedAlloc");
        if (!dvmReleaseTrackedAllocHook) {
            return JNI_FALSE;
        }

        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}
#endif //CUSTOMAPPVMP_ALLOC_H
