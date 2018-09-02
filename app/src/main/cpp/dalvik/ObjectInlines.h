//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_OBJECTINLINES_H
#define CUSTOMAPPVMP_OBJECTINLINES_H

#include "Inlines.h"
#include "Object.h"
#include "WriteBarrier.h"
#define BYTE_OFFSET(_ptr, _offset)  ((void*) (((u1*)(_ptr)) + (_offset)))

INLINE s4 dvmGetFieldInt(const Object* obj, int offset) {
    return ((JValue*)BYTE_OFFSET(obj, offset))->i;
}
INLINE Object* dvmGetFieldObject(const Object* obj, int offset) {
    return ((JValue*)BYTE_OFFSET(obj, offset))->l;
}
INLINE void dvmSetObjectArrayElement(const ArrayObject* obj, int index,
                                     Object* val) {
    ((Object **)(void *)(obj)->contents)[index] = val;
    if (val != NULL) {
        dvmWriteBarrierArray(obj, index, index + 1);
    }
}
#endif //CUSTOMAPPVMP_OBJECTINLINES_H
