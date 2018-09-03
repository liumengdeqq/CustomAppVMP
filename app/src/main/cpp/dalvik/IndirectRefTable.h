//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_INDIRECTREFTABLE_H
#define CUSTOMAPPVMP_INDIRECTREFTABLE_H

struct IndirectRefSlot {
    struct Object* obj;        /* object pointer itself, NULL if the slot is unused */
    u4      serial;     /* slot serial number */
};
enum IndirectRefKind {
    kIndirectKindInvalid    = 0,
    kIndirectKindLocal      = 1,
    kIndirectKindGlobal     = 2,
    kIndirectKindWeakGlobal = 3
};
union IRTSegmentState {
    u4          all;
    struct {
        u4      topIndex:16;            /* index of first unused entry */
        u4      numHoles:16;            /* #of holes in entire table */
    } parts;
};
struct iref_iterator{
    void* table_;
    size_t i_;
    size_t capacity_;
};
struct IndirectRefTable{
    struct iref_iterator iterator;

    /* semi-public - read/write by interpreter in native call handler */
    union IRTSegmentState segmentState;

    /*
     * private:
     *
     * TODO: we can't make these private as long as the interpreter
     * uses offsetof, since private member data makes us non-POD.
     */
    /* bottom of the stack */
    struct IndirectRefSlot* table_;
    /* bit mask, ORed into all irefs */
    enum IndirectRefKind kind_;
    /* #of entries we have space for */
    size_t          alloc_entries_;
    /* max #of entries allowed */
    size_t          max_entries_;
};
#endif //CUSTOMAPPVMP_INDIRECTREFTABLE_H
