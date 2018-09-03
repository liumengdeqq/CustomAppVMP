#pragma once

#include <jni.h>
#include <string.h>
/**
 * ���APP�ļ�·����
 * @param[in] env JNI������
 * @return ����APP�ļ�·�������·��ʹ�������Ҫͨ��free�����ͷ��ڴ档
 * @note TODO ���������ʹ���˷�����APP�ļ���·����Ӧ�����м���������ġ�
 */
char* GetAppPath(JNIEnv* env);
