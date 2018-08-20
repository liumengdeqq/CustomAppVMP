//
// Created by liu meng on 2018/8/20.
//

#ifndef CUSTOMAPPVMP_INTERPSTATE_H
#define CUSTOMAPPVMP_INTERPSTATE_H
struct InterpSaveState {
    const u2*       pc;         // Dalvik PC
    u4*             curFrame;   // Dalvik frame pointer
    const Method    *method;    // Method being executed
    DvmDex*         methodClassDex;
    JValue          retval;
    void*           bailPtr;
#if defined(WITH_TRACKREF_CHECKS)
    int             debugTrackedRefStart;
#else
    int             unused;        // Keep struct size constant
#endif
    struct InterpSaveState* prev;  // To follow nested activations
} __attribute__ ((__packed__));

enum ExecutionMode {
    kExecutionModeUnknown = 0,
    kExecutionModeInterpPortable,
    kExecutionModeInterpFast,
#if defined(WITH_JIT)
    kExecutionModeJit,
#endif
#if defined(WITH_JIT)  /* IA only */
    kExecutionModeNcgO0,
    kExecutionModeNcgO1,
#endif
};
#endif //CUSTOMAPPVMP_INTERPSTATE_H
