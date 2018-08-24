//
// Created by liu meng on 2018/8/24.
//
#include "common.h"
#include "Globals.h"
#include "HeapInternal.h"
#ifndef CUSTOMAPPVMP_CARDTABLE_H
#define CUSTOMAPPVMP_CARDTABLE_H
#define GC_CARD_SHIFT 7
#define GC_CARD_SIZE (1 << GC_CARD_SHIFT)
#define GC_CARD_CLEAN 0
#define GC_CARD_DIRTY 0x70
bool dvmIsValidCard(const u1 *cardAddr)
{
    GcHeap *h = gDvm.gcHeap;
    u1* begin = h->cardTableBase + h->cardTableOffset;
    u1* end = &begin[h->cardTableLength];
    return cardAddr >= begin && cardAddr < end;
}

u1 *dvmCardFromAddr(const void *addr)
{
    u1 *biasedBase = gDvm.biasedCardTableBase;
    u1 *cardAddr = biasedBase + ((uintptr_t)addr >> GC_CARD_SHIFT);
    assert(dvmIsValidCard(cardAddr));
    return cardAddr;
}
void dvmMarkCard(const void *addr)
{
    u1 *cardAddr = dvmCardFromAddr(addr);
    *cardAddr = GC_CARD_DIRTY;
}
#endif //CUSTOMAPPVMP_CARDTABLE_H
