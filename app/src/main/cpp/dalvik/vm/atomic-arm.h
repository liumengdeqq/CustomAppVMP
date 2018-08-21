//
// Created by liu meng on 2018/8/21.
//
#include "common.h"
#include "System.h"
#ifndef CUSTOMAPPVMP_ATOMIC_ARM_H
#define CUSTOMAPPVMP_ATOMIC_ARM_H
#ifndef ANDROID_ATOMIC_INLINE
#define ANDROID_ATOMIC_INLINE inline __attribute__((always_inline))
#endif
extern ANDROID_ATOMIC_INLINE void android_compiler_barrier()
{
    __asm__ __volatile__ ("" : : : "memory");
}

extern ANDROID_ATOMIC_INLINE void android_memory_barrier()
{
#if ANDROID_SMP == 0
    android_compiler_barrier();
#else
    __asm__ __volatile__ ("dmb" : : : "memory");
#endif
}
extern ANDROID_ATOMIC_INLINE
int android_atomic_cas(int32_t old_value, int32_t new_value,
                       volatile int32_t *ptr)
{
    int32_t prev, status;
    do {
        __asm__ __volatile__ ("ldrex %0, [%3]\n"
                "mov %1, #0\n"
                "teq %0, %4\n"
#ifdef __thumb2__
                "it eq\n"
#endif
                "strexeq %1, %5, [%3]"
        : "=&r" (prev), "=&r" (status), "+m"(*ptr)
        : "r" (ptr), "Ir" (old_value), "r" (new_value)
        : "cc");
    } while (__builtin_expect(status != 0, 0));
    return prev != old_value;
}
extern ANDROID_ATOMIC_INLINE
int android_atomic_release_cas(int32_t old_value, int32_t new_value,
                               volatile int32_t *ptr)
{
    android_memory_barrier();
    return android_atomic_cas(old_value, new_value, ptr);
}
extern ANDROID_ATOMIC_INLINE
void android_atomic_release_store(int32_t value, volatile int32_t *ptr)
{
    android_memory_barrier();
    *ptr = value;
}
void android_atomic_acquire_store(int32_t value, volatile int32_t *ptr)
{
    *ptr = value;
    android_memory_barrier();
}

#endif //CUSTOMAPPVMP_ATOMIC_ARM_H
