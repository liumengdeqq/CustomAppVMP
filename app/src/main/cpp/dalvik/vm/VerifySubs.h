//
// Created by liu meng on 2018/8/23.
//
#include "common.h"
#include "DexOpcodes.h"
#include "object.h"
#ifndef CUSTOMAPPVMP_VERIFYSUBS_H
#define CUSTOMAPPVMP_VERIFYSUBS_H
typedef u4 InsnFlags;
#define kInsnFlagWidthMask      0x0000ffff
#define kInsnFlagInTry          (1 << 16)
#define kInsnFlagBranchTarget   (1 << 17)
#define kInsnFlagGcPoint        (1 << 18)
#define kInsnFlagVisited        (1 << 30)
#define kInsnFlagChanged        (1 << 31)
bool dvmGetBranchOffset(const Method* meth, const InsnFlags* insnFlags,
                        int curOffset, s4* pOffset, bool* pConditional)
{
    const u2* insns = meth->insns + curOffset;

    switch (*insns & 0xff) {
        case OP_GOTO:
            *pOffset = ((s2) *insns) >> 8;
            *pConditional = false;
            break;
        case OP_GOTO_32:
            *pOffset = insns[1] | (((u4) insns[2]) << 16);
            *pConditional = false;
            break;
        case OP_GOTO_16:
            *pOffset = (s2) insns[1];
            *pConditional = false;
            break;
        case OP_IF_EQ:
        case OP_IF_NE:
        case OP_IF_LT:
        case OP_IF_GE:
        case OP_IF_GT:
        case OP_IF_LE:
        case OP_IF_EQZ:
        case OP_IF_NEZ:
        case OP_IF_LTZ:
        case OP_IF_GEZ:
        case OP_IF_GTZ:
        case OP_IF_LEZ:
            *pOffset = (s2) insns[1];
            *pConditional = true;
            break;
        default:
            return false;
            break;
    }

    return true;
}

#endif //CUSTOMAPPVMP_VERIFYSUBS_H
