//
// Created by liu meng on 2018/8/31.
//
#include "Alloc.h"
#include <dlfcn.h>
#include <jni.h>
bool initAllocFuction(void *dvm_hand,int apilevel){

    if (dvm_hand) {
        dvmAllocObjectHook = (dvmAllocObject_func)dlsym(dvm_hand,"dvmAllocObject");
        if (!dvmAllocObjectHook) {
            return JNI_FALSE;
        }
        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}