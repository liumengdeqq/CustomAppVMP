#include "UtfString.h"
#include "ObjectInlines.h"
#include "DexUtf.h"
#include "Class.h"
#include "log.h"
#include "Globals.h"
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
            MY_LOG_PRINT("FATAL: Could not initialize class String");
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
