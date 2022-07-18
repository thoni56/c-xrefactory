#ifndef FILETAB_H_INCLUDED
#define FILETAB_H_INCLUDED

#include "proto.h"

#include <time.h>
#include <stdbool.h>


/* FILETABLE is an implementation of HASH_TAB but those functions
 * should not be used outside the filetable module

   The .tab field is an array of file items. A file "number" is an
   index into this array. Since the file items have the filename as a
   field, internally you can do

   <filetab>.tab[<fileno>]->name

   externally you should

   getFileItem(<fileno>)->name

 */

typedef struct fileItem {	/* to be renamed to constant pool item TODO: Why?*/
    char *name;
    time_t lastModified;
    time_t lastInspected;
    time_t lastUpdateMtime;
    time_t lastFullUpdateMtime;
    bool cxLoading : 1;
    bool cxLoaded : 1;
    bool cxSaved : 1;
    bool isArgument : 1;
    bool isScheduled : 1;
    bool scheduledToUpdate : 1;
    bool fullUpdateIncludesProcessed : 1;
    bool isInterface : 1;       // class/interface for .class
    bool isFromCxfile : 1;      // is this file indexed in XFiles
    unsigned sourceFileNumber : 20; // file number containing the class definition
    struct classHierarchyReference	*superClasses; /* super-classes references */
    struct classHierarchyReference	*inferiorClasses; /* sub-classes references   */
    int directEnclosingInstance;  /* for Java Only  */
    struct fileItem *next;
} FileItem;


typedef enum {
    FILE_IS_SCHEDULED,
    FILE_IS_ARGUMENT
} FileSource;

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


/* Index into file table for the "NON FILE" */
extern int noFileIndex;


extern void initFileTable(int size);
extern void initNoFileIndex(void);

extern int addToFileTable(FileItem *fileItem);
extern int addFileNameToFileTable(char *name);

extern int  lookupFileTable(char *fileName);
extern bool existsInFileTable(char *fileName);

extern FileItem *getFileItem(int fileIndex);
extern int getNextExistingFileIndex(int index);

extern void mapOverFileTable(void (*fun)(FileItem *));
extern void mapOverFileTableWithIndex(void (*fun)(FileItem *, int));
extern void mapOverFileTableWithPointer(void (*fun)(FileItem *, void *), void *pointer);

extern char *getNextScheduledFile(int *indexP);
extern char *getNextArgumentFile(int *indexP);

#endif
