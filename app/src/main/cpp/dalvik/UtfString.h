//
// Created by liu meng on 2018/9/2.
//

#ifndef CUSTOMAPPVMP_UTFSTRING_H
#define CUSTOMAPPVMP_UTFSTRING_H
#ifdef USE_GLOBAL_STRING_DEFS
# define STRING_FIELDOFF_VALUE      gDvm.offJavaLangString_value
# define STRING_FIELDOFF_OFFSET     gDvm.offJavaLangString_offset
# define STRING_FIELDOFF_COUNT      gDvm.offJavaLangString_count
# define STRING_FIELDOFF_HASHCODE   gDvm.offJavaLangString_hashCode
#else
# define STRING_FIELDOFF_VALUE      8
# define STRING_FIELDOFF_HASHCODE   12
# define STRING_FIELDOFF_OFFSET     16
# define STRING_FIELDOFF_COUNT      20
#endif

#endif //CUSTOMAPPVMP_UTFSTRING_H
