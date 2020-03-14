#include "fileitem.h"
#include <string.h>


void fillFileItem(S_fileItem *item, char *name, bool fromCommandLine) {
    memset(item, 0, sizeof(S_fileItem));
    item->name = name;
    item->b.commandLineEntered = fromCommandLine;
}
