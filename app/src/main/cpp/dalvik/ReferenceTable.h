//
// Created by liu meng on 2018/8/31.
//

#ifndef CUSTOMAPPVMP_REFERENCETABLE_H
#define CUSTOMAPPVMP_REFERENCETABLE_H

struct ReferenceTable {
    struct  Object**        nextEntry;          /* top of the list */
    struct Object**        table;              /* bottom of the list */

    int             allocEntries;       /* #of entries we have space for */
    int             maxEntries;         /* max #of entries allowed */
};

#endif //CUSTOMAPPVMP_REFERENCETABLE_H
