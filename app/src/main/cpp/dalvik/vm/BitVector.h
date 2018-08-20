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
#endif //CUSTOMAPPVMP_BITVECTOR_H
