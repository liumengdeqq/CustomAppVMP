//
// Created by liu meng on 2018/8/25.
//
#include "object.h"
#include "Thread.h"
#include "log.h"
#include "Globals.h"
#include "Stack.h"
#ifndef CUSTOMAPPVMP_ALLOCTRACKER_H
#define CUSTOMAPPVMP_ALLOCTRACKER_H
#define kMaxAllocRecordStackDepth   16      /* max 255 */
#define SAVEAREA_FROM_FP(_fp)   ((StackSaveArea*)(_fp) -1)
#define kDefaultNumAllocRecords 64*1024 /* MUST be power of 2 */
#define dvmTrackAllocation(_clazz, _size)                                   \
    {                                                                       \
        if (gDvm.allocRecords != NULL)                                      \
            dvmDoTrackAllocation(_clazz, _size);                            \
    }
struct AllocRecord {
    ClassObject*    clazz;      /* class allocated in this block */
    u4              size;       /* total size requested */
    u2              threadId;   /* simple thread ID; could be recycled */

    /* stack trace elements; unused entries have method==NULL */
    struct {
        const Method* method;   /* which method we're executing in */
        int         pc;         /* current execution offset, in 16-bit units */
    } stackElem[kMaxAllocRecordStackDepth];
};
static void getStackFrames(Thread* self, AllocRecord* pRec)
{
    int stackDepth = 0;
    void* fp;

    fp = self->interpSave.curFrame;

    while ((fp != NULL) && (stackDepth < kMaxAllocRecordStackDepth)) {
        const StackSaveArea* saveArea = SAVEAREA_FROM_FP(fp);
        const Method* method = saveArea->method;

        if (!dvmIsBreakFrame((u4*) fp)) {
            pRec->stackElem[stackDepth].method = method;
            if (dvmIsNativeMethod(method)) {
                pRec->stackElem[stackDepth].pc = 0;
            } else {
                assert(saveArea->xtra.currentPc >= method->insns &&
                       saveArea->xtra.currentPc <
                       method->insns + dvmGetMethodInsnsSize(method));
                pRec->stackElem[stackDepth].pc =
                        (int) (saveArea->xtra.currentPc - method->insns);
            }
            stackDepth++;
        }

        assert(fp != saveArea->prevFrame);
        fp = saveArea->prevFrame;
    }

    /* clear out the rest (normally there won't be any) */
    while (stackDepth < kMaxAllocRecordStackDepth) {
        pRec->stackElem[stackDepth].method = NULL;
        pRec->stackElem[stackDepth].pc = 0;
        stackDepth++;
    }
}

void dvmDoTrackAllocation(ClassObject* clazz, size_t size);

#endif //CUSTOMAPPVMP_ALLOCTRACKER_H
