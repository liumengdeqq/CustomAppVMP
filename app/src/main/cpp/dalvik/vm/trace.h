//
// Created by liu meng on 2018/8/21.
//
#include "common.h"
#include <unistd.h>
#ifndef CUSTOMAPPVMP_TRACE_H
#define CUSTOMAPPVMP_TRACE_H
#define ATRACE_BEGIN(name) atrace_begin(ATRACE_TAG, name)
static inline void atrace_begin(uint64_t tag, const char* name)
{

}

/**
 * Trace the end of a context.
 * This should match up (and occur after) a corresponding ATRACE_BEGIN.
 */
#define ATRACE_END() atrace_end(ATRACE_TAG)
static inline void atrace_end(uint64_t tag)
{

}

#endif //CUSTOMAPPVMP_TRACE_H
