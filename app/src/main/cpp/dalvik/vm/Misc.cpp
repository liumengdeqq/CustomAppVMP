//
// Created by liu meng on 2018/8/21.
//
#include "Misc.h"
#include <sys/time.h>
#include <malloc.h>
u8 dvmGetRelativeTimeNsec()
{
#ifdef HAVE_POSIX_CLOCKS
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (u8)now.tv_sec*1000000000LL + now.tv_nsec;
#else
    struct timeval now;
    gettimeofday(&now, NULL);
    return (u8)now.tv_sec*1000000000LL + now.tv_usec * 1000LL;
#endif
}
char* dvmDescriptorToName(const char* str)
{
    if (str[0] == 'L') {
        size_t length = strlen(str) - 1;
        char* newStr = (char*)malloc(length);

        if (newStr == NULL) {
            return NULL;
        }

        strlcpy(newStr, str + 1, length);
        return newStr;
    }

    return strdup(str);
}

