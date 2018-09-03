#include "stdafx.h"
#include "Common.h"
#include "InterpC.h"
#include "Globals.h"
#include "Utils.h"
#include "Resolve.h"
#include <dlfcn.h>
#include "Sync.h"
#include "TypeCheck.h"
#include "Alloc.h"
#include "Class.h"
#include "Array.h"
#include "Stack.h"
#include "Interp.h"
#include "InlineNative.h"
void nativeLog(JNIEnv* env, jobject thiz) {
    MY_LOG_INFO("nativeLog, thiz=%p", thiz);
}

jint separatorTest(JNIEnv* env, jobject thiz, jint value) {
    MY_LOG_INFO("separatorTest - value=%d", value);
    jvalue result = BWdvmInterpretPortable(env);
    return 2;
}

/**
 * ע�᱾�ط�����
 */
bool registerNatives(JNIEnv* env) {
    const char* classDesc = "com/appvmp/MainActivity";
    const JNINativeMethod methods[] = {
        { "separatorTest", "(I)I", (void*) separatorTest },
        { "nativeLog", "()V", (void*) nativeLog }
    };

    jclass clazz = env->FindClass(classDesc);
    if (!clazz) {
        MY_LOG_ERROR("not find class��%s��", classDesc);
        return false;
    }

    bool bRet = false;
    if ( JNI_OK == env->RegisterNatives(clazz, methods, array_size(methods)) ) {
        bRet = true;
    } else {
        MY_LOG_ERROR("register class:%s.register native method fail.", classDesc);
    }
    env->DeleteLocalRef(clazz);
    return bRet;
}

void registerFunctions(JNIEnv* env) {
    if (!registerNatives(env)) {
        MY_LOG_ERROR("registerFunctions fail��");
        return;
    }
}



JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env = NULL;

    if (vm->GetEnv((void **)&env, JNI_VERSION_1_4) != JNI_OK) {
        return JNI_ERR;
    }
    void* dvm_hand = dlopen("libdvm.so", RTLD_NOW);
    initResolveFuction(dvm_hand,16);
    initThreadFuction(dvm_hand,16);
    initSynFuction(dvm_hand,16);
    initTypeCheckFuction(dvm_hand,16);
    initAllocFuction(dvm_hand,16);
    initClassFuction(dvm_hand,16);
    initArrayFuction(dvm_hand,16);
    initCarTableFuction(dvm_hand,16);
    initStackFuction(dvm_hand,16);
    initInterpFuction(dvm_hand,16);
    initExceptionFuction(dvm_hand,16);
    initInlineNaticeFuction(dvm_hand,16);
    // ע�᱾�ط�����
    registerFunctions(env);

//    // ���apk·����
//    gAdvmp.apkPath = GetAppPath(env);
//    MY_LOG_INFO("apk path��%s", gAdvmp.apkPath);
//
//    // �ͷ�yc�ļ���
//    gAdvmp.ycSize = ReleaseYcFile(gAdvmp.apkPath, &gAdvmp.ycData);
//    if (0 == gAdvmp.ycSize) {
//        MY_LOG_WARNING("release Yc file fail!");
//        goto _ret;
//    }
//
//    // ����yc�ļ���
//    gAdvmp.ycFile = new YcFile;
//    if (!gAdvmp.ycFile->parse(gAdvmp.ycData, gAdvmp.ycSize)) {
//        MY_LOG_WARNING("parse Yc file fail.");
//        goto _ret;
//    }

_ret:
    return JNI_VERSION_1_4;
}
