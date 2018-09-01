//
// Created by liu meng on 2018/9/1.
//

#ifndef CUSTOMAPPVMP_JNIINTERNAL_H
#define CUSTOMAPPVMP_JNIINTERNAL_H

#include "Inlines.h"
#include "Thread.h"
#include "Stack.h"
INLINE void dvmPopJniLocals(Thread* self, StackSaveArea* saveArea)
{
    self->jniLocalRefTable.segmentState.all = saveArea->xtra.localRefCookie;
}
#endif //CUSTOMAPPVMP_JNIINTERNAL_H
