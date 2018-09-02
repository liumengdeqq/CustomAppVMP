#pragma once

#include <jni.h>
#include "YcFile.h"
#include "unzip.h"

// TODO �����ǵ��Ա�־��
#define _AVMP_DEBUG_

extern const char* gYcFileName;

typedef struct _ADVMPGlobals {
    char* apkPath;

    /**
     * ����yc�ļ������ݡ�
     */
    unsigned char* ycData;
    /**
     * yc�ļ����С��
     */
    uLong ycSize;

    YcFile* ycFile;
    //char* ycFilePath;
} ADVMPGlobals;

extern ADVMPGlobals gAdvmp;
extern JNIEnv *gEnv;
