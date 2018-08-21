//
// Created by liu meng on 2018/8/21.
//
#include "Alloc.h"
Object* dvmAllocObject(ClassObject* clazz, int flags)
{
    Object* newObj;

    assert(clazz != NULL);
    assert(dvmIsClassInitialized(clazz) || dvmIsClassInitializing(clazz));

    /* allocate on GC heap; memory is zeroed out */
    newObj = (Object*)dvmMalloc(clazz->objectSize, flags);
    if (newObj != NULL) {
        DVM_OBJECT_INIT(newObj, clazz);
        dvmTrackAllocation(clazz, clazz->objectSize);   /* notify DDMS */
    }

    return newObj;
}
