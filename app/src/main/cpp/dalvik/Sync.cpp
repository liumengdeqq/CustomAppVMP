//
// Created by liu meng on 2018/8/31.
//

#include "Sync.h"

bool initSynFuction(void *dvm_hand,int apilevel){
    if (dvm_hand) {
        dvmLockObjectHook = (dvmLockObject_func)dlsym(dvm_hand,"dvmLockObject");
        if (!dvmLockObjectHook) {
            return JNI_FALSE;
        }
        dvmUnlockObjectHook= (dvmUnlockObject_func)dlsym(dvm_hand,"dvmUnlockObject");
        if (!dvmUnlockObjectHook) {
            return JNI_FALSE;
        }
        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}