//
// Created by liu meng on 2018/8/21.
//
#include "DexDebugInfo.h"
#include <unistd.h>
#include "DexProto.h"
#include "Leb128.h"
#include "log.h"
#include <malloc.h>
struct LocalInfo {
    const char *name;
    const char *descriptor;
    const char *signature;
    u2 startAddress;
    bool live;
};
static void emitLocalCbIfLive(void *cnxt, int reg, u4 endAddress,
                              LocalInfo *localInReg, DexDebugNewLocalCb localCb)
{
    if (localCb != NULL && localInReg[reg].live) {
        localCb(cnxt, reg, localInReg[reg].startAddress, endAddress,
                localInReg[reg].name,
                localInReg[reg].descriptor,
                localInReg[reg].signature == NULL
                ? "" : localInReg[reg].signature );
    }
}

static void invalidStream(const char* classDescriptor, const DexProto* proto) {
        char* methodDescriptor = dexProtoCopyMethodDescriptor(proto);
        ALOGE("Invalid debug info stream. class %s; proto %s",
              classDescriptor, methodDescriptor);
        free(methodDescriptor);
}
static const char* readStringIdx(const DexFile* pDexFile,
                                 const u1** pStream) {
    u4 stringIdx = readUnsignedLeb128(pStream);

    // Remember, encoded string indicies have 1 added to them.
    if (stringIdx == 0) {
        return NULL;
    } else {
        return dexStringById(pDexFile, stringIdx - 1);
    }
}
static const char* readTypeIdx(const DexFile* pDexFile,
                               const u1** pStream) {
    u4 typeIdx = readUnsignedLeb128(pStream);

    // Remember, encoded type indicies have 1 added to them.
    if (typeIdx == 0) {
        return NULL;
    } else {
        return dexStringByTypeIdx(pDexFile, typeIdx - 1);
    }
}

static void dexDecodeDebugInfo0(
        const DexFile* pDexFile,
        const DexCode* pCode,
        const char* classDescriptor,
        u4 protoIdx,
        u4 accessFlags,
        DexDebugNewPositionCb posCb, DexDebugNewLocalCb localCb,
        void* cnxt,
        const u1* stream,
        LocalInfo* localInReg)
{
    DexProto proto = { pDexFile, protoIdx };
    u4 insnsSize = pCode->insnsSize;
    u4 line = readUnsignedLeb128(&stream);
    u4 parametersSize = readUnsignedLeb128(&stream);
    u2 argReg = pCode->registersSize - pCode->insSize;
    u4 address = 0;

    if ((accessFlags & ACC_STATIC) == 0) {
        /*
         * The code is an instance method, which means that there is
         * an initial this parameter. Also, the proto list should
         * contain exactly one fewer argument word than the insSize
         * indicates.
         */
        assert(pCode->insSize == (dexProtoComputeArgsSize(&proto) + 1));
        localInReg[argReg].name = "this";
        localInReg[argReg].descriptor = classDescriptor;
        localInReg[argReg].startAddress = 0;
        localInReg[argReg].live = true;
        argReg++;
    } else {
        assert(pCode->insSize == dexProtoComputeArgsSize(&proto));
    }

    DexParameterIterator iterator;
    dexParameterIteratorInit(&iterator, &proto);

    while (parametersSize-- != 0) {
        const char* descriptor = dexParameterIteratorNextDescriptor(&iterator);
        const char *name;
        int reg;

        if ((argReg >= pCode->registersSize) || (descriptor == NULL)) {
            invalidStream(classDescriptor, &proto);
            return;
        }

        name = readStringIdx(pDexFile, &stream);
        reg = argReg;

        switch (descriptor[0]) {
            case 'D':
            case 'J':
                argReg += 2;
                break;
            default:
                argReg += 1;
                break;
        }

        if (name != NULL) {
            localInReg[reg].name = name;
            localInReg[reg].descriptor = descriptor;
            localInReg[reg].signature = NULL;
            localInReg[reg].startAddress = address;
            localInReg[reg].live = true;
        }
    }

    for (;;)  {
        u1 opcode = *stream++;
        u2 reg;

        switch (opcode) {
            case DBG_END_SEQUENCE:
                return;

            case DBG_ADVANCE_PC:
                address += readUnsignedLeb128(&stream);
                break;

            case DBG_ADVANCE_LINE:
                line += readSignedLeb128(&stream);
                break;

            case DBG_START_LOCAL:
            case DBG_START_LOCAL_EXTENDED:
                reg = readUnsignedLeb128(&stream);
                if (reg > pCode->registersSize) {
                    invalidStream(classDescriptor, &proto);
                    return;
                }

                // Emit what was previously there, if anything
                emitLocalCbIfLive(cnxt, reg, address,
                                  localInReg, localCb);

                localInReg[reg].name = readStringIdx(pDexFile, &stream);
                localInReg[reg].descriptor = readTypeIdx(pDexFile, &stream);
                if (opcode == DBG_START_LOCAL_EXTENDED) {
                    localInReg[reg].signature
                            = readStringIdx(pDexFile, &stream);
                } else {
                    localInReg[reg].signature = NULL;
                }
                localInReg[reg].startAddress = address;
                localInReg[reg].live = true;
                break;

            case DBG_END_LOCAL:
                reg = readUnsignedLeb128(&stream);
                if (reg > pCode->registersSize) {
                    invalidStream(classDescriptor, &proto);
                    return;
                }

                emitLocalCbIfLive (cnxt, reg, address, localInReg, localCb);
                localInReg[reg].live = false;
                break;

            case DBG_RESTART_LOCAL:
                reg = readUnsignedLeb128(&stream);
                if (reg > pCode->registersSize) {
                    invalidStream(classDescriptor, &proto);
                    return;
                }

                if (localInReg[reg].name == NULL
                    || localInReg[reg].descriptor == NULL) {
                    invalidStream(classDescriptor, &proto);
                    return;
                }

                /*
                 * If the register is live, the "restart" is superfluous,
                 * and we don't want to mess with the existing start address.
                 */
                if (!localInReg[reg].live) {
                    localInReg[reg].startAddress = address;
                    localInReg[reg].live = true;
                }
                break;

            case DBG_SET_PROLOGUE_END:
            case DBG_SET_EPILOGUE_BEGIN:
            case DBG_SET_FILE:
                break;

            default: {
                int adjopcode = opcode - DBG_FIRST_SPECIAL;

                address += adjopcode / DBG_LINE_RANGE;
                line += DBG_LINE_BASE + (adjopcode % DBG_LINE_RANGE);

                if (posCb != NULL) {
                    int done;
                    done = posCb(cnxt, address, line);

                    if (done) {
                        // early exit
                        return;
                    }
                }
                break;
            }
        }
    }
}

void dexDecodeDebugInfo(
        const DexFile* pDexFile,
        const DexCode* pCode,
        const char* classDescriptor,
        u4 protoIdx,
        u4 accessFlags,
        DexDebugNewPositionCb posCb, DexDebugNewLocalCb localCb,
        void* cnxt)
{
    const u1* stream = dexGetDebugInfoStream(pDexFile, pCode);
    LocalInfo localInReg[pCode->registersSize];

    memset(localInReg, 0, sizeof(LocalInfo) * pCode->registersSize);

    if (stream != NULL) {
        dexDecodeDebugInfo0(pDexFile, pCode, classDescriptor, protoIdx, accessFlags,
                            posCb, localCb, cnxt, stream, localInReg);
    }

    for (int reg = 0; reg < pCode->registersSize; reg++) {
        emitLocalCbIfLive(cnxt, reg, pCode->insnsSize, localInReg, localCb);
    }
}
