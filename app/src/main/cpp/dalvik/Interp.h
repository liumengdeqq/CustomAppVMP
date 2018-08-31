//
// Created by liu meng on 2018/9/1.
//

#ifndef CUSTOMAPPVMP_INTERP_H
#define CUSTOMAPPVMP_INTERP_H

#include "log.h"
typedef void (*dvmReportExceptionThrow_func)(Thread* self, Object* exception);
dvmReportExceptionThrow_func dvmReportExceptionThrowHook;
bool initInterpFuction(void *dvm_hand,int apilevel);
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
#endif

#endif //CUSTOMAPPVMP_INTERP_H
