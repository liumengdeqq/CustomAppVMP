//
// Created by liu meng on 2018/9/1.
//
#include "InlineNative.h"
#include <jni.h>
#include <dlfcn.h>
const InlineOperation gDvmInlineOpsTable[] = {
        { org_apache_harmony_dalvik_NativeTestTarget_emptyInlineMethod,
                                              "Lorg/apache/harmony/dalvik/NativeTestTarget;",
                                                                    "emptyInlineMethod", "()V" },

        { javaLangString_charAt, "Ljava/lang/String;", "charAt", "(I)C" },
        { javaLangString_compareTo, "Ljava/lang/String;", "compareTo", "(Ljava/lang/String;)I" },
        { javaLangString_equals, "Ljava/lang/String;", "equals", "(Ljava/lang/Object;)Z" },
        { javaLangString_fastIndexOf_II, "Ljava/lang/String;", "fastIndexOf", "(II)I" },
        { javaLangString_isEmpty, "Ljava/lang/String;", "isEmpty", "()Z" },
        { javaLangString_length, "Ljava/lang/String;", "length", "()I" },

        { javaLangMath_abs_int, "Ljava/lang/Math;", "abs", "(I)I" },
        { javaLangMath_abs_long, "Ljava/lang/Math;", "abs", "(J)J" },
        { javaLangMath_abs_float, "Ljava/lang/Math;", "abs", "(F)F" },
        { javaLangMath_abs_double, "Ljava/lang/Math;", "abs", "(D)D" },
        { javaLangMath_min_int, "Ljava/lang/Math;", "min", "(II)I" },
        { javaLangMath_max_int, "Ljava/lang/Math;", "max", "(II)I" },
        { javaLangMath_sqrt, "Ljava/lang/Math;", "sqrt", "(D)D" },
        { javaLangMath_cos, "Ljava/lang/Math;", "cos", "(D)D" },
        { javaLangMath_sin, "Ljava/lang/Math;", "sin", "(D)D" },

        { javaLangFloat_floatToIntBits, "Ljava/lang/Float;", "floatToIntBits", "(F)I" },
        { javaLangFloat_floatToRawIntBits, "Ljava/lang/Float;", "floatToRawIntBits", "(F)I" },
        { javaLangFloat_intBitsToFloat, "Ljava/lang/Float;", "intBitsToFloat", "(I)F" },

        { javaLangDouble_doubleToLongBits, "Ljava/lang/Double;", "doubleToLongBits", "(D)J" },
        { javaLangDouble_doubleToRawLongBits, "Ljava/lang/Double;", "doubleToRawLongBits", "(D)J" },
        { javaLangDouble_longBitsToDouble, "Ljava/lang/Double;", "longBitsToDouble", "(J)D" },

        // These are implemented exactly the same in Math and StrictMath,
        // so we can make the StrictMath calls fast too. Note that this
        // isn't true in general!
        { javaLangMath_abs_int, "Ljava/lang/StrictMath;", "abs", "(I)I" },
        { javaLangMath_abs_long, "Ljava/lang/StrictMath;", "abs", "(J)J" },
        { javaLangMath_abs_float, "Ljava/lang/StrictMath;", "abs", "(F)F" },
        { javaLangMath_abs_double, "Ljava/lang/StrictMath;", "abs", "(D)D" },
        { javaLangMath_min_int, "Ljava/lang/StrictMath;", "min", "(II)I" },
        { javaLangMath_max_int, "Ljava/lang/StrictMath;", "max", "(II)I" },
        { javaLangMath_sqrt, "Ljava/lang/StrictMath;", "sqrt", "(D)D" },
};
bool initInlineNaticeFuction(void *dvm_hand,int apilevel){
    if (dvm_hand) {
        dvmPerformInlineOp4DbgHook = (dvmPerformInlineOp4Dbg_func)dlsym(dvm_hand,"dvmPerformInlineOp4Dbg");
        if (!dvmPerformInlineOp4DbgHook) {
            return JNI_FALSE;
        }


        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}

