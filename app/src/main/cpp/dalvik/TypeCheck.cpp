//
// Created by liu meng on 2018/8/31.
//

#include "TypeCheck.h"

 dvmInstanceofNonTrivial_func dvmInstanceofNonTrivialHook;

 dvmCanPutArrayElement_func dvmCanPutArrayElementHook;
  bool initTypeCheckFuction(void *dvm_hand,int apilevel){
    if (dvm_hand) {
        dvmInstanceofNonTrivialHook = (dvmInstanceofNonTrivial_func)dlsym(dvm_hand,"dvmInstanceofNonTrivial");
        if (!dvmInstanceofNonTrivialHook) {
            return JNI_FALSE;
        }
        dvmCanPutArrayElementHook = (dvmCanPutArrayElement_func)dlsym(dvm_hand,"dvmCanPutArrayElement");
        if (!dvmCanPutArrayElementHook) {
            return JNI_FALSE;
        }

        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}

