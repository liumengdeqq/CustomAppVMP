//
// Created by liu meng on 2018/8/21.
//
#include "DexProto.h"
static inline const DexProtoId* getProtoId(const DexProto* pProto) {
    return dexGetProtoId(pProto->dexFile, pProto->protoIdx);
}

const char* dexProtoGetShorty(const DexProto* pProto) {
    const DexProtoId* protoId = getProtoId(pProto);

    return dexStringById(pProto->dexFile, protoId->shortyIdx);
}

int dexProtoComputeArgsSize(const DexProto* pProto) {
    const char* shorty = dexProtoGetShorty(pProto);
    int count = 0;

    /* Skip the return type. */
    shorty++;

    for (;;) {
        switch (*(shorty++)) {
            case '\0': {
                return count;
            }
            case 'D':
            case 'J': {
                count += 2;
                break;
            }
            default: {
                count++;
                break;
            }
        }
    }
}
u4 dexParameterIteratorNextIndex(DexParameterIterator* pIterator) {
    int cursor = pIterator->cursor;
    int parameterCount = pIterator->parameterCount;

    if (cursor >= parameterCount) {
        // The iteration is complete.
        return kDexNoIndex;
    } else {
        u4 idx = dexTypeListGetIdx(pIterator->parameters, cursor);
        pIterator->cursor++;
        return idx;
    }
}

const char* dexParameterIteratorNextDescriptor(
        DexParameterIterator* pIterator) {
    u4 idx = dexParameterIteratorNextIndex(pIterator);

    if (idx == kDexNoIndex) {
        return NULL;
    }

    return dexStringByTypeIdx(pIterator->proto->dexFile, idx);
}
