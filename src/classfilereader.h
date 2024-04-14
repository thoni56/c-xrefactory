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


extern void convertLinkNameToClassFileName(char classFileName[], char *linkName);
extern Symbol *cfAddCastsToModule(Symbol *memb, Symbol *sup);
extern int javaCreateClassFileItem(Symbol *memb);

#endif
