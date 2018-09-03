//
// Created by liu meng on 2018/9/1.
//

#ifndef CUSTOMAPPVMP_FINDINTERFACE_H
#define CUSTOMAPPVMP_FINDINTERFACE_H

#include "stdafx.h"
#include "AtomicCache.h"
#include "Interp.h"
INLINE Method* dvmFindInterfaceMethodInCache(ClassObject* thisClass,
u4 methodIdx, const Method* method, DvmDex* methodClassDex)
{
#define ATOMIC_CACHE_CALC \
    dvmInterpFindInterfaceMethodHook(thisClass, methodIdx, method, methodClassDex)
#define ATOMIC_CACHE_NULL_ALLOWED false

return (Method*) ATOMIC_CACHE_LOOKUP(methodClassDex->pInterfaceCache,
DEX_INTERFACE_CACHE_SIZE, thisClass, methodIdx);

#undef ATOMIC_CACHE_CALC
}
#endif //CUSTOMAPPVMP_FINDINTERFACE_H
