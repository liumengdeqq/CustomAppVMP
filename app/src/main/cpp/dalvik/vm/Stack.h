//
// Created by liu meng on 2018/8/21.
//
#include "Class.h"
#ifndef CUSTOMAPPVMP_STACK_H
#define CUSTOMAPPVMP_STACK_H
struct StackSaveArea;

//#define PAD_SAVE_AREA       /* help debug stack trampling */

/*
 * The VM-specific internal goop.
 *
 * The idea is to mimic a typical native stack frame, with copies of the
 * saved PC and FP.  At some point we'd like to have interpreted and
 * native code share the same stack, though this makes portability harder.
 */
struct StackSaveArea {
#ifdef PAD_SAVE_AREA
    u4          pad0, pad1, pad2;
#endif

#ifdef EASY_GDB
    /* make it easier to trek through stack frames in GDB */
    StackSaveArea* prevSave;
#endif

    /* saved frame pointer for previous frame, or NULL if this is at bottom */
    u4*         prevFrame;

    /* saved program counter (from method in caller's frame) */
    const u2*   savedPc;

    /* pointer to method we're *currently* executing; handy for exceptions */
    const Method* method;

    union {
        /* for JNI native methods: bottom of local reference segment */
        u4          localRefCookie;

        /* for interpreted methods: saved current PC, for exception stack
         * traces and debugger traces */
        const u2*   currentPc;
    } xtra;

    /* Native return pointer for JIT, or 0 if interpreted */
    const u2* returnAddr;
#ifdef PAD_SAVE_AREA
    u4          pad3, pad4, pad5;
#endif
};
#define SAVEAREA_FROM_FP(_fp)   ((StackSaveArea*)(_fp) -1)
INLINE bool dvmIsBreakFrame(const u4* fp)
{
    return SAVEAREA_FROM_FP(fp)->method == NULL;
}

extern "C" int dvmLineNumFromPC(const Method* method, u4 relPc);
#endif //CUSTOMAPPVMP_STACK_H
