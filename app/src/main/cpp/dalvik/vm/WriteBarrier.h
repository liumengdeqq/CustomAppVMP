//
// Created by liu meng on 2018/8/24.
//
#include "Inlines.h"
#include "object.h"
#include "CardTable.h"
#ifndef CUSTOMAPPVMP_WRITEBARRIER_H
#define CUSTOMAPPVMP_WRITEBARRIER_H
INLINE void dvmWriteBarrierField(const Object *obj, void *addr)
{
    dvmMarkCard(obj);
}
#endif //CUSTOMAPPVMP_WRITEBARRIER_H
