//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_SYSUTIL_H
#define CUSTOMAPPVMP_SYSUTIL_H

struct MemMapping {
    void*   addr;           /* start of data */
    size_t  length;         /* length of data */

    void*   baseAddr;       /* page-aligned base address */
    size_t  baseLength;     /* length of mapping */
};

#endif //CUSTOMAPPVMP_SYSUTIL_H
