#ifndef PROJECTSTRUCTURE_H_INCLUDED
#define PROJECTSTRUCTURE_H_INCLUDED

#include "stringlist.h"

extern void scanProjectForFilesAndIncludes(const char *projectDir);
extern void markMissingFilesAsDeleted(StringList *discoveredFiles);

#endif
