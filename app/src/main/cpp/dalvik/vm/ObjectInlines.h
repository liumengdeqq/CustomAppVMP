//
// Created by liu meng on 2018/8/20.
//
#include "Inlines.h"
#include "object.h"
#ifndef CUSTOMAPPVMP_OBJECTINLINES_H
#define CUSTOMAPPVMP_OBJECTINLINES_H
#define BYTE_OFFSET(_ptr, _offset)  ((void*) (((u1*)(_ptr)) + (_offset)))
INLINE void dvmSetFieldInt(Object* obj, int offset, s4 val) {
    ((JValue*)BYTE_OFFSET(obj, offset))->i = val;
}
#endif //CUSTOMAPPVMP_OBJECTINLINES_H
