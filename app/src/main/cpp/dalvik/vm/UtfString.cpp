#include "UtfString.h"
#include "ObjectInlines.h"
#include "DexUtf.h"
#include "Class.h"
#include "log.h"
#include "Globals.h"
#include "Alloc.h"
#include "Array.h"
//
// Created by liu meng on 2018/8/20.
//
static StringObject* makeStringObject(u4 charsLength, ArrayObject** pChars)
{
    /*
     * The String class should have already gotten found (but not
     * necessarily initialized) before making it here. We assert it
     * explicitly, since historically speaking, we have had bugs with
     * regard to when the class String gets set up. The assert helps
     * make any regressions easier to diagnose.
     */
    assert(gDvm.classJavaLangString != NULL);

    if (!dvmIsClassInitialized(gDvm.classJavaLangString)) {
        /* Perform first-time use initialization of the class. */
        if (!dvmInitClass(gDvm.classJavaLangString)) {
            ALOGI("FATAL: Could not initialize class String");
            dvmAbort();
        }
    }

    Object* result = dvmAllocObject(gDvm.classJavaLangString, ALLOC_DEFAULT);
    if (result == NULL) {
        return NULL;
    }

    ArrayObject* chars = dvmAllocPrimitiveArray('C', charsLength, ALLOC_DEFAULT);
    if (chars == NULL) {
        dvmReleaseTrackedAlloc(result, NULL);
        return NULL;
    }

    dvmSetFieldInt(result, STRING_FIELDOFF_COUNT, charsLength);
    dvmSetFieldObject(result, STRING_FIELDOFF_VALUE, (Object*) chars);
    dvmReleaseTrackedAlloc((Object*) chars, NULL);
    /* Leave offset and hashCode set to zero. */

    *pChars = chars;
    return (StringObject*) result;
}
void dvmConvertUtf8ToUtf16(u2* utf16Str, const char* utf8Str)
{
    while (*utf8Str != '\0')
        *utf16Str++ = dexGetUtf16FromUtf8(&utf8Str);
}
static inline u4 computeUtf16Hash(const u2* utf16Str, size_t len)
{
    u4 hash = 0;

    while (len--)
        hash = hash * 31 + *utf16Str++;

    return hash;
}
StringObject* dvmCreateStringFromCstrAndLength(const char* utf8Str,
                                               size_t utf16Length)
{
    assert(utf8Str != NULL);

    ArrayObject* chars;
    StringObject* newObj = makeStringObject(utf16Length, &chars);
    if (newObj == NULL) {
        return NULL;
    }

    dvmConvertUtf8ToUtf16((u2*)(void*)chars->contents, utf8Str);

    u4 hashCode = computeUtf16Hash((u2*)(void*)chars->contents, utf16Length);
    dvmSetFieldInt((Object*) newObj, STRING_FIELDOFF_HASHCODE, hashCode);

    return newObj;
}
int dvmHashcmpStrings(const void* vstrObj1, const void* vstrObj2)
{
    const StringObject* strObj1 = (const StringObject*) vstrObj1;
    const StringObject* strObj2 = (const StringObject*) vstrObj2;

    assert(gDvm.classJavaLangString != NULL);

    /* get offset and length into char array; all values are in 16-bit units */
    int len1 = dvmGetFieldInt(strObj1, STRING_FIELDOFF_COUNT);
    int offset1 = dvmGetFieldInt(strObj1, STRING_FIELDOFF_OFFSET);
    int len2 = dvmGetFieldInt(strObj2, STRING_FIELDOFF_COUNT);
    int offset2 = dvmGetFieldInt(strObj2, STRING_FIELDOFF_OFFSET);
    if (len1 != len2) {
        return len1 - len2;
    }

    ArrayObject* chars1 =
            (ArrayObject*) dvmGetFieldObject(strObj1, STRING_FIELDOFF_VALUE);
    ArrayObject* chars2 =
            (ArrayObject*) dvmGetFieldObject(strObj2, STRING_FIELDOFF_VALUE);

    /* damage here actually indicates a broken java/lang/String */
    assert(offset1 + len1 <= (int) chars1->length);
    assert(offset2 + len2 <= (int) chars2->length);

    return memcmp((const u2*)(void*)chars1->contents + offset1,
                  (const u2*)(void*)chars2->contents + offset2,
                  len1 * sizeof(u2));
}
u4 dvmComputeStringHash(StringObject* strObj) {
    int hashCode = dvmGetFieldInt(strObj, STRING_FIELDOFF_HASHCODE);
    if (hashCode != 0) {
        return hashCode;
    }
    int len = dvmGetFieldInt(strObj, STRING_FIELDOFF_COUNT);
    int offset = dvmGetFieldInt(strObj, STRING_FIELDOFF_OFFSET);
    ArrayObject* chars =
            (ArrayObject*) dvmGetFieldObject(strObj, STRING_FIELDOFF_VALUE);
    hashCode = computeUtf16Hash((u2*)(void*)chars->contents + offset, len);
    dvmSetFieldInt(strObj, STRING_FIELDOFF_HASHCODE, hashCode);
    return hashCode;
}