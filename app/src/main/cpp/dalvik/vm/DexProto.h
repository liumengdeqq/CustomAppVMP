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
#endif //DUMPDEX_DEXPROTO_H
