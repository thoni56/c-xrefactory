#include "projectstructure.h"

#include <string.h>

#include "commons.h"
#include "fileio.h"
#include "filetable.h"
#include "misc.h"
#include "stringlist.h"
#include "yylex.h"

#define SCAN_BUFFER_SIZE 4096

/* Resolve an include path: try includer's directory first, then walk
   includeDirs. Returns the resolved path (pointer to static buffer). */
static char *resolveIncludePath(const char *includedFile, const char *includerPath,
                                StringList *includeDirs) {
    char *dir = directoryName_static((char *)includerPath);
    char *resolved = normalizeFileName_static((char *)includedFile, dir);
    if (fileExists(resolved))
        return resolved;

    for (StringList *d = includeDirs; d != NULL; d = d->next) {
        resolved = normalizeFileName_static((char *)includedFile, d->string);
        if (fileExists(resolved))
            return resolved;
    }

    /* Not found anywhere — return includer-relative path (will fail to open) */
    return normalizeFileName_static((char *)includedFile,
                                    directoryName_static((char *)includerPath));
}

/* Scan a single file for #include "..." lines. Returns a list of newly
   discovered resolved paths (caller must free). */
static StringList *scanFileForIncludes(const char *filePath, StringList *includeDirs) {
    StringList *discovered = NULL;
    FILE *file = openFile((char *)filePath, "r");
    if (file == NULL)
        return NULL;
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
                char *resolved = resolveIncludePath(includedFile, filePath, includeDirs);
                bool alreadyKnown = existsInFileTable(resolved);
                int fileNumber = addFileNameToFileTable(resolved);
                addFileAsIncludeReference(fileNumber);
                if (!alreadyKnown)
                    discovered = newStringList(strdup(resolved), discovered);
            }

            line = eol + 1;
        }
    }
    closeFile(file);
    return discovered;
}

StringList *scanProjectForFilesAndIncludes(const char *projectDir, StringList *includeDirs) {
    StringList *worklist = NULL;
    StringList *discoveredCUs = NULL;

    /* Phase 1: discover CUs and scan them */
    StringList *files = listFilesInDirectory(projectDir);
    for (StringList *f = files; f != NULL; f = f->next) {
        if (!isCompilationUnit(f->string))
            continue;
        addFileNameToFileTable(f->string);
        discoveredCUs = newStringList(f->string, discoveredCUs);
        StringList *newHeaders = scanFileForIncludes(f->string, includeDirs);
        /* Prepend discovered headers to worklist */
        if (newHeaders != NULL) {
            StringList *tail = newHeaders;
            while (tail->next != NULL)
                tail = tail->next;
            tail->next = worklist;
            worklist = newHeaders;
        }
    }
    freeStringList(files);

    /* Phase 2: transitively scan discovered headers */
    while (worklist != NULL) {
        StringList *current = worklist;
        worklist = worklist->next;
        current->next = NULL;

        StringList *newHeaders = scanFileForIncludes(current->string, includeDirs);
        if (newHeaders != NULL) {
            StringList *tail = newHeaders;
            while (tail->next != NULL)
                tail = tail->next;
            tail->next = worklist;
            worklist = newHeaders;
        }
        freeStringList(current);
    }
    return discoveredCUs;
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
