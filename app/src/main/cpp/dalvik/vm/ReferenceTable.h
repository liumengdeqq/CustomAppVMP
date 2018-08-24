//
// Created by liu meng on 2018/8/21.
//
#include "object.h"
#ifndef CUSTOMAPPVMP_REFERENCETABLE_H
#define CUSTOMAPPVMP_REFERENCETABLE_H
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
#endif //CUSTOMAPPVMP_REFERENCETABLE_H
