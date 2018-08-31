//
// Created by liu meng on 2018/8/31.
//
#include "TypeCheck.h"
#include <dlfcn.h>

bool initTypeCheckFuction(void *dvm_hand,int apilevel){
    if (dvm_hand) {
        dvmInstanceofNonTrivialHook = (dvmInstanceofNonTrivial_func)dlsym(dvm_hand,"dvmInstanceofNonTrivial");
        if (!dvmInstanceofNonTrivialHook) {
            return JNI_FALSE;
        }
        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}

