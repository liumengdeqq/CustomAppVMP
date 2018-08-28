//
// Created by liu meng on 2018/8/27.
//

#include "../Dalvik.h"
#include "../interp/interpDefs.h"
extern "C" {

/*
 * Look up an interface on a class using the cache.
 *
 * This function used to be defined in mterp/c/header.c, but it is now used by
 * the JIT compiler as well so it is separated into its own header file to
 * avoid potential out-of-sync changes in the future.
 */
INLINE Method* dvmFindInterfaceMethodInCache(ClassObject* thisClass,
u4 methodIdx, const Method* method, DvmDex* methodClassDex)
{
#define ATOMIC_CACHE_CALC \
    dvmInterpFindInterfaceMethod(thisClass, methodIdx, method, methodClassDex)
#define ATOMIC_CACHE_NULL_ALLOWED false

return (Method*) ATOMIC_CACHE_LOOKUP(methodClassDex->pInterfaceCache,
DEX_INTERFACE_CACHE_SIZE, thisClass, methodIdx);

#undef ATOMIC_CACHE_CALC
}

}
