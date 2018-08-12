#pragma once

#include <jni.h>
#include "YcFile.h"
#include "unzip.h"

#define _AVMP_DEBUG_

extern const char* gYcFileName;

typedef struct _ADVMPGlobals {
    char* apkPath;
    unsigned char* ycData;
    uLong ycSize;
    YcFile* ycFile;
} ADVMPGlobals;

extern ADVMPGlobals gAdvmp;
