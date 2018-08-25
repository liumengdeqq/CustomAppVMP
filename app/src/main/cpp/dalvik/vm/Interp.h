//
// Created by liu meng on 2018/8/25.
//
#include "object.h"
#include "Globals.h"
#ifndef CUSTOMAPPVMP_INTERP_H
#define CUSTOMAPPVMP_INTERP_H
void dvmFlushBreakpoints(ClassObject* clazz)
{
    BreakpointSet* pSet = gDvm.breakpointSet;

    if (pSet == NULL)
        return;

    assert(dvmIsClassVerified(clazz));
    dvmBreakpointSetLock(pSet);
    dvmBreakpointSetFlush(pSet, clazz);
    dvmBreakpointSetUnlock(pSet);
}

#endif //CUSTOMAPPVMP_INTERP_H
