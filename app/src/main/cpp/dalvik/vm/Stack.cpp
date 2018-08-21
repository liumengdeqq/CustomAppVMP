//
// Created by liu meng on 2018/8/21.
//
#include "Stack.h"
#include <unistd.h>
#include "DexDebugInfo.h"
struct LineNumFromPcContext {
    u4 address;
    u4 lineNum;
};
static int lineNumForPcCb(void *cnxt, u4 address, u4 lineNum)
{
    LineNumFromPcContext *pContext = (LineNumFromPcContext *)cnxt;

    // We know that this callback will be called in
    // ascending address order, so keep going until we find
    // a match or we've just gone past it.

    if (address > pContext->address) {
        // The line number from the previous positions callback
        // wil be the final result.
        return 1;
    }

    pContext->lineNum = lineNum;

    return (address == pContext->address) ? 1 : 0;
}

int dvmLineNumFromPC(const Method* method, u4 relPc)
{
    const DexCode* pDexCode = dvmGetMethodCode(method);

    if (pDexCode == NULL) {
        if (dvmIsNativeMethod(method) && !dvmIsAbstractMethod(method))
            return -2;
        return -1;      /* can happen for abstract method stub */
    }

    LineNumFromPcContext context;
    memset(&context, 0, sizeof(context));
    context.address = relPc;
    // A method with no line number info should return -1
    context.lineNum = -1;

    dexDecodeDebugInfo(method->clazz->pDvmDex->pDexFile, pDexCode,
                       method->clazz->descriptor,
                       method->prototype.protoIdx,
                       method->accessFlags,
                       lineNumForPcCb, NULL, &context);

    return context.lineNum;
}