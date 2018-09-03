#pragma once

#include <stdint.h>
#include "unzip.h"
class FileReader {
public:
    FileReader();
    ~FileReader();

    /**
     * ���ļ���
     * @param[in] filePath �ļ�·����
     */
    bool Open(const char* filePath);

    /**
     * �ƶ��ļ�ָ�뵽ָ��ƫ��λ�á�
     * @param[in] offset �����ļ���ʼλ�õ�ƫ�ơ�
     * @return true���ɹ���false��ʧ�ܡ�
     */
    bool Seek(unsigned int offset);

    /**
     * �رմ򿪵��ļ���
     */
    void Close();

    /**
     * ��ȡһ���޷���shortֵ��
     * @param[out] value ��ȡ�ɹ��򷵻ض�ȡ����ֵ��
     * @return true����ȡ�ɹ���false����ȡʧ�ܡ�
     */
    bool ReadUShort(unsigned short* value);

    /**
     * ��ȡһ���޷�������ֵ��
     * @param[out] value ��ȡ�ɹ��򷵻ض�ȡ����ֵ��
     * @return true����ȡ�ɹ���false����ȡʧ�ܡ�
     */
    bool ReadUInt(unsigned int* value);

    /**
     * ��ȡ�ֽ����顣
     * @param[out] buffer ����ɹ����򷵻ص��ֽ����顣
     * @param[in] count ��ȡ���ֽ�����
     * @return true����ȡ�ɹ���false����ȡʧ�ܡ�
     */
    bool ReadBytes(unsigned char* buffer, int count);

    /**
     * ��ȡ�޷���short���顣
     * @param[out] buffer ��ȡ�ɹ����򷵻�һ��short���顣
     * @param[in] size ������Ԫ�ظ�����
     * @return true����ȡ�ɹ���false����ȡʧ�ܡ�
     */
    bool ReadUShorts(unsigned short* buffer, int size);

private:
    char* mFilePath;
    FILE* mFP;
};

//////////////////////////////////////////////////////////////////////////
// ZipReader

class ZipReader {
public:
    ZipReader(const char* zipFilePath);
    ~ZipReader();

    /**
     * ��zip�ļ���
     */
    bool Open();

    /**
     * ���zip��ĳ���ļ���С��
     * @param[in] fileName ��zip�е��ļ�·����
     * @return �ɹ��������ļ���С��ʧ�ܣ�����0��
     */
    uLong GetFileSizeInZip(const char *fileName);

    /**
     * ��ȡĳ���ļ���
     * @param[in] fileName ��zip�е��ļ�·����
     * @param[out] ��ȡ�������ݡ�
     * @param[in] Ҫ��ȡ�����ݳ��ȡ�
     * @return true���ɹ���false��ʧ�ܡ�
     */
    bool ReadBytes(const char *fileName, unsigned char *buffer, size_t len);

    /**
     * �ر��ļ���
     */
    bool Close();

private:
    char* mZipFilePath;
    unzFile mUnzFile;
};
