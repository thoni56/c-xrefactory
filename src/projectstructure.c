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

    /* Not found anywhere — let the caller decide. Returning a phantom
     * includer-relative path here would add a non-existent file to the
     * file table and pollute the snapshot's include graph. */
    return NULL;
}

/* Scan a single file for #include "..." lines. Returns a list of newly
   discovered resolved paths (caller must free). */
static StringList *scanFileForIncludes(const char *filePath, int includerFileNumber,
                                       StringList *includeDirs) {
    StringList *discovered = NULL;
    FILE *file = openFile((char *)filePath, "r");
    if (file == NULL)
        return NULL;
    Position includerPosition = makePosition(includerFileNumber, 1, 0);
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
                if (resolved != NULL) {
                    /* Copy the static buffer before any other call that uses
                     * normalizeFileName_static can clobber it. */
                    char resolvedPath[MAX_FILE_NAME_SIZE];
                    strcpy(resolvedPath, resolved);
                    bool alreadyKnown = existsInFileTable(resolvedPath);
                    int includedFileNumber = addFileNameToFileTable(resolvedPath);
                    addIncludeReference(includedFileNumber, includerPosition);
                    if (!alreadyKnown)
                        discovered = newStringList(strdup(resolvedPath), discovered);
                }
            }

            line = eol + 1;
        }
    }
    closeFile(file);
    return discovered;
}

static bool isInList(const char *name, StringList *list);

/* Recursively walk a directory tree, one level at a time via the fileio
 * primitive listFilesInDirectory(). Compilation units are added to
 * *discoveredCUs and the headers they include to *worklist. A directory whose
 * path is in prunePaths is neither descended into nor considered. Depth-first,
 * in listing order. */
static void walkForCompilationUnits(const char *dir, StringList *includeDirs,
                                    StringList *prunePaths,
                                    StringList **discoveredCUs, StringList **worklist) {
    StringList *entries = listFilesInDirectory(dir);
    for (StringList *e = entries; e != NULL; e = e->next) {
        if (isInList(e->string, prunePaths))
            continue;                       /* pruned: don't descend, don't consider */
        if (isDirectory(e->string)) {
            walkForCompilationUnits(e->string, includeDirs, prunePaths, discoveredCUs, worklist);
        } else if (isCompilationUnit(e->string)) {
            int cuFileNumber = addFileNameToFileTable(e->string);
            *discoveredCUs = newStringList(e->string, *discoveredCUs);
            StringList *newHeaders = scanFileForIncludes(e->string, cuFileNumber, includeDirs);
            /* Prepend discovered headers to the worklist */
            if (newHeaders != NULL) {
                StringList *tail = newHeaders;
                while (tail->next != NULL)
                    tail = tail->next;
                tail->next = *worklist;
                *worklist = newHeaders;
            }
        }
    }
    freeStringList(entries);
}

StringList *scanProjectForFilesAndIncludes(const char *projectDir, StringList *includeDirs, StringList *prunePaths) {
    StringList *worklist = NULL;
    StringList *discoveredCUs = NULL;

    /* Phase 1: recursively discover CUs and scan them for includes */
    walkForCompilationUnits(projectDir, includeDirs, prunePaths, &discoveredCUs, &worklist);

    /* Phase 2: transitively scan discovered headers */
    while (worklist != NULL) {
        StringList *current = worklist;
        worklist = worklist->next;
        current->next = NULL;

        int headerFileNumber = addFileNameToFileTable(current->string);
        StringList *newHeaders = scanFileForIncludes(current->string, headerFileNumber, includeDirs);
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
        if (isCompilationUnit(item->name) && !isInList(item->name, discoveredFiles)) {
            markFileAsDeleted(fileNumber);
        }
        fileNumber = getNextExistingFileNumber(fileNumber + 1);
    }
}
