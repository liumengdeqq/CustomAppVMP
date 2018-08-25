//
// Created by liu meng on 2018/8/20.
//
#include "Inlines.h"
#include "object.h"
#include "WriteBarrier.h"
#ifndef CUSTOMAPPVMP_OBJECTINLINES_H
#define CUSTOMAPPVMP_OBJECTINLINES_H
#define BYTE_OFFSET(_ptr, _offset)  ((void*) (((u1*)(_ptr)) + (_offset)))
INLINE void dvmSetFieldInt(Object* obj, int offset, s4 val) {
    ((JValue*)BYTE_OFFSET(obj, offset))->i = val;
}
INLINE void dvmSetFieldObject(Object* obj, int offset, Object* val) {
    JValue* lhs = (JValue*)BYTE_OFFSET(obj, offset);
    lhs->l = val;
    if (val != NULL) {
        dvmWriteBarrierField(obj, &lhs->l);
    }
}
INLINE Object* dvmGetFieldObject(const Object* obj, int offset) {
    return ((JValue*)BYTE_OFFSET(obj, offset))->l;
}
INLINE s4 dvmGetFieldInt(const Object* obj, int offset) {
    return ((JValue*)BYTE_OFFSET(obj, offset))->i;
}
INLINE void dvmSetStaticFieldObject(StaticField* sfield, Object* val) {
    sfield->value.l = val;
    if (val != NULL) {
        dvmWriteBarrierField(sfield->clazz, &sfield->value.l);
    }
}
#endif //CUSTOMAPPVMP_OBJECTINLINES_H
