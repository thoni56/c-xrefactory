#include "organize_includes.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "commons.h"
#include "editor.h"
#include "misc.h"
#include "refactory.h"
#include "stackmemory.h"
#include "undo.h"


typedef struct {
    int offset;  // Offset in buffer
    int length;  // Length of #include line
    char *text;  // The actual line text
} IncludeEntry;


// Find start of line containing offset
static int findLineStart(char *text, int offset) {
    while (offset > 0 && text[offset - 1] != '\n') {
        offset--;
    }
    return offset;
}

// Find end of line containing offset (points to newline or end of buffer)
static int findLineEnd(char *text, int size, int offset) {
    while (offset < size && text[offset] != '\n') {
        offset++;
    }
    return offset;
}

// Check if line at offset is an include directive
static bool isIncludeLine(char *text, int size, int lineStart) {
    int i = lineStart;
    // Skip leading whitespace
    while (i < size && (text[i] == ' ' || text[i] == '\t'))
        i++;
    // Check for #include
    return (i < size && text[i] == '#' && i + 8 <= size && strncmp(&text[i], "#include", 8) == 0);
}

static int collectIncludes(EditorBuffer *buffer, int cursorOffset, IncludeEntry **outIncludes) {
    char *text = buffer->allocation.text;
    int size = buffer->size;
    int count = 0;
    int capacity = 10;
    IncludeEntry *includes = stackMemoryAlloc(capacity * sizeof(IncludeEntry));

    // Find the start of the line where cursor is
    int cursorLine = findLineStart(text, cursorOffset);

    // Scan backward to find start of include block
    int blockStart = cursorLine;
    while (blockStart > 0) {
        int prevLineEnd = blockStart - 1;  // Points to newline before current line
        int prevLineStart = findLineStart(text, prevLineEnd);

        // Check if previous line is include or blank
        bool isBlank = true;
        for (int i = prevLineStart; i < prevLineEnd; i++) {
            if (text[i] != ' ' && text[i] != '\t') {
                isBlank = false;
                break;
            }
        }

        if (isIncludeLine(text, size, prevLineStart)) {
            blockStart = prevLineStart;
        } else if (isBlank) {
            blockStart = prevLineStart;
        } else {
            // Hit non-include, non-blank line - stop
            break;
        }
    }

    // Scan forward to find end of include block
    int blockEnd = findLineEnd(text, size, cursorLine);
    if (blockEnd < size)
        blockEnd++;  // Include the newline

    while (blockEnd < size) {
        int nextLineStart = blockEnd;
        int nextLineEnd = findLineEnd(text, size, nextLineStart);

        // Check if next line is include or blank
        bool isBlank = true;
        for (int i = nextLineStart; i < nextLineEnd; i++) {
            if (text[i] != ' ' && text[i] != '\t') {
                isBlank = false;
                break;
            }
        }

        if (isIncludeLine(text, size, nextLineStart)) {
            blockEnd = nextLineEnd;
            if (blockEnd < size)
                blockEnd++;  // Include newline
        } else if (isBlank) {
            blockEnd = nextLineEnd;
            if (blockEnd < size)
                blockEnd++;  // Include newline
        } else {
            // Hit non-include, non-blank line - stop
            break;
        }
    }

    // Now collect all includes in the range [blockStart, blockEnd)
    int i = blockStart;
    while (i < blockEnd) {
        // Skip whitespace/blank lines
        int lineStart = i;
        int lineEnd = findLineEnd(text, size, i);

        if (isIncludeLine(text, size, lineStart)) {
            // Store include
            if (count >= capacity) {
                capacity *= 2;
                IncludeEntry *newIncludes = stackMemoryAlloc(capacity * sizeof(IncludeEntry));
                memcpy(newIncludes, includes, count * sizeof(IncludeEntry));
                includes = newIncludes;
            }

            includes[count].offset = lineStart;
            includes[count].length = lineEnd - lineStart + 1;  // Include newline
            includes[count].text = stackMemoryAlloc(includes[count].length + 1);
            strncpy(includes[count].text, &text[lineStart], includes[count].length);
            includes[count].text[includes[count].length] = '\0';
            count++;
        }

        // Move to next line
        i = lineEnd;
        if (i < size && text[i] == '\n')
            i++;
    }

    *outIncludes = includes;
    return count;
}

static char *getOwnHeaderName(char *sourceFileName) {
    // Get basename without path or suffix
    char *basename = simpleFileNameWithoutSuffix_static(sourceFileName);

    // Build .h filename
    int nameLen = strlen(basename);
    char *headerName = stackMemoryAlloc(nameLen + 3);  // +3 for ".h\0"
    strcpy(headerName, basename);
    strcpy(headerName + nameLen, ".h");

    return headerName;
}

static int compareIncludeTexts(const void *a, const void *b) {
    const IncludeEntry *ia = *(const IncludeEntry **)a;
    const IncludeEntry *ib = *(const IncludeEntry **)b;
    return strcmp(ia->text, ib->text);
}

static char *createOrganizedIncludes(IncludeEntry **groups[], int groupCounts[]) {
    char *organizedIncludes = stackMemoryAlloc(10000);  // Large buffer
    organizedIncludes[0] = '\0';

    for (int g = 0; g < 4; g++) {
        if (groupCounts[g] == 0)
            continue;

        // Add blank line between groups (except before first group)
        if (organizedIncludes[0] != '\0') {
            strcat(organizedIncludes, "\n");
        }

        for (int i = 0; i < groupCounts[g]; i++) {
            strcat(organizedIncludes, groups[g][i]->text);  // Already includes newline
        }
    }

    // Ensure trailing newline
    int len = strlen(organizedIncludes);
    if (len == 0 || organizedIncludes[len - 1] != '\n') {
        strcat(organizedIncludes, "\n");
    }
    return organizedIncludes;
}

static void sortEachGroup(IncludeEntry **groups[], int groupCounts[]) {
    for (int g = 0; g < 4; g++) {
        if (groupCounts[g] > 1) {
            qsort(groups[g], groupCounts[g], sizeof(IncludeEntry *), compareIncludeTexts);
        }
    }
}

static void growArray(IncludeEntry **groups[], int groupCounts[], int groupCapacities[], int group) {
    groupCapacities[group] *= 2;
    IncludeEntry **newGroup = stackMemoryAlloc(groupCapacities[group] * sizeof(IncludeEntry *));
    memcpy(newGroup, groups[group], groupCounts[group] * sizeof(IncludeEntry *));
    groups[group] = newGroup;
}

static void applyReplacement(EditorBuffer *buffer, IncludeEntry *includes, int count, char *newText) {
    int startOffset = includes[0].offset;
    int endOffset = includes[count - 1].offset + includes[count - 1].length;
    int deleteSize = endOffset - startOffset;

    replaceStringInEditorBuffer(buffer, startOffset, deleteSize, newText, strlen(newText), &editorUndo);
}

static int classifyInclude(char *ownHeaderName, char *includeStart) {
    int group;
    includeStart += 8;  // Skip "#include"
    while (*includeStart && isspace(*includeStart))
        includeStart++;

    if (*includeStart == '<') {
        // System header
        group = 1;
    } else if (*includeStart == '"') {
        // Extract filename
        char *nameStart = includeStart + 1;
        char *nameEnd = strchr(nameStart, '"');
        if (nameEnd) {
            int nameLen = nameEnd - nameStart;
            char *includedFile = stackMemoryAlloc(nameLen + 1);
            strncpy(includedFile, nameStart, nameLen);
            includedFile[nameLen] = '\0';

            // Check if it's own header
            if (ownHeaderName && strcmp(includedFile, ownHeaderName) == 0) {
                group = 0;  // Own header
            } else if (strstr(includedFile, ".h")) {
                group = 2;  // Project .h header
            } else {
                group = 3;  // Other
            }
        } else {
            group = 2;  // Default to project header
        }
    } else {
        group = 2;  // Default to project header
    }

    return group;
}

static void addIncludeToCorrectGroup(IncludeEntry *include, char *ownHeaderName, IncludeEntry **groups[],
                                     int groupCounts[], int groupCapacities[]) {
    char *includeStart = strstr(include->text, "#include");

    if (includeStart) {
        int group = classifyInclude(ownHeaderName, includeStart);

        if (groupCounts[group] >= groupCapacities[group]) {
            growArray(groups, groupCounts, groupCapacities, group);
        }

        groups[group][groupCounts[group]++] = include;
    }
}

void organizeIncludes(EditorMarker *point) {
    assert(point);

    EditorBuffer *buffer = point->buffer;

    IncludeEntry *includes;
    int noOfIncludes = collectIncludes(buffer, point->offset, &includes);

    if (noOfIncludes == 0) {
        errorMessage(ERR_ST, "No includes found");
        return;
    }

    char *ownHeaderName = getOwnHeaderName(buffer->fileName);

    // Classify includes into 4 groups: own header, system headers, other headers, other includes
    IncludeEntry **groups[4];  // Array of pointers for each group
    int groupCounts[4] = {0, 0, 0, 0};
    int groupCapacities[4];

    // Allocate initial capacity for each group
    for (int g = 0; g < 4; g++) {
        groupCapacities[g] = 10;
        groups[g] = stackMemoryAlloc(groupCapacities[g] * sizeof(IncludeEntry *));
    }

    for (int i = 0; i < noOfIncludes; i++) {
        addIncludeToCorrectGroup(&includes[i], ownHeaderName, groups, groupCounts, groupCapacities);
    }

    sortEachGroup(groups, groupCounts);

    char *newText = createOrganizedIncludes(groups, groupCounts);

    applyReplacement(buffer, includes, noOfIncludes, newText);

    applyWholeRefactoringFromUndo();
}
