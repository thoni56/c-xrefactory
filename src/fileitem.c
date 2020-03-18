#include "fileitem.h"
#include <string.h>

#include "globals.h"            /* For s_noneFileIndex */


void fillFileItem(S_fileItem *item, char *name, bool fromCommandLine) {
    memset(item, 0, sizeof(S_fileItem));
    item->name = name;
    item->b.commandLineEntered = fromCommandLine;
    item->directEnclosingInstance = s_noneFileIndex;
    item->b.sourceFile = s_noneFileIndex;
}
