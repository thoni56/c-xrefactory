#ifndef _FILETAB_H_
#define _FILETAB_H_

#include "proto.h"
#include "fileitem.h"

#define HASH_TAB_NAME fileTab
#define HASH_TAB_TYPE FileTab
#define HASH_ELEM_TYPE FileItem

#include "hashtab.th"

#ifndef _FILETAB_
#undef HASH_TAB_NAME
#undef HASH_ELEM_TYPE
#endif

#define NON_FILE_NAME "___None___"


extern FileTab fileTable;

/* Index into file table for the "NON FILE" */
extern int s_noneFileIndex;


extern void initFileTab(FileTab *table);
extern bool fileTabExists(FileTab *table, char *fileName);
extern int fileTabLookup(FileTab *table, char *fileName);

#endif
