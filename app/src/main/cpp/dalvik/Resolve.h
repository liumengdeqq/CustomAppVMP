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

extern dvmResolveString_func dvmResolveStringhook;
extern dvmResolveClass_func dvmResolveClasshook;

typedef Method* (*dvmResolveMethod_func)(const ClassObject* referrer, u4 methodIdx,
                                            MethodType methodType);
extern dvmResolveMethod_func dvmResolveMethodhook;

typedef InstField* (*dvmResolveInstField_func)(const ClassObject* referrer, u4 ifieldIdx);
extern dvmResolveInstField_func dvmResolveInstFieldhook;
typedef StaticField* (*dvmResolveStaticField_func)(const ClassObject* referrer, u4 sfieldIdx);
extern  dvmResolveStaticField_func dvmResolveStaticFieldhook;

bool initResolveFuction(void * dvm_hand,int apilevel);

#endif //CUSTOMAPPVMP_RESOLVE_H
