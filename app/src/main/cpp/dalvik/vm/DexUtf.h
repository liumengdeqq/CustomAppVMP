//
// Created by liu meng on 2018/8/20.
//
#include "DexFile.h"
#ifndef CUSTOMAPPVMP_DEXUTF_H
#define CUSTOMAPPVMP_DEXUTF_H
DEX_INLINE u2 dexGetUtf16FromUtf8(const char** pUtf8Ptr)
{
    unsigned int one, two, three;

    one = *(*pUtf8Ptr)++;
    if ((one & 0x80) != 0) {
        /* two- or three-byte encoding */
        two = *(*pUtf8Ptr)++;
        if ((one & 0x20) != 0) {
            /* three-byte encoding */
            three = *(*pUtf8Ptr)++;
            return ((one & 0x0f) << 12) |
                   ((two & 0x3f) << 6) |
                   (three & 0x3f);
        } else {
            /* two-byte encoding */
            return ((one & 0x1f) << 6) |
                   (two & 0x3f);
        }
    } else {
        /* one-byte encoding */
        return one;
    }
}

#endif //CUSTOMAPPVMP_DEXUTF_H
