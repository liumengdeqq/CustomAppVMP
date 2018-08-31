//
// Created by liu meng on 2018/8/31.
//

#include "Sync.h"
#include <dlfcn.h>
void initSynFuction(void *dvm_hand,int apilevel){
    dvmLockObjectHook = (dvmLockObject_func)dlsym(dvm_hand,"dvmLockObject");


}