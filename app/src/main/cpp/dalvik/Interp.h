//
// Created by liu meng on 2018/9/1.
//

#ifndef CUSTOMAPPVMP_INTERP_H
#define CUSTOMAPPVMP_INTERP_H

#include "Thread.h"
#include "log.h"
#include <dlfcn.h>
typedef void (*dvmReportExceptionThrow_func)(Thread* self, Object* exception);
 dvmReportExceptionThrow_func dvmReportExceptionThrowHook;
typedef void (*dvmReportInvoke_func)(Thread* self, const Method* methodToCall);
 dvmReportInvoke_func dvmReportInvokeHook;
typedef void (*dvmReportPreNativeInvoke_func)(const Method* methodToCall, Thread* self, u4* fp);
 dvmReportPreNativeInvoke_func dvmReportPreNativeInvokeHook;
typedef void (*dvmReportPostNativeInvoke_func)(const Method* methodToCall, Thread* self, u4* fp);
 dvmReportPostNativeInvoke_func dvmReportPostNativeInvokeHook;
typedef u1 (*dvmGetOriginalOpcode_func)(const u2* addr);
dvmGetOriginalOpcode_func dvmGetOriginalOpcodeHook;
typedef void (*dvmThrowVerificationError_func)(const Method* method, int kind, int ref);
dvmThrowVerificationError_func dvmThrowVerificationErrorHook;
typedef Method* (*dvmInterpFindInterfaceMethod_func)(ClassObject* thisClass, u4 methodIdx,
                                                  const Method* method, DvmDex* methodClassDex);
 dvmInterpFindInterfaceMethod_func dvmInterpFindInterfaceMethodHook;

typedef void (*dvmAbort_func)();
dvmAbort_func dvmAbortHook;
typedef void (*dvmReportReturn_func)(Thread* self);
dvmReportReturn_func dvmReportReturnHook;
static  bool initInterpFuction(void *dvm_hand,int apilevel){


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
void dvmDumpRegs(const Method* method, const u4* framePtr, bool inOnly)
{
    int i, localCount;

    localCount = method->registersSize - method->insSize;

    MY_LOG_INFO("i", "Registers (fp=%p):", framePtr);
    for (i = method->registersSize-1; i >= 0; i--) {
        if (i >= localCount) {
            MY_LOG_INFO("i", "  v%-2d in%-2d : 0x%08x",
                    i, i-localCount, framePtr[i]);
        } else {
            if (inOnly) {
                MY_LOG_INFO("i", "  [...]");
                break;
            }
            const char* name = "";
#if 0   // "locals" structure has changed -- need to rewrite this
            int j;
            DexFile* pDexFile = method->clazz->pDexFile;
            const DexCode* pDexCode = dvmGetMethodCode(method);
            int localsSize = dexGetLocalsSize(pDexFile, pDexCode);
            const DexLocal* locals = dvmDexGetLocals(pDexFile, pDexCode);
            for (j = 0; j < localsSize, j++) {
                if (locals[j].registerNum == (u4) i) {
                    name = dvmDexStringStr(locals[j].pName);
                    break;
                }
            }
#endif
            MY_LOG_INFO("i", "  v%-2d      : 0x%08x %s",
                    i, framePtr[i], name);
        }
    }
}
#endif //CUSTOMAPPVMP_INTERP_H
