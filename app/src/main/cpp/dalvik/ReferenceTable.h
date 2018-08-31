//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_REFERENCETABLE_H
#define CUSTOMAPPVMP_REFERENCETABLE_H

#include "Object.h"
struct ReferenceTable {
    Object**        nextEntry;          /* top of the list */
    Object**        table;              /* bottom of the list */

    int             allocEntries;       /* #of entries we have space for */
    int             maxEntries;         /* max #of entries allowed */
};

#endif //CUSTOMAPPVMP_REFERENCETABLE_H
