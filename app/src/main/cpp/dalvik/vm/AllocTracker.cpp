//
// Created by liu meng on 2018/8/25.
//
#include "AllocTracker.h"
void dvmDoTrackAllocation(ClassObject* clazz, size_t size)
{
    Thread* self = dvmThreadSelf();
    if (self == NULL) {
        ALOGW("alloc tracker: no thread");
        return;
    }

    dvmLockMutex(&gDvm.allocTrackerLock);
    if (gDvm.allocRecords == NULL) {
        dvmUnlockMutex(&gDvm.allocTrackerLock);
        return;
    }

    /* advance and clip */
    if (++gDvm.allocRecordHead == gDvm.allocRecordMax)
        gDvm.allocRecordHead = 0;

    AllocRecord* pRec = &gDvm.allocRecords[gDvm.allocRecordHead];

    pRec->clazz = clazz;
    pRec->size = size;
    pRec->threadId = self->threadId;
    getStackFrames(self, pRec);

    if (gDvm.allocRecordCount < gDvm.allocRecordMax)
        gDvm.allocRecordCount++;

    dvmUnlockMutex(&gDvm.allocTrackerLock);
}