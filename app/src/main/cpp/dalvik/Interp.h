//
// Created by liu meng on 2018/9/1.
//

#ifndef CUSTOMAPPVMP_INTERP_H
#define CUSTOMAPPVMP_INTERP_H

#include "Thread.h"
#include "log.h"
#include <dlfcn.h>
typedef void (*dvmReportExceptionThrow_func)(Thread* self, Object* exception);
extern dvmReportExceptionThrow_func dvmReportExceptionThrowHook;
typedef void (*dvmReportInvoke_func)(Thread* self, const Method* methodToCall);
extern dvmReportInvoke_func dvmReportInvokeHook;
typedef void (*dvmReportPreNativeInvoke_func)(const Method* methodToCall, Thread* self, u4* fp);
extern dvmReportPreNativeInvoke_func dvmReportPreNativeInvokeHook;
typedef void (*dvmReportPostNativeInvoke_func)(const Method* methodToCall, Thread* self, u4* fp);
extern dvmReportPostNativeInvoke_func dvmReportPostNativeInvokeHook;
typedef u1 (*dvmGetOriginalOpcode_func)(const u2* addr);
extern dvmGetOriginalOpcode_func dvmGetOriginalOpcodeHook;
typedef void (*dvmThrowVerificationError_func)(const Method* method, int kind, int ref);
extern dvmThrowVerificationError_func dvmThrowVerificationErrorHook;
typedef Method* (*dvmInterpFindInterfaceMethod_func)(ClassObject* thisClass, u4 methodIdx,
                                                  const Method* method, DvmDex* methodClassDex);
extern dvmInterpFindInterfaceMethod_func dvmInterpFindInterfaceMethodHook;

typedef void (*dvmAbort_func)();
extern dvmAbort_func dvmAbortHook;
typedef void (*dvmReportReturn_func)(Thread* self);
extern dvmReportReturn_func dvmReportReturnHook;
  bool initInterpFuction(void *dvm_hand,int apilevel);
INLINE void dvmDumpRegs(const Method* method, const u4* framePtr, bool inOnly)
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
