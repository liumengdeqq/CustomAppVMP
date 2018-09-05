//
// Created by liu meng on 2018/9/2.
//

#ifndef CUSTOMAPPVMP_ATOMIC_ARM_H
#define CUSTOMAPPVMP_ATOMIC_ARM_H

#include <stdint.h>
#include <jni.h>
#include <dlfcn.h>
#ifndef ANDROID_ATOMIC_INLINE
#define ANDROID_ATOMIC_INLINE inline __attribute__((always_inline))
#endif
#  define __builtin_expect(x, y) x
#if ANDROID_SMP == 0
#define ANDROID_MEMBAR_STORE android_compiler_barrier
#else
#define ANDROID_MEMBAR_STORE android_memory_store_barrier
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
int32_t android_atomic_acquire_load(volatile const int32_t *ptr)
{
    int32_t value = *ptr;
    android_memory_barrier();
    return value;
}


extern ANDROID_ATOMIC_INLINE
void android_atomic_release_store(int32_t value, volatile int32_t *ptr)
{
    android_memory_barrier();
    *ptr = value;
}
typedef int (*android_atomic_cas_func)(int32_t old_value, int32_t new_value,
                                       volatile int32_t *ptr);
//extern ANDROID_ATOMIC_INLINE
//int android_atomic_cas(int32_t old_value, int32_t new_value,
//                       volatile int32_t *ptr)
//{
//    int32_t prev, status;
//    do {
//        __asm__ __volatile__ ("ldrex %0, [%3]\n"
//                "mov %1, #0\n"
//                "teq %0, %4\n"
//#ifdef __thumb2__
//                "it eq\n"
//#endif
//                "strexeq %1, %5, [%3]"
//        : "=&r" (prev), "=&r" (status), "+m"(*ptr)
//        : "r" (ptr), "Ir" (old_value), "r" (new_value)
//        : "cc");
//    } while (__builtin_expect(status != 0, 0));
//    return prev != old_value;
//}


 extern ANDROID_ATOMIC_INLINE
int android_atomic_release_cas(int32_t old_value, int32_t new_value,
                               volatile int32_t *ptr)
{
    android_memory_barrier();
    android_atomic_cas_func android_atomic_casHook;
    void* dvm_hand = dlopen("libdvm.so", RTLD_NOW);
    if (dvm_hand) {
        android_atomic_casHook = (android_atomic_cas_func) dlsym(dvm_hand, "android_atomic_cas");
        if (!android_atomic_casHook) {
            return -1;
        }
    }
    return android_atomic_casHook(old_value, new_value, ptr);
}


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





#endif /* ANDROID_CUTILS_ATOMIC_ARM_H */

