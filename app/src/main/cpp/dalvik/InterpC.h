#pragma once


#include "Exception.h"
#include "DexOpcodes.h"
#include "Resolve.h"
#include "ObjectInlines.h"
#include "Globals.h"
#include "Array.h"
#include "Class.h"
#include "Stack.h"
#include "FindInterface.h"
#include "Alloc.h"
#include "InlineNative.h"
#include "TypeCheck.h"
#include "Sync.h"
#include "JniInternal.h"
/**
 * �ֽ����������
 * @param[in] Separator ���ݡ�
 * @param[in] env JNI������
 * @param[in] thiz ��ǰ����
 * @param[in] ... �ɱ�������������Java�����Ĳ�����
 * @return 
 */
jvalue BWdvmInterpretPortable(JNIEnv* env);
