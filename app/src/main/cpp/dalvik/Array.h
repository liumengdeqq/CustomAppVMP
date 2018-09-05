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
extern dvmAllocArrayByClass_func dvmAllocArrayByClassHook;
bool initArrayFuction(void *dvm_hand,int apilevel);
#endif //CUSTOMAPPVMP_ARRAY_H
