//
// Created by liu meng on 2018/8/21.
//
#include "object.h"

const char* dvmGetMethodSourceFile(const Method* meth)
{
    /*
     * TODO: A method's debug info can override the default source
     * file for a class, so we should account for that possibility
     * here.
     */
    return meth->clazz->sourceFile;
}