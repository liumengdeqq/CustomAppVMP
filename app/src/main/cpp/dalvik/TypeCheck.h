//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_TYPECHECK_H
#define CUSTOMAPPVMP_TYPECHECK_H

#include "object.h"
#include <dlfcn.h>
typedef int (*dvmInstanceofNonTrivial_func)(const ClassObject* instance,const ClassObject* clazz);
 dvmInstanceofNonTrivial_func dvmInstanceofNonTrivialHook;
INLINE int dvmInstanceof(const ClassObject* instance, const ClassObject* clazz)
{
    if (instance == clazz) {
        return 1;
    } else
        return dvmInstanceofNonTrivialHook(instance, clazz);
}

typedef bool (*dvmCanPutArrayElement_func)(const ClassObject* objectClass,
                                          const ClassObject* arrayClass);
 dvmCanPutArrayElement_func dvmCanPutArrayElementHook;
static  bool initTypeCheckFuction(void *dvm_hand,int apilevel){
    if (dvm_hand) {
        dvmInstanceofNonTrivialHook = (dvmInstanceofNonTrivial_func)dlsym(dvm_hand,"dvmInstanceofNonTrivial");
        if (!dvmInstanceofNonTrivialHook) {
            return JNI_FALSE;
        }
        dvmCanPutArrayElementHook = (dvmCanPutArrayElement_func)dlsym(dvm_hand,"dvmCanPutArrayElement");
        if (!dvmCanPutArrayElementHook) {
            return JNI_FALSE;
        }

        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}

#endif //CUSTOMAPPVMP_TYPECHECK_H
