#include "refactory.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "argumentsvector.h"
#include "commons.h"
#include "cxref.h"
#include "editor.h"
#include "editorbuffer.h"
#include "editormarker.h"
#include "filetable.h"
#include "globals.h"
#include "head.h"
#include "list.h"
#include "log.h"
#include "misc.h"
#include "move_function.h"
#include "options.h"
#include "organize_includes.h"
#include "parsing.h"
#include "position.h"
#include "ppc.h"
#include "progress.h"
#include "proto.h"
#include "protocol.h"
#include "refactorings.h"
#include "server.h"
#include "session.h"
#include "startup.h"
#include "undo.h"
#include "xref.h"


#define RRF_CHARS_TO_PRE_CHECK_AROUND 1
#define MAX_NARGV_OPTIONS_COUNT 50


static EditorUndo *refactoringStartingPoint;

static char *serverDefaultOptions[] = {
    "xref",
    "-xrefactory-II",
    //& "-debug",
    "-server",
    NULL,
};


Options refactoringOptions;


static int argument_count(char **argv) {
    int count;
    for (count = 0; *argv != NULL; count++, argv++)
        ;
    return count;
}

static void setArguments(char *argv[MAX_NARGV_OPTIONS_COUNT], char *project,
                         EditorMarker *point, EditorMarker *mark) {
    static char optPoint[TMP_STRING_SIZE];
    static char optMark[TMP_STRING_SIZE];
    static char optXrefrc[MAX_FILE_NAME_SIZE];
    int         i = 0;

    argv[i] = "null";
    i++;
    if (refactoringOptions.xrefrc != NULL) {
        sprintf(optXrefrc, "-xrefrc=%s", refactoringOptions.xrefrc);
        assert(strlen(optXrefrc) + 1 < MAX_FILE_NAME_SIZE);
        argv[i] = optXrefrc;
        i++;
    }
    if (refactoringOptions.eolConversion & CR_LF_EOL_CONVERSION) {
        argv[i] = "-crlfconversion";
        i++;
    }
    if (refactoringOptions.eolConversion & CR_EOL_CONVERSION) {
        argv[i] = "-crconversion";
        i++;
    }
    if (project != NULL) {
        argv[i] = "-p";
        i++;
        argv[i] = project;
        i++;
    }
    assert(i < MAX_NARGV_OPTIONS_COUNT);
    if (point != NULL) {
        sprintf(optPoint, "-olcursor=%d", point->offset);
        argv[i] = optPoint;
        i++;
    }
    assert(i < MAX_NARGV_OPTIONS_COUNT);
    if (mark != NULL) {
        sprintf(optMark, "-olmark=%d", mark->offset);
        argv[i] = optMark;
        i++;
    }
    assert(i < MAX_NARGV_OPTIONS_COUNT);

    if (point) {
        argv[i] = point->buffer->fileName;
        i++;
    }
    assert(i < MAX_NARGV_OPTIONS_COUNT);

    // finally mark end of options
    argv[i] = NULL;
    i++;
    assert(i < MAX_NARGV_OPTIONS_COUNT);
}

void parseBufferUsingServer(char *project, EditorMarker *point, EditorMarker *mark,
                            char *pushOption, char *pushOption2) {
    int   argumentCount;
    char *argumentVector[MAX_NARGV_OPTIONS_COUNT];

    currentPass = ANY_PASS;

    assert(options.mode == ServerMode);

    /* Clear accumulated input files from previous parses to avoid global state pollution.
     * This is crucial when parseBufferUsingServer() is called multiple times (e.g.,
     * target validation followed by function boundaries check). Without this, getNextScheduledFile()
     * would return the wrong file because it searches from index 0. */
    options.inputFiles = NULL;

    setArguments(argumentVector, project, point, mark);
    argumentCount = argument_count(argumentVector);
    if (pushOption != NULL) {
        argumentVector[argumentCount++] = pushOption;
    }
    if (pushOption2 != NULL) {
        argumentVector[argumentCount++] = pushOption2;
    }
    ArgumentsVector args = {.argc = argument_count(serverDefaultOptions), .argv = serverDefaultOptions};
    ArgumentsVector nargs = {.argc = argumentCount, .argv = argumentVector};
    initServer(nargs);

    /* Bridge: Sync parsingConfig with options for old code path */
    syncParsingConfigFromOptions(options);
    callServer(args, nargs);
}

/* Interactive command loop for refactoring operations that need user input.
 *
 * Called from refactoring operations (like rename) when user interaction is needed,
 * typically via displayResolutionDialog() to handle name collisions or symbol selection.
 *
 * OPERATION:
 * - Reads commands from stdin via readOptionsFromPipe() (sent by editor/test driver)
 * - Processes interactive commands like -olcxmenufilter, -olcxfilter
 * - Continues looping until:
 *   1. -continuerefactoring is received (sets continueRefactoring = RC_CONTINUE)
 *   2. Empty input (pipedOptions.argc <= 1)
 *
 * IMPORTANT: After this function returns, control goes back to refactory() which
 * then exits the process. The editor should NOT send <exit> after -continuerefactoring.
 */
static void beInteractive(void) {
    ENTER();
    // Use a local copy so we don't clobber the server loop's global savedOptions
    static Options localSavedOptions;
    deepCopyOptionsFromTo(&options, &localSavedOptions);
    for (;;) {
        closeOutputFile();
        ppcSynchronize();
        deepCopyOptionsFromTo(&localSavedOptions, &options);

        ArgumentsVector args = {.argc = argument_count(serverDefaultOptions), .argv = serverDefaultOptions};
        processOptions(args, PROCESS_FILE_ARGUMENTS_NO);

        ArgumentsVector pipedOptions = readOptionsFromPipe();
        openOutputFile(refactoringOptions.outputFileName);
        if (pipedOptions.argc <= 1)
            break;

        initServer(pipedOptions);
        if (options.continueRefactoring != RC_NONE)
            break;

        callServer(args, pipedOptions);
        answerEditorAction();
    }
    LEAVE();
}

// -------------------- end of interface to edit server sub-task ----------------------
////////////////////////////////////////////////////////////////////////////////////////

static void displayResolutionDialog(char *message, int messageType) {
    char buf[TMP_BUFF_SIZE];
    strcpy(buf, message);
    formatOutputLine(buf, ERROR_MESSAGE_STARTING_OFFSET);
    ppcDisplaySelection(buf, messageType);
    beInteractive();
}

#define STANDARD_SELECT_SYMBOLS_MESSAGE                                                                           \
    "Select classes in left window. These classes will be processed during refactoring. It is highly "            \
    "recommended to process whole hierarchy of related classes all at once. Unselection of any class and its "    \
    "exclusion from refactoring may cause changes in your program behaviour."
#define STANDARD_C_SELECT_SYMBOLS_MESSAGE                                                                         \
    "There are several symbols referred from this place. Continuing this refactoring will process the selected "  \
    "symbols all at once."

static void pushReferences(EditorMarker *point, char *pushOption, char *resolveMessage,
                           int messageType) {
    /* now remake task initialisation as for edit server */
    parseBufferUsingServer(refactoringOptions.project, point, NULL, pushOption, NULL);

    assert(sessionData.browsingStack.top != NULL);
    if (sessionData.browsingStack.top->hkSelectedSym == NULL) {
        errorMessage(ERR_INTERNAL, "no symbol found for refactoring push");
    }
    createSelectionMenuForOperation(sessionData.browsingStack.top->operation);
    if (resolveMessage != NULL && olcxShowSelectionMenu()) {
        displayResolutionDialog(resolveMessage, messageType);
    }
}

static void safetyCheck(char *project, EditorMarker *point) {
    parseBufferUsingServer(project, point, NULL, "-olcxsafetycheck", NULL);

    assert(sessionData.browsingStack.top != NULL);
    if (sessionData.browsingStack.top->hkSelectedSym == NULL) {
        errorMessage(ERR_ST, "No symbol found for refactoring safety check");
    }
    createSelectionMenuForOperation(sessionData.browsingStack.top->operation);
}

static char *getIdentifierOnMarker_static(EditorMarker *marker) {
    EditorBuffer *buffer;
    char         *start, *end, *textMax, *textMin;
    static char   identifier[TMP_STRING_SIZE];

    buffer = marker->buffer;
    assert(buffer && buffer->allocation.text && marker->offset <= buffer->allocation.bufferSize);
    start    = buffer->allocation.text + marker->offset;
    textMin = buffer->allocation.text;
    textMax = buffer->allocation.text + buffer->allocation.bufferSize;
    // move to the beginning of identifier
    for (; start >= textMin && (isalpha(*start) || isdigit(*start) || *start == '_' || *start == '$'); start--)
        ;
    for (start++; start < textMax && isdigit(*start); start++)
        ;
    // now get it
    for (end = start; end < textMax && (isalpha(*end) || isdigit(*end) || *end == '_' || *end == '$'); end++)
        ;
    int length = end - start;
    assert(length < TMP_STRING_SIZE - 1);
    strncpy(identifier, start, length);
    identifier[length] = 0;

    return identifier;
}

static char *getStringInInclude_static(EditorMarker *marker) {
    EditorBuffer *buffer;
    char         *start, *end, *textMax, *textMin;
    static char   string[TMP_STRING_SIZE];

    buffer = marker->buffer;
    assert(buffer && buffer->allocation.text && marker->offset <= buffer->allocation.bufferSize);
    start   = buffer->allocation.text + marker->offset;
    textMin = buffer->allocation.text;
    textMax = buffer->allocation.text + buffer->allocation.bufferSize;

    // Move to the beginning of #include
    for (; start >= textMin && *start != '#'; start--)
        ;
    // TODO ensure we are on an '#include'?
    // Move to first quote or angle bracket
    for (start++; start < textMax && *start != '"' && *start != '<'; start++)
        ;
    // now get it
    for (end = start+1; end < textMax && *end != '"' && *end != '>'; end++)
        ;
    end++;                      /* Include the terminating character */

    int length = end - start;
    assert(length < TMP_STRING_SIZE - 1);
    strncpy(string, start, length);
    string[length] = 0;

    return string;
}

static void replaceString(EditorMarker *marker, int len, char *newString) {
    replaceStringInEditorBuffer(marker->buffer, marker->offset, len, newString,
                                strlen(newString), &editorUndo);
}

static void checkedReplaceString(EditorMarker *marker, int len, char *oldString, char *newString) {
    char *bVal  = marker->buffer->allocation.text + marker->offset;
    bool check = (strlen(oldString) == len && strncmp(oldString, bVal, len) == 0);
    if (check) {
        replaceString(marker, len, newString);
    } else {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "checked replacement of '%s' to '%s' failed on '", oldString, newString);
        int d = strlen(tmpBuff);
        for (int i = 0; i < len; i++)
            tmpBuff[d++] = bVal[i];
        tmpBuff[d++] = '\'';
        tmpBuff[d++] = 0;
        errorMessage(ERR_INTERNAL, tmpBuff);
    }
}

// -------------------------- Undos

static void editorFreeSingleUndo(EditorUndo *uu) {
    if (uu->u.replace.str != NULL && uu->u.replace.strlen != 0) {
        switch (uu->operation) {
        case UNDO_REPLACE_STRING:
            free(uu->u.replace.str);
            break;
        case UNDO_RENAME_BUFFER:
            free(uu->u.rename.name);
            break;
        case UNDO_MOVE_BLOCK:
            break;
        default:
            errorMessage(ERR_INTERNAL, "Unknown operation to undo");
        }
    }
    free(uu);
}

static void editorApplyUndos(EditorUndo *undos, EditorUndo *until, EditorUndo **undoundo, int gen) {
    EditorUndo   *uu, *next;
    EditorMarker *m1, *m2;
    uu = undos;
    while (uu != until && uu != NULL) {
        switch (uu->operation) {
        case UNDO_REPLACE_STRING:
            if (gen == GEN_FULL_OUTPUT) {
                ppcReplace(uu->buffer->fileName, uu->u.replace.offset,
                           uu->buffer->allocation.text + uu->u.replace.offset, uu->u.replace.size,
                           uu->u.replace.str);
            }
            replaceStringInEditorBuffer(uu->buffer, uu->u.replace.offset, uu->u.replace.size,
                                        uu->u.replace.str, uu->u.replace.strlen, undoundo);

            break;
        case UNDO_RENAME_BUFFER:
            if (gen == GEN_FULL_OUTPUT) {
                ppcGotoOffsetPosition(uu->buffer->fileName, 0);
                ppcGenRecord(PPC_MOVE_FILE_AS, uu->u.rename.name);
            }
            renameEditorBuffer(uu->buffer, uu->u.rename.name, undoundo);
            break;
        case UNDO_MOVE_BLOCK:
            m1 = newEditorMarker(uu->buffer, uu->u.moveBlock.offset);
            m2 = newEditorMarker(uu->u.moveBlock.dbuffer, uu->u.moveBlock.doffset);
            if (gen == GEN_FULL_OUTPUT) {
                ppcGotoMarker(m1);
                ppcValueRecord(PPC_REFACTORING_CUT_BLOCK, uu->u.moveBlock.size, "");
            }
            moveBlockInEditorBuffer(m1, m2, uu->u.moveBlock.size, undoundo);
            if (gen == GEN_FULL_OUTPUT) {
                ppcGotoMarker(m1);
                ppcGenRecord(PPC_REFACTORING_PASTE_BLOCK, "");
            }
            freeEditorMarker(m2);
            freeEditorMarker(m1);
            break;
        default:
            errorMessage(ERR_INTERNAL, "Unknown operation to undo");
        }
        next = uu->next;
        editorFreeSingleUndo(uu);
        uu = next;
    }
    assert(uu == until);
}

static void editorUndoUntil(EditorUndo *until, EditorUndo **undoundo) {
    editorApplyUndos(editorUndo, until, undoundo, GEN_NO_OUTPUT);
    editorUndo = until;
}

void applyWholeRefactoringFromUndo(void) {
    EditorUndo *redoTrack;
    redoTrack = NULL;
    editorUndoUntil(refactoringStartingPoint, &redoTrack);
    editorApplyUndos(redoTrack, NULL, NULL, GEN_FULL_OUTPUT);
}


static void renameFromTo(EditorMarker *pos, char *oldName, char *newName) {
    char *actName;
    int   nlen;
    nlen    = strlen(oldName);
    actName = getIdentifierOnMarker_static(pos);
    assert(strcmp(actName, oldName) == 0);
    checkedReplaceString(pos, nlen, oldName, newName);
}

static EditorMarker *createMarkerAt(EditorBuffer *buf, int offset) {
    EditorMarker *point;
    point = NULL;
    if (offset >= 0) {
        point = newEditorMarker(buf, offset);
    }
    return point;
}

static EditorMarker *getPointFromOptions(EditorBuffer *buf) {
    assert(buf);
    return createMarkerAt(buf, refactoringOptions.cursorOffset);
}

static EditorMarker *getMarkFromOptions(EditorBuffer *buf) {
    assert(buf);
    return createMarkerAt(buf, refactoringOptions.markOffset);
}

static void pushMarkersAsReferences(EditorMarkerList **markers, SessionStackEntry *refs, char *name) {
    Reference *rr;

    rr = convertEditorMarkersToReferences(markers);
    for (BrowsingMenu *mm = refs->menu; mm != NULL; mm = mm->next) {
        if (strcmp(mm->referenceable.linkName, name) == 0) {
            for (Reference *r = rr; r != NULL; r = r->next) {
                addReferenceToList(r, &mm->referenceable.references);
            }
        }
    }
    freeReferences(rr);
    recomputeSelectedReferenceable(refs);
}

// ------------------------- Trivial prechecks --------------------------------------

static void askForReallyContinueConfirmation(void) {
    ppcAskConfirmation("The refactoring may change program behaviour, really continue?");
}

// ---------------------------------------------------------------------------------

static bool handleSafetyCheckDifferenceLists(EditorMarkerList *diff1, EditorMarkerList *diff2,
                                             SessionStackEntry *diffrefs) {
    if (diff1 != NULL || diff2 != NULL) {
        for (BrowsingMenu *mm = diffrefs->menu; mm != NULL; mm = mm->next) {
            mm->selected = true;
            mm->visible  = true;
            mm->filterLevel   = 07777777;
            // hack, freeing now all diffs computed by old method
            freeReferences(mm->referenceable.references);
            mm->referenceable.references = NULL;
        }
        pushMarkersAsReferences(&diff1, diffrefs, LINK_NAME_SAFETY_CHECK_MISSED);
        pushMarkersAsReferences(&diff2, diffrefs, LINK_NAME_SAFETY_CHECK_MISSED);
        freeEditorMarkerListButNotMarkers(diff1);
        freeEditorMarkerListButNotMarkers(diff2);
        popFromSession();
        if (refactoringOptions.theRefactoring == AVR_RENAME_MODULE) {
            /* TODO: Handle whatever this for C! Does it even happen!?!? */
            displayResolutionDialog("The module already exists and is referenced in the original"
                                    "project. Renaming will join two modules without possibility"
                                    "of inverse refactoring",
                                    PPCV_BROWSER_TYPE_WARNING);
        } else {
            displayResolutionDialog("These references may be misinterpreted after refactoring",
                                    PPCV_BROWSER_TYPE_WARNING);
        }
        return false;
    } else
        return true;
}

static bool makeSafetyCheckAndUndo(EditorMarker *point, EditorMarkerList **occs, EditorUndo *startPoint,
                                   EditorUndo **redoTrack) {
    bool              result;
    EditorMarkerList *chks;
    EditorMarker     *defin;
    EditorMarkerList *diff1, *diff2;
    SessionStackEntry   *refs, *origrefs, *newrefs, *diffrefs;
    int               pbflag;
    UNUSED            pbflag;

    // safety check

    defin = point;
    // find definition reference? why this was there?
    //&for(dd= *occs; dd!=NULL; dd=dd->next) {
    //& if (isDefinitionUsage(dd->usg.base)) break;
    //&}
    //&if (dd != NULL) defin = dd->d;

    olcxPushSpecialCheckMenuSym(LINK_NAME_SAFETY_CHECK_MISSED);
    safetyCheck(refactoringOptions.project, defin);

    chks = convertReferencesToEditorMarkers(sessionData.browsingStack.top->references);

    editorMarkersDifferences(occs, &chks, &diff1, &diff2);

    freeEditorMarkerListAndMarkers(chks);

    editorUndoUntil(startPoint, redoTrack);

    origrefs = newrefs = diffrefs = NULL;
    SAFETY_CHECK_GET_SYM_LISTS(refs, origrefs, newrefs, diffrefs, pbflag);
    assert(origrefs != NULL && newrefs != NULL && diffrefs != NULL);
    result = handleSafetyCheckDifferenceLists(diff1, diff2, diffrefs);
    return result;
}

static void precheckThatSymbolRefsCorresponds(char *oldName, EditorMarkerList *occs) {
    char         *cid;
    int           off1, off2;
    EditorMarker *pos, *pp;

    for (EditorMarkerList *ll = occs; ll != NULL; ll = ll->next) {
        pos = ll->marker;
        // first check that I have updated reference
        cid = getIdentifierOnMarker_static(pos);
        if (strcmp(cid, oldName) != 0) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff, "something goes wrong: expecting %s instead of %s at %s, offset:%d", oldName, cid,
                    simpleFileName(getRealFileName_static(pos->buffer->fileName)), pos->offset);
            errorMessage(ERR_INTERNAL, tmpBuff);
            return;
        }
        // O.K. check also few characters around
        off1 = pos->offset - RRF_CHARS_TO_PRE_CHECK_AROUND;
        off2 = pos->offset + strlen(oldName) + RRF_CHARS_TO_PRE_CHECK_AROUND;
        if (off1 < 0)
            off1 = 0;
        if (off2 >= pos->buffer->allocation.bufferSize)
            off2 = pos->buffer->allocation.bufferSize - 1;
        pp = newEditorMarker(pos->buffer, off1);
        ppcPreCheck(pp, off2 - off1);
        freeEditorMarker(pp);
    }
}

static void checkedRenameBuffer(EditorBuffer *buffer, char *newName, EditorUndo **undo) {
    if (editorFileExists(newName)) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "Renaming buffer %s to an existing file.\nCan I do this?", buffer->fileName);
        ppcAskConfirmation(tmpBuff);
    }
    renameEditorBuffer(buffer, newName, undo);
}

static void moveMarkerOverSpaces(EditorBuffer *buffer, EditorMarker *marker) {
    do {
        marker->offset++;
    } while (isspace(buffer->allocation.text[marker->offset]));
}

static EditorMarker *adjustMarkerForInclude(EditorMarker *marker) {
    EditorBuffer *buffer    = getOpenedAndLoadedEditorBuffer(marker->buffer->fileName);
    EditorMarker *newMarker = newEditorMarker(buffer, marker->offset);

    assert(buffer->allocation.text[newMarker->offset] == '#');
    moveMarkerOverSpaces(buffer, newMarker);
    assert(strncmp("include", &buffer->allocation.text[newMarker->offset], strlen("include")) == 0);
    newMarker->offset += strlen("include");
    moveMarkerOverSpaces(buffer, newMarker);
    newMarker->offset++;
    return newMarker;
}

static void renameFile(EditorMarker *marker, char *newName) {
    checkedRenameBuffer(marker->buffer, newName, &editorUndo);
}

static void renameIncludes(EditorMarkerList *markers, char *currentIncludeFileName) {
    char newName[MAX_FILE_NAME_SIZE];
    char newPath[MAX_FILE_NAME_SIZE];
    char currentIncludeFilePath[MAX_FILE_NAME_SIZE];

    strcpy(newName, refactoringOptions.renameTo);
    strcpy(newPath, normalizeFileName_static(newName, cwd));
    strcpy(currentIncludeFilePath, normalizeFileName_static(currentIncludeFileName, cwd));

    EditorMarker *markerForTheFile = NULL;
    for (EditorMarkerList *l = markers; l != NULL; l = l->next) {
        // References for #include always points to the '#' but there is also one that
        // points to the first position in the include file. We need to adjust the
        // markers so they point to the filename, and ignore the marker that points to
        // the include file.
        if (strcmp(l->marker->buffer->fileName, currentIncludeFilePath) == 0) {
            markerForTheFile = l->marker;
        } else {
            EditorMarker *adjustedMarker = adjustMarkerForInclude(l->marker);
            if (adjustedMarker != NULL) {
                checkedReplaceString(adjustedMarker, strlen(currentIncludeFileName), currentIncludeFileName,
                                     newName);
            }
            freeEditorMarker(adjustedMarker);
        }
    }
    renameFile(markerForTheFile, newName);
}

static void markOccurrenceFilesAsStale(EditorMarkerList *occurrences) {
    for (EditorMarkerList *l = occurrences; l != NULL; l = l->next) {
        FileItem *fi = getFileItemWithFileNumber(l->marker->buffer->fileNumber);
        fi->lastParsedMtime = 0;
    }
}

static void simpleRename(EditorMarkerList *markerList, EditorMarker *marker, char *symbolName) {
    assert(refactoringOptions.theRefactoring != AVR_RENAME_INCLUDED_FILE);
    for (EditorMarkerList *l = markerList; l != NULL; l = l->next) {
        renameFromTo(l->marker, symbolName, refactoringOptions.renameTo);
    }
    ppcGotoMarker(marker);
}

static EditorMarkerList *getReferences(EditorMarker *point, char *resolveMessage,
                                       int messageType) {
    EditorMarkerList *occs;
    pushReferences(point, "-olcxrename", resolveMessage, messageType); /* TODO: WTF do we use "rename"?!? */
    assert(sessionData.browsingStack.top && sessionData.browsingStack.top->hkSelectedSym);
    occs = convertReferencesToEditorMarkers(sessionData.browsingStack.top->references);
    return occs;
}

static EditorMarkerList *getAndPreCheckReferences(EditorMarker *point, char *nameOnPoint,
                                                      char *resolveMessage, int messageType) {
    EditorMarkerList *occs;
    occs = getReferences(point, resolveMessage, messageType);
    precheckThatSymbolRefsCorresponds(nameOnPoint, occs);
    return occs;
}

static void multipleReferencesInSamePlaceMessage(Reference *r) {
    char tmpBuff[TMP_BUFF_SIZE];
    ppcGotoPosition(r->position);
    sprintf(tmpBuff, "The reference at this place refers to multiple symbols. The refactoring will probably "
                     "damage your program. Do you really want to continue?");
    formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
    ppcAskConfirmation(tmpBuff);
}

static void checkForMultipleReferencesInSamePlace(SessionStackEntry *rstack, BrowsingMenu *ccms) {
    ReferenceableItem *p, *sss;
    BrowsingMenu    *cms;
    bool            pushed;

    p = &ccms->referenceable;
    assert(rstack && rstack->menu);
    sss    = &rstack->menu->referenceable;
    pushed = itIsSymbolToPushOlReferences(p, rstack, &cms, DEFAULT_VALUE);
    // TODO, this can be simplified, as ccms == cms.
    log_debug(":checking %s to %s (%d)", p->linkName, sss->linkName, pushed);
    if (!pushed && haveSameBareName(p, sss)) {
        log_debug("checking %s references", p->linkName);
        for (Reference *r = p->references; r != NULL; r = r->next) {
            if (isReferenceInList(r, rstack->references)) {
                multipleReferencesInSamePlaceMessage(r);
            }
        }
    }
}

static void multipleOccurrenciesSafetyCheck(void) {
    SessionStackEntry *rstack;

    rstack = sessionData.browsingStack.top;
    processSelectedReferences(rstack, checkForMultipleReferencesInSamePlace);
}

// -------------------------------------------- Rename

static void renameAtPoint(EditorMarker *point) {
    assert(refactoringOptions.theRefactoring == AVR_RENAME_SYMBOL);
    if (refactoringOptions.renameTo == NULL) {
        errorMessage(ERR_ST, "this refactoring requires -renameto=<new name> option");
    }

    char *message = STANDARD_C_SELECT_SYMBOLS_MESSAGE;

    char nameOnPoint[TMP_STRING_SIZE];
    EditorMarkerList *occurrences;
    strcpy(nameOnPoint, getIdentifierOnMarker_static(point));
    assert(strlen(nameOnPoint) < TMP_STRING_SIZE - 1);
    occurrences = getAndPreCheckReferences(point, nameOnPoint, message, PPCV_BROWSER_TYPE_INFO);

    EditorUndo *undoStartPoint = editorUndo;

    multipleOccurrenciesSafetyCheck();

    simpleRename(occurrences, point, nameOnPoint);
    //&dumpEditorBuffers();

    markOccurrenceFilesAsStale(occurrences);
    EditorUndo *redoTrack = NULL;
    if (!makeSafetyCheckAndUndo(point, &occurrences, undoStartPoint, &redoTrack)) {
        askForReallyContinueConfirmation();
    }

    editorApplyUndos(redoTrack, NULL, NULL, GEN_FULL_OUTPUT);
    markOccurrenceFilesAsStale(occurrences);

    ppcGotoMarker(point);

    freeEditorMarkerListAndMarkers(occurrences); // O(n^2)!
}

static void renameAtInclude(EditorMarker *point) {
    assert(refactoringOptions.theRefactoring == AVR_RENAME_INCLUDED_FILE
           || refactoringOptions.theRefactoring == AVR_RENAME_MODULE);

    if (refactoringOptions.renameTo == NULL) {
        errorMessage(ERR_ST, "this refactoring requires -renameto=<new name> option");
        return;
    }

    char *message = STANDARD_C_SELECT_SYMBOLS_MESSAGE;

    char stringInInclude[TMP_STRING_SIZE];
    EditorMarkerList *occurrences;
    occurrences = getReferences(point, message, PPCV_BROWSER_TYPE_INFO);
    strcpy(stringInInclude, getStringInInclude_static(point));

    EditorUndo *undoStartPoint = editorUndo;

    multipleOccurrenciesSafetyCheck();

    if (stringInInclude[0] != '"') {
        errorMessage(ERR_ST, "You cannot rename files from the standard library");
        return;
    }

    char *includedFileName = &stringInInclude[1];
    stringInInclude[strlen(stringInInclude)-1] = '\0';

    /* For module rename, the user provides the module name ("target"), not the
     * full filename ("target.h").  Append the original suffix so that
     * renameIncludes gets the correct header filename.
     * newHeaderName must outlive renameAtInclude since renameTo points to it. */
    char newHeaderName[MAX_FILE_NAME_SIZE];
    if (refactoringOptions.theRefactoring == AVR_RENAME_MODULE) {
        char *suffix = getFileSuffix(includedFileName);
        sprintf(newHeaderName, "%s%s", refactoringOptions.renameTo, suffix);
        refactoringOptions.renameTo = newHeaderName;
    }

    /* For module rename, update include guard before renameIncludes renames
     * the file.  The undo/replay mechanism replays edits in forward order,
     * so guard edits must come before the file rename. */
    char headerPath[MAX_FILE_NAME_SIZE];
    strcpy(headerPath, normalizeFileName_static(includedFileName, cwd));
    if (refactoringOptions.theRefactoring == AVR_RENAME_MODULE) {
        EditorBuffer *headerBuf = findOrCreateAndLoadEditorBufferForFile(headerPath);
        if (headerBuf != NULL) {
            char *text = headerBuf->allocation.text;
            int textSize = headerBuf->allocation.bufferSize;

            /* Find the first non-blank character */
            int pos = 0;
            while (pos < textSize && (text[pos] == ' ' || text[pos] == '\t' || text[pos] == '\n'))
                pos++;

            /* Check for #ifndef GUARD pattern */
            if (pos < textSize && strncmp(text + pos, "#ifndef ", 8) == 0) {
                int ifndefPos = pos + 8; /* position of the guard macro */
                /* Extract the guard macro name */
                int guardEnd = ifndefPos;
                while (guardEnd < textSize && text[guardEnd] != '\n' && text[guardEnd] != ' ')
                    guardEnd++;
                int guardLen = guardEnd - ifndefPos;

                char oldGuard[MAX_FILE_NAME_SIZE];
                strncpy(oldGuard, text + ifndefPos, guardLen);
                oldGuard[guardLen] = '\0';

                /* Verify next non-blank line is #define with the same macro */
                int nextLine = guardEnd;
                while (nextLine < textSize && text[nextLine] == '\n')
                    nextLine++;
                bool hasMatchingDefine = (nextLine < textSize
                    && strncmp(text + nextLine, "#define ", 8) == 0
                    && strncmp(text + nextLine + 8, oldGuard, guardLen) == 0);

                if (hasMatchingDefine) {
                    int definePos = nextLine + 8; /* position of macro in #define */

                    /* Build new guard: replace the old module prefix with new.
                     * Old prefix: uppercase filename "source.h" → "SOURCE_H"
                     * Preserves any suffix like "_INCLUDED" */
                    char oldPrefix[MAX_FILE_NAME_SIZE], newPrefix[MAX_FILE_NAME_SIZE];
                    strcpy(oldPrefix, includedFileName);
                    for (char *p = oldPrefix; *p; p++) {
                        if (*p == '.') *p = '_';
                        else *p = toupper(*p);
                    }
                    strcpy(newPrefix, refactoringOptions.renameTo);
                    for (char *p = newPrefix; *p; p++) {
                        if (*p == '.') *p = '_';
                        else *p = toupper(*p);
                    }

                    int oldPrefixLen = strlen(oldPrefix);
                    /* Only replace if the guard starts with our expected prefix */
                    if (strncmp(oldGuard, oldPrefix, oldPrefixLen) == 0) {
                        char newGuard[MAX_FILE_NAME_SIZE];
                        sprintf(newGuard, "%s%s", newPrefix, oldGuard + oldPrefixLen);

                        /* Replace #ifndef first, then #define.  EditorMarkers
                         * track position shifts, so order doesn't affect
                         * correctness.  This order matches the replay output. */
                        EditorMarker *m1 = newEditorMarker(headerBuf, ifndefPos);
                        EditorMarker *m2 = newEditorMarker(headerBuf, definePos);
                        checkedReplaceString(m1, guardLen, oldGuard, newGuard);
                        checkedReplaceString(m2, guardLen, oldGuard, newGuard);
                        freeEditorMarker(m1);
                        freeEditorMarker(m2);
                    }
                }
            }
        }
    }

    renameIncludes(occurrences, includedFileName);

    /* For module rename, also rename the companion .c file */
    if (refactoringOptions.theRefactoring == AVR_RENAME_MODULE) {
        char companionPath[MAX_FILE_NAME_SIZE];
        char newCompanionName[MAX_FILE_NAME_SIZE];

        /* Build current companion path: same as header but with .c suffix */
        strcpy(companionPath, headerPath);
        strcpy(getFileSuffix(companionPath), ".c");

        if (editorFileExists(companionPath)) {
            /* Build new name: renameTo already has .h, replace with .c */
            strcpy(newCompanionName, refactoringOptions.renameTo);
            strcpy(getFileSuffix(newCompanionName), ".c");

            EditorBuffer *buf = findOrCreateAndLoadEditorBufferForFile(companionPath);
            EditorMarker *marker = newEditorMarker(buf, 0);
            renameFile(marker, newCompanionName);
            freeEditorMarker(marker);
        }
    }

    EditorUndo *redoTrack = NULL;
    editorUndoUntil(undoStartPoint, &redoTrack);
    editorApplyUndos(redoTrack, NULL, NULL, GEN_FULL_OUTPUT);

    ppcGotoMarker(point);

    freeEditorMarkerListAndMarkers(occurrences); // O(n^2)!
}

static void clearParameterPositions(void) {
    parameterPosition      = NO_POSITION;
    parameterBeginPosition = NO_POSITION;
    parameterEndPosition   = NO_POSITION;
}

static Result getParameterNamePosition(EditorMarker *point, char *fileName, int argn) {
    char  pushOptions[TMP_STRING_SIZE];
    char *nameOnPoint;

    nameOnPoint = getIdentifierOnMarker_static(point);
    clearParameterPositions();
    assert(strcmp(nameOnPoint, fileName) == 0);
    sprintf(pushOptions, "-olcxgotoparname%d", argn);
    parseBufferUsingServer(refactoringOptions.project, point, NULL, pushOptions, NULL);
    popFromSession();
    if (parameterPosition.file != NO_FILE_NUMBER) {
        return RESULT_OK;
    } else {
        return RESULT_ERR;
    }
}

static Result getParameterPosition(EditorMarker *point, char *functionOrMacroName, int argn) {
    char  pushOptions[TMP_STRING_SIZE];
    char *nameOnPoint;

    nameOnPoint = getIdentifierOnMarker_static(point);
    if (strcmp(nameOnPoint, functionOrMacroName) != 0) {
        char tmpBuff[TMP_BUFF_SIZE];
        ppcGotoMarker(point);
        sprintf(tmpBuff, "This reference is not pointing to the function/method name. Maybe a composed symbol. "
                         "Sorry, do not know how to handle this case.");
        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
        errorMessage(ERR_ST, tmpBuff);
    }

    clearParameterPositions();
    sprintf(pushOptions, "-olcxgetparamcoord%d", argn);
    parseBufferUsingServer(refactoringOptions.project, point, NULL, pushOptions, NULL);
    popFromSession();

    Result result = RESULT_OK;
    if (parameterBeginPosition.file == NO_FILE_NUMBER || parameterEndPosition.file == NO_FILE_NUMBER ||
        parameterBeginPosition.file == -1 || parameterEndPosition.file == -1) {
        ppcGotoMarker(point);
        errorMessage(ERR_INTERNAL, "Can't get end of parameter");
        result = RESULT_ERR;
    }
    // check some logical preconditions,
    if (parameterBeginPosition.file != parameterEndPosition.file ||
        parameterBeginPosition.line > parameterEndPosition.line ||
        (parameterBeginPosition.line == parameterEndPosition.line &&
         parameterBeginPosition.col > parameterEndPosition.col)) {
        char tmpBuff[TMP_BUFF_SIZE];
        ppcGotoMarker(point);
        sprintf(tmpBuff, "Something goes wrong at this occurence, can't get reasonable parameter limits");
        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
        errorMessage(ERR_ST, tmpBuff);
        result = RESULT_ERR;
    }

    return result;
}

// !!!!!!!!! point and endMarker can be the same marker !!!!!!
static int addStringAsParameter(EditorMarker *point, EditorMarker *endMarkerOrMark, char *fileName,
                                int argumentNumber, char *parameterDeclaration) {
    char         *text;
    char          insertionText[REFACTORING_TMP_STRING_SIZE];
    char         *separator1, *separator2;
    int           insertionOffset;
    EditorMarker *beginMarker;

    insertionOffset = -1;
    Result rr = getParameterPosition(point, fileName, argumentNumber);
    if (rr != RESULT_OK) {
        errorMessage(ERR_INTERNAL, "Problem while adding parameter");
        return insertionOffset;
    }
    if (argumentNumber > parameterCount + 1) {
        errorMessage(ERR_ST, "Parameter number out of limits");
        return insertionOffset;
    }


    text = point->buffer->allocation.text;

    if (endMarkerOrMark == NULL) {
        beginMarker = newEditorMarkerForPosition(parameterBeginPosition);
    } else {
        beginMarker = endMarkerOrMark;
        assert(beginMarker->buffer->fileNumber == parameterBeginPosition.file);
        moveEditorMarkerToLineAndColumn(beginMarker, parameterBeginPosition.line,
                                        parameterBeginPosition.col);
    }

    separator1 = "";
    separator2 = "";
    if (positionsAreEqual(parameterBeginPosition, parameterEndPosition)) {
        if (text[beginMarker->offset] == '(') {
            // function with no parameter
            beginMarker->offset++;
            separator1 = "";
            separator2 = "";
        } else if (text[beginMarker->offset] == ')') {
            // beyond limit
            separator1 = ", ";
            separator2 = "";
        } else {
            char tmpBuff[TMP_BUFF_SIZE];
            ppcGotoMarker(point);
            sprintf(tmpBuff,
                    "Something went wrong here, probably different parameter coordinates at different cpp passes.");
            formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
            FATAL_ERROR(ERR_INTERNAL, tmpBuff, EXIT_FAILURE);
            assert(0);
        }
    } else {
        if (text[beginMarker->offset] == '(') {
            separator1 = "";
            separator2 = ", ";
        } else {
            separator1 = " ";
            separator2 = ",";
        }
        beginMarker->offset++;
    }

    int replacementLength = 0;

    if (parameterListIsVoid) {
        /* Then we neeed to remove the "void", or rather everything between the "()" */
        replacementLength = parameterEndPosition.col - parameterBeginPosition.col - 1;
        separator2        = ""; /* And there should be no separator after the new parameter */
    }

    sprintf(insertionText, "%s%s%s", separator1, parameterDeclaration, separator2);
    assert(strlen(parameterDeclaration) < REFACTORING_TMP_STRING_SIZE - 1);

    insertionOffset = beginMarker->offset;
    replaceString(beginMarker, replacementLength, insertionText);

    if (endMarkerOrMark == NULL) {
        freeEditorMarker(beginMarker);
    }

    return insertionOffset;
}

static int isThisSymbolUsed(EditorMarker *marker) {
    int refn;
    pushReferences(marker, "-olcxpushforusagecheck", STANDARD_SELECT_SYMBOLS_MESSAGE, PPCV_BROWSER_TYPE_INFO);
    LIST_LEN(refn, Reference, sessionData.browsingStack.top->references);
    popFromSession();
    return refn > 1;
}

static int isParameterUsedExceptRecursiveCalls(EditorMarker *pmarker) {
    // TODO: simplification for now - is it used at all?
    return isThisSymbolUsed(pmarker);
}

typedef enum {
    CHECK_FOR_ADD_PARAM,
    CHECK_FOR_DEL_PARAM
} ParameterCheckKind;

static void checkThatParameterIsUnused(EditorMarker *marker, char *functionName, int argn,
                                       ParameterCheckKind checkKind) {
    char          parameterName[TMP_STRING_SIZE];

    Result result = getParameterNamePosition(marker, functionName, argn);
    if (result != RESULT_OK) {
        ppcAskConfirmation("Can not parse parameter definition, continue anyway?");
        return;
    }

    EditorMarker *positionMarker = newEditorMarkerForPosition(parameterPosition);
    strncpy(parameterName, getIdentifierOnMarker_static(positionMarker), TMP_STRING_SIZE);
    parameterName[TMP_STRING_SIZE - 1] = 0;
    if (isParameterUsedExceptRecursiveCalls(positionMarker)) {
        char tmpBuff[TMP_BUFF_SIZE];
        if (checkKind == CHECK_FOR_ADD_PARAM) {
            sprintf(tmpBuff, "parameter '%s' clashes with an existing symbol, continue anyway?", parameterName);
            ppcAskConfirmation(tmpBuff);
        } else if (checkKind == CHECK_FOR_DEL_PARAM) {
            sprintf(tmpBuff, "parameter '%s' is used, delete it anyway?", parameterName);
            ppcAskConfirmation(tmpBuff);
        } else {
            assert(0);
        }
    }
    freeEditorMarker(positionMarker);
}

static void addParameter(EditorMarker *pos, char *fname, int argumentNumber, Usage usage) {
    if (isDefinitionOrDeclarationUsage(usage)) {
        if (addStringAsParameter(pos, NULL, fname, argumentNumber, refactoringOptions.refactor_parameter_name) != -1)
            // now check that there is no conflict
            if (isDefinitionUsage(usage))
                checkThatParameterIsUnused(pos, fname, argumentNumber, CHECK_FOR_ADD_PARAM);
    } else {
        addStringAsParameter(pos, NULL, fname, argumentNumber, refactoringOptions.refactor_parameter_value);
    }
}

static void deleteParameter(EditorMarker *marker, char *fname, int argumentNumber, Usage usage) {
    char         *text;
    EditorMarker *m1, *m2;

    Result res = getParameterPosition(marker, fname, argumentNumber);
    if (res != RESULT_OK)
        return;

    m1 = newEditorMarkerForPosition(parameterBeginPosition);
    m2 = newEditorMarkerForPosition(parameterEndPosition);

    text = marker->buffer->allocation.text;

    if (positionsAreEqual(parameterBeginPosition, parameterEndPosition)) {
        if (text[m1->offset] == '(') {
            // function with no parameter
        } else if (text[m1->offset] == ')') {
            // beyond limit
        } else {
            FATAL_ERROR(ERR_INTERNAL,
                       "Something goes wrong, probably different parameter coordinates at different cpp passes.",
                       EXIT_FAILURE);
            assert(0);
        }
        errorMessage(ERR_ST, "Parameter number out of limits");
    } else {
        if (text[m1->offset] == '(') {
            m1->offset++;
            if (text[m2->offset] == ',') {
                m2->offset++;
                // here pass also blank symbols
            }
            moveEditorMarkerToNonBlank(m2, 1);
        }
        if (isDefinitionUsage(usage)) {
            // this must be at the end, because it discards values
            // of parameterBeginPosition and parameterEndPosition
            checkThatParameterIsUnused(marker, fname, argumentNumber, CHECK_FOR_DEL_PARAM);
        }

        assert(m1->offset <= m2->offset);
        replaceString(m1, m2->offset - m1->offset, "");
    }
    freeEditorMarker(m1);
    freeEditorMarker(m2);
}

static void moveParameter(EditorMarker *marker, char *fname, int argFrom, int argTo) {
    char         *text;
    char          par[REFACTORING_TMP_STRING_SIZE];
    int           plen;
    EditorMarker *m1, *m2;

    Result res = getParameterPosition(marker, fname, argFrom);
    if (res != RESULT_OK)
        return;

    m1 = newEditorMarkerForPosition(parameterBeginPosition);
    m2 = newEditorMarkerForPosition(parameterEndPosition);

    text      = marker->buffer->allocation.text;
    plen      = 0;
    par[plen] = 0;

    if (positionsAreEqual(parameterBeginPosition, parameterEndPosition)) {
        if (text[m1->offset] == '(') {
            // function with no parameter
        } else if (text[m1->offset] == ')') {
            // beyond limit
        } else {
            FATAL_ERROR(ERR_INTERNAL,
                       "Something goes wrong, probably different parameter coordinates at different cpp passes.",
                       EXIT_FAILURE);
            assert(0);
        }
        errorMessage(ERR_ST, "Parameter out of limit");
    } else {
        m1->offset++;
        moveEditorMarkerToNonBlank(m1, 1);
        m2->offset--;
        moveEditorMarkerToNonBlank(m2, -1);
        m2->offset++;
        assert(m1->offset <= m2->offset);
        plen = m2->offset - m1->offset;
        strncpy(par, MARKER_TO_POINTER(m1), plen);
        par[plen] = 0;
        deleteParameter(marker, fname, argFrom, UsageUsed);
        addStringAsParameter(marker, NULL, fname, argTo, par);
    }
    freeEditorMarker(m1);
    freeEditorMarker(m2);
}

static void applyParameterManipulationToFunction(char *functionName, EditorMarkerList *occurrences,
                                                 int manipulation, int argn1, int argn2) {
    int progress, count;

    /* TODO Is it guaranteed that the occurrences always starts with Defined/Declared? */
    LIST_LEN(count, EditorMarkerList, occurrences);
    progress = 0;
    for (EditorMarkerList *l = occurrences; l != NULL; l = l->next) {
        if (l->usage != UsageUndefinedMacro) {
            /* TODO: Should we not abort if any of the occurrences fail? */
            if (manipulation == PPC_AVR_ADD_PARAMETER) {
                addParameter(l->marker, functionName, argn1, l->usage);
            } else if (manipulation == PPC_AVR_DEL_PARAMETER) {
                deleteParameter(l->marker, functionName, argn1, l->usage);
            } else if (manipulation == PPC_AVR_MOVE_PARAMETER) {
                moveParameter(l->marker, functionName, argn1, argn2);
            } else {
                errorMessage(ERR_INTERNAL, "unknown parameter manipulation");
                break;
            }
        }
        writeRelativeProgress((100 * progress++)/count);
    }
    writeRelativeProgress(100);
}

// -------------------------------------- ParameterManipulations

static void applyParameterManipulation(EditorMarker *point, int manipulation, int argn1,
                                       int argn2) {
    char              nameOnPoint[TMP_STRING_SIZE];
    EditorMarkerList *occurrences;

    strcpy(nameOnPoint, getIdentifierOnMarker_static(point));
    pushReferences(point, "-olcxargmanip", STANDARD_SELECT_SYMBOLS_MESSAGE, PPCV_BROWSER_TYPE_INFO);
    occurrences = convertReferencesToEditorMarkers(sessionData.browsingStack.top->references);

    ppcGotoMarker(point);

    applyParameterManipulationToFunction(nameOnPoint, occurrences, manipulation, argn1, argn2);
    freeEditorMarkerListAndMarkers(occurrences); // O(n^2)!
}

static void parameterManipulation(EditorMarker *point, int manip, int argn1, int argn2) {
    applyParameterManipulation(point, manip, argn1, argn2);
    // and generate output
    applyWholeRefactoringFromUndo();
    ppcGotoMarker(point);
}


// -------------------  Extract

static void extractFunction(EditorMarker *point, EditorMarker *mark) {
    parseBufferUsingServer(refactoringOptions.project, point, mark, "-olcxextract", NULL);
}

static void extractMacro(EditorMarker *point, EditorMarker *mark) {
    parseBufferUsingServer(refactoringOptions.project, point, mark, "-olcxextract", "-olexmacro");
}

static void extractVariable(EditorMarker *point, EditorMarker *mark) {
    parseBufferUsingServer(refactoringOptions.project, point, mark, "-olcxextract", "-olexvariable");
}

void serverPerformRefactoring(void) {
    ENTER();

    assert(options.project != NULL);

    deepCopyOptionsFromTo(&options, &refactoringOptions);
    refactoringOptions.serverOperation = OP_INTERNAL_LIST;

    loadAllOpenedEditorBuffers();
    quasiSaveModifiedEditorBuffers();

    if (inputFileName == NULL) {
        errorMessage(ERR_ST, "No input file for refactoring");
        LEAVE();
        return;
    }

    EditorBuffer *buf = findOrCreateAndLoadEditorBufferForFile(inputFileName);

    EditorMarker *point = getPointFromOptions(buf);
    EditorMarker *mark  = getMarkFromOptions(buf);

    refactoringStartingPoint = editorUndo;

    /* Server is already initialized — no mainTaskEntryInitialisations needed */

    progressFactor = 1;

    switch (refactoringOptions.theRefactoring) {
    case AVR_RENAME_SYMBOL:
        progressFactor = 3;
        renameAtPoint(point);
        break;
    case AVR_EXTRACT_FUNCTION:
        extractFunction(point, mark);
        break;
    case AVR_EXTRACT_MACRO:
        extractMacro(point, mark);
        break;
    case AVR_EXTRACT_VARIABLE:
        extractVariable(point, mark);
        break;
    case AVR_ADD_PARAMETER:
    case AVR_DEL_PARAMETER:
    case AVR_MOVE_PARAMETER:
        progressFactor = 3;
        parameterManipulation(point, refactoringOptions.theRefactoring, refactoringOptions.parnum,
                              refactoringOptions.parnum2);
        break;
    case AVR_RENAME_MODULE:
    case AVR_RENAME_INCLUDED_FILE:
        progressFactor = 2;
        renameAtInclude(point);
        break;
    case AVR_MOVE_FUNCTION:
        progressFactor = 2;
        moveFunction(point);
        break;
    case AVR_ORGANIZE_INCLUDES:
        organizeIncludes(point);
        break;
    default:
        errorMessage(ERR_ST, "This refactoring is not yet supported via server");
        break;
    }

    writeRelativeProgress(0);
    writeRelativeProgress(100);

    markModifiedEditorBuffersAsStale();
    quasiSaveModifiedEditorBuffers();

    LEAVE();
}
