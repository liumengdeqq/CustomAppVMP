//
// Created by liu meng on 2018/8/25.
//
#include "Globals.h"
#include "log.h"
#include "ObjectInlines.h"
#include "Heap.h"
#include "AllocTracker.h"
#ifndef CUSTOMAPPVMP_ARRAY_H
#define CUSTOMAPPVMP_ARRAY_H
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
        std::string descriptor(dvmHumanReadableDescriptor(arrayClass->descriptor));
        dvmThrowExceptionFmt(gDvm.exOutOfMemoryError,
                             "%s of length %zd exceeds the VM limit", descriptor.c_str(), length);
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

#endif //CUSTOMAPPVMP_ARRAY_H
