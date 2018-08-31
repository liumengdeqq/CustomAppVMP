//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_DEBUGGER_H
#define CUSTOMAPPVMP_DEBUGGER_H

#include "Object.h"
#include "JdwpConstants.h"
typedef u4 FieldId;     /* static or instance field */
typedef u4 MethodId;    /* any kind of method, including constructors */
typedef u8 ObjectId;    /* any object (threadID, stringID, arrayID, etc) */
typedef u8 RefTypeId;   /* like ObjectID, but unique for Class objects */
typedef u8 FrameId;     /* short-lived stack frame ID */
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
