//
// Created by liu meng on 2018/7/28.
//

#ifndef DUMPDEX_DEXPROTO_H
#define DUMPDEX_DEXPROTO_H

#include "DexFile.h"
struct DexProto {
    const DexFile* dexFile;     /* file the idx refers to */
    u4 protoIdx;                /* index into proto_ids table of dexFile */
};


struct DexParameterIterator {
    const DexProto* proto;
    const DexTypeList* parameters;
    int parameterCount;
    int cursor;
};
struct DexStringCache {
    char* value;          /* the latest value */
    size_t allocatedSize; /* size of the allocated buffer, if allocated */
    char buffer[120];     /* buffer used to hold small-enough results */
};
char* dexStringCacheAbandon(DexStringCache* pCache, const char* value);
const char* dexProtoGetMethodDescriptor(const DexProto* pProto,
                                        DexStringCache* pCache);
int dexProtoComputeArgsSize(const DexProto* pProto);
const char* dexParameterIteratorNextDescriptor(
        DexParameterIterator* pIterator);
void dexParameterIteratorInit(DexParameterIterator* pIterator,
                              const DexProto* pProto);
char* dexProtoCopyMethodDescriptor(const DexProto* pProto);
static inline const DexProtoId* getProtoId(const DexProto* pProto) {
    return dexGetProtoId(pProto->dexFile, pProto->protoIdx);
}

const char* dexProtoGetReturnType(const DexProto* pProto) {
    const DexProtoId* protoId = getProtoId(pProto);
    return dexStringByTypeIdx(pProto->dexFile, protoId->returnTypeIdx);
}
size_t dexProtoGetParameterCount(const DexProto* pProto) {
    const DexProtoId* protoId = getProtoId(pProto);
    const DexTypeList* typeList =
            dexGetProtoParameters(pProto->dexFile, protoId);
    return (typeList == NULL) ? 0 : typeList->size;
}
#endif //DUMPDEX_DEXPROTO_H
