#ifndef PROJECTSTRUCTURE_H_INCLUDED
#define PROJECTSTRUCTURE_H_INCLUDED

#include "stringlist.h"

extern StringList *scanProjectForFilesAndIncludes(const char *projectDir, StringList *includeDirs, StringList *prunePaths);
extern void markMissingFilesAsDeleted(StringList *discoveredFiles);

#endif
