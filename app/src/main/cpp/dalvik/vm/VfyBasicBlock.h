//
// Created by liu meng on 2018/8/23.
//
#include "common.h"
#include "BitVector.h"
#include "PointerSet.h"
#ifndef CUSTOMAPPVMP_VFYBASICBLOCK_H
#define CUSTOMAPPVMP_VFYBASICBLOCK_H
struct VfyBasicBlock {
    u4              firstAddr;      /* address of first instruction */
    u4              lastAddr;       /* address of last instruction */
    PointerSet*     predecessors;   /* set of basic blocks that can flow here */
    BitVector*      liveRegs;       /* liveness for each register */
    bool            changed;        /* input set has changed, must re-eval */
    bool            visited;        /* block has been visited at least once */
};

#endif //CUSTOMAPPVMP_VFYBASICBLOCK_H
