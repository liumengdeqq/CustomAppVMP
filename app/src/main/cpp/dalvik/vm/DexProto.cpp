//
// Created by liu meng on 2018/8/21.
//
#include "DexProto.h"
#include <string.h>
#include <unistd.h>
#include <malloc.h>
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
void dexParameterIteratorInit(DexParameterIterator* pIterator,
                              const DexProto* pProto) {
    pIterator->proto = pProto;
    pIterator->cursor = 0;

    pIterator->parameters =
            dexGetProtoParameters(pProto->dexFile, getProtoId(pProto));
    pIterator->parameterCount = (pIterator->parameters == NULL) ? 0
                                                                : pIterator->parameters->size;
}
void dexStringCacheInit(DexStringCache* pCache) {
    pCache->value = pCache->buffer;
    pCache->allocatedSize = 0;
    pCache->buffer[0] = '\0';
}
void dexStringCacheAlloc(DexStringCache* pCache, size_t length) {
    if (pCache->allocatedSize != 0) {
        if (pCache->allocatedSize >= length) {
            return;
        }
        free((void*) pCache->value);
    }

    if (length <= sizeof(pCache->buffer)) {
        pCache->value = pCache->buffer;
        pCache->allocatedSize = 0;
    } else {
        pCache->value = (char*) malloc(length);
        pCache->allocatedSize = length;
    }
}

const char* dexProtoGetMethodDescriptor(const DexProto* pProto,
                                        DexStringCache* pCache) {
    const DexFile* dexFile = pProto->dexFile;
    const DexProtoId* protoId = getProtoId(pProto);
    const DexTypeList* typeList = dexGetProtoParameters(dexFile, protoId);
    size_t length = 3; // parens and terminating '\0'
    u4 paramCount = (typeList == NULL) ? 0 : typeList->size;
    u4 i;

    for (i = 0; i < paramCount; i++) {
        u4 idx = dexTypeListGetIdx(typeList, i);
        length += strlen(dexStringByTypeIdx(dexFile, idx));
    }

    length += strlen(dexStringByTypeIdx(dexFile, protoId->returnTypeIdx));

    dexStringCacheAlloc(pCache, length);

    char *at = (char*) pCache->value;
    *(at++) = '(';

    for (i = 0; i < paramCount; i++) {
        u4 idx = dexTypeListGetIdx(typeList, i);
        const char* desc = dexStringByTypeIdx(dexFile, idx);
        strcpy(at, desc);
        at += strlen(desc);
    }

    *(at++) = ')';

    strcpy(at, dexStringByTypeIdx(dexFile, protoId->returnTypeIdx));
    return pCache->value;
}

char* dexProtoCopyMethodDescriptor(const DexProto* pProto) {
    DexStringCache cache;

    dexStringCacheInit(&cache);
    return dexStringCacheAbandon(&cache,
                                 dexProtoGetMethodDescriptor(pProto, &cache));
}
