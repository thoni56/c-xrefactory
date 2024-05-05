#include "refactory.h"

/* Main is currently needed for:
   mainTaskEntryInitialisations
   mainOpenOutputFile
 */
#include "commons.h"
#include "cxref.h"
#include "editor.h"
#include "filetable.h"
#include "globals.h"
#include "head.h"
#include "list.h"
#include "main.h"
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
    int xrefUpdateOptionsCount;

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
    xrefUpdateOptionsCount = argument_count(xrefUpdateOptions);
    for (int i = 1; i < xrefUpdateOptionsCount; i++) {
        argumentVector[argumentCount++] = xrefUpdateOptions[i];
    }
    argumentVector[argumentCount++] = updateOption;

    currentPass = ANY_PASS;
    mainTaskEntryInitialisations(argumentCount, argumentVector);

    callXref(argumentCount, argumentVector, true);

    deepCopyOptionsFromTo(&savedOptions, &options);
    ppcEnd(PPC_UPDATE_REPORT);

    // return into editSubTaskState
    mainTaskEntryInitialisations(argument_count(serverStandardOptions), serverStandardOptions);
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
    callServer(argument_count(serverStandardOptions), serverStandardOptions, argumentCount, argumentVector,
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
        processOptions(argument_count(serverStandardOptions), serverStandardOptions, DONT_PROCESS_FILE_ARGUMENTS);
        getPipedOptions(&argumentCount, &argumentVectorP);
        openOutputFile(refactoringOptions.outputFileName);
        if (argumentCount <= 1)
            break;
        initServer(argumentCount, argumentVectorP);
        if (options.continueRefactoring != RC_NONE)
            break;
        callServer(argument_count(serverStandardOptions), serverStandardOptions, argumentCount, argumentVectorP,
                   &editServerSubTaskFirstPass);
        answerEditAction();
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

    assert(sessionData.browserStack.top != NULL);
    if (sessionData.browserStack.top->hkSelectedSym == NULL) {
        errorMessage(ERR_INTERNAL, "no symbol found for refactoring push");
    }
    olCreateSelectionMenu(sessionData.browserStack.top->command);
    if (resolveMessage != NULL && olcxShowSelectionMenu()) {
        displayResolutionDialog(resolveMessage, messageType);
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

// ------------------------- Trivial prechecks --------------------------------------

static void askForReallyContinueConfirmation(void) {
    ppcAskConfirmation("The refactoring may change program behaviour, really continue?");
}

// ---------------------------------------------------------------------------------

static bool handleSafetyCheckDifferenceLists(EditorMarkerList *diff1, EditorMarkerList *diff2,
                                             OlcxReferences *diffrefs) {
    if (diff1 != NULL || diff2 != NULL) {
        for (SymbolsMenu *mm = diffrefs->menuSym; mm != NULL; mm = mm->next) {
            mm->selected = true;
            mm->visible  = true;
            mm->ooBits   = 07777777;
            // hack, freeing now all diffs computed by old method
            freeReferences(mm->references.references);
            mm->references.references = NULL;
        }
        pushMarkersAsReferences(&diff1, diffrefs, LINK_NAME_SAFETY_CHECK_MISSED);
        pushMarkersAsReferences(&diff2, diffrefs, LINK_NAME_SAFETY_CHECK_MISSED);
        freeEditorMarkerListButNotMarkers(diff1);
        freeEditorMarkerListButNotMarkers(diff2);
        olcxPopOnly();
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

static void simpleModuleRename(EditorMarkerList *occs, char *symname, char *symLinkName) {
    char          rtpack[MAX_FILE_NAME_SIZE];
    char          rtprefix[MAX_FILE_NAME_SIZE];
    char         *ss;
    int           snlen, slnlen;
    bool          mvfile;
    EditorMarker *pp;

    /* THIS IS THE OLD JAVA VERSION, NEED TO ADAPT TO MOVE C MODULE!!! */

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

static void simpleRename(EditorMarkerList *markerList, EditorMarker *marker, char *symbolName,
                         char *symbolLinkName
) {
    if (refactoringOptions.theRefactoring == AVR_RENAME_MODULE) {
        simpleModuleRename(markerList, symbolName, symbolLinkName);
    } else {
        for (EditorMarkerList *l = markerList; l != NULL; l = l->next) {
            renameFromTo(l->marker, symbolName, refactoringOptions.renameTo);
        }
        ppcGotoMarker(marker);
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
    symLinkName    = csym->references.linkName;
    undoStartPoint = editorUndo;

    multipleOccurencesSafetyCheck();

    simpleRename(occs, point, nameOnPoint, symLinkName);
    //&dumpEditorBuffers();
    redoTrack = NULL;
    if (!makeSafetyCheckAndUndo(point, &occs, undoStartPoint, &redoTrack)) {
        askForReallyContinueConfirmation();
    }

    editorApplyUndos(redoTrack, NULL, NULL, GEN_FULL_OUTPUT);

    // finish where you have started
    ppcGotoMarker(point);

    freeEditorMarkersAndMarkerList(occs); // O(n^2)!

    if (refactoringOptions.theRefactoring == AVR_RENAME_MODULE) {
        ppcGenRecord(PPC_INFORMATION, "\nDone.\nDo not forget to remove .class files of former package");
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

// ------------------------------------------------------ ExtractMethod

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
    currentLanguage = getLanguageFor(point->buffer->name);

    bool hasHeaderReferences = false;
    bool isMultiFileReferences = false;
    EditorMarkerList *markerList = getReferences(point, NULL, PPCV_BROWSER_TYPE_WARNING);
    SymbolsMenu *menu = sessionData.browserStack.top->hkSelectedSym;
    ReferenceScope scope = menu->references.scope;
    ReferenceCategory cat = menu->references.category;

    if (markerList == NULL) {
        fileNumber = NO_FILE_NUMBER;
    } else {
        assert(markerList->marker != NULL && markerList->marker->buffer != NULL);
        fileNumber = markerList->marker->buffer->fileNumber;
    }
    for (EditorMarkerList *l = markerList; l != NULL; l = l->next) {
        assert(l->marker != NULL && l->marker->buffer != NULL);
        FileItem *fileItem = getFileItem(l->marker->buffer->fileNumber);
        if (fileNumber != l->marker->buffer->fileNumber) {
            isMultiFileReferences = true;
        }
        if (!fileItem->isArgument) {
            hasHeaderReferences = true;
        }
    }

    if (cat == CategoryLocal) {
        // useless to update when there is nothing about the symbol in Tags
        selectedUpdateOption = "";
    } else if (hasHeaderReferences) {
        // once it is in a header, full update is required
        selectedUpdateOption = "-update";
    } else if (scope == ScopeAuto || scope == ScopeFile) {
        // for example a local var or a static function not used in any header
        if (isMultiFileReferences) {
            errorMessage(ERR_INTERNAL, "something goes wrong, a local symbol is used in several files");
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

    freeEditorMarkersAndMarkerList(markerList);
    markerList = NULL;
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
    mainTaskEntryInitialisations(argument_count(serverStandardOptions), serverStandardOptions);
    editServerSubTaskFirstPass = true;

    progressFactor = 1;

    switch (refactoringOptions.theRefactoring) {
    case AVR_RENAME_SYMBOL:
        progressFactor = 3;
        updateOption   = computeUpdateOptionForSymbol(point);
        renameAtPoint(point);
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

    closeMainOutputFile();
    ppcSynchronize();

    // exiting, put undefined, so that main will finish
    options.mode = UndefinedMode;

    LEAVE();
}
