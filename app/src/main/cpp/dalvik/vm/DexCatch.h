//
// Created by liu meng on 2018/8/23.
//
#include "common.h"
#include "DexFile.h"
#include "Leb128.h"
#ifndef CUSTOMAPPVMP_DEXCATCH_H
#define CUSTOMAPPVMP_DEXCATCH_H
struct DexCatchHandler {
    u4          typeIdx;    /* type index of the caught exception type */
    u4          address;    /* handler address */
};

struct DexCatchIterator {
    const u1* pEncodedData;
    bool catchesAll;
    u4 countRemaining;
    DexCatchHandler handler;
};
DEX_INLINE void dexCatchIteratorInitToPointer(DexCatchIterator* pIterator,
                                              const u1* pEncodedData)
{
    s4 count = readSignedLeb128(&pEncodedData);

    if (count <= 0) {
        pIterator->catchesAll = true;
        count = -count;
    } else {
        pIterator->catchesAll = false;
    }

    pIterator->pEncodedData = pEncodedData;
    pIterator->countRemaining = count;
}
DEX_INLINE DexCatchHandler* dexCatchIteratorNext(DexCatchIterator* pIterator) {
    if (pIterator->countRemaining == 0) {
        if (! pIterator->catchesAll) {
            return NULL;
        }

        pIterator->catchesAll = false;
        pIterator->handler.typeIdx = kDexNoIndex;
    } else {
        u4 typeIdx = readUnsignedLeb128(&pIterator->pEncodedData);
        pIterator->handler.typeIdx = typeIdx;
        pIterator->countRemaining--;
    }

    pIterator->handler.address = readUnsignedLeb128(&pIterator->pEncodedData);
    return &pIterator->handler;
}
/* Initialize a DexCatchIterator to a particular handler offset. */
DEX_INLINE void dexCatchIteratorInit(DexCatchIterator* pIterator,
                                     const DexCode* pCode, u4 offset)
{
    dexCatchIteratorInitToPointer(pIterator,
                                  dexGetCatchHandlerData(pCode) + offset);
}
u4 dexCatchIteratorGetEndOffset(DexCatchIterator* pIterator,
                                const DexCode* pCode) {
    while (dexCatchIteratorNext(pIterator) != NULL) /* empty */ ;

    return (u4) (pIterator->pEncodedData - dexGetCatchHandlerData(pCode));
}

#endif //CUSTOMAPPVMP_DEXCATCH_H
