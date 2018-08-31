//
// Created by liu meng on 2018/9/1.
//

#include "Stack.h"
#include <dlfcn.h>
#include <jni.h>
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