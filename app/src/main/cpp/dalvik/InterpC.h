#ifndef CUSTOMAPPVMP_INTERP_C_H
#define CUSTOMAPPVMP_INTERP_C_H

#include <jni.h>


/**
 * �ֽ����������
 * @param[in] Separator ���ݡ�
 * @param[in] env JNI������
 * @param[in] thiz ��ǰ����
 * @param[in] ... �ɱ�������������Java�����Ĳ�����
 * @return 
 */
jvalue BWdvmInterpretPortable(JNIEnv* env);

#endif
