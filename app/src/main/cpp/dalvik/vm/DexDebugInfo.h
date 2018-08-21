//
// Created by liu meng on 2018/8/21.
//
#include "common.h"
#ifndef CUSTOMAPPVMP_DEXDEBUGINFO_H
#define CUSTOMAPPVMP_DEXDEBUGINFO_H
typedef int (*DexDebugNewPositionCb)(void *cnxt, u4 address, u4 lineNum);
typedef void (*DexDebugNewLocalCb)(void *cnxt, u2 reg, u4 startAddress,
                                   u4 endAddress, const char *name, const char *descriptor,
                                   const char *signature);

#endif //CUSTOMAPPVMP_DEXDEBUGINFO_H
