//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_TYPECHECK_H
#define CUSTOMAPPVMP_TYPECHECK_H

#include "Inlines.h"
#include "Object.h"
int dvmInstanceofNonTrivial(const ClassObject* instance,const ClassObject* clazz);
INLINE int dvmInstanceof(const ClassObject* instance, const ClassObject* clazz)
{
    if (instance == clazz) {
        return 1;
    } else
        return dvmInstanceofNonTrivial(instance, clazz);
}
#endif //CUSTOMAPPVMP_TYPECHECK_H
