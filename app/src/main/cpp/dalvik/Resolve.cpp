//
// Created by liu meng on 2018/8/31.
//

#include "Resolve.h"

bool initResolveFuction(void * dvm_hand,int apilevel){
    if (dvm_hand) {
        dvmResolveStringhook = (dvmResolveString_func)dlsym(dvm_hand,"dvmResolveString");
        if (!dvmResolveStringhook) {
            return JNI_FALSE;
        }
        dvmResolveClasshook = (dvmResolveClass_func)dlsym(dvm_hand,"dvmResolveClass");
        if (!dvmResolveClasshook) {
            return JNI_FALSE;
        }
        dvmResolveMethodhook = (dvmResolveMethod_func)dlsym(dvm_hand,"dvmResolveMethod");
        if (!dvmResolveMethodhook) {
            return JNI_FALSE;
        }
        dvmResolveInstFieldhook = (dvmResolveInstField_func)dlsym(dvm_hand,"dvmResolveInstField");
        if (!dvmResolveInstFieldhook) {
            return JNI_FALSE;
        }
        dvmResolveStaticFieldhook = (dvmResolveStaticField_func)dlsym(dvm_hand,"dvmResolveStaticField");
        if (!dvmResolveStaticFieldhook) {
            return JNI_FALSE;
        }

        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}
