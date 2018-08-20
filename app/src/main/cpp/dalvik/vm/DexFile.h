//
// Created by liu meng on 2018/7/28.
//

#ifndef DUMPDEX_DEXFILE_H_H
#define DUMPDEX_DEXFILE_H_H

#include "common.h"
#define BUFLEN 1024
#define TAG "DEXDUMP2"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#ifndef _DEX_GEN_INLINES             /* only defined by DexInlines.c */
# define DEX_INLINE extern __inline__
#else
# define DEX_INLINE
#endif

typedef uint8_t             u1;
typedef uint16_t            u2;
typedef uint32_t            u4;
typedef uint64_t            u8;
typedef int8_t              s1;
typedef int16_t             s2;
typedef int32_t             s4;
typedef int64_t             s8;
enum {
    ACC_PUBLIC       = 0x00000001,       // class, field, method, ic
    ACC_PRIVATE      = 0x00000002,       // field, method, ic
    ACC_PROTECTED    = 0x00000004,       // field, method, ic
    ACC_STATIC       = 0x00000008,       // field, method, ic
    ACC_FINAL        = 0x00000010,       // class, field, method, ic
    ACC_SYNCHRONIZED = 0x00000020,       // method (only allowed on natives)
    ACC_SUPER        = 0x00000020,       // class (not used in Dalvik)
    ACC_VOLATILE     = 0x00000040,       // field
    ACC_BRIDGE       = 0x00000040,       // method (1.5)
    ACC_TRANSIENT    = 0x00000080,       // field
    ACC_VARARGS      = 0x00000080,       // method (1.5)
    ACC_NATIVE       = 0x00000100,       // method
    ACC_INTERFACE    = 0x00000200,       // class, ic
    ACC_ABSTRACT     = 0x00000400,       // class, method, ic
    ACC_STRICT       = 0x00000800,       // method
    ACC_SYNTHETIC    = 0x00001000,       // field, method, ic
    ACC_ANNOTATION   = 0x00002000,       // class, ic (1.5)
    ACC_ENUM         = 0x00004000,       // class, field, ic (1.5)
    ACC_CONSTRUCTOR  = 0x00010000,       // method (Dalvik only)
    ACC_DECLARED_SYNCHRONIZED =
    0x00020000,       // method (Dalvik only)
    ACC_CLASS_MASK =
    (ACC_PUBLIC | ACC_FINAL | ACC_INTERFACE | ACC_ABSTRACT
     | ACC_SYNTHETIC | ACC_ANNOTATION | ACC_ENUM),
    ACC_INNER_CLASS_MASK =
    (ACC_CLASS_MASK | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC),
    ACC_FIELD_MASK =
    (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL
     | ACC_VOLATILE | ACC_TRANSIENT | ACC_SYNTHETIC | ACC_ENUM),
    ACC_METHOD_MASK =
    (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL
     | ACC_SYNCHRONIZED | ACC_BRIDGE | ACC_VARARGS | ACC_NATIVE
     | ACC_ABSTRACT | ACC_STRICT | ACC_SYNTHETIC | ACC_CONSTRUCTOR
     | ACC_DECLARED_SYNCHRONIZED),
};

struct DexCode {
    u2  registersSize;
    u2  insSize;
    u2  outsSize;
    u2  triesSize;
    u4  debugInfoOff;       /* file offset to debug info stream */
    u4  insnsSize;          /* size of the insns array, in u2 units */
    u2  insns[1];
    /* followed by optional u2 padding */
    /* followed by try_item[triesSize] */
    /* followed by uleb128 handlersSize */
    /* followed by catch_handler_item[handlersSize] */
};

enum PrimitiveType {
    PRIM_NOT        = 0,       /* value is a reference type, not a primitive type */
    PRIM_VOID       = 1,
    PRIM_BOOLEAN    = 2,
    PRIM_BYTE       = 3,
    PRIM_SHORT      = 4,
    PRIM_CHAR       = 5,
    PRIM_INT        = 6,
    PRIM_LONG       = 7,
    PRIM_FLOAT      = 8,
    PRIM_DOUBLE     = 9,
};
enum { kSHA1DigestLen = 20,
    kSHA1DigestOutputLen = kSHA1DigestLen*2 +1 };

struct DexHeader {
    u1  magic[8];           /* includes version number */
    u4  checksum;           /* adler32 checksum */
    u1  signature[kSHA1DigestLen]; /* SHA-1 hash */
    u4  fileSize;           /* length of entire file */
    u4  headerSize;         /* offset to start of next section */
    u4  endianTag;
    u4  linkSize;
    u4  linkOff;
    u4  mapOff;
    u4  stringIdsSize;
    u4  stringIdsOff;
    u4  typeIdsSize;
    u4  typeIdsOff;
    u4  protoIdsSize;
    u4  protoIdsOff;
    u4  fieldIdsSize;
    u4  fieldIdsOff;
    u4  methodIdsSize;
    u4  methodIdsOff;
    u4  classDefsSize;
    u4  classDefsOff;
    u4  dataSize;
    u4  dataOff;
};

struct DexOptHeader {
    u1  magic[8];           /* includes version number */

    u4  dexOffset;          /* file offset of DEX header */
    u4  dexLength;
    u4  depsOffset;         /* offset of optimized DEX dependency table */
    u4  depsLength;
    u4  optOffset;          /* file offset of optimized data tables */
    u4  optLength;

    u4  flags;              /* some info flags */
    u4  checksum;           /* adler32 checksum covering deps/opt */

    /* pad for 64-bit alignment if necessary */
};


struct DexStringId {
    u4 stringDataOff;      /* file offset to string_data_item */
};

struct DexTypeId {
    u4  descriptorIdx;      /* index into stringIds list for type descriptor */
};

struct DexFieldId {
    u2  classIdx;           /* index into typeIds list for defining class */
    u2  typeIdx;            /* index into typeIds for field type */
    u4  nameIdx;            /* index into stringIds for field name */
};

struct DexMethodId {
    u2  classIdx;           /* index into typeIds list for defining class */
    u2  protoIdx;           /* index into protoIds for method prototype */
    u4  nameIdx;            /* index into stringIds for method name */
};

struct DexProtoId {
    u4  shortyIdx;          /* index into stringIds for shorty descriptor */
    u4  returnTypeIdx;      /* index into typeIds list for return type */
    u4  parametersOff;      /* file offset to type_list for parameter types */
};

struct DexClassDef {
    u4  classIdx;           /* index into typeIds for this class */
    u4  accessFlags;
    u4  superclassIdx;      /* index into typeIds for superclass */
    u4  interfacesOff;      /* file offset to DexTypeList */
    u4  sourceFileIdx;      /* index into stringIds for source file name */
    u4  annotationsOff;     /* file offset to annotations_directory_item */
    u4  classDataOff;       /* file offset to class_data_item */
    u4  staticValuesOff;    /* file offset to DexEncodedArray */
};

struct DexLink {
    u1  bleargh;
};

struct DexClassLookup {
    int     size;                       // total size, including "size"
    int     numEntries;                 // size of table[]; always power of 2
    struct {
        u4      classDescriptorHash;    // class descriptor hash code
        int     classDescriptorOffset;  // in bytes, from start of DEX
        int     classDefOffset;         // in bytes, from start of DEX
    } table[1];
};

struct DexFile {
    /* directly-mapped "opt" header */
    const DexOptHeader* pOptHeader;

    /* pointers to directly-mapped structs and arrays in base DEX */
    const DexHeader*    pHeader;
    const DexStringId*  pStringIds;
    const DexTypeId*    pTypeIds;
    const DexFieldId*   pFieldIds;
    const DexMethodId*  pMethodIds;
    const DexProtoId*   pProtoIds;
    const DexClassDef*  pClassDefs;
    const DexLink*      pLinkData;

    /*
     * These are mapped out of the "auxillary" section, and may not be
     * included in the file.
     */
    const DexClassLookup* pClassLookup;
    const void*         pRegisterMapPool;       // RegisterMapClassPool

    /* points to start of DEX file data */
    const u1*           baseAddr;

    /* track memory overhead for auxillary structures */
    int                 overhead;

    /* additional app-specific data structures associated with the DEX */
    //void*               auxData;
};
struct DexTry {
    u4  startAddr;          /* start address, in 16-bit code units */
    u2  insnCount;          /* instruction count, in 16-bit code units */
    u2  handlerOff;         /* offset in encoded handler data to handlers */
};
const DexTry* dexGetTries(const DexCode* pCode) {
    const u2* insnsEnd = &pCode->insns[pCode->insnsSize];

    // Round to four bytes.
    if ((((uintptr_t) insnsEnd) & 3) != 0) {
        insnsEnd++;
    }

    return (const DexTry*) insnsEnd;
}

/* Get the base of the encoded data for the given DexCode. */
const u1* dexGetCatchHandlerData(const DexCode* pCode) {
    const DexTry* pTries = dexGetTries(pCode);
    return (const u1*) &pTries[pCode->triesSize];
}
const char* dexStringAndSizeById(const DexFile* pDexFile, u4 idx,
                                 u4* utf16Size);
/* return the StringId with the specified index */
DEX_INLINE const DexStringId* dexGetStringId(const DexFile* pDexFile, u4 idx) {
    assert(idx < pDexFile->pHeader->stringIdsSize);
    return &pDexFile->pStringIds[idx];
}

#endif //DUMPDEX_DEXFILE_H_H
