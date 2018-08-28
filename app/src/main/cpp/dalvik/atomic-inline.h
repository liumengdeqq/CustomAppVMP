//
// Created by liu meng on 2018/8/27.
//
#ifndef ANDROID_CUTILS_ATOMIC_INLINE_H
#define ANDROID_CUTILS_ATOMIC_INLINE_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Inline declarations and macros for some special-purpose atomic
 * operations.  These are intended for rare circumstances where a
 * memory barrier needs to be issued inline rather than as a function
 * call.
 *
 * Most code should not use these.
 *
 * Anything that does include this file must set ANDROID_SMP to either
 * 0 or 1, indicating compilation for UP or SMP, respectively.
 *
 * Macros defined in this header:
 *
 * void ANDROID_MEMBAR_FULL(void)
 *   Full memory barrier.  Provides a compiler reordering barrier, and
 *   on SMP systems emits an appropriate instruction.
 */

#if !defined(ANDROID_SMP)
//# error "Must define ANDROID_SMP before including atomic-inline.h"
#endif

#if defined(__arm__)
#include "atomic-arm.h"
#else
//#error atomic operations are unsupported
#endif

#if ANDROID_SMP == 0
#define ANDROID_MEMBAR_FULL android_compiler_barrier
#else
#define ANDROID_MEMBAR_FULL android_memory_barrier
#endif

#if ANDROID_SMP == 0
#define ANDROID_MEMBAR_STORE android_compiler_barrier
#else
#define ANDROID_MEMBAR_STORE android_memory_store_barrier
#endif

#ifdef __cplusplus
}
#endif

#endif /* ANDROID_CUTILS_ATOMIC_INLINE_H */
