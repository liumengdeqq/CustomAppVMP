//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_WRITEBARRIER_H
#define CUSTOMAPPVMP_WRITEBARRIER_H

#include "Object.h"
#include "CardTable.h"
INLINE void dvmWriteBarrierField(const Object *obj, void *addr)
{
    dvmMarkCardHook(obj);
}

/*
 * All of the Object may have changed.
 */
INLINE void dvmWriteBarrierObject(const Object *obj)
{
    dvmMarkCardHook(obj);
}

/*
 * Some or perhaps all of the array indexes in the Array, greater than
 * or equal to start and strictly less than end, have been written,
 * and perhaps changed.
 */
INLINE void dvmWriteBarrierArray(const ArrayObject *obj,
                                 size_t start, size_t end)
{
    dvmMarkCardHook((Object *)obj);
}
#endif //CUSTOMAPPVMP_WRITEBARRIER_H
