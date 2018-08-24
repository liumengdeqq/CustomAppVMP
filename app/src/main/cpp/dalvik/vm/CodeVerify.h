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
#include "Dalvik.h"
#include "InstrUtils.h"
#include "DexCatch.h"
#include <unistd.h>
#include "DexDebugInfo.h"
#ifndef CUSTOMAPPVMP_CODEVERIFY_H
#define CUSTOMAPPVMP_CODEVERIFY_H
#define kUninitThisArgAddr  (-1)
#define kUninitThisArgSlot  0
#define kRegTypeUninitMask  0xff
#define kRegTypeUninitShift 8
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
typedef struct RegisterTable {
    /*
     * Array of RegisterLine structs, one per address in the method.  We only
     * set the pointers for certain addresses, based on instruction widths
     * and what we're trying to accomplish.
     */
    RegisterLine* registerLines;

    /*
     * Number of registers we track for each instruction.  This is equal
     * to the method's declared "registersSize" plus kExtraRegs.
     */
    size_t      insnRegCountPlus;

    /*
     * Storage for a register line we're currently working on.
     */
    RegisterLine workLine;

    /*
     * Storage for a register line we're saving for later.
     */
    RegisterLine savedLine;

    /*
     * A single large alloc, with all of the storage needed for RegisterLine
     * data (RegType array, MonitorEntries array, monitor stack).
     */
    void*       lineAlloc;
} RegisterTable;
#ifndef NDEBUG
/*
 * Verify symmetry in the conversion table.
 */
enum {
    kRegTypeUnknown = 0,    /* initial state; use value=0 so calloc works */
    kRegTypeUninit = 1,     /* MUST be odd to distinguish from pointer */
    kRegTypeConflict,       /* merge clash makes this reg's type unknowable */

    /*
     * Category-1nr types.  The order of these is chiseled into a couple
     * of tables, so don't add, remove, or reorder if you can avoid it.
     */
#define kRegType1nrSTART    kRegTypeZero
    kRegTypeZero,           /* 32-bit 0, could be Boolean, Int, Float, or Ref */
    kRegTypeOne,            /* 32-bit 1, could be Boolean, Int, Float */
    kRegTypeBoolean,        /* must be 0 or 1 */
    kRegTypeConstPosByte,   /* const derived byte, known positive */
    kRegTypeConstByte,      /* const derived byte */
    kRegTypeConstPosShort,  /* const derived short, known positive */
    kRegTypeConstShort,     /* const derived short */
    kRegTypeConstChar,      /* const derived char */
    kRegTypeConstInteger,   /* const derived integer */
    kRegTypePosByte,        /* byte, known positive (can become char) */
    kRegTypeByte,
    kRegTypePosShort,       /* short, known positive (can become char) */
    kRegTypeShort,
    kRegTypeChar,
    kRegTypeInteger,
    kRegTypeFloat,
#define kRegType1nrEND      kRegTypeFloat

    kRegTypeConstLo,        /* const derived wide, lower half */
    kRegTypeConstHi,        /* const derived wide, upper half */
    kRegTypeLongLo,         /* lower-numbered register; endian-independent */
    kRegTypeLongHi,
    kRegTypeDoubleLo,
    kRegTypeDoubleHi,

    /*
     * Enumeration max; this is used with "full" (32-bit) RegType values.
     *
     * Anything larger than this is a ClassObject or uninit ref.  Mask off
     * all but the low 8 bits; if you're left with kRegTypeUninit, pull
     * the uninit index out of the high 24.  Because kRegTypeUninit has an
     * odd value, there is no risk of a particular ClassObject pointer bit
     * pattern being confused for it (assuming our class object allocator
     * uses word alignment).
     */
            kRegTypeMAX
};
extern const char gDvmMergeTab[kRegTypeMAX][kRegTypeMAX];
static void checkMergeTab()
{
    int i, j;

    for (i = 0; i < kRegTypeMAX; i++) {
        for (j = i; j < kRegTypeMAX; j++) {
            if (gDvmMergeTab[i][j] != gDvmMergeTab[j][i]) {
                ALOGE("Symmetry violation: %d,%d vs %d,%d", i, j, j, i);
                dvmAbort();
            }
        }
    }
}
#endif
static void verifyPrep()
{
#ifndef NDEBUG
    /* only need to do this if the table was updated */
    checkMergeTab();
#endif
}
enum RegisterTrackingMode {
    kTrackRegsBranches,
    kTrackRegsGcPoints,
    kTrackRegsAll
};
#ifndef NDEBUG
# define DEAD_CODE_SCAN  true
#else
# define DEAD_CODE_SCAN  false
#endif

static bool gDebugVerbose = false;

#define SHOW_REG_DETAILS \
    (0 | DRT_SHOW_LIVENESS /*| DRT_SHOW_REF_TYPES | DRT_SHOW_LOCALS*/)

/*
 * We need an extra "pseudo register" to hold the return type briefly.  It
 * can be category 1 or 2, so we need two slots.
 */
#define kExtraRegs  2
#define RESULT_REGISTER(_insnRegCount)  (_insnRegCount)
INLINE bool dvmInsnIsGcPoint(const InsnFlags* insnFlags, int addr) {
    return (insnFlags[addr] & kInsnFlagGcPoint) != 0;
}
INLINE bool dvmInsnIsBranchTarget(const InsnFlags* insnFlags, int addr) {
    return (insnFlags[addr] & kInsnFlagBranchTarget) != 0;
}
typedef u4 MonitorEntries;
#define kMaxMonitorStackDepth   (sizeof(MonitorEntries) * 8)
static u1* assignLineStorage(u1* storage, RegisterLine* line,
                             bool trackMonitors, size_t regTypeSize, size_t monEntSize, size_t stackSize)
{
    line->regTypes = (RegType*) storage;
    storage += regTypeSize;

    if (trackMonitors) {
        line->monitorEntries = (MonitorEntries*) storage;
        storage += monEntSize;
        line->monitorStack = (u4*) storage;
        storage += stackSize;

        assert(line->monitorStackTop == 0);
    }

    return storage;
}

static bool initRegisterTable(const VerifierData* vdata,
                              RegisterTable* regTable, RegisterTrackingMode trackRegsFor)
{
    const Method* meth = vdata->method;
    const int insnsSize = vdata->insnsSize;
    const InsnFlags* insnFlags = vdata->insnFlags;
    const int kExtraLines = 2;  /* workLine, savedLine */
    int i;

    /*
     * Every address gets a RegisterLine struct.  This is wasteful, but
     * not so much that it's worth chasing through an extra level of
     * indirection.
     */
    regTable->insnRegCountPlus = meth->registersSize + kExtraRegs;
    regTable->registerLines =
            (RegisterLine*) calloc(insnsSize, sizeof(RegisterLine));
    if (regTable->registerLines == NULL)
        return false;

    assert(insnsSize > 0);

    /*
     * Count up the number of "interesting" instructions.
     *
     * "All" means "every address that holds the start of an instruction".
     * "Branches" and "GcPoints" mean just those addresses.
     *
     * "GcPoints" fills about half the addresses, "Branches" about 15%.
     */
    int interestingCount = kExtraLines;

    for (i = 0; i < insnsSize; i++) {
        bool interesting;

        switch (trackRegsFor) {
            case kTrackRegsAll:
                interesting = dvmInsnIsOpcode(insnFlags, i);
                break;
            case kTrackRegsGcPoints:
                interesting = dvmInsnIsGcPoint(insnFlags, i) ||
                              dvmInsnIsBranchTarget(insnFlags, i);
                break;
            case kTrackRegsBranches:
                interesting = dvmInsnIsBranchTarget(insnFlags, i);
                break;
            default:
                dvmAbort();
                return false;
        }

        if (interesting)
            interestingCount++;

        /* count instructions, for display only */
        //if (dvmInsnIsOpcode(insnFlags, i))
        //    insnCount++;
    }

    /*
     * Allocate storage for the register type arrays.
     * TODO: set trackMonitors based on global config option
     */
    size_t regTypeSize = regTable->insnRegCountPlus * sizeof(RegType);
    size_t monEntSize = regTable->insnRegCountPlus * sizeof(MonitorEntries);
    size_t stackSize = kMaxMonitorStackDepth * sizeof(u4);
    bool trackMonitors;

    if (gDvm.monitorVerification) {
        trackMonitors = (vdata->monitorEnterCount != 0);
    } else {
        trackMonitors = false;
    }

    size_t spacePerEntry = regTypeSize +
                           (trackMonitors ? monEntSize + stackSize : 0);
    regTable->lineAlloc = calloc(interestingCount, spacePerEntry);
    if (regTable->lineAlloc == NULL)
        return false;

#ifdef VERIFIER_STATS
    size_t totalSpace = interestingCount * spacePerEntry +
        insnsSize * sizeof(RegisterLine);
    if (gDvm.verifierStats.biggestAlloc < totalSpace)
        gDvm.verifierStats.biggestAlloc = totalSpace;
#endif

    /*
     * Populate the sparse register line table.
     *
     * There is a RegisterLine associated with every address, but not
     * every RegisterLine has non-NULL pointers to storage for its fields.
     */
    u1* storage = (u1*)regTable->lineAlloc;
    for (i = 0; i < insnsSize; i++) {
        bool interesting;

        switch (trackRegsFor) {
            case kTrackRegsAll:
                interesting = dvmInsnIsOpcode(insnFlags, i);
                break;
            case kTrackRegsGcPoints:
                interesting = dvmInsnIsGcPoint(insnFlags, i) ||
                              dvmInsnIsBranchTarget(insnFlags, i);
                break;
            case kTrackRegsBranches:
                interesting = dvmInsnIsBranchTarget(insnFlags, i);
                break;
            default:
                dvmAbort();
                return false;
        }

        if (interesting) {
            storage = assignLineStorage(storage, &regTable->registerLines[i],
                                        trackMonitors, regTypeSize, monEntSize, stackSize);
        }
    }

    /*
     * Grab storage for our "temporary" register lines.
     */
    storage = assignLineStorage(storage, &regTable->workLine,
                                trackMonitors, regTypeSize, monEntSize, stackSize);
    storage = assignLineStorage(storage, &regTable->savedLine,
                                trackMonitors, regTypeSize, monEntSize, stackSize);

    //ALOGD("Tracking registers for [%d], total %d in %d units",
    //    trackRegsFor, interestingCount-kExtraLines, insnsSize);

    assert(storage - (u1*)regTable->lineAlloc ==
           (int) (interestingCount * spacePerEntry));
    assert(regTable->registerLines[0].regTypes != NULL);
    return true;
}
static u4 extractCatchHandlers(const DexCode* pCode, const DexTry* pTry,
                               u4* addrBuf, size_t addrBufSize)
{
    DexCatchIterator iterator;
    unsigned int idx = 0;

    dexCatchIteratorInit(&iterator, pCode, pTry->handlerOff);
    while (true) {
        DexCatchHandler* handler = dexCatchIteratorNext(&iterator);
        if (handler == NULL)
            break;

        if (idx < addrBufSize) {
            addrBuf[idx] = handler->address;
        }
        idx++;
    }

    return idx;
}
INLINE bool dvmInsnIsInTry(const InsnFlags* insnFlags, int addr) {
    return (insnFlags[addr] & kInsnFlagInTry) != 0;
}
static bool isDataChunk(u2 insn)
{
    return (insn == kPackedSwitchSignature ||
            insn == kSparseSwitchSignature ||
            insn == kArrayDataSignature);
}
static VfyBasicBlock* allocVfyBasicBlock(VerifierData* vdata, u4 idx)
{
    VfyBasicBlock* newBlock = (VfyBasicBlock*) calloc(1, sizeof(VfyBasicBlock));
    if (newBlock == NULL)
        return NULL;

    /*
     * TODO: there is no good default size here -- the problem is that most
     * addresses will only have one predecessor, but a fair number will
     * have 10+, and a few will have 100+ (e.g. the synthetic "finally"
     * in a large synchronized method).  We probably want to use a small
     * base allocation (perhaps two) and then have the first overflow
     * allocation jump dramatically (to 32 or thereabouts).
     */
    newBlock->predecessors = dvmPointerSetAlloc(32);
    if (newBlock->predecessors == NULL) {
        free(newBlock);
        return NULL;
    }

    newBlock->firstAddr = (u4) -1;      // DEBUG

    newBlock->liveRegs = dvmAllocBitVector(vdata->insnRegCount, false);
    if (newBlock->liveRegs == NULL) {
        dvmPointerSetFree(newBlock->predecessors);
        free(newBlock);
        return NULL;
    }

    return newBlock;
}
static bool addToPredecessor(VerifierData* vdata, VfyBasicBlock* curBlock,
                             u4 targetIdx)
{
    assert(targetIdx < vdata->insnsSize);

    /*
     * Allocate the target basic block if necessary.  This will happen
     * on e.g. forward branches.
     *
     * We can't fill in all the fields, but that will happen automatically
     * when we get to that part of the code.
     */
    VfyBasicBlock* targetBlock = vdata->basicBlocks[targetIdx];
    if (targetBlock == NULL) {
        targetBlock = allocVfyBasicBlock(vdata, targetIdx);
        if (targetBlock == NULL)
            return false;
        vdata->basicBlocks[targetIdx] = targetBlock;
    }

    PointerSet* preds = targetBlock->predecessors;
    bool added = dvmPointerSetAddEntry(preds, curBlock);
    if (!added) {
        /*
         * This happens sometimes for packed-switch instructions, where
         * the same target address appears more than once.  Also, a
         * (pointless) conditional branch to the next instruction will
         * trip over this.
         */
        ALOGV("ODD: point set for targ=0x%04x (%p) already had block "
                      "fir=0x%04x (%p)",
              targetIdx, targetBlock, curBlock->firstAddr, curBlock);
    }

    return true;
}

static bool setPredecessors(VerifierData* vdata, VfyBasicBlock* curBlock,
                            u4 curIdx, OpcodeFlags opFlags, u4 nextIdx, u4* handlerList,
                            size_t numHandlers)
{
    const InsnFlags* insnFlags = vdata->insnFlags;
    const Method* meth = vdata->method;

    unsigned int handlerIdx;
    for (handlerIdx = 0; handlerIdx < numHandlers; handlerIdx++) {
        if (!addToPredecessor(vdata, curBlock, handlerList[handlerIdx]))
            return false;
    }

    if ((opFlags & kInstrCanContinue) != 0) {
        if (!addToPredecessor(vdata, curBlock, nextIdx))
            return false;
    }
    if ((opFlags & kInstrCanBranch) != 0) {
        bool unused, gotBranch;
        s4 branchOffset, absOffset;

        gotBranch = dvmGetBranchOffset(meth, insnFlags, curIdx,
                                       &branchOffset, &unused);
        assert(gotBranch);
        absOffset = curIdx + branchOffset;
        assert(absOffset >= 0 && (u4) absOffset < vdata->insnsSize);

        if (!addToPredecessor(vdata, curBlock, absOffset))
            return false;
    }

    if ((opFlags & kInstrCanSwitch) != 0) {
        const u2* curInsn = &meth->insns[curIdx];
        const u2* dataPtr;

        /* these values have already been verified, so we can trust them */
        s4 offsetToData = curInsn[1] | ((s4) curInsn[2]) << 16;
        dataPtr = curInsn + offsetToData;

        /*
         * dataPtr points to the start of the switch data.  The first
         * item is the NOP+magic, the second is the number of entries in
         * the switch table.
         */
        u2 switchCount = dataPtr[1];

        /*
         * Skip past the ident field, size field, and the first_key field
         * (for packed) or the key list (for sparse).
         */
        if (dexOpcodeFromCodeUnit(meth->insns[curIdx]) == OP_PACKED_SWITCH) {
            dataPtr += 4;
        } else {
            assert(dexOpcodeFromCodeUnit(meth->insns[curIdx]) ==
                   OP_SPARSE_SWITCH);
            dataPtr += 2 + 2 * switchCount;
        }

        u4 switchIdx;
        for (switchIdx = 0; switchIdx < switchCount; switchIdx++) {
            s4 offset, absOffset;

            offset = (s4) dataPtr[switchIdx*2] |
                     (s4) (dataPtr[switchIdx*2 +1] << 16);
            absOffset = curIdx + offset;
            assert(absOffset >= 0 && (u4) absOffset < vdata->insnsSize);

            if (!addToPredecessor(vdata, curBlock, absOffset))
                return false;
        }
    }

    if (false) {
        if (dvmPointerSetGetCount(curBlock->predecessors) > 256) {
            ALOGI("Lots of preds at 0x%04x in %s.%s:%s", curIdx,
                  meth->clazz->descriptor, meth->name, meth->shorty);
        }
    }

    return true;
}
static void dumpBasicBlocks(const VerifierData* vdata)
{
    char printBuf[256];
    unsigned int idx;
    int count;

    ALOGI("Basic blocks for %s.%s:%s", vdata->method->clazz->descriptor,
          vdata->method->name, vdata->method->shorty);
    for (idx = 0; idx < vdata->insnsSize; idx++) {
        VfyBasicBlock* block = vdata->basicBlocks[idx];
        if (block == NULL)
            continue;

        assert(block->firstAddr == idx);
        count = snprintf(printBuf, sizeof(printBuf), " %04x-%04x ",
                         block->firstAddr, block->lastAddr);

        PointerSet* preds = block->predecessors;
        size_t numPreds = dvmPointerSetGetCount(preds);

        if (numPreds > 0) {
            count += snprintf(printBuf + count, sizeof(printBuf) - count,
                              "preds:");

            unsigned int predIdx;
            for (predIdx = 0; predIdx < numPreds; predIdx++) {
                if (count >= (int) sizeof(printBuf))
                    break;
                const VfyBasicBlock* pred =
                        (const VfyBasicBlock*) dvmPointerSetGetEntry(preds, predIdx);
                count += snprintf(printBuf + count, sizeof(printBuf) - count,
                                  "%04x(%p),", pred->firstAddr, pred);
            }
        } else {
            count += snprintf(printBuf + count, sizeof(printBuf) - count,
                              "(no preds)");
        }

        printBuf[sizeof(printBuf)-2] = '!';
        printBuf[sizeof(printBuf)-1] = '\0';

        ALOGI("%s", printBuf);
    }

    usleep(100 * 1000);      /* ugh...let logcat catch up */
}

bool dvmComputeVfyBasicBlocks(VerifierData* vdata)
{
    const InsnFlags* insnFlags = vdata->insnFlags;
    const Method* meth = vdata->method;
    const u4 insnsSize = vdata->insnsSize;
    const DexCode* pCode = dvmGetMethodCode(meth);
    const DexTry* pTries = NULL;
    const size_t kHandlerStackAllocSize = 16;   /* max seen so far is 7 */
    u4 handlerAddrs[kHandlerStackAllocSize];
    u4* handlerListAlloc = NULL;
    u4* handlerList = NULL;
    size_t numHandlers = 0;
    u4 idx, blockStartAddr;
    bool result = false;

    bool verbose = false; //dvmWantVerboseVerification(meth);
    if (verbose) {
        ALOGI("Basic blocks for %s.%s:%s",
              meth->clazz->descriptor, meth->name, meth->shorty);
    }

    /*
     * Allocate a data structure that allows us to map from an address to
     * the corresponding basic block.  Initially all pointers are NULL.
     * They are populated on demand as we proceed (either when we reach a
     * new BB, or when we need to add an item to the predecessor list in
     * a not-yet-reached BB).
     *
     * Only the first instruction in the block points to the BB structure;
     * the rest remain NULL.
     */
    vdata->basicBlocks =
            (VfyBasicBlock**) calloc(insnsSize, sizeof(VfyBasicBlock*));
    if (vdata->basicBlocks == NULL)
        return false;

    /*
     * The "tries" list is a series of non-overlapping regions with a list
     * of "catch" handlers.  Rather than do the "find a matching try block"
     * computation at each step, we just walk the "try" list in parallel.
     *
     * Not all methods have "try" blocks.  If this one does, we init tryEnd
     * to zero, so that the (exclusive bound) range check trips immediately.
     */
    u4 tryIndex = 0, tryStart = 0, tryEnd = 0;
    if (pCode->triesSize != 0) {
        pTries = dexGetTries(pCode);
    }

    u4 debugBBIndex = 0;

    /*
     * The address associated with a basic block is the start address.
     */
    blockStartAddr = 0;

    for (idx = 0; idx < insnsSize; ) {
        /*
         * Make sure we're pointing at the right "try" block.  It should
         * not be possible to "jump over" a block, so if we're no longer
         * in the correct one we can just advance to the next.
         */
        if (pTries != NULL && idx >= tryEnd) {
            if (tryIndex == pCode->triesSize) {
                /* no more try blocks in this method */
                pTries = NULL;
                numHandlers = 0;
            } else {
                /*
                 * Extract the set of handlers.  We want to avoid doing
                 * this for each block, so we copy them to local storage.
                 * If it doesn't fit in the small stack area, we'll use
                 * the heap instead.
                 *
                 * It's rare to encounter a method with more than half a
                 * dozen possible handlers.
                 */
                tryStart = pTries[tryIndex].startAddr;
                tryEnd = tryStart + pTries[tryIndex].insnCount;

                if (handlerListAlloc != NULL) {
                    free(handlerListAlloc);
                    handlerListAlloc = NULL;
                }
                numHandlers = extractCatchHandlers(pCode, &pTries[tryIndex],
                                                   handlerAddrs, kHandlerStackAllocSize);
                assert(numHandlers > 0);    // TODO make sure this is verified
                if (numHandlers <= kHandlerStackAllocSize) {
                    handlerList = handlerAddrs;
                } else {
                    ALOGD("overflow, numHandlers=%d", numHandlers);
                    handlerListAlloc = (u4*) malloc(sizeof(u4) * numHandlers);
                    if (handlerListAlloc == NULL)
                        return false;
                    extractCatchHandlers(pCode, &pTries[tryIndex],
                                         handlerListAlloc, numHandlers);
                    handlerList = handlerListAlloc;
                }

                ALOGV("+++ start=%x end=%x numHan=%d",
                      tryStart, tryEnd, numHandlers);

                tryIndex++;
            }
        }

        /*
         * Check the current instruction, and possibly aspects of the
         * next instruction, to see if this instruction ends the current
         * basic block.
         *
         * Instructions that can throw only end the block if there is the
         * possibility of a local handler catching the exception.
         */
        Opcode opcode = dexOpcodeFromCodeUnit(meth->insns[idx]);
        OpcodeFlags opFlags = dexGetFlagsFromOpcode(opcode);
        size_t nextIdx = idx + dexGetWidthFromInstruction(&meth->insns[idx]);
        bool endBB = false;
        bool ignoreInstr = false;

        if ((opFlags & kInstrCanContinue) == 0) {
            /* does not continue */
            endBB = true;
        } else if ((opFlags & (kInstrCanBranch | kInstrCanSwitch)) != 0) {
            /* conditionally branches elsewhere */
            endBB = true;
        } else if ((opFlags & kInstrCanThrow) != 0 &&
                   dvmInsnIsInTry(insnFlags, idx))
        {
            /* throws an exception that might be caught locally */
            endBB = true;
        } else if (isDataChunk(meth->insns[idx])) {
            /*
             * If this is a data chunk (e.g. switch data) we want to skip
             * over it entirely.  Set endBB so we don't carry this along as
             * the start of a block, and ignoreInstr so we don't try to
             * open a basic block for this instruction.
             */
            endBB = ignoreInstr = true;
        } else if (dvmInsnIsBranchTarget(insnFlags, nextIdx)) {
            /*
             * We also need to end it if the next instruction is a branch
             * target.  Note we've tagged exception catch blocks as such.
             *
             * If we're this far along in the "else" chain, we know that
             * this isn't a data-chunk NOP, and control can continue to
             * the next instruction, so we're okay examining "nextIdx".
             */
            assert(nextIdx < insnsSize);
            endBB = true;
        } else if (opcode == OP_NOP && isDataChunk(meth->insns[nextIdx])) {
            /*
             * Handle an odd special case: if this is NOP padding before a
             * data chunk, also treat it as "ignore".  Otherwise it'll look
             * like a block that starts and doesn't end.
             */
            endBB = ignoreInstr = true;
        } else {
            /* check: return ops should be caught by absence of can-continue */
            assert((opFlags & kInstrCanReturn) == 0);
        }

        if (verbose) {
            char btc = dvmInsnIsBranchTarget(insnFlags, idx) ? '>' : ' ';
            char tryc =
                    (pTries != NULL && idx >= tryStart && idx < tryEnd) ? 't' : ' ';
            bool startBB = (idx == blockStartAddr);
            const char* startEnd;


            if (ignoreInstr)
                startEnd = "IGNORE";
            else if (startBB && endBB)
                startEnd = "START/END";
            else if (startBB)
                startEnd = "START";
            else if (endBB)
                startEnd = "END";
            else
                startEnd = "-";

            ALOGI("%04x: %c%c%s #%d", idx, tryc, btc, startEnd, debugBBIndex);

            if (pTries != NULL && idx == tryStart) {
                assert(numHandlers > 0);
                ALOGI("  EXC block: [%04x, %04x) %d:(%04x...)",
                      tryStart, tryEnd, numHandlers, handlerList[0]);
            }
        }

        if (idx != blockStartAddr) {
            /* should not be a basic block struct associated with this addr */
            assert(vdata->basicBlocks[idx] == NULL);
        }
        if (endBB) {
            if (!ignoreInstr) {
                /*
                 * Create a new BB if one doesn't already exist.
                 */
                VfyBasicBlock* curBlock = vdata->basicBlocks[blockStartAddr];
                if (curBlock == NULL) {
                    curBlock = allocVfyBasicBlock(vdata, blockStartAddr);
                    if (curBlock == NULL)
                        return false;
                    vdata->basicBlocks[blockStartAddr] = curBlock;
                }

                curBlock->firstAddr = blockStartAddr;
                curBlock->lastAddr = idx;

                if (!setPredecessors(vdata, curBlock, idx, opFlags, nextIdx,
                                     handlerList, numHandlers))
                {
                    goto bail;
                }
            }

            blockStartAddr = nextIdx;
            debugBBIndex++;
        }

        idx = nextIdx;
    }

    assert(idx == insnsSize);

    result = true;

    if (verbose)
        dumpBasicBlocks(vdata);

    bail:
    free(handlerListAlloc);
    return result;
}
static InstructionWidth* createBackwardWidthTable(VerifierData* vdata)
{
    InstructionWidth* widths;

    widths = (InstructionWidth*)
            calloc(vdata->insnsSize, sizeof(InstructionWidth));
    if (widths == NULL)
        return NULL;

    u4 insnWidth = 0;
    for (u4 idx = 0; idx < vdata->insnsSize; ) {
        widths[idx] = insnWidth;
        insnWidth = dvmInsnGetWidth(vdata->insnFlags, idx);
        idx += insnWidth;
    }

    return widths;
}
bool dvmIsBitSet(const BitVector* pBits, unsigned int num)
{
    assert(num < pBits->storageSize * sizeof(u4) * 8);

    unsigned int val = pBits->storage[num >> 5] & (1 << (num & 0x1f));
    return (val != 0);
}

static void dumpLiveState(const VerifierData* vdata, u4 curIdx,
                          const BitVector* workBits)
{
    u4 insnRegCount = vdata->insnRegCount;
    size_t regCharSize = insnRegCount + (insnRegCount-1)/4 + 2 +1;
    char regChars[regCharSize +1];
    unsigned int idx;

    memset(regChars, ' ', regCharSize);
    regChars[0] = '[';
    if (insnRegCount == 0)
        regChars[1] = ']';
    else
        regChars[1 + (insnRegCount-1) + (insnRegCount-1)/4 +1] = ']';
    regChars[regCharSize] = '\0';

    for (idx = 0; idx < insnRegCount; idx++) {
        char ch = dvmIsBitSet(workBits, idx) ? '+' : '-';
        regChars[1 + idx + (idx/4)] = ch;
    }

    ALOGI("0x%04x %s", curIdx, regChars);
}
static inline void GEN(BitVector* workBits, u4 regIndex)
{
    dvmSetBit(workBits, regIndex);
}

static inline void GENW(BitVector* workBits, u4 regIndex)
{
    dvmSetBit(workBits, regIndex);
    dvmSetBit(workBits, regIndex+1);
}

/*
 * Remove a register from the LIVE set.
 */
static inline void KILL(BitVector* workBits, u4 regIndex)
{
    dvmClearBit(workBits, regIndex);
}

/*
 * Remove a register pair from the LIVE set.
 */
static inline void KILLW(BitVector* workBits, u4 regIndex)
{
    dvmClearBit(workBits, regIndex);
    dvmClearBit(workBits, regIndex+1);
}

static bool processInstruction(VerifierData* vdata, u4 insnIdx,
                               BitVector* workBits)
{
    const Method* meth = vdata->method;
    const u2* insns = meth->insns + insnIdx;
    DecodedInstruction decInsn;

    dexDecodeInstruction(insns, &decInsn);

    /*
     * Add registers to the "GEN" or "KILL" sets.  We want to do KILL
     * before GEN to handle cases where the source and destination
     * register is the same.
     */
    switch (decInsn.opcode) {
        case OP_NOP:
        case OP_RETURN_VOID:
        case OP_GOTO:
        case OP_GOTO_16:
        case OP_GOTO_32:
            /* no registers are used */
            break;

        case OP_RETURN:
        case OP_RETURN_OBJECT:
        case OP_MONITOR_ENTER:
        case OP_MONITOR_EXIT:
        case OP_CHECK_CAST:
        case OP_THROW:
        case OP_PACKED_SWITCH:
        case OP_SPARSE_SWITCH:
        case OP_FILL_ARRAY_DATA:
        case OP_IF_EQZ:
        case OP_IF_NEZ:
        case OP_IF_LTZ:
        case OP_IF_GEZ:
        case OP_IF_GTZ:
        case OP_IF_LEZ:
        case OP_SPUT:
        case OP_SPUT_BOOLEAN:
        case OP_SPUT_BYTE:
        case OP_SPUT_CHAR:
        case OP_SPUT_SHORT:
        case OP_SPUT_OBJECT:
            /* action <- vA */
            GEN(workBits, decInsn.vA);
            break;

        case OP_RETURN_WIDE:
        case OP_SPUT_WIDE:
            /* action <- vA(wide) */
            GENW(workBits, decInsn.vA);
            break;

        case OP_IF_EQ:
        case OP_IF_NE:
        case OP_IF_LT:
        case OP_IF_GE:
        case OP_IF_GT:
        case OP_IF_LE:
        case OP_IPUT:
        case OP_IPUT_BOOLEAN:
        case OP_IPUT_BYTE:
        case OP_IPUT_CHAR:
        case OP_IPUT_SHORT:
        case OP_IPUT_OBJECT:
            /* action <- vA, vB */
            GEN(workBits, decInsn.vA);
            GEN(workBits, decInsn.vB);
            break;

        case OP_IPUT_WIDE:
            /* action <- vA(wide), vB */
            GENW(workBits, decInsn.vA);
            GEN(workBits, decInsn.vB);
            break;

        case OP_APUT:
        case OP_APUT_BOOLEAN:
        case OP_APUT_BYTE:
        case OP_APUT_CHAR:
        case OP_APUT_SHORT:
        case OP_APUT_OBJECT:
            /* action <- vA, vB, vC */
            GEN(workBits, decInsn.vA);
            GEN(workBits, decInsn.vB);
            GEN(workBits, decInsn.vC);
            break;

        case OP_APUT_WIDE:
            /* action <- vA(wide), vB, vC */
            GENW(workBits, decInsn.vA);
            GEN(workBits, decInsn.vB);
            GEN(workBits, decInsn.vC);
            break;

        case OP_FILLED_NEW_ARRAY:
        case OP_INVOKE_VIRTUAL:
        case OP_INVOKE_SUPER:
        case OP_INVOKE_DIRECT:
        case OP_INVOKE_STATIC:
        case OP_INVOKE_INTERFACE:
            /* action <- vararg */
        {
            unsigned int idx;
            for (idx = 0; idx < decInsn.vA; idx++) {
                GEN(workBits, decInsn.arg[idx]);
            }
        }
            break;

        case OP_FILLED_NEW_ARRAY_RANGE:
        case OP_INVOKE_VIRTUAL_RANGE:
        case OP_INVOKE_SUPER_RANGE:
        case OP_INVOKE_DIRECT_RANGE:
        case OP_INVOKE_STATIC_RANGE:
        case OP_INVOKE_INTERFACE_RANGE:
            /* action <- vararg/range */
        {
            unsigned int idx;
            for (idx = 0; idx < decInsn.vA; idx++) {
                GEN(workBits, decInsn.vC + idx);
            }
        }
            break;

        case OP_MOVE_RESULT:
        case OP_MOVE_RESULT_WIDE:
        case OP_MOVE_RESULT_OBJECT:
        case OP_MOVE_EXCEPTION:
        case OP_CONST_4:
        case OP_CONST_16:
        case OP_CONST:
        case OP_CONST_HIGH16:
        case OP_CONST_STRING:
        case OP_CONST_STRING_JUMBO:
        case OP_CONST_CLASS:
        case OP_NEW_INSTANCE:
        case OP_SGET:
        case OP_SGET_BOOLEAN:
        case OP_SGET_BYTE:
        case OP_SGET_CHAR:
        case OP_SGET_SHORT:
        case OP_SGET_OBJECT:
            /* vA <- value */
            KILL(workBits, decInsn.vA);
            break;

        case OP_CONST_WIDE_16:
        case OP_CONST_WIDE_32:
        case OP_CONST_WIDE:
        case OP_CONST_WIDE_HIGH16:
        case OP_SGET_WIDE:
            /* vA(wide) <- value */
            KILLW(workBits, decInsn.vA);
            break;

        case OP_MOVE:
        case OP_MOVE_FROM16:
        case OP_MOVE_16:
        case OP_MOVE_OBJECT:
        case OP_MOVE_OBJECT_FROM16:
        case OP_MOVE_OBJECT_16:
        case OP_INSTANCE_OF:
        case OP_ARRAY_LENGTH:
        case OP_NEW_ARRAY:
        case OP_IGET:
        case OP_IGET_BOOLEAN:
        case OP_IGET_BYTE:
        case OP_IGET_CHAR:
        case OP_IGET_SHORT:
        case OP_IGET_OBJECT:
        case OP_NEG_INT:
        case OP_NOT_INT:
        case OP_NEG_FLOAT:
        case OP_INT_TO_FLOAT:
        case OP_FLOAT_TO_INT:
        case OP_INT_TO_BYTE:
        case OP_INT_TO_CHAR:
        case OP_INT_TO_SHORT:
        case OP_ADD_INT_LIT16:
        case OP_RSUB_INT:
        case OP_MUL_INT_LIT16:
        case OP_DIV_INT_LIT16:
        case OP_REM_INT_LIT16:
        case OP_AND_INT_LIT16:
        case OP_OR_INT_LIT16:
        case OP_XOR_INT_LIT16:
        case OP_ADD_INT_LIT8:
        case OP_RSUB_INT_LIT8:
        case OP_MUL_INT_LIT8:
        case OP_DIV_INT_LIT8:
        case OP_REM_INT_LIT8:
        case OP_SHL_INT_LIT8:
        case OP_SHR_INT_LIT8:
        case OP_USHR_INT_LIT8:
        case OP_AND_INT_LIT8:
        case OP_OR_INT_LIT8:
        case OP_XOR_INT_LIT8:
            /* vA <- vB */
            KILL(workBits, decInsn.vA);
            GEN(workBits, decInsn.vB);
            break;

        case OP_IGET_WIDE:
        case OP_INT_TO_LONG:
        case OP_INT_TO_DOUBLE:
        case OP_FLOAT_TO_LONG:
        case OP_FLOAT_TO_DOUBLE:
            /* vA(wide) <- vB */
            KILLW(workBits, decInsn.vA);
            GEN(workBits, decInsn.vB);
            break;

        case OP_LONG_TO_INT:
        case OP_LONG_TO_FLOAT:
        case OP_DOUBLE_TO_INT:
        case OP_DOUBLE_TO_FLOAT:
            /* vA <- vB(wide) */
            KILL(workBits, decInsn.vA);
            GENW(workBits, decInsn.vB);
            break;

        case OP_MOVE_WIDE:
        case OP_MOVE_WIDE_FROM16:
        case OP_MOVE_WIDE_16:
        case OP_NEG_LONG:
        case OP_NOT_LONG:
        case OP_NEG_DOUBLE:
        case OP_LONG_TO_DOUBLE:
        case OP_DOUBLE_TO_LONG:
            /* vA(wide) <- vB(wide) */
            KILLW(workBits, decInsn.vA);
            GENW(workBits, decInsn.vB);
            break;

        case OP_CMPL_FLOAT:
        case OP_CMPG_FLOAT:
        case OP_AGET:
        case OP_AGET_BOOLEAN:
        case OP_AGET_BYTE:
        case OP_AGET_CHAR:
        case OP_AGET_SHORT:
        case OP_AGET_OBJECT:
        case OP_ADD_INT:
        case OP_SUB_INT:
        case OP_MUL_INT:
        case OP_REM_INT:
        case OP_DIV_INT:
        case OP_AND_INT:
        case OP_OR_INT:
        case OP_XOR_INT:
        case OP_SHL_INT:
        case OP_SHR_INT:
        case OP_USHR_INT:
        case OP_ADD_FLOAT:
        case OP_SUB_FLOAT:
        case OP_MUL_FLOAT:
        case OP_DIV_FLOAT:
        case OP_REM_FLOAT:
            /* vA <- vB, vC */
            KILL(workBits, decInsn.vA);
            GEN(workBits, decInsn.vB);
            GEN(workBits, decInsn.vC);
            break;

        case OP_AGET_WIDE:
            /* vA(wide) <- vB, vC */
            KILLW(workBits, decInsn.vA);
            GEN(workBits, decInsn.vB);
            GEN(workBits, decInsn.vC);
            break;

        case OP_CMPL_DOUBLE:
        case OP_CMPG_DOUBLE:
        case OP_CMP_LONG:
            /* vA <- vB(wide), vC(wide) */
            KILL(workBits, decInsn.vA);
            GENW(workBits, decInsn.vB);
            GENW(workBits, decInsn.vC);
            break;

        case OP_SHL_LONG:
        case OP_SHR_LONG:
        case OP_USHR_LONG:
            /* vA(wide) <- vB(wide), vC */
            KILLW(workBits, decInsn.vA);
            GENW(workBits, decInsn.vB);
            GEN(workBits, decInsn.vC);
            break;

        case OP_ADD_LONG:
        case OP_SUB_LONG:
        case OP_MUL_LONG:
        case OP_DIV_LONG:
        case OP_REM_LONG:
        case OP_AND_LONG:
        case OP_OR_LONG:
        case OP_XOR_LONG:
        case OP_ADD_DOUBLE:
        case OP_SUB_DOUBLE:
        case OP_MUL_DOUBLE:
        case OP_DIV_DOUBLE:
        case OP_REM_DOUBLE:
            /* vA(wide) <- vB(wide), vC(wide) */
            KILLW(workBits, decInsn.vA);
            GENW(workBits, decInsn.vB);
            GENW(workBits, decInsn.vC);
            break;

        case OP_ADD_INT_2ADDR:
        case OP_SUB_INT_2ADDR:
        case OP_MUL_INT_2ADDR:
        case OP_REM_INT_2ADDR:
        case OP_SHL_INT_2ADDR:
        case OP_SHR_INT_2ADDR:
        case OP_USHR_INT_2ADDR:
        case OP_AND_INT_2ADDR:
        case OP_OR_INT_2ADDR:
        case OP_XOR_INT_2ADDR:
        case OP_DIV_INT_2ADDR:
            /* vA <- vA, vB */
            /* KILL(workBits, decInsn.vA); */
            GEN(workBits, decInsn.vA);
            GEN(workBits, decInsn.vB);
            break;

        case OP_SHL_LONG_2ADDR:
        case OP_SHR_LONG_2ADDR:
        case OP_USHR_LONG_2ADDR:
            /* vA(wide) <- vA(wide), vB */
            /* KILLW(workBits, decInsn.vA); */
            GENW(workBits, decInsn.vA);
            GEN(workBits, decInsn.vB);
            break;

        case OP_ADD_LONG_2ADDR:
        case OP_SUB_LONG_2ADDR:
        case OP_MUL_LONG_2ADDR:
        case OP_DIV_LONG_2ADDR:
        case OP_REM_LONG_2ADDR:
        case OP_AND_LONG_2ADDR:
        case OP_OR_LONG_2ADDR:
        case OP_XOR_LONG_2ADDR:
        case OP_ADD_FLOAT_2ADDR:
        case OP_SUB_FLOAT_2ADDR:
        case OP_MUL_FLOAT_2ADDR:
        case OP_DIV_FLOAT_2ADDR:
        case OP_REM_FLOAT_2ADDR:
        case OP_ADD_DOUBLE_2ADDR:
        case OP_SUB_DOUBLE_2ADDR:
        case OP_MUL_DOUBLE_2ADDR:
        case OP_DIV_DOUBLE_2ADDR:
        case OP_REM_DOUBLE_2ADDR:
            /* vA(wide) <- vA(wide), vB(wide) */
            /* KILLW(workBits, decInsn.vA); */
            GENW(workBits, decInsn.vA);
            GENW(workBits, decInsn.vB);
            break;

            /* we will only see this if liveness analysis is done after general vfy */
        case OP_THROW_VERIFICATION_ERROR:
            /* no registers used */
            break;

            /* quickened instructions, not expected to appear */
        case OP_EXECUTE_INLINE:
        case OP_EXECUTE_INLINE_RANGE:
        case OP_IGET_QUICK:
        case OP_IGET_WIDE_QUICK:
        case OP_IGET_OBJECT_QUICK:
        case OP_IPUT_QUICK:
        case OP_IPUT_WIDE_QUICK:
        case OP_IPUT_OBJECT_QUICK:
        case OP_INVOKE_VIRTUAL_QUICK:
        case OP_INVOKE_VIRTUAL_QUICK_RANGE:
        case OP_INVOKE_SUPER_QUICK:
        case OP_INVOKE_SUPER_QUICK_RANGE:
            /* fall through to failure */

            /* correctness fixes, not expected to appear */
        case OP_INVOKE_OBJECT_INIT_RANGE:
        case OP_RETURN_VOID_BARRIER:
        case OP_SPUT_VOLATILE:
        case OP_SPUT_OBJECT_VOLATILE:
        case OP_SPUT_WIDE_VOLATILE:
        case OP_IPUT_VOLATILE:
        case OP_IPUT_OBJECT_VOLATILE:
        case OP_IPUT_WIDE_VOLATILE:
        case OP_SGET_VOLATILE:
        case OP_SGET_OBJECT_VOLATILE:
        case OP_SGET_WIDE_VOLATILE:
        case OP_IGET_VOLATILE:
        case OP_IGET_OBJECT_VOLATILE:
        case OP_IGET_WIDE_VOLATILE:
            /* fall through to failure */

            /* these should never appear during verification */
        case OP_UNUSED_3E:
        case OP_UNUSED_3F:
        case OP_UNUSED_40:
        case OP_UNUSED_41:
        case OP_UNUSED_42:
        case OP_UNUSED_43:
        case OP_UNUSED_73:
        case OP_UNUSED_79:
        case OP_UNUSED_7A:
        case OP_BREAKPOINT:
        case OP_UNUSED_FF:
            return false;
    }

    return true;
}
static void markLocalsCb(void* ctxt, u2 reg, u4 startAddress, u4 endAddress,
                         const char* name, const char* descriptor, const char* signature)
{
    VerifierData* vdata = (VerifierData*) ctxt;
    bool verbose = dvmWantVerboseVerification(vdata->method);

    if (verbose) {
        ALOGI("%04x-%04x %2d (%s %s)",
              startAddress, endAddress, reg, name, descriptor);
    }

    bool wide = (descriptor[0] == 'D' || descriptor[0] == 'J');
    assert(reg <= vdata->insnRegCount + (wide ? 1 : 0));

    /*
     * Set the bit in all GC point instructions in the range
     * [startAddress, endAddress).
     */
    unsigned int idx;
    for (idx = startAddress; idx < endAddress; idx++) {
        BitVector* liveRegs = vdata->registerLines[idx].liveRegs;
        if (liveRegs != NULL) {
            if (wide) {
                GENW(liveRegs, reg);
            } else {
                GEN(liveRegs, reg);
            }
        }
    }
}
static bool markDebugLocals(VerifierData* vdata)
{
    const Method* meth = vdata->method;

    dexDecodeDebugInfo(meth->clazz->pDvmDex->pDexFile, dvmGetMethodCode(meth),
                       meth->clazz->descriptor, meth->prototype.protoIdx, meth->accessFlags,
                       NULL, markLocalsCb, vdata);

    return true;
}
static inline RegType regTypeFromClass(ClassObject* clazz) {
    return (u4) clazz;
}
bool dvmComputeLiveness(VerifierData* vdata)
{
    const InsnFlags* insnFlags = vdata->insnFlags;
    InstructionWidth* backwardWidth;
    VfyBasicBlock* startGuess = NULL;
    BitVector* workBits = NULL;
    bool result = false;

    bool verbose = false; //= dvmWantVerboseVerification(vdata->method);
    if (verbose) {
        const Method* meth = vdata->method;
        ALOGI("Computing liveness for %s.%s:%s",
              meth->clazz->descriptor, meth->name, meth->shorty);
    }

    assert(vdata->registerLines != NULL);

    backwardWidth = createBackwardWidthTable(vdata);
    if (backwardWidth == NULL)
        goto bail;

    /*
     * Allocate space for intra-block work set.  Does not include space
     * for method result "registers", which aren't visible to the GC.
     * (They would be made live by move-result and then die on the
     * instruction immediately before it.)
     */
    workBits = dvmAllocBitVector(vdata->insnRegCount, false);
    if (workBits == NULL)
        goto bail;

    /*
     * We continue until all blocks have been visited, and no block
     * requires further attention ("visited" is set and "changed" is
     * clear).
     *
     * TODO: consider creating a "dense" array of basic blocks to make
     * the walking faster.
     */
    for (int iter = 0;;) {
        VfyBasicBlock* workBlock = NULL;

        if (iter++ > 100000) {
            LOG_VFY_METH(vdata->method, "oh dear");
            dvmAbort();
        }

        /*
         * If a block is marked "changed", we stop and handle it.  If it
         * just hasn't been visited yet, we remember it but keep searching
         * for one that has been changed.
         *
         * The thought here is that this is more likely to let us work
         * from end to start, which reduces the amount of re-evaluation
         * required (both by using "changed" as a work list, and by picking
         * un-visited blocks from the tail end of the method).
         */
        if (startGuess != NULL) {
            assert(startGuess->changed);
            workBlock = startGuess;
        } else {
            for (u4 idx = 0; idx < vdata->insnsSize; idx++) {
                VfyBasicBlock* block = vdata->basicBlocks[idx];
                if (block == NULL)
                    continue;

                if (block->changed) {
                    workBlock = block;
                    break;
                } else if (!block->visited) {
                    workBlock = block;
                }
            }
        }

        if (workBlock == NULL) {
            /* all done */
            break;
        }

        assert(workBlock->changed || !workBlock->visited);
        startGuess = NULL;

        /*
         * Load work bits.  These represent the liveness of registers
         * after the last instruction in the block has finished executing.
         */
        assert(workBlock->liveRegs != NULL);
        dvmCopyBitVector(workBits, workBlock->liveRegs);
        if (verbose) {
            ALOGI("Loaded work bits from last=0x%04x", workBlock->lastAddr);
            dumpLiveState(vdata, 0xfffd, workBlock->liveRegs);
            dumpLiveState(vdata, 0xffff, workBits);
        }

        /*
         * Process a single basic block.
         *
         * If this instruction is a GC point, we want to save the result
         * in the RegisterLine.
         *
         * We don't break basic blocks on every GC point -- in particular,
         * instructions that might throw but have no "try" block don't
         * end a basic block -- so there could be more than one GC point
         * in a given basic block.
         *
         * We could change this, but it turns out to be not all that useful.
         * At first glance it appears that we could share the liveness bit
         * vector between the basic block struct and the register line,
         * but the basic block needs to reflect the state *after* the
         * instruction has finished, while the GC points need to describe
         * the state before the instruction starts.
         */
        u4 curIdx = workBlock->lastAddr;
        while (true) {
            if (!processInstruction(vdata, curIdx, workBits))
                goto bail;

            if (verbose) {
                dumpLiveState(vdata, curIdx + 0x8000, workBits);
            }

            if (dvmInsnIsGcPoint(insnFlags, curIdx)) {
                BitVector* lineBits = vdata->registerLines[curIdx].liveRegs;
                if (lineBits == NULL) {
                    lineBits = vdata->registerLines[curIdx].liveRegs =
                            dvmAllocBitVector(vdata->insnRegCount, false);
                }
                dvmCopyBitVector(lineBits, workBits);
            }

            if (curIdx == workBlock->firstAddr)
                break;
            assert(curIdx >= backwardWidth[curIdx]);
            curIdx -= backwardWidth[curIdx];
        }

        workBlock->visited = true;
        workBlock->changed = false;

        if (verbose) {
            dumpLiveState(vdata, curIdx, workBits);
        }

        /*
         * Merge changes to all predecessors.  If the new bits don't match
         * the old bits, set the "changed" flag.
         */
        PointerSet* preds = workBlock->predecessors;
        size_t numPreds = dvmPointerSetGetCount(preds);
        unsigned int predIdx;

        for (predIdx = 0; predIdx < numPreds; predIdx++) {
            VfyBasicBlock* pred =
                    (VfyBasicBlock*) dvmPointerSetGetEntry(preds, predIdx);

            pred->changed = dvmCheckMergeBitVectors(pred->liveRegs, workBits);
            if (verbose) {
                ALOGI("merging cur=%04x into pred last=%04x (ch=%d)",
                      curIdx, pred->lastAddr, pred->changed);
                dumpLiveState(vdata, 0xfffa, pred->liveRegs);
                dumpLiveState(vdata, 0xfffb, workBits);
            }

            /*
             * We want to set the "changed" flag on unvisited predecessors
             * as a way of guiding the verifier through basic blocks in
             * a reasonable order.  We can't count on variable liveness
             * changing, so we force "changed" to true even if it hasn't.
             */
            if (!pred->visited)
                pred->changed = true;

            /*
             * Keep track of one of the changed blocks so we can start
             * there instead of having to scan through the list.
             */
            if (pred->changed)
                startGuess = pred;
        }
    }

#ifndef NDEBUG
    /*
     * Sanity check: verify that all GC point register lines have a
     * liveness bit vector allocated.  Also, we're not expecting non-GC
     * points to have them.
     */
    u4 checkIdx;
    for (checkIdx = 0; checkIdx < vdata->insnsSize; ) {
        if (dvmInsnIsGcPoint(insnFlags, checkIdx)) {
            if (vdata->registerLines[checkIdx].liveRegs == NULL) {
                LOG_VFY_METH(vdata->method,
                             "GLITCH: no liveRegs for GC point 0x%04x", checkIdx);
                dvmAbort();
            }
        } else if (vdata->registerLines[checkIdx].liveRegs != NULL) {
            LOG_VFY_METH(vdata->method,
                         "GLITCH: liveRegs for non-GC point 0x%04x", checkIdx);
            dvmAbort();
        }
        u4 insnWidth = dvmInsnGetWidth(insnFlags, checkIdx);
        checkIdx += insnWidth;
    }
#endif

    /*
     * Factor in the debug info, if any.
     */
    if (!markDebugLocals(vdata))
        goto bail;

    result = true;

    bail:
    free(backwardWidth);
    dvmFreeBitVector(workBits);
    return result;
}
static int setUninitInstance(UninitInstanceMap* uninitMap, int addr,
                             ClassObject* clazz)
{
    int idx;

    assert(clazz != NULL);

#ifdef VERIFIER_STATS
    gDvm.verifierStats.uninitSearches++;
#endif

    /* TODO: binary search when numEntries > 8 */
    for (idx = uninitMap->numEntries - 1; idx >= 0; idx--) {
        if (uninitMap->map[idx].addr == addr) {
            if (uninitMap->map[idx].clazz != NULL &&
                uninitMap->map[idx].clazz != clazz)
            {
                LOG_VFY("VFY: addr %d already set to %p, not setting to %p",
                        addr, uninitMap->map[idx].clazz, clazz);
                return -1;          // already set to something else??
            }
            uninitMap->map[idx].clazz = clazz;
            return idx;
        }
    }

    LOG_VFY("VFY: addr %d not found in uninit map", addr);
    assert(false);      // shouldn't happen
    return -1;
}
static RegType regTypeFromUninitIndex(int uidx) {
    return (u4) (kRegTypeUninit | (uidx << kRegTypeUninitShift));
}
static ClassObject* lookupClassByDescriptor(const Method* meth,
                                            const char* pDescriptor, VerifyError* pFailure)
{
    /*
     * The javac compiler occasionally puts references to nonexistent
     * classes in signatures.  For example, if you have a non-static
     * inner class with no constructor, the compiler provides
     * a private <init> for you.  Constructing the class
     * requires <init>(parent), but the outer class can't call
     * that because the method is private.  So the compiler
     * generates a package-scope <init>(parent,bogus) method that
     * just calls the regular <init> (the "bogus" part being necessary
     * to distinguish the signature of the synthetic method).
     * Treating the bogus class as an instance of java.lang.Object
     * allows the verifier to process the class successfully.
     */

    //ALOGI("Looking up '%s'", typeStr);
    ClassObject* clazz;
    clazz = dvmFindClassNoInit(pDescriptor, meth->clazz->classLoader);
    if (clazz == NULL) {
        dvmClearOptException(dvmThreadSelf());
        if (strchr(pDescriptor, '$') != NULL) {
            ALOGV("VFY: unable to find class referenced in signature (%s)",
                  pDescriptor);
        } else {
            LOG_VFY("VFY: unable to find class referenced in signature (%s)",
                    pDescriptor);
        }

        if (pDescriptor[0] == '[') {
            /* We are looking at an array descriptor. */

            /*
             * There should never be a problem loading primitive arrays.
             */
            if (pDescriptor[1] != 'L' && pDescriptor[1] != '[') {
                LOG_VFY("VFY: invalid char in signature in '%s'",
                        pDescriptor);
                *pFailure = VERIFY_ERROR_GENERIC;
            }

            /*
             * Try to continue with base array type.  This will let
             * us pass basic stuff (e.g. get array len) that wouldn't
             * fly with an Object.  This is NOT correct if the
             * missing type is a primitive array, but we should never
             * have a problem loading those.  (I'm not convinced this
             * is correct or even useful.  Just use Object here?)
             */
            clazz = dvmFindClassNoInit("[Ljava/lang/Object;",
                                       meth->clazz->classLoader);
        } else if (pDescriptor[0] == 'L') {
            /*
             * We are looking at a non-array reference descriptor;
             * try to continue with base reference type.
             */
            clazz = gDvm.classJavaLangObject;
        } else {
            /* We are looking at a primitive type. */
            LOG_VFY("VFY: invalid char in signature in '%s'", pDescriptor);
            *pFailure = VERIFY_ERROR_GENERIC;
        }

        if (clazz == NULL) {
            *pFailure = VERIFY_ERROR_GENERIC;
        }
    }

    if (dvmIsPrimitiveClass(clazz)) {
        LOG_VFY("VFY: invalid use of primitive type '%s'", pDescriptor);
        *pFailure = VERIFY_ERROR_GENERIC;
        clazz = NULL;
    }

    return clazz;
}

static bool setTypesFromSignature(const Method* meth, RegType* regTypes,
                                  UninitInstanceMap* uninitMap)
{
    DexParameterIterator iterator;
    int actualArgs, expectedArgs, argStart;
    VerifyError failure = VERIFY_ERROR_NONE;
    const char* descriptor;

    dexParameterIteratorInit(&iterator, &meth->prototype);
    argStart = meth->registersSize - meth->insSize;
    expectedArgs = meth->insSize;     /* long/double count as two */
    actualArgs = 0;

    assert(argStart >= 0);      /* should have been verified earlier */

    /*
     * Include the "this" pointer.
     */
    if (!dvmIsStaticMethod(meth)) {
        /*
         * If this is a constructor for a class other than java.lang.Object,
         * mark the first ("this") argument as uninitialized.  This restricts
         * field access until the superclass constructor is called.
         */
        if (isInitMethod(meth) && meth->clazz != gDvm.classJavaLangObject) {
            int uidx = setUninitInstance(uninitMap, kUninitThisArgAddr,
                                         meth->clazz);
            assert(uidx == 0);
            regTypes[argStart + actualArgs] = regTypeFromUninitIndex(uidx);
        } else {
            regTypes[argStart + actualArgs] = regTypeFromClass(meth->clazz);
        }
        actualArgs++;
    }

    for (;;) {
        descriptor = dexParameterIteratorNextDescriptor(&iterator);

        if (descriptor == NULL) {
            break;
        }

        if (actualArgs >= expectedArgs) {
            LOG_VFY("VFY: expected %d args, found more (%s)",
                    expectedArgs, descriptor);
            goto bad_sig;
        }

        switch (*descriptor) {
            case 'L':
            case '[':
                /*
                 * We assume that reference arguments are initialized.  The
                 * only way it could be otherwise (assuming the caller was
                 * verified) is if the current method is <init>, but in that
                 * case it's effectively considered initialized the instant
                 * we reach here (in the sense that we can return without
                 * doing anything or call virtual methods).
                 */
            {
                ClassObject* clazz =
                        lookupClassByDescriptor(meth, descriptor, &failure);
                if (!VERIFY_OK(failure))
                    goto bad_sig;
                regTypes[argStart + actualArgs] = regTypeFromClass(clazz);
            }
                actualArgs++;
                break;
            case 'Z':
                regTypes[argStart + actualArgs] = kRegTypeBoolean;
                actualArgs++;
                break;
            case 'C':
                regTypes[argStart + actualArgs] = kRegTypeChar;
                actualArgs++;
                break;
            case 'B':
                regTypes[argStart + actualArgs] = kRegTypeByte;
                actualArgs++;
                break;
            case 'I':
                regTypes[argStart + actualArgs] = kRegTypeInteger;
                actualArgs++;
                break;
            case 'S':
                regTypes[argStart + actualArgs] = kRegTypeShort;
                actualArgs++;
                break;
            case 'F':
                regTypes[argStart + actualArgs] = kRegTypeFloat;
                actualArgs++;
                break;
            case 'D':
                regTypes[argStart + actualArgs] = kRegTypeDoubleLo;
                regTypes[argStart + actualArgs +1] = kRegTypeDoubleHi;
                actualArgs += 2;
                break;
            case 'J':
                regTypes[argStart + actualArgs] = kRegTypeLongLo;
                regTypes[argStart + actualArgs +1] = kRegTypeLongHi;
                actualArgs += 2;
                break;
            default:
                LOG_VFY("VFY: unexpected signature type char '%c'", *descriptor);
                goto bad_sig;
        }
    }

    if (actualArgs != expectedArgs) {
        LOG_VFY("VFY: expected %d args, found %d", expectedArgs, actualArgs);
        goto bad_sig;
    }

    descriptor = dexProtoGetReturnType(&meth->prototype);

    /*
     * Validate return type.  We don't do the type lookup; just want to make
     * sure that it has the right format.  Only major difference from the
     * method argument format is that 'V' is supported.
     */
    switch (*descriptor) {
        case 'I':
        case 'C':
        case 'S':
        case 'B':
        case 'Z':
        case 'V':
        case 'F':
        case 'D':
        case 'J':
            if (*(descriptor+1) != '\0')
                goto bad_sig;
            break;
        case '[':
            /* single/multi, object/primitive */
            while (*++descriptor == '[')
                ;
            if (*descriptor == 'L') {
                while (*++descriptor != ';' && *descriptor != '\0')
                    ;
                if (*descriptor != ';')
                    goto bad_sig;
            } else {
                if (*(descriptor+1) != '\0')
                    goto bad_sig;
            }
            break;
        case 'L':
            /* could be more thorough here, but shouldn't be required */
            while (*++descriptor != ';' && *descriptor != '\0')
                ;
            if (*descriptor != ';')
                goto bad_sig;
            break;
        default:
            goto bad_sig;
    }

    return true;

//fail:
//    LOG_VFY_METH(meth, "VFY:  bad sig");
//    return false;

    bad_sig:
    {
        char* desc = dexProtoCopyMethodDescriptor(&meth->prototype);
        LOG_VFY("VFY: bad signature '%s' for %s.%s",
                desc, meth->clazz->descriptor, meth->name);
        free(desc);
    }
    return false;
}
static bool doCodeVerification(VerifierData* vdata, RegisterTable* regTable)
{
    const Method* meth = vdata->method;
    InsnFlags* insnFlags = vdata->insnFlags;
    UninitInstanceMap* uninitMap = vdata->uninitMap;
    const int insnsSize = dvmGetMethodInsnsSize(meth);
    bool result = false;
    bool debugVerbose = false;
    int insnIdx, startGuess;

    /*
     * Begin by marking the first instruction as "changed".
     */
    dvmInsnSetChanged(insnFlags, 0, true);

    if (dvmWantVerboseVerification(meth)) {

        debugVerbose = true;
        gDebugVerbose = true;
    } else {
        gDebugVerbose = false;
    }

    startGuess = 0;

    /*
     * Continue until no instructions are marked "changed".
     */
    while (true) {
        /*
         * Find the first marked one.  Use "startGuess" as a way to find
         * one quickly.
         */
        for (insnIdx = startGuess; insnIdx < insnsSize; insnIdx++) {
            if (dvmInsnIsChanged(insnFlags, insnIdx))
                break;
        }

        if (insnIdx == insnsSize) {
            if (startGuess != 0) {
                /* try again, starting from the top */
                startGuess = 0;
                continue;
            } else {
                /* all flags are clear */
                break;
            }
        }

        /*
         * We carry the working set of registers from instruction to
         * instruction.  If this address can be the target of a branch
         * (or throw) instruction, or if we're skipping around chasing
         * "changed" flags, we need to load the set of registers from
         * the table.
         *
         * Because we always prefer to continue on to the next instruction,
         * we should never have a situation where we have a stray
         * "changed" flag set on an instruction that isn't a branch target.
         */
        if (dvmInsnIsBranchTarget(insnFlags, insnIdx)) {
            RegisterLine* workLine = &regTable->workLine;

            copyLineFromTable(workLine, regTable, insnIdx);
        } else {
#ifndef NDEBUG
            /*
             * Sanity check: retrieve the stored register line (assuming
             * a full table) and make sure it actually matches.
             */
            RegisterLine* registerLine = getRegisterLine(regTable, insnIdx);
            if (registerLine->regTypes != NULL &&
                compareLineToTable(regTable, insnIdx, &regTable->workLine) != 0)
            {
                char* desc = dexProtoCopyMethodDescriptor(&meth->prototype);
                LOG_VFY("HUH? workLine diverged in %s.%s %s",
                        meth->clazz->descriptor, meth->name, desc);
                free(desc);
                dumpRegTypes(vdata, registerLine, 0, "work",
                             uninitMap, DRT_SHOW_REF_TYPES | DRT_SHOW_LOCALS);
                dumpRegTypes(vdata, registerLine, 0, "insn",
                             uninitMap, DRT_SHOW_REF_TYPES | DRT_SHOW_LOCALS);
            }
#endif
        }
        if (debugVerbose) {
            dumpRegTypes(vdata, &regTable->workLine, insnIdx,
                         NULL, uninitMap, SHOW_REG_DETAILS);
        }

        //ALOGI("process %s.%s %s %d",
        //    meth->clazz->descriptor, meth->name, meth->descriptor, insnIdx);
        if (!verifyInstruction(meth, insnFlags, regTable, insnIdx,
                               uninitMap, &startGuess))
        {
            //ALOGD("+++ %s bailing at %d", meth->name, insnIdx);
            goto bail;
        }

        /*
         * Clear "changed" and mark as visited.
         */
        dvmInsnSetVisited(insnFlags, insnIdx, true);
        dvmInsnSetChanged(insnFlags, insnIdx, false);
    }

    if (DEAD_CODE_SCAN && !IS_METHOD_FLAG_SET(meth, METHOD_ISWRITABLE)) {
        /*
         * Scan for dead code.  There's nothing "evil" about dead code
         * (besides the wasted space), but it indicates a flaw somewhere
         * down the line, possibly in the verifier.
         *
         * If we've substituted "always throw" instructions into the stream,
         * we are almost certainly going to have some dead code.
         */
        int deadStart = -1;
        for (insnIdx = 0; insnIdx < insnsSize;
             insnIdx += dvmInsnGetWidth(insnFlags, insnIdx))
        {
            /*
             * Switch-statement data doesn't get "visited" by scanner.  It
             * may or may not be preceded by a padding NOP (for alignment).
             */
            int instr = meth->insns[insnIdx];
            if (instr == kPackedSwitchSignature ||
                instr == kSparseSwitchSignature ||
                instr == kArrayDataSignature ||
                (instr == OP_NOP && (insnIdx + 1 < insnsSize) &&
                 (meth->insns[insnIdx+1] == kPackedSwitchSignature ||
                  meth->insns[insnIdx+1] == kSparseSwitchSignature ||
                  meth->insns[insnIdx+1] == kArrayDataSignature)))
            {
                dvmInsnSetVisited(insnFlags, insnIdx, true);
            }

            if (!dvmInsnIsVisited(insnFlags, insnIdx)) {
                if (deadStart < 0)
                    deadStart = insnIdx;
            } else if (deadStart >= 0) {

                deadStart = -1;
            }
        }
        if (deadStart >= 0) {

        }
    }

    result = true;

    bail:
    return result;
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
