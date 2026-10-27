#include "move_function.h"

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commons.h"
#include "cxref.h"
#include "editor.h"
#include "editormarker.h"
#include "editorbuffer.h"
#include "filetable.h"
#include "fileio.h"
#include "globals.h"
#include "misc.h"
#include "options.h"
#include "parsing.h"
#include "position.h"
#include "ppc.h"
#include "proto.h"
#include "protocol.h"
#include "refactory.h"
#include "reference.h"
#include "referenceableitemtable.h"
#include "stackmemory.h"
#include "undo.h"
#include "usage.h"


static void moveMarkerToTheEndOfDefinitionScope(EditorMarker *marker) {
    int offset;
    offset = marker->offset;
    moveEditorMarkerToNonBlankOrNewline(marker, 1);
    if (marker->offset >= marker->buffer->allocation.bufferSize) {
        return;
    }
    if (CHAR_ON_MARKER(marker) == '/' && CHAR_AFTER_MARKER(marker) == '/') {
        if (options.commentMovingMode == CM_NO_COMMENT)
            return;
        moveEditorMarkerToNewline(marker, 1);
        marker->offset++;
    } else if (CHAR_ON_MARKER(marker) == '/' && CHAR_AFTER_MARKER(marker) == '*') {
        if (options.commentMovingMode == CM_NO_COMMENT)
            return;
        marker->offset++;
        marker->offset++;
        while (marker->offset < marker->buffer->allocation.bufferSize &&
               (CHAR_ON_MARKER(marker) != '*' || CHAR_AFTER_MARKER(marker) != '/')) {
            marker->offset++;
        }
        if (marker->offset < marker->buffer->allocation.bufferSize) {
            marker->offset++;
            marker->offset++;
        }
        offset = marker->offset;
        moveEditorMarkerToNonBlankOrNewline(marker, 1);
        if (CHAR_ON_MARKER(marker) == '\n')
            marker->offset++;
        else
            marker->offset = offset;
    } else if (CHAR_ON_MARKER(marker) == '\n') {
        marker->offset++;
    } else {
        if (options.commentMovingMode == CM_NO_COMMENT)
            return;
        marker->offset = offset;
    }
}

static MarkerLocationKind markerWRTComment(EditorMarker *marker, int *commentBeginOffset) {
    char *begin, *end, *mms;
    assert(marker->buffer && marker->buffer->allocation.text);
    char *text = marker->buffer->allocation.text;
    end = text + marker->buffer->allocation.bufferSize;
    mms = text + marker->offset;
    while (text < end && text < mms) {
        begin = text;
        if (*text == '/' && (text + 1) < end && *(text + 1) == '*') {
            // /**/ comment
            text += 2;
            while ((text + 1) < end && !(*text == '*' && *(text + 1) == '/'))
                text++;
            if (text + 1 < end)
                text += 2;
            if (text > mms) {
                *commentBeginOffset = begin - marker->buffer->allocation.text;
                return MARKER_IS_IN_STAR_COMMENT;
            }
        } else if (*text == '/' && text + 1 < end && *(text + 1) == '/') {
            // // comment
            text += 2;
            while (text < end && *text != '\n')
                text++;
            if (text < end)
                text += 1;
            if (text > mms) {
                *commentBeginOffset = begin - marker->buffer->allocation.text;
                return MARKER_IS_IN_SLASH_COMMENT;
            }
        } else if (*text == '"') {
            // string, pass it removing all inside (also /**/ comments)
            text++;
            while (text < end && *text != '"') {
                text++;
                if (*text == '\\') {
                    text++;
                    text++;
                }
            }
            if (text < end)
                text++;
        } else {
            text++;
        }
    }
    return MARKER_IS_IN_CODE;
}

static void moveMarkerToTheBeginOfDefinitionScope(EditorMarker *marker) {
    int offsetToBeginning;

    int slashedCommentsProcessed = 0;
    int staredCommentsProcessed = 0;
    for (;;) {
        offsetToBeginning = marker->offset;
        marker->offset--;
        moveEditorMarkerToNonBlankOrNewline(marker, -1);
        if (CHAR_ON_MARKER(marker) == '\n') {
            offsetToBeginning = marker->offset + 1;
            marker->offset--;
        }
        if (options.commentMovingMode == CM_NO_COMMENT)
            break;
        moveEditorMarkerToNonBlank(marker, -1);
        int comBeginOffset;
        MarkerLocationKind markerLocationKind = markerWRTComment(marker, &comBeginOffset);
        if (markerLocationKind == MARKER_IS_IN_CODE)
            break;
        else if (markerLocationKind == MARKER_IS_IN_STAR_COMMENT) {
            if (options.commentMovingMode == CM_SINGLE_SLASHED)
                break;
            if (options.commentMovingMode == CM_ALL_SLASHED)
                break;
            if (staredCommentsProcessed > 0 && options.commentMovingMode == CM_SINGLE_STARRED)
                break;
            if (staredCommentsProcessed > 0 &&
                options.commentMovingMode == CM_SINGLE_SLASHED_AND_STARRED)
                break;
            staredCommentsProcessed++;
            marker->offset = comBeginOffset;
        }
        // slash comment, skip them all
        else if (markerLocationKind == MARKER_IS_IN_SLASH_COMMENT) {
            if (options.commentMovingMode == CM_SINGLE_STARRED)
                break;
            if (options.commentMovingMode == CM_ALL_STARRED)
                break;
            if (slashedCommentsProcessed > 0 && options.commentMovingMode == CM_SINGLE_SLASHED)
                break;
            if (slashedCommentsProcessed > 0 &&
                options.commentMovingMode == CM_SINGLE_SLASHED_AND_STARRED)
                break;
            slashedCommentsProcessed++;
            marker->offset = comBeginOffset;
        } else {
            warningMessage(ERR_INTERNAL, "A new comment?");
            break;
        }
    }
    marker->offset = offsetToBeginning;
}

static EditorMarker *getTargetFromOptions(void) {
    EditorMarker *target;
    EditorBuffer *targetBuffer;
    int           targetLine;
    char         *targetFileName = strdup(normalizeFileName_static(refactoringOptions.moveTargetFile, cwd));

    targetBuffer = findOrCreateAndLoadEditorBufferForFile(targetFileName);

    if (targetBuffer == NULL)
        FATAL_ERROR(ERR_ST, "Could not find a buffer for target position", EXIT_FAILURE);
    target = newEditorMarker(targetBuffer, 0);
    sscanf(refactoringOptions.refactorTargetLine, "%d", &targetLine);
    moveEditorMarkerToLineAndColumn(target, targetLine, 0);
    return target;
}

static char *extractFunctionSignature(EditorMarker *startMarker, EditorMarker *endMarker) {

    int searchOffset = startMarker->offset;
    int braceOffset = -1;
    char *text = startMarker->buffer->allocation.text;

    while (searchOffset < endMarker->offset) {
        if (text[searchOffset] == '{') {
            braceOffset = searchOffset;
            break;
        }
        searchOffset++;
    }

    char *functionSignature = NULL;
    if (braceOffset != -1) {
        /* Extract signature (from start to just before '{') */
        int signatureLength = braceOffset - startMarker->offset;
        functionSignature = stackMemoryAlloc(signatureLength + 1);
        strncpy(functionSignature, &text[startMarker->offset], signatureLength);
        functionSignature[signatureLength] = '\0';

        /* Trim trailing whitespace from signature */
        int i = signatureLength - 1;
        while (i >= 0
               && (functionSignature[i] == ' ' || functionSignature[i] == '\t'
                   || functionSignature[i] == '\n' || functionSignature[i] == '\r')) {
            functionSignature[i] = '\0';
            i--;
        }
    }

    return functionSignature;
}

static char *findCorrespondingHeaderFile(EditorMarker *target) {
    char *targetFileName = target->buffer->fileName;
    char *suffix = getFileSuffix(targetFileName);
    char *headerFileName = NULL;

    if (strcmp(suffix, ".c") == 0) {
        /* Build header filename by replacing .c with .h */
        char headerPath[MAX_FILE_NAME_SIZE];
        int baseLength = suffix - targetFileName; /* Length up to the '.' */

        strncpy(headerPath, targetFileName, baseLength);
        headerPath[baseLength] = '\0';
        strcat(headerPath, ".h");

        if (fileExists(headerPath)) {
            headerFileName = stackMemoryAlloc(strlen(headerPath) + 1);
            strcpy(headerFileName, headerPath);
        }
    }

    return headerFileName;
}

static int findHeaderInsertionPoint(EditorBuffer *headerBuffer) {
    char *text = headerBuffer->allocation.text;
    int size = getSizeOfEditorBuffer(headerBuffer);

    /* Search backwards for "\n#endif" pattern */
    for (int i = size - 1; i >= 1; i--) {
        if (text[i-1] == '\n' && text[i] == '#') {
            /* Found newline followed by # - check if it's #endif */
            if (i + 5 < size && strncmp(&text[i], "#endif", 6) == 0) {
                int offset = i - 1;  /* Start at the '\n' before '#' */
                while (offset > 0 && isspace(text[offset-1])) {
                    offset--;
                }
                return offset;
            }
        }
    }

    /* Edge case: file starts with #endif (no preceding newline), just insert it first */
    if (size >= 6 && strncmp(text, "#endif", 6) == 0) {
        return 0;
    }

    /* No #endif found - insert at end */
    return size;
}

static void insertExternDeclaration(char *functionSignature, EditorBuffer *headerBuffer, int offset) {
    /* Build extern declaration: "extern <signature>;\n" */
    char *externDecl =
        stackMemoryAlloc(strlen("\nextern ") + strlen(functionSignature) + strlen(";") + 1);
    strcpy(externDecl, "\nextern ");
    strcat(externDecl, functionSignature);
    strcat(externDecl, ";");

    /* Insert at the beginning of the header file */
    replaceStringInEditorBuffer(headerBuffer, offset, 0, externDecl, strlen(externDecl), &editorUndo);
}

static bool lookingAtStatic(char *text, int remaining) {
    return remaining >= strlen("static ") && strncmp(text, "static", 6) == 0
        && (text[6] == ' ' || text[6] == '\t');
}

static int removeStaticKeywordIfPresent(EditorMarker *startMarker, EditorMarker *point, int size) {
    EditorMarker *searchMarker = newEditorMarker(startMarker->buffer, startMarker->offset);
    EditorMarker *staticMarker = NULL;
    bool foundStatic = false;

    while (searchMarker->offset < point->offset) {
        char *text = &startMarker->buffer->allocation.text[searchMarker->offset];
        int remaining = point->offset - searchMarker->offset;

        /* Check if we're at "static " (with space or tab after) */
        if (lookingAtStatic(text, remaining)) {
            foundStatic = true;
            staticMarker = newEditorMarker(startMarker->buffer, searchMarker->offset);
            break;
        }
        searchMarker->offset++;
    }
    freeEditorMarker(searchMarker);

    if (foundStatic) {
        replaceStringInEditorBuffer(staticMarker->buffer, staticMarker->offset, strlen("static "),
                                    "", 0, &editorUndo);
        freeEditorMarker(staticMarker);
        /* Adjust size since we removed "static " */
        size -= strlen("static ");
    }

    return size;
}

static bool sourceAlreadyIncludesHeader(EditorBuffer *sourceBuffer, char *headerFileName) {
    int sourceFileNumber = sourceBuffer->fileNumber;
    int headerFileNumber = getFileNumberFromName(headerFileName);

    if (headerFileNumber == NO_FILE_NUMBER) {
        return false;  /* Header file not in file table yet */
    }

    /* Create search item for the header file */
    ReferenceableItem searchItem = makeReferenceableItem(LINK_NAME_INCLUDE_REFS, TypeCppInclude,
                                                         StorageExtern, VisibilityGlobal,
                                                         headerFileNumber);

    /* Look it up in the reference table */
    ReferenceableItem *foundItem;
    if (isMemberInReferenceableItemTable(&searchItem, NULL, &foundItem)) {
        /* Check if source file appears in the references */
        for (Reference *ref = foundItem->references; ref != NULL; ref = ref->next) {
            if (ref->position.file == sourceFileNumber && ref->usage == UsageUsed) {
                return true;  /* source file already includes header! */
            }
        }
    }

    return false;  /* Include not found */
}

static int findIncludeInsertionPoint(EditorBuffer *buffer) {
    char *text = buffer->allocation.text;
    int size = getSizeOfEditorBuffer(buffer);
    int lastIncludeEnd = 0;  /* Offset after last include, or 0 if none found */

    /* Scan forward looking for #include directives */
    for (int i = 0; i < size - 8; i++) {  /* 8 = strlen("#include") */
        /* Look for newline followed by #include (or start of file) */
        if ((i == 0 || text[i-1] == '\n') && text[i] == '#') {
            if (strncmp(&text[i], "#include", 8) == 0) {
                /* Found an include - find the end of this line */
                int j = i + 8;
                while (j < size && text[j] != '\n') {
                    j++;
                }
                if (j < size) {
                    lastIncludeEnd = j + 1;  /* Offset after the newline */
                }
            }
        }
    }

    return lastIncludeEnd;  /* 0 if no includes found = insert at start */
}


static void moveStaticFunctionAndMakeItExtern(EditorMarker *startMarker, EditorMarker *point,
                                              EditorMarker *endMarker, EditorMarker *target) {
    int functionBlockSize = endMarker->offset - startMarker->offset;
    EditorBuffer *sourceBuffer = startMarker->buffer;
    if (target->buffer == startMarker->buffer && target->offset > startMarker->offset &&
        target->offset < startMarker->offset + functionBlockSize) {
        errorMessage(ERR_ST, "You can't move something into itself.");
        return;
    }

    /* Check if function has "static" keyword and remove it when moving between files.
     * When moving within the same file, keep static (visibility doesn't change). */
    bool movingBetweenFiles = (startMarker->buffer != target->buffer);

    if (movingBetweenFiles) {
        functionBlockSize = removeStaticKeywordIfPresent(startMarker, point, functionBlockSize);
    }

    /* Extract function signature for extern declaration (before moving) */
    char *functionSignature = NULL;
    if (movingBetweenFiles) {
        /* Find the opening brace to determine where signature ends */
        functionSignature = extractFunctionSignature(startMarker, endMarker);
    }

    /* Now move the (possibly modified) function block */
    moveBlockInEditorBuffer(startMarker, target, functionBlockSize, &editorUndo);

    /* After moving the function, check if we need to add an extern declaration to the header */
    char *headerFileName = NULL;
    if (movingBetweenFiles) {
        headerFileName = findCorrespondingHeaderFile(target);
    }

    /* Insert extern declaration into header file if we have both header and signature */
    if (headerFileName != NULL && functionSignature != NULL) {
        EditorBuffer *headerBuffer = findOrCreateAndLoadEditorBufferForFile(headerFileName);
        int insertionOffset = findHeaderInsertionPoint(headerBuffer);
        insertExternDeclaration(functionSignature, headerBuffer, insertionOffset);

        /* Check if source file needs to include the header */
        if (!sourceAlreadyIncludesHeader(sourceBuffer, headerFileName)) {
            /* Build the include directive */
            char *baseHeaderName = simpleFileName(headerFileName);
            char *includeDirective = stackMemoryAlloc(strlen("#include \"") + strlen(baseHeaderName) + strlen("\"\n") + 1);
            strcpy(includeDirective, "#include \"");
            strcat(includeDirective, baseHeaderName);
            strcat(includeDirective, "\"\n");

            /* Insert at beginning of source file */
            int insertionOffset = findIncludeInsertionPoint(sourceBuffer);
            replaceStringInEditorBuffer(sourceBuffer, insertionOffset, 0, includeDirective, strlen(includeDirective), &editorUndo);
        }
    }
}

#define MAX_STATICS 10

typedef struct {
    ReferenceableItem       *item;
    Position                 definition;
    FunctionBoundariesResult bounds; /* Of a helper */
} StaticUsed;

typedef struct {
    FunctionBoundariesResult moved;
    ReferenceableItem       *movedItem;
    StaticUsed               leftBehind[MAX_STATICS];
    int                      leftBehindCount;
    StaticUsed               helpers[MAX_STATICS]; /* Used only by what moves */
    int                      helperCount;
    StaticUsed               candidates[MAX_STATICS];
    int                      candidateCount;
    ReferenceableItem       *cannotMove[MAX_STATICS]; /* Variables declared with others */
    int                      cannotMoveCount;
    StaticUsed               declaredWithOthers[MAX_STATICS];
    int                      declaredWithOthersCount;
} UsesOfMovedFunction;

static void addStaticUsed(StaticUsed used, StaticUsed list[], int *count) {
    if (*count < MAX_STATICS)
        list[(*count)++] = used;
}

static bool isInside(Position position, FunctionBoundariesResult bounds) {
    return positionIsBetween(position, bounds.functionBegin, bounds.functionEnd);
}

static bool isMoving(UsesOfMovedFunction *uses, Position position) {
    if (isInside(position, uses->moved))
        return true;
    for (int i = 0; i < uses->helperCount; i++)
        if (isInside(position, uses->helpers[i].bounds))
            return true;
    return false;
}

static bool cannotMove(UsesOfMovedFunction *uses, ReferenceableItem *item) {
    for (int i = 0; i < uses->cannotMoveCount; i++)
        if (uses->cannotMove[i] == item)
            return true;
    return false;
}

static void findStaticsUsed(ReferenceableItem *item, void *pointer) {
    UsesOfMovedFunction *uses = pointer;
    bool usedInMoving = false;
    bool usedOutsideMoving = false;
    bool definedInMoved = false;
    bool definedInMoving = false;
    bool definedInSourceFile = false;
    Position definition = NO_POSITION;

    for (Reference *r = item->references; r != NULL; r = r->next) {
        bool moving = isMoving(uses, r->position);
        if (isDefinitionUsage(r->usage)) {
            definedInMoved |= isInside(r->position, uses->moved);
            definedInMoving |= moving;
            if (r->position.file == uses->moved.functionBegin.file) {
                definedInSourceFile = true;
                definition = r->position;
            }
        } else if (moving) {
            usedInMoving = true;
        } else {
            usedOutsideMoving = true;
        }
    }
    if (definedInMoved && item->visibility == VisibilityGlobal)
        uses->movedItem = item;
    else if (definedInMoving)
        return;                 /* Already a helper */
    else if (usedInMoving && definedInSourceFile && item->storage == StorageStatic) {
        StaticUsed used = {.item = item, .definition = definition};
        if (usedOutsideMoving)
            addStaticUsed(used, uses->leftBehind, &uses->leftBehindCount);
        else if (cannotMove(uses, item))
            addStaticUsed(used, uses->declaredWithOthers, &uses->declaredWithOthersCount);
        else
            addStaticUsed(used, uses->candidates, &uses->candidateCount);
    }
}

static int compareDefinitions(const void *a, const void *b) {
    Position first = ((const StaticUsed *)a)->definition;
    Position second = ((const StaticUsed *)b)->definition;
    if (positionIsLessThan(first, second))
        return -1;
    return positionIsLessThan(second, first);
}

static int composeUses(char *moved, StaticUsed statics[], int count, char message[]) {
    char name[TMP_STRING_SIZE];
    int length = sprintf(message, "'%s' uses ", moved);
    for (int i = 0; i < count; i++) {
        char *separator = i == 0 ? "" : i == count - 1 ? " and " : ", ";
        prettyPrintLinkName(name, statics[i].item->linkName, TMP_STRING_SIZE);
        length += sprintf(message + length, "%s'%s'", separator, name);
    }
    return length;
}

static void composeLeftBehindMessage(UsesOfMovedFunction *uses, char message[]) {
    char moved[TMP_STRING_SIZE];
    char name[TMP_STRING_SIZE];
    prettyPrintLinkName(moved, uses->movedItem->linkName, TMP_STRING_SIZE);
    int length = 0;
    if (uses->leftBehindCount > 0) {
        length += composeUses(moved, uses->leftBehind, uses->leftBehindCount, message + length);
        prettyPrintLinkName(name, uses->leftBehind[0].item->linkName, TMP_STRING_SIZE);
        if (uses->leftBehindCount == 1)
            length += sprintf(message + length, ", which is static in this file. Move '%s' first.", name);
        else
            length += sprintf(message + length, ", which are static in this file. Move them first.");
    }
    if (uses->declaredWithOthersCount > 0) {
        if (length > 0)
            length += sprintf(message + length, " ");
        length += composeUses(moved, uses->declaredWithOthers, uses->declaredWithOthersCount, message + length);
        prettyPrintLinkName(name, uses->declaredWithOthers[0].item->linkName, TMP_STRING_SIZE);
        if (uses->declaredWithOthersCount == 1)
            sprintf(message + length, ", which is declared together with other names. Declare '%s' on its own first.", name);
        else
            sprintf(message + length, ", which are declared together with other names. Declare them on their own first.");
    }
}

/* A variable's definition is its line up to the ';', if it declares nothing else */
static FunctionBoundariesResult getVariableBoundaries(EditorMarker *definition) {
    FunctionBoundariesResult bounds = {.found = false};
    char *text = definition->buffer->allocation.text;
    int lineStart = definition->offset;
    while (lineStart > 0 && text[lineStart-1] != '\n')
        lineStart--;
    EditorMarker *end = newEditorMarker(definition->buffer, lineStart);
    int depth = 0;
    while (end->offset < end->buffer->allocation.bufferSize
           && (CHAR_ON_MARKER(end) != ';' || depth > 0)) {
        char c = CHAR_ON_MARKER(end);
        if (c == '(' || c == '[' || c == '{')
            depth++;
        else if (c == ')' || c == ']' || c == '}')
            depth--;
        else if (c == ',' && depth == 0)
            break;
        end->offset++;
    }
    if (end->offset < end->buffer->allocation.bufferSize && CHAR_ON_MARKER(end) == ';') {
        end->offset++;
        bounds.found = true;
        bounds.functionBegin = makePositionFromEditorMarker(definition);
        bounds.functionBegin.col = 0;
        bounds.functionEnd = makePositionFromEditorMarker(end);
    }
    freeEditorMarker(end);
    return bounds;
}

/* A candidate that is neither a function nor a variable has no boundaries, so it stays */
static void addCandidatesAsHelpers(UsesOfMovedFunction *uses) {
    for (int i = 0; i < uses->candidateCount; i++) {
        StaticUsed candidate = uses->candidates[i];
        EditorMarker *definition = newEditorMarkerForPosition(candidate.definition);
        candidate.bounds = getFunctionBoundaries(definition);
        if (!candidate.bounds.found)
            candidate.bounds = getVariableBoundaries(definition);
        freeEditorMarker(definition);
        if (candidate.bounds.found && uses->helperCount < MAX_STATICS)
            addStaticUsed(candidate, uses->helpers, &uses->helperCount);
        else if (uses->cannotMoveCount < MAX_STATICS)
            uses->cannotMove[uses->cannotMoveCount++] = candidate.item;
    }
}

static int decidedCount(UsesOfMovedFunction *uses) {
    return uses->helperCount + uses->cannotMoveCount;
}

/* Helpers can have helpers, so look again until nothing new is found */
static UsesOfMovedFunction findUsesOfMovedFunction(FunctionBoundariesResult bounds) {
    UsesOfMovedFunction uses = {.moved = bounds};
    int decided;
    do {
        uses.leftBehindCount = 0;
        uses.declaredWithOthersCount = 0;
        uses.candidateCount = 0;
        mapOverReferenceableItemTableWithPointer(findStaticsUsed, &uses);
        decided = decidedCount(&uses);
        addCandidatesAsHelpers(&uses);
    } while (uses.candidateCount > 0 && decidedCount(&uses) > decided);
    /* Out of room, so the candidates left stay behind */
    for (int i = 0; i < uses.candidateCount && decidedCount(&uses) == decided; i++)
        addStaticUsed(uses.candidates[i], uses.leftBehind, &uses.leftBehindCount);
    qsort(uses.helpers, uses.helperCount, sizeof(StaticUsed), compareDefinitions);
    return uses;
}

/* A static in the source file that the function uses would be left behind */
static bool refuseToLeaveStaticBehind(UsesOfMovedFunction *uses) {
    if (uses->movedItem == NULL || uses->leftBehindCount + uses->declaredWithOthersCount == 0)
        return false;
    qsort(uses->leftBehind, uses->leftBehindCount, sizeof(StaticUsed), compareDefinitions);
    qsort(uses->declaredWithOthers, uses->declaredWithOthersCount, sizeof(StaticUsed), compareDefinitions);

    char message[(2*MAX_STATICS+4)*TMP_STRING_SIZE];
    composeLeftBehindMessage(uses, message);
    errorMessage(ERR_ST, message);
    return true;
}

static EditorMarker *newDefinitionScopeStart(FunctionBoundariesResult bounds) {
    EditorMarker *marker = newEditorMarkerForPosition(bounds.functionBegin);
    moveMarkerToTheBeginOfDefinitionScope(marker);
    return marker;
}

static EditorMarker *newDefinitionScopeEnd(FunctionBoundariesResult bounds) {
    EditorMarker *marker = newEditorMarkerForPosition(bounds.functionEnd);
    moveMarkerToTheEndOfDefinitionScope(marker);
    return marker;
}

/* The helpers keep 'static' and go before the moved function, in their order */
static bool moveHelpersAlongOrRefuse(FunctionBoundariesResult bounds, EditorMarker *target) {
    UsesOfMovedFunction uses = findUsesOfMovedFunction(bounds);
    if (refuseToLeaveStaticBehind(&uses))
        return false;

    EditorMarker *starts[MAX_STATICS];
    EditorMarker *ends[MAX_STATICS];
    for (int i = 0; i < uses.helperCount; i++) {
        starts[i] = newDefinitionScopeStart(uses.helpers[i].bounds);
        ends[i] = newDefinitionScopeEnd(uses.helpers[i].bounds);
    }
    for (int i = 0; i < uses.helperCount; i++) {
        moveBlockInEditorBuffer(starts[i], target, ends[i]->offset - starts[i]->offset, &editorUndo);
        freeEditorMarker(starts[i]);
        freeEditorMarker(ends[i]);
    }
    return true;
}

void moveFunction(EditorMarker *point) {
    EditorMarker *target = getTargetFromOptions();

    if (!isValidMoveTarget(target)) {
        errorMessage(ERR_ST, "Invalid target place");
        return;
    }

    FunctionBoundariesResult bounds = getFunctionBoundaries(point);

    if (!bounds.found) {
        FATAL_ERROR(ERR_INTERNAL, "Can't find declaration coordinates", EXIT_FAILURE);
    }

    /* Markers before any change, since the positions are from the parse */
    EditorMarker *functionStart = newDefinitionScopeStart(bounds);
    EditorMarker *functionEnd = newDefinitionScopeEnd(bounds);

    int lines = countLinesBetweenEditorMarkers(functionStart, functionEnd);

    if (target->buffer != point->buffer && !moveHelpersAlongOrRefuse(bounds, target))
        return;

    moveStaticFunctionAndMakeItExtern(functionStart, point, functionEnd, target);

    // and generate output
    applyWholeRefactoringFromUndo();
    ppcGotoMarker(point);
    ppcValueRecord(PPC_INDENT, lines, "");
}
