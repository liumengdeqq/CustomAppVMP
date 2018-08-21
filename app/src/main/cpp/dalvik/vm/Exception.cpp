#include "stdafx.h"
#include "Exception.h"
#include "Globals.h"
#include "Misc.h"
#include "Thread.h"
#include "Dalvik.h"

void dvmThrowNullPointerException(JNIEnv* env, const char* msg) {
    jclass clazz = env->FindClass("java/lang/NullPointerException");
    env->ThrowNew(clazz, msg);
    env->DeleteLocalRef(clazz);
}

void dvmThrowArrayIndexOutOfBoundsException(JNIEnv* env, int length, int index) {
    char* msg = (char*) calloc(100, 1);
    sprintf(msg, "length=%d; index=%d", length, index);
    jclass clazz = env->FindClass("java/lang/ArrayIndexOutOfBoundsException");
    env->ThrowNew(clazz, msg);
    env->DeleteLocalRef(clazz);
    free(msg);
}

void dvmThrowArithmeticException(JNIEnv* env, const char* msg) {
    jclass clazz = env->FindClass("java/lang/ArithmeticException");
    env->ThrowNew(clazz, msg);
    env->DeleteLocalRef(clazz);
}
void dvmThrowChainedException(ClassObject* excepClass, const char* msg,
                              Object* cause)
{
    Thread* self = dvmThreadSelf();
    Object* exception;

    if (excepClass == NULL) {
        /*
         * The exception class was passed in as NULL. This might happen
         * early on in VM initialization. There's nothing better to do
         * than just log the message as an error and abort.
         */
        ALOGE("Fatal error: %s", msg);
        dvmAbort();
    }

    /* make sure the exception is initialized */
    if (!dvmIsClassInitialized(excepClass) && !dvmInitClass(excepClass)) {
        ALOGE("ERROR: unable to initialize exception class '%s'",
              excepClass->descriptor);
        if (strcmp(excepClass->descriptor, "Ljava/lang/InternalError;") == 0)
            dvmAbort();
        dvmThrowChainedException(gDvm.exInternalError,
                                 "failed to init original exception class", cause);
        return;
    }

    exception = dvmAllocObject(excepClass, ALLOC_DEFAULT);
    if (exception == NULL) {
        /*
         * We're in a lot of trouble.  We might be in the process of
         * throwing an out-of-memory exception, in which case the
         * pre-allocated object will have been thrown when our object alloc
         * failed.  So long as there's an exception raised, return and
         * allow the system to try to recover.  If not, something is broken
         * and we need to bail out.
         */
        if (dvmCheckException(self))
            goto bail;
        ALOGE("FATAL: unable to allocate exception '%s' '%s'",
              excepClass->descriptor, msg != NULL ? msg : "(no msg)");
        dvmAbort();
    }

    /*
     * Init the exception.
     */
    if (gDvm.optimizing) {
        /* need the exception object, but can't invoke interpreted code */
        ALOGV("Skipping init of exception %s '%s'",
              excepClass->descriptor, msg);
    } else {
        assert(excepClass == exception->clazz);
        if (!initException(exception, msg, cause, self)) {
            /*
             * Whoops.  If we can't initialize the exception, we can't use
             * it.  If there's an exception already set, the constructor
             * probably threw an OutOfMemoryError.
             */
            if (!dvmCheckException(self)) {
                /*
                 * We're required to throw something, so we just
                 * throw the pre-constructed internal error.
                 */
                self->exception = gDvm.internalErrorObj;
            }
            goto bail;
        }
    }

    self->exception = exception;

    bail:
    dvmReleaseTrackedAlloc(exception, self);
}
void dvmThrowChainedExceptionWithClassMessage(
        ClassObject* exceptionClass, const char* messageDescriptor,
        Object* cause)
{
    char* message = dvmDescriptorToName(messageDescriptor);

    dvmThrowChainedException(exceptionClass, message, cause);
    free(message);
}
INLINE void dvmThrowExceptionWithClassMessage(
        ClassObject* exceptionClass, const char* messageDescriptor)
{
    dvmThrowChainedExceptionWithClassMessage(exceptionClass,
                                             messageDescriptor, NULL);
}

void dvmThrowNoClassDefFoundError(const char* descriptor) {
    dvmThrowExceptionWithClassMessage(gDvm.exNoClassDefFoundError,
                                      descriptor);
}
