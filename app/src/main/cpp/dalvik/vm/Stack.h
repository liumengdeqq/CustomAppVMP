//
// Created by liu meng on 2018/8/21.
//
#include "Class.h"
#ifndef CUSTOMAPPVMP_STACK_H
#define CUSTOMAPPVMP_STACK_H
struct StackSaveArea;

//#define PAD_SAVE_AREA       /* help debug stack trampling */

/*
 * The VM-specific internal goop.
 *
 * The idea is to mimic a typical native stack frame, with copies of the
 * saved PC and FP.  At some point we'd like to have interpreted and
 * native code share the same stack, though this makes portability harder.
 */
struct StackSaveArea {
#ifdef PAD_SAVE_AREA
    u4          pad0, pad1, pad2;
#endif

#ifdef EASY_GDB
    /* make it easier to trek through stack frames in GDB */
    StackSaveArea* prevSave;
#endif

    /* saved frame pointer for previous frame, or NULL if this is at bottom */
    u4*         prevFrame;

    /* saved program counter (from method in caller's frame) */
    const u2*   savedPc;

    /* pointer to method we're *currently* executing; handy for exceptions */
    const Method* method;

    union {
        /* for JNI native methods: bottom of local reference segment */
        u4          localRefCookie;

        /* for interpreted methods: saved current PC, for exception stack
         * traces and debugger traces */
        const u2*   currentPc;
    } xtra;

    /* Native return pointer for JIT, or 0 if interpreted */
    const u2* returnAddr;
#ifdef PAD_SAVE_AREA
    u4          pad3, pad4, pad5;
#endif
};
#define SAVEAREA_FROM_FP(_fp)   ((StackSaveArea*)(_fp) -1)
INLINE bool dvmIsBreakFrame(const u4* fp)
{
    return SAVEAREA_FROM_FP(fp)->method == NULL;
}

extern "C" int dvmLineNumFromPC(const Method* method, u4 relPc);
static ClassObject* callPrep(Thread* self, const Method* method, Object* obj,
                             bool checkAccess)
{
    ClassObject* clazz;

#ifndef NDEBUG
    if (self->status != THREAD_RUNNING) {
        ALOGW("threadid=%d: status=%d on call to %s.%s -",
              self->threadId, self->status,
              method->clazz->descriptor, method->name);
    }
#endif

    assert(self != NULL);
    assert(method != NULL);

    if (obj != NULL)
        clazz = obj->clazz;
    else
        clazz = method->clazz;

    IF_LOGVV() {
        char* desc = dexProtoCopyMethodDescriptor(&method->prototype);
        LOGVV("thread=%d native code calling %s.%s %s", self->threadId,
              clazz->descriptor, method->name, desc);
        free(desc);
    }

    if (checkAccess) {
        /* needed for java.lang.reflect.Method.invoke */
        if (!dvmCheckMethodAccess(dvmGetCaller2Class(self->interpSave.curFrame),
                                  method))
        {
            /* note this throws IAException, not IAError */
            dvmThrowIllegalAccessException("access to method denied");
            return NULL;
        }
    }

    /*
     * Push a call frame on.  If there isn't enough room for ins, locals,
     * outs, and the saved state, it will throw an exception.
     *
     * This updates self->interpSave.curFrame.
     */
    if (dvmIsNativeMethod(method)) {
        /* native code calling native code the hard way */
        if (!dvmPushJNIFrame(self, method)) {
            assert(dvmCheckException(self));
            return NULL;
        }
    } else {
        /* native code calling interpreted code */
        if (!dvmPushInterpFrame(self, method)) {
            assert(dvmCheckException(self));
            return NULL;
        }
    }

    return clazz;
}
void dvmCallMethodV(Thread* self, const Method* method, Object* obj,
                    bool fromJni, JValue* pResult, va_list args)
{
    const char* desc = &(method->shorty[1]); // [0] is the return type.
    int verifyCount = 0;
    ClassObject* clazz;
    u4* ins;

    clazz = callPrep(self, method, obj, false);
    if (clazz == NULL)
        return;

    /* "ins" for new frame start at frame pointer plus locals */
    ins = ((u4*)self->interpSave.curFrame) +
          (method->registersSize - method->insSize);

    //ALOGD("  FP is %p, INs live at >= %p", self->interpSave.curFrame, ins);

    /* put "this" pointer into in0 if appropriate */
    if (!dvmIsStaticMethod(method)) {
#ifdef WITH_EXTRA_OBJECT_VALIDATION
        assert(obj != NULL && dvmIsHeapAddress(obj));
#endif
        *ins++ = (u4) obj;
        verifyCount++;
    }

    while (*desc != '\0') {
        switch (*(desc++)) {
            case 'D': case 'J': {
                u8 val = va_arg(args, u8);
                memcpy(ins, &val, 8);       // EABI prevents direct store
                ins += 2;
                verifyCount += 2;
                break;
            }
            case 'F': {
                /* floats were normalized to doubles; convert back */
                float f = (float) va_arg(args, double);
                *ins++ = dvmFloatToU4(f);
                verifyCount++;
                break;
            }
            case 'L': {     /* 'shorty' descr uses L for all refs, incl array */
                void* arg = va_arg(args, void*);
                assert(obj == NULL || dvmIsHeapAddress(obj));
                jobject argObj = reinterpret_cast<jobject>(arg);
                if (fromJni)
                    *ins++ = (u4) dvmDecodeIndirectRef(self, argObj);
                else
                    *ins++ = (u4) argObj;
                verifyCount++;
                break;
            }
            default: {
                /* Z B C S I -- all passed as 32-bit integers */
                *ins++ = va_arg(args, u4);
                verifyCount++;
                break;
            }
        }
    }

#ifndef NDEBUG
    if (verifyCount != method->insSize) {
        ALOGE("Got vfycount=%d insSize=%d for %s.%s", verifyCount,
              method->insSize, clazz->descriptor, method->name);
        assert(false);
        goto bail;
    }
#endif

    //dvmDumpThreadStack(dvmThreadSelf());

    if (dvmIsNativeMethod(method)) {
        TRACE_METHOD_ENTER(self, method);
        /*
         * Because we leave no space for local variables, "curFrame" points
         * directly at the method arguments.
         */
        (*method->nativeFunc)((u4*)self->interpSave.curFrame, pResult,
                              method, self);
        TRACE_METHOD_EXIT(self, method);
    } else {
        dvmInterpret(self, method, pResult);
    }

#ifndef NDEBUG
    bail:
#endif
    dvmPopFrame(self);
}

void dvmCallMethod(Thread* self, const Method* method, Object* obj,
                   JValue* pResult, ...)
{
    va_list args;
    va_start(args, pResult);
    dvmCallMethodV(self, method, obj, false, pResult, args);
    va_end(args);
}

#endif //CUSTOMAPPVMP_STACK_H
