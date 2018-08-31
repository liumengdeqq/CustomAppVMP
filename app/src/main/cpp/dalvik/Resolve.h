//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_RESOLVE_H
#define CUSTOMAPPVMP_RESOLVE_H

#include "Object.h"
typedef StringObject* (*dvmResolveString_func)(const ClassObject* referrer, u4 stringIdx);
typedef ClassObject* (*dvmResolveClass_func)(const ClassObject* referrer, u4 classIdx,bool fromUnverifiedConstant);

dvmResolveString_func dvmResolveStringhook;
dvmResolveClass_func dvmResolveClasshook;
bool initResolveFuction(void * dvm_hand,int apilevel);
#endif //CUSTOMAPPVMP_RESOLVE_H
