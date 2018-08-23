//
// Created by liu meng on 2018/8/20.
//
#include "common.h"
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

#endif //CUSTOMAPPVMP_BITVECTOR_H
