#include "refactory.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/* Main is currently needed for:
   mainTaskEntryInitialisations
   mainOpenOutputFile
 */
#include "argumentsvector.h"
#include "commons.h"
#include "cxref.h"
#include "editor.h"
#include "editormarker.h"
#include "filetable.h"
#include "globals.h"
#include "head.h"
#include "list.h"
#include "parsing.h"
#include "startup.h"
#include "misc.h"
#include "options.h"
#include "ppc.h"
#include "progress.h"
#include "proto.h"
#include "protocol.h"
#include "refactorings.h"
#include "scope.h"
#include "server.h"
#include "session.h"
#include "undo.h"
#include "xref.h"

#include "log.h"

#define RRF_CHARS_TO_PRE_CHECK_AROUND 1
#define MAX_NARGV_OPTIONS_COUNT 50


typedef enum {
    APPLY_CHECKS,
    NO_CHECKS
} ToCheckOrNot;

static EditorUndo *refactoringStartingPoint;

static bool editServerSubTaskFirstPass = true;

static char *serverStandardOptions[] = {
    "xref",
    "-xrefactory-II",
    //& "-debug",
    "-server",
    NULL,
};

// Refactory will always use xref2 protocol when generating/updating xrefs
static char *xrefUpdateOptions[] = {
    "xref",
    "-xrefactory-II",
    NULL,
};

static Options refactoringOptions;

static char *updateOption = "-fastupdate";


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

////////////////////////////////////////////////////////////////////////////////////////
// ----------------------- interface to refactory sub-task --------------------------

// be very careful when calling this function as it is messing all static variables
// including options, ...
// call to this function MUST be followed by a pushing action, to refresh options
static void ensureReferencesAreUpdated(char *project) {
    int argumentCount;
    char *argumentVector[MAX_NARGV_OPTIONS_COUNT];

    // following would be too long to be allocated on stack
    static Options savedOptions;

    if (updateOption == NULL || *updateOption == 0) {
        writeRelativeProgress(100);
        return;
    }

    ppcBegin(PPC_UPDATE_REPORT);

    quasiSaveModifiedEditorBuffers();

    deepCopyOptionsFromTo(&options, &savedOptions);

    setArguments(argumentVector, project, NULL, NULL);
    argumentCount = argument_count(argumentVector);
    int xrefUpdateOptionsCount = argument_count(xrefUpdateOptions);
    for (int i = 1; i < xrefUpdateOptionsCount; i++) {
        argumentVector[argumentCount++] = xrefUpdateOptions[i];
    }
    argumentVector[argumentCount++] = updateOption;

    currentPass = ANY_PASS;
    ArgumentsVector args = {.argc = argumentCount, .argv = argumentVector};
    mainTaskEntryInitialisations(args);

    callXref(args, true);

    deepCopyOptionsFromTo(&savedOptions, &options);
    ppcEnd(PPC_UPDATE_REPORT);

    args = (ArgumentsVector){.argc = argument_count(serverStandardOptions), .argv = serverStandardOptions};
    mainTaskEntryInitialisations(args);
    editServerSubTaskFirstPass = true;
}

static void parseBufferUsingServer(char *project, EditorMarker *point, EditorMarker *mark,
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
    ArgumentsVector args = {.argc = argument_count(serverStandardOptions), .argv = serverStandardOptions};
    ArgumentsVector nargs = {.argc = argumentCount, .argv = argumentVector};
    initServer(nargs);
    callServer(args, nargs, &editServerSubTaskFirstPass);
}

/* Interactive command loop for refactoring operations that need user input.
 *
 * Called from refactoring operations (like rename) when user interaction is needed,
 * typically via displayResolutionDialog() to handle name collisions or symbol selection.
 *
 * OPERATION:
 * - Reads commands from stdin via getPipedOptions() (sent by editor/test driver)
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
    deepCopyOptionsFromTo(&options, &savedOptions);
    for (;;) {
        closeOutputFile();
        ppcSynchronize();
        deepCopyOptionsFromTo(&savedOptions, &options);

        ArgumentsVector args = {.argc = argument_count(serverStandardOptions), .argv = serverStandardOptions};
        processOptions(args, PROCESS_FILE_ARGUMENTS_NO);

        ArgumentsVector pipedOptions = getPipedOptions();
        openOutputFile(refactoringOptions.outputFileName);
        if (pipedOptions.argc <= 1)
            break;

        initServer(pipedOptions);
        if (options.continueRefactoring != RC_NONE)
            break;

        callServer(args, pipedOptions,
                   &editServerSubTaskFirstPass);
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
    // !!!!update references MUST be followed by a pushing action, to refresh options
    ensureReferencesAreUpdated(refactoringOptions.project);
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

static void applyWholeRefactoringFromUndo(void) {
    EditorUndo *redoTrack;
    redoTrack = NULL;
    editorUndoUntil(refactoringStartingPoint, &redoTrack);
    editorApplyUndos(redoTrack, NULL, NULL, GEN_FULL_OUTPUT);
}

static void fatalErrorOnPosition(EditorMarker *p, int errType, char *message) {
    EditorUndo *redo;
    redo = NULL;
    editorUndoUntil(refactoringStartingPoint, &redo);
    ppcGotoMarker(p);
    FATAL_ERROR(errType, message, XREF_EXIT_ERR);
    // unreachable, but do the things properly
    editorApplyUndos(redo, NULL, &editorUndo, GEN_NO_OUTPUT);
}

// -------------------------- end of Undos

static void removeNonCommentCode(EditorMarker *marker, int length) {
    assert(marker->buffer && marker->buffer->allocation.text);
    char *s  = marker->buffer->allocation.text + marker->offset;
    int l = length;
    EditorMarker *mm = newEditorMarker(marker->buffer, marker->offset);
    if (marker->offset + l > marker->buffer->allocation.bufferSize) {
        l = marker->buffer->allocation.bufferSize - marker->offset;
    }

    int n = 0;
    while (l > 0) {
        int c = *s;
        if (c == '/' && l > 1 && *(s + 1) == '*' && (l <= 2 || *(s + 2) != '&')) {
            // /**/ comment
            replaceString(mm, n, "");
            s = mm->buffer->allocation.text + mm->offset;
            s += 2;
            l -= 2;
            while (!(*s == '*' && *(s + 1) == '/')) {
                s++;
                l--;
            }
            s += 2;
            l -= 2;
            mm->offset = s - mm->buffer->allocation.text;
            n          = 0;
        } else if (c == '/' && l > 1 && *(s + 1) == '/' && (l <= 2 || *(s + 2) != '&')) {
            // // comment
            replaceString(mm, n, "");
            s = mm->buffer->allocation.text + mm->offset;
            s += 2;
            l -= 2;
            while (*s != '\n') {
                s++;
                l--;
            }
            s += 1;
            l -= 1;
            mm->offset = s - mm->buffer->allocation.text;
            n          = 0;
        } else if (c == '"') {
            // string, pass it removing all inside (also /**/ comments)
            s++;
            l--;
            n++;
            while (*s != '"' && l > 0) {
                s++;
                l--;
                n++;
                if (*s == '\\') {
                    s++;
                    l--;
                    n++;
                    s++;
                    l--;
                    n++;
                }
            }
        } else {
            s++;
            l--;
            n++;
        }
    }
    if (n > 0) {
        replaceString(mm, n, "");
    }
    freeEditorMarker(mm);
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
    return createMarkerAt(buf, refactoringOptions.olCursorOffset);
}

static EditorMarker *getMarkFromOptions(EditorBuffer *buf) {
    assert(buf);
    return createMarkerAt(buf, refactoringOptions.olMarkOffset);
}

static void pushMarkersAsReferences(EditorMarkerList **markers, SessionStackEntry *refs, char *name) {
    Reference *rr;

    rr = convertEditorMarkersToReferences(markers);
    for (BrowserMenu *mm = refs->menu; mm != NULL; mm = mm->next) {
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
        for (BrowserMenu *mm = diffrefs->menu; mm != NULL; mm = mm->next) {
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

static EditorMarker *createMarkerForExpressionStart(EditorMarker *marker, ExpressionStartKind startKind) {
    Position position;
    parseBufferUsingServer(refactoringOptions.project, marker, NULL, "-olcxprimarystart", NULL);
    deleteEntryFromSessionStack(sessionData.browsingStack.top);
    if (startKind == GET_PRIMARY_START) {
        position = primaryStartPosition;
    } else if (startKind == GET_STATIC_PREFIX_START) {
        position = staticPrefixStartPosition;
    } else {
        assert(0);
    }
    if (position.file == NO_FILE_NUMBER) {
        if (startKind == GET_STATIC_PREFIX_START) {
            fatalErrorOnPosition(marker, ERR_ST,
                                 "Can't determine static prefix. Maybe non-static reference to a static object? "
                                 "Make this invocation static before refactoring.");
        } else {
            fatalErrorOnPosition(marker, ERR_INTERNAL, "Can't determine beginning of primary expression");
        }
        return NULL;
    } else {
        EditorBuffer *buffer    = getOpenedAndLoadedEditorBuffer(getFileItemWithFileNumber(position.file)->name);
        EditorMarker *newMarker = newEditorMarker(buffer, 0);
        moveEditorMarkerToLineAndColumn(newMarker, position.line, position.col);
        assert(newMarker->buffer == marker->buffer);
        assert(newMarker->offset <= marker->offset);
        return newMarker;
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

static void simpleRename(EditorMarkerList *markerList, EditorMarker *marker, char *symbolName,
                         char *symbolLinkName
) {
    // assert(options.theRefactoring == refactoringOptions.theRefactoring);
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

static EditorMarkerList *pushGetAndPreCheckReferences(EditorMarker *point, char *nameOnPoint,
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

static void checkForMultipleReferencesInSamePlace(SessionStackEntry *rstack, BrowserMenu *ccms) {
    ReferenceableItem *p, *sss;
    BrowserMenu    *cms;
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

    ensureReferencesAreUpdated(refactoringOptions.project);

    char *message = STANDARD_C_SELECT_SYMBOLS_MESSAGE;

    char nameOnPoint[TMP_STRING_SIZE];
    EditorMarkerList *occurrences;
    strcpy(nameOnPoint, getIdentifierOnMarker_static(point));
    assert(strlen(nameOnPoint) < TMP_STRING_SIZE - 1);
    occurrences = pushGetAndPreCheckReferences(point, nameOnPoint, message, PPCV_BROWSER_TYPE_INFO);

    BrowserMenu *symbolsMenu = sessionData.browsingStack.top->hkSelectedSym;
    char *symLinkName = symbolsMenu->referenceable.linkName;

    EditorUndo *undoStartPoint = editorUndo;

    multipleOccurrenciesSafetyCheck();

    simpleRename(occurrences, point, nameOnPoint, symLinkName);
    //&dumpEditorBuffers();
    EditorUndo *redoTrack = NULL;
    if (!makeSafetyCheckAndUndo(point, &occurrences, undoStartPoint, &redoTrack)) {
        askForReallyContinueConfirmation();
    }

    editorApplyUndos(redoTrack, NULL, NULL, GEN_FULL_OUTPUT);

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

    ensureReferencesAreUpdated(refactoringOptions.project);

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

    renameIncludes(occurrences, includedFileName);

    EditorUndo *redoTrack = NULL;
    editorUndoUntil(undoStartPoint, &redoTrack);
    editorApplyUndos(redoTrack, NULL, NULL, GEN_FULL_OUTPUT);

    ppcGotoMarker(point);

    freeEditorMarkerListAndMarkers(occurrences); // O(n^2)!
}

static void clearParamPositions(void) {
    parameterPosition      = noPosition;
    parameterBeginPosition = noPosition;
    parameterEndPosition   = noPosition;
}

static Result getParameterNamePosition(EditorMarker *point, char *fileName, int argn) {
    char  pushOptions[TMP_STRING_SIZE];
    char *nameOnPoint;

    nameOnPoint = getIdentifierOnMarker_static(point);
    clearParamPositions();
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

    clearParamPositions();
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
            FATAL_ERROR(ERR_INTERNAL, tmpBuff, XREF_EXIT_ERR);
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
    pushReferences(marker, "-olcxpushforlm", STANDARD_SELECT_SYMBOLS_MESSAGE, PPCV_BROWSER_TYPE_INFO);
    LIST_LEN(refn, Reference, sessionData.browsingStack.top->references);
    popFromSession();
    return refn > 1;
}

static int isParameterUsedExceptRecursiveCalls(EditorMarker *pmarker, EditorMarker *fmarker) {
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
    if (isParameterUsedExceptRecursiveCalls(positionMarker, marker)) {
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

static void deleteParameter(EditorMarker *pos, char *fname, int argumentNumber, Usage usage) {
    char         *text;
    EditorMarker *m1, *m2;

    Result res = getParameterPosition(pos, fname, argumentNumber);
    if (res != RESULT_OK)
        return;

    m1 = newEditorMarkerForPosition(parameterBeginPosition);
    m2 = newEditorMarkerForPosition(parameterEndPosition);

    text = pos->buffer->allocation.text;

    if (positionsAreEqual(parameterBeginPosition, parameterEndPosition)) {
        if (text[m1->offset] == '(') {
            // function with no parameter
        } else if (text[m1->offset] == ')') {
            // beyond limit
        } else {
            FATAL_ERROR(ERR_INTERNAL,
                       "Something goes wrong, probably different parameter coordinates at different cpp passes.",
                       XREF_EXIT_ERR);
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
            // of s_paramBeginPosition and s_paramEndPosition
            checkThatParameterIsUnused(pos, fname, argumentNumber, CHECK_FOR_DEL_PARAM);
        }

        assert(m1->offset <= m2->offset);
        replaceString(m1, m2->offset - m1->offset, "");
    }
    freeEditorMarker(m1);
    freeEditorMarker(m2);
}

static void moveParameter(EditorMarker *pos, char *fname, int argFrom, int argTo) {
    char         *text;
    char          par[REFACTORING_TMP_STRING_SIZE];
    int           plen;
    EditorMarker *m1, *m2;

    Result res = getParameterPosition(pos, fname, argFrom);
    if (res != RESULT_OK)
        return;

    m1 = newEditorMarkerForPosition(parameterBeginPosition);
    m2 = newEditorMarkerForPosition(parameterEndPosition);

    text      = pos->buffer->allocation.text;
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
                       XREF_EXIT_ERR);
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
        deleteParameter(pos, fname, argFrom, UsageUsed);
        addStringAsParameter(pos, NULL, fname, argTo, par);
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

    ensureReferencesAreUpdated(refactoringOptions.project);

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

//------------------------------------------------------------

// basically move marker to the first non blank and non comment symbol at the same
// line as the marker is or to the newline character
static void moveMarkerToTheEndOfDefinitionScope(EditorMarker *mm) {
    int offset;
    offset = mm->offset;
    moveEditorMarkerToNonBlankOrNewline(mm, 1);
    if (mm->offset >= mm->buffer->allocation.bufferSize) {
        return;
    }
    if (CHAR_ON_MARKER(mm) == '/' && CHAR_AFTER_MARKER(mm) == '/') {
        if (refactoringOptions.commentMovingMode == CM_NO_COMMENT)
            return;
        moveEditorMarkerToNewline(mm, 1);
        mm->offset++;
    } else if (CHAR_ON_MARKER(mm) == '/' && CHAR_AFTER_MARKER(mm) == '*') {
        if (refactoringOptions.commentMovingMode == CM_NO_COMMENT)
            return;
        mm->offset++;
        mm->offset++;
        while (mm->offset < mm->buffer->allocation.bufferSize &&
               (CHAR_ON_MARKER(mm) != '*' || CHAR_AFTER_MARKER(mm) != '/')) {
            mm->offset++;
        }
        if (mm->offset < mm->buffer->allocation.bufferSize) {
            mm->offset++;
            mm->offset++;
        }
        offset = mm->offset;
        moveEditorMarkerToNonBlankOrNewline(mm, 1);
        if (CHAR_ON_MARKER(mm) == '\n')
            mm->offset++;
        else
            mm->offset = offset;
    } else if (CHAR_ON_MARKER(mm) == '\n') {
        mm->offset++;
    } else {
        if (refactoringOptions.commentMovingMode == CM_NO_COMMENT)
            return;
        mm->offset = offset;
    }
}

static int markerWRTComment(EditorMarker *mm, int *commentBeginOffset) {
    char *b, *s, *e, *mms;
    assert(mm->buffer && mm->buffer->allocation.text);
    s   = mm->buffer->allocation.text;
    e   = s + mm->buffer->allocation.bufferSize;
    mms = s + mm->offset;
    while (s < e && s < mms) {
        b = s;
        if (*s == '/' && (s + 1) < e && *(s + 1) == '*') {
            // /**/ comment
            s += 2;
            while ((s + 1) < e && !(*s == '*' && *(s + 1) == '/'))
                s++;
            if (s + 1 < e)
                s += 2;
            if (s > mms) {
                *commentBeginOffset = b - mm->buffer->allocation.text;
                return MARKER_IS_IN_STAR_COMMENT;
            }
        } else if (*s == '/' && s + 1 < e && *(s + 1) == '/') {
            // // comment
            s += 2;
            while (s < e && *s != '\n')
                s++;
            if (s < e)
                s += 1;
            if (s > mms) {
                *commentBeginOffset = b - mm->buffer->allocation.text;
                return MARKER_IS_IN_SLASH_COMMENT;
            }
        } else if (*s == '"') {
            // string, pass it removing all inside (also /**/ comments)
            s++;
            while (s < e && *s != '"') {
                s++;
                if (*s == '\\') {
                    s++;
                    s++;
                }
            }
            if (s < e)
                s++;
        } else {
            s++;
        }
    }
    return MARKER_IS_IN_CODE;
}

static void moveMarkerToTheBeginOfDefinitionScope(EditorMarker *mm) {
    int theBeginningOffset, comBeginOffset, mp;
    int slashedCommentsProcessed, staredCommentsProcessed;

    slashedCommentsProcessed = staredCommentsProcessed = 0;
    for (;;) {
        theBeginningOffset = mm->offset;
        mm->offset--;
        moveEditorMarkerToNonBlankOrNewline(mm, -1);
        if (CHAR_ON_MARKER(mm) == '\n') {
            theBeginningOffset = mm->offset + 1;
            mm->offset--;
        }
        if (refactoringOptions.commentMovingMode == CM_NO_COMMENT)
            goto fini;
        moveEditorMarkerToNonBlank(mm, -1);
        mp = markerWRTComment(mm, &comBeginOffset);
        if (mp == MARKER_IS_IN_CODE)
            goto fini;
        else if (mp == MARKER_IS_IN_STAR_COMMENT) {
            if (refactoringOptions.commentMovingMode == CM_SINGLE_SLASHED)
                goto fini;
            if (refactoringOptions.commentMovingMode == CM_ALL_SLASHED)
                goto fini;
            if (staredCommentsProcessed > 0 && refactoringOptions.commentMovingMode == CM_SINGLE_STARRED)
                goto fini;
            if (staredCommentsProcessed > 0 &&
                refactoringOptions.commentMovingMode == CM_SINGLE_SLASHED_AND_STARRED)
                goto fini;
            staredCommentsProcessed++;
            mm->offset = comBeginOffset;
        }
        // slash comment, skip them all
        else if (mp == MARKER_IS_IN_SLASH_COMMENT) {
            if (refactoringOptions.commentMovingMode == CM_SINGLE_STARRED)
                goto fini;
            if (refactoringOptions.commentMovingMode == CM_ALL_STARRED)
                goto fini;
            if (slashedCommentsProcessed > 0 && refactoringOptions.commentMovingMode == CM_SINGLE_SLASHED)
                goto fini;
            if (slashedCommentsProcessed > 0 &&
                refactoringOptions.commentMovingMode == CM_SINGLE_SLASHED_AND_STARRED)
                goto fini;
            slashedCommentsProcessed++;
            mm->offset = comBeginOffset;
        } else {
            warningMessage(ERR_INTERNAL, "A new comment?");
            goto fini;
        }
    }
fini:
    mm->offset = theBeginningOffset;
}

static void getFunctionBoundariesForMoving(EditorMarker *point, EditorMarker **methodStartP,
                                           EditorMarker **methodEndP) {
    EditorMarker *mstart, *mend;

    parsedPositions[IPP_FUNCTION_BEGIN].file = parsedPositions[IPP_FUNCTION_END].file = NO_FILE_NUMBER;

    // get function boundaries
    parseBufferUsingServer(refactoringOptions.project, point, NULL, "-olcxgetfunctionbounds", NULL);

    if (parsedPositions[IPP_FUNCTION_BEGIN].file == NO_FILE_NUMBER || parsedPositions[IPP_FUNCTION_END].file == NO_FILE_NUMBER) {
        FATAL_ERROR(ERR_INTERNAL, "Can't find declaration coordinates", XREF_EXIT_ERR);
    }

    mstart = newEditorMarkerForPosition(parsedPositions[IPP_FUNCTION_BEGIN]);
    mend   = newEditorMarkerForPosition(parsedPositions[IPP_FUNCTION_BEGIN + 1]);

    moveMarkerToTheBeginOfDefinitionScope(mstart);
    moveMarkerToTheEndOfDefinitionScope(mend);

    assert(mstart->buffer == mend->buffer);
    *methodStartP = mstart;
    *methodEndP   = mend;
}

static EditorMarker *getTargetFromOptions(void) {
    EditorMarker *target;
    EditorBuffer *tb;
    int           tline;

    tb = findOrCreateAndLoadEditorBufferForFile(
        normalizeFileName_static(refactoringOptions.moveTargetFile, cwd));
    if (tb == NULL)
        FATAL_ERROR(ERR_ST, "Could not find a buffer for target position", XREF_EXIT_ERR);
    target = newEditorMarker(tb, 0);
    sscanf(refactoringOptions.refactor_target_line, "%d", &tline);
    moveEditorMarkerToLineAndColumn(target, tline, 0);
    return target;
}

static bool validTargetPlace(EditorMarker *target, char *checkOpt) {
    bool valid = true;

    parseBufferUsingServer(refactoringOptions.project, target, NULL, checkOpt, NULL);
    if (!parsedInfo.moveTargetAccepted) {
        valid = false;
        errorMessage(ERR_ST, "Invalid target place");
    }
    return valid;
}

EditorMarker *removeStaticPrefix(EditorMarker *marker) {
    EditorMarker *startMarker;

    startMarker = createMarkerForExpressionStart(marker, GET_STATIC_PREFIX_START);
    if (startMarker == NULL) {
        // this is an error, this is just to avoid possible core dump in the future
        startMarker = newEditorMarker(marker->buffer, marker->offset);
    } else {
        int savedOffset = startMarker->offset;
        removeNonCommentCode(startMarker, marker->offset - startMarker->offset);
        // return it back to beginning of name(?)
        startMarker->offset = savedOffset;
    }
    return startMarker;
}

static void moveStaticFunctionAndMakeItExtern(EditorMarker *startMarker, EditorMarker *point,
                                              EditorMarker *endMarker, EditorMarker *target,
                                              ToCheckOrNot check, int limitIndex) {
    int size = endMarker->offset - startMarker->offset;
    if (target->buffer == startMarker->buffer && target->offset > startMarker->offset &&
        target->offset < startMarker->offset + size) {
        ppcGenRecord(PPC_INFORMATION, "You can't move something into itself.");
        return;
    }

    /* Check if function has "static" keyword and remove it when moving between files.
     * When moving within the same file, keep static (visibility doesn't change).
     * For Phase 1 MVP: we just remove static, user manually handles extern/headers. */
    bool movingBetweenFiles = (startMarker->buffer != target->buffer);
    EditorMarker *searchMarker = newEditorMarker(startMarker->buffer, startMarker->offset);
    bool foundStatic = false;
    EditorMarker *staticMarker = NULL;

    if (movingBetweenFiles) {
        while (searchMarker->offset < point->offset) {
            char *text = &startMarker->buffer->allocation.text[searchMarker->offset];
            int remaining = point->offset - searchMarker->offset;

            /* Check if we're at "static " (with space or tab after) */
            if (remaining >= 7 && strncmp(text, "static", 6) == 0 &&
                (text[6] == ' ' || text[6] == '\t')) {
                foundStatic = true;
                staticMarker = newEditorMarker(startMarker->buffer, searchMarker->offset);
                break;
            }
            searchMarker->offset++;
        }
    }
    freeEditorMarker(searchMarker);

    /* Remove "static " keyword BEFORE moving (only when moving between files) */
    if (foundStatic) {
        replaceStringInEditorBuffer(staticMarker->buffer, staticMarker->offset, 7, "", 0, &editorUndo);
        freeEditorMarker(staticMarker);
        /* Adjust size since we removed 7 characters */
        size -= 7;
    }

    /* Now move the (possibly modified) function block */
    moveBlockInEditorBuffer(startMarker, target, size, &editorUndo);
}

static void moveFunction(EditorMarker *point) {
    EditorMarker *target = getTargetFromOptions();

    if (!validTargetPlace(target, "-olcxmovetarget"))
        return;

    /* Test: Prove parsing module links correctly */
    ParseConfig config = createParseConfigFromOptions();
    (void)config; /* Unused for now */

    ensureReferencesAreUpdated(refactoringOptions.project);

    EditorMarker *functionStart, *functionEnd;
    getFunctionBoundariesForMoving(point, &functionStart, &functionEnd);
    int lines = countLinesBetweenEditorMarkers(functionStart, functionEnd);

    // O.K. Now STARTING!
    moveStaticFunctionAndMakeItExtern(functionStart, point, functionEnd, target, APPLY_CHECKS, IPP_FUNCTION_BEGIN);

    // and generate output
    applyWholeRefactoringFromUndo();
    ppcGotoMarker(point);
    ppcValueRecord(PPC_INDENT, lines, "");
}
// ------------------------------------------------------ Extract

static void extractFunction(EditorMarker *point, EditorMarker *mark) {
    parseBufferUsingServer(refactoringOptions.project, point, mark, "-olcxextract", NULL);
}

static void extractMacro(EditorMarker *point, EditorMarker *mark) {
    parseBufferUsingServer(refactoringOptions.project, point, mark, "-olcxextract", "-olexmacro");
}

static void extractVariable(EditorMarker *point, EditorMarker *mark) {
    parseBufferUsingServer(refactoringOptions.project, point, mark, "-olcxextract", "-olexvariable");
}

static char *computeUpdateOptionForSymbol(EditorMarker *point) {
    int               fileNumber;
    char             *selectedUpdateOption;

    assert(point != NULL && point->buffer != NULL);
    currentLanguage = getLanguageFor(point->buffer->fileName);

    bool hasHeaderReferences = false;
    bool isMultiFileReferences = false;
    EditorMarkerList *markerList = getReferences(point, NULL, PPCV_BROWSER_TYPE_WARNING);
    BrowserMenu *menu = sessionData.browsingStack.top->hkSelectedSym;
    Scope scope = menu->referenceable.scope;
    Visibility visibility = menu->referenceable.visibility;

    if (markerList == NULL) {
        fileNumber = NO_FILE_NUMBER;
    } else {
        assert(markerList->marker != NULL && markerList->marker->buffer != NULL);
        fileNumber = markerList->marker->buffer->fileNumber;
    }
    for (EditorMarkerList *l = markerList; l != NULL; l = l->next) {
        assert(l->marker != NULL && l->marker->buffer != NULL);
        FileItem *fileItem = getFileItemWithFileNumber(l->marker->buffer->fileNumber);
        if (fileNumber != l->marker->buffer->fileNumber) {
            isMultiFileReferences = true;
        }
        if (!fileItem->isArgument) {
            hasHeaderReferences = true;
        }
    }

    if (visibility == VisibilityLocal) {
        // useless to update when there is nothing about the symbol in Tags
        selectedUpdateOption = "";
    } else if (hasHeaderReferences) {
        // once it is in a header, full update is required
        selectedUpdateOption = "-update";
    } else if (scope == AutoScope || scope == FileScope) {
        // for example a local var or a static function not used in any header
        if (isMultiFileReferences) {
            errorMessage(ERR_INTERNAL, "something went wrong, a local symbol is used in several files");
            selectedUpdateOption = "-update";
        } else {
            selectedUpdateOption = "";
        }
    } else if (!isMultiFileReferences) {
        // this is a little bit tricky. It may provoke a bug when
        // a new external function is not yet indexed, but used in another file.
        // But it is so practical, so take the risk.
        selectedUpdateOption = "";
    } else {
        // may seems too strong, but implicitly linked global functions
        // requires this (for example).
        selectedUpdateOption = "-fastupdate";
    }

    freeEditorMarkerListAndMarkers(markerList);
    markerList = NULL;
    popFromSession();

    return selectedUpdateOption;
}

// --------------------------------------------------------------------

/* Main entry point for refactoring operations (-refactory mode).
 *
 * This function is called when c-xref is invoked with the -refactory option.
 * It runs as a separate process from the main editor server and handles the
 * complete lifecycle of a refactoring operation.
 *
 * PROCESS LIFECYCLE:
 * 1. Performs initial refactoring operation (rename, extract, parameter manipulation, etc.)
 * 2. If user interaction is needed (e.g., name collisions, symbol selection), enters
 *    beInteractive() which waits for piped commands from the editor
 * 3. When -continuerefactoring is received, exits interactive mode
 * 4. Completes the refactoring and THE PROCESS EXITS AUTOMATICALLY
 *
 * IMPORTANT: The refactory process always exits when this function returns. Tests and
 * editor clients should NOT send an explicit <exit> command after -continuerefactoring,
 * as the process will have already terminated (causing a race condition in tests).
 *
 * The function never returns to a command loop - it's a one-shot operation that either:
 * - Completes immediately (for simple refactorings)
 * - Enters beInteractive() for user decisions, then completes
 * - Errors out via FATAL_ERROR
 */
void refactory(void) {

    ENTER();

    if (options.project == NULL) {
        FATAL_ERROR(ERR_ST, "You have to specify active project with -p option", XREF_EXIT_ERR);
    }

    deepCopyOptionsFromTo(&options, &refactoringOptions); // save command line options !!!!
    // in general in this file:
    //   'refactoringOptions' are options passed to c-xrefactory
    //   'options' are options valid for interactive edit-server 'sub-task'
    deepCopyOptionsFromTo(&options, &savedOptions);

    // MAGIC, set the server operation to anything that just refreshes
    // or generates xrefs since we will be calling the "main task"
    // below
    refactoringOptions.serverOperation = OLO_LIST;

    openOutputFile(refactoringOptions.outputFileName);
    loadAllOpenedEditorBuffers();
    // initialise lastQuasySaveTime
    quasiSaveModifiedEditorBuffers();


    int argCount = 0;
    char *argumentFile = getNextScheduledFile(&argCount);

    if (argumentFile == NULL)
        FATAL_ERROR(ERR_ST, "no input file", XREF_EXIT_ERR);

    char inputFileName[MAX_FILE_NAME_SIZE];
    strcpy(inputFileName, argumentFile);
    char *file = inputFileName;
    EditorBuffer *buf = findOrCreateAndLoadEditorBufferForFile(file);

    EditorMarker *point = getPointFromOptions(buf);
    EditorMarker *mark  = getMarkFromOptions(buf);

    refactoringStartingPoint = editorUndo;

    // init subtask
    ArgumentsVector args = {.argc = argument_count(serverStandardOptions), .argv = serverStandardOptions};
    mainTaskEntryInitialisations(args);
    editServerSubTaskFirstPass = true;

    progressFactor = 1;

    switch (refactoringOptions.theRefactoring) {
    case AVR_RENAME_SYMBOL:
        progressFactor = 3;
        updateOption   = computeUpdateOptionForSymbol(point);
        renameAtPoint(point);
        break;
    case AVR_RENAME_MODULE:
    case AVR_RENAME_INCLUDED_FILE:
        progressFactor = 2;
        updateOption   = computeUpdateOptionForSymbol(point);
        renameAtInclude(point);
        break;
    case AVR_ADD_PARAMETER:
    case AVR_DEL_PARAMETER:
    case AVR_MOVE_PARAMETER:
        progressFactor = 3;
        updateOption   = computeUpdateOptionForSymbol(point);
        currentLanguage = getLanguageFor(file);
        parameterManipulation(point, refactoringOptions.theRefactoring, refactoringOptions.parnum,
                              refactoringOptions.parnum2);
        break;
    case AVR_MOVE_FUNCTION:
        progressFactor = 2;  /* Simple move without multi-step progress */
        moveFunction(point);
        break;
    case AVR_EXTRACT_FUNCTION:
        progressFactor = 1;
        extractFunction(point, mark);
        break;
    case AVR_EXTRACT_MACRO:
        progressFactor = 1;
        extractMacro(point, mark);
        break;
    case AVR_EXTRACT_VARIABLE:
        progressFactor = 1;
        extractVariable(point, mark);
        break;
    default:
        errorMessage(ERR_INTERNAL, "unknown refactoring");
        break;
    }

    // always finish once more time
    writeRelativeProgress(0);
    writeRelativeProgress(100);

    if (progressOffset != progressFactor) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "progressOffset (%d) != progressFactor (%d)", progressOffset, progressFactor);
        ppcGenRecord(PPC_DEBUG_INFORMATION, tmpBuff);
    }

    // synchronisation, wait so files won't be saved with the same time
    quasiSaveModifiedEditorBuffers();

    closeOutputFile();
    ppcSynchronize();

    // exiting, put undefined, so that main will finish
    options.mode = UndefinedMode;

    LEAVE();
}
