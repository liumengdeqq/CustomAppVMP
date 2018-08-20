//
// Created by liu meng on 2018/7/28.
//

#ifndef DUMPDEX_SYSUTIL_H
#define DUMPDEX_SYSUTIL_H

#include "common.h"
struct MemMapping {
    void*   addr;           /* start of data */
    size_t  length;         /* length of data */

    void*   baseAddr;       /* page-aligned base address */
    size_t  baseLength;     /* length of mapping */
};

#endif //DUMPDEX_SYSUTIL_H
