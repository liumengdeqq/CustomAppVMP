//
// Created by liu meng on 2018/8/27.
//

/*
 * Some declarations used throughout mterp.
 */
#ifndef DALVIK_MTERP_MTERP_H_
#define DALVIK_MTERP_MTERP_H_

#include "../Dalvik.h"
#include "../interp/InterpDefs.h"
#if defined(WITH_JIT)
#include "interp/Jit.h"
#endif

/*
 * Call this during initialization to verify that the values in asm-constants.h
 * are still correct.
 */
extern "C" bool dvmCheckAsmConstants(void);

/*
 * Local entry and exit points.  The platform-specific implementation must
 * provide these two.
 */
extern "C" void dvmMterpStdRun(Thread* self);
extern "C" void dvmMterpStdBail(Thread* self);

/*
 * Helper for common_printMethod(), invoked from the assembly
 * interpreter.
 */
extern "C" void dvmMterpPrintMethod(Method* method);

#endif  // DALVIK_MTERP_MTERP_H_
