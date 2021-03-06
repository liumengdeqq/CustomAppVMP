//
// Created by liu meng on 2018/9/5.
//
#include "Allocc.h"
dvmReleaseTrackedAlloc_func dvmReleaseTrackedAllocHook;
dvmAddTrackedAlloc_func dvmAddTrackedAllocHook;
dvmAllocObject_func dvmAllocObjectHook;
bool initAllocFuction(void *dvm_hand,int apilevel){

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
