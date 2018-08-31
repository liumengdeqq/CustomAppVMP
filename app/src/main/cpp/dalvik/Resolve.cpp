//
// Created by liu meng on 2018/8/31.
//

#include "Resolve.h"
#include <dlfcn.h>
#include "log.h"


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
        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}
