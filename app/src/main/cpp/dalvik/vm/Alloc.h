//
// Created by liu meng on 2018/8/21.
//
#ifndef CUSTOMAPPVMP_ALLOC_H
#define CUSTOMAPPVMP_ALLOC_H
#include "object.h"
#include "Alloc.h"
#include "Class.h"
#include "ObjectInlines.h"
#include "Heap.h"
#include "Array.h"
#include "TypeCheck.h"
#include "MarkSweep.h"
#include "ReferenceTable.h"
enum {
    ALLOC_DEFAULT = 0x00,
    ALLOC_DONT_TRACK = 0x01,  /* don't add to internal tracking list */
    ALLOC_NON_MOVING = 0x02,
};
struct CountContext {
    const ClassObject *clazz;
    size_t count;
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
static void countAssignableInstancesOfClassCallback(Object *obj, void *arg)
{
    CountContext *ctx = (CountContext *)arg;
    assert(ctx != NULL);
    if (obj->clazz != NULL && dvmInstanceof(obj->clazz, ctx->clazz)) {
        ctx->count += 1;
    }
}
bool dvmIsNonMovingObject(const Object* object)
{
    return true;
}
Object* dvmCloneObject(Object* obj, int flags)
{
    assert(dvmIsValidObject(obj));
    ClassObject* clazz = obj->clazz;

    /* Class.java shouldn't let us get here (java.lang.Class is final
     * and does not implement Clonable), but make extra sure.
     * A memcpy() clone will wreak havoc on a ClassObject's "innards".
     */
    assert(!dvmIsTheClassClass(clazz));

    size_t size;
    if (IS_CLASS_FLAG_SET(clazz, CLASS_ISARRAY)) {
        size = dvmArrayObjectSize((ArrayObject *)obj);
    } else {
        size = clazz->objectSize;
    }

    Object* copy = (Object*)dvmMalloc(size, flags);
    if (copy == NULL)
        return NULL;

    DVM_OBJECT_INIT(copy, clazz);
    size_t offset = sizeof(Object);
    /* Copy instance data.  We assume memcpy copies by words. */
    memcpy((char*)copy + offset, (char*)obj + offset, size - offset);

    /* Mark the clone as finalizable if appropriate. */
    if (IS_CLASS_FLAG_SET(clazz, CLASS_ISFINALIZABLE)) {
        dvmSetFinalizable(copy);
    }

    dvmTrackAllocation(clazz, size);    /* notify DDMS */

    return copy;
}

void dvmAddTrackedAlloc(Object* obj, Thread* self)
{
    if (self == NULL)
        self = dvmThreadSelf();

    assert(obj != NULL);
    assert(self != NULL);
    if (!dvmAddToReferenceTable(&self->internalLocalRefTable, obj)) {
        ALOGE("threadid=%d: unable to add %p to internal ref table",
              self->threadId, obj);
        dvmDumpThread(self, false);
        dvmAbort();
    }
}
#endif //CUSTOMAPPVMP_ALLOC_H
