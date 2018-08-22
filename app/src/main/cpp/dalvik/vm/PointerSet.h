//
// Created by liu meng on 2018/8/23.
//
#include "common.h"
#ifndef CUSTOMAPPVMP_POINTERSET_H
#define CUSTOMAPPVMP_POINTERSET_H
struct PointerSet {
    u2          alloc;
    u2          count;
    const void** list;
};

#endif //CUSTOMAPPVMP_POINTERSET_H
