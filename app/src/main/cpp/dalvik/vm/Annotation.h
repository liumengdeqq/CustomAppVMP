//
// Created by liu meng on 2018/8/25.
//
#include "object.h"
#include "Reflect.h"
#include "log.h"
#include "Thread.h"
#include "Resolve.h"
#ifndef CUSTOMAPPVMP_ANNOTATION_H
#define CUSTOMAPPVMP_ANNOTATION_H
enum AnnotationResultStyle {
    kAllObjects,         /* return everything as an object */
    kAllRaw,             /* return everything as a raw value or index */
    kPrimitivesOrObjects /* return primitives as-is but the rest as objects */
};
static u4 readUleb128(const u1** pBuf)
{
    u4 result = 0;
    int shift = 0;
    const u1* buf = *pBuf;
    u1 val;

    do {
        /*
         * Worst-case on bad data is we read too much data and return a bogus
         * result.  Safe to assume that we will encounter a byte with its
         * high bit clear before the end of the mapped file.
         */
        assert(shift < 32);

        val = *buf++;
        result |= (val & 0x7f) << shift;
        shift += 7;
    } while ((val & 0x80) != 0);

    *pBuf = buf;
    return result;
}
static s4 readSignedInt(const u1* ptr, int zwidth)
{
    s4 val = 0;
    int i;

    for (i = zwidth; i >= 0; --i)
        val = ((u4)val >> 8) | (((s4)*ptr++) << 24);
    val >>= (3 - zwidth) * 8;

    return val;
}
static u8 readUnsignedLong(const u1* ptr, int zwidth, bool fillOnRight)
{
    u8 val = 0;
    int i;

    if (!fillOnRight) {
        for (i = zwidth; i >= 0; --i)
            val = (val >> 8) | (((u8)*ptr++) << 56);
        val >>= (7 - zwidth) * 8;
    } else {
        for (i = zwidth; i >= 0; --i)
            val = (val >> 8) | (((u8)*ptr++) << 56);
    }
    return val;
}
static s8 readSignedLong(const u1* ptr, int zwidth)
{
    s8 val = 0;
    int i;

    for (i = zwidth; i >= 0; --i)
        val = ((u8)val >> 8) | (((s8)*ptr++) << 56);
    val >>= (7 - zwidth) * 8;

    return val;
}
static u4 readUnsignedInt(const u1* ptr, int zwidth, bool fillOnRight)
{
    u4 val = 0;
    int i;

    if (!fillOnRight) {
        for (i = zwidth; i >= 0; --i)
            val = (val >> 8) | (((u4)*ptr++) << 24);
        val >>= (3 - zwidth) * 8;
    } else {
        for (i = zwidth; i >= 0; --i)
            val = (val >> 8) | (((u4)*ptr++) << 24);
    }
    return val;
}
void dvmEncodedArrayIteratorInitialize(EncodedArrayIterator* iterator,
                                       const DexEncodedArray* encodedArray, const ClassObject* clazz) {
    iterator->encodedArray = encodedArray;
    iterator->cursor = encodedArray->array;
    iterator->size = readUleb128(&iterator->cursor);
    iterator->elementsLeft = iterator->size;
    iterator->clazz = clazz;
}
bool dvmEncodedArrayIteratorHasNext(const EncodedArrayIterator* iterator) {
    return (iterator->elementsLeft != 0);
}
static bool processAnnotationValue(const ClassObject* clazz,
                                   const u1** pPtr, AnnotationValue* pValue,
                                   AnnotationResultStyle resultStyle)
{
    Thread* self = dvmThreadSelf();
    Object* elemObj = NULL;
    bool setObject = false;
    const u1* ptr = *pPtr;
    u1 valueType, valueArg;
    int width;
    u4 idx;

    valueType = *ptr++;
    valueArg = valueType >> kDexAnnotationValueArgShift;
    width = valueArg + 1;       /* assume, correct later */

    ALOGV("----- type is 0x%02x %d, ptr=%p [0x%06x]",
          valueType & kDexAnnotationValueTypeMask, valueArg, ptr-1,
          (ptr-1) - (u1*)clazz->pDvmDex->pDexFile->baseAddr);

    pValue->type = valueType & kDexAnnotationValueTypeMask;

    switch (valueType & kDexAnnotationValueTypeMask) {
        case kDexAnnotationByte:
            pValue->value.i = (s1) readSignedInt(ptr, valueArg);
            if (resultStyle == kAllObjects) {
                elemObj = (Object*) dvmBoxPrimitive(pValue->value,
                                                    dvmFindPrimitiveClass('B'));
                setObject = true;
            }
            break;
        case kDexAnnotationShort:
            pValue->value.i = (s2) readSignedInt(ptr, valueArg);
            if (resultStyle == kAllObjects) {
                elemObj = (Object*) dvmBoxPrimitive(pValue->value,
                                                    dvmFindPrimitiveClass('S'));
                setObject = true;
            }
            break;
        case kDexAnnotationChar:
            pValue->value.i = (u2) readUnsignedInt(ptr, valueArg, false);
            if (resultStyle == kAllObjects) {
                elemObj = (Object*) dvmBoxPrimitive(pValue->value,
                                                    dvmFindPrimitiveClass('C'));
                setObject = true;
            }
            break;
        case kDexAnnotationInt:
            pValue->value.i = readSignedInt(ptr, valueArg);
            if (resultStyle == kAllObjects) {
                elemObj = (Object*) dvmBoxPrimitive(pValue->value,
                                                    dvmFindPrimitiveClass('I'));
                setObject = true;
            }
            break;
        case kDexAnnotationLong:
            pValue->value.j = readSignedLong(ptr, valueArg);
            if (resultStyle == kAllObjects) {
                elemObj = (Object*) dvmBoxPrimitive(pValue->value,
                                                    dvmFindPrimitiveClass('J'));
                setObject = true;
            }
            break;
        case kDexAnnotationFloat:
            pValue->value.i = readUnsignedInt(ptr, valueArg, true);
            if (resultStyle == kAllObjects) {
                elemObj = (Object*) dvmBoxPrimitive(pValue->value,
                                                    dvmFindPrimitiveClass('F'));
                setObject = true;
            }
            break;
        case kDexAnnotationDouble:
            pValue->value.j = readUnsignedLong(ptr, valueArg, true);
            if (resultStyle == kAllObjects) {
                elemObj = (Object*) dvmBoxPrimitive(pValue->value,
                                                    dvmFindPrimitiveClass('D'));
                setObject = true;
            }
            break;
        case kDexAnnotationBoolean:
            pValue->value.i = (valueArg != 0);
            if (resultStyle == kAllObjects) {
                elemObj = (Object*) dvmBoxPrimitive(pValue->value,
                                                    dvmFindPrimitiveClass('Z'));
                setObject = true;
            }
            width = 0;
            break;

        case kDexAnnotationString:
            idx = readUnsignedInt(ptr, valueArg, false);
            if (resultStyle == kAllRaw) {
                pValue->value.i = idx;
            } else {
                elemObj = (Object*) dvmResolveString(clazz, idx);
                setObject = true;
                if (elemObj == NULL)
                    return false;
                dvmAddTrackedAlloc(elemObj, self);      // balance the Release
            }
            break;
        case kDexAnnotationType:
            idx = readUnsignedInt(ptr, valueArg, false);
            if (resultStyle == kAllRaw) {
                pValue->value.i = idx;
            } else {
                elemObj = (Object*) dvmResolveClass(clazz, idx, true);
                setObject = true;
                if (elemObj == NULL) {
                    /* we're expected to throw a TypeNotPresentException here */
                    DexFile* pDexFile = clazz->pDvmDex->pDexFile;
                    const char* desc = dexStringByTypeIdx(pDexFile, idx);
                    dvmClearException(self);
                    dvmThrowTypeNotPresentException(desc);
                    return false;
                } else {
                    dvmAddTrackedAlloc(elemObj, self);      // balance the Release
                }
            }
            break;
        case kDexAnnotationMethod:
            idx = readUnsignedInt(ptr, valueArg, false);
            if (resultStyle == kAllRaw) {
                pValue->value.i = idx;
            } else {
                Method* meth = resolveAmbiguousMethod(clazz, idx);
                if (meth == NULL)
                    return false;
                elemObj = dvmCreateReflectObjForMethod(clazz, meth);
                setObject = true;
                if (elemObj == NULL)
                    return false;
            }
            break;
        case kDexAnnotationField:
            idx = readUnsignedInt(ptr, valueArg, false);
            assert(false);      // TODO
            break;
        case kDexAnnotationEnum:
            /* enum values are the contents of a static field */
            idx = readUnsignedInt(ptr, valueArg, false);
            if (resultStyle == kAllRaw) {
                pValue->value.i = idx;
            } else {
                StaticField* sfield;

                sfield = dvmResolveStaticField(clazz, idx);
                if (sfield == NULL) {
                    return false;
                } else {
                    assert(sfield->clazz->descriptor[0] == 'L');
                    elemObj = sfield->value.l;
                    setObject = true;
                    dvmAddTrackedAlloc(elemObj, self);      // balance the Release
                }
            }
            break;
        case kDexAnnotationArray:
            /*
             * encoded_array format, which is a size followed by a stream
             * of annotation_value.
             *
             * We create an array of Object, populate it, and return it.
             */
            if (resultStyle == kAllRaw) {
                return false;
            } else {
                ArrayObject* newArray;
                u4 size, count;

                size = readUleb128(&ptr);
                LOGVV("--- annotation array, size is %u at %p", size, ptr);
                newArray = dvmAllocArrayByClass(gDvm.classJavaLangObjectArray,
                                                size, ALLOC_DEFAULT);
                if (newArray == NULL) {
                    ALOGE("annotation element array alloc failed (%d)", size);
                    return false;
                }

                AnnotationValue avalue;
                for (count = 0; count < size; count++) {
                    if (!processAnnotationValue(clazz, &ptr, &avalue,
                                                kAllObjects)) {
                        dvmReleaseTrackedAlloc((Object*)newArray, self);
                        return false;
                    }
                    Object* obj = (Object*)avalue.value.l;
                    dvmSetObjectArrayElement(newArray, count, obj);
                    dvmReleaseTrackedAlloc(obj, self);
                }

                elemObj = (Object*) newArray;
                setObject = true;
            }
            width = 0;
            break;
        case kDexAnnotationAnnotation:
            /* encoded_annotation format */
            if (resultStyle == kAllRaw)
                return false;
            elemObj = processEncodedAnnotation(clazz, &ptr);
            setObject = true;
            if (elemObj == NULL)
                return false;
            dvmAddTrackedAlloc(elemObj, self);      // balance the Release
            width = 0;
            break;
        case kDexAnnotationNull:
            if (resultStyle == kAllRaw) {
                pValue->value.i = 0;
            } else {
                assert(elemObj == NULL);
                setObject = true;
            }
            width = 0;
            break;
        default:
            ALOGE("Bad annotation element value byte 0x%02x (0x%02x)",
                  valueType, valueType & kDexAnnotationValueTypeMask);
            assert(false);
            return false;
    }

    ptr += width;

    *pPtr = ptr;
    if (setObject)
        pValue->value.l = elemObj;
    return true;
}

bool dvmEncodedArrayIteratorGetNext(EncodedArrayIterator* iterator,
                                    AnnotationValue* value) {
    bool processed;

    if (iterator->elementsLeft == 0) {
        return false;
    }

    processed = processAnnotationValue(iterator->clazz, &iterator->cursor,
                                       value, kPrimitivesOrObjects);

    if (! processed) {
        ALOGE("Failed to process array element %d from %p",
              iterator->size - iterator->elementsLeft,
              iterator->encodedArray);
        iterator->elementsLeft = 0;
        return false;
    }

    iterator->elementsLeft--;
    return true;
}

#endif //CUSTOMAPPVMP_ANNOTATION_H
