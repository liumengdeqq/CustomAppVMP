//
// Created by liu meng on 2018/7/28.
//

#ifndef DUMPDEX_REGISTERMAP_H
#define DUMPDEX_REGISTERMAP_H

#include "common.h"
struct RegisterMap {
    /* header */
    u1      format;         /* enum RegisterMapFormat; MUST be first entry */
    u1      regWidth;       /* bytes per register line, 1+ */
    u1      numEntries[2];  /* number of entries */

    /* raw data starts here; need not be aligned */
    u1      data[1];
};
#endif //DUMPDEX_REGISTERMAP_H
