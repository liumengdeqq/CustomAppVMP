//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_CLASS_H
#define CUSTOMAPPVMP_CLASS_H

#include "object.h"
#include <dlfcn.h>
INLINE bool dvmIsClassInitialized(const ClassObject* clazz) {
    return (clazz->status == CLASS_INITIALIZED);
}
typedef bool (*dvmInitClass_func)(ClassObject* clazz);
dvmInitClass_func dvmInitClassHook;
static  bool initClassFuction(void *dvm_hand,int apilevel){

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
#endif //CUSTOMAPPVMP_CLASS_H
