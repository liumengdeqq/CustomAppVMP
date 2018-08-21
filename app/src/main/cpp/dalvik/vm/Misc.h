//
// Created by liu meng on 2018/8/21.
//
#include "Inlines.h"
#include "common.h"
#ifndef CUSTOMAPPVMP_MISC_H
#define CUSTOMAPPVMP_MISC_H
u8 dvmGetRelativeTimeNsec(void);
INLINE u8 dvmGetRelativeTimeUsec(void) {
    return dvmGetRelativeTimeNsec() / 1000;
}
char* dvmDescriptorToName(const char* str);
#endif //CUSTOMAPPVMP_MISC_H
