#include "stdafx.h"
#include "Common.h"
#include "InterpC.h"
#include "Globals.h"
#include "Utils.h"
#include "Resolve.h"
#ifdef _AVMP_DEBUG_
#include <dlfcn.h>
#include "Thread.h"
#include "Sync.h"
void nativeLog(JNIEnv* env, jobject thiz) {
    MY_LOG_INFO("nativeLog, thiz=%p", thiz);
}

jint separatorTest(JNIEnv* env, jobject thiz, jint value) {
    MY_LOG_INFO("separatorTest - value=%d", value);
    jvalue result = BWdvmInterpretPortable(gAdvmp.ycFile->GetSeparatorData(0), env, thiz, value);
    return result.i;
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

#else

//+${replaceAll}

#endif

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env = NULL;

    if (vm->GetEnv((void **)&env, JNI_VERSION_1_4) != JNI_OK) {
        return JNI_ERR;
    }
    void* dvm_hand = dlopen("libdvm.so", RTLD_NOW);
    initResolveFuction(dvm_hand,16);
    initThreadFuction(dvm_hand,16);
    initSynFuction(dvm_hand,16);
    // ע�᱾�ط�����
    registerFunctions(env);

    // ���apk·����
    gAdvmp.apkPath = GetAppPath(env);
    MY_LOG_INFO("apk path��%s", gAdvmp.apkPath);

    // �ͷ�yc�ļ���
    gAdvmp.ycSize = ReleaseYcFile(gAdvmp.apkPath, &gAdvmp.ycData);
    if (0 == gAdvmp.ycSize) {
        MY_LOG_WARNING("release Yc file fail!");
        goto _ret;
    }

    // ����yc�ļ���
    gAdvmp.ycFile = new YcFile;
    if (!gAdvmp.ycFile->parse(gAdvmp.ycData, gAdvmp.ycSize)) {
        MY_LOG_WARNING("parse Yc file fail.");
        goto _ret;
    }

_ret:
    return JNI_VERSION_1_4;
}
