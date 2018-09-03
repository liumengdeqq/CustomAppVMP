//
// Created by liu meng on 2018/8/31.
//

#include "Array.h"
bool initArrayFuction(void *dvm_hand,int apilevel){
    if (dvm_hand) {
        dvmAllocArrayByClassHook = (dvmAllocArrayByClass_func)dlsym(dvm_hand,"dvmInitClass");
        if (!dvmAllocArrayByClassHook) {
            return JNI_FALSE;
        }
        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}