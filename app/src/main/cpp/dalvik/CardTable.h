//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_CARDTABLE_H
#define CUSTOMAPPVMP_CARDTABLE_H

#include "Object.h"
typedef void (*dvmMarkCard_func)(const void *addr);
dvmMarkCard_func dvmMarkCardHook;
bool initCarTableFuction(void *dvm_hand,int apilevel);
#endif //CUSTOMAPPVMP_CARDTABLE_H
