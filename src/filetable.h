#ifndef FILETAB_H_INCLUDED
#define FILETAB_H_INCLUDED


#include "timestamp.h"

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
    FileTimestamp lastModified;
    FileTimestamp lastParsedMtime;
    FileTimestamp knowledgeTime;        // when the CU's knowledge was built (ADR-0032)
    bool needsBrowsingStackRefresh : 1; // entry-point reparsed this file, stack needs update
    bool isDeleted : 1;                 // file no longer exists on disk
    bool isLeftBehind : 1;              // includers were built without stripping it (#80)
    struct fileItem *next;
} FileItem;


#define NO_FILE_NAME "___None___"


/* Index into file table for the "NON FILE" */
extern int NO_FILE_NUMBER;


extern void initFileTable(int size);

extern int addFileItemToFileTable(FileItem *fileItem);
extern int addFileNameToFileTable(char *name);

extern int  getFileNumberFromFileName(char *fileName);
extern bool existsInFileTable(char *fileName);
extern void updateFileModificationTracking(int fileNumber);

extern FileItem *getFileItemWithFileNumber(int fileNumber);
extern int getNextExistingFileNumber(int index);

extern void mapOverFileTable(void (*fun)(FileItem *));
extern void mapOverFileTableWithIndex(void (*fun)(FileItem *, int));
extern void mapOverFileTableWithPointer(void (*fun)(FileItem *, void *), void *pointer);


extern void markFileAsDeleted(int fileNumber);

extern void fileTableMemoryStatistics(void);

#endif
