//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_CARDTABLE_H
#define CUSTOMAPPVMP_CARDTABLE_H

#include <dlfcn.h>
#include <jni.h>
#include "base.h"
typedef void (*dvmMarkCard_func)(const void *addr);
extern dvmMarkCard_func dvmMarkCardHook;
bool initCarTableFuction(void *dvm_hand,int apilevel);
#endif //CUSTOMAPPVMP_CARDTABLE_H
