
#include "io.h"

//////////////////////////////////////////////////////////////////////////
// 

FileReader::FileReader() : mFilePath(NULL), mFP(NULL) {}

FileReader::~FileReader() { Close(); }

// ���ļ���
bool FileReader::Open(const char* filePath) {
    mFilePath = strdup(filePath);
    mFP = fopen(filePath, "r");
}

// �ƶ��ļ�ָ�뵽ָ��ƫ��λ�á�
bool FileReader::Seek(unsigned int offset) {
    if (0 == fseek(mFP, offset, SEEK_SET)) {
        return true;
    } else {
        return false;
    }
}

// �رմ򿪵��ļ���
void FileReader::Close() {
    if (NULL != mFilePath) {
        free(mFilePath);
        mFilePath = NULL;
    }
    if (NULL != mFP) {
        fclose(mFP);
        mFP = NULL;
    }
}

// ��ȡһ���޷���shortֵ��
bool FileReader::ReadUShort(unsigned short* value) {
    size_t ret = fread(value, sizeof(unsigned short), 1, mFP);
    if (ret != sizeof(unsigned short)) {
        return false;
    }
    return true;
}

// ��ȡһ���޷�������ֵ��
bool FileReader::ReadUInt(unsigned int* value) {
    size_t ret = fread(value, sizeof(unsigned int), 1, mFP);
    if (ret != sizeof(unsigned int)) {
        return false;
    }
    return true;
}

// ��ȡ�ֽ����顣
bool FileReader::ReadBytes(unsigned char* buffer, int count) {
    size_t ret = fread(buffer, sizeof(unsigned char), count, mFP);
    if (ret != (sizeof(unsigned char) * count)) {
        return false;
    }
    return true;
}

// ��ȡ�޷���short���顣
bool FileReader::ReadUShorts(unsigned short* buffer, int size) {
    for (int i = 0; i < size; i++) {
        unsigned short value;
        if (!ReadUShort(&value)) {
            return false;
        }
        buffer[i] = value;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
// ZipReader

ZipReader::ZipReader(const char* zipFilePath) : mUnzFile(NULL) {
    mZipFilePath = strdup(zipFilePath);
}

ZipReader::~ZipReader() {
    if (NULL != mZipFilePath) {
        free(mZipFilePath);
    }
    Close();
}

// ��zip�ļ���
bool ZipReader::Open() {
    mUnzFile = unzOpen(mZipFilePath);
    if (NULL == mUnzFile) {
        return false;
    } else {
        return true;
    }
}

// ���zip��ĳ���ļ���С��
uLong ZipReader::GetFileSizeInZip(const char *fileName) {
    if (UNZ_OK != unzLocateFile(mUnzFile, fileName, false)) {
        MY_LOG_WARNING("δ�ҵ��ļ���%s", fileName);
        return 0;
    }
    unz_file_info info = { 0 };

    if (UNZ_OK != unzGetCurrentFileInfo(mUnzFile, &info, NULL, 0, NULL, 0, NULL, 0)) {
        MY_LOG_WARNING("����ļ���Ϣʧ�ܡ�");
        return 0;
    }
    return info.uncompressed_size;
}

// ��ȡĳ���ļ���
bool ZipReader::ReadBytes(const char *fileName, unsigned char *buffer, size_t len) {
    if (UNZ_OK != unzLocateFile(mUnzFile, fileName, false)) {
        MY_LOG_WARNING("��λ�ļ�ʧ�ܡ�");
        return false;
    }
    unz_file_info info = { 0 };

    if (UNZ_OK != unzGetCurrentFileInfo(mUnzFile, &info, NULL, 0, NULL, 0, NULL, 0)) {
        MY_LOG_WARNING("����ļ���Ϣʧ�ܡ�");
        return false;
    }

    if (len > info.uncompressed_size) {
        MY_LOG_WARNING("Ҫ��ȡ�ĳ��ȳ����ļ��ĳ��ȡ�");
        return false;
    }

    // 
    if (UNZ_OK != unzOpenCurrentFile(mUnzFile)) {
        MY_LOG_WARNING("�򿪵�ǰ�ļ�ʧ�ܡ�");
        return false;
    }

    int result;
    if ( ((result = unzReadCurrentFile(mUnzFile, buffer, len)) < 0 ) || (result != len) ) {
        MY_LOG_WARNING("��ȡ��ǰ�ļ�ʧ�ܡ�result=%d.", result);
        return false;
    } else {
        return true;
    }
}

// �ر��ļ���
bool ZipReader::Close() {
    if (NULL == mUnzFile) {
        return true;
    }
    if (UNZ_OK == unzCloseCurrentFile(mUnzFile)) {
        return true;
    }
    return false;
}
