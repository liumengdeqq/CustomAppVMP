//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_OBJECTINLINES_H
#define CUSTOMAPPVMP_OBJECTINLINES_H

#include "Inlines.h"
#include "Object.h"
#include "WriteBarrier.h"
INLINE void dvmSetObjectArrayElement(const ArrayObject* obj, int index,
                                     Object* val) {
    ((Object **)(void *)(obj)->contents)[index] = val;
    if (val != NULL) {
        dvmWriteBarrierArray(obj, index, index + 1);
    }
}
#endif //CUSTOMAPPVMP_OBJECTINLINES_H
