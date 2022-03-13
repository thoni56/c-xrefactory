#ifndef FILETAB_H_INCLUDED
#define FILETAB_H_INCLUDED

#include "proto.h"

#include <time.h>
#include <stdbool.h>


/* FILETAB:

   The .tab field is an array of file items. A file "number" is an
   index into this array. Since the file items have the filename as a
   field, all you need to find a filename from a "number" is

   <filetab>.tab[<fileno>]->name

 */

typedef struct fileItem {	/* to be renamed to constant pool item TODO: Why?*/
    char *name;
    time_t lastModified;
    time_t lastInspected;
    time_t lastUpdateMtime;
    time_t lastFullUpdateMtime;
    struct fileItemBits {
        bool cxLoading : 1;
        bool cxLoaded : 1;
        bool cxSaved : 1;
        bool commandLineEntered : 1;
        bool scheduledToProcess : 1;
        bool scheduledToUpdate : 1;
        bool fullUpdateIncludesProcessed : 1;
        bool isInterface : 1;        // class/interface for .class
        bool isFromCxfile : 1;       // is this file indexed in XFiles
        unsigned sourceFileNumber : 20;    // file number containing the class definition
    } b;
    struct classHierarchyReference	*superClasses; /* super-classes references */
    struct classHierarchyReference	*inferiorClasses; /* sub-classes references   */
    int directEnclosingInstance;  /* for Java Only  */
    struct fileItem *next;
} FileItem;


#define HASH_TAB_NAME fileTable
#define HASH_TAB_TYPE FileTable
#define HASH_ELEM_TYPE FileItem

#include "hashtab.th"

#ifndef IN_FILETAB_C
#undef HASH_TAB_NAME
#undef HASH_TAB_TYPE
#undef HASH_ELEM_TYPE
#endif

#define NO_FILE_NAME "___None___"


extern FileTable fileTable;

/* Index into file table for the "NON FILE" */
extern int noFileIndex;

extern void initFileTable(void);
extern void initNoFile(void);
extern bool existsInFileTable(char *fileName);
extern int fileTableLookup(FileTable *table, char *fileName);
extern int addFileTableItem(char *name);
extern FileItem *getFileItem(int fileIndex);
extern int getNextExistingFileIndex(int index);

extern void mapOverFileTable(void (*fun)(FileItem *));
extern void mapOverFileTableWithIndex(void (*fun)(FileItem *, int));
extern void mapOverFileTableWithPointer(void (*fun)(FileItem *, void *), void *pointer);

extern int  lookupFileTable(char *fileName);

#endif
