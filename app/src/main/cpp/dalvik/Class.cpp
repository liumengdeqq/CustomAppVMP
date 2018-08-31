//
// Created by liu meng on 2018/8/31.
//
#include "Class.h"
#include <dlfcn.h>
bool initClassFuction(void *dvm_hand,int apilevel){

    if (dvm_hand) {
        dvmInitClassHook = (dvmInitClass_func)dlsym(dvm_hand,"dvmInitClass");
        if (!dvmInitClassHook) {
            return JNI_FALSE;
        }
        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}