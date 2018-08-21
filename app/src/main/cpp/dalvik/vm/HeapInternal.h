//
// Created by liu meng on 2018/8/21.
//
#include "object.h"
#ifndef CUSTOMAPPVMP_HEAPINTERNAL_H
#define CUSTOMAPPVMP_HEAPINTERNAL_H
struct GcHeap {
    HeapSource *heapSource;

    /* Linked lists of subclass instances of java/lang/ref/Reference
     * that we find while recursing.  The "next" pointers are hidden
     * in the Reference objects' pendingNext fields.  These lists are
     * cleared and rebuilt each time the GC runs.
     */
    Object *softReferences;
    Object *weakReferences;
    Object *finalizerReferences;
    Object *phantomReferences;

    /* The list of Reference objects that need to be enqueued.
     */
    Object *clearedReferences;

    /* The current state of the mark step.
     * Only valid during a GC.
     */
    GcMarkContext markContext;

    /* GC's card table */
    u1* cardTableBase;
    size_t cardTableLength;
    size_t cardTableMaxLength;
    size_t cardTableOffset;

    /* Is the GC running?  Used to avoid recursive calls to GC.
     */
    bool gcRunning;

    /*
     * Debug control values
     */
    int ddmHpifWhen;
    int ddmHpsgWhen;
    int ddmHpsgWhat;
    int ddmNhsgWhen;
    int ddmNhsgWhat;
};

#endif //CUSTOMAPPVMP_HEAPINTERNAL_H
