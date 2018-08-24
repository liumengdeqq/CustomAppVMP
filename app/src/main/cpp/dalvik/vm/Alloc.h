//
// Created by liu meng on 2018/8/21.
//
#include "object.h"
#include "Alloc.h"
#include "Class.h"
#include "ObjectInlines.h"
#include "Heap.h"
#include "Array.h"
#ifndef CUSTOMAPPVMP_ALLOC_H
#define CUSTOMAPPVMP_ALLOC_H
enum {
    ALLOC_DEFAULT = 0x00,
    ALLOC_DONT_TRACK = 0x01,  /* don't add to internal tracking list */
    ALLOC_NON_MOVING = 0x02,
};


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
void dvmReleaseTrackedAlloc(Object* obj, Thread* self)
{
    if (obj == NULL)
        return;

    if (self == NULL)
        self = dvmThreadSelf();
    assert(self != NULL);

    if (!dvmRemoveFromReferenceTable(&self->internalLocalRefTable,
                                     self->internalLocalRefTable.table, obj))
    {
        ALOGE("threadid=%d: failed to remove %p from internal ref table",
              self->threadId, obj);
        dvmAbort();
    }
}

#endif //CUSTOMAPPVMP_ALLOC_H
