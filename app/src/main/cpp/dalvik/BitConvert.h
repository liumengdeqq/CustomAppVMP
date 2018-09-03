#pragma once

#include <stddef.h>
#include <malloc.h>
/**
 * ���ֽ�����ת��Ϊ�ַ�����
 * @param[in] bytes �ֽ����顣
 * @param[in] size �ֽڳ��ȡ�
 * @return �ɹ��������ַ�����ʧ�ܣ�����NULL��
 * @note ����ָ��ֵ��Ҫ�������ͷš�
 */
char* ToString(unsigned char bytes[], size_t size);

/**
 * ���ֽ�����ת��Ϊ�޷������Ρ�
 * ���bytes����ʼλ�ÿ�ʼ��������3���ֽڵ�bytes����ת��Ϊunsigned int��
 * ���bytes����ʼλ�ú���û��3���ֽڣ���ô���ҵ�ʣ����ֽڣ�Ȼ��һ��ת��Ϊunsigned int��
 * @param[in] bytes �ֽ����顣
 * @param[in] size �ֽڳ��ȡ�
 * @return ����ת���õ����Ρ����bytes�ĳ��ȵ���0����ô������0��
 */
unsigned int ToUInt(unsigned char bytes[], size_t size);

/**
 * ���ֽ�����ת��Ϊ�޷������Ρ�
 * �ὫstartΪ��ʼλ�ð�������3���ֽڵ�bytes����ת��Ϊunsigned int��
 * ���start����û��3���ֽڣ���ô���ҵ�ʣ����ֽڣ�Ȼ��һ��ת��Ϊunsigned int��
 * @param[in] bytes �ֽ����顣
 * @param[in] size �ֽڳ��ȡ�
 * @param[in] start Ҫת������ʼ��ַ��
 * @return ����ת���õ����Ρ����ת��ʧ�ܣ��򷵻�0��
 *         ֻ�е�start���ڵ���sizeʱ���Ż�ת��ʧ�ܡ�
 */
