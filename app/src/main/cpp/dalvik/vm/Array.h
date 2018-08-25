//
// Created by liu meng on 2018/8/25.
//
#include "Globals.h"
#include "log.h"
#include "ObjectInlines.h"
#include "Heap.h"
#include "AllocTracker.h"
#include "Exception.h"
#include <jni.h>
#ifndef CUSTOMAPPVMP_ARRAY_H
#define CUSTOMAPPVMP_ARRAY_H
INLINE bool dvmIsArrayClass(const ClassObject* clazz)
{
    return (clazz->descriptor[0] == '[');
}
static ArrayObject* allocArray(ClassObject* arrayClass, size_t length,
                               size_t elemWidth, int allocFlags)
{
    assert(arrayClass != NULL);
    assert(arrayClass->descriptor != NULL);
    assert(arrayClass->descriptor[0] == '[');
    assert(length <= 0x7fffffff);
    assert(elemWidth > 0);
    assert(elemWidth <= 8);
    assert((elemWidth & (elemWidth - 1)) == 0);
    size_t elementShift = sizeof(size_t) * CHAR_BIT - 1 - CLZ(elemWidth);
    size_t elementSize = length << elementShift;
    size_t headerSize = OFFSETOF_MEMBER(ArrayObject, contents);
    size_t totalSize = elementSize + headerSize;
    if (elementSize >> elementShift != length || totalSize < elementSize) {
//        std::string descriptor(dvmHumanReadableDescriptor(arrayClass->descriptor));
        dvmThrowExceptionFmt(gDvm.exOutOfMemoryError,
                             "%s of length %zd exceeds the VM limit", "", length);
        return NULL;
    }
    ArrayObject* newArray = (ArrayObject*)dvmMalloc(totalSize, allocFlags);
    if (newArray != NULL) {
        DVM_OBJECT_INIT(newArray, arrayClass);
        newArray->length = length;
        dvmTrackAllocation(arrayClass, totalSize);
    }
    return newArray;
}

ArrayObject* dvmAllocPrimitiveArray(char type, size_t length, int allocFlags)
{
    ArrayObject* newArray;
    ClassObject* arrayClass;
    int width;

    switch (type) {
        case 'I':
            arrayClass = gDvm.classArrayInt;
            width = 4;
            break;
        case 'C':
            arrayClass = gDvm.classArrayChar;
            width = 2;
            break;
        case 'B':
            arrayClass = gDvm.classArrayByte;
            width = 1;
            break;
        case 'Z':
            arrayClass = gDvm.classArrayBoolean;
            width = 1; /* special-case this? */
            break;
        case 'F':
            arrayClass = gDvm.classArrayFloat;
            width = 4;
            break;
        case 'D':
            arrayClass = gDvm.classArrayDouble;
            width = 8;
            break;
        case 'S':
            arrayClass = gDvm.classArrayShort;
            width = 2;
            break;
        case 'J':
            arrayClass = gDvm.classArrayLong;
            width = 8;
            break;
        default:
            ALOGE("Unknown primitive type '%c'", type);
            dvmAbort();
            return NULL; // Keeps the compiler happy.
    }

    newArray = allocArray(arrayClass, length, width, allocFlags);

    /* the caller must dvmReleaseTrackedAlloc if allocFlags==ALLOC_DEFAULT */
    return newArray;
}
INLINE bool dvmIsObjectArrayClass(const ClassObject* clazz)
{
    const char* descriptor = clazz->descriptor;
    return descriptor[0] == '[' && (descriptor[1] == 'L' ||
                                    descriptor[1] == '[');
}

size_t dvmArrayClassElementWidth(const ClassObject* arrayClass)
{
    const char *descriptor;

    assert(dvmIsArrayClass(arrayClass));

    if (dvmIsObjectArrayClass(arrayClass)) {
        return sizeof(Object *);
    } else {
        descriptor = arrayClass->descriptor;
        switch (descriptor[1]) {
            case 'B': return 1;  /* byte */
            case 'C': return 2;  /* char */
            case 'D': return 8;  /* double */
            case 'F': return 4;  /* float */
            case 'I': return 4;  /* int */
            case 'J': return 8;  /* long */
            case 'S': return 2;  /* short */
            case 'Z': return 1;  /* boolean */
        }
    }
    ALOGE("class %p has an unhandled descriptor '%s'", arrayClass, descriptor);
    dvmDumpThread(dvmThreadSelf(), false);
    dvmAbort();
    return 0;  /* Quiet the compiler. */
}
size_t dvmArrayObjectSize(const ArrayObject *array)
{
    assert(array != NULL);
    size_t size = OFFSETOF_MEMBER(ArrayObject, contents);
    size += array->length * dvmArrayClassElementWidth(array->clazz);
    return size;
}
ClassObject* dvmFindArrayClass(const char* descriptor, Object* loader)
{
    ClassObject* clazz;

    assert(descriptor[0] == '[');
    //ALOGV("dvmFindArrayClass: '%s' %p", descriptor, loader);

    clazz = dvmLookupClass(descriptor, loader, false);
    if (clazz == NULL) {
        ALOGV("Array class '%s' %p not found; creating", descriptor, loader);
        clazz = createArrayClass(descriptor, loader);
        if (clazz != NULL)
            dvmAddInitiatingLoader(clazz, loader);
    }

    return clazz;
}

#endif //CUSTOMAPPVMP_ARRAY_H
