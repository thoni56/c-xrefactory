#ifndef CFREAD_H
#define CFREAD_H

#include "proto.h"

extern void javaHumanizeLinkName( char *inn, char *outn, int size);
extern S_symbol *cfAddCastsToModule(S_symbol *memb, S_symbol *sup);
extern void addSuperClassOrInterface( S_symbol *memb, S_symbol *supp, int origin );
extern int javaCreateClassFileItem( S_symbol *memb);
extern void addSuperClassOrInterfaceByName(S_symbol *memb, char *super, int origin, int loadSuper);
extern void fsRecMapOnFiles(S_zipArchiveDir *dir, char *zip, char *path, void (*fun)(char *zip, char *file, void *arg), void *arg);
extern int fsIsMember(S_zipArchiveDir **dir, char *fn, unsigned offset,
                      int addFlag, S_zipArchiveDir **place);
extern int zipIndexArchive(char *name);
extern int zipFindFile(char *name, char **resName, S_zipFileTabItem *zipfile);
extern void javaMapZipDirFile(
                              S_zipFileTabItem *zipfile,
                              char *packfile,
                              S_completions *a1,
                              void *a2,
                              int *a3,
                              void (*fun)(MAP_FUN_PROFILE),
                              char *classPath,
                              char *dirname
                              );
extern void javaReadClassFile(char *name, S_symbol *cdef, int loadSuper);

#endif
