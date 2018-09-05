//
// Created by liu meng on 2018/8/31.
//


#include "CardTable.h"
dvmMarkCard_func dvmMarkCardHook;
bool initCarTableFuction(void *dvm_hand,int apilevel){
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
