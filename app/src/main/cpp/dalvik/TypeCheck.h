//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_TYPECHECK_H
#define CUSTOMAPPVMP_TYPECHECK_H

#include "Inlines.h"
#include "Object.h"
typedef int (*dvmInstanceofNonTrivial_func)(const ClassObject* instance,const ClassObject* clazz);
dvmInstanceofNonTrivial_func dvmInstanceofNonTrivialHook;
INLINE int dvmInstanceof(const ClassObject* instance, const ClassObject* clazz)
{
    if (instance == clazz) {
        return 1;
    } else
        return dvmInstanceofNonTrivialHook(instance, clazz);
}
bool initTypeCheckFuction(void *dvm_hand,int apilevel);
#endif //CUSTOMAPPVMP_TYPECHECK_H
