//
// Created by liu meng on 2018/9/3.
//

#ifndef CUSTOMAPPVMP_ATOMIC_H
#define CUSTOMAPPVMP_ATOMIC_H
#ifndef DALVIK_ATOMIC_H_
#define DALVIK_ATOMIC_H_

#include <stdint.h>
void dvmQuasiAtomicsStartup();
void dvmQuasiAtomicsShutdown();

/*
 * NOTE: Two "quasiatomic" operations on the exact same memory address
 * are guaranteed to operate atomically with respect to each other,
 * but no guarantees are made about quasiatomic operations mixed with
 * non-quasiatomic operations on the same address, nor about
 * quasiatomic operations that are performed on partially-overlapping
 * memory.
 *
 * Only the "Sync" versions of these provide a memory barrier.
 */

/*
 * Swap the 64-bit value at "addr" with "value".  Returns the previous
 * value.
 */
extern "C" int64_t dvmQuasiAtomicSwap64(int64_t value, volatile int64_t* addr);

/*
 * Swap the 64-bit value at "addr" with "value".  Returns the previous
 * value.  Provides memory barriers.
 */
extern "C" int64_t dvmQuasiAtomicSwap64Sync(int64_t value,
                                            volatile int64_t* addr);

/*
 * Read the 64-bit value at "addr".
 */
extern "C" int64_t dvmQuasiAtomicRead64(volatile const int64_t* addr);

/*
 * If the value at "addr" is equal to "oldvalue", replace it with "newvalue"
 * and return 0.  Otherwise, don't swap, and return nonzero.
 */
int dvmQuasiAtomicCas64(int64_t oldvalue, int64_t newvalue,
                        volatile int64_t* addr);

#endif  // DALVIK_ATOMIC_H_

#endif //CUSTOMAPPVMP_ATOMIC_H
