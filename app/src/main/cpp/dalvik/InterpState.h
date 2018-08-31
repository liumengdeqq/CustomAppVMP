//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_INTERPSTATE_H
#define CUSTOMAPPVMP_INTERPSTATE_H

#include "Common.h"
#include "Object.h"
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
#endif //CUSTOMAPPVMP_INTERPSTATE_H
