//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_DEXPROTO_H
#define CUSTOMAPPVMP_DEXPROTO_H

#include "DexFile.h"
struct DexProto {
    const DexFile* dexFile;     /* file the idx refers to */
    u4 protoIdx;                /* index into proto_ids table of dexFile */
};
#endif //CUSTOMAPPVMP_DEXPROTO_H
