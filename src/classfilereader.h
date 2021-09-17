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

typedef struct zipFileTableItem {
    char                    fn[MAX_FILE_NAME_SIZE];	/* stored with ';' at the end */
    struct stat				st;						/* status of the archive file */
    struct zipArchiveDir	*dir;
} ZipFileTableItem;

extern ZipFileTableItem zipArchiveTable[MAX_JAVA_ZIP_ARCHIVES];

extern void convertLinkNameToClassFileName(char classFileName[], char *linkName);
extern Symbol *cfAddCastsToModule(Symbol *memb, Symbol *sup);
extern void addSuperClassOrInterface(Symbol *memb, Symbol *supp, int origin );
extern int javaCreateClassFileItem(Symbol *memb);
extern void addSuperClassOrInterfaceByName(Symbol *memb, char *super, int origin, LoadSuperOrNot loadSuper);
extern void fsRecMapOnFiles(ZipArchiveDir *dir, char *zip, char *path,
                            void (*fun)(char *zip, char *file, void *arg),
                            void *arg);
extern bool fsIsMember(ZipArchiveDir **dir, char *fn, unsigned offset,
                       AddYesNo addFlag, ZipArchiveDir **place);
extern int zipIndexArchive(char *name);
extern bool zipFindFile(char *name, char **resName, ZipFileTableItem *zipfile);
extern void javaMapZipDirFile(ZipFileTableItem *zipfile,
                              char *packfile,
                              Completions *a1,
                              void *a2,
                              int *a3,
                              void (*fun)(MAP_FUN_SIGNATURE),
                              char *classPath,
                              char *dirname
);
extern void javaReadClassFile(char *name, Symbol *cdef, LoadSuperOrNot loadSuper);

#endif
