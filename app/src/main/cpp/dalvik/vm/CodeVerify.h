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
