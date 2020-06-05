#include "fileitem.h"

#include <string.h>
#include "filetab.h"


void fillFileItem(FileItem *item, char *name, bool fromCommandLine) {
    memset(item, 0, sizeof(FileItem));
    item->name = name;
    item->b.commandLineEntered = fromCommandLine;
    item->directEnclosingInstance = s_noneFileIndex;
    item->b.sourceFile = s_noneFileIndex;
}
