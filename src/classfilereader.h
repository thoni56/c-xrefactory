#ifndef CLASSFILEREADER_H_INCLUDED
#define CLASSFILEREADER_H_INCLUDED

#include "proto.h"
#include "symbol.h"

typedef struct zipArchiveDir {
    union {
        struct zipArchiveDir *sub;
        unsigned offset;
    } u;
    struct zipArchiveDir *next;
    char name[1];			/* array of char */
} ZipArchiveDir;

typedef struct {
    char                    fn[MAX_FILE_NAME_SIZE];	/* stored with ';' at the end */
    struct stat				stat;                   /* status of the archive file */
    struct zipArchiveDir	*dir;
} ZipFileTableItem;

extern ZipFileTableItem zipArchiveTable[MAX_JAVA_ZIP_ARCHIVES];

extern void convertLinkNameToClassFileName(char classFileName[], char *linkName);
extern Symbol *cfAddCastsToModule(Symbol *memb, Symbol *sup);
extern void addSuperClassOrInterface(Symbol *memb, Symbol *supp, int origin );
extern int javaCreateClassFileItem(Symbol *memb);
extern void addSuperClassOrInterfaceByName(Symbol *memb, char *super, int origin, LoadSuperOrNot loadSuper);

#endif
