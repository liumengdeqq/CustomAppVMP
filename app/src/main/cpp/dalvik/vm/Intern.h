//
// Created by liu meng on 2018/8/25.
//
#include "object.h"
#include "Thread.h"
#include "Globals.h"
#include "Alloc.h"
#include "UtfString.h"
#ifndef CUSTOMAPPVMP_INTERN_H
#define CUSTOMAPPVMP_INTERN_H
static StringObject* lookupString(HashTable* table, u4 key, StringObject* value)
{
    void* entry = dvmHashTableLookup(table, key, (void*)value,
                                     dvmHashcmpStrings, false);
    return (StringObject*)entry;
}
static StringObject* insertString(HashTable* table, u4 key, StringObject* value)
{
    if (dvmIsNonMovingObject(value) == false) {
        value = (StringObject*)dvmCloneObject(value, ALLOC_NON_MOVING);
    }
    void* entry = dvmHashTableLookup(table, key, (void*)value,
                                     dvmHashcmpStrings, true);
    return (StringObject*)entry;
}

static StringObject* lookupInternedString(StringObject* strObj, bool isLiteral)
{
    StringObject* found;

    assert(strObj != NULL);
    u4 key = dvmComputeStringHash(strObj);
    dvmLockMutex(&gDvm.internLock);
    if (isLiteral) {
        /*
         * Check the literal table for a match.
         */
        StringObject* literal = lookupString(gDvm.literalStrings, key, strObj);
        if (literal != NULL) {
            /*
             * A match was found in the literal table, the easy case.
             */
            found = literal;
        } else {
            /*
             * There is no match in the literal table, check the
             * interned string table.
             */
            StringObject* interned = lookupString(gDvm.internedStrings, key, strObj);
            if (interned != NULL) {
                /*
                 * A match was found in the interned table.  Move the
                 * matching string to the literal table.
                 */
                dvmHashTableRemove(gDvm.internedStrings, key, interned);
                found = insertString(gDvm.literalStrings, key, interned);
                assert(found == interned);
            } else {
                /*
                 * No match in the literal table or the interned
                 * table.  Insert into the literal table.
                 */
                found = insertString(gDvm.literalStrings, key, strObj);
                assert(found == strObj);
            }
        }
    } else {
        /*
         * Check the literal table for a match.
         */
        found = lookupString(gDvm.literalStrings, key, strObj);
        if (found == NULL) {
            /*
             * No match was found in the literal table.  Insert into
             * the intern table if it does not already exist.
             */
            found = insertString(gDvm.internedStrings, key, strObj);
        }
    }
    assert(found != NULL);
    dvmUnlockMutex(&gDvm.internLock);
    return found;
}
StringObject* dvmLookupImmortalInternedString(StringObject* strObj)
{
    return lookupInternedString(strObj, true);
}
#endif //CUSTOMAPPVMP_INTERN_H
