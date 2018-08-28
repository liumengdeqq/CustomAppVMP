//
// Created by liu meng on 2018/8/27.
//

#include "../Dalvik.h"
#include "../mterp/Mterp.h"

#include <stddef.h>


/*
 * Verify some constants used by the mterp interpreter.
 */
bool dvmCheckAsmConstants()
{

    return true;
}


/*
 * "Mterp entry point.
 */
void dvmMterpStd(Thread* self)
{
    /* configure mterp items */
    self->interpSave.methodClassDex = self->interpSave.method->clazz->pDvmDex;

    IF_LOGVV() {
        char* desc = dexProtoCopyMethodDescriptor(
                &self->interpSave.method->prototype);
        LOGVV("mterp threadid=%d : %s.%s %s",
              dvmThreadSelf()->threadId,
              self->interpSave.method->clazz->descriptor,
              self->interpSave.method->name,
              desc);
        free(desc);
    }
    //ALOGI("self is %p, pc=%p, fp=%p", self, self->interpSave.pc,
    //      self->interpSave.curFrame);
    //ALOGI("first instruction is 0x%04x", self->interpSave.pc[0]);

    /*
     * Handle any ongoing profiling and prep for debugging
     */
    if (self->interpBreak.ctl.subMode != 0) {
        TRACE_METHOD_ENTER(self, self->interpSave.method);
        self->debugIsMethodEntry = true;   // Always true on startup
    }

    dvmMterpStdRun(self);

#ifdef LOG_INSTR
    ALOGD("|-- Leaving interpreter loop");
#endif
}
