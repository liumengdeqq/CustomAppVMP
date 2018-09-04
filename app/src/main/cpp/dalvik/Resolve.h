//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_RESOLVE_H
#define CUSTOMAPPVMP_RESOLVE_H

#include "object.h"
#include <dlfcn.h>
enum MethodType {
    METHOD_UNKNOWN  = 0,
    METHOD_DIRECT,      // <init>, private
    METHOD_STATIC,      // static
    METHOD_VIRTUAL,     // virtual, super
    METHOD_INTERFACE    // interface
};

typedef StringObject* (*dvmResolveString_func)(const ClassObject* referrer, u4 stringIdx);
typedef ClassObject* (*dvmResolveClass_func)(const ClassObject* referrer, u4 classIdx,bool fromUnverifiedConstant);

 dvmResolveString_func dvmResolveStringhook;
 dvmResolveClass_func dvmResolveClasshook;

typedef Method* (*dvmResolveMethod_func)(const ClassObject* referrer, u4 methodIdx,
                                            MethodType methodType);
 dvmResolveMethod_func dvmResolveMethodhook;

typedef InstField* (*dvmResolveInstField_func)(const ClassObject* referrer, u4 ifieldIdx);
 dvmResolveInstField_func dvmResolveInstFieldhook;
typedef StaticField* (*dvmResolveStaticField_func)(const ClassObject* referrer, u4 sfieldIdx);
 dvmResolveStaticField_func dvmResolveStaticFieldhook;

static bool initResolveFuction(void * dvm_hand,int apilevel){
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

#endif //CUSTOMAPPVMP_RESOLVE_H
