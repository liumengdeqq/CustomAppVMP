//
// Created by liu meng on 2018/8/20.
//
#include "common.h"
#include "Dalvik.h"
#ifndef CUSTOMAPPVMP_BITVECTOR_H
#define CUSTOMAPPVMP_BITVECTOR_H
struct BitVector {
    bool    expandable;     /* expand bitmap if we run out? */
    u4      storageSize;    /* current size, in 32-bit words */
    u4*     storage;
};
BitVector* dvmAllocBitVector(unsigned int startBits, bool expandable)
{
    BitVector* bv;
    unsigned int count;

    assert(sizeof(bv->storage[0]) == 4);        /* assuming 32-bit units */

    bv = (BitVector*) malloc(sizeof(BitVector));

    count = (startBits + 31) >> 5;

    bv->storageSize = count;
    bv->expandable = expandable;
    bv->storage = (u4*) calloc(count, sizeof(u4));
    return bv;
}
static void checkSizes(const BitVector* bv1, const BitVector* bv2)
{
    if (bv1->storageSize != bv2->storageSize) {
        ALOGE("Mismatched vector sizes (%d, %d)",
              bv1->storageSize, bv2->storageSize);
        dvmAbort();
    }
}
void dvmCopyBitVector(BitVector *dest, const BitVector *src)
{
    /* if dest is expandable and < src, we could expand dest to match */
    checkSizes(dest, src);

    memcpy(dest->storage, src->storage, sizeof(u4) * dest->storageSize);
}
void dvmSetBit(BitVector* pBits, unsigned int num)
{
    if (num >= pBits->storageSize * sizeof(u4) * 8) {
        if (!pBits->expandable) {
            ALOGE("Attempt to set bit outside valid range (%d, limit is %d)",
                  num, pBits->storageSize * sizeof(u4) * 8);
            dvmAbort();
        }

        /* Round up to word boundaries for "num+1" bits */
        unsigned int newSize = (num + 1 + 31) >> 5;
        assert(newSize > pBits->storageSize);
        pBits->storage = (u4*)realloc(pBits->storage, newSize * sizeof(u4));
        if (pBits->storage == NULL) {
            ALOGE("BitVector expansion to %d failed", newSize * sizeof(u4));
            dvmAbort();
        }
        memset(&pBits->storage[pBits->storageSize], 0x00,
               (newSize - pBits->storageSize) * sizeof(u4));
        pBits->storageSize = newSize;
    }

    pBits->storage[num >> 5] |= 1 << (num & 0x1f);
}
void dvmClearBit(BitVector* pBits, unsigned int num)
{
    assert(num < pBits->storageSize * sizeof(u4) * 8);

    pBits->storage[num >> 5] &= ~(1 << (num & 0x1f));
}
bool dvmCheckMergeBitVectors(BitVector* dst, const BitVector* src)
{
    bool changed = false;

    checkSizes(dst, src);

    unsigned int idx;
    for (idx = 0; idx < dst->storageSize; idx++) {
        u4 merged = src->storage[idx] | dst->storage[idx];
        if (dst->storage[idx] != merged) {
            dst->storage[idx] = merged;
            changed = true;
        }
    }

    return changed;
}
void dvmFreeBitVector(BitVector* pBits)
{
    if (pBits == NULL)
        return;

    free(pBits->storage);
    free(pBits);
}

#endif //CUSTOMAPPVMP_BITVECTOR_H
