//
// Created by liu meng on 2018/8/21.
//
#include <stddef.h>
struct HeapSource {
    /* Target ideal heap utilization ratio; range 1..HEAP_UTILIZATION_MAX
     */
    size_t targetUtilization;

    /* The starting heap size.
     */
    size_t startSize;

    /* The largest that the heap source as a whole is allowed to grow.
     */
    size_t maximumSize;

    /*
     * The largest size we permit the heap to grow.  This value allows
     * the user to limit the heap growth below the maximum size.  This
     * is a work around until we can dynamically set the maximum size.
     * This value can range between the starting size and the maximum
     * size but should never be set below the current footprint of the
     * heap.
     */
    size_t growthLimit;

    /* The desired max size of the heap source as a whole.
     */
    size_t idealSize;

    /* The maximum number of bytes allowed to be allocated from the
     * active heap before a GC is forced.  This is used to "shrink" the
     * heap in lieu of actual compaction.
     */
    size_t softLimit;

    /* Minimum number of free bytes. Used with the target utilization when
     * setting the softLimit. Never allows less bytes than this to be free
     * when the heap size is below the maximum size or growth limit.
     */
    size_t minFree;

    /* Maximum number of free bytes. Used with the target utilization when
     * setting the softLimit. Never allows more bytes than this to be free
     * when the heap size is below the maximum size or growth limit.
     */
    size_t maxFree;

    /* The heaps; heaps[0] is always the active heap,
     * which new objects should be allocated from.
     */
    Heap heaps[HEAP_SOURCE_MAX_HEAP_COUNT];

    /* The current number of heaps.
     */
    size_t numHeaps;

    /* True if zygote mode was active when the HeapSource was created.
     */
    bool sawZygote;

    /*
     * The base address of the virtual memory reservation.
     */
    char *heapBase;

    /*
     * The length in bytes of the virtual memory reservation.
     */
    size_t heapLength;

    /*
     * The live object bitmap.
     */
    HeapBitmap liveBits;

    /*
     * The mark bitmap.
     */
    HeapBitmap markBits;

    /*
     * Native allocations.
     */
    int32_t nativeBytesAllocated;
    size_t nativeFootprintGCWatermark;
    size_t nativeFootprintLimit;
    bool nativeNeedToRunFinalization;

    /*
     * State for the GC daemon.
     */
    bool hasGcThread;
    pthread_t gcThread;
    bool gcThreadShutdown;
    pthread_mutex_t gcThreadMutex;
    pthread_cond_t gcThreadCond;
    bool gcThreadTrimNeeded;
};
