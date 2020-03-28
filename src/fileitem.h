#ifndef _FILEITEM_H
#define _FILEITEM_H

#include <time.h>
#include <stdbool.h>


typedef struct fileItem {	/* to be renamed to constant pool item */
    char                *name;
    time_t				lastModified;
    time_t				lastInspected;
    time_t				lastUpdateMtime;
    time_t				lastFullUpdateMtime;
    struct fileItemBits {
        unsigned		cxLoading: 1;
        unsigned		cxLoaded : 1;
        unsigned		cxSaved  : 1;
        unsigned		commandLineEntered : 1;
        unsigned		scheduledToProcess : 1;
        unsigned		scheduledToUpdate : 1;
        unsigned		fullUpdateIncludesProcessed : 1;
        unsigned		isInterface : 1;        // class/interface for .class
        unsigned		isFromCxfile : 1;       // is this file indexed in XFiles
        unsigned		sourceFile : 20;// file containing the class definition
    } b;
    struct chReference	*superClasses;			/* super-classes references */
    struct chReference	*inferiorClasses;			/* sub-classes references   */
    int					directEnclosingInstance;  /* for Java Only  */
    struct fileItem		*next;
} S_fileItem;


/* Replacement for FILLF_fileItem() */
extern void fillFileItem(S_fileItem *item, char *name, bool fromCommandLine);

#endif
