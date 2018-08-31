//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_CLASS_H
#define CUSTOMAPPVMP_CLASS_H

#include "Inlines.h"
#include "Object.h"
INLINE bool dvmIsClassInitialized(const ClassObject* clazz) {
    return (clazz->status == CLASS_INITIALIZED);
}
typedef bool (*dvmInitClass_func)(ClassObject* clazz);
dvmInitClass_func dvmInitClassHook;
bool initClassFuction(void *dvm_hand,int apilevel);
#endif //CUSTOMAPPVMP_CLASS_H
