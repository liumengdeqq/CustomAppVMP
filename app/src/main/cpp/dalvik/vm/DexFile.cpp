//
// Created by liu meng on 2018/8/20.
//
#include "DexFile.h"
#include "Leb128.h"
const char* dexStringAndSizeById(const DexFile* pDexFile, u4 idx,
                                 u4* utf16Size) {
    const DexStringId* pStringId = dexGetStringId(pDexFile, idx);
    const u1* ptr = pDexFile->baseAddr + pStringId->stringDataOff;

    *utf16Size = readUnsignedLeb128(&ptr);
    return (const char*) ptr;
}

