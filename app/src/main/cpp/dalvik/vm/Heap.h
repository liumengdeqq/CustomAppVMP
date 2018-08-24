//
// Created by liu meng on 2018/8/21.
//

//
// Created by liu meng on 2018/8/21.
//
#include "Heap.h"
#include "Thread.h"
#include "Globals.h"
#include <stddef.h>
#include "HeapSource.h"
#include <malloc.h>
#ifndef CUSTOMAPPVMP_HEAP_H
#define CUSTOMAPPVMP_HEAP_H
#define hs2heap(hs_) (&((hs_)->heaps[0]))
void* dvmHeapSourceAlloc(size_t n)
{
    HS_BOILERPLATE();

    HeapSource *hs = gHs;
    Heap* heap = hs2heap(hs);
    if (heap->bytesAllocated + n > hs->softLimit) {
        /*
         * This allocation would push us over the soft limit; act as
         * if the heap is full.
         */

        return NULL;
    }
    void* ptr;
    if (gDvm.lowMemoryMode) {
        /* This is only necessary because mspace_calloc always memsets the
         * allocated memory to 0. This is bad for memory usage since it leads
         * to dirty zero pages. If low memory mode is enabled, we use
         * mspace_malloc which doesn't memset the allocated memory and madvise
         * the page aligned region back to the kernel.
         */
        ptr = mspace_malloc(heap->msp, n);
        if (ptr == NULL) {
            return NULL;
        }
        uintptr_t zero_begin = (uintptr_t)ptr;
        uintptr_t zero_end = (uintptr_t)ptr + n;
        /* Calculate the page aligned region.
         */
        uintptr_t begin = ALIGN_UP_TO_PAGE_SIZE(zero_begin);
        uintptr_t end = zero_end & ~(uintptr_t)(SYSTEM_PAGE_SIZE - 1);
        /* If our allocation spans more than one page, we attempt to madvise.
         */
        if (begin < end) {
            /* madvise the page aligned region to kernel.
             */
            madvise((void*)begin, end - begin, MADV_DONTNEED);
            /* Zero the region after the page aligned region.
             */
            memset((void*)end, 0, zero_end - end);
            /* Zero out the region before the page aligned region.
             */
            zero_end = begin;
        }
        memset((void*)zero_begin, 0, zero_end - zero_begin);
    } else {
        ptr = mspace_calloc(heap->msp, 1, n);
        if (ptr == NULL) {
            return NULL;
        }
    }

    countAllocation(heap, ptr);
    /*
     * Check to see if a concurrent GC should be initiated.
     */
    if (gDvm.gcHeap->gcRunning || !hs->hasGcThread) {
        /*
         * The garbage collector thread is already running or has yet
         * to be started.  Do nothing.
         */
        return ptr;
    }
    if (heap->bytesAllocated > heap->concurrentStartBytes) {
        /*
         * We have exceeded the allocation threshold.  Wake up the
         * garbage collector.
         */
        dvmSignalCond(&gHs->gcThreadCond);
    }
    return ptr;
}
bool dvmLockHeap()
{
    if (dvmTryLockMutex(&gDvm.gcHeapLock) != 0) {
        Thread *self;
        ThreadStatus oldStatus;

        self = dvmThreadSelf();
        oldStatus = dvmChangeStatus(self, THREAD_VMWAIT);
        dvmLockMutex(&gDvm.gcHeapLock);
        dvmChangeStatus(self, oldStatus);
    }

    return true;
}
void dvmUnlockHeap()
{
    dvmUnlockMutex(&gDvm.gcHeapLock);
}
static void *tryMalloc(size_t size)
{
    void *ptr;

//TODO: figure out better heuristics
//    There will be a lot of churn if someone allocates a bunch of
//    big objects in a row, and we hit the frag case each time.
//    A full GC for each.
//    Maybe we grow the heap in bigger leaps
//    Maybe we skip the GC if the size is large and we did one recently
//      (number of allocations ago) (watch for thread effects)
//    DeflateTest allocs a bunch of ~128k buffers w/in 0-5 allocs of each other
//      (or, at least, there are only 0-5 objects swept each time)

    ptr = dvmHeapSourceAlloc(size);
    if (ptr != NULL) {
        return ptr;
    }

    /*
     * The allocation failed.  If the GC is running, block until it
     * completes and retry.
     */
    if (gDvm.gcHeap->gcRunning) {
        /*
         * The GC is concurrently tracing the heap.  Release the heap
         * lock, wait for the GC to complete, and retrying allocating.
         */
        dvmWaitForConcurrentGcToComplete();
    } else {
        /*
         * Try a foreground GC since a concurrent GC is not currently running.
         */
        gcForMalloc(false);
    }

    ptr = dvmHeapSourceAlloc(size);
    if (ptr != NULL) {
        return ptr;
    }

    /* Even that didn't work;  this is an exceptional state.
     * Try harder, growing the heap if necessary.
     */
    ptr = dvmHeapSourceAllocAndGrow(size);
    if (ptr != NULL) {
        size_t newHeapSize;

        newHeapSize = dvmHeapSourceGetIdealFootprint();
//TODO: may want to grow a little bit more so that the amount of free
//      space is equal to the old free space + the utilization slop for
//      the new allocation.
        LOGI_HEAP("Grow heap (frag case) to "
                          "%zu.%03zuMB for %zu-byte allocation",
                  FRACTIONAL_MB(newHeapSize), size);
        return ptr;
    }

    /* Most allocations should have succeeded by now, so the heap
     * is really full, really fragmented, or the requested size is
     * really big.  Do another GC, collecting SoftReferences this
     * time.  The VM spec requires that all SoftReferences have
     * been collected and cleared before throwing an OOME.
     */
//TODO: wait for the finalizers from the previous GC to finish
    LOGI_HEAP("Forcing collection of SoftReferences for %zu-byte allocation",
              size);
    gcForMalloc(true);
    ptr = dvmHeapSourceAllocAndGrow(size);
    if (ptr != NULL) {
        return ptr;
    }
//TODO: maybe wait for finalizers and try one last time

    LOGE_HEAP("Out of memory on a %zd-byte allocation.", size);
//TODO: tell the HeapSource to dump its state
    dvmDumpThread(dvmThreadSelf(), false);

    return NULL;
}
void* dvmMalloc(size_t size, int flags)
{
    void *ptr;

    dvmLockHeap();

    /* Try as hard as possible to allocate some memory.
     */
    ptr = tryMalloc(size);
    if (ptr != NULL) {
        /* We've got the memory.
         */
        if (gDvm.allocProf.enabled) {
            Thread* self = dvmThreadSelf();
            gDvm.allocProf.allocCount++;
            gDvm.allocProf.allocSize += size;
            if (self != NULL) {
                self->allocProf.allocCount++;
                self->allocProf.allocSize += size;
            }
        }
    } else {
        /* The allocation failed.
         */

        if (gDvm.allocProf.enabled) {
            Thread* self = dvmThreadSelf();
            gDvm.allocProf.failedAllocCount++;
            gDvm.allocProf.failedAllocSize += size;
            if (self != NULL) {
                self->allocProf.failedAllocCount++;
                self->allocProf.failedAllocSize += size;
            }
        }
    }

    dvmUnlockHeap();

    if (ptr != NULL) {
        /*
         * If caller hasn't asked us not to track it, add it to the
         * internal tracking list.
         */
        if ((flags & ALLOC_DONT_TRACK) == 0) {
            dvmAddTrackedAlloc((Object*)ptr, NULL);
        }
    } else {
        /*
         * The allocation failed; throw an OutOfMemoryError.
         */
        throwOOME();
    }

    return ptr;
}

#endif //CUSTOMAPPVMP_HEAP_H
