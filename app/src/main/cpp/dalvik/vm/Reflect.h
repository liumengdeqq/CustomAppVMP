//
// Created by liu meng on 2018/8/25.
//
#include "common.h"
#include "DexFile.h"
#include "object.h"
#include "Alloc.h"
#ifndef CUSTOMAPPVMP_REFLECT_H
#define CUSTOMAPPVMP_REFLECT_H
struct EncodedArrayIterator {
    const u1* cursor;                    /* current cursor */
    u4 elementsLeft;                     /* number of elements left to read */
    const DexEncodedArray* encodedArray; /* instance being iterated over */
    u4 size;                             /* number of elements in instance */
    const ClassObject* clazz;            /* class to resolve with respect to */
};
struct AnnotationValue {
    JValue  value;
    u1      type;
};
DataObject* dvmBoxPrimitive(JValue value, ClassObject* returnType)
{
    ClassObject* wrapperClass;
    DataObject* wrapperObj;
    s4* dataPtr;
    PrimitiveType typeIndex = returnType->primitiveType;
    const char* classDescriptor;

    if (typeIndex == PRIM_NOT) {
        /* add to tracking table so return value is always in table */
        if (value.l != NULL)
            dvmAddTrackedAlloc((Object*)value.l, NULL);
        return (DataObject*) value.l;
    }

    classDescriptor = dexGetBoxedTypeDescriptor(typeIndex);
    if (classDescriptor == NULL) {
        return NULL;
    }

    wrapperClass = dvmFindSystemClass(classDescriptor);
    if (wrapperClass == NULL) {
        ALOGW("Unable to find '%s'", classDescriptor);
        assert(dvmCheckException(dvmThreadSelf()));
        return NULL;
    }

    wrapperObj = (DataObject*) dvmAllocObject(wrapperClass, ALLOC_DEFAULT);
    if (wrapperObj == NULL)
        return NULL;
    dataPtr = (s4*) wrapperObj->instanceData;

    /* assumes value is stored in first instance field */
    /* (see dvmValidateBoxClasses) */
    if (typeIndex == PRIM_LONG || typeIndex == PRIM_DOUBLE)
        *(s8*)dataPtr = value.j;
    else
        *dataPtr = value.i;

    return wrapperObj;
}

#endif //CUSTOMAPPVMP_REFLECT_H
