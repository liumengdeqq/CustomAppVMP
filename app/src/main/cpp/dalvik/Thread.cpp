//
// Created by liu meng on 2018/8/31.
//
#include "Thread.h"
#include <dlfcn.h>
void initThreadFuction(void *dvm_hand,int apilevel){
    dvmThreadSelfHook = (dvmThreadSelf_func)dlsym(dvm_hand,
                                                        apilevel > 10 ? "_Z13dvmThreadSelfv" : "dvmThreadSelf");
}