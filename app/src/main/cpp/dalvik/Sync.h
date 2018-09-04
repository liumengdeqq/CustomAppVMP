//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_SYNC_H
#define CUSTOMAPPVMP_SYNC_H

#include "Thread.h"
#include <dlfcn.h>
typedef void* (*dvmLockObject_func)(Thread* self, Object *obj);
 dvmLockObject_func dvmLockObjectHook;

typedef void* (*dvmUnlockObject_func)(Thread* self, Object *obj);
 dvmUnlockObject_func dvmUnlockObjectHook;

static  bool initSynFuction(void *dvm_hand,int apilevel){
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
#endif //CUSTOMAPPVMP_SYNC_H
