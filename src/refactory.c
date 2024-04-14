#include "refactory.h"

/* Main is currently needed for:
   mainTaskEntryInitialisations
   mainOpenOutputFile
 */
#include "classhierarchy.h"
#include "commons.h"
#include "cxfile.h"
#include "cxref.h"
#include "editor.h"
#include "filetable.h"
#include "globals.h"
#include "head.h"
#include "jsemact.h"
#include "list.h"
#include "main.h"
#include "misc.h"
#include "options.h"
#include "ppc.h"
#include "progress.h"
#include "proto.h"
#include "protocol.h"
#include "refactorings.h"
#include "reftab.h"
#include "server.h"
#include "undo.h"
#include "xref.h"

#include "log.h"

#define RRF_CHARS_TO_PRE_CHECK_AROUND 1
#define MAX_NARGV_OPTIONS_COUNT 50


typedef struct tpCheckMoveClassData {
    PushRange pushRange;
    bool      transPackageMove;
    char      *sourceClass;
} TpCheckMoveClassData;

typedef struct tpCheckSpecialReferencesData {
    PushRange            pushRange;
    char                 *symbolToTest;
    int                  classToTest;
    struct referenceItem *foundSpecialRefItem;
    struct reference     *foundSpecialR;
    struct referenceItem *foundRefToTestedClass;
    struct referenceItem *foundRefNotToTestedClass;
    struct reference     *foundOuterScopeRef;
} TpCheckSpecialReferencesData;

typedef struct disabledList {
    int                  file;
    int                  clas;
    struct disabledList *next;
} DisabledList;

static EditorUndo *refactoringStartingPoint;

static bool editServerSubTaskFirstPass = true;

static char *editServInitOptions[] = {
    "xref",
    "-xrefactory-II",
    //& "-debug",
    "-server",
    NULL,
};

// Refactory will always use xref2 protocol and inhibit a few messages when generating/updating xrefs
static char *initOptionsForReferencesUpdate[] = {
    "xref",
    "-xrefactory-II",
    "-briefoutput",
    NULL,
};

static Options refactoringOptions;

static char *updateOption = "-fastupdate";

static void javaDotifyClassName(char *ss) {
    char *s;
    for (s = ss; *s; s++) {
        if (*s == '/' || *s == '\\' || *s == '$')
            *s = '.';
    }
}

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
        argv[i] = point->buffer->name;
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
    int refactoryXrefInitOptionsCount;

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
    refactoryXrefInitOptionsCount = argument_count(initOptionsForReferencesUpdate);
    for (int i = 1; i < refactoryXrefInitOptionsCount; i++) {
        argumentVector[argumentCount++] = initOptionsForReferencesUpdate[i];
    }
    argumentVector[argumentCount++] = updateOption;

    currentPass = ANY_PASS;
    mainTaskEntryInitialisations(argumentCount, argumentVector);

    callXref(argumentCount, argumentVector, true);

    deepCopyOptionsFromTo(&savedOptions, &options);
    ppcEnd(PPC_UPDATE_REPORT);

    // return into editSubTaskState
    mainTaskEntryInitialisations(argument_count(editServInitOptions), editServInitOptions);
    editServerSubTaskFirstPass = true;
}

static void parseBufferUsingServer(char *project, EditorMarker *point, EditorMarker *mark,
                                  char *pushOption, char *pushOption2) {
    int   argumentCount;
    char *argumentVector[MAX_NARGV_OPTIONS_COUNT];

    currentPass = ANY_PASS;

    assert(options.mode == ServerMode);

    setArguments(argumentVector, project, point, mark);
    argumentCount = argument_count(argumentVector);
    if (pushOption != NULL) {
        argumentVector[argumentCount++] = pushOption;
    }
    if (pushOption2 != NULL) {
        argumentVector[argumentCount++] = pushOption2;
    }
    initServer(argumentCount, argumentVector);
    callServer(argument_count(editServInitOptions), editServInitOptions, argumentCount, argumentVector,
               &editServerSubTaskFirstPass);
}

static void beInteractive(void) {
    int    argumentCount;
    char **argumentVectorP;

    ENTER();
    deepCopyOptionsFromTo(&options, &savedOptions);
    for (;;) {
        closeMainOutputFile();
        ppcSynchronize();
        deepCopyOptionsFromTo(&savedOptions, &options);
        processOptions(argument_count(editServInitOptions), editServInitOptions, DONT_PROCESS_FILE_ARGUMENTS);
        getPipedOptions(&argumentCount, &argumentVectorP);
        openOutputFile(refactoringOptions.outputFileName);
        if (argumentCount <= 1)
            break;
        initServer(argumentCount, argumentVectorP);
        if (options.continueRefactoring != RC_NONE)
            break;
        callServer(argument_count(editServInitOptions), editServInitOptions, argumentCount, argumentVectorP,
                   &editServerSubTaskFirstPass);
        answerEditAction();
    }
    LEAVE();
}

// -------------------- end of interface to edit server sub-task ----------------------
////////////////////////////////////////////////////////////////////////////////////////

static void displayResolutionDialog(char *message, int messageType, int continuation) {
    char buf[TMP_BUFF_SIZE];
    strcpy(buf, message);
    formatOutputLine(buf, ERROR_MESSAGE_STARTING_OFFSET);
    ppcDisplaySelection(buf, messageType, continuation);
    beInteractive();
}

#define STANDARD_SELECT_SYMBOLS_MESSAGE                                                                           \
    "Select classes in left window. These classes will be processed during refactoring. It is highly "            \
    "recommended to process whole hierarchy of related classes all at once. Unselection of any class and its "    \
    "exclusion from refactoring may cause changes in your program behaviour."
#define STANDARD_C_SELECT_SYMBOLS_MESSAGE                                                                         \
    "There are several symbols referred from this place. Continuing this refactoring will process the selected "  \
    "symbols all at once."
#define ERROR_SELECT_SYMBOLS_MESSAGE                                                                              \
    "If you see this message, then probably something is going wrong. You are refactoring a virtual method when " \
    "only statically linked symbol is required. It is strongly recommended to cancel the refactoring."

static void pushReferences(EditorMarker *point, char *pushOption, char *resolveMessage,
                           int messageType) {
    /* now remake task initialisation as for edit server */
    parseBufferUsingServer(refactoringOptions.project, point, NULL, pushOption, NULL);

    assert(sessionData.browserStack.top != NULL);
    if (sessionData.browserStack.top->hkSelectedSym == NULL) {
        errorMessage(ERR_INTERNAL, "no symbol found for refactoring push");
    }
    olCreateSelectionMenu(sessionData.browserStack.top->command);
    if (resolveMessage != NULL && olcxShowSelectionMenu()) {
        displayResolutionDialog(resolveMessage, messageType, CONTINUATION_ENABLED);
    }
}

static void safetyCheck(char *project, EditorMarker *point) {
    // !!!!update references MUST be followed by a pushing action, to refresh options
    ensureReferencesAreUpdated(refactoringOptions.project);
    parseBufferUsingServer(project, point, NULL, "-olcxsafetycheck2", NULL);

    assert(sessionData.browserStack.top != NULL);
    if (sessionData.browserStack.top->hkSelectedSym == NULL) {
        errorMessage(ERR_ST, "No symbol found for refactoring safety check");
    }
    olCreateSelectionMenu(sessionData.browserStack.top->command);
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

static void replaceString(EditorMarker *marker, int len, char *newString) {
    replaceStringInEditorBuffer(marker->buffer, marker->offset, len, newString,
                                strlen(newString), &editorUndo);
}

static void checkedReplaceString(EditorMarker *pos, int len, char *oldVal, char *newVal) {
    char *bVal;
    int   check, d;

    bVal  = pos->buffer->allocation.text + pos->offset;
    check = (strlen(oldVal) == len && strncmp(oldVal, bVal, len) == 0);
    if (check) {
        replaceString(pos, len, newVal);
    } else {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "checked replacement of %s to %s failed on ", oldVal, newVal);
        d = strlen(tmpBuff);
        for (int i = 0; i < len; i++)
            tmpBuff[d++] = bVal[i];
        tmpBuff[d++] = 0;
        errorMessage(ERR_INTERNAL, tmpBuff);
    }
}

// -------------------------- Undos

static void editorFreeSingleUndo(EditorUndo *uu) {
    if (uu->u.replace.str != NULL && uu->u.replace.strlen != 0) {
        switch (uu->operation) {
        case UNDO_REPLACE_STRING:
            editorFree(uu->u.replace.str, uu->u.replace.strlen + 1);
            break;
        case UNDO_RENAME_BUFFER:
            editorFree(uu->u.rename.name, strlen(uu->u.rename.name) + 1);
            break;
        case UNDO_MOVE_BLOCK:
            break;
        default:
            errorMessage(ERR_INTERNAL, "Unknown operation to undo");
        }
    }
    editorFree(uu, sizeof(EditorUndo));
}

static void editorApplyUndos(EditorUndo *undos, EditorUndo *until, EditorUndo **undoundo, int gen) {
    EditorUndo   *uu, *next;
    EditorMarker *m1, *m2;
    uu = undos;
    while (uu != until && uu != NULL) {
        switch (uu->operation) {
        case UNDO_REPLACE_STRING:
            if (gen == GEN_FULL_OUTPUT) {
                ppcReplace(uu->buffer->name, uu->u.replace.offset,
                           uu->buffer->allocation.text + uu->u.replace.offset, uu->u.replace.size,
                           uu->u.replace.str);
            }
            replaceStringInEditorBuffer(uu->buffer, uu->u.replace.offset, uu->u.replace.size,
                                        uu->u.replace.str, uu->u.replace.strlen, undoundo);

            break;
        case UNDO_RENAME_BUFFER:
            if (gen == GEN_FULL_OUTPUT) {
                ppcGotoOffsetPosition(uu->buffer->name, 0);
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
            moveBlockInEditorBuffer(m2, m1, uu->u.moveBlock.size, undoundo);
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

static void removeNonCommentCode(EditorMarker *m, int len) {
    int           c, nn, n;
    char         *s;
    EditorMarker *mm;
    assert(m->buffer && m->buffer->allocation.text);
    s  = m->buffer->allocation.text + m->offset;
    nn = len;
    mm = newEditorMarker(m->buffer, m->offset);
    if (m->offset + nn > m->buffer->allocation.bufferSize) {
        nn = m->buffer->allocation.bufferSize - m->offset;
    }
    n = 0;
    while (nn > 0) {
        c = *s;
        if (c == '/' && nn > 1 && *(s + 1) == '*' && (nn <= 2 || *(s + 2) != '&')) {
            // /**/ comment
            replaceString(mm, n, "");
            s = mm->buffer->allocation.text + mm->offset;
            s += 2;
            nn -= 2;
            while (!(*s == '*' && *(s + 1) == '/')) {
                s++;
                nn--;
            }
            s += 2;
            nn -= 2;
            mm->offset = s - mm->buffer->allocation.text;
            n          = 0;
        } else if (c == '/' && nn > 1 && *(s + 1) == '/' && (nn <= 2 || *(s + 2) != '&')) {
            // // comment
            replaceString(mm, n, "");
            s = mm->buffer->allocation.text + mm->offset;
            s += 2;
            nn -= 2;
            while (*s != '\n') {
                s++;
                nn--;
            }
            s += 1;
            nn -= 1;
            mm->offset = s - mm->buffer->allocation.text;
            n          = 0;
        } else if (c == '"') {
            // string, pass it removing all inside (also /**/ comments)
            s++;
            nn--;
            n++;
            while (*s != '"' && nn > 0) {
                s++;
                nn--;
                n++;
                if (*s == '\\') {
                    s++;
                    nn--;
                    n++;
                    s++;
                    nn--;
                    n++;
                }
            }
        } else {
            s++;
            nn--;
            n++;
        }
    }
    if (n > 0) {
        replaceString(mm, n, "");
    }
    freeEditorMarker(mm);
}

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
        editorMoveMarkerToNonBlank(mm, -1);
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

static void pushMarkersAsReferences(EditorMarkerList **markers, OlcxReferences *refs, char *name) {
    Reference *rr;

    rr = convertEditorMarkersToReferences(markers);
    for (SymbolsMenu *mm = refs->menuSym; mm != NULL; mm = mm->next) {
        if (strcmp(mm->references.linkName, name) == 0) {
            for (Reference *r = rr; r != NULL; r = r->next) {
                olcxAddReference(&mm->references.references, r, 0);
            }
        }
    }
    freeReferences(rr);
    olcxRecomputeSelRefs(refs);
}

static bool validTargetPlace(EditorMarker *target, char *checkOpt) {
    bool valid = true;

    parseBufferUsingServer(refactoringOptions.project, target, NULL, checkOpt, NULL);
    if (!parsedInfo.moveTargetApproved) {
        valid = false;
        errorMessage(ERR_ST, "Invalid target place");
    }
    return valid;
}

// ------------------------- Trivial prechecks --------------------------------------

static void askForReallyContinueConfirmation(void) {
    ppcAskConfirmation("The refactoring may change program behaviour, really continue?");
}

// ---------------------------------------------------------------------------------

static bool handleSafetyCheckDifferenceLists(EditorMarkerList *diff1, EditorMarkerList *diff2,
                                             OlcxReferences *diffrefs) {
    if (diff1 != NULL || diff2 != NULL) {
        //&editorDumpMarkerList(diff1);
        //&editorDumpMarkerList(diff2);
        for (SymbolsMenu *mm = diffrefs->menuSym; mm != NULL; mm = mm->next) {
            mm->selected = true;
            mm->visible  = true;
            mm->ooBits   = 07777777;
            // hack, freeing now all diffs computed by old method
            freeReferences(mm->references.references);
            mm->references.references = NULL;
        }
        //&editorDumpMarkerList(diff1);
        //& safetyCheckFailPrepareRefStack();
        //& pushMarkersAsReferences(&diff1, diffrefs, LINK_NAME_SAFETY_CHECK_LOST);
        //& pushMarkersAsReferences(&diff2, diffrefs, LINK_NAME_SAFETY_CHECK_FOUND);
        pushMarkersAsReferences(&diff1, diffrefs, LINK_NAME_SAFETY_CHECK_MISSED);
        pushMarkersAsReferences(&diff2, diffrefs, LINK_NAME_SAFETY_CHECK_MISSED);
        freeEditorMarkerListButNotMarkers(diff1);
        freeEditorMarkerListButNotMarkers(diff2);
        olcxPopOnly();
        if (refactoringOptions.theRefactoring == AVR_RENAME_PACKAGE) {
            displayResolutionDialog("The package exists and is referenced in the original project. Renaming will "
                                    "join two packages without possibility of inverse refactoring",
                                    PPCV_BROWSER_TYPE_WARNING, CONTINUATION_ENABLED);
        } else {
            displayResolutionDialog("These references may be misinterpreted after refactoring",
                                    PPCV_BROWSER_TYPE_WARNING, CONTINUATION_ENABLED);
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
    OlcxReferences   *refs, *origrefs, *newrefs, *diffrefs;
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

    chks = convertReferencesToEditorMarkers(sessionData.browserStack.top->references);

    editorMarkersDifferences(occs, &chks, &diff1, &diff2);

    freeEditorMarkersAndMarkerList(chks);

    editorUndoUntil(startPoint, redoTrack);

    origrefs = newrefs = diffrefs = NULL;
    SAFETY_CHECK2_GET_SYM_LISTS(refs, origrefs, newrefs, diffrefs, pbflag);
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
                    simpleFileName(getRealFileName_static(pos->buffer->name)), pos->offset);
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

static void makeSyntaxPassOnSource(EditorMarker *point) {
    parseBufferUsingServer(refactoringOptions.project, point, NULL, "-olcxsyntaxpass", NULL);
    olStackDeleteSymbol(sessionData.browserStack.top);
}

static EditorMarker *createNewMarkerForExpressionStart(EditorMarker *marker, int kind) {
    Position *pos;
    parseBufferUsingServer(refactoringOptions.project, marker, NULL, "-olcxprimarystart", NULL);
    olStackDeleteSymbol(sessionData.browserStack.top);
    if (kind == GET_PRIMARY_START) {
        pos = &s_primaryStartPosition;
    } else if (kind == GET_STATIC_PREFIX_START) {
        pos = &s_staticPrefixStartPosition;
    } else {
        pos = NULL;
        assert(0);
    }
    if (pos->file == NO_FILE_NUMBER) {
        if (kind == GET_STATIC_PREFIX_START) {
            fatalErrorOnPosition(marker, ERR_ST,
                                 "Can't determine static prefix. Maybe non-static reference to a static object? "
                                 "Make this invocation static before refactoring.");
        } else {
            fatalErrorOnPosition(marker, ERR_INTERNAL, "Can't determine beginning of primary expression");
        }
        return NULL;
    } else {
        EditorBuffer *buffer    = getOpenedAndLoadedEditorBuffer(getFileItem(pos->file)->name);
        EditorMarker *newMarker = newEditorMarker(buffer, 0);
        moveEditorMarkerToLineAndColumn(newMarker, pos->line, pos->col);
        assert(newMarker->buffer == marker->buffer);
        assert(newMarker->offset <= marker->offset);
        return newMarker;
    }
}

static void checkedRenameBuffer(EditorBuffer *buff, char *newName, EditorUndo **undo) {
    struct stat stat;
    if (editorFileStatus(newName, &stat) == 0) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "Renaming buffer %s to an existing file.\nCan I do this?", buff->name);
        ppcAskConfirmation(tmpBuff);
    }
    renameEditorBuffer(buff, newName, undo);
}

static void javaSlashifyDotName(char *ss) {
    char *s;
    for (s = ss; *s; s++) {
        if (*s == '.')
            *s = FILE_PATH_SEPARATOR;
    }
}

static void moveFileAndDirForPackageRename(char *currentPath, EditorMarker *lld, char *symLinkName) {
    char newfile[2 * MAX_FILE_NAME_SIZE];
    char packdir[2 * MAX_FILE_NAME_SIZE];
    char newpackdir[2 * MAX_FILE_NAME_SIZE];
    char path[MAX_FILE_NAME_SIZE];
    int  plen;
    strcpy(path, currentPath);
    plen = strlen(path);
    if (plen > 0 && (path[plen - 1] == '/' || path[plen - 1] == '\\')) {
        plen--;
        path[plen] = 0;
    }
    sprintf(packdir, "%s%c%s", path, FILE_PATH_SEPARATOR, symLinkName);
    sprintf(newpackdir, "%s%c%s", path, FILE_PATH_SEPARATOR, refactoringOptions.renameTo);
    javaSlashifyDotName(newpackdir + strlen(path));
    sprintf(newfile, "%s%s", newpackdir, lld->buffer->name + strlen(packdir));
    checkedRenameBuffer(lld->buffer, newfile, &editorUndo);
}

static bool renamePackageFileMove(char *currentPath, EditorMarkerList *ll, char *symLinkName, int slnlen) {
    int  pathLength;
    bool res = false;

    pathLength = strlen(currentPath);
    log_trace("checking %s<->%s, %s<->%s", ll->marker->buffer->name, currentPath,
              ll->marker->buffer->name + pathLength + 1, symLinkName);
    if (filenameCompare(ll->marker->buffer->name, currentPath, pathLength) == 0 &&
        ll->marker->buffer->name[pathLength] == FILE_PATH_SEPARATOR &&
        filenameCompare(ll->marker->buffer->name + pathLength + 1, symLinkName, slnlen) == 0) {
        moveFileAndDirForPackageRename(currentPath, ll->marker, symLinkName);
        res = true;
        goto fini;
    }
fini:
    return res;
}

static void simplePackageRename(EditorMarkerList *occs, char *symname, char *symLinkName) {
    char          rtpack[MAX_FILE_NAME_SIZE];
    char          rtprefix[MAX_FILE_NAME_SIZE];
    char         *ss;
    int           snlen, slnlen;
    bool          mvfile;
    EditorMarker *pp;

    // get original and new directory, but how?
    snlen  = strlen(symname);
    slnlen = strlen(symLinkName);
    strcpy(rtprefix, refactoringOptions.renameTo);
    ss = lastOccurenceInString(rtprefix, '.');
    if (ss == NULL) {
        strcpy(rtpack, rtprefix);
        rtprefix[0] = 0;
    } else {
        strcpy(rtpack, ss + 1);
        *(ss + 1) = 0;
    }
    for (EditorMarkerList *ll = occs; ll != NULL; ll = ll->next) {
        pp = createNewMarkerForExpressionStart(ll->marker, GET_STATIC_PREFIX_START);
        if (pp != NULL) {
            removeNonCommentCode(pp, ll->marker->offset - pp->offset);
            // make attention here, so that markers still points
            // to the package name, the best would be to replace
            // package name per single names, ...
            checkedReplaceString(pp, snlen, symname, rtpack);
            replaceString(pp, 0, rtprefix);
        }
        freeEditorMarker(pp);
    }
    for (EditorMarkerList *ll = occs; ll != NULL; ll = ll->next) {
        if (ll->next == NULL || ll->next->marker->buffer != ll->marker->buffer) {
            // O.K. verify whether I should move the file
            MapOverPaths(javaSourcePaths, {
                mvfile = renamePackageFileMove(currentPath, ll, symLinkName, slnlen);
                if (mvfile)
                    goto moved;
            });
        moved:;
        }
    }
}

static void simpleRename(EditorMarkerList *occs, EditorMarker *point, char *symname, char *symLinkName,
                         int symtype) {
    char  nfile[MAX_FILE_NAME_SIZE];
    char *ss;

    if (refactoringOptions.theRefactoring == AVR_RENAME_PACKAGE) {
        simplePackageRename(occs, symname, symLinkName);
    } else {
        for (EditorMarkerList *ll = occs; ll != NULL; ll = ll->next) {
            renameFromTo(ll->marker, symname, refactoringOptions.renameTo);
        }
        ppcGotoMarker(point);
        if (refactoringOptions.theRefactoring == AVR_RENAME_CLASS) {
            if (strcmp(simpleFileNameWithoutSuffix_st(point->buffer->name), symname) == 0) {
                // O.K. file name equals to class name, rename file
                strcpy(nfile, point->buffer->name);
                ss = lastOccurenceOfSlashOrBackslash(nfile);
                if (ss == NULL)
                    ss = nfile;
                else
                    ss++;
                sprintf(ss, "%s.java", refactoringOptions.renameTo);
                assert(strlen(nfile) < MAX_FILE_NAME_SIZE - 1);
                if (strcmp(nfile, point->buffer->name) != 0) {
                    // O.K. I should move file
                    checkedRenameBuffer(point->buffer, nfile, &editorUndo);
                }
            }
        }
    }
}

static EditorMarkerList *getReferences(EditorMarker *point, char *resolveMessage,
                                       int messageType) {
    EditorMarkerList *occs;
    pushReferences(point, "-olcxrename", resolveMessage, messageType); /* TODO: WTF do we use "rename"?!? */
    assert(sessionData.browserStack.top && sessionData.browserStack.top->hkSelectedSym);
    occs = convertReferencesToEditorMarkers(sessionData.browserStack.top->references);
    return occs;
}

static EditorMarkerList *pushGetAndPreCheckReferences(EditorMarker *point, char *nameOnPoint,
                                                      char *resolveMessage, int messageType) {
    EditorMarkerList *occs;
    occs = getReferences(point, resolveMessage, messageType);
    precheckThatSymbolRefsCorresponds(nameOnPoint, occs);
    return occs;
}

/* JAVA only */
static EditorMarker *findModifierAndCreateMarker(EditorMarker *point, char *modifier, int limitIndex) {
    int           i, mlen, blen, mini;
    char         *text;
    EditorMarker *mm, *mb, *me;

    text = point->buffer->allocation.text;
    blen = point->buffer->allocation.bufferSize;
    mlen = strlen(modifier);
    makeSyntaxPassOnSource(point);
    if (parsedPositions[limitIndex].file == NO_FILE_NUMBER) {
        warningMessage(ERR_INTERNAL, "cant get field declaration");
        mini = point->offset;
        while (mini > 0 && text[mini] != '\n')
            mini--;
        i = point->offset;
    } else {
        mb = newEditorMarkerForPosition(&parsedPositions[limitIndex]);
        // TODO, this limitIndex+2 should be done more semantically
        me   = newEditorMarkerForPosition(&parsedPositions[limitIndex + 2]);
        mini = mb->offset;
        i    = me->offset;
        freeEditorMarker(mb);
        freeEditorMarker(me);
    }
    while (i >= mini) {
        if (strncmp(text + i, modifier, mlen) == 0 && (i == 0 || isspace(text[i - 1])) &&
            (i + mlen == blen || isspace(text[i + mlen]))) {
            mm = newEditorMarker(point->buffer, i);
            return mm;
        }
        i--;
    }
    return NULL;
}

static void removeModifier(EditorMarker *point, int limitIndex, char *modifier) {
    int           i, j, mlen;
    char         *text;
    EditorMarker *mm;

    mlen = strlen(modifier);
    text = point->buffer->allocation.text;
    mm   = findModifierAndCreateMarker(point, modifier, limitIndex);
    if (mm != NULL) {
        i = mm->offset;
        for (j = i + mlen; isspace(text[j]); j++)
            ;
        replaceString(mm, j - i, "");
    }
    freeEditorMarker(mm);
}

/* JAVA only */
static void addModifier(EditorMarker *point, int limit, char *modifier) {
    char          modifSpace[TMP_STRING_SIZE];
    EditorMarker *mm;
    makeSyntaxPassOnSource(point);
    if (parsedPositions[limit].file == NO_FILE_NUMBER) {
        errorMessage(ERR_INTERNAL, "cant find beginning of field declaration");
    }
    mm = newEditorMarkerForPosition(&parsedPositions[limit]);
    sprintf(modifSpace, "%s ", modifier);
    replaceString(mm, 0, modifSpace);
    freeEditorMarker(mm);
}

static void changeAccessModifier(EditorMarker *point, int limitIndex, char *modifier) {
    EditorMarker *mm;
    mm = findModifierAndCreateMarker(point, modifier, limitIndex);
    if (mm == NULL) {
        removeModifier(point, limitIndex, "private");
        removeModifier(point, limitIndex, "protected");
        removeModifier(point, limitIndex, "public");
        if (*modifier)
            addModifier(point, limitIndex, modifier);
    } else {
        freeEditorMarker(mm);
    }
}

static void restrictAccessibility(EditorMarker *point, int limitIndex, int minAccess) {
    int accessIndex, access;

    minAccess &= ACCESS_PPP_MODIFER_MASK;
    for (accessIndex = 0; accessIndex < MAX_REQUIRED_ACCESS; accessIndex++) {
        if (javaRequiredAccessibilityTable[accessIndex] == minAccess)
            break;
    }

    // must update, because usualy they are out of date here
    ensureReferencesAreUpdated(refactoringOptions.project);

    pushReferences(point, "-olcxrename", NULL, 0);
    assert(sessionData.browserStack.top && sessionData.browserStack.top->menuSym);

    for (Reference *rr = sessionData.browserStack.top->references; rr != NULL; rr = rr->next) {
        if (!isDefinitionOrDeclarationUsage(rr->usage.kind)) {
            if (rr->usage.requiredAccess < accessIndex) {
                accessIndex = rr->usage.requiredAccess;
            }
        }
    }

    olcxPopOnly();

    access = javaRequiredAccessibilityTable[accessIndex];

    if (access == AccessPublic)
        changeAccessModifier(point, limitIndex, "public");
    else if (access == AccessProtected)
        changeAccessModifier(point, limitIndex, "protected");
    else if (access == AccessDefault)
        changeAccessModifier(point, limitIndex, "");
    else if (access == AccessPrivate)
        changeAccessModifier(point, limitIndex, "private");
    else
        errorMessage(ERR_INTERNAL, "No access modifier could be computed");
}

static void multipleReferencesInSamePlaceMessage(Reference *r) {
    char tmpBuff[TMP_BUFF_SIZE];
    ppcGotoPosition(&r->position);
    sprintf(tmpBuff, "The reference at this place refers to multiple symbols. The refactoring will probably "
                     "damage your program. Do you really want to continue?");
    formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
    ppcAskConfirmation(tmpBuff);
}

static void checkForMultipleReferencesInSamePlace(OlcxReferences *rstack, SymbolsMenu *ccms) {
    ReferenceItem *p, *sss;
    SymbolsMenu    *cms;
    bool            pushed;

    p = &ccms->references;
    assert(rstack && rstack->menuSym);
    sss    = &rstack->menuSym->references;
    pushed = itIsSymbolToPushOlReferences(p, rstack, &cms, DEFAULT_VALUE);
    // TODO, this can be simplified, as ccms == cms.
    log_trace(":checking %s to %s (%d)", p->linkName, sss->linkName, pushed);
    if (!pushed && olcxIsSameCxSymbol(p, sss)) {
        log_trace("checking %s references", p->linkName);
        for (Reference *r = p->references; r != NULL; r = r->next) {
            if (isReferenceInList(r, rstack->references)) {
                multipleReferencesInSamePlaceMessage(r);
            }
        }
    }
}

static void multipleOccurencesSafetyCheck(void) {
    OlcxReferences *rstack;

    rstack = sessionData.browserStack.top;
    olProcessSelectedReferences(rstack, checkForMultipleReferencesInSamePlace);
}

// -------------------------------------------- Rename

static void renameAtPoint(EditorMarker *point) {
    char              nameOnPoint[TMP_STRING_SIZE];
    char             *symLinkName, *message;
    Type              symtype;
    EditorMarkerList *occs;
    EditorUndo       *undoStartPoint, *redoTrack;
    SymbolsMenu      *csym;

    if (refactoringOptions.renameTo == NULL) {
        errorMessage(ERR_ST, "this refactoring requires -renameto=<new name> option");
    }

    ensureReferencesAreUpdated(refactoringOptions.project);

    message = STANDARD_C_SELECT_SYMBOLS_MESSAGE;

    // rename
    strcpy(nameOnPoint, getIdentifierOnMarker_static(point));
    assert(strlen(nameOnPoint) < TMP_STRING_SIZE - 1);
    occs           = pushGetAndPreCheckReferences(point, nameOnPoint, message, PPCV_BROWSER_TYPE_INFO);
    csym           = sessionData.browserStack.top->hkSelectedSym;
    symtype        = csym->references.type;
    symLinkName    = csym->references.linkName;
    undoStartPoint = editorUndo;

    multipleOccurencesSafetyCheck();

    simpleRename(occs, point, nameOnPoint, symLinkName, symtype);
    //&dumpEditorBuffers();
    redoTrack = NULL;
    if (!makeSafetyCheckAndUndo(point, &occs, undoStartPoint, &redoTrack)) {
        askForReallyContinueConfirmation();
    }

    editorApplyUndos(redoTrack, NULL, NULL, GEN_FULL_OUTPUT);

    // finish where you have started
    ppcGotoMarker(point);

    freeEditorMarkersAndMarkerList(occs); // O(n^2)!

    if (refactoringOptions.theRefactoring == AVR_RENAME_PACKAGE) {
        ppcGenRecord(PPC_INFORMATION, "\nDone.\nDo not forget to remove .class files of former package");
    } else if (refactoringOptions.theRefactoring == AVR_RENAME_CLASS &&
               strcmp(simpleFileNameWithoutSuffix_st(point->buffer->name), refactoringOptions.renameTo) == 0) {
        ppcGenRecord(PPC_INFORMATION, "\nDone.\nDo not forget to remove .class file of former class");
    }
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
    olcxPopOnly();
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
    if (!(strcmp(nameOnPoint, functionOrMacroName) == 0 || strcmp(nameOnPoint, "this") == 0 || strcmp(nameOnPoint, "super") == 0)) {
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
    olcxPopOnly();

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
static int addStringAsParameter(EditorMarker *point, EditorMarker *endMarkerOrMark, char *fileName, int argn,
                                char *parameterDeclaration) {
    char         *text;
    char          insertionText[REFACTORING_TMP_STRING_SIZE];
    char         *separator1, *separator2;
    int           insertionOffset;
    EditorMarker *beginMarker;

    insertionOffset = -1;
    Result rr = getParameterPosition(point, fileName, argn);
    if (rr != RESULT_OK) {
        errorMessage(ERR_INTERNAL, "Problem while adding parameter");
        return insertionOffset;
    }
    if (argn > parameterCount + 1) {
        errorMessage(ERR_ST, "Parameter number out of limits");
        return insertionOffset;
    }


    text = point->buffer->allocation.text;

    if (endMarkerOrMark == NULL) {
        beginMarker = newEditorMarkerForPosition(&parameterBeginPosition);
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
                    "Something goes wrong, probably different parameter coordinates at different cpp passes.");
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
    LIST_LEN(refn, Reference, sessionData.browserStack.top->references);
    olcxPopOnly();
    return refn > 1;
}

static int isParameterUsedExceptRecursiveCalls(EditorMarker *pmarker, EditorMarker *fmarker) {
    // for the moment
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

    EditorMarker *positionMarker = newEditorMarkerForPosition(&parameterPosition);
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

static void addParameter(EditorMarker *pos, char *fname, int argn, int usage) {
    if (isDefinitionOrDeclarationUsage(usage)) {
        if (addStringAsParameter(pos, NULL, fname, argn, refactoringOptions.refpar1) != -1)
            // now check that there is no conflict
            if (isDefinitionUsage(usage))
                checkThatParameterIsUnused(pos, fname, argn, CHECK_FOR_ADD_PARAM);
    } else {
        addStringAsParameter(pos, NULL, fname, argn, refactoringOptions.refpar2);
    }
}

static void deleteParameter(EditorMarker *pos, char *fname, int argn, int usage) {
    char         *text;
    EditorMarker *m1, *m2;

    Result res = getParameterPosition(pos, fname, argn);
    if (res != RESULT_OK)
        return;

    m1 = newEditorMarkerForPosition(&parameterBeginPosition);
    m2 = newEditorMarkerForPosition(&parameterEndPosition);

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
            editorMoveMarkerToNonBlank(m2, 1);
        }
        if (isDefinitionUsage(usage)) {
            // this must be at the end, because it discards values
            // of s_paramBeginPosition and s_paramEndPosition
            checkThatParameterIsUnused(pos, fname, argn, CHECK_FOR_DEL_PARAM);
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

    m1 = newEditorMarkerForPosition(&parameterBeginPosition);
    m2 = newEditorMarkerForPosition(&parameterEndPosition);

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
        editorMoveMarkerToNonBlank(m1, 1);
        m2->offset--;
        editorMoveMarkerToNonBlank(m2, -1);
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
        if (l->usage.kind != UsageUndefinedMacro) {
            /* TODO: Should we not abort if any of the occurrences fail? */
            if (manipulation == PPC_AVR_ADD_PARAMETER) {
                addParameter(l->marker, functionName, argn1, l->usage.kind);
            } else if (manipulation == PPC_AVR_DEL_PARAMETER) {
                deleteParameter(l->marker, functionName, argn1, l->usage.kind);
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
    occurrences = convertReferencesToEditorMarkers(sessionData.browserStack.top->references);

    ppcGotoMarker(point);

    applyParameterManipulationToFunction(nameOnPoint, occurrences, manipulation, argn1, argn2);
    freeEditorMarkersAndMarkerList(occurrences); // O(n^2)!
}

static void parameterManipulation(EditorMarker *point, int manip, int argn1, int argn2) {
    applyParameterManipulation(point, manip, argn1, argn2);
    // and generate output
    applyWholeRefactoringFromUndo();
    ppcGotoMarker(point);
}

static int createMarkersForAllReferencesInRegions(SymbolsMenu *menu, EditorRegionList **regions) {
    int res, n;

    res = 0;
    for (SymbolsMenu *mm = menu; mm != NULL; mm = mm->next) {
        assert(mm->markers == NULL);
        if (mm->selected && mm->visible) {
            mm->markers =
                convertReferencesToEditorMarkers(mm->references.references);
            if (regions != NULL) {
                restrictEditorMarkersToRegions(&mm->markers, regions);
            }
            LIST_MERGE_SORT(EditorMarkerList, mm->markers, editorMarkerListBefore);
            LIST_LEN(n, EditorMarkerList, mm->markers);
            res += n;
        }
    }
    return res;
}

// --------------------------------------- ExpandShortNames

static void applyExpandShortNames(EditorMarker *point) {
    char  fqtName[MAX_FILE_NAME_SIZE];
    char  fqtNameDot[2 * MAX_FILE_NAME_SIZE];
    char *shortName;
    int   shortNameLen;

    parseBufferUsingServer(refactoringOptions.project, point, NULL, "-olcxnotfqt", NULL);
    olcxPushSpecial(LINK_NAME_NOT_FQT_ITEM, OLO_NOT_FQT_REFS);

    // Do it in two steps because when changing file the references
    // are not updated while markers are, so first I need to change
    // everything to markers
    createMarkersForAllReferencesInRegions(sessionData.browserStack.top->menuSym, NULL);
    // Hmm. what if one reference will be twice ? Is it possible?
    for (SymbolsMenu *mm = sessionData.browserStack.top->menuSym; mm != NULL; mm = mm->next) {
        if (mm->selected && mm->visible) {
            javaGetClassNameFromFileNumber(mm->references.vApplClass, fqtName, DOTIFY_NAME);
            javaDotifyClassName(fqtName);
            sprintf(fqtNameDot, "%s.", fqtName);
            shortName    = javaGetShortClassName(fqtName);
            shortNameLen = strlen(shortName);
            if (isdigit(shortName[0])) {
                goto cont; // anonymous nested class, no expansion
            }
            for (EditorMarkerList *ppp = mm->markers; ppp != NULL; ppp = ppp->next) {
                char tmpBuff[TMP_BUFF_SIZE];
                log_trace("expanding at %s:%d", ppp->marker->buffer->name, ppp->marker->offset);
                if (ppp->usage.kind == UsageNonExpandableNotFQTNameInClassOrMethod) {
                    ppcGotoMarker(ppp->marker);
                    sprintf(tmpBuff,
                            "This occurence of %s would be misinterpreted after expansion to %s.\nNo action made "
                            "at this place.",
                            shortName, fqtName);
                    warningMessage(ERR_ST, tmpBuff);
                } else if (ppp->usage.kind == UsageNotFQFieldInClassOrMethod) {
                    replaceString(ppp->marker, 0, fqtNameDot);
                } else if (ppp->usage.kind == UsageNotFQTypeInClassOrMethod) {
                    checkedReplaceString(ppp->marker, shortNameLen, shortName, fqtName);
                }
            }
        }
    cont:;
        freeEditorMarkerListButNotMarkers(mm->markers);
        mm->markers = NULL;
    }
}

static void expandShortNames(EditorMarker *point) {
    applyExpandShortNames(point);
    applyWholeRefactoringFromUndo();
    ppcGotoMarker(point);
}

static EditorMarker *replaceStaticPrefix(EditorMarker *d, char *npref) {
    int           ppoffset, npreflen;
    EditorMarker *pp;
    char          pdot[MAX_FILE_NAME_SIZE];

    pp = createNewMarkerForExpressionStart(d, GET_STATIC_PREFIX_START);
    if (pp == NULL) {
        // this is an error, this is just to avoid possible core dump in the future
        pp = newEditorMarker(d->buffer, d->offset);
    } else {
        ppoffset = pp->offset;
        removeNonCommentCode(pp, d->offset - pp->offset);
        if (*npref != 0) {
            npreflen = strlen(npref);
            strcpy(pdot, npref);
            pdot[npreflen]     = '.';
            pdot[npreflen + 1] = 0;
            replaceString(pp, 0, pdot);
        }
        // return it back to beginning of fqt
        pp->offset = ppoffset;
    }
    return pp;
}

// -------------------------------------- ReduceLongNames

static void reduceLongReferencesInRegions(EditorMarker *point, EditorRegionList **regions) {
    EditorMarkerList *rli, *ri, *ro;
    int               progress, count;

    parseBufferUsingServer(refactoringOptions.project, point, NULL, "-olcxuselesslongnames",
                          "-olallchecks");
    olcxPushSpecial(LINK_NAME_IMPORTED_QUALIFIED_ITEM, OLO_USELESS_LONG_NAME);
    rli = convertReferencesToEditorMarkers(sessionData.browserStack.top->references);
    splitEditorMarkersWithRespectToRegions(&rli, regions, &ri, &ro);
    freeEditorMarkersAndMarkerList(ro);

    // a hack, as we cannot predict how many times this will be
    // invoked, adjust progress bar counter ratio

    progressFactor += 1;
    LIST_LEN(count, EditorMarkerList, ri);
    progress = 0;
    for (EditorMarkerList *rr = ri; rr != NULL; rr = rr->next) {
        EditorMarker *m = replaceStaticPrefix(rr->marker, "");
        freeEditorMarker(m);
        writeRelativeProgress((100*progress++)/count);
    }
    writeRelativeProgress(100);
}

// ------------------------------------------------------ Reduce Long Names In The File

static bool isTheImportUsed(EditorMarker *point, int line, int col) {
    char importSymbolName[TMP_STRING_SIZE];
    bool used;

    strcpy(importSymbolName, javaImportSymbolName_st(point->buffer->fileNumber, line, col));
    parseBufferUsingServer(refactoringOptions.project, point, NULL, "-olcxpushfileunused",
                          "-olallchecks");
    pushLocalUnusedSymbolsAction();
    used = true;
    for (SymbolsMenu *m = sessionData.browserStack.top->menuSym; m != NULL; m = m->next) {
        if (m->visible && strcmp(m->references.linkName, importSymbolName) == 0) {
            used = false;
            goto finish;
        }
    }
finish:
    olcxPopOnly();
    return used;
}

static int pushFileUnimportedFqts(EditorMarker *point, EditorRegionList **regions) {
    char pushOpt[TMP_STRING_SIZE];
    int  lastImportLine;

    sprintf(pushOpt, "-olcxpushspecialname=%s", LINK_NAME_UNIMPORTED_QUALIFIED_ITEM);
    parseBufferUsingServer(refactoringOptions.project, point, NULL, pushOpt, "-olallchecks");
    lastImportLine = parsedInfo.lastImportLine;
    olcxPushSpecial(LINK_NAME_UNIMPORTED_QUALIFIED_ITEM, OLO_PUSH_SPECIAL_NAME);
    createMarkersForAllReferencesInRegions(sessionData.browserStack.top->menuSym, regions);
    return lastImportLine;
}

static bool isImportNeeded(EditorMarker *point, EditorRegionList **regions, int vApplCl) {
    bool res;

    // check whether the symbol is reduced
    pushFileUnimportedFqts(point, regions);
    for (SymbolsMenu *mm = sessionData.browserStack.top->menuSym; mm != NULL; mm = mm->next) {
        if (mm->references.vApplClass == vApplCl) {
            res = true;
            goto fini;
        }
    }
    res = false;
fini:
    olcxPopOnly();
    return res;
}

static bool addImport(EditorMarker *point, EditorRegionList **regions, char *iname, int line, int vApplCl,
                      int interactive) {
    char              istat[MAX_CX_SYMBOL_SIZE];
    char              icoll;
    char             *ld1, *ld2;
    EditorMarker     *mm;
    bool              res;
    EditorUndo       *undoBase;
    EditorRegionList *wholeBuffer;

    undoBase = editorUndo;
    sprintf(istat, "import %s;\n", iname);
    mm = newEditorMarker(point->buffer, 0);
    // a little hack, make one free line after 'package'
    if (line > 1) {
        moveEditorMarkerToLineAndColumn(mm, line - 1, 0);
        if (strncmp(MARKER_TO_POINTER(mm), "package", 7) == 0) {
            // insert newline
            moveEditorMarkerToLineAndColumn(mm, line, 0);
            replaceString(mm, 0, "\n");
            line++;
        }
    }
    // add the import
    moveEditorMarkerToLineAndColumn(mm, line, 0);
    replaceString(mm, 0, istat);

    res = true;

    // TODO verify that import is unused
    ld1 = NULL;
    ld2 = istat + strlen("import");
    for (char *ss = ld2; *ss; ss++) {
        if (*ss == '.') {
            ld1 = ld2;
            ld2 = ss;
        }
    }
    if (ld1 == NULL) {
        errorMessage(ERR_INTERNAL, "can't find imported package");
    } else {
        icoll = ld1 - istat + 1;
        if (isTheImportUsed(mm, line, icoll)) {
            if (interactive == INTERACTIVE_YES) {
                ppcGenRecord(PPC_WARNING, "Sorry, adding this import would cause misinterpretation of\nsome of "
                                          "classes used elsewhere it the file.");
            }
            res = false;
        } else {
            wholeBuffer = createEditorRegionForWholeBuffer(point->buffer);
            reduceLongReferencesInRegions(point, &wholeBuffer);
            freeEditorMarkersAndRegionList(wholeBuffer);
            wholeBuffer = NULL;
            if (isImportNeeded(point, regions, vApplCl)) {
                if (interactive == INTERACTIVE_YES) {
                    ppcGenRecord(PPC_WARNING, "Sorry, this import will not help to reduce class references.");
                }
                res = false;
            }
        }
    }
    if (!res)
        editorUndoUntil(undoBase, NULL);

    freeEditorMarker(mm);
    return res;
}

static bool isInDisabledList(DisabledList *list, int file, int vApplCl) {
    for (DisabledList *ll = list; ll != NULL; ll = ll->next) {
        if (ll->file == file && ll->clas == vApplCl)
            return true;
    }
    return false;
}

static ContinueRefactoringKind translateAddImportStrategyToAction(AddImportStrategyKind strategy) {
    switch (strategy) {
    case IMPORT_ON_DEMAND:
        return RC_IMPORT_ON_DEMAND;
    case IMPORT_SINGLE_TYPE:
        return RC_IMPORT_SINGLE_TYPE;
    case IMPORT_KEEP_FQT_NAME:
        return RC_CONTINUE;
    default:
        errorMessage(ERR_INTERNAL, "wrong code for noninteractive add import");
    }
    return 0; /* Never happens */
}

static int interactiveAskForAddImportAction(EditorMarkerList *ppp, AddImportStrategyKind importStrategy, char *fqtName) {
    int action;

    applyWholeRefactoringFromUndo(); // make current state visible
    ppcGotoMarker(ppp->marker);
    ppcValueRecord(PPC_ADD_TO_IMPORTS_DIALOG, importStrategy, fqtName);
    beInteractive();
    action = options.continueRefactoring;
    return action;
}

static DisabledList *newDisabledList(SymbolsMenu *menu, int cfile, DisabledList *disabled) {
    DisabledList *dl;

    dl = editorAlloc(sizeof(DisabledList));
    *dl = (DisabledList){.file = cfile, .clas = menu->references.vApplClass, .next = disabled};

    return dl;
}

static void reduceNamesAndAddImportsInSingleFile(EditorMarker *point, EditorRegionList **regions,
                                                 int interactive) {
    EditorBuffer   *b;
    int             action, lastImportLine;
    bool            keepAdding;
    int             fileNumber;
    char            fqtName[MAX_FILE_NAME_SIZE];
    char            starName[MAX_FILE_NAME_SIZE];
    char           *dd;
    DisabledList *disabled, *dl;

    // just verify that all references are from single file
    if (regions != NULL && *regions != NULL) {
        b = (*regions)->region.begin->buffer;
        for (EditorRegionList *rl = *regions; rl != NULL; rl = rl->next) {
            if (rl->region.begin->buffer != b || rl->region.end->buffer != b) {
                assert(0 && "[refactoryPerformAddImportsInSingleFile] region list contains multiple buffers!");
                break;
            }
        }
    }

    reduceLongReferencesInRegions(point, regions);

    disabled   = NULL;
    keepAdding = true;
    while (keepAdding) {
        keepAdding     = false;
        lastImportLine = pushFileUnimportedFqts(point, regions);
        for (SymbolsMenu *menu = sessionData.browserStack.top->menuSym; menu != NULL; menu = menu->next) {
            EditorMarkerList *markers;
            markers                 = menu->markers;
            AddImportStrategyKind addImportStrategy = refactoringOptions.defaultAddImportStrategy;
            while (markers != NULL && !keepAdding &&
                   !isInDisabledList(disabled, markers->marker->buffer->fileNumber, menu->references.vApplClass)) {
                fileNumber = markers->marker->buffer->fileNumber;
                javaGetClassNameFromFileNumber(menu->references.vApplClass, fqtName, DOTIFY_NAME);
                javaDotifyClassName(fqtName);
                if (interactive == INTERACTIVE_YES) {
                    action = interactiveAskForAddImportAction(markers, addImportStrategy, fqtName);
                } else {
                    action = translateAddImportStrategyToAction(addImportStrategy);
                }
                //&sprintf(tmpBuff,"%s, %s, %d", simpleFileNameFromFileNum(markers->marker->buffer->fileNumber),
                // fqtName, action); ppcBottomInformation(tmpBuff);
                switch (action) {
                case RC_IMPORT_ON_DEMAND:
                    strcpy(starName, fqtName);
                    dd = lastOccurenceInString(starName, '.');
                    if (dd != NULL) {
                        sprintf(dd, ".*");
                        keepAdding = addImport(point, regions, starName, lastImportLine + 1,
                                               menu->references.vApplClass, interactive);
                    }
                    addImportStrategy = IMPORT_ON_DEMAND;
                    break;
                case RC_IMPORT_SINGLE_TYPE:
                    keepAdding          = addImport(point, regions, fqtName, lastImportLine + 1,
                                                    menu->references.vApplClass, interactive);
                    addImportStrategy = IMPORT_SINGLE_TYPE;
                    break;
                case RC_CONTINUE:
                    dl                  = newDisabledList(menu, fileNumber, disabled);
                    disabled            = dl;
                    addImportStrategy = IMPORT_KEEP_FQT_NAME;
                    break;
                default:
                    FATAL_ERROR(ERR_INTERNAL, "wrong continuation code", XREF_EXIT_ERR);
                }
                if (addImportStrategy <= 1)
                    addImportStrategy++;
            }
            freeEditorMarkersAndMarkerList(menu->markers);
            menu->markers = NULL;
        }
        olcxPopOnly();
    }
}

static void reduceNamesAndAddImports(EditorRegionList **regions, int interactive) {
    EditorBuffer      *cb;
    EditorRegionList **cr, **cl, *ncr;

    LIST_MERGE_SORT(EditorRegionList, *regions, editorRegionListBefore);
    cr = regions;
    // split regions per files and add imports
    while (*cr != NULL) {
        cl = cr;
        cb = (*cr)->region.begin->buffer;
        while (*cr != NULL && (*cr)->region.begin->buffer == cb)
            cr = &(*cr)->next;
        ncr = *cr;
        *cr = NULL;
        reduceNamesAndAddImportsInSingleFile((*cl)->region.begin, cl, interactive);
        // following line this was big bug, regions may be sortes, some may even be
        // even removed due to overlaps
        //& *cr = ncr;
        cr = cl;
        while (*cr != NULL)
            cr = &(*cr)->next;
        *cr = ncr;
    }
}

// ------------------------------------------- ReduceLongReferencesAddImports

// this is reduction of all names within file
static void reduceLongNamesInTheFile(EditorBuffer *buf, EditorMarker *point) {
    EditorRegionList *wholeBuffer;
    wholeBuffer = createEditorRegionForWholeBuffer(buf);
    // don't be interactive, I am too lazy to write jEdit interface
    // for <add-import-dialog>
    reduceNamesAndAddImportsInSingleFile(point, &wholeBuffer, INTERACTIVE_NO);
    freeEditorMarkersAndRegionList(wholeBuffer);
    wholeBuffer = NULL;
    applyWholeRefactoringFromUndo();
    ppcGotoMarker(point);
}

// this is reduction of a single fqt, problem is with detection of applicable context
static void addToImports(EditorMarker *point) {
    EditorMarker     *begin, *end;
    EditorRegionList *regionList;

    begin = duplicateEditorMarker(point);
    end   = duplicateEditorMarker(point);
    moveEditorMarkerBeyondIdentifier(begin, -1);
    moveEditorMarkerBeyondIdentifier(end, 1);

    regionList = newEditorRegionList(begin, end, NULL);
    reduceNamesAndAddImportsInSingleFile(point, &regionList, INTERACTIVE_YES);

    freeEditorMarkersAndRegionList(regionList);
    regionList = NULL;

    applyWholeRefactoringFromUndo();
    ppcGotoMarker(point);
}

static void pushAllReferencesOfMethod(EditorMarker *m1, char *specialOption) {
    parseBufferUsingServer(refactoringOptions.project, m1, NULL, "-olcxpushallinmethod", specialOption);
    olPushAllReferencesInBetween(parsedInfo.cxMemoryIndexAtMethodBegin, parsedInfo.cxMemoryIndexAtMethodEnd);
}

static void moveFirstElementOfMarkerList(EditorMarkerList **l1, EditorMarkerList **l2) {
    EditorMarkerList *mm;
    if (*l1 != NULL) {
        mm       = *l1;
        *l1      = (*l1)->next;
        mm->next = NULL;
        LIST_APPEND(EditorMarkerList, *l2, mm);
    }
}

static void showSafetyCheckFailingDialog(EditorMarkerList **totalDiff, char *message) {
    EditorUndo *redo;
    redo = NULL;
    editorUndoUntil(refactoringStartingPoint, &redo);
    olcxPushSpecialCheckMenuSym(LINK_NAME_SAFETY_CHECK_MISSED);
    pushMarkersAsReferences(totalDiff, sessionData.browserStack.top, LINK_NAME_SAFETY_CHECK_MISSED);
    displayResolutionDialog(message, PPCV_BROWSER_TYPE_WARNING, CONTINUATION_DISABLED);
    editorApplyUndos(redo, NULL, &editorUndo, GEN_NO_OUTPUT);
}

#define EACH_SYMBOL_ONCE 1

static void staticMoveCheckCorrespondance(SymbolsMenu *menu1, SymbolsMenu *menu2, ReferenceItem *theMethod) {
    SymbolsMenu      *mm1, *mm2;
    EditorMarkerList *diff1, *diff2, *totalDiff;

    totalDiff = NULL;
    mm1       = menu1;
    while (mm1 != NULL) {
        // do not check recursive calls
        if (isSameCxSymbolIncludingFunctionClass(&mm1->references, theMethod))
            goto cont;
        // nor local variables
        if (mm1->references.storage == StorageAuto)
            goto cont;
        // nor labels
        if (mm1->references.type == TypeLabel)
            goto cont;
        // do not check also any symbols from classes defined in inner scope
        if (isStrictlyEnclosingClass(mm1->references.vFunClass, theMethod->vFunClass))
            goto cont;
        // (maybe I should not test any local symbols ???)
        // O.K. something to be checked, find correspondance in mm2
        //&fprintf(dumpOut, "Looking for correspondance to %s\n", mm1->references.linkName);
        for (mm2 = menu2; mm2 != NULL; mm2 = mm2->next) {
            log_trace("Checking '%s'", mm2->references.linkName);
            if (isSameCxSymbolIncludingApplicationClass(&mm1->references, &mm2->references))
                break;
        }
        if (mm2 == NULL) {
            // O(n^2) !!!
            //&fprintf(dumpOut, "Did not find correspondance to %s\n", mm1->references.linkName);
#ifdef EACH_SYMBOL_ONCE
            // if each symbol reported only once
            moveFirstElementOfMarkerList(&mm1->markers, &totalDiff);
#else
            LIST_APPEND(S_editorMarkerList, totalDiff, mm1->markers);
            // hack!
            mm1->markers = NULL;
#endif
        } else {
            editorMarkersDifferences(&mm1->markers, &mm2->markers, &diff1, &diff2);
#ifdef EACH_SYMBOL_ONCE
            // if each symbol reported only once
            if (diff1 != NULL) {
                moveFirstElementOfMarkerList(&diff1, &totalDiff);
                //&fprintf(dumpOut, "problem with symbol %s corr %s\n", mm1->references.linkName,
                // mm2->references.name);
            } else {
                // no, do not put there new symbols, only lost are problems
                moveFirstElementOfMarkerList(&diff2, &totalDiff);
            }
            freeEditorMarkersAndMarkerList(diff1);
            freeEditorMarkersAndMarkerList(diff2);
#else
            LIST_APPEND(S_editorMarkerList, totalDiff, diff1);
            LIST_APPEND(S_editorMarkerList, totalDiff, diff2);
#endif
        }
        freeEditorMarkersAndMarkerList(mm1->markers);
        mm1->markers = NULL;
    cont:
        mm1 = mm1->next;
    }
    for (mm2 = menu2; mm2 != NULL; mm2 = mm2->next) {
        freeEditorMarkersAndMarkerList(mm2->markers);
        mm2->markers = NULL;
    }
    if (totalDiff != NULL) {
#ifdef EACH_SYMBOL_ONCE
        showSafetyCheckFailingDialog(&totalDiff, "These references will be  misinterpreted after refactoring. Fix "
                                                 "them first. (each symbol is reported only once)");
#else
        showSafetyCheckFailingDialog(&totalDiff, "These references will be  misinterpreted after refactoring");
#endif
        freeEditorMarkersAndMarkerList(totalDiff);
        totalDiff = NULL;
        askForReallyContinueConfirmation();
    }
}

// make it public, because you update references after and some references can
// be lost, later you can restrict accessibility

static void moveStaticObjectAndMakeItPublic(EditorMarker *mstart, EditorMarker *point, EditorMarker *mend,
                                            EditorMarker *target, char fqtname[], unsigned *outAccessFlags,
                                            int check, int limitIndex) {
    char              nameOnPoint[TMP_STRING_SIZE];
    int               size;
    SymbolsMenu      *mm1, *mm2;
    EditorMarker     *pp, *ppp, *movedEnd;
    EditorMarkerList *occs;
    EditorRegionList *regions;
    ReferenceItem   *theMethod;
    int               progress, count;

    movedEnd = duplicateEditorMarker(mend);
    movedEnd->offset--;

    //&editorDumpMarker(mstart);
    //&editorDumpMarker(movedEnd);

    size = mend->offset - mstart->offset;
    if (target->buffer == mstart->buffer && target->offset > mstart->offset &&
        target->offset < mstart->offset + size) {
        ppcGenRecord(PPC_INFORMATION, "You can't move something into itself.");
        return;
    }

    // O.K. move
    applyExpandShortNames(point);
    strcpy(nameOnPoint, getIdentifierOnMarker_static(point));
    assert(strlen(nameOnPoint) < TMP_STRING_SIZE - 1);
    occs = getReferences(point, STANDARD_SELECT_SYMBOLS_MESSAGE, PPCV_BROWSER_TYPE_INFO);
    assert(sessionData.browserStack.top && sessionData.browserStack.top->hkSelectedSym);
    if (outAccessFlags != NULL) {
        *outAccessFlags = sessionData.browserStack.top->hkSelectedSym->references.access;
    }
    //&parseBufferUsingServer(refactoringOptions.project, point, "-olcxrename");

    LIST_MERGE_SORT(EditorMarkerList, occs, editorMarkerListBefore);
    LIST_LEN(count, EditorMarkerList, occs);
    progress = 0;
    regions  = NULL;
    for (EditorMarkerList *ll = occs; ll != NULL; ll = ll->next) {
        if ((!isDefinitionOrDeclarationUsage(ll->usage.kind)) && ll->usage.kind != UsageConstructorDefinition) {
            pp  = replaceStaticPrefix(ll->marker, fqtname);
            ppp = newEditorMarker(ll->marker->buffer, ll->marker->offset);
            moveEditorMarkerBeyondIdentifier(ppp, 1);
            regions = newEditorRegionList(pp, ppp, regions);
        }
        writeRelativeProgress((100*progress++) / count);
    }
    writeRelativeProgress(100);

    size = mend->offset - mstart->offset;
    if (check == NO_CHECKS) {
        moveBlockInEditorBuffer(target, mstart, size, &editorUndo);
        changeAccessModifier(point, limitIndex, "public");
    } else {
        assert(sessionData.browserStack.top != NULL && sessionData.browserStack.top->hkSelectedSym != NULL);
        theMethod = &sessionData.browserStack.top->hkSelectedSym->references;
        pushAllReferencesOfMethod(point, "-olallchecks");
        createMarkersForAllReferencesInRegions(sessionData.browserStack.top->menuSym, NULL);
        moveBlockInEditorBuffer(target, mstart, size, &editorUndo);
        changeAccessModifier(point, limitIndex, "public");
        pushAllReferencesOfMethod(point, "-olallchecks");
        createMarkersForAllReferencesInRegions(sessionData.browserStack.top->menuSym, NULL);
        assert(sessionData.browserStack.top && sessionData.browserStack.top->previous);
        mm1 = sessionData.browserStack.top->previous->menuSym;
        mm2 = sessionData.browserStack.top->menuSym;
        staticMoveCheckCorrespondance(mm1, mm2, theMethod);
    }

    //&editorDumpMarker(mstart);
    //&editorDumpMarker(movedEnd);

    // reduce long names in the method
    pp      = duplicateEditorMarker(mstart);
    ppp     = duplicateEditorMarker(movedEnd);
    regions = newEditorRegionList(pp, ppp, regions);

    reduceNamesAndAddImports(&regions, INTERACTIVE_NO);
}

static EditorMarker *getTargetFromOptions(void) {
    EditorMarker *target;
    EditorBuffer *tb;
    int           tline;
    tb = findEditorBufferForFileOrCreateEmpty(
        normalizeFileName(refactoringOptions.moveTargetFile, cwd));
    target = newEditorMarker(tb, 0);
    sscanf(refactoringOptions.refpar1, "%d", &tline);
    moveEditorMarkerToLineAndColumn(target, tline, 0);
    return target;
}

/* JAVA only */
static void getMethodLimitsForMoving(EditorMarker *point, EditorMarker **methodStartP, EditorMarker **methodEndP,
                                     SyntaxPassParsedImportantPosition limitIndex) {
    EditorMarker *mstart, *mend;

    // get method limites
    makeSyntaxPassOnSource(point);
    if (parsedPositions[limitIndex].file == NO_FILE_NUMBER || parsedPositions[limitIndex + 1].file == NO_FILE_NUMBER) {
        FATAL_ERROR(ERR_INTERNAL, "Can't find declaration coordinates", XREF_EXIT_ERR);
    }
    mstart = newEditorMarkerForPosition(&parsedPositions[limitIndex]);
    mend   = newEditorMarkerForPosition(&parsedPositions[limitIndex + 1]);
    moveMarkerToTheBeginOfDefinitionScope(mstart);
    moveMarkerToTheEndOfDefinitionScope(mend);
    assert(mstart->buffer == mend->buffer);
    *methodStartP = mstart;
    *methodEndP   = mend;
}

static void getNameOfTheClassAndSuperClass(EditorMarker *point, char *ccname, char *supercname) {
    parseBufferUsingServer(refactoringOptions.project, point, NULL, "-olcxcurrentclass", NULL);
    if (ccname != NULL) {
        if (parsedInfo.currentClassAnswer[0] == 0) {
            FATAL_ERROR(ERR_ST, "can't get class on point", XREF_EXIT_ERR);
        }
        strcpy(ccname, parsedInfo.currentClassAnswer);
        javaDotifyClassName(ccname);
    }
    if (supercname != NULL) {
        if (parsedInfo.currentSuperClassAnswer[0] == 0) {
            FATAL_ERROR(ERR_ST, "can't get superclass of class on point", XREF_EXIT_ERR);
        }
        strcpy(supercname, parsedInfo.currentSuperClassAnswer);
        javaDotifyClassName(supercname);
    }
}

// ---------------------------------------------------- MoveStaticMethod

static void moveStaticFieldOrMethod(EditorMarker *point, SyntaxPassParsedImportantPosition limitIndex) {
    char          targetFqtName[MAX_FILE_NAME_SIZE];
    int           lines;
    unsigned      accFlags;
    EditorMarker *target, *mstart, *mend;

    target = getTargetFromOptions();

    if (!validTargetPlace(target, "-olcxmmtarget"))
        return;
    ensureReferencesAreUpdated(refactoringOptions.project);
    getMethodLimitsForMoving(point, &mstart, &mend, limitIndex);
    lines = countLinesBetweenEditorMarkers(mstart, mend);

    // O.K. Now STARTING!
    moveStaticObjectAndMakeItPublic(mstart, point, mend, target, targetFqtName, &accFlags, APPLY_CHECKS,
                                    limitIndex);
    //&sprintf(tmpBuff,"original acc == %d", accFlags); ppcBottomInformation(tmpBuff);
    restrictAccessibility(point, limitIndex, accFlags);

    // and generate output
    applyWholeRefactoringFromUndo();
    ppcGotoMarker(point);
    ppcValueRecord(PPC_INDENT, lines, "");
}

static void moveStaticMethod(EditorMarker *point) {
    moveStaticFieldOrMethod(point, SPP_METHOD_DECLARATION_BEGIN_POSITION);
}

static void moveStaticField(EditorMarker *point) {
    moveStaticFieldOrMethod(point, SPP_FIELD_DECLARATION_BEGIN_POSITION);
}

// ---------------------------------------------------------- MoveField

static void moveField(EditorMarker *point) {
    char              targetFqtName[MAX_FILE_NAME_SIZE];
    char              nameOnPoint[TMP_STRING_SIZE];
    char              prefixDot[TMP_STRING_SIZE];
    int               lines, size, check, accessFlags;
    EditorMarker     *target, *mstart, *mend, *movedEnd;
    EditorMarkerList *occs;
    EditorMarker     *pp, *ppp;
    EditorRegionList *regions;
    EditorUndo       *undoStartPoint, *redoTrack;
    int               progress, count;

    target = getTargetFromOptions();
    if (refactoringOptions.refpar2 != NULL && *refactoringOptions.refpar2 != 0) {
        sprintf(prefixDot, "%s.", refactoringOptions.refpar2);
    } else {
        ; // sprintf(prefixDot, "");
    }

    if (!validTargetPlace(target, "-olcxmmtarget"))
        return;
    ensureReferencesAreUpdated(refactoringOptions.project);
    getNameOfTheClassAndSuperClass(target, targetFqtName, NULL);
    getMethodLimitsForMoving(point, &mstart, &mend, SPP_FIELD_DECLARATION_BEGIN_POSITION);
    lines = countLinesBetweenEditorMarkers(mstart, mend);

    // O.K. Now STARTING!
    movedEnd = duplicateEditorMarker(mend);
    movedEnd->offset--;

    //&editorDumpMarker(mstart);
    //&editorDumpMarker(movedEnd);

    size = mend->offset - mstart->offset;
    if (target->buffer == mstart->buffer && target->offset > mstart->offset &&
        target->offset < mstart->offset + size) {
        ppcGenRecord(PPC_INFORMATION, "You can't move something into itself.");
        return;
    }

    // O.K. move
    applyExpandShortNames(point);
    strcpy(nameOnPoint, getIdentifierOnMarker_static(point));
    assert(strlen(nameOnPoint) < TMP_STRING_SIZE - 1);
    occs = getReferences(point, STANDARD_SELECT_SYMBOLS_MESSAGE, PPCV_BROWSER_TYPE_INFO);
    assert(sessionData.browserStack.top && sessionData.browserStack.top->hkSelectedSym);
    accessFlags = sessionData.browserStack.top->hkSelectedSym->references.access;

    undoStartPoint = editorUndo;
    LIST_MERGE_SORT(EditorMarkerList, occs, editorMarkerListBefore);
    LIST_LEN(count, EditorMarkerList, occs);
    progress = 0;
    regions   = NULL;
    for (EditorMarkerList *ll = occs; ll != NULL; ll = ll->next) {
        if (!isDefinitionOrDeclarationUsage(ll->usage.kind)) {
            if (*prefixDot != 0) {
                replaceString(ll->marker, 0, prefixDot);
            }
        }
        writeRelativeProgress((100*progress++) / count);
    }
    writeRelativeProgress(100);

    size = mend->offset - mstart->offset;
    moveBlockInEditorBuffer(target, mstart, size, &editorUndo);

    // reduce long names in the method
    pp      = duplicateEditorMarker(mstart);
    ppp     = duplicateEditorMarker(movedEnd);
    regions = newEditorRegionList(pp, ppp, regions);

    reduceNamesAndAddImports(&regions, INTERACTIVE_NO);

    changeAccessModifier(point, SPP_FIELD_DECLARATION_BEGIN_POSITION, "public");
    restrictAccessibility(point, SPP_FIELD_DECLARATION_BEGIN_POSITION, accessFlags);

    redoTrack = NULL;
    check     = makeSafetyCheckAndUndo(point, &occs, undoStartPoint, &redoTrack);
    if (!check) {
        askForReallyContinueConfirmation();
    }
    // and generate output
    editorApplyUndos(redoTrack, NULL, NULL, GEN_FULL_OUTPUT);

    ppcGotoMarker(point);
    ppcValueRecord(PPC_INDENT, lines, "");
}

// ------------------------------------------------------ ExtractMethod

static void extractMethod(EditorMarker *point, EditorMarker *mark) {
    parseBufferUsingServer(refactoringOptions.project, point, mark, "-olcxextract", NULL);
}

static void extractMacro(EditorMarker *point, EditorMarker *mark) {
    parseBufferUsingServer(refactoringOptions.project, point, mark, "-olcxextract", "-olexmacro");
}

static void extractVariable(EditorMarker *point, EditorMarker *mark) {
    parseBufferUsingServer(refactoringOptions.project, point, mark, "-olcxextract", "-olexvariable");
}

static char *computeUpdateOptionForSymbol(EditorMarker *point) {
    EditorMarkerList *occs;
    SymbolsMenu      *csym;
    int               hasHeaderReferenceFlag, scope, cat, multiFileRefsFlag, fn;
    char             *selectedUpdateOption;

    assert(point != NULL && point->buffer != NULL);
    currentLanguage = getLanguageFor(point->buffer->name);

    hasHeaderReferenceFlag = 0;
    multiFileRefsFlag      = 0;
    occs                   = getReferences(point, NULL, PPCV_BROWSER_TYPE_WARNING);
    csym                   = sessionData.browserStack.top->hkSelectedSym;
    scope                  = csym->references.scope;
    cat                    = csym->references.category;

    if (occs == NULL) {
        fn = NO_FILE_NUMBER;
    } else {
        assert(occs->marker != NULL && occs->marker->buffer != NULL);
        fn = occs->marker->buffer->fileNumber;
    }
    for (EditorMarkerList *o = occs; o != NULL; o = o->next) {
        assert(o->marker != NULL && o->marker->buffer != NULL);
        FileItem *fileItem = getFileItem(o->marker->buffer->fileNumber);
        if (fn != o->marker->buffer->fileNumber) {
            multiFileRefsFlag = 1;
        }
        if (!fileItem->isArgument) {
            hasHeaderReferenceFlag = 1;
        }
    }

    if (cat == CategoryLocal) {
        // useless to update when there is nothing about the symbol in Tags
        selectedUpdateOption = "";
    } else if (hasHeaderReferenceFlag) {
        // once it is in a header, full update is required
        selectedUpdateOption = "-update";
    } else if (scope == ScopeAuto || scope == ScopeFile) {
        // for example a local var or a static function not used in any header
        if (multiFileRefsFlag) {
            errorMessage(ERR_INTERNAL, "something goes wrong, a local symbol is used in several files");
            selectedUpdateOption = "-update";
        } else {
            selectedUpdateOption = "";
        }
    } else if (!multiFileRefsFlag) {
        // this is a little bit tricky. It may provoke a bug when
        // a new external function is not yet indexed, but used in another file.
        // But it is so practical, so take the risk.
        selectedUpdateOption = "";
    } else {
        // may seems too strong, but implicitly linked global functions
        // requires this (for example).
        selectedUpdateOption = "-fastupdate";
    }

    freeEditorMarkersAndMarkerList(occs);
    occs = NULL;
    olcxPopOnly();

    return selectedUpdateOption;
}

// --------------------------------------------------------------------

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
    EditorBuffer *buf = findEditorBufferForFile(file);

    EditorMarker *point = getPointFromOptions(buf);
    EditorMarker *mark  = getMarkFromOptions(buf);

    refactoringStartingPoint = editorUndo;

    // init subtask
    mainTaskEntryInitialisations(argument_count(editServInitOptions), editServInitOptions);
    editServerSubTaskFirstPass = true;

    progressFactor = 1;

    switch (refactoringOptions.theRefactoring) {
    case AVR_RENAME_SYMBOL:
    case AVR_RENAME_CLASS:
    case AVR_RENAME_PACKAGE:
        progressFactor = 3;
        updateOption   = computeUpdateOptionForSymbol(point);
        renameAtPoint(point);
        break;
    case AVR_EXPAND_NAMES:
        progressFactor = 1;
        expandShortNames(point);
        break;
    case AVR_REDUCE_NAMES:
        progressFactor = 1;
        reduceLongNamesInTheFile(buf, point);
        break;
    case AVR_ADD_ALL_POSSIBLE_IMPORTS:
        progressFactor = 2;
        reduceLongNamesInTheFile(buf, point);
        break;
    case AVR_ADD_TO_IMPORT:     /* AVR_ADD_TO_INCLUDE ? */
        progressFactor = 2;
        addToImports(point);
        break;
    case AVR_ADD_PARAMETER:
    case AVR_DEL_PARAMETER:
    case AVR_MOVE_PARAMETER:
        progressFactor = 3;
        updateOption   = computeUpdateOptionForSymbol(point);
        currentLanguage = getLanguageFor(file);
        parameterManipulation(point, refactoringOptions.theRefactoring, refactoringOptions.olcxGotoVal,
                              refactoringOptions.parnum2);
        break;
    case AVR_MOVE_FIELD:
        progressFactor = 6;
        moveField(point);
        break;
    case AVR_MOVE_STATIC_FIELD:
        progressFactor = 4;
        moveStaticField(point);
        break;
    case AVR_MOVE_STATIC_METHOD: /* AVR_MOVE_FUNCTION ? */
        progressFactor = 4;
        moveStaticMethod(point);
        break;
    case AVR_EXTRACT_METHOD:    /* AVR_EXTRACT_FUNCTION ? */
        progressFactor = 1;
        extractMethod(point, mark);
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

    closeMainOutputFile();
    ppcSynchronize();

    // exiting, put undefined, so that main will finish
    options.mode = UndefinedMode;

    LEAVE();
}
