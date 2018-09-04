//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_ARRAY_H
#define CUSTOMAPPVMP_ARRAY_H
#include "object.h"
#include <dlfcn.h>
INLINE bool dvmIsArrayClass(const ClassObject* clazz)
{
    return (clazz->descriptor[0] == '[');
}

typedef ArrayObject*  (*dvmAllocArrayByClass_func)(ClassObject* arrayClass, size_t length, int allocFlags);
dvmAllocArrayByClass_func dvmAllocArrayByClassHook;
static  bool initArrayFuction(void *dvm_hand,int apilevel){
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
#endif //CUSTOMAPPVMP_ARRAY_H
