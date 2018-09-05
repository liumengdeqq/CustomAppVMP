//
// Created by liu meng on 2018/8/31.
//


#include "Stack.h"

 dvmHandleStackOverflow_func dvmHandleStackOverflowhook;

 dvmLineNumFromPC_func dvmLineNumFromPChook;

 dvmCleanupStackOverflow_func dvmCleanupStackOverflowhook;


bool initStackFuction(void * dvm_hand,int apilevel){
    if (dvm_hand) {
        dvmHandleStackOverflowhook = (dvmHandleStackOverflow_func)dlsym(dvm_hand,"dvmHandleStackOverflow");
        if (!dvmHandleStackOverflowhook) {
            return JNI_FALSE;
        }
        dvmLineNumFromPChook = (dvmLineNumFromPC_func)dlsym(dvm_hand,"dvmLineNumFromPC");
        if (!dvmLineNumFromPChook) {
            return JNI_FALSE;
        }
        dvmCleanupStackOverflowhook = (dvmCleanupStackOverflow_func)dlsym(dvm_hand,"dvmCleanupStackOverflow");
        if (!dvmCleanupStackOverflowhook) {
            return JNI_FALSE;
        }

        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}
