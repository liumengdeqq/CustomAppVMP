//
// Created by liu meng on 2018/8/20.
//
#include "Inlines.h"
#include "object.h"
#ifndef CUSTOMAPPVMP_CLASS_H
#define CUSTOMAPPVMP_CLASS_H
INLINE bool dvmIsClassInitialized(const ClassObject* clazz) {
    return (clazz->status == CLASS_INITIALIZED);
}
bool dvmIsClassInitializing(const ClassObject* clazz);
enum ClassPathEntryKind {
    kCpeUnknown = 0,
    kCpeJar,
    kCpeDex,
    kCpeLastEntry       /* used as sentinel at end of array */
};

struct ClassPathEntry {
    ClassPathEntryKind kind;
    char*   fileName;
    void*   ptr;            /* JarFile* or DexFile* */
};
bool dvmInitClass(ClassObject* clazz);
#endif //CUSTOMAPPVMP_CLASS_H
