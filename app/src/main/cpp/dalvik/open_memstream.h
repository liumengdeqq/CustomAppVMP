
#ifndef __CUTILS_OPEN_MEMSTREAM_H__
#define __CUTILS_OPEN_MEMSTREAM_H__

#include <stdio.h>

#ifndef HAVE_OPEN_MEMSTREAM

#ifdef __cplusplus
extern "C" {
#endif

FILE* open_memstream(char** bufp, size_t* sizep);

#ifdef __cplusplus
}
#endif

#endif /*!HAVE_OPEN_MEMSTREAM*/

#endif /*__CUTILS_OPEN_MEMSTREAM_H__*/
