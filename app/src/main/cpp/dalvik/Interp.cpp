//
// Created by liu meng on 2018/9/1.
//

#include "Interp.h"
#include <dlfcn.h>
#include <jni.h>
bool initInterpFuction(void *dvm_hand,int apilevel){


    if (dvm_hand) {
        dvmReportExceptionThrowHook = (dvmReportExceptionThrow_func)dlsym(dvm_hand,"dvmReportExceptionThrow");
        if (!dvmReportExceptionThrowHook) {
            return JNI_FALSE;
        }
        dvmReportInvokeHook = (dvmReportInvoke_func)dlsym(dvm_hand,"dvmReportInvoke");
        if (!dvmReportInvokeHook) {
            return JNI_FALSE;
        }
        dvmReportPreNativeInvokeHook = (dvmReportPreNativeInvoke_func)dlsym(dvm_hand,"dvmReportPreNativeInvoke");
        if (!dvmReportPreNativeInvokeHook) {
            return JNI_FALSE;
        }
        dvmReportPostNativeInvokeHook = (dvmReportPostNativeInvoke_func)dlsym(dvm_hand,"dvmReportPostNativeInvoke");
        if (!dvmReportPostNativeInvokeHook) {
            return JNI_FALSE;
        }
        dvmGetOriginalOpcodeHook = (dvmGetOriginalOpcode_func)dlsym(dvm_hand,"dvmGetOriginalOpcode");
        if (!dvmGetOriginalOpcodeHook) {
            return JNI_FALSE;
        }
        dvmThrowVerificationErrorHook = (dvmThrowVerificationError_func)dlsym(dvm_hand,"dvmThrowVerificationError");
        if (!dvmThrowVerificationErrorHook) {
            return JNI_FALSE;
        }
        dvmInterpFindInterfaceMethodHook=(dvmInterpFindInterfaceMethod_func)dlsym(dvm_hand,"dvmInterpFindInterfaceMethod");
        if (!dvmInterpFindInterfaceMethodHook) {
            return JNI_FALSE;
        }
        dvmAbortHook=(dvmAbort_func)dlsym(dvm_hand,"dvmAbort");
        if (!dvmAbortHook) {
            return JNI_FALSE;
        }
        dvmReportReturnHook=(dvmReportReturn_func)dlsym(dvm_hand,"dvmReportReturn");
        if (!dvmReportReturnHook) {
            return JNI_FALSE;
        }
        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}