#include "projectstructure.h"

#include <string.h>

#include "commons.h"
#include "fileio.h"
#include "filetable.h"
#include "misc.h"
#include "stringlist.h"
#include "yylex.h"

#define SCAN_BUFFER_SIZE 4096

void scanProjectForFilesAndIncludes(const char *projectDir) {
    StringList *files = listFilesInDirectory(projectDir);
    for (StringList *f = files; f != NULL; f = f->next) {
        if (!isCompilationUnit(f->string))
            continue;
        addFileNameToFileTable(f->string);

        FILE *file = openFile(f->string, "r");
        char buf[SCAN_BUFFER_SIZE];
        size_t leftover = 0;
        size_t bytesRead;
        while ((bytesRead = readFile(file, buf + leftover, 1, sizeof(buf) - 1 - leftover)) > 0) {
            size_t total = leftover + bytesRead;
            buf[total] = '\0';
            leftover = 0;

            char *line = buf;
            while (line < buf + total) {
                char *eol = strchr(line, '\n');
                if (eol != NULL)
                    *eol = '\0';
                else {
                    /* Incomplete line — carry over to next read */
                    leftover = buf + total - line;
                    memmove(buf, line, leftover);
                    break;
                }

                char includedFile[256];
                if (sscanf(line, " # include \"%255[^\"]\"", includedFile) == 1) {
                    char *dir = directoryName_static(f->string);
                    char *resolved = normalizeFileName_static(includedFile, dir);
                    int fileNumber = addFileNameToFileTable(resolved);
                    addFileAsIncludeReference(fileNumber);
                }

                line = eol + 1;
            }
        }
        closeFile(file);
    }
    freeStringList(files);
}

static bool isInList(const char *name, StringList *list) {
    for (StringList *s = list; s != NULL; s = s->next) {
        if (strcmp(s->string, name) == 0)
            return true;
    }
    return false;
}

void markMissingFilesAsDeleted(StringList *discoveredFiles) {
    int fileNumber = getNextExistingFileNumber(0);
    while (fileNumber != -1) {
        FileItem *item = getFileItemWithFileNumber(fileNumber);
        if (!isInList(item->name, discoveredFiles)) {
            markFileAsDeleted(fileNumber);
        }
        fileNumber = getNextExistingFileNumber(fileNumber + 1);
    }
}
