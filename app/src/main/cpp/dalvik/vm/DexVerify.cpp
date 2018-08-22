//
// Created by liu meng on 2018/8/21.
//
#include "Class.h"
#include "log.h"
#include "object.h"
#include <unistd.h>
#include "CodeVerify.h"
static bool verifyMethod(Method* meth);

static bool verifyMethod(Method* meth)
{
    bool result = false;

    /*
     * Verifier state blob.  Various values will be cached here so we
     * can avoid expensive lookups and pass fewer arguments around.
     */
    VerifierData vdata;
#if 1   // ndef NDEBUG
    memset(&vdata, 0x99, sizeof(vdata));
#endif

    vdata.method = meth;
    vdata.insnsSize = dvmGetMethodInsnsSize(meth);
    vdata.insnRegCount = meth->registersSize;
    vdata.insnFlags = NULL;
    vdata.uninitMap = NULL;
    vdata.basicBlocks = NULL;

    /*
     * If there aren't any instructions, make sure that's expected, then
     * exit successfully.  Note: for native methods, meth->insns gets set
     * to a native function pointer on first call, so don't use that as
     * an indicator.
     */
    if (vdata.insnsSize == 0) {
        if (!dvmIsNativeMethod(meth) && !dvmIsAbstractMethod(meth)) {
            LOG_VFY_METH(meth,
                         "VFY: zero-length code in concrete non-native method");
            goto bail;
        }

        goto success;
    }

    /*
     * Sanity-check the register counts.  ins + locals = registers, so make
     * sure that ins <= registers.
     */
    if (meth->insSize > meth->registersSize) {
        LOG_VFY_METH(meth, "VFY: bad register counts (ins=%d regs=%d)",
                     meth->insSize, meth->registersSize);
        goto bail;
    }

    /*
     * Allocate and populate an array to hold instruction data.
     *
     * TODO: Consider keeping a reusable pre-allocated array sitting
     * around for smaller methods.
     */
    vdata.insnFlags = (InsnFlags*) calloc(vdata.insnsSize, sizeof(InsnFlags));
    if (vdata.insnFlags == NULL)
        goto bail;

    /*
     * Compute the width of each instruction and store the result in insnFlags.
     * Count up the #of occurrences of certain opcodes while we're at it.
     */
    if (!computeWidthsAndCountOps(&vdata))
        goto bail;

    /*
     * Allocate a map to hold the classes of uninitialized instances.
     */
    vdata.uninitMap = dvmCreateUninitInstanceMap(meth, vdata.insnFlags,
                                                 vdata.newInstanceCount);
    if (vdata.uninitMap == NULL)
        goto bail;

    /*
     * Set the "in try" flags for all instructions guarded by a "try" block.
     * Also sets the "branch target" flag on exception handlers.
     */
    if (!scanTryCatchBlocks(meth, vdata.insnFlags))
        goto bail;

    /*
     * Perform static instruction verification.  Also sets the "branch
     * target" flags.
     */
    if (!verifyInstructions(&vdata))
        goto bail;

    /*
     * Do code-flow analysis.
     *
     * We could probably skip this for a method with no registers, but
     * that's so rare that there's little point in checking.
     */
    if (!dvmVerifyCodeFlow(&vdata)) {
        //ALOGD("+++ %s failed code flow", meth->name);
        goto bail;
    }

    success:
    result = true;

    bail:
    dvmFreeVfyBasicBlocks(&vdata);
    dvmFreeUninitInstanceMap(vdata.uninitMap);
    free(vdata.insnFlags);
    return result;
}

bool dvmVerifyClass(ClassObject* clazz)
{
    int i;

    if (dvmIsClassVerified(clazz)) {
        ALOGD("Ignoring duplicate verify attempt on %s", clazz->descriptor);
        return true;
    }

    for (i = 0; i < clazz->directMethodCount; i++) {
        if (!verifyMethod(&clazz->directMethods[i])) {
            LOG_VFY("Verifier rejected class %s", clazz->descriptor);
            return false;
        }
    }
    for (i = 0; i < clazz->virtualMethodCount; i++) {
        if (!verifyMethod(&clazz->virtualMethods[i])) {
            LOG_VFY("Verifier rejected class %s", clazz->descriptor);
            return false;
        }
    }

    return true;
}
