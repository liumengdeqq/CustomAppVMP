//
// Created by liu meng on 2018/8/21.
//
#include "object.h"
#include "Jdwp.h"
#ifndef CUSTOMAPPVMP_DEBUGGER_H
#define CUSTOMAPPVMP_DEBUGGER_H
struct DebugInvokeReq {
    /* boolean; only set when we're in the tail end of an event handler */
    bool ready;

    /* boolean; set if the JDWP thread wants this thread to do work */
    bool invokeNeeded;

    /* request */
    Object* obj;        /* not used for ClassType.InvokeMethod */
    Object* thread;
    ClassObject* clazz;
    Method* method;
    u4 numArgs;
    u8* argArray;   /* will be NULL if numArgs==0 */
    u4 options;

    /* result */
    JdwpError err;
    u1 resultTag;
    JValue resultValue;
    ObjectId exceptObj;

    /* condition variable to wait on while the method executes */
    pthread_mutex_t lock;
    pthread_cond_t cv;
};

#endif //CUSTOMAPPVMP_DEBUGGER_H
