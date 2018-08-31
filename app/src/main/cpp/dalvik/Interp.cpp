//
// Created by liu meng on 2018/9/1.
//

#include "Interp.h"
#include <dlfcn.h>
#include <jni.h>
bool initInterpFuction(void *dvm_hand,int apilevel){


    if (dvm_hand) {
        dvmReportExceptionThrowHook = (dvmReportExceptionThrow_func)dlsym(dvm_hand,"dvmReportExceptionThrow");
        if (!dvmReportExceptionThrowHook) {
            return JNI_FALSE;
        }

        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}