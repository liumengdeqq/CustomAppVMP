//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_CARDTABLE_H
#define CUSTOMAPPVMP_CARDTABLE_H

#include <dlfcn.h>
#include <jni.h>
#include "base.h"
typedef void (*dvmMarkCard_func)(const void *addr);
dvmMarkCard_func dvmMarkCardHook;
static  bool initCarTableFuction(void *dvm_hand,int apilevel){
    if (dvm_hand) {
        dvmMarkCardHook = (dvmMarkCard_func)dlsym(dvm_hand,"dvmMarkCard");
        if (!dvmMarkCardHook) {
            return JNI_FALSE;
        }
        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}
#endif //CUSTOMAPPVMP_CARDTABLE_H
