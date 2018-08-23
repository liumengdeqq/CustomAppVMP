//
// Created by liu meng on 2018/8/23.
//
#include "common.h"
#include <malloc.h>
#include "log.h"
#include <string.h>
#include "Dalvik.h"
#ifndef CUSTOMAPPVMP_POINTERSET_H
#define CUSTOMAPPVMP_POINTERSET_H
struct PointerSet {
    u2          alloc;
    u2          count;
    const void** list;
};
PointerSet* dvmPointerSetAlloc(int initialSize)
{
    PointerSet* pSet = (PointerSet*)calloc(1, sizeof(PointerSet));
    if (pSet != NULL) {
        if (initialSize > 0) {
            pSet->list = (const void**)malloc(sizeof(void*) * initialSize);
            if (pSet->list == NULL) {
                free(pSet);
                return NULL;
            }
            pSet->alloc = initialSize;
        }
    }

    return pSet;
}
int dvmPointerSetGetCount(const PointerSet* pSet)
{
    return pSet->count;
}

void dvmPointerSetFree(PointerSet* pSet)
{
    if (pSet == NULL)
        return;

    if (pSet->list != NULL) {
        free(pSet->list);
        pSet->list = NULL;
    }
    free(pSet);
}
bool dvmPointerSetHas(const PointerSet* pSet, const void* ptr, int* pIndex)
{
    int hi, lo, mid;

    lo = mid = 0;
    hi = pSet->count-1;

    /* array is sorted, use a binary search */
    while (lo <= hi) {
        mid = (lo + hi) / 2;
        const void* listVal = pSet->list[mid];

        if (ptr > listVal) {
            lo = mid + 1;
        } else if (ptr < listVal) {
            hi = mid - 1;
        } else /* listVal == ptr */ {
            if (pIndex != NULL)
                *pIndex = mid;
            return true;
        }
    }

    if (pIndex != NULL)
        *pIndex = mid;
    return false;
}
#ifndef NDEBUG
static bool verifySorted(PointerSet* pSet)
{
    const void* last = NULL;
    int i;

    for (i = 0; i < pSet->count; i++) {
        const void* cur = pSet->list[i];
        if (cur < last)
            return false;
        last = cur;
    }

    return true;
}
#endif
bool dvmPointerSetAddEntry(PointerSet* pSet, const void* ptr)
{
    int nearby;

    if (dvmPointerSetHas(pSet, ptr, &nearby))
        return false;

    /* ensure we have space to add one more */
    if (pSet->count == pSet->alloc) {
        /* time to expand */
        const void** newList;

        if (pSet->alloc == 0)
            pSet->alloc = 4;
        else
            pSet->alloc *= 2;
        LOGVV("expanding %p to %d", pSet, pSet->alloc);
        newList = (const void**)realloc(pSet->list, pSet->alloc * sizeof(void*));
        if (newList == NULL) {
            ALOGE("Failed expanding ptr set (alloc=%d)", pSet->alloc);
            dvmAbort();
        }
        pSet->list = newList;
    }

    if (pSet->count == 0) {
        /* empty list */
        assert(nearby == 0);
    } else {
        /*
         * Determine the insertion index.  The binary search might have
         * terminated "above" or "below" the value.
         */
        if (nearby != 0 && ptr < pSet->list[nearby-1]) {
            //ALOGD("nearby-1=%d %p, inserting %p at -1",
            //    nearby-1, pSet->list[nearby-1], ptr);
            nearby--;
        } else if (ptr < pSet->list[nearby]) {
            //ALOGD("nearby=%d %p, inserting %p at +0",
            //    nearby, pSet->list[nearby], ptr);
        } else {
            //ALOGD("nearby+1=%d %p, inserting %p at +1",
            //    nearby+1, pSet->list[nearby+1], ptr);
            nearby++;
        }

        /*
         * Move existing values, if necessary.
         */
        if (nearby != pSet->count) {
            /* shift up */
            memmove(&pSet->list[nearby+1], &pSet->list[nearby],
                    (pSet->count - nearby) * sizeof(pSet->list[0]));
        }
    }

    pSet->list[nearby] = ptr;
    pSet->count++;

    assert(verifySorted(pSet));
    return true;
}
const void* dvmPointerSetGetEntry(const PointerSet* pSet, int i)
{
    return pSet->list[i];
}

#endif //CUSTOMAPPVMP_POINTERSET_H
