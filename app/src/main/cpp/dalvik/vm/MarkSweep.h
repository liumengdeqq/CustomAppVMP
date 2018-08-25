//
// Created by liu meng on 2018/8/25.
//
#include "object.h"
#include "Thread.h"
#include "Globals.h"
#ifndef CUSTOMAPPVMP_MARKSWEEP_H
#define CUSTOMAPPVMP_MARKSWEEP_H
void dvmSetFinalizable(Object *obj)
{
    assert(obj != NULL);
    Thread *self = dvmThreadSelf();
    assert(self != NULL);
    Method *meth = gDvm.methJavaLangRefFinalizerReferenceAdd;
    assert(meth != NULL);
    JValue unusedResult;
    dvmCallMethod(self, meth, NULL, &unusedResult, obj);
}

#endif //CUSTOMAPPVMP_MARKSWEEP_H
