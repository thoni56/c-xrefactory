#ifndef _FILETAB_H_
#define _FILETAB_H_

#include "proto.h"

#define HASH_TAB_NAME fileTab
#define HASH_ELEM_TYPE S_fileItem

#include "hashtab.th"

#ifndef _FILETAB_
#undef HASH_TAB_NAME
#undef HASH_ELEM_TYPE
#endif

extern bool fileTabExists(S_fileTab *table, char *fileName);
extern int fileTabLookup(S_fileTab *table, char *fileName);
#endif
