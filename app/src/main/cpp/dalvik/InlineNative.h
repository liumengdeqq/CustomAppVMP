//
// Created by liu meng on 2018/9/1.
//

#ifndef CUSTOMAPPVMP_INLINENATIVE_H
#define CUSTOMAPPVMP_INLINENATIVE_H

#include "object.h"
#include "ObjectInlines.h"
#include "Exception.h"
#include "Globals.h"
#include "UtfString.h"
#include <math.h>
enum NativeInlineOps {
    INLINE_EMPTYINLINEMETHOD = 0,
    INLINE_STRING_CHARAT = 1,
    INLINE_STRING_COMPARETO = 2,
    INLINE_STRING_EQUALS = 3,
    INLINE_STRING_FASTINDEXOF_II = 4,
    INLINE_STRING_IS_EMPTY = 5,
    INLINE_STRING_LENGTH = 6,
    INLINE_MATH_ABS_INT = 7,
    INLINE_MATH_ABS_LONG = 8,
    INLINE_MATH_ABS_FLOAT = 9,
    INLINE_MATH_ABS_DOUBLE = 10,
    INLINE_MATH_MIN_INT = 11,
    INLINE_MATH_MAX_INT = 12,
    INLINE_MATH_SQRT = 13,
    INLINE_MATH_COS = 14,
    INLINE_MATH_SIN = 15,
    INLINE_FLOAT_TO_INT_BITS = 16,
    INLINE_FLOAT_TO_RAW_INT_BITS = 17,
    INLINE_INT_BITS_TO_FLOAT = 18,
    INLINE_DOUBLE_TO_LONG_BITS = 19,
    INLINE_DOUBLE_TO_RAW_LONG_BITS = 20,
    INLINE_LONG_BITS_TO_DOUBLE = 21,
    INLINE_STRICT_MATH_ABS_INT = 22,
    INLINE_STRICT_MATH_ABS_LONG = 23,
    INLINE_STRICT_MATH_ABS_FLOAT = 24,
    INLINE_STRICT_MATH_ABS_DOUBLE = 25,
    INLINE_STRICT_MATH_MIN_INT = 26,
    INLINE_STRICT_MATH_MAX_INT = 27,
    INLINE_STRICT_MATH_SQRT = 28,
};
typedef bool (*InlineOp4Func)(u4 arg0, u4 arg1, u4 arg2, u4 arg3,
                              JValue* pResult);
struct InlineOperation {
    InlineOp4Func   func;               /* MUST be first entry */
    const char*     classDescriptor;
    const char*     methodName;
    const char*     methodSignature;
};
typedef bool (*dvmPerformInlineOp4Dbg_func)(u4 arg0, u4 arg1, u4 arg2, u4 arg3,
                                            JValue* pResult, int opIndex);
extern dvmPerformInlineOp4Dbg_func dvmPerformInlineOp4DbgHook;
extern const InlineOperation gDvmInlineOpsTable[];
INLINE bool dvmPerformInlineOp4Std(u4 arg0, u4 arg1, u4 arg2, u4 arg3,
                                   JValue* pResult, int opIndex)
{
    return (*gDvmInlineOpsTable[opIndex].func)(arg0, arg1, arg2, arg3, pResult);
}

bool initInlineNaticeFuction(void *dvm_hand,int apilevel);
#endif //CUSTOMAPPVMP_INLINENATIVE_H
