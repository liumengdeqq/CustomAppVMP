//
// Created by liu meng on 2018/8/23.
//
#include <stddef.h>
#include "object.h"
#include "VerifySubs.h"
#include "BitVector.h"
#include "VfyBasicBlock.h"
#include <string.h>
#include <malloc.h>
#include "DexOpcodes.h"
#include "Globals.h"
#include "log.h"
#ifndef CUSTOMAPPVMP_CODEVERIFY_H
#define CUSTOMAPPVMP_CODEVERIFY_H
#define kUninitThisArgAddr  (-1)
#define kUninitThisArgSlot  0
typedef u4 MonitorEntries;
typedef u4 RegType;
struct UninitInstanceMap {
    int numEntries;
    struct {
        int             addr;   /* code offset, or -1 for method arg ("this") */
        ClassObject*    clazz;  /* class created at this address */
    } map[1];
};
struct RegisterLine {
    RegType*        regTypes;
    MonitorEntries* monitorEntries;
    u4*             monitorStack;
    unsigned int    monitorStackTop;
    BitVector*      liveRegs;
};

struct VerifierData {
    /*
     * The method we're working on.
     */
    const Method*   method;

    /*
     * Number of code units of instructions in the method.  A cache of the
     * value calculated by dvmGetMethodInsnsSize().
     */
    u4              insnsSize;

    /*
     * Number of registers we track for each instruction.  This is equal
     * to the method's declared "registersSize".  (Does not include the
     * pending return value.)
     */
    u4              insnRegCount;

    /*
     * Instruction widths and flags, one entry per code unit.
     */
    InsnFlags*      insnFlags;

    /*
     * Uninitialized instance map, used for tracking the movement of
     * objects that have been allocated but not initialized.
     */
    UninitInstanceMap* uninitMap;

    /*
     * Array of RegisterLine structs, one entry per code unit.  We only need
     * entries for code units that hold the start of an "interesting"
     * instruction.  For register map generation, we're only interested
     * in GC points.
     */
    RegisterLine*   registerLines;

    /*
     * The number of occurrences of specific opcodes.
     */
    size_t          newInstanceCount;
    size_t          monitorEnterCount;

    /*
     * Array of pointers to basic blocks, one entry per code unit.  Used
     * for liveness analysis.
     */
    VfyBasicBlock** basicBlocks;
};
static bool isInitMethod(const Method* meth)
{
    return (*meth->name == '<' && strcmp(meth->name+1, "init>") == 0);
}
INLINE int dvmInsnGetWidth(const InsnFlags* insnFlags, int addr) {
    return insnFlags[addr] & kInsnFlagWidthMask;
}

UninitInstanceMap* dvmCreateUninitInstanceMap(const Method* meth,
                                              const InsnFlags* insnFlags, int newInstanceCount)
{
    const int insnsSize = dvmGetMethodInsnsSize(meth);
    const u2* insns = meth->insns;
    UninitInstanceMap* uninitMap;
    bool isInit = false;
    int idx, addr;

    if (isInitMethod(meth)) {
        newInstanceCount++;
        isInit = true;
    }

    /*
     * Allocate the header and map as a single unit.
     *
     * TODO: consider having a static instance so we can avoid allocations.
     * I don't think the verifier is guaranteed to be single-threaded when
     * running in the VM (rather than dexopt), so that must be taken into
     * account.
     */
    int size = offsetof(UninitInstanceMap, map) +
               newInstanceCount * sizeof(uninitMap->map[0]);
    uninitMap = (UninitInstanceMap*)calloc(1, size);
    if (uninitMap == NULL)
        return NULL;
    uninitMap->numEntries = newInstanceCount;

    idx = 0;
    if (isInit) {
        uninitMap->map[idx++].addr = kUninitThisArgAddr;
    }

    /*
     * Run through and find the new-instance instructions.
     */
    for (addr = 0; addr < insnsSize; /**/) {
        int width = dvmInsnGetWidth(insnFlags, addr);

        Opcode opcode = dexOpcodeFromCodeUnit(*insns);
        if (opcode == OP_NEW_INSTANCE)
            uninitMap->map[idx++].addr = addr;

        addr += width;
        insns += width;
    }

    assert(idx == newInstanceCount);
    return uninitMap;
}
INLINE void dvmInsnSetInTry(InsnFlags* insnFlags, int addr, bool inTry)
{
    assert(inTry);
    //if (inTry)
    insnFlags[addr] |= kInsnFlagInTry;
    //else
    //    insnFlags[addr] &= ~kInsnFlagInTry;
}
INLINE void dvmInsnSetBranchTarget(InsnFlags* insnFlags, int addr,
                                   bool isBranch)
{
    assert(isBranch);
    //if (isBranch)
    insnFlags[addr] |= kInsnFlagBranchTarget;
    //else
    //    insnFlags[addr] &= ~kInsnFlagBranchTarget;
}
INLINE bool dvmInsnIsOpcode(const InsnFlags* insnFlags, int addr) {
    return (insnFlags[addr] & kInsnFlagWidthMask) != 0;
}
INLINE void dvmInsnSetGcPoint(InsnFlags* insnFlags, int addr,
                              bool isGcPoint)
{
    assert(isGcPoint);
    //if (isGcPoint)
    insnFlags[addr] |= kInsnFlagGcPoint;
    //else
    //    insnFlags[addr] &= ~kInsnFlagGcPoint;
}
bool dvmVerifyCodeFlow(VerifierData* vdata)
{
    bool result = false;
    const Method* meth = vdata->method;
    const int insnsSize = vdata->insnsSize;
    const bool generateRegisterMap = gDvm.generateRegisterMaps;
    RegisterTable regTable;

    memset(&regTable, 0, sizeof(regTable));

#ifdef VERIFIER_STATS
    gDvm.verifierStats.methodsExamined++;
    if (vdata->monitorEnterCount)
        gDvm.verifierStats.monEnterMethods++;
#endif

    /* TODO: move this elsewhere -- we don't need to do this for every method */
    verifyPrep();

    if (meth->registersSize * insnsSize > 4*1024*1024) {
        LOG_VFY_METH(meth,
                     "VFY: warning: method is huge (regs=%d insnsSize=%d)",
                     meth->registersSize, insnsSize);
        /* might be bogus data, might be some huge generated method */
    }

    /*
     * Create register lists, and initialize them to "Unknown".  If we're
     * also going to create the register map, we need to retain the
     * register lists for a larger set of addresses.
     */
    if (!initRegisterTable(vdata, &regTable,
                           generateRegisterMap ? kTrackRegsGcPoints : kTrackRegsBranches))
        goto bail;

    vdata->registerLines = regTable.registerLines;

    /*
     * Perform liveness analysis.
     *
     * We can do this before or after the main verifier pass.  The choice
     * affects whether or not we see the effects of verifier instruction
     * changes, i.e. substitution of throw-verification-error.
     *
     * In practice the ordering doesn't really matter, because T-V-E
     * just prunes "can continue", creating regions of dead code (with
     * corresponding register map data that will never be used).
     */
    if (generateRegisterMap &&
        gDvm.registerMapMode == kRegisterMapModeLivePrecise)
    {
        /*
         * Compute basic blocks and predecessor lists.
         */
        if (!dvmComputeVfyBasicBlocks(vdata))
            goto bail;

        /*
         * Compute liveness.
         */
        if (!dvmComputeLiveness(vdata))
            goto bail;
    }

    /*
     * Initialize the types of the registers that correspond to the
     * method arguments.  We can determine this from the method signature.
     */
    if (!setTypesFromSignature(meth, regTable.registerLines[0].regTypes,
                               vdata->uninitMap))
        goto bail;

    /*
     * Run the verifier.
     */
    if (!doCodeVerification(vdata, &regTable))
        goto bail;

    /*
     * Generate a register map.
     */
    if (generateRegisterMap) {
        RegisterMap* pMap = dvmGenerateRegisterMapV(vdata);
        if (pMap != NULL) {
            /*
             * Tuck it into the Method struct.  It will either get used
             * directly or, if we're in dexopt, will be packed up and
             * appended to the DEX file.
             */
            dvmSetRegisterMap((Method*)meth, pMap);
        }
    }

    /*
     * Success.
     */
    result = true;

    bail:
    freeRegisterLineInnards(vdata);
    free(regTable.registerLines);
    free(regTable.lineAlloc);
    return result;
}


#endif //CUSTOMAPPVMP_CODEVERIFY_H
