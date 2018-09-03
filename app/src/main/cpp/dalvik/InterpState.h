//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_INTERPSTATE_H
#define CUSTOMAPPVMP_INTERPSTATE_H

#include "object.h"

struct InterpSaveState {
    const u2*       pc;         // Dalvik PC
    u4*             curFrame;   // Dalvik frame pointer
    const struct Method    *method;    // Method being executed
    struct  DvmDex*         methodClassDex;
     JValue          retval;
    void*           bailPtr;
#if defined(WITH_TRACKREF_CHECKS)
    int             debugTrackedRefStart;
#else
    int             unused;        // Keep struct size constant
#endif
    struct InterpSaveState* prev;  // To follow nested activations
} __attribute__ ((__packed__));
enum ExecutionSubModes {
    kSubModeNormal            = 0x0000,   /* No active subMode */
    kSubModeMethodTrace       = 0x0001,
    kSubModeEmulatorTrace     = 0x0002,
    kSubModeInstCounting      = 0x0004,
    kSubModeDebuggerActive    = 0x0008,
    kSubModeSuspendPending    = 0x0010,
    kSubModeCallbackPending   = 0x0020,
    kSubModeCountedStep       = 0x0040,
    kSubModeCheckAlways       = 0x0080,
    kSubModeSampleTrace       = 0x0100,
    kSubModeJitTraceBuild     = 0x4000,
    kSubModeJitSV             = 0x8000,
    kSubModeDebugProfile   = (kSubModeMethodTrace |
                              kSubModeEmulatorTrace |
                              kSubModeInstCounting |
                              kSubModeDebuggerActive)
};

#endif //CUSTOMAPPVMP_INTERPSTATE_H
