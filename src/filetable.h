#ifndef _FILETAB_H_
#define _FILETAB_H_

#include "proto.h"
#include "fileitem.h"

#define HASH_TAB_NAME fileTable
#define HASH_TAB_TYPE FileTable
#define HASH_ELEM_TYPE FileItem

#include "hashtab.th"

#ifndef _IN_FILETAB_C_
#undef HASH_TAB_NAME
#undef HASH_ELEM_TYPE
#endif

#define NON_FILE_NAME "___None___"


extern FileTable fileTable;

/* Index into file table for the "NON FILE" */
extern int s_noneFileIndex;


extern void initFileTable(FileTable *table);
extern bool fileTableExists(FileTable *table, char *fileName);
extern int fileTableLookup(FileTable *table, char *fileName);

#endif
