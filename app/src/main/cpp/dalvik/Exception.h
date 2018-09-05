/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Exception handling.
 */
#ifndef DALVIK_EXCEPTION_H_
#define DALVIK_EXCEPTION_H_


#include "Thread.h"
#include <malloc.h>
#include <dlfcn.h>
typedef int (*dvmFindCatchBlock_func)(Thread* self, int relPc, Object* exception,
                                       bool scanOnly, void** newFrame);
extern dvmFindCatchBlock_func dvmFindCatchBlockHook;



INLINE bool dvmCheckException(Thread* self) {
    return (self->exception != NULL);
}
INLINE void dvmThrowClassCastException(struct ClassObject* actual, struct ClassObject* desired)
{

}
INLINE void dvmThrowNegativeArraySizeException(s4 size) {
//    dvmThrowExceptionFmt(gDvm.exNegativeArraySizeException, "%d", size);
}
INLINE void dvmThrowRuntimeException(const char* msg){

}
INLINE void dvmThrowInternalError(const char* msg){

}
INLINE void dvmSetException(struct Thread* self, struct Object* exception)
{
    assert(exception != NULL);
    self->exception = exception;
}
INLINE Object* dvmGetException(Thread* self) {
    return self->exception;
}
INLINE void dvmClearException(Thread* self) {
    self->exception = NULL;
}
INLINE void dvmThrowNoSuchMethodError(const char* msg) {
//    dvmThrowException(gDvm.exNoSuchMethodError, msg);
}

INLINE void dvmThrowArrayStoreExceptionIncompatibleElement(ClassObject* objectType,
                                                    ClassObject* arrayType)
{
//    throwTypeError(gDvm.exArrayStoreException,
//                   "%s cannot be stored in an array of type %s",
//                   objectType, arrayType);
}
INLINE void dvmThrowStringIndexOutOfBoundsExceptionWithIndex(jsize stringLength,
                                                      jsize requestIndex){

}
INLINE void dvmThrowNullPointerException(JNIEnv* env, const char* msg) {
    jclass clazz = env->FindClass("java/lang/NullPointerException");
    env->ThrowNew(clazz, msg);
    env->DeleteLocalRef(clazz);
}

INLINE void dvmThrowArrayIndexOutOfBoundsException(JNIEnv* env, int length, int index) {
    char* msg = (char*) calloc(100, 1);
    sprintf(msg, "length=%d; index=%d", length, index);
    jclass clazz = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
    env->ThrowNew(clazz, msg);
    env->DeleteLocalRef(clazz);
    free(msg);
}

INLINE void dvmThrowArithmeticException(JNIEnv* env, const char* msg) {
    jclass clazz = env->FindClass("java/lang/ArithmeticException");
    env->ThrowNew(clazz, msg);
    env->DeleteLocalRef(clazz);
}
 bool initExceptionFuction(void *dvm_hand,int apilevel);
#endif  // DALVIK_EXCEPTION_H_
