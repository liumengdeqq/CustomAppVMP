//
// Created by liu meng on 2018/8/21.
//
#ifndef CUSTOMAPPVMP_REFERENCETABLE_H
#define CUSTOMAPPVMP_REFERENCETABLE_H
#include "object.h"
#include "log.h"
struct ReferenceTable {
    Object**        nextEntry;          /* top of the list */
    Object**        table;              /* bottom of the list */

    int             allocEntries;       /* #of entries we have space for */
    int             maxEntries;         /* max #of entries allowed */
};
Object** dvmFindInReferenceTable(const ReferenceTable* pRef, Object** bottom,
                                 Object* obj)
{
    Object** ptr;

    ptr = pRef->nextEntry;
    while (--ptr >= bottom) {
        if (*ptr == obj)
            return ptr;
    }
    return NULL;
}
bool dvmRemoveFromReferenceTable(ReferenceTable* pRef, Object** bottom,
                                 Object* obj)
{
    Object** ptr;

    assert(pRef->table != NULL);

    /*
     * Scan from the most-recently-added entry up to the bottom entry for
     * this frame.
     */
    ptr = dvmFindInReferenceTable(pRef, bottom, obj);
    if (ptr == NULL)
        return false;

    /*
     * Delete the entry.
     */
    pRef->nextEntry--;
    int moveCount = pRef->nextEntry - ptr;
    if (moveCount != 0) {
        /* remove from middle, slide the rest down */
        memmove(ptr, ptr+1, moveCount * sizeof(Object*));
        //ALOGV("LREF delete %p, shift %d down", obj, moveCount);
    } else {
        /* last entry, falls off the end */
        //ALOGV("LREF delete %p from end", obj);
    }

    return true;
}
bool dvmIsHeapAddress(void *address)
{
    return address != NULL && (((uintptr_t) address & (8-1)) == 0);
}
bool dvmAddToReferenceTable(ReferenceTable* pRef, Object* obj)
{
    assert(obj != NULL);
    assert(dvmIsHeapAddress(obj));
    assert(pRef->table != NULL);
    assert(pRef->allocEntries <= pRef->maxEntries);

    if (pRef->nextEntry == pRef->table + pRef->allocEntries) {
        /* reached end of allocated space; did we hit buffer max? */
        if (pRef->nextEntry == pRef->table + pRef->maxEntries) {
            ALOGW("ReferenceTable overflow (max=%d)", pRef->maxEntries);
            return false;
        }

        Object** newTable;
        int newSize;

        newSize = pRef->allocEntries * 2;
        if (newSize > pRef->maxEntries)
            newSize = pRef->maxEntries;
        assert(newSize > pRef->allocEntries);

        newTable = (Object**) realloc(pRef->table, newSize * sizeof(Object*));
        if (newTable == NULL) {
            ALOGE("Unable to expand ref table (from %d to %d %d-byte entries)",
                  pRef->allocEntries, newSize, sizeof(Object*));
            return false;
        }
        LOGVV("Growing %p from %d to %d", pRef, pRef->allocEntries, newSize);

        /* update entries; adjust "nextEntry" in case memory moved */
        pRef->nextEntry = newTable + (pRef->nextEntry - pRef->table);
        pRef->table = newTable;
        pRef->allocEntries = newSize;
    }

    *pRef->nextEntry++ = obj;
    return true;
}
#endif //CUSTOMAPPVMP_REFERENCETABLE_H
