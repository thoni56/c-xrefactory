#include "refactory.h"

/* Main is currently needed for:
   mainTaskEntryInitialisations
   mainOpenOutputFile
 */
#include "classhierarchy.h"
#include "commons.h"
#include "complete.h"
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
#include "yylex.h"
#include "xref.h"

#include "log.h"

#define RRF_CHARS_TO_PRE_CHECK_AROUND 1
#define MAX_NARGV_OPTIONS_COUNT 50


typedef struct tpCheckMoveClassData {
    PushAllInBetweenData mm;
    char                       *spack;
    char                       *tpack;
    int                         transPackageMove;
    char                       *sclass;
} TpCheckMoveClassData;

typedef struct tpCheckSpecialReferencesData {
    PushAllInBetweenData mm;
    char                       *symbolToTest;
    int                         classToTest;
    struct referencesItem      *foundSpecialRefItem;
    struct reference           *foundSpecialR;
    struct referencesItem      *foundRefToTestedClass;
    struct referencesItem      *foundRefNotToTestedClass;
    struct reference           *foundOuterScopeRef;
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
static char *xrefInitOptions[] = {
    "xref",
    "-xrefactory-II",
    "-briefoutput",
    NULL,
};

static Options refactoringOptions;

static char *updateOption = "-fastupdate";

static bool moveClassMapFunReturnOnUninterestingSymbols(ReferencesItem *ri, TpCheckMoveClassData *dd) {
    if (!isPushAllMethodsValidRefItem(ri))
        return true;
    /* this is too strong, but check only fields and methods */
    if (ri->storage != StorageField && ri->storage != StorageMethod && ri->storage != StorageConstructor)
        return true;
    /* check that it has default accessibility*/
    if (ri->access & AccessPublic)
        return true;
    if (ri->access & AccessProtected)
        return true;
    if (!(ri->access & AccessPrivate)) {
        /* default accessibility, check only if transpackage move*/
        if (!dd->transPackageMove)
            return true;
    }
    return false;
}

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
// including options in s_opt, ...
// call to this function MUST be followed by a pushing action, to refresh options
static void ensureReferencesUpdated(char *project) {
    int   nargc, refactoryXrefInitOptionsNum;
    char *nargv[MAX_NARGV_OPTIONS_COUNT];

    // following would be too long to be allocated on stack
    static Options savedOptions;

    if (updateOption == NULL || *updateOption == 0) {
        writeRelativeProgress(100);
        return;
    }

    ppcBegin(PPC_UPDATE_REPORT);

    quasiSaveModifiedEditorBuffers();

    deepCopyOptionsFromTo(&options, &savedOptions);

    setArguments(nargv, project, NULL, NULL);
    nargc                       = argument_count(nargv);
    refactoryXrefInitOptionsNum = argument_count(xrefInitOptions);
    for (int i = 1; i < refactoryXrefInitOptionsNum; i++) {
        nargv[nargc++] = xrefInitOptions[i];
    }
    nargv[nargc++] = updateOption;

    currentPass = ANY_PASS;
    mainTaskEntryInitialisations(nargc, nargv);

    callXref(nargc, nargv, true);

    deepCopyOptionsFromTo(&savedOptions, &options);
    ppcEnd(PPC_UPDATE_REPORT);

    // return into editSubTaskState
    mainTaskEntryInitialisations(argument_count(editServInitOptions), editServInitOptions);
    editServerSubTaskFirstPass = true;
}

static void parseBufferUsingServer(char *project, EditorMarker *point, EditorMarker *mark,
                                  char *pushOption, char *pushOption2) {
    char *nargv[MAX_NARGV_OPTIONS_COUNT];
    int   nargc;

    currentPass = ANY_PASS;

    assert(options.mode == ServerMode);

    setArguments(nargv, project, point, mark);
    nargc = argument_count(nargv);
    if (pushOption != NULL) {
        nargv[nargc++] = pushOption;
    }
    if (pushOption2 != NULL) {
        nargv[nargc++] = pushOption2;
    }
    initServer(nargc, nargv);
    callServer(argument_count(editServInitOptions), editServInitOptions, nargc, nargv,
               &editServerSubTaskFirstPass);
}

static void beInteractive(void) {
    int    pargc;
    char **pargv;

    ENTER();
    deepCopyOptionsFromTo(&options, &savedOptions);
    for (;;) {
        closeMainOutputFile();
        ppcSynchronize();
        deepCopyOptionsFromTo(&savedOptions, &options);
        processOptions(argument_count(editServInitOptions), editServInitOptions, DONT_PROCESS_FILE_ARGUMENTS);
        getPipedOptions(&pargc, &pargv);
        openOutputFile(refactoringOptions.outputFileName);
        if (pargc <= 1)
            break;
        initServer(pargc, pargv);
        if (options.continueRefactoring != RC_NONE)
            break;
        callServer(argument_count(editServInitOptions), editServInitOptions, pargc, pargv,
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
    ensureReferencesUpdated(refactoringOptions.project);
    parseBufferUsingServer(project, point, NULL, "-olcxsafetycheck2", NULL);

    assert(sessionData.browserStack.top != NULL);
    if (sessionData.browserStack.top->hkSelectedSym == NULL) {
        errorMessage(ERR_ST, "No symbol found for refactoring safety check");
    }
    olCreateSelectionMenu(sessionData.browserStack.top->command);
    if (safetyCheck2ShouldWarn()) {
        char tmpBuff[TMP_BUFF_SIZE];
        if (LANGUAGE(LANG_JAVA)) {
            sprintf(tmpBuff, "This is class hierarchy of given symbol as it will appear after the refactoring. It "
                             "does not correspond to the hierarchy before the refactoring. It is probable that "
                             "the refactoring will not be behaviour preserving. If you are not sure about your "
                             "action, you should abandon this refactoring!");
        } else {
            sprintf(tmpBuff, "These symbols will be refererred at this place after the refactoring. It is "
                             "probable that the refactoring will not be behaviour preserving. If you are not sure "
                             "about your action, you should abandon this refactoring!");
        }
        displayResolutionDialog(tmpBuff, PPCV_BROWSER_TYPE_WARNING, CONTINUATION_ENABLED);
    }
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

static SymbolsMenu *javaGetRelevantHkSelectedItem(ReferencesItem *ri) {
    SymbolsMenu    *ss;
    OlcxReferences *rstack;

    rstack = sessionData.browserStack.top;
    for (ss = rstack->hkSelectedSym; ss != NULL; ss = ss->next) {
        if (isSameCxSymbol(ri, &ss->references) && ri->vFunClass == ss->references.vFunClass) {
            break;
        }
    }

    return ss;
}

static void tpCheckFutureAccOfLocalReferences(ReferencesItem *ri, void *ddd) {
    TpCheckMoveClassData *dd;
    SymbolsMenu          *ss;

    dd = (TpCheckMoveClassData *)ddd;
    log_trace("!mapping %s", ri->linkName);
    if (moveClassMapFunReturnOnUninterestingSymbols(ri, dd))
        return;

    ss = javaGetRelevantHkSelectedItem(ri);
    if (ss != NULL) {
        // relevant symbol
        for (Reference *rr = ri->references; rr != NULL; rr = rr->next) {
            // I should check only references from this file
            if (rr->position.file == inputFileNumber) {
                // check if the reference is outside moved class
                if ((!dm_isBetween(cxMemory, rr, dd->mm.minMemi, dd->mm.maxMemi)) && OL_VIEWABLE_REFS(rr)) {
                    // yes there is a reference from outside to our symbol
                    ss->selected = true;
                    ss->visible  = true;
                    break;
                }
            }
        }
    }
}

static void tpCheckMoveClassPutClassDefaultSymbols(ReferencesItem *ri, void *ddd) {
    OlcxReferences       *rstack;
    TpCheckMoveClassData *dd;

    dd = (TpCheckMoveClassData *)ddd;
    log_trace("!mapping %s", ri->linkName);
    if (moveClassMapFunReturnOnUninterestingSymbols(ri, dd))
        return;

    // fine, add it to Menu, so we will load its references
    for (Reference *rr = ri->references; rr != NULL; rr = rr->next) {
        log_trace("Checking %d.%d ref of %s", rr->position.line, rr->position.col, ri->linkName);
        if (IS_PUSH_ALL_METHODS_VALID_REFERENCE(rr, (&dd->mm))) {
            if (isDefinitionOrDeclarationUsage(rr->usage.kind)) {
                // definition inside class, default or private acces to be checked
                rstack = sessionData.browserStack.top;
                olAddBrowsedSymbolToMenu(&rstack->hkSelectedSym, ri, 1, 1, 0, UsageUsed, 0, &rr->position,
                                         rr->usage.kind);
                break;
            }
        }
    }
}

static void tpCheckFutureAccessibilitiesOfSymbolsDefinedInsideMovedClass(TpCheckMoveClassData dd) {
    OlcxReferences *rstack;
    SymbolsMenu   **sss;

    rstack                = sessionData.browserStack.top;
    rstack->hkSelectedSym = olCreateSpecialMenuItem(LINK_NAME_MOVE_CLASS_MISSED, NO_FILE_NUMBER, StorageDefault);
    // push them into hkSelection,
    mapOverReferenceTableWithPointer(tpCheckMoveClassPutClassDefaultSymbols, &dd);
    // load all theirs references
    olCreateSelectionMenu(rstack->command);
    // mark all as unselected unvisible
    for (SymbolsMenu *ss = rstack->hkSelectedSym; ss != NULL; ss = ss->next) {
        ss->selected = true;
        ss->visible  = true;
    }
    // checks all references from within this file
    mapOverReferenceTableWithPointer(tpCheckFutureAccOfLocalReferences, &dd);
    // check references from outside
    for (SymbolsMenu *mm = rstack->menuSym; mm != NULL; mm = mm->next) {
        SymbolsMenu *ss = javaGetRelevantHkSelectedItem(&mm->references);
        if (ss != NULL && !ss->selected) {
            for (Reference *rr = mm->references.references; rr != NULL; rr = rr->next) {
                if (rr->position.file != inputFileNumber) {
                    // yes there is a reference from outside to our symbol
                    ss->selected = true;
                    ss->visible  = true;
                    goto nextsym;
                }
            }
        }
    nextsym:;
    }

    sss = &rstack->menuSym;
    while (*sss != NULL) {
        SymbolsMenu *ss = javaGetRelevantHkSelectedItem(&(*sss)->references);
        if (ss != NULL && ss->selected) {
            sss = &(*sss)->next;
        } else {
            *sss = freeSymbolsMenu(*sss);
        }
    }
}

static void tpCheckDefaultAccessibilitiesMoveClass(ReferencesItem *ri, void *ddd) {
    OlcxReferences       *rstack;
    TpCheckMoveClassData *dd;
    char                  symclass[MAX_FILE_NAME_SIZE];
    int                   sclen, symclen;

    dd = (TpCheckMoveClassData *)ddd;
    //&fprintf(communicationChannel,"!mapping %s\n", ri->linkName);
    if (moveClassMapFunReturnOnUninterestingSymbols(ri, dd))
        return;

    // check that it is not from moved class
    javaGetClassNameFromFileNumber(ri->vFunClass, symclass, KEEP_SLASHES);
    sclen   = strlen(dd->sclass);
    symclen = strlen(symclass);
    if (sclen <= symclen && filenameCompare(dd->sclass, symclass, sclen) == 0)
        return;
    // O.K. finally check if there is a reference
    for (Reference *rr = ri->references; rr != NULL; rr = rr->next) {
        if (IS_PUSH_ALL_METHODS_VALID_REFERENCE(rr, (&dd->mm))) {
            // O.K. there is a reference inside the moved class, add it to the list,
            assert(sessionData.browserStack.top);
            rstack = sessionData.browserStack.top;
            olcxAddReference(&rstack->references, rr, 0);
            break;
        }
    }
}

static void fillTpCheckMoveClassData(TpCheckMoveClassData *checkMoveClassData, int minMemi, int maxMemi,
                                     char *spack, char *tpack, bool transPackageMove, char *sclass) {
    checkMoveClassData->mm.minMemi       = minMemi;
    checkMoveClassData->mm.maxMemi       = maxMemi;
    checkMoveClassData->spack            = spack;
    checkMoveClassData->tpack            = tpack;
    checkMoveClassData->transPackageMove = transPackageMove;
    checkMoveClassData->sclass           = sclass;
}

static void tpCheckFillMoveClassData(TpCheckMoveClassData *dd, char *spack, char *tpack) {
    OlcxReferences *rstack;
    SymbolsMenu    *sclass;
    char           *targetfile, *srcfile;
    bool            transPackageMove;

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    sclass = rstack->hkSelectedSym;
    assert(sclass);
    targetfile = options.moveTargetFile;
    assert(targetfile);
    srcfile = inputFileName;
    assert(srcfile);

    javaGetPackageNameFromSourceFileName(srcfile, spack);
    javaGetPackageNameFromSourceFileName(targetfile, tpack);

    // O.K. moving from package spack to package tpack
    if (compareFileNames(spack, tpack) == 0)
        transPackageMove = false;
    else
        transPackageMove = true;

    fillTpCheckMoveClassData(dd, parsedInfo.cxMemoryIndexAtClassBeginning, parsedInfo.cxMemoryIndexAtClassEnd, spack, tpack,
                             transPackageMove, sclass->references.linkName);
}

static void askForReallyContinueConfirmation(void) {
    ppcAskConfirmation("The refactoring may change program behaviour, really continue?");
}

static Reference * olcxCopyRefList(Reference *references) {
    Reference *res, *a, **aa;
    res = NULL; aa= &res;
    for (Reference *rr=references; rr!=NULL; rr=rr->next) {
        a = olcxAlloc(sizeof(Reference));
        *a = *rr;
        a->next = NULL;
        *aa = a;
        aa = &(a->next);
    }
    return res;
}

static bool checkMoveClassAccessibilities(void) {
    OlcxReferences      *rstack;
    SymbolsMenu         *ss;
    TpCheckMoveClassData dd;
    char                 spack[MAX_FILE_NAME_SIZE];
    char                 tpack[MAX_FILE_NAME_SIZE];

    tpCheckFillMoveClassData(&dd, spack, tpack);
    olcxPushSpecialCheckMenuSym(LINK_NAME_MOVE_CLASS_MISSED);
    mapOverReferenceTableWithPointer(tpCheckDefaultAccessibilitiesMoveClass, &dd);

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    if (rstack->references != NULL) {
        ss = rstack->menuSym;
        assert(ss);
        ss->references.references = olcxCopyRefList(rstack->references);
        rstack->actual            = rstack->references;
        if (refactoringOptions.refactoringMode == RefactoryMode) {
            displayResolutionDialog(
                "These references inside moved class are refering to symbols which will be inaccessible at new "
                "class location. You should adjust their access first. (Each symbol is listed only once)",
                PPCV_BROWSER_TYPE_WARNING, CONTINUATION_DISABLED);
            askForReallyContinueConfirmation();
        }
        return false;
    }
    olStackDeleteSymbol(sessionData.browserStack.top);
    // O.K. now check symbols defined inside the class
    pushEmptySession(&sessionData.browserStack);
    tpCheckFutureAccessibilitiesOfSymbolsDefinedInsideMovedClass(dd);
    rstack = sessionData.browserStack.top;
    if (rstack->menuSym != NULL) {
        olcxRecomputeSelRefs(rstack);
        // TODO, synchronize this with emacs, but how?
        rstack->refsFilterLevel = RFilterDefinitions;
        if (refactoringOptions.refactoringMode == RefactoryMode) {
            displayResolutionDialog("These symbols defined inside moved class and used outside the class will be "
                                    "inaccessible at new class location. You should adjust their access first.",
                                    PPCV_BROWSER_TYPE_WARNING, CONTINUATION_DISABLED);
            askForReallyContinueConfirmation();
        }
        return false;
    }
    olStackDeleteSymbol(sessionData.browserStack.top);
    return true;
}

static bool checkSourceIsNotInnerClass(void) {
    OlcxReferences *rstack;
    SymbolsMenu    *menu;

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    menu   = rstack->hkSelectedSym;
    assert(menu);

    // I can rely that it is a class
    int index = getClassNumFromClassLinkName(menu->references.linkName, NO_FILE_NUMBER);
    //& target = options.moveTargetClass;
    //& assert(target!=NULL);

    int directEnclosingInstanceIndex = getFileItem(index)->directEnclosingInstance;
    if (directEnclosingInstanceIndex != -1 && directEnclosingInstanceIndex != NO_FILE_NUMBER &&
        (menu->references.access & AccessInterface) == 0) {
        char tmpBuff[TMP_BUFF_SIZE];
        // If there exists a direct enclosing instance, it is an inner class
        sprintf(tmpBuff, "This is an inner class. Current version of C-xrefactory can only move top level classes "
                         "and nested classes that are declared 'static'. If the class does not depend on its "
                         "enclosing instances, you should declare it 'static' and then move it.");
        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
        errorMessage(ERR_ST, tmpBuff);
        return false;
    }
    return true;
}

static void tpCheckSpecialReferencesMapFun(ReferencesItem *ri, void *voidDataP) {
    TpCheckSpecialReferencesData *dd;

    dd = (TpCheckSpecialReferencesData *)voidDataP;
    assert(sessionData.browserStack.top);
    // todo make supermethod symbol special type
    //&fprintf(dumpOut,"! checking %s\n", ri->linkName);
    if (strcmp(ri->linkName, dd->symbolToTest) != 0)
        return;
    for (Reference *rr = ri->references; rr != NULL; rr = rr->next) {
        if (dm_isBetween(cxMemory, rr, dd->mm.minMemi, dd->mm.maxMemi)) {
            // a super method reference
            dd->foundSpecialRefItem = ri;
            dd->foundSpecialR       = rr;
            if (ri->vFunClass == dd->classToTest) {
                // a super reference to direct superclass
                dd->foundRefToTestedClass = ri;
            } else {
                dd->foundRefNotToTestedClass = ri;
            }
            // TODO! check if it is reference to outer scope
            if (rr->usage.kind == UsageMaybeQualifiedThis) {
                dd->foundOuterScopeRef = rr;
            }
        }
    }
}

static void initTpCheckSpecialReferencesData(TpCheckSpecialReferencesData *referencesData, int minMemi,
                                             int maxMemi, char *symbolToTest, int classToTest) {
    referencesData->mm.minMemi               = minMemi;
    referencesData->mm.maxMemi               = maxMemi;
    referencesData->symbolToTest             = symbolToTest;
    referencesData->classToTest              = classToTest;
    referencesData->foundSpecialRefItem      = NULL;
    referencesData->foundSpecialR            = NULL;
    referencesData->foundRefToTestedClass    = NULL;
    referencesData->foundRefNotToTestedClass = NULL;
    referencesData->foundOuterScopeRef       = NULL;
}

static bool tpCheckSuperMethodReferencesInit(TpCheckSpecialReferencesData *rr) {
    SymbolsMenu    *ss;
    int             scl;
    OlcxReferences *rstack;

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    ss     = rstack->hkSelectedSym;
    assert(ss);
    scl = javaGetSuperClassNumFromClassNum(ss->references.vApplClass);
    if (scl == NO_FILE_NUMBER) {
        errorMessage(ERR_ST, "no super class, something is going wrong");
        return false;
        ;
    }
    initTpCheckSpecialReferencesData(rr, parsedInfo.cxMemoryIndexAtMethodBegin, parsedInfo.cxMemoryIndexAtMethodEnd,
                                     LINK_NAME_SUPER_METHOD_ITEM, scl);
    mapOverReferenceTableWithPointer(tpCheckSpecialReferencesMapFun, rr);
    return true;
}

static bool tpCheckSuperMethodReferencesForPullUp(void) {
    TpCheckSpecialReferencesData rr;
    OlcxReferences                *rstack;
    SymbolsMenu                   *ss;
    int                            tmp;
    char                           tt[TMP_STRING_SIZE];
    char                           ttt[MAX_CX_SYMBOL_SIZE];

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    ss     = rstack->hkSelectedSym;
    assert(ss);
    tmp = tpCheckSuperMethodReferencesInit(&rr);
    if (!tmp)
        return false;

    // synthetize an answer
    if (rr.foundRefToTestedClass != NULL) {
        char tmpBuff[TMP_BUFF_SIZE];
        linkNamePrettyPrint(ttt, ss->references.linkName, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
        javaGetClassNameFromFileNumber(rr.foundRefToTestedClass->vFunClass, tt, DOTIFY_NAME);
        sprintf(tmpBuff,
                "'%s' invokes another method using the keyword \"super\" and this invocation is refering to class "
                "'%s', i.e. to the class where '%s' will be moved. In consequence, it is not possible to ensure "
                "behaviour preseving pulling-up of this method.",
                ttt, tt, ttt);
        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
        errorMessage(ERR_ST, tmpBuff);
        return false;
    }
    return true;
}

static bool tpCheckSuperMethodReferencesAfterPushDown(void) {
    TpCheckSpecialReferencesData rr;
    OlcxReferences                *rstack;
    SymbolsMenu                   *ss;
    char                           ttt[MAX_CX_SYMBOL_SIZE];

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    ss     = rstack->hkSelectedSym;
    assert(ss);
    if (!tpCheckSuperMethodReferencesInit(&rr))
        return false;

    // synthetize an answer
    if (rr.foundRefToTestedClass != NULL) {
        char tmpBuff[TMP_BUFF_SIZE];
        linkNamePrettyPrint(ttt, ss->references.linkName, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
        sprintf(tmpBuff,
                "'%s' invokes another method using the keyword \"super\" and the invoked method is also defined "
                "in current class. After pushing down, the reference will be misrelated. In consequence, it is "
                "not possible to ensure behaviour preseving pushing-down of this method.",
                ttt);
        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
        fprintf(communicationChannel, ":[warning] %s", tmpBuff);
        //&errorMessage(ERR_ST, tmpBuff);
        return false;
    }
    return true;
}

static bool tpCheckSuperMethodReferencesForDynToSt(void) {
    TpCheckSpecialReferencesData rr;

    if (!tpCheckSuperMethodReferencesInit(&rr))
        return false;

    // synthetize an answer
    if (rr.foundSpecialRefItem != NULL) {
        char tmpBuff[TMP_BUFF_SIZE];
        if (options.xref2)
            ppcGotoPosition(&rr.foundSpecialR->position);
        sprintf(tmpBuff, "This method invokes another method using the keyword \"super\". Current version of "
                         "C-xrefactory does not know how to make it static.");
        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
        errorMessage(ERR_ST, tmpBuff);
        return false;
    }
    return true;
}

static bool tpCheckOuterScopeUsagesForDynToSt(void) {
    TpCheckSpecialReferencesData rr;
    SymbolsMenu                   *ss;
    OlcxReferences                *rstack;

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    ss     = rstack->hkSelectedSym;
    assert(ss);
    initTpCheckSpecialReferencesData(&rr, parsedInfo.cxMemoryIndexAtMethodBegin, parsedInfo.cxMemoryIndexAtMethodEnd,
                                     LINK_NAME_MAYBE_THIS_ITEM, ss->references.vApplClass);
    mapOverReferenceTableWithPointer(tpCheckSpecialReferencesMapFun, &rr);
    if (rr.foundOuterScopeRef != NULL) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "Inner class method is using symbols from outer scope. Current version of C-xrefactory "
                         "does not know how to make it static.");
        // be soft, so that user can try it and see.
        if (options.xref2) {
            ppcGotoPosition(&rr.foundOuterScopeRef->position);
            formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
            ppcGenRecord(PPC_ERROR, tmpBuff);
        } else {
            fprintf(communicationChannel, ":[warning] %s", tmpBuff);
        }
        //& errorMessage(ERR_ST, tmpBuff);
        return false;
    }
    return true;
}

static bool tpCheckMethodReferencesWithApplOnSuperClassForPullUp(void) {
    // is there an application to original class or some of super types?
    // you should not consider any call from within the method,
    // when the method is invoked as single name, am I right?
    // No. Do not consider only invocations of form super.method().
    OlcxReferences *rstack;
    SymbolsMenu    *ss, *mm;
    Symbol         *target;
    int             srccn;
    int             targetcn;
    UNUSED          targetcn;

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    ss     = rstack->hkSelectedSym;
    assert(ss);
    srccn  = ss->references.vApplClass;
    target = getMoveTargetClass();
    assert(target != NULL && target->u.structSpec);
    targetcn = target->u.structSpec->classFileNumber;
    for (mm = rstack->menuSym; mm != NULL; mm = mm->next) {
        if (isSameCxSymbol(&ss->references, &mm->references)) {
            if (javaIsSuperClass(mm->references.vApplClass, srccn)) {
                // finally check there is some other reference than super.method()
                // and definition
                for (Reference *r = mm->references.references; r != NULL; r = r->next) {
                    if ((!isDefinitionOrDeclarationUsage(r->usage.kind)) &&
                        r->usage.kind != UsageMethodInvokedViaSuper) {
                        // well there is, issue warning message and finish
                        char tempName[MAX_CX_SYMBOL_SIZE];
                        char tmpBuff[TMP_BUFF_SIZE];
                        linkNamePrettyPrint(tempName, ss->references.linkName, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
                        sprintf(tmpBuff,
                                "%s is defined also in superclass and there are invocations syntactically "
                                "refering to one of superclasses. Under some circumstances this may cause that "
                                "pulling up of this method will not be behaviour preserving.",
                                tempName);
                        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
                        warningMessage(ERR_ST, tmpBuff);
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

static bool tpCheckTargetToBeDirectSubOrSuperClass(int flag, char *subOrSuper) {
    OlcxReferences          *rstack;
    SymbolsMenu             *ss;
    char                     ttt[TMP_STRING_SIZE];
    char                     targetClassName[TMP_STRING_SIZE];
    ClassHierarchyReference *cl;
    Symbol                  *target;
    bool                     found = false;

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    ss     = rstack->hkSelectedSym;
    assert(ss);
    target = getMoveTargetClass();
    if (target == NULL) {
        errorMessage(ERR_ST, "moving to NULL target class?");
        return false;
    }

    scanForClassHierarchy();
    assert(target->u.structSpec != NULL && target->u.structSpec->classFileNumber != NO_FILE_NUMBER);
    FileItem *fileItem = getFileItem(ss->references.vApplClass);
    if (flag == REQ_SUBCLASS)
        cl = fileItem->inferiorClasses;
    else
        cl = fileItem->superClasses;
    for (; cl != NULL; cl = cl->next) {
        log_trace("!checking %d(%s) <-> %d(%s)", cl->superClass, getFileItem(cl->superClass)->name,
                  target->u.structSpec->classFileNumber, getFileItem(target->u.structSpec->classFileNumber)->name);
        if (cl->superClass == target->u.structSpec->classFileNumber) {
            found = true;
            break;
        }
    }
    if (found)
        return true;

    javaGetClassNameFromFileNumber(target->u.structSpec->classFileNumber, targetClassName, DOTIFY_NAME);
    javaGetClassNameFromFileNumber(ss->references.vApplClass, ttt, DOTIFY_NAME);

    char tmpBuff[TMP_BUFF_SIZE];
    sprintf(tmpBuff, "Class %s is not direct %s of %s. This refactoring provides moving to direct %ses only.",
            targetClassName, subOrSuper, ttt, subOrSuper);
    formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
    errorMessage(ERR_ST, tmpBuff);
    return false;
}

static bool tpPullUpFieldLastPreconditions(void) {
    OlcxReferences *rstack;
    SymbolsMenu    *ss, *mm;
    char            ttt[TMP_STRING_SIZE];
    Symbol         *target;
    int             pcharFlag;
    char            tmpBuff[TMP_BUFF_SIZE];

    pcharFlag = 0;
    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    ss     = rstack->hkSelectedSym;
    assert(ss);
    target = getMoveTargetClass();
    assert(target != NULL);
    assert(target->u.structSpec != NULL && target->u.structSpec->classFileNumber != NO_FILE_NUMBER);
    for (mm = rstack->menuSym; mm != NULL; mm = mm->next) {
        if (isSameCxSymbol(&ss->references, &mm->references) &&
            mm->references.vApplClass == target->u.structSpec->classFileNumber)
            goto cont2;
    }
    // it is O.K. no item found
    return true;
cont2:
    // an item found, it must be empty
    if (mm->references.references == NULL)
        return true;
    javaGetClassNameFromFileNumber(target->u.structSpec->classFileNumber, ttt, DOTIFY_NAME);
    if (isDefinitionOrDeclarationUsage(mm->references.references->usage.kind) &&
        mm->references.references->next == NULL) {
        if (pcharFlag == 0) {
            pcharFlag = 1;
            fprintf(communicationChannel, ":[warning] ");
        }
        sprintf(
            tmpBuff,
            "%s is already defined in the superclass %s.  Pulling up will do nothing, but removing the definition "
            "from the subclass. You should make sure that both fields are initialized to the same value.",
            mm->references.linkName, ttt);
        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
        warningMessage(ERR_ST, tmpBuff);
        return false;
    }
    sprintf(tmpBuff,
            "There are already references of the field %s syntactically applied on the superclass %s, pulling up "
            "this field would cause confusion!",
            mm->references.linkName, ttt);
    formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
    errorMessage(ERR_ST, tmpBuff);
    return false;
}

static bool tpPushDownFieldLastPreconditions(void) {
    OlcxReferences *rstack;
    SymbolsMenu    *ss, *sourcesm, *targetsm;
    char            ttt[TMP_STRING_SIZE];
    Reference      *rr;
    Symbol         *target;
    int             thisclassi;
    bool            res;
    char            tmpBuff[TMP_BUFF_SIZE];

    res = true;
    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    ss     = rstack->hkSelectedSym;
    assert(ss);
    thisclassi = ss->references.vApplClass;
    target     = getMoveTargetClass();
    assert(target != NULL);
    assert(target->u.structSpec != NULL && target->u.structSpec->classFileNumber != NO_FILE_NUMBER);
    sourcesm = targetsm = NULL;
    for (SymbolsMenu *mm = rstack->menuSym; mm != NULL; mm = mm->next) {
        if (isSameCxSymbol(&ss->references, &mm->references)) {
            if (mm->references.vApplClass == target->u.structSpec->classFileNumber)
                targetsm = mm;
            if (mm->references.vApplClass == thisclassi)
                sourcesm = mm;
        }
    }
    if (targetsm != NULL) {
        rr = getDefinitionRef(targetsm->references.references);
        if (rr != NULL && isDefinitionOrDeclarationUsage(rr->usage.kind)) {
            javaGetClassNameFromFileNumber(target->u.structSpec->classFileNumber, ttt, DOTIFY_NAME);
            sprintf(tmpBuff, "The field %s is already defined in %s!", targetsm->references.linkName, ttt);
            formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
            errorMessage(ERR_ST, tmpBuff);
            return false;
        }
    }
    if (sourcesm != NULL) {
        if (sourcesm->references.references != NULL && sourcesm->references.references->next != NULL) {
            //& if (pcharFlag==0) {pcharFlag=1; fprintf(communicationChannel,":[warning] ");}
            javaGetClassNameFromFileNumber(thisclassi, ttt, DOTIFY_NAME);
            sprintf(tmpBuff,
                    "There are several references of %s syntactically applied on %s. This may cause that the "
                    "refactoring will not be behaviour preserving!",
                    sourcesm->references.linkName, ttt);
            formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
            warningMessage(ERR_ST, tmpBuff);
            res = false;
        }
    }
    return res;
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

    if (symtype == TypeStruct && LANGUAGE(LANG_JAVA) && refactoringOptions.theRefactoring != AVR_RENAME_CLASS) {
        errorMessage(ERR_INTERNAL, "Use Rename Class to rename classes");
    }
    if (symtype == TypePackage && LANGUAGE(LANG_JAVA) && refactoringOptions.theRefactoring != AVR_RENAME_PACKAGE) {
        errorMessage(ERR_INTERNAL, "Use Rename Package to rename packages");
    }

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
    ensureReferencesUpdated(refactoringOptions.project);

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
    ReferencesItem *p, *sss;
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

    ensureReferencesUpdated(refactoringOptions.project);

    if (LANGUAGE(LANG_JAVA)) {
        message = STANDARD_SELECT_SYMBOLS_MESSAGE;
    } else {
        message = STANDARD_C_SELECT_SYMBOLS_MESSAGE;
    }
    // rename
    strcpy(nameOnPoint, getIdentifierOnMarker_static(point));
    assert(strlen(nameOnPoint) < TMP_STRING_SIZE - 1);
    occs           = pushGetAndPreCheckReferences(point, nameOnPoint, message, PPCV_BROWSER_TYPE_INFO);
    csym           = sessionData.browserStack.top->hkSelectedSym;
    symtype        = csym->references.type;
    symLinkName    = csym->references.linkName;
    undoStartPoint = editorUndo;
    if (!LANGUAGE(LANG_JAVA)) {
        multipleOccurencesSafetyCheck();
    }
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
    if (!LANGUAGE(LANG_JAVA)) {
        // check preconditions to avoid cases like
        // #define PAR1 toto,
        // ... lot of text
        // #define PAR2 tutu,
        //    function(PAR1 PAR2 0);
        // Hmmm, but how to do it? TODO!!!!
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
    int               check;
    EditorMarkerList *occurrences;
    EditorUndo       *startPoint, *redoTrack;

    ensureReferencesUpdated(refactoringOptions.project);

    strcpy(nameOnPoint, getIdentifierOnMarker_static(point));
    pushReferences(point, "-olcxargmanip", STANDARD_SELECT_SYMBOLS_MESSAGE, PPCV_BROWSER_TYPE_INFO);
    occurrences = convertReferencesToEditorMarkers(sessionData.browserStack.top->references);
    startPoint = editorUndo;
    // first just check that loaded files are up to date
    //& precheckThatSymbolRefsCorresponds(nameOnPoint, occurrences);

    //&dumpEditorBuffer(occurrences->marker->buffer);
    //&editorDumpMarkerList(occurrences);
    // for some error mesages it is more natural that cursor does not move
    ppcGotoMarker(point);
    redoTrack = NULL;
    applyParameterManipulationToFunction(nameOnPoint, occurrences, manipulation, argn1, argn2);
    if (LANGUAGE(LANG_JAVA)) {
        check = makeSafetyCheckAndUndo(point, &occurrences, startPoint, &redoTrack);
        if (!check)
            askForReallyContinueConfirmation();
        editorApplyUndos(redoTrack, NULL, &editorUndo, GEN_NO_OUTPUT);
    }
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

static ContinueRefactoringKind translatePassToAddImportAction(int pass) {
    switch (pass) {
    case 0:
        return RC_IMPORT_ON_DEMAND;
    case 1:
        return RC_IMPORT_SINGLE_TYPE;
    case 2:
        return RC_CONTINUE;
    default:
        errorMessage(ERR_INTERNAL, "wrong code for noninteractive add import");
    }
    return 0; /* Never happens */
}

static int interactiveAskForAddImportAction(EditorMarkerList *ppp, int defaultAction, char *fqtName) {
    int action;

    applyWholeRefactoringFromUndo(); // make current state visible
    ppcGotoMarker(ppp->marker);
    ppcValueRecord(PPC_ADD_TO_IMPORTS_DIALOG, defaultAction, fqtName);
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
            int defaultImportAction = refactoringOptions.defaultAddImportStrategy;
            while (markers != NULL && !keepAdding &&
                   !isInDisabledList(disabled, markers->marker->buffer->fileNumber, menu->references.vApplClass)) {
                fileNumber = markers->marker->buffer->fileNumber;
                javaGetClassNameFromFileNumber(menu->references.vApplClass, fqtName, DOTIFY_NAME);
                javaDotifyClassName(fqtName);
                if (interactive == INTERACTIVE_YES) {
                    action = interactiveAskForAddImportAction(markers, defaultImportAction, fqtName);
                } else {
                    action = translatePassToAddImportAction(defaultImportAction);
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
                    defaultImportAction = NID_IMPORT_ON_DEMAND;
                    break;
                case RC_IMPORT_SINGLE_TYPE:
                    keepAdding          = addImport(point, regions, fqtName, lastImportLine + 1,
                                                    menu->references.vApplClass, interactive);
                    defaultImportAction = NID_SINGLE_TYPE_IMPORT;
                    break;
                case RC_CONTINUE:
                    dl                  = newDisabledList(menu, fileNumber, disabled);
                    disabled            = dl;
                    defaultImportAction = NID_KEPP_FQT_NAME;
                    break;
                default:
                    FATAL_ERROR(ERR_INTERNAL, "wrong continuation code", XREF_EXIT_ERR);
                }
                if (defaultImportAction <= 1)
                    defaultImportAction++;
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

static void staticMoveCheckCorrespondance(SymbolsMenu *menu1, SymbolsMenu *menu2, ReferencesItem *theMethod) {
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
    ReferencesItem   *theMethod;
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
    ensureReferencesUpdated(refactoringOptions.project);
    if (LANGUAGE(LANG_JAVA))
        getNameOfTheClassAndSuperClass(target, targetFqtName, NULL);
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
    ensureReferencesUpdated(refactoringOptions.project);
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

// ---------------------------------------------------------- MoveClass

static void setMovingPrecheckStandardEnvironment(EditorMarker *point, char *targetFqtName) {
    SymbolsMenu *ss;

    /* TODO: WTF... There must be some huge gap in the logic
     * here. editServerParseBuffer() does not call answerEditAction()
     * where the case for OLO_TRIVIAL_PRECHECK is triggered. Also the
     * call to olTrivialRefactoringPreCheck() in cxref.c is expecting
     * a precheck code which doesn't get sent here, nor is it parsed
     * by handleOptions()... */
    parseBufferUsingServer(refactoringOptions.project, point, NULL, "-olcxtrivialprecheck", NULL);
    assert(sessionData.browserStack.top);
    olCreateSelectionMenu(sessionData.browserStack.top->command);
    options.moveTargetFile  = refactoringOptions.moveTargetFile;
    options.moveTargetClass = targetFqtName;
    assert(sessionData.browserStack.top);
    ss = sessionData.browserStack.top->hkSelectedSym;
    assert(ss);
}

static void performMoveClass(EditorMarker *point, EditorMarker *target, EditorMarker **outstart,
                             EditorMarker **outend) {
    char                 spack[MAX_FILE_NAME_SIZE];
    char                 tpack[MAX_FILE_NAME_SIZE];
    char                 targetFqtName[MAX_FILE_NAME_SIZE];
    int                  targetIsNestedInClass;
    EditorMarker        *mstart, *mend;
    SymbolsMenu         *ss;
    TpCheckMoveClassData dd;

    *outstart = *outend = NULL;

    // get target place
    parseBufferUsingServer(refactoringOptions.project, target, NULL, "-olcxcurrentclass", NULL);
    if (parsedInfo.currentPackageAnswer[0] == 0) {
        errorMessage(ERR_ST, "Can't get target class or package");
        return;
    }
    if (parsedInfo.currentClassAnswer[0] == 0) {
        sprintf(targetFqtName, "%s", parsedInfo.currentPackageAnswer);
        targetIsNestedInClass = 0;
    } else {
        sprintf(targetFqtName, "%s", parsedInfo.currentClassAnswer);
        targetIsNestedInClass = 1;
    }
    javaDotifyClassName(targetFqtName);

    // get limits
    makeSyntaxPassOnSource(point);
    if (parsedPositions[SPP_CLASS_DECLARATION_BEGIN_POSITION].file == NO_FILE_NUMBER ||
        parsedPositions[SPP_CLASS_DECLARATION_END_POSITION].file == NO_FILE_NUMBER) {
        FATAL_ERROR(ERR_INTERNAL, "Can't find declaration coordinates", XREF_EXIT_ERR);
    }
    mstart = newEditorMarkerForPosition(
        &parsedPositions[SPP_CLASS_DECLARATION_BEGIN_POSITION]);
    mend =
        newEditorMarkerForPosition(&parsedPositions[SPP_CLASS_DECLARATION_END_POSITION]);
    moveMarkerToTheBeginOfDefinitionScope(mstart);
    moveMarkerToTheEndOfDefinitionScope(mend);

    assert(mstart->buffer == mend->buffer);

    *outstart = mstart;
    // put outend -1 to be updated during moving
    *outend = newEditorMarker(mend->buffer, mend->offset - 1);

    // prechecks
    setMovingPrecheckStandardEnvironment(point, targetFqtName);
    ss = sessionData.browserStack.top->hkSelectedSym;
    tpCheckFillMoveClassData(&dd, spack, tpack);
    checkSourceIsNotInnerClass();
    checkMoveClassAccessibilities();

    // O.K. Now STARTING!
    moveStaticObjectAndMakeItPublic(mstart, point, mend, target, targetFqtName, NULL, NO_CHECKS,
                                    SPP_CLASS_DECLARATION_BEGIN_POSITION);

    // recover end marker
    (*outend)->offset++;

    // finally fiddle modifiers
    if (ss->references.access & AccessStatic) {
        if (!targetIsNestedInClass) {
            // nested -> top level
            //&sprintf(tmpBuff,"removing modifier"); ppcBottomInformation(tmpBuff);
            removeModifier(point, SPP_CLASS_DECLARATION_BEGIN_POSITION, "static");
        }
    } else {
        if (targetIsNestedInClass) {
            // top level -> nested
            addModifier(point, SPP_CLASS_DECLARATION_BEGIN_POSITION, "static");
        }
    }
    if (dd.transPackageMove) {
        // add public
        changeAccessModifier(point, SPP_CLASS_DECLARATION_BEGIN_POSITION, "public");
    }
}

static void moveClass(EditorMarker *point) {
    EditorMarker *target, *start, *end;
    int           linenum;

    target = getTargetFromOptions();
    if (!validTargetPlace(target, "-olcxmctarget"))
        return;

    ensureReferencesUpdated(refactoringOptions.project);

    performMoveClass(point, target, &start, &end);
    linenum = countLinesBetweenEditorMarkers(start, end);

    // and generate output
    applyWholeRefactoringFromUndo();
    ppcGotoMarker(point);
    ppcValueRecord(PPC_INDENT, linenum, "");

    ppcGenRecord(PPC_INFORMATION, "\nDone.\nDo not forget to remove .class files of former class.");
}

static void getPackageNameFromMarkerFileName(EditorMarker *target, char *tclass) {
    char *dd;
    strcpy(tclass, javaCutSourcePathFromFileName(target->buffer->name));
    dd = lastOccurenceInString(tclass, '.');
    if (dd != NULL)
        *dd = 0;
    dd = lastOccurenceOfSlashOrBackslash(tclass);
    if (dd != NULL) {
        *dd = 0;
        javaDotifyFileName(tclass);
    } else {
        *tclass = 0;
    }
}

static void insertPackageStatToNewFile(EditorMarker *target) {
    char tclass[MAX_FILE_NAME_SIZE];
    char pack[2 * MAX_FILE_NAME_SIZE];

    getPackageNameFromMarkerFileName(target, tclass);
    if (tclass[0] == 0) {
        sprintf(pack, "\n");
    } else {
        sprintf(pack, "package %s;\n", tclass);
    }
    sprintf(pack + strlen(pack), "\n\n");
    replaceString(target, 0, pack);
    target->offset--;
}

static void moveClassToNewFile(EditorMarker *point) {
    EditorMarker *target, *mstart, *mend, *npoint;
    EditorBuffer *buff;
    int           linenum;

    buff = point->buffer;
    ensureReferencesUpdated(refactoringOptions.project);
    target = getTargetFromOptions();

    // insert package statement
    insertPackageStatToNewFile(target);

    performMoveClass(point, target, &mstart, &mend);

    if (mstart == NULL || mend == NULL)
        return;
    //&editorDumpMarker(mstart);
    //&editorDumpMarker(mend);
    linenum = countLinesBetweenEditorMarkers(mstart, mend);

    // and generate output
    applyWholeRefactoringFromUndo();

    // indentation must be at the end (undo, redo does not work with)
    ppcGotoMarker(point);
    ppcValueRecord(PPC_INDENT, linenum, "");

    // TODO check whether the original class was the only class in the file
    npoint = newEditorMarker(buff, 0);
    // just to parse the file
    parseBufferUsingServer(refactoringOptions.project, npoint, NULL, "-olcxpushspecialname=", NULL);
    if (parsedPositions[SPP_LAST_TOP_LEVEL_CLASS_POSITION].file == NO_FILE_NUMBER) {
        ppcGotoMarker(npoint);
        ppcGenRecord(PPC_KILL_BUFFER_REMOVE_FILE, "This file does not contain classes anymore, can I remove it?");
    }
    ppcGenRecord(PPC_INFORMATION, "\nDone.\nDo not forget to remove .class files of former class.");
}

static void moveAllClassesToNewFile(EditorMarker *point) {
    // TODO: this should really copy whole file, including commentaries
    // between classes, etc... Then update all references
}

static void addCopyOfMarkerToList(EditorMarkerList **ll, EditorMarker *mm, Usage usage) {
    EditorMarker     *nn;
    EditorMarkerList *lll;
    nn = newEditorMarker(mm->buffer, mm->offset);
    lll = editorAlloc(sizeof(EditorMarkerList));
    *lll = (EditorMarkerList){.marker = nn, .usage = usage, .next = *ll};
    *ll  = lll;
}

// ------------------------------------------ TurnDynamicToStatic

static void refactorVirtualToStatic(EditorMarker *point) {
    char              nameOnPoint[TMP_STRING_SIZE];
    char              primary[REFACTORING_TMP_STRING_SIZE];
    char              fqstaticname[REFACTORING_TMP_STRING_SIZE];
    char              fqthis[REFACTORING_TMP_STRING_SIZE];
    char              pardecl[2 * REFACTORING_TMP_STRING_SIZE];
    char              parusage[REFACTORING_TMP_STRING_SIZE];
    char              cid[TMP_STRING_SIZE];
    int               plen, ppoffset, poffset;
    int               progress, progressj, count;
    EditorMarker     *pp, *ppp, *nparamdefpos;
    EditorMarkerList *occs, *allrefs;
    EditorMarkerList *npoccs, *npadded, *diff1, *diff2;
    EditorRegionList *regions, **reglast, *lll;
    SymbolsMenu      *csym;
    EditorUndo       *undoStartPoint;
    Usage             defaultUsage;

    nparamdefpos = NULL;
    ensureReferencesUpdated(refactoringOptions.project);
    strcpy(nameOnPoint, getIdentifierOnMarker_static(point));
    assert(strlen(nameOnPoint) < TMP_STRING_SIZE - 1);
    occs = pushGetAndPreCheckReferences(point, nameOnPoint,
        "If you see this message it is highly probable that turning this virtual method into static will not be "
        "behaviour preserving! This refactoring is behaviour preserving only  if the method does not use "
        "mechanism of virtual invocations. In this dialog you should select the application classes which are "
        "refering to the method which will become static. If you can't unambiguously determine those references "
        "do not continue in this refactoring!",
        PPCV_BROWSER_TYPE_WARNING);
    freeEditorMarkersAndMarkerList(occs);

    if (!tpCheckOuterScopeUsagesForDynToSt())
        return;
    if (!tpCheckSuperMethodReferencesForDynToSt())
        return;

    // Pass over all references and move primary prefix to first parameter
    // also insert new first parameter on definition
    csym = sessionData.browserStack.top->hkSelectedSym;
    javaGetClassNameFromFileNumber(csym->references.vFunClass, fqstaticname, DOTIFY_NAME);
    javaDotifyClassName(fqstaticname);
    sprintf(pardecl, "%s %s", fqstaticname, refactoringOptions.refpar1);
    sprintf(fqstaticname + strlen(fqstaticname), ".");

    count = progress = 0;
    for (SymbolsMenu *mm = sessionData.browserStack.top->menuSym; mm != NULL; mm = mm->next) {
        if (mm->selected && mm->visible) {
            mm->markers =
                convertReferencesToEditorMarkers(mm->references.references);
            LIST_MERGE_SORT(EditorMarkerList, mm->markers, editorMarkerListBefore);
            LIST_LEN(progressj, EditorMarkerList, mm->markers);
            count += progressj;
        }
    }
    undoStartPoint = editorUndo;
    regions        = NULL;
    reglast        = &regions;
    allrefs        = NULL;
    for (SymbolsMenu *mm = sessionData.browserStack.top->menuSym; mm != NULL; mm = mm->next) {
        if (mm->selected && mm->visible) {
            javaGetClassNameFromFileNumber(mm->references.vApplClass, fqthis, DOTIFY_NAME);
            javaDotifyClassName(fqthis);
            sprintf(fqthis + strlen(fqthis), ".this");
            for (EditorMarkerList *ll = mm->markers; ll != NULL; ll = ll->next) {
                addCopyOfMarkerToList(&allrefs, ll->marker, ll->usage);
                pp = NULL;
                if (isDefinitionOrDeclarationUsage(ll->usage.kind)) {
                    pp = newEditorMarker(ll->marker->buffer, ll->marker->offset);
                    pp->offset = addStringAsParameter(ll->marker, ll->marker, nameOnPoint, 1, pardecl);
                    // remember definition position of new parameter
                    nparamdefpos = newEditorMarker(pp->buffer,
                                                   pp->offset + strlen(pardecl) - strlen(refactoringOptions.refpar1));
                } else {
                    pp = createNewMarkerForExpressionStart(ll->marker, GET_PRIMARY_START);
                    assert(pp != NULL);
                    ppoffset = pp->offset;
                    plen     = ll->marker->offset - pp->offset;
                    strncpy(primary, MARKER_TO_POINTER(pp), plen);
                    // delete pending dot
                    while (plen > 0 && primary[plen - 1] != '.')
                        plen--;
                    if (plen > 0) {
                        primary[plen - 1] = 0;
                    } else {
                        if (javaLinkNameIsAnnonymousClass(mm->references.linkName)) {
                            strcpy(primary, "this");
                        } else {
                            strcpy(primary, fqthis);
                        }
                    }
                    replaceString(pp, ll->marker->offset - pp->offset, fqstaticname);
                    addStringAsParameter(ll->marker, ll->marker, nameOnPoint, 1, primary);
                    // return offset back to beginning of fqt
                    pp->offset = ppoffset;
                }
                ppp = newEditorMarker(ll->marker->buffer, ll->marker->offset);
                moveEditorMarkerBeyondIdentifier(ppp, 1);

                /* TODO: clean up... */
                lll      = newEditorRegionList(pp, ppp, NULL);
                *reglast = lll;
                reglast  = &lll->next;
                writeRelativeProgress((100*progress++) / count);
            }
            freeEditorMarkersAndMarkerList(mm->markers);
            mm->markers = NULL;
        }
    }
    writeRelativeProgress(100);

    // pop references
    sessionData.browserStack.top = sessionData.browserStack.top->previous;

    sprintf(parusage, "%s.", refactoringOptions.refpar1);

    parseBufferUsingServer(refactoringOptions.project, point, NULL, "-olcxmaybethis", NULL);
    olcxPushSpecial(LINK_NAME_MAYBE_THIS_ITEM, OLO_MAYBE_THIS);

    count = progress = 0;
    assert(sessionData.browserStack.top && sessionData.browserStack.top->hkSelectedSym);
    for (SymbolsMenu *mm = sessionData.browserStack.top->menuSym; mm != NULL; mm = mm->next) {
        if (mm->selected && mm->visible) {
            mm->markers =
                convertReferencesToEditorMarkers(mm->references.references);
            LIST_MERGE_SORT(EditorMarkerList, mm->markers, editorMarkerListBefore);
            LIST_LEN(progressj, EditorMarkerList, mm->markers);
            count += progressj;
        }
    }

    // passing references inside method and change them to the new parameter
    npadded = NULL;
    fillUsage(&defaultUsage, UsageDefined, 0);
    addCopyOfMarkerToList(&npadded, nparamdefpos, defaultUsage);

    for (SymbolsMenu *mm = sessionData.browserStack.top->menuSym; mm != NULL; mm = mm->next) {
        if (mm->selected && mm->visible) {
            for (EditorMarkerList *ll = mm->markers; ll != NULL; ll = ll->next) {
                if (ll->usage.kind == UsageMaybeQualifThisInClassOrMethod) {
                    editorUndoUntil(undoStartPoint, NULL);
                    ppcGotoMarker(ll->marker);
                    errorMessage(ERR_ST, "The method is using qualified this to access enclosed instance. Do not "
                                         "know how to make it static.");
                    return;
                } else if (ll->usage.kind == UsageMaybeThisInClassOrMethod) {
                    strncpy(cid, getIdentifierOnMarker_static(ll->marker), TMP_STRING_SIZE);
                    cid[TMP_STRING_SIZE - 1] = 0;
                    poffset                  = ll->marker->offset;
                    //&sprintf(tmpBuff, "Checking %s", cid); ppcGenRecord(PPC_INFORMATION, tmpBuff);
                    if (strcmp(cid, "this") == 0 || strcmp(cid, "super") == 0) {
                        pp      = replaceStaticPrefix(ll->marker, "");
                        poffset = pp->offset;
                        freeEditorMarker(pp);
                        checkedReplaceString(ll->marker, 4, cid, refactoringOptions.refpar1);
                    } else {
                        replaceString(ll->marker, 0, parusage);
                    }
                    ll->marker->offset = poffset;
                    addCopyOfMarkerToList(&npadded, ll->marker, ll->usage);
                }
                writeRelativeProgress((100*progress++) / count);
            }
        }
    }
    writeRelativeProgress(100);

    addModifier(point, SPP_METHOD_DECLARATION_BEGIN_POSITION, "static");

    // reduce long names at the end because of recursive calls
    reduceNamesAndAddImports(&regions, INTERACTIVE_NO);
    freeEditorMarkersAndRegionList(regions);
    regions = NULL;

    // safety check checking that new parameter has exactly
    // those references as expected (not hidden by a local variable and no
    // occurence of extra variable is resolved to parameter)
    npoccs = getReferences(nparamdefpos, "Internal problem, during new parameter resolution",
                           PPCV_BROWSER_TYPE_WARNING);
    editorMarkersDifferences(&npoccs, &npadded, &diff1, &diff2);
    LIST_APPEND(EditorMarkerList, diff1, diff2);
    diff2 = NULL;
    if (diff1 != NULL) {
        ppcGotoMarker(point);
        showSafetyCheckFailingDialog(&diff1, "The new parameter conflicts with existing symbols");
    }

    if (npoccs != NULL && npoccs->next == NULL) {
        // only one occurence, this must be the definition
        // but check it for being sure
        // maybe you should update references and delete the parameter
        // after, but for now, use computed references, it should work.
        if (isDefinitionUsage(npoccs->usage.kind)) {
            applyParameterManipulationToFunction(nameOnPoint, allrefs, PPC_AVR_DEL_PARAMETER, 1, 1);
            //& deleteParameter(point, nameOnPoint, 1, UsageDefined);
        }
    }

    // TODO!!! add safety checks, as changing the profile of the method
    // can introduce new conflicts

    freeEditorMarkersAndMarkerList(allrefs);
    allrefs = NULL;

    applyWholeRefactoringFromUndo();
    ppcGotoMarker(point);
}

static int noSpaceChar(int c) {
    return !isspace(c);
}

static void pushMethodSymbolsPlusThoseWithClearedRegion(EditorMarker *m1, EditorMarker *m2) {
    char        spaces[REFACTORING_TMP_STRING_SIZE];
    EditorUndo *undoMark;
    int         slen;

    assert(m1->buffer == m2->buffer);
    undoMark = editorUndo;
    pushAllReferencesOfMethod(m1, NULL);
    slen = m2->offset - m1->offset;
    assert(slen >= 0 && slen < REFACTORING_TMP_STRING_SIZE);
    memset(spaces, ' ', slen);
    spaces[slen] = 0;
    replaceString(m1, slen, spaces);
    pushAllReferencesOfMethod(m1, NULL);
    editorUndoUntil(undoMark, NULL);
}

static int isMethodPartRedundant(EditorMarker *m1, EditorMarker *m2) {
    SymbolsMenu      *mm1, *mm2;
    Reference        *diff;
    EditorMarkerList *lll, *ll;
    bool              res = true;

    pushMethodSymbolsPlusThoseWithClearedRegion(m1, m2);
    assert(sessionData.browserStack.top && sessionData.browserStack.top->previous);
    mm1 = sessionData.browserStack.top->menuSym;
    mm2 = sessionData.browserStack.top->previous->menuSym;
    while (mm1 != NULL && mm2 != NULL && res) {
        //&symbolRefItemDump(&mm1->references); dumpReferences(mm1->references.references);
        //&symbolRefItemDump(&mm2->references); dumpReferences(mm2->references.references);
        olcxReferencesDiff(&mm1->references.references, &mm2->references.references, &diff);
        if (diff != NULL) {
            lll = convertReferencesToEditorMarkers(diff);
            LIST_MERGE_SORT(EditorMarkerList, lll, editorMarkerListBefore);
            for (ll = lll; ll != NULL; ll = ll->next) {
                assert(ll->marker->buffer == m1->buffer);
                //&sprintf(tmpBuff, "checking diff %d", ll->marker->offset); ppcGenRecord(PPC_INFORMATION,
                // tmpBuff);
                if (editorMarkerBefore(ll->marker, m1) || editorMarkerBeforeOrSame(m2, ll->marker)) {
                    res = false;
                }
            }
            freeEditorMarkersAndMarkerList(lll);
            freeReferences(diff);
        }
        mm1 = mm1->next;
        mm2 = mm2->next;
    }
    olcxPopOnly();
    olcxPopOnly();

    return res;
}

static void removeMethodPartIfRedundant(EditorMarker *m, int len) {
    EditorMarker *mm;
    mm = newEditorMarker(m->buffer, m->offset + len);
    if (isMethodPartRedundant(m, mm)) {
        replaceString(m, len, "");
    }
    freeEditorMarker(mm);
}

static int isMethodBegin(int c) {
    return c == '{';
}

static bool staticToDynCanBeThisOccurence(EditorMarker *pp, char *param, int *rlen) {
    char         *pp2;
    EditorMarker *mm;
    bool          res = false;

    mm  = newEditorMarker(pp->buffer, pp->offset);
    pp2 = strchr(param, '.');
    if (pp2 == NULL) {
        *rlen = strlen(param);
        res   = strcmp(getIdentifierOnMarker_static(pp), param) == 0;
        goto fini;
    }
    // param.field so parse it
    if (strncmp(getIdentifierOnMarker_static(mm), param, pp2 - param) != 0)
        goto fini;
    mm->offset += (pp2 - param);
    editorMoveMarkerToNonBlank(mm, 1);
    if (*(MARKER_TO_POINTER(mm)) != '.')
        goto fini;
    mm->offset++;
    editorMoveMarkerToNonBlank(mm, 1);
    if (strcmp(getIdentifierOnMarker_static(mm), pp2 + 1) != 0)
        goto fini;
    *rlen = mm->offset - pp->offset + strlen(pp2 + 1);
    res   = true;
fini:
    freeEditorMarker(mm);
    return res;
}

// ----------------------------------------------- TurnStaticToDynamic

static void turnStaticIntoDynamic(EditorMarker *point) {
    char              nameOnPoint[TMP_STRING_SIZE];
    char              param[REFACTORING_TMP_STRING_SIZE];
    char              tparam[REFACTORING_TMP_STRING_SIZE];
    char              testi[2 * REFACTORING_TMP_STRING_SIZE];
    int               plen, tplen, rlen, argn, bi;
    int               classnum, parclassnum;
    int               progress, count;
    EditorMarker     *mm, *m1, *m2, *pp;
    EditorMarkerList *occs, *poccs;
    EditorUndo       *checkPoint;

    ensureReferencesUpdated(refactoringOptions.project);

    argn = 0;
    sscanf(refactoringOptions.refpar1, "%d", &argn);

    assert(argn != 0);

    strcpy(nameOnPoint, getIdentifierOnMarker_static(point));
    Result res = getParameterNamePosition(point, nameOnPoint, argn);
    if (res != RESULT_OK) {
        ppcGotoMarker(point);
        errorMessage(ERR_INTERNAL, "Can't determine position of parameter");
        return;
    }
    mm = newEditorMarkerForPosition(&parameterPosition);
    if (refactoringOptions.refpar2[0] != 0) {
        sprintf(param, "%s.%s", getIdentifierOnMarker_static(mm), refactoringOptions.refpar2);
    } else {
        sprintf(param, "%s", getIdentifierOnMarker_static(mm));
    }
    plen = strlen(param);

    // TODO!!! precheck
    parseBufferUsingServer(refactoringOptions.project, point, NULL, "-olcxcurrentclass", NULL);
    if (parsedInfo.currentClassAnswer[0] == 0) {
        errorMessage(ERR_INTERNAL, "Can't get current class");
        return;
    }
    classnum = getClassNumFromClassLinkName(parsedInfo.currentClassAnswer, NO_FILE_NUMBER);
    if (classnum == NO_FILE_NUMBER) {
        errorMessage(ERR_INTERNAL, "Problem when getting current class");
        return;
    }

    checkPoint = editorUndo;
    pp         = newEditorMarker(point->buffer, point->offset);
    if (!runWithEditorMarkerUntil(pp, isMethodBegin, 1)) {
        errorMessage(ERR_INTERNAL, "Can't find beginning of method");
        return;
    }
    pp->offset++;
    sprintf(testi, "xxx(%s)", param);
    bi = pp->offset + 3 + plen;
    replaceStringInEditorBuffer(pp->buffer, pp->offset, 0, testi, strlen(testi), &editorUndo);
    pp->offset = bi;
    parseBufferUsingServer(refactoringOptions.project, pp, NULL, "-olcxgetsymboltype", "-no-errors");
    // -no-errors is basically very dangerous in this context, recover it in s_opt
    options.noErrors = 0;
    if (!olstringServed) {
        errorMessage(ERR_ST, "Can't infer type for parameter/field");
        return;
    }
    parclassnum = getClassNumFromClassLinkName(olSymbolClassType, NO_FILE_NUMBER);
    if (parclassnum == NO_FILE_NUMBER) {
        errorMessage(ERR_INTERNAL, "Problem when getting parameter/field class");
        return;
    }
    if (!isSmallerOrEqClass(parclassnum, classnum)) {
        errorMessage(ERR_ST, "Type of parameter.field must be current class or its subclass");
        return;
    }

    editorUndoUntil(checkPoint, NULL);
    freeEditorMarker(pp);

    // O.K. turn it virtual

    // STEP 1) inspect all references and copy the parameter to application object
    strcpy(nameOnPoint, getIdentifierOnMarker_static(point));
    assert(strlen(nameOnPoint) < TMP_STRING_SIZE - 1);
    occs = pushGetAndPreCheckReferences(point, nameOnPoint, STANDARD_SELECT_SYMBOLS_MESSAGE,
                                        PPCV_BROWSER_TYPE_INFO);

    LIST_LEN(count, EditorMarkerList, occs);
    progress = 0;
    for (EditorMarkerList *ll = occs; ll != NULL; ll = ll->next) {
        if (!isDefinitionOrDeclarationUsage(ll->usage.kind)) {
            res = getParameterPosition(ll->marker, nameOnPoint, argn);
            if (res == RESULT_OK) {
                m1 = newEditorMarkerForPosition(&parameterBeginPosition);
                m1->offset++;
                runWithEditorMarkerUntil(m1, noSpaceChar, 1);
                m2 = newEditorMarkerForPosition(&parameterEndPosition);
                m2->offset--;
                runWithEditorMarkerUntil(m2, noSpaceChar, -1);
                m2->offset++;
                tplen = m2->offset - m1->offset;
                assert(tplen < REFACTORING_TMP_STRING_SIZE - 1);
                strncpy(tparam, MARKER_TO_POINTER(m1), tplen);
                tparam[tplen] = 0;
                if (strcmp(tparam, "this") != 0) {
                    if (refactoringOptions.refpar2[0] != 0) {
                        sprintf(tparam + strlen(tparam), ".%s", refactoringOptions.refpar2);
                    }
                    pp = replaceStaticPrefix(ll->marker, tparam);
                    freeEditorMarker(pp);
                }
                freeEditorMarker(m2);
                freeEditorMarker(m1);
            }
        }
        writeRelativeProgress((100*progress++) / count);
    }
    writeRelativeProgress(100);
    // you can remove 'static' now, hope it is not virtual symbol,
    removeModifier(point, SPP_METHOD_DECLARATION_BEGIN_POSITION, "static");

    // TODO verify that new profile does not make clash

    // STEP 2) inspect all usages of parameter and replace them by 'this',
    // remove this this if useless
    poccs = getReferences(mm, STANDARD_SELECT_SYMBOLS_MESSAGE, PPCV_BROWSER_TYPE_INFO);
    for (EditorMarkerList *ll = poccs; ll != NULL; ll = ll->next) {
        if (!isDefinitionOrDeclarationUsage(ll->usage.kind)) {
            if (ll->marker->offset + plen <= ll->marker->buffer->allocation.bufferSize
                // TODO! do this at least little bit better, by skipping spaces, etc.
                && staticToDynCanBeThisOccurence(ll->marker, param, &rlen)) {
                replaceString(ll->marker, rlen, "this");
                removeMethodPartIfRedundant(ll->marker, strlen("this."));
            }
        }
    }

    // STEP 3) remove the parameter if not used anymore
    if (!isThisSymbolUsed(mm)) {
        applyParameterManipulation(point, PPC_AVR_DEL_PARAMETER, argn, 0);
    } else {
        // at least update the progress
        writeRelativeProgress(100);
    }

    // and generate output
    applyWholeRefactoringFromUndo();
    ppcGotoMarker(point);

    // DONE!
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

// ------------------------------------------------------- Encapsulate

static Reference *checkEncapsulateGetterSetterForExistingMethods(char *mname) {
    SymbolsMenu *hk;
    char         clist[REFACTORING_TMP_STRING_SIZE];
    char         cn[TMP_STRING_SIZE];
    Reference   *anotherDefinition = NULL;

    clist[0] = 0;
    assert(sessionData.browserStack.top);
    assert(sessionData.browserStack.top->hkSelectedSym);
    assert(sessionData.browserStack.top->menuSym);
    hk = sessionData.browserStack.top->hkSelectedSym;
    for (SymbolsMenu *mm = sessionData.browserStack.top->menuSym; mm != NULL; mm = mm->next) {
        if (isSameCxSymbol(&mm->references, &hk->references) && mm->defRefn != 0) {
            if (mm->references.vFunClass == hk->references.vFunClass) {
                // find definition of another function
                for (Reference *rr = mm->references.references; rr != NULL; rr = rr->next) {
                    if (isDefinitionUsage(rr->usage.kind)) {
                        if (positionsAreNotEqual(rr->position, hk->defpos)) {
                            anotherDefinition = rr;
                            goto refbreak;
                        }
                    }
                }
            refbreak:;
            } else {
                if (isSmallerOrEqClass(mm->references.vFunClass, hk->references.vFunClass) ||
                    isSmallerOrEqClass(hk->references.vFunClass, mm->references.vFunClass)) {
                    linkNamePrettyPrint(cn, getShortClassNameFromClassNum_st(mm->references.vFunClass),
                                        TMP_STRING_SIZE, SHORT_NAME);
                    if (substringIndex(clist, cn) == -1) {
                        sprintf(clist + strlen(clist), " %s", cn);
                    }
                }
            }
        }
    }
    // O.K. now I have list of classes in clist
    char tmpBuff[TMP_BUFF_SIZE];
    if (clist[0] != 0) {
        sprintf(tmpBuff,
                "The method %s is also defined in the following related classes: %s. Its definition in current "
                "class may (under some circumstance) change your program behaviour. Do you really want to "
                "continue with this refactoring?",
                mname, clist);
        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
        ppcAskConfirmation(tmpBuff);
    }
    if (anotherDefinition != NULL) {
        sprintf(tmpBuff,
                "The method %s is yet defined in this class. C-xrefactory will not generate new method. Continue "
                "anyway?",
                mname);
        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
        ppcAskConfirmation(tmpBuff);
    }
    return anotherDefinition;
}

static void addMethodToForbiddenRegions(Reference *methodRef, EditorRegionList **forbiddenRegions) {
    EditorMarker *mm, *mb, *me;

    mm = newEditorMarkerForPosition(&methodRef->position);
    makeSyntaxPassOnSource(mm);
    mb = newEditorMarkerForPosition(
        &parsedPositions[SPP_METHOD_DECLARATION_BEGIN_POSITION]);
    me = newEditorMarkerForPosition(
        &parsedPositions[SPP_METHOD_DECLARATION_END_POSITION]);
    *forbiddenRegions = newEditorRegionList(mb, me, *forbiddenRegions);
    freeEditorMarker(mm);
}

static void performEncapsulateField(EditorMarker *point, EditorRegionList **forbiddenRegions) {
    char              nameOnPoint[TMP_STRING_SIZE];
    char              upcasedName[TMP_STRING_SIZE];
    char              getter[2 * TMP_STRING_SIZE];
    char              setter[2 * TMP_STRING_SIZE];
    char              cclass[TMP_STRING_SIZE];
    char              getterBody[3 * REFACTORING_TMP_STRING_SIZE];
    char              setterBody[3 * REFACTORING_TMP_STRING_SIZE];
    char              declarator[REFACTORING_TMP_STRING_SIZE];
    char             *scclass;
    int               nameOnPointLen, declLen, indlines, indoffset;
    Reference        *anotherGetter, *anotherSetter;
    unsigned          accFlags;
    EditorMarkerList *occs, *insiders, *outsiders;
    EditorMarker     *dte, *dtb, *de;
    EditorMarker     *getterm, *setterm, *tbeg, *tend;
    EditorUndo       *beforeInsertionUndo;
    EditorMarker     *eqm, *ee, *db;
    UNUSED            db;

    strcpy(nameOnPoint, getIdentifierOnMarker_static(point));
    nameOnPointLen = strlen(nameOnPoint);
    assert(nameOnPointLen < TMP_STRING_SIZE - 1);
    occs = pushGetAndPreCheckReferences(point, nameOnPoint, ERROR_SELECT_SYMBOLS_MESSAGE,
                                        PPCV_BROWSER_TYPE_WARNING);
    for (EditorMarkerList *ll = occs; ll != NULL; ll = ll->next) {
        if (ll->usage.kind == UsageAddrUsed) {
            char tmpBuff[TMP_BUFF_SIZE];
            ppcGotoMarker(ll->marker);
            sprintf(tmpBuff, "There is a combined l-value reference of the field. Current version of C-xrefactory "
                             "doesn't  know how  to encapsulate such  assignment. Please, turn it into simple "
                             "assignment (i.e. field = field 'op' ...;) first.");
            formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
            errorMessage(ERR_ST, tmpBuff);
            return;
        }
    }

    assert(sessionData.browserStack.top && sessionData.browserStack.top->hkSelectedSym);
    accFlags = sessionData.browserStack.top->hkSelectedSym->references.access;

    cclass[0] = 0;
    scclass   = cclass;
    if (accFlags & AccessStatic) {
        getNameOfTheClassAndSuperClass(point, cclass, NULL);
        scclass = lastOccurenceInString(cclass, '.');
        if (scclass == NULL)
            scclass = cclass;
        else
            scclass++;
    }

    strcpy(upcasedName, nameOnPoint);
    upcasedName[0] = toupper(upcasedName[0]);
    sprintf(getter, "get%s", upcasedName);
    sprintf(setter, "set%s", upcasedName);

    // generate getter and setter bodies
    makeSyntaxPassOnSource(point);
    db = newEditorMarkerForPosition(
        &parsedPositions[SPP_FIELD_DECLARATION_BEGIN_POSITION]);
    dtb = newEditorMarkerForPosition(
        &parsedPositions[SPP_FIELD_DECLARATION_TYPE_BEGIN_POSITION]);
    dte = newEditorMarkerForPosition(
        &parsedPositions[SPP_FIELD_DECLARATION_TYPE_END_POSITION]);
    de =
        newEditorMarkerForPosition(&parsedPositions[SPP_FIELD_DECLARATION_END_POSITION]);
    moveMarkerToTheEndOfDefinitionScope(de);
    assert(dtb->buffer == dte->buffer);
    assert(dtb->offset <= dte->offset);
    declLen = dte->offset - dtb->offset;
    strncpy(declarator, MARKER_TO_POINTER(dtb), declLen);
    declarator[declLen] = 0;

    sprintf(getterBody, "public %s%s %s() {\nreturn %s;\n}\n", ((accFlags & AccessStatic) ? "static " : ""),
            declarator, getter, nameOnPoint);
    sprintf(setterBody, "public %s%s %s(%s %s) {\n%s.%s = %s;\nreturn %s;\n}\n",
            ((accFlags & AccessStatic) ? "static " : ""), declarator, setter, declarator, nameOnPoint,
            ((accFlags & AccessStatic) ? scclass : "this"), nameOnPoint, nameOnPoint, nameOnPoint);

    beforeInsertionUndo = editorUndo;
    if (CHAR_BEFORE_MARKER(de) != '\n')
        replaceString(de, 0, "\n");
    tbeg = duplicateEditorMarker(de);
    tbeg->offset--;

    getterm = setterm = NULL;
    getterm           = newEditorMarker(de->buffer, de->offset - 1);
    replaceString(de, 0, getterBody);
    getterm->offset += substringIndex(getterBody, getter) + 1;

    if ((accFlags & AccessFinal) == 0) {
        setterm = newEditorMarker(de->buffer, de->offset - 1);
        replaceString(de, 0, setterBody);
        setterm->offset += substringIndex(setterBody, setter) + 1;
    }
    tbeg->offset++;
    tend = duplicateEditorMarker(de);

    // check if not yet defined or used
    anotherGetter = anotherSetter = NULL;
    pushReferences(getterm, "-olcxrename", NULL, 0);
    anotherGetter = checkEncapsulateGetterSetterForExistingMethods(getter);
    freeEditorMarker(getterm);
    if ((accFlags & AccessFinal) == 0) {
        pushReferences(setterm, "-olcxrename", NULL, 0);
        anotherSetter = checkEncapsulateGetterSetterForExistingMethods(setter);
        freeEditorMarker(setterm);
    }
    if (anotherGetter != NULL || anotherSetter != NULL) {
        if (anotherGetter != NULL) {
            addMethodToForbiddenRegions(anotherGetter, forbiddenRegions);
        }
        if (anotherSetter != NULL) {
            addMethodToForbiddenRegions(anotherSetter, forbiddenRegions);
        }
        editorUndoUntil(beforeInsertionUndo, &editorUndo);
        de->offset = tbeg->offset;
        if (CHAR_BEFORE_MARKER(de) != '\n')
            replaceString(de, 0, "\n");
        tbeg->offset--;
        if (!anotherGetter) {
            replaceString(de, 0, getterBody);
        }
        if ((accFlags & AccessFinal) == 0 && !anotherSetter) {
            replaceString(de, 0, setterBody);
        }
        tend->offset = de->offset;
        tbeg->offset++;
    }
    // do not move this before, as anotherdef reference would be freed!
    if ((accFlags & AccessFinal) == 0)
        olcxPopOnly();
    olcxPopOnly();

    // generate getter and setter invocations
    splitEditorMarkersWithRespectToRegions(&occs, forbiddenRegions, &insiders, &outsiders);
    for (EditorMarkerList *ll = outsiders; ll != NULL; ll = ll->next) {
        if (ll->usage.kind == UsageLvalUsed) {
            makeSyntaxPassOnSource(ll->marker);
            if (parsedPositions[SPP_ASSIGNMENT_OPERATOR_POSITION].file == NO_FILE_NUMBER) {
                errorMessage(ERR_INTERNAL, "Can't get assignment coordinates");
            } else {
                eqm = newEditorMarkerForPosition(
                    &parsedPositions[SPP_ASSIGNMENT_OPERATOR_POSITION]);
                ee = newEditorMarkerForPosition(
                    &parsedPositions[SPP_ASSIGNMENT_END_POSITION]);
                // make it in two steps to move the ll->d marker to the end
                checkedReplaceString(ll->marker, nameOnPointLen, nameOnPoint, "");
                replaceString(ll->marker, 0, setter);
                replaceString(ll->marker, 0, "(");
                removeBlanksAtEditorMarker(ll->marker, 1, &editorUndo);
                checkedReplaceString(eqm, 1, "=", "");
                removeBlanksAtEditorMarker(eqm, 0, &editorUndo);
                replaceString(ee, 0, ")");
                ee->offset--;
                removeBlanksAtEditorMarker(ee, -1, &editorUndo);
                freeEditorMarker(eqm);
                freeEditorMarker(ee);
            }
        } else if (!isDefinitionOrDeclarationUsage(ll->usage.kind)) {
            checkedReplaceString(ll->marker, nameOnPointLen, nameOnPoint, "");
            replaceString(ll->marker, 0, getter);
            replaceString(ll->marker, 0, "()");
        }
    }

    restrictAccessibility(point, SPP_FIELD_DECLARATION_BEGIN_POSITION, AccessPrivate);

    indoffset = tbeg->offset;
    indlines  = countLinesBetweenEditorMarkers(tbeg, tend);

    // and generate output
    applyWholeRefactoringFromUndo();

    // put it here, undo-redo sometimes shifts markers
    de->offset = indoffset;
    ppcGotoMarker(de);
    ppcValueRecord(PPC_INDENT, indlines, "");

    ppcGotoMarker(point);
}

static void selfEncapsulateField(EditorMarker *point) {
    EditorRegionList *forbiddenRegions;
    forbiddenRegions = NULL;
    ensureReferencesUpdated(refactoringOptions.project);
    performEncapsulateField(point, &forbiddenRegions);
}

static void encapsulateField(EditorMarker *point) {
    EditorRegionList *forbiddenRegions;
    EditorMarker     *cb, *ce;

    ensureReferencesUpdated(refactoringOptions.project);

    //&editorDumpMarker(point);
    makeSyntaxPassOnSource(point);
    //&editorDumpMarker(point);
    if (parsedPositions[SPP_CLASS_DECLARATION_BEGIN_POSITION].file == NO_FILE_NUMBER ||
        parsedPositions[SPP_CLASS_DECLARATION_END_POSITION].file == NO_FILE_NUMBER) {
        FATAL_ERROR(ERR_INTERNAL, "can't deetrmine class coordinates", XREF_EXIT_ERR);
    }

    cb = newEditorMarkerForPosition(
        &parsedPositions[SPP_CLASS_DECLARATION_BEGIN_POSITION]);
    ce =
        newEditorMarkerForPosition(&parsedPositions[SPP_CLASS_DECLARATION_END_POSITION]);

    forbiddenRegions = newEditorRegionList(cb, ce, NULL);

    //&editorDumpMarker(point);
    performEncapsulateField(point, &forbiddenRegions);
}

static bool markersAreEqual(EditorMarker *m1, EditorMarker *m2) {
    return m1->buffer == m2->buffer && m1->offset == m2->offset;
}

// -------------------------------------------------- pulling-up/pushing-down

static SymbolsMenu *findSymbolCorrespondingToReferenceWrtPullUpPushDown(SymbolsMenu *menu2, SymbolsMenu *mm1,
                                                                        EditorMarkerList *rr1) {
    SymbolsMenu      *mm2;
    EditorMarkerList *rr2;

    // find corresponding reference
    for (mm2 = menu2; mm2 != NULL; mm2 = mm2->next) {
        if (mm1->references.type != mm2->references.type && mm2->references.type != TypeInducedError)
            continue;
        for (rr2 = mm2->markers; rr2 != NULL; rr2 = rr2->next) {
            if (markersAreEqual(rr1->marker, rr2->marker))
                goto breakrr2;
        }
    breakrr2:
        // check if symbols corresponds
        if (rr2 != NULL && symbolsCorrespondWrtMoving(mm1, mm2, OLO_PP_PRE_CHECK)) {
            goto breakmm2;
        }
        log_trace("Checking %s", mm2->references.linkName);
    }
breakmm2:
    return mm2;
}

static bool isMethodPartRedundantWrtPullUpPushDown(EditorMarker *m1, EditorMarker *m2) {
    SymbolsMenu      *mm1, *mm2;
    bool              isRedundant;
    EditorRegionList *regions;
    EditorBuffer     *buf;

    assert(m1->buffer == m2->buffer);

    regions = NULL;
    buf     = m1->buffer;
    regions = newEditorRegionList(createEditorMarkerForBufferBegin(buf),
                                  duplicateEditorMarker(m1), regions);
    regions = newEditorRegionList(duplicateEditorMarker(m2),
                                  createEditorMarkerForBufferEnd(buf), regions);

    pushMethodSymbolsPlusThoseWithClearedRegion(m1, m2);
    assert(sessionData.browserStack.top && sessionData.browserStack.top->previous);
    mm1 = sessionData.browserStack.top->menuSym;
    mm2 = sessionData.browserStack.top->previous->menuSym;
    createMarkersForAllReferencesInRegions(mm1, &regions);
    createMarkersForAllReferencesInRegions(mm2, &regions);

    isRedundant = true;
    while (mm1 != NULL) {
        for (EditorMarkerList *rr1 = mm1->markers; rr1 != NULL; rr1 = rr1->next) {
            if (findSymbolCorrespondingToReferenceWrtPullUpPushDown(mm2, mm1, rr1) == NULL) {
                isRedundant = false;
                goto fini;
            }
        }
        mm1 = mm1->next;
    }
fini:
    freeEditorMarkersAndRegionList(regions);
    olcxPopOnly();
    olcxPopOnly();
    return isRedundant;
}

static EditorMarkerList *pullUpPushDownDifferences(SymbolsMenu *menu1, SymbolsMenu *menu2,
                                                   ReferencesItem *theMethod) {
    SymbolsMenu      *mm1, *mm2;
    EditorMarkerList *rr, *diff;

    diff = NULL;
    mm1  = menu1;
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
        for (EditorMarkerList *rr1 = mm1->markers; rr1 != NULL; rr1 = rr1->next) {
            mm2 = findSymbolCorrespondingToReferenceWrtPullUpPushDown(menu2, mm1, rr1);
            if (mm2 == NULL) {
                /* TODO Extract to newEditorMarkerList() */
                rr = editorAlloc(sizeof(EditorMarkerList));
                *rr  = (EditorMarkerList){.marker = duplicateEditorMarker(rr1->marker),
                                          .usage  = rr1->usage,
                                          .next   = diff};
                diff = rr;
            }
        }
    cont:
        mm1 = mm1->next;
    }
    return diff;
}

static void pullUpPushDownCheckCorrespondance(SymbolsMenu *menu1, SymbolsMenu *menu2, ReferencesItem *theMethod) {
    EditorMarkerList *diff;

    diff = pullUpPushDownDifferences(menu1, menu2, theMethod);
    if (diff != NULL) {
        showSafetyCheckFailingDialog(&diff, "These references will be  misinterpreted after refactoring");
        freeEditorMarkersAndMarkerList(diff);
        diff = NULL;
        askForReallyContinueConfirmation();
    }
}

static void reduceParenthesesAroundExpression(EditorMarker *mm, char *expression) {
    EditorMarker *lp, *rp, *eb, *ee;
    int           elen;
    makeSyntaxPassOnSource(mm);
    if (parsedPositions[SPP_PARENTHESED_EXPRESSION_LPAR_POSITION].file != NO_FILE_NUMBER) {
        assert(parsedPositions[SPP_PARENTHESED_EXPRESSION_RPAR_POSITION].file != NO_FILE_NUMBER);
        assert(parsedPositions[SPP_PARENTHESED_EXPRESSION_BEGIN_POSITION].file != NO_FILE_NUMBER);
        assert(parsedPositions[SPP_PARENTHESED_EXPRESSION_END_POSITION].file != NO_FILE_NUMBER);
        elen = strlen(expression);
        lp   = newEditorMarkerForPosition(
            &parsedPositions[SPP_PARENTHESED_EXPRESSION_LPAR_POSITION]);
        rp = newEditorMarkerForPosition(
            &parsedPositions[SPP_PARENTHESED_EXPRESSION_RPAR_POSITION]);
        eb = newEditorMarkerForPosition(
            &parsedPositions[SPP_PARENTHESED_EXPRESSION_BEGIN_POSITION]);
        ee = newEditorMarkerForPosition(
            &parsedPositions[SPP_PARENTHESED_EXPRESSION_END_POSITION]);
        if (ee->offset - eb->offset == elen && strncmp(MARKER_TO_POINTER(eb), expression, elen) == 0) {
            replaceString(lp, 1, "");
            replaceString(rp, 1, "");
        }
        freeEditorMarker(lp);
        freeEditorMarker(rp);
        freeEditorMarker(eb);
        freeEditorMarker(ee);
    }
}

static void removeRedundantParenthesesAroundThisOrSuper(EditorMarker *mm, char *keyword) {
    char *ss;

    ss = getIdentifierOnMarker_static(mm);
    if (strcmp(ss, keyword) == 0) {
        reduceParenthesesAroundExpression(mm, keyword);
    }
}

static void reduceCastedThis(EditorMarker *mm, char *superFqtName) {
    EditorMarker *lp, *rp, *eb, *ee, *tb, *te, *rr, *dd;
    char         *ss;
    int           superFqtLen, castExprLen;
    char          castExpr[MAX_FILE_NAME_SIZE];
    superFqtLen = strlen(superFqtName);
    ss          = getIdentifierOnMarker_static(mm);
    if (strcmp(ss, "this") == 0) {
        makeSyntaxPassOnSource(mm);
        if (parsedPositions[SPP_CAST_LPAR_POSITION].file != NO_FILE_NUMBER) {
            assert(parsedPositions[SPP_CAST_RPAR_POSITION].file != NO_FILE_NUMBER);
            assert(parsedPositions[SPP_CAST_EXPRESSION_BEGIN_POSITION].file != NO_FILE_NUMBER);
            assert(parsedPositions[SPP_CAST_EXPRESSION_END_POSITION].file != NO_FILE_NUMBER);
            lp = newEditorMarkerForPosition(&parsedPositions[SPP_CAST_LPAR_POSITION]);
            rp = newEditorMarkerForPosition(&parsedPositions[SPP_CAST_RPAR_POSITION]);
            tb = newEditorMarkerForPosition(
                &parsedPositions[SPP_CAST_TYPE_BEGIN_POSITION]);
            te =
                newEditorMarkerForPosition(&parsedPositions[SPP_CAST_TYPE_END_POSITION]);
            eb = newEditorMarkerForPosition(
                &parsedPositions[SPP_CAST_EXPRESSION_BEGIN_POSITION]);
            ee = newEditorMarkerForPosition(
                &parsedPositions[SPP_CAST_EXPRESSION_END_POSITION]);
            rp->offset++;
            if (ee->offset - eb->offset == 4 /*strlen("this")*/) {
                if (isMethodPartRedundantWrtPullUpPushDown(lp, rp)) {
                    replaceString(lp, rp->offset - lp->offset, "");
                } else if (te->offset - tb->offset == superFqtLen &&
                           strncmp(MARKER_TO_POINTER(tb), superFqtName, superFqtLen) == 0) {
                    // a little bit hacked:  ((superfqt)this).  -> super
                    rr = duplicateEditorMarker(ee);
                    editorMoveMarkerToNonBlank(rr, 1);
                    if (CHAR_ON_MARKER(rr) == ')') {
                        dd = duplicateEditorMarker(rr);
                        dd->offset++;
                        editorMoveMarkerToNonBlank(dd, 1);
                        if (CHAR_ON_MARKER(dd) == '.') {
                            castExprLen = ee->offset - lp->offset;
                            strncpy(castExpr, MARKER_TO_POINTER(lp), castExprLen);
                            castExpr[castExprLen] = 0;
                            reduceParenthesesAroundExpression(mm, castExpr);
                            replaceString(lp, ee->offset - lp->offset, "super");
                        }
                        freeEditorMarker(dd);
                    }
                    freeEditorMarker(rr);
                }
            }
            freeEditorMarker(lp);
            freeEditorMarker(rp);
            freeEditorMarker(tb);
            freeEditorMarker(te);
            freeEditorMarker(eb);
            freeEditorMarker(ee);
        }
    }
}

static bool isThereACastOfThis(EditorMarker *mm) {
    EditorMarker *eb, *ee;
    char         *ss;
    bool          thereIsACast = false;

    ss = getIdentifierOnMarker_static(mm);
    if (strcmp(ss, "this") == 0) {
        makeSyntaxPassOnSource(mm);
        if (parsedPositions[SPP_CAST_LPAR_POSITION].file != NO_FILE_NUMBER) {
            assert(parsedPositions[SPP_CAST_RPAR_POSITION].file != NO_FILE_NUMBER);
            assert(parsedPositions[SPP_CAST_EXPRESSION_BEGIN_POSITION].file != NO_FILE_NUMBER);
            assert(parsedPositions[SPP_CAST_EXPRESSION_END_POSITION].file != NO_FILE_NUMBER);
            eb = newEditorMarkerForPosition(
                &parsedPositions[SPP_CAST_EXPRESSION_BEGIN_POSITION]);
            ee = newEditorMarkerForPosition(
                &parsedPositions[SPP_CAST_EXPRESSION_END_POSITION]);
            if (ee->offset - eb->offset == 4 /*strlen("this")*/) {
                thereIsACast = true;
            }
            freeEditorMarker(eb);
            freeEditorMarker(ee);
        }
    }
    return thereIsACast;
}

static void reduceRedundantCastedThissInMethod(EditorMarker *point, EditorRegionList **methodreg) {
    char  superFqtName[MAX_FILE_NAME_SIZE];
    char *ss;

    getNameOfTheClassAndSuperClass(point, NULL, superFqtName);
    parseBufferUsingServer(refactoringOptions.project, point, NULL, "-olcxmaybethis", NULL);
    olcxPushSpecial(LINK_NAME_MAYBE_THIS_ITEM, OLO_MAYBE_THIS);

    createMarkersForAllReferencesInRegions(sessionData.browserStack.top->menuSym, methodreg);
    for (SymbolsMenu *mm = sessionData.browserStack.top->menuSym; mm != NULL; mm = mm->next) {
        if (mm->selected && mm->visible) {
            for (EditorMarkerList *ll = mm->markers; ll != NULL; ll = ll->next) {
                // casted expression "((cast)this) -> this"
                // casted expression "((cast)this) -> super"
                ss = getIdentifierOnMarker_static(ll->marker);
                if (strcmp(ss, "this") == 0) {
                    reduceCastedThis(ll->marker, superFqtName);
                    removeRedundantParenthesesAroundThisOrSuper(ll->marker, "this");
                    //&removeRedundantParenthesesAroundThisOrSuper(ll->marker, "super");
                }
            }
        }
    }
}

static void expandThissToCastedThisInTheMethod(EditorMarker *point, char *thiscFqtName, char *supercFqtName,
                                               EditorRegionList *methodreg) {
    char thisCast[MAX_FILE_NAME_SIZE];
    char superCast[MAX_FILE_NAME_SIZE];

    sprintf(thisCast, "((%s)this)", thiscFqtName);
    sprintf(superCast, "((%s)this)", supercFqtName);

    parseBufferUsingServer(refactoringOptions.project, point, NULL, "-olcxmaybethis", NULL);
    olcxPushSpecial(LINK_NAME_MAYBE_THIS_ITEM, OLO_MAYBE_THIS);
    createMarkersForAllReferencesInRegions(sessionData.browserStack.top->menuSym, &methodreg);
    for (SymbolsMenu *mm = sessionData.browserStack.top->menuSym; mm != NULL; mm = mm->next) {
        if (mm->selected && mm->visible) {
            for (EditorMarkerList *ll = mm->markers; ll != NULL; ll = ll->next) {
                char *ss = getIdentifierOnMarker_static(ll->marker);
                // add casts only if there is yet this or super
                if (strcmp(ss, "this") == 0) {
                    // check whether there is yet a casted this
                    if (!isThereACastOfThis(ll->marker)) {
                        checkedReplaceString(ll->marker, 4, "this", "");
                        replaceString(ll->marker, 0, thisCast);
                    }
                } else if (strcmp(ss, "super") == 0) {
                    checkedReplaceString(ll->marker, 5, "super", "");
                    replaceString(ll->marker, 0, superCast);
                }
            }
        }
    }
}

static void pushDownPullUp(EditorMarker *point, PushPullDirection direction, int limitIndex) {
    char              sourceFqtName[MAX_FILE_NAME_SIZE];
    char              superFqtName[MAX_FILE_NAME_SIZE];
    char              targetFqtName[MAX_FILE_NAME_SIZE];
    EditorMarker     *target, *movedStart, *mend, *movedEnd, *startMarker, *endMarker;
    EditorRegionList *methodreg;
    SymbolsMenu      *mm1, *mm2;
    ReferencesItem   *theMethod;
    int               size;
    int               lines;
    UNUSED            lines;

    target = getTargetFromOptions();
    if (!validTargetPlace(target, "-olcxmmtarget"))
        return;

    ensureReferencesUpdated(refactoringOptions.project);

    getNameOfTheClassAndSuperClass(point, sourceFqtName, superFqtName);
    getNameOfTheClassAndSuperClass(target, targetFqtName, NULL);
    getMethodLimitsForMoving(point, &movedStart, &mend, limitIndex);
    lines = countLinesBetweenEditorMarkers(movedStart, mend);

    // prechecks
    setMovingPrecheckStandardEnvironment(point, targetFqtName);
    if (limitIndex == SPP_METHOD_DECLARATION_BEGIN_POSITION) {
        // method
        if (direction == PULLING_UP) {
            if (!(tpCheckTargetToBeDirectSubOrSuperClass(REQ_SUPERCLASS, "superclass") &&
                  tpCheckSuperMethodReferencesForPullUp() &&
                  tpCheckMethodReferencesWithApplOnSuperClassForPullUp())) {
                FATAL_ERROR(ERR_INTERNAL, "A trivial precondition failed", XREF_EXIT_ERR);
            }
        } else {
            if (!(tpCheckTargetToBeDirectSubOrSuperClass(REQ_SUBCLASS, "subclass"))) {
                FATAL_ERROR(ERR_INTERNAL, "A trivial precondition failed", XREF_EXIT_ERR);
            }
        }
    } else {
        // field
        if (direction == PULLING_UP) {
            if (!(tpCheckTargetToBeDirectSubOrSuperClass(REQ_SUPERCLASS, "superclass") &&
                  tpPullUpFieldLastPreconditions())) {
                FATAL_ERROR(ERR_INTERNAL, "A trivial precondition failed", XREF_EXIT_ERR);
            }
        } else {
            if (!(tpCheckTargetToBeDirectSubOrSuperClass(REQ_SUBCLASS, "subclass") &&
                  tpPushDownFieldLastPreconditions())) {
                FATAL_ERROR(ERR_INTERNAL, "A trivial precondition failed", XREF_EXIT_ERR);
            }
        }
    }

    methodreg = newEditorRegionList(movedStart, mend, NULL);

    expandThissToCastedThisInTheMethod(point, sourceFqtName, superFqtName, methodreg);

    movedEnd = duplicateEditorMarker(mend);
    movedEnd->offset--;

    // perform moving
    applyExpandShortNames(point);
    size = mend->offset - movedStart->offset;
    pushAllReferencesOfMethod(point, "-olallchecks");
    createMarkersForAllReferencesInRegions(sessionData.browserStack.top->menuSym, NULL);
    assert(sessionData.browserStack.top != NULL && sessionData.browserStack.top->hkSelectedSym != NULL);
    theMethod = &sessionData.browserStack.top->hkSelectedSym->references;
    moveBlockInEditorBuffer(target, movedStart, size, &editorUndo);

    // recompute methodregion, maybe free old methodreg before!!
    startMarker = duplicateEditorMarker(movedStart);
    endMarker   = duplicateEditorMarker(movedEnd);
    endMarker->offset++;

    methodreg = newEditorRegionList(startMarker, endMarker, NULL);

    // checks correspondance
    pushAllReferencesOfMethod(point, "-olallchecks");
    createMarkersForAllReferencesInRegions(sessionData.browserStack.top->menuSym, NULL);
    assert(sessionData.browserStack.top && sessionData.browserStack.top->previous);
    mm1 = sessionData.browserStack.top->previous->menuSym;
    mm2 = sessionData.browserStack.top->menuSym;

    pullUpPushDownCheckCorrespondance(mm1, mm2, theMethod);
    // push down super.method() check
    if (limitIndex == SPP_METHOD_DECLARATION_BEGIN_POSITION) {
        if (direction == PUSHING_DOWN) {
            setMovingPrecheckStandardEnvironment(point, targetFqtName);
            if (!tpCheckSuperMethodReferencesAfterPushDown()) {
                FATAL_ERROR(ERR_INTERNAL, "A trivial precondition failed", XREF_EXIT_ERR);
            }
        }
    }

    // O.K. now repass maybethis and reduce casts on this
    reduceRedundantCastedThissInMethod(point, &methodreg);

    // reduce long names in the method
    reduceNamesAndAddImports(&methodreg, INTERACTIVE_NO);

    // and generate output
    applyWholeRefactoringFromUndo();
}

static void pullUpField(EditorMarker *point) {
    pushDownPullUp(point, PULLING_UP, SPP_FIELD_DECLARATION_BEGIN_POSITION);
}

static void pullUpMethod(EditorMarker *point) {
    pushDownPullUp(point, PULLING_UP, SPP_METHOD_DECLARATION_BEGIN_POSITION);
}

static void pushDownField(EditorMarker *point) {
    pushDownPullUp(point, PUSHING_DOWN, SPP_FIELD_DECLARATION_BEGIN_POSITION);
}

static void pushDownMethod(EditorMarker *point) {
    pushDownPullUp(point, PUSHING_DOWN, SPP_METHOD_DECLARATION_BEGIN_POSITION);
}

// --------------------------------------------------------------------

static char *computeUpdateOptionForSymbol(EditorMarker *point) {
    EditorMarkerList *occs;
    SymbolsMenu      *csym;
    int               hasHeaderReferenceFlag, scope, cat, multiFileRefsFlag, fn;
    int               symtype, storage, accflags;
    char             *selectedUpdateOption;

    assert(point != NULL && point->buffer != NULL);
    currentLanguage = getLanguageFor(point->buffer->name);

    hasHeaderReferenceFlag = 0;
    multiFileRefsFlag      = 0;
    occs                   = getReferences(point, NULL, PPCV_BROWSER_TYPE_WARNING);
    csym                   = sessionData.browserStack.top->hkSelectedSym;
    scope                  = csym->references.scope;
    cat                    = csym->references.category;
    symtype                = csym->references.type;
    storage                = csym->references.storage;
    accflags               = csym->references.access;
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

    if (LANGUAGE(LANG_JAVA)) {
        if (cat == CategoryLocal) {
            // useless to update when there is nothing about the symbol in Tags
            selectedUpdateOption = "";
        } else if (symtype == TypeDefault && (storage == StorageMethod || storage == StorageField) &&
                   ((accflags & AccessPrivate) != 0)) {
            // private field or method,
            // no update makes renaming after extract method much faster
            selectedUpdateOption = "";
        } else {
            selectedUpdateOption = "-fastupdate";
        }
    } else {
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
    }

    freeEditorMarkersAndMarkerList(occs);
    occs = NULL;
    olcxPopOnly();

    return selectedUpdateOption;
}

// --------------------------------------------------------------------

void refactory(void) {
    int           argCount;
    char         *file, *argumentFile;
    char          inputFileName[MAX_FILE_NAME_SIZE];
    EditorBuffer *buf;
    EditorMarker *point, *mark;

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

    argCount     = 0;
    argumentFile = getNextScheduledFile(&argCount);
    if (argumentFile == NULL) {
        file = NULL;
    } else {
        strcpy(inputFileName, argumentFile);
        file = inputFileName;
    }

    buf = NULL;
    if (file == NULL)
        FATAL_ERROR(ERR_ST, "no input file", XREF_EXIT_ERR);

    buf = findEditorBufferForFile(file);

    point = getPointFromOptions(buf);
    mark  = getMarkFromOptions(buf);

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
    case AVR_ADD_TO_IMPORT:
        progressFactor = 2;
        addToImports(point);
        break;
    case AVR_ADD_PARAMETER:
    case AVR_DEL_PARAMETER:
    case AVR_MOVE_PARAMETER:
        progressFactor = 3;
        updateOption   = computeUpdateOptionForSymbol(point);
        currentLanguage = getLanguageFor(file);
        if (LANGUAGE(LANG_JAVA))
            progressFactor++;
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
    case AVR_MOVE_STATIC_METHOD:
        progressFactor = 4;
        moveStaticMethod(point);
        break;
    case AVR_MOVE_CLASS:
        progressFactor = 3;
        moveClass(point);
        break;
    case AVR_MOVE_CLASS_TO_NEW_FILE:
        progressFactor = 3;
        moveClassToNewFile(point);
        break;
    case AVR_MOVE_ALL_CLASSES_TO_NEW_FILE:
        progressFactor = 3;
        moveAllClassesToNewFile(point);
        break;
    case AVR_PULL_UP_METHOD:
        progressFactor = 2;
        pullUpMethod(point);
        break;
    case AVR_PULL_UP_FIELD:
        progressFactor = 2;
        pullUpField(point);
        break;
    case AVR_PUSH_DOWN_METHOD:
        progressFactor = 2;
        pushDownMethod(point);
        break;
    case AVR_PUSH_DOWN_FIELD:
        progressFactor = 2;
        pushDownField(point);
        break;
    case AVR_TURN_STATIC_METHOD_TO_DYNAMIC:
        progressFactor = 6;
        turnStaticIntoDynamic(point);
        break;
    case AVR_TURN_DYNAMIC_METHOD_TO_STATIC:
        progressFactor = 4;
        refactorVirtualToStatic(point);
        break;
    case AVR_EXTRACT_METHOD:
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
    case AVR_SELF_ENCAPSULATE_FIELD:
        progressFactor = 3;
        selfEncapsulateField(point);
        break;
    case AVR_ENCAPSULATE_FIELD:
        progressFactor = 3;
        encapsulateField(point);
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
