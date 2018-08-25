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
void dvmThrowNullPointerException(JNIEnv* env, const char* msg);

void dvmThrowArrayIndexOutOfBoundsException(JNIEnv* env, int length, int index);
void dvmThrowChainedExceptionWithClassMessage(
        ClassObject* exceptionClass, const char* messageDescriptor,
        Object* cause);
void dvmThrowArithmeticException(JNIEnv* env, const char* msg);
INLINE void dvmThrowExceptionWithClassMessage(
        ClassObject* exceptionClass, const char* messageDescriptor)
{
    dvmThrowChainedExceptionWithClassMessage(exceptionClass,
                                             messageDescriptor, NULL);
}
INLINE bool dvmCheckException(Thread* self) {
    return (self->exception != NULL);
}
void dvmThrowExceptionFmtV(ClassObject* exceptionClass,
                           const char* fmt, va_list args);
void dvmThrowExceptionFmt(ClassObject* exceptionClass,
                          const char* fmt, ...)
#if defined(__GNUC__)
__attribute__ ((format(printf, 2, 3)))
#endif
;

INLINE void dvmThrowExceptionFmt(ClassObject* exceptionClass,
                                 const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    dvmThrowExceptionFmtV(exceptionClass, fmt, args);
    va_end(args);
}
#endif  // DALVIK_EXCEPTION_H_
