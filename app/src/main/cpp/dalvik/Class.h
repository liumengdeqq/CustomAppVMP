//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_CLASS_H
#define CUSTOMAPPVMP_CLASS_H

#include "object.h"
#include <dlfcn.h>
#include <jni.h>
INLINE bool dvmIsClassInitialized(const ClassObject* clazz) {
    return (clazz->status == CLASS_INITIALIZED);
}
typedef bool (*dvmInitClass_func)(ClassObject* clazz);
extern dvmInitClass_func dvmInitClassHook;
bool initClassFuction(void *dvm_hand,int apilevel);
#endif //CUSTOMAPPVMP_CLASS_H
