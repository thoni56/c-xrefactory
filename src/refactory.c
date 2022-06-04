#include "refactory.h"

/* Main is currently needed for:
   writeRelativeProgress
   copyOptions
   mainTaskEntryInitialisations
   mainCallXref
   mainCallEditServerInit
   mainCallEditServer
   mainCloseOutputFile
   processOptions
   getPipedOptions
   mainOpenOutputFile
   mainSetLanguage
   getInputFile
 */
#include "main.h"
#include "options.h"
#include "misc.h"
#include "complete.h"
#include "commons.h"
#include "globals.h"
#include "proto.h"
#include "yylex.h"
#include "cxref.h"
#include "cxfile.h"
#include "jsemact.h"
#include "editor.h"
#include "reftab.h"
#include "list.h"
#include "classhierarchy.h"
#include "protocol.h"
#include "filetable.h"

#include "log.h"
#include "utils.h"

#define RRF_CHARS_TO_PRE_CHECK_AROUND       1
#define MAX_NARGV_OPTIONS_COUNT               50

typedef struct tpCheckSpecialReferencesData {
    struct pushAllInBetweenData	mm;
    char						*symbolToTest;
    int							classToTest;
    struct referencesItem	*foundSpecialRefItem;
    struct reference			*foundSpecialR;
    struct referencesItem  *foundRefToTestedClass;
    struct referencesItem  *foundRefNotToTestedClass;
    struct reference            *foundOuterScopeRef;
} S_tpCheckSpecialReferencesData;

typedef struct disabledList {
    int                  file;
    int                  clas;
    struct disabledList  *next;
} S_disabledList;

static EditorUndo *refactoringStartingPoint;

static bool refactoryXrefEditServerSubTaskFirstPass = true;

static char *refactoryEditServInitOptions[] = {
    "xref",
    "-xrefactory-II",
    //& "-debug",
    "-task_regime_server",
    NULL,
};

// Refactory will always use xref2 protocol and inhibit a few messages when generating/updating xrefs
static char *refactoryXrefInitOptions[] = {
    "xref",
    "-xrefactory-II",
    "-briefoutput",
    NULL,
};

static char *refactoryUpdateOption = "-fastupdate";


static bool moveClassMapFunReturnOnUninterestingSymbols(ReferencesItem *ri, TpCheckMoveClassData *dd) {
    if (!isPushAllMethodsValidRefItem(ri))
        return true;
    /* this is too strong, but check only fields and methods */
    if (ri->storage!=StorageField
        && ri->storage!=StorageMethod
        && ri->storage!=StorageConstructor)
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
    for (s=ss; *s; s++) {
        if (*s == '/' || *s == '\\' || *s=='$') *s = '.';
    }
}

static int argument_count(char **argv) {
    int count;
    for (count=0; *argv!=NULL; count++,argv++)
        ;
    return count;
}

static bool filter0(Reference *reference, void *dummy) {
    return reference->usage.kind < UsageMaxOLUsages;
}

static void refactorySetArguments(char *argv[MAX_NARGV_OPTIONS_COUNT],
                                  EditorBuffer *buf,
                                  char *project,
                                  EditorMarker *point,
                                  EditorMarker *mark
) {
    static char optPoint[TMP_STRING_SIZE];
    static char optMark[TMP_STRING_SIZE];
    static char optXrefrc[MAX_FILE_NAME_SIZE];
    int i;
    i = 0;
    argv[i] = "null";
    i++;
    if (refactoringOptions.xrefrc!=NULL) {
        sprintf(optXrefrc, "-xrefrc=%s", refactoringOptions.xrefrc);
        assert(strlen(optXrefrc)+1 < MAX_FILE_NAME_SIZE);
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
    if (project!=NULL) {
        argv[i] = "-p";
        i++;
        argv[i] = project;
        i++;
    }
    assert(i < MAX_NARGV_OPTIONS_COUNT);
    if (point!=NULL) {
        sprintf(optPoint, "-olcursor=%d", point->offset);
        argv[i] = optPoint;
        i++;
    }
    assert(i < MAX_NARGV_OPTIONS_COUNT);
    if (mark!=NULL) {
        sprintf(optMark, "-olmark=%d", mark->offset);
        argv[i] = optMark;
        i++;
    }
    assert(i < MAX_NARGV_OPTIONS_COUNT);
    if (buf!=NULL) {
        // if following assertion does not fail, you can delet buf parameter
        assert(buf == point->buffer);
        argv[i] = buf->name;
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
static void refactoryUpdateReferences(char *project) {
    int nargc, refactoryXrefInitOptionsNum;
    char *nargv[MAX_NARGV_OPTIONS_COUNT];

    // following would be too long to be allocated on stack
    static Options savedOptions;

    if (refactoryUpdateOption==NULL || *refactoryUpdateOption==0) {
        writeRelativeProgress(100);
        return;
    }

    ppcBegin(PPC_UPDATE_REPORT);

    editorQuasiSaveModifiedBuffers();

    copyOptions(&savedOptions, &options);

    refactorySetArguments(nargv, NULL, project, NULL, NULL);
    nargc = argument_count(nargv);
    refactoryXrefInitOptionsNum = argument_count(refactoryXrefInitOptions);
    for (int i=1; i<refactoryXrefInitOptionsNum; i++) {
        nargv[nargc++] = refactoryXrefInitOptions[i];
    }
    nargv[nargc++] = refactoryUpdateOption;

    currentPass = ANY_PASS;
    mainTaskEntryInitialisations(nargc, nargv);

    mainCallXref(nargc, nargv);

    copyOptions(&options, &savedOptions);
    ppcEnd(PPC_UPDATE_REPORT);

    // return into editSubTaskState
    mainTaskEntryInitialisations(argument_count(refactoryEditServInitOptions),
                                 refactoryEditServInitOptions);
    refactoryXrefEditServerSubTaskFirstPass = true;
    return;
}

static void refactoryEditServerParseBuffer(char *project,
                                           EditorBuffer *buf,
                                           EditorMarker *point, EditorMarker *mark,
                                           char *pushOption, char *pushOption2
) {
    char *nargv[MAX_NARGV_OPTIONS_COUNT];
    int nargc;

    currentPass = ANY_PASS;

    assert(options.taskRegime == RegimeEditServer);

    refactorySetArguments(nargv, buf, project, point, mark);
    nargc = argument_count(nargv);
    if (pushOption!=NULL) {
        nargv[nargc++] = pushOption;
    }
    if (pushOption2!=NULL) {
        nargv[nargc++] = pushOption2;
    }
    mainCallEditServerInit(nargc, nargv);
    mainCallEditServer(argument_count(refactoryEditServInitOptions),
                       refactoryEditServInitOptions,
                       nargc, nargv, &refactoryXrefEditServerSubTaskFirstPass);
}

static void refactoryBeInteractive(void) {
    int pargc;
    char **pargv;

    ENTER();
    copyOptions(&savedOptions, &options);
    for (;;) {
        closeMainOutputFile();
        ppcSynchronize();
        copyOptions(&options, &savedOptions);
        processOptions(argument_count(refactoryEditServInitOptions),
                       refactoryEditServInitOptions, INFILES_DISABLED);
        getPipedOptions(&pargc, &pargv);
        mainOpenOutputFile(refactoringOptions.outputFileName);
        if (pargc <= 1)
            break;
        mainCallEditServerInit(pargc, pargv);
        if (options.continueRefactoring != RC_NONE)
            break;
        mainCallEditServer(argument_count(refactoryEditServInitOptions),
                           refactoryEditServInitOptions,
                           pargc, pargv, &refactoryXrefEditServerSubTaskFirstPass);
        mainAnswerEditAction();
    }
    LEAVE();
}

// -------------------- end of interface to edit server sub-task ----------------------
////////////////////////////////////////////////////////////////////////////////////////


void refactoryDisplayResolutionDialog(char *message,int messageType,int continuation) {
    char buf[TMP_BUFF_SIZE];
    strcpy(buf, message);
    formatOutputLine(buf, ERROR_MESSAGE_STARTING_OFFSET);
    ppcDisplaySelection(buf, messageType, continuation);
    refactoryBeInteractive();
}

#define STANDARD_SELECT_SYMBOLS_MESSAGE "Select classes in left window. These classes will be processed during refactoring. It is highly recommended to process whole hierarchy of related classes all at once. Unselection of any class and its exclusion from refactoring may cause changes in your program behaviour."
#define STANDARD_C_SELECT_SYMBOLS_MESSAGE "There are several symbols referred from this place. Continuing this refactoring will process the selected symbols all at once."
#define ERROR_SELECT_SYMBOLS_MESSAGE "If you see this message, then probably something is going wrong. You are refactoring a virtual method when only statically linked symbol is required. It is strongly recommended to cancel the refactoring."

static void refactoryPushReferences(EditorBuffer *buf, EditorMarker *point,
                                    char *pushOption, char *resolveMessage,
                                    int messageType
) {
    /* now remake task initialisation as for edit server */
    refactoryEditServerParseBuffer(refactoringOptions.project, buf, point, NULL, pushOption, NULL);

    assert(sessionData.browserStack.top!=NULL);
    if (sessionData.browserStack.top->hkSelectedSym==NULL) {
        errorMessage(ERR_INTERNAL, "no symbol found for refactoring push");
    }
    olCreateSelectionMenu(sessionData.browserStack.top->command);
    if (resolveMessage!=NULL && olcxShowSelectionMenu()) {
        refactoryDisplayResolutionDialog(resolveMessage, messageType,CONTINUATION_ENABLED);
    }
}

static void refactorySafetyCheck(char *project, EditorBuffer *buf, EditorMarker *point) {
    // !!!!update references MUST be followed by a pushing action, to refresh options
    refactoryUpdateReferences(refactoringOptions.project);
    refactoryEditServerParseBuffer(project, buf, point,NULL, "-olcxsafetycheck2",NULL);

    assert(sessionData.browserStack.top!=NULL);
    if (sessionData.browserStack.top->hkSelectedSym==NULL) {
        errorMessage(ERR_ST, "No symbol found for refactoring safety check");
    }
    olCreateSelectionMenu(sessionData.browserStack.top->command);
    if (safetyCheck2ShouldWarn()) {
        char tmpBuff[TMP_BUFF_SIZE];
        if (LANGUAGE(LANG_JAVA)) {
            sprintf(tmpBuff, "This is class hierarchy of given symbol as it will appear after the refactoring. It does not correspond to the hierarchy before the refactoring. It is probable that the refactoring will not be behaviour preserving. If you are not sure about your action, you should abandon this refactoring!");
        } else {
            sprintf(tmpBuff, "These symbols will be refererred at this place after the refactoring. It is probable that the refactoring will not be behaviour preserving. If you are not sure about your action, you should abandon this refactoring!");
        }
        refactoryDisplayResolutionDialog(tmpBuff, PPCV_BROWSER_TYPE_WARNING,CONTINUATION_ENABLED);
    }
}

static char *refactoryGetIdentifierOnMarker_st(EditorMarker *pos) {
    EditorBuffer  *buff;
    char            *s, *e, *smax, *smin;
    static char     res[TMP_STRING_SIZE];
    int             reslen;
    buff = pos->buffer;
    assert(buff && buff->allocation.text && pos->offset<=buff->allocation.bufferSize);
    s = buff->allocation.text + pos->offset;
    smin = buff->allocation.text;
    smax = buff->allocation.text + buff->allocation.bufferSize;
    // move to the beginning of identifier
    for (; s>=smin && (isalpha(*s) || isdigit(*s) || *s=='_' || *s=='$'); s--)
        ;
    for (s++; s<smax && isdigit(*s); s++)
        ;
    // now get it
    for (e=s; e<smax && (isalpha(*e) || isdigit(*e) || *e=='_' || *e=='$'); e++)
        ;
    reslen = e-s;
    assert(reslen < TMP_STRING_SIZE-1);
    strncpy(res, s, reslen);
    res[reslen] = 0;

    return res;
}

static void refactoryReplaceString(EditorMarker *pos, int len, char *newVal) {
    editorReplaceString(pos->buffer, pos->offset, len,
                        newVal, strlen(newVal), &editorUndo);
}

static void refactoryCheckedReplaceString(EditorMarker *pos, int len,
                                          char *oldVal, char *newVal) {
    char    *bVal;
    int     check, d;

    bVal = pos->buffer->allocation.text + pos->offset;
    check = (strlen(oldVal)==len && strncmp(oldVal, bVal, len) == 0);
    if (check) {
        refactoryReplaceString(pos, len, newVal);
    } else {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "checked replacement of %s to %s failed on ", oldVal, newVal);
        d = strlen(tmpBuff);
        for (int i=0; i<len; i++)
            tmpBuff[d++] = bVal[i];
        tmpBuff[d++]=0;
        errorMessage(ERR_INTERNAL, tmpBuff);
    }
}

// -------------------------- Undos

static void editorFreeSingleUndo(EditorUndo *uu) {
    if (uu->u.replace.str!=NULL && uu->u.replace.strlen!=0) {
        switch (uu->operation) {
        case UNDO_REPLACE_STRING:
            ED_FREE(uu->u.replace.str, uu->u.replace.strlen+1);
            break;
        case UNDO_RENAME_BUFFER:
            ED_FREE(uu->u.rename.name, strlen(uu->u.rename.name)+1);
            break;
        case UNDO_MOVE_BLOCK:
            break;
        default:
            errorMessage(ERR_INTERNAL,"Unknown operation to undo");
        }
    }
    ED_FREE(uu, sizeof(EditorUndo));
}

void editorApplyUndos(EditorUndo *undos, EditorUndo *until,
                      EditorUndo **undoundo, int gen) {
    EditorUndo *uu, *next;
    EditorMarker *m1, *m2;
    uu = undos;
    while (uu!=until && uu!=NULL) {
        switch (uu->operation) {
        case UNDO_REPLACE_STRING:
            if (gen == GEN_FULL_OUTPUT) {
                ppcReplace(uu->buffer->name, uu->u.replace.offset,
                                    uu->buffer->allocation.text+uu->u.replace.offset,
                                    uu->u.replace.size, uu->u.replace.str);
            }
            editorReplaceString(uu->buffer, uu->u.replace.offset, uu->u.replace.size,
                                uu->u.replace.str, uu->u.replace.strlen, undoundo);

            break;
        case UNDO_RENAME_BUFFER:
            if (gen == GEN_FULL_OUTPUT) {
                ppcGotoOffsetPosition(uu->buffer->name, 0);
                ppcGenRecord(PPC_MOVE_FILE_AS, uu->u.rename.name);
            }
            editorRenameBuffer(uu->buffer, uu->u.rename.name, undoundo);
            break;
        case UNDO_MOVE_BLOCK:
            m1 = editorCreateNewMarker(uu->buffer, uu->u.moveBlock.offset);
            m2 = editorCreateNewMarker(uu->u.moveBlock.dbuffer, uu->u.moveBlock.doffset);
            if (gen == GEN_FULL_OUTPUT) {
                ppcGotoMarker(m1);
                ppcValueRecord(PPC_REFACTORING_CUT_BLOCK,uu->u.moveBlock.size,"");
            }
            editorMoveBlock(m2, m1, uu->u.moveBlock.size, undoundo);
            if (gen == GEN_FULL_OUTPUT) {
                ppcGotoMarker(m1);
                ppcGenRecord(PPC_REFACTORING_PASTE_BLOCK, "");
            }
            editorFreeMarker(m2);
            editorFreeMarker(m1);
            break;
        default:
            errorMessage(ERR_INTERNAL,"Unknown operation to undo");
        }
        next = uu->next;
        editorFreeSingleUndo(uu);
        uu = next;
    }
    assert(uu==until);
}

void editorUndoUntil(EditorUndo *until, EditorUndo **undoundo) {
    editorApplyUndos(editorUndo, until, undoundo, GEN_NO_OUTPUT);
    editorUndo = until;
}

static void refactoryApplyWholeRefactoringFromUndo(void) {
    EditorUndo        *redoTrack;
    redoTrack = NULL;
    editorUndoUntil(refactoringStartingPoint, &redoTrack);
    editorApplyUndos(redoTrack, NULL, NULL, GEN_FULL_OUTPUT);
}

static void refactoryFatalErrorOnPosition(EditorMarker *p, int errType, char *message) {
    EditorUndo *redo;
    redo = NULL;
    editorUndoUntil(refactoringStartingPoint, &redo);
    ppcGotoMarker(p);
    fatalError(errType, message, XREF_EXIT_ERR);
    // unreachable, but do the things properly
    editorApplyUndos(redo, NULL, &editorUndo, GEN_NO_OUTPUT);
}

// -------------------------- end of Undos

static void refactoryRemoveNonCommentCode(EditorMarker *m, int len) {
    int             c, nn, n;
    char            *s;
    EditorMarker  *mm;
    assert(m->buffer && m->buffer->allocation.text);
    s = m->buffer->allocation.text + m->offset;
    nn = len;
    mm = editorCreateNewMarker(m->buffer, m->offset);
    if (m->offset + nn > m->buffer->allocation.bufferSize) {
        nn = m->buffer->allocation.bufferSize - m->offset;
    }
    n = 0;
    while(nn>0) {
        c = *s;
        if (c=='/' && nn>1 && *(s+1)=='*' && (nn<=2 || *(s+2)!='&')) {
            // /**/ comment
            refactoryReplaceString(mm, n, "");
            s = mm->buffer->allocation.text + mm->offset;
            s += 2; nn -= 2;
            while (! (*s=='*' && *(s+1)=='/')) {s++; nn--;}
            s += 2; nn -= 2;
            mm->offset = s - mm->buffer->allocation.text;
            n = 0;
        } else if (c=='/' && nn>1 && *(s+1)=='/' && (nn<=2 || *(s+2)!='&')) {
            // // comment
            refactoryReplaceString(mm, n, "");
            s = mm->buffer->allocation.text + mm->offset;
            s += 2; nn -= 2;
            while (*s!='\n') {s++; nn--;}
            s += 1; nn -= 1;
            mm->offset = s - mm->buffer->allocation.text;
            n = 0;
        } else if (c=='"') {
            // string, pass it removing all inside (also /**/ comments)
            s++; nn--; n++;
            while (*s!='"' && nn>0) {
                s++; nn--; n++;
                if (*s=='\\') {
                    s++; nn--; n++;
                    s++; nn--; n++;
                }
            }
        } else {
            s++; nn--; n++;
        }
    }
    if (n>0) {
        refactoryReplaceString(mm, n, "");
    }
    editorFreeMarker(mm);
}

// basically move marker to the first non blank and non comment symbol at the same
// line as the marker is or to the newline character
static void refactoryMoveMarkerToTheEndOfDefinitionScope(EditorMarker *mm) {
    int offset;
    offset = mm->offset;
    editorMoveMarkerToNonBlankOrNewline(mm, 1);
    if (mm->offset >= mm->buffer->allocation.bufferSize) {
        return;
    }
    if (CHAR_ON_MARKER(mm)=='/' && CHAR_AFTER_MARKER(mm)=='/') {
        if (refactoringOptions.commentMovingMode == CM_NO_COMMENT) return;
        editorMoveMarkerToNewline(mm, 1);
        mm->offset ++;
    } else if (CHAR_ON_MARKER(mm)=='/' && CHAR_AFTER_MARKER(mm)=='*') {
        if (refactoringOptions.commentMovingMode == CM_NO_COMMENT) return;
        mm->offset ++; mm->offset ++;
        while (mm->offset<mm->buffer->allocation.bufferSize &&
               (CHAR_ON_MARKER(mm)!='*' || CHAR_AFTER_MARKER(mm)!='/')) {
            mm->offset++;
        }
        if (mm->offset<mm->buffer->allocation.bufferSize) {
            mm->offset ++;
            mm->offset ++;
        }
        offset = mm->offset;
        editorMoveMarkerToNonBlankOrNewline(mm, 1);
        if (CHAR_ON_MARKER(mm)=='\n') mm->offset ++;
        else mm->offset = offset;
    } else if (CHAR_ON_MARKER(mm)=='\n') {
        mm->offset ++;
    } else {
        if (refactoringOptions.commentMovingMode == CM_NO_COMMENT) return;
        mm->offset = offset;
    }
}

static int refactoryMarkerWRTComment(EditorMarker *mm, int *commentBeginOffset) {
    char *b, *s, *e, *mms;
    assert(mm->buffer && mm->buffer->allocation.text);
    s = mm->buffer->allocation.text;
    e = s + mm->buffer->allocation.bufferSize;
    mms = s + mm->offset;
    while(s<e && s<mms) {
        b = s;
        if (*s=='/' && (s+1)<e && *(s+1)=='*') {
            // /**/ comment
            s += 2;
            while ((s+1)<e && ! (*s=='*' && *(s+1)=='/')) s++;
            if (s+1<e) s += 2;
            if (s>mms) {
                *commentBeginOffset = b-mm->buffer->allocation.text;
                return MARKER_IS_IN_STAR_COMMENT;
            }
        } else if (*s=='/' && s+1<e && *(s+1)=='/') {
            // // comment
            s += 2;
            while (s<e && *s!='\n') s++;
            if (s<e) s += 1;
            if (s>mms) {
                *commentBeginOffset = b-mm->buffer->allocation.text;
                return MARKER_IS_IN_SLASH_COMMENT;
            }
        } else if (*s=='"') {
            // string, pass it removing all inside (also /**/ comments)
            s++;
            while (s<e && *s!='"') {
                s++;
                if (*s=='\\') { s++; s++; }
            }
            if (s<e) s++;
        } else {
            s++;
        }
    }
    return MARKER_IS_IN_CODE;
}

static void refactoryMoveMarkerToTheBeginOfDefinitionScope(EditorMarker *mm) {
    int             theBeginningOffset, comBeginOffset, mp;
    int             slashedCommentsProcessed, staredCommentsProcessed;

    slashedCommentsProcessed = staredCommentsProcessed = 0;
    for (;;) {
        theBeginningOffset = mm->offset;
        mm->offset --;
        editorMoveMarkerToNonBlankOrNewline(mm, -1);
        if (CHAR_ON_MARKER(mm)=='\n') {
            theBeginningOffset = mm->offset+1;
            mm->offset --;
        }
        if (refactoringOptions.commentMovingMode == CM_NO_COMMENT)
            goto fini;
        editorMoveMarkerToNonBlank(mm, -1);
        mp = refactoryMarkerWRTComment(mm, &comBeginOffset);
        if (mp == MARKER_IS_IN_CODE)
            goto fini;
        else if (mp == MARKER_IS_IN_STAR_COMMENT) {
            if (refactoringOptions.commentMovingMode == CM_SINGLE_SLASHED)
                goto fini;
            if (refactoringOptions.commentMovingMode == CM_ALL_SLASHED)
                goto fini;
            if (staredCommentsProcessed>0 && refactoringOptions.commentMovingMode==CM_SINGLE_STARRED)
                goto fini;
            if (staredCommentsProcessed>0 && refactoringOptions.commentMovingMode==CM_SINGLE_SLASHED_AND_STARRED)
                goto fini;
            staredCommentsProcessed ++;
            mm->offset = comBeginOffset;
        }
        // slash comment, skip them all
        else if (mp == MARKER_IS_IN_SLASH_COMMENT) {
            if (refactoringOptions.commentMovingMode == CM_SINGLE_STARRED)
                goto fini;
            if (refactoringOptions.commentMovingMode == CM_ALL_STARRED)
                goto fini;
            if (slashedCommentsProcessed>0 && refactoringOptions.commentMovingMode==CM_SINGLE_SLASHED)
                goto fini;
            if (slashedCommentsProcessed>0 && refactoringOptions.commentMovingMode==CM_SINGLE_SLASHED_AND_STARRED)
                goto fini;
            slashedCommentsProcessed ++;
            mm->offset = comBeginOffset;
        } else {
            warningMessage(ERR_INTERNAL, "A new comment?");
            goto fini;
        }
    }
 fini:
    mm->offset = theBeginningOffset;
}

static void refactoryRenameTo(EditorMarker *pos, char *oldName, char *newName) {
    char    *actName;
    int     nlen;
    nlen = strlen(oldName);
    actName = refactoryGetIdentifierOnMarker_st(pos);
    assert(strcmp(actName, oldName)==0);
    refactoryCheckedReplaceString(pos, nlen, oldName, newName);
}

static EditorMarker *refactoryPointMark(EditorBuffer *buf, int offset) {
    EditorMarker *point;
    point = NULL;
    if (offset >= 0) {
        point = editorCreateNewMarker(buf, offset);
    }
    return point;
}

static EditorMarker *refactoryGetPointFromRefactoryOptions(EditorBuffer *buf) {
    assert(buf);
    return refactoryPointMark(buf, refactoringOptions.olCursorPos);
}

static EditorMarker *refactoryGetMarkFromRefactoryOptions(EditorBuffer *buf) {
    assert(buf);
    return refactoryPointMark(buf, refactoringOptions.olMarkPos);
}

static void refactoryPushMarkersAsReferences(EditorMarkerList **markers,
                                             OlcxReferences *refs, char *sym) {
    Reference *rr;

    rr = editorMarkersToReferences(markers);
    for (SymbolsMenu *mm=refs->menuSym; mm!=NULL; mm=mm->next) {
        if (strcmp(mm->s.name, sym)==0) {
            for (Reference *r=rr; r!=NULL; r=r->next) {
                olcxAddReference(&mm->s.references, r, 0);
            }
        }
    }
    olcxFreeReferences(rr);
    olcxRecomputeSelRefs(refs);
}

static bool validTargetPlace(EditorMarker *target, char *checkOpt) {
    bool valid = true;

    refactoryEditServerParseBuffer(refactoringOptions.project, target->buffer, target, NULL, checkOpt, NULL);
    if (!s_cps.moveTargetApproved) {
        valid = false;
        errorMessage(ERR_ST, "Invalid target place");
    }
    return valid;
}

// ------------------------- Trivial prechecks --------------------------------------

static SymbolsMenu *javaGetRelevantHkSelectedItem(ReferencesItem *ri) {
    SymbolsMenu *ss;
    OlcxReferences *rstack;

    rstack = sessionData.browserStack.top;
    for (ss=rstack->hkSelectedSym; ss!=NULL; ss=ss->next) {
        if (isSameCxSymbol(ri, &ss->s) && ri->vFunClass == ss->s.vFunClass) {
            break;
        }
    }

    return ss;
}

static void tpCheckFutureAccOfLocalReferences(ReferencesItem *ri, void *ddd) {
    TpCheckMoveClassData *dd;
    SymbolsMenu *ss;

    dd = (TpCheckMoveClassData *) ddd;
    log_trace("!mapping %s", ri->name);
    if (moveClassMapFunReturnOnUninterestingSymbols(ri, dd))
        return;

    ss = javaGetRelevantHkSelectedItem(ri);
    if (ss!=NULL) {
        // relevant symbol
        for (Reference *rr=ri->references; rr!=NULL; rr=rr->next) {
            // I should check only references from this file
            if (rr->position.file == inputFileNumber) {
                // check if the reference is outside moved class
                if ((!dm_isBetween(cxMemory, rr, dd->mm.minMemi, dd->mm.maxMemi))
                    && OL_VIEWABLE_REFS(rr)) {
                    // yes there is a reference from outside to our symbol
                    ss->selected = true; ss->visible = true;
                    break;
                }
            }
        }
    }
}

static void tpCheckMoveClassPutClassDefaultSymbols(ReferencesItem *ri, void *ddd) {
    OlcxReferences *rstack;
    TpCheckMoveClassData *dd;

    dd = (TpCheckMoveClassData *) ddd;
    log_trace("!mapping %s", ri->name);
    if (moveClassMapFunReturnOnUninterestingSymbols(ri, dd))
        return;

    // fine, add it to Menu, so we will load its references
    for (Reference *rr=ri->references; rr!=NULL; rr=rr->next) {
        log_trace("Checking %d.%d ref of %s", rr->position.line,rr->position.col,ri->name);
        if (IS_PUSH_ALL_METHODS_VALID_REFERENCE(rr, (&dd->mm))) {
            if (IS_DEFINITION_OR_DECL_USAGE(rr->usage.kind)) {
                // definition inside class, default or private acces to be checked
                rstack = sessionData.browserStack.top;
                olAddBrowsedSymbol(ri, &rstack->hkSelectedSym, 1, 1,
                                   0, UsageUsed, 0, &rr->position, rr->usage.kind);
                break;
            }
        }
    }
}

static void tpCheckFutureAccessibilitiesOfSymbolsDefinedInsideMovedClass(TpCheckMoveClassData dd) {
    OlcxReferences *rstack;
    SymbolsMenu **sss;

    rstack = sessionData.browserStack.top;
    rstack->hkSelectedSym = olCreateSpecialMenuItem(LINK_NAME_MOVE_CLASS_MISSED, noFileIndex, StorageDefault);
    // push them into hkSelection,
    mapOverReferenceTableWithPointer(tpCheckMoveClassPutClassDefaultSymbols, &dd);
    // load all theirs references
    olCreateSelectionMenu(rstack->command);
    // mark all as unselected unvisible
    for (SymbolsMenu *ss=rstack->hkSelectedSym; ss!=NULL; ss=ss->next) {
        ss->selected = true; ss->visible = true;
    }
    // checks all references from within this file
    mapOverReferenceTableWithPointer(tpCheckFutureAccOfLocalReferences, &dd);
    // check references from outside
    for (SymbolsMenu *mm=rstack->menuSym; mm!=NULL; mm=mm->next) {
        SymbolsMenu *ss = javaGetRelevantHkSelectedItem(&mm->s);
        if (ss!=NULL && !ss->selected) {
            for (Reference *rr=mm->s.references; rr!=NULL; rr=rr->next) {
                if (rr->position.file != inputFileNumber) {
                    // yes there is a reference from outside to our symbol
                    ss->selected = true; ss->visible = true;
                    goto nextsym;
                }
            }
        }
    nextsym:;
    }

    sss= &rstack->menuSym;
    while (*sss!=NULL) {
        SymbolsMenu *ss = javaGetRelevantHkSelectedItem(&(*sss)->s);
        if (ss!=NULL && ss->selected) {
            sss= &(*sss)->next;
        } else {
            *sss = olcxFreeSymbolMenuItem(*sss);
        }
    }
}

static void tpCheckDefaultAccessibilitiesMoveClass(ReferencesItem *ri, void *ddd) {
    OlcxReferences        *rstack;
    TpCheckMoveClassData  *dd;
    char                    symclass[MAX_FILE_NAME_SIZE];
    int                     sclen, symclen;

    dd = (TpCheckMoveClassData *) ddd;
    //&fprintf(communicationChannel,"!mapping %s\n", ri->name);
    if (moveClassMapFunReturnOnUninterestingSymbols(ri, dd))
        return;

    // check that it is not from moved class
    javaGetClassNameFromFileIndex(ri->vFunClass, symclass, KEEP_SLASHES);
    sclen = strlen(dd->sclass);
    symclen = strlen(symclass);
    if (sclen<=symclen && filenameCompare(dd->sclass, symclass, sclen)==0)
        return;
    // O.K. finally check if there is a reference
    for (Reference *rr=ri->references; rr!=NULL; rr=rr->next) {
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
                                       char *spack, char *tpack, int transPackageMove, char *sclass) {
    checkMoveClassData->mm.minMemi = minMemi;
    checkMoveClassData->mm.maxMemi = maxMemi;
    checkMoveClassData->spack = spack;
    checkMoveClassData->tpack = tpack;
    checkMoveClassData->transPackageMove = transPackageMove;
    checkMoveClassData->sclass = sclass;
}


static void tpCheckFillMoveClassData(TpCheckMoveClassData *dd, char *spack, char *tpack) {
    OlcxReferences *rstack;
    SymbolsMenu *sclass;
    char *targetfile, *srcfile;
    int transPackageMove;

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    sclass = rstack->hkSelectedSym;
    assert(sclass);
    targetfile = options.moveTargetFile;
    assert(targetfile);
    srcfile = inputFilename;
    assert(srcfile);

    javaGetPackageNameFromSourceFileName(srcfile, spack);
    javaGetPackageNameFromSourceFileName(targetfile, tpack);

    // O.K. moving from package spack to package tpack
    if (compareFileNames(spack, tpack)==0) transPackageMove = 0;
    else transPackageMove = 1;

    fillTpCheckMoveClassData(dd, s_cps.cxMemoryIndexAtClassBeginning, s_cps.cxMemoryIndexAtClassEnd,
                               spack, tpack, transPackageMove, sclass->s.name);

}

static bool tpCheckMoveClassAccessibilities(void) {
    OlcxReferences *rstack;
    SymbolsMenu *ss;
    TpCheckMoveClassData dd;
    char spack[MAX_FILE_NAME_SIZE];
    char tpack[MAX_FILE_NAME_SIZE];

    tpCheckFillMoveClassData(&dd, spack, tpack);
    olcxPushSpecialCheckMenuSym(LINK_NAME_MOVE_CLASS_MISSED);
    mapOverReferenceTableWithPointer(tpCheckDefaultAccessibilitiesMoveClass, &dd);

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    if (rstack->references!=NULL) {
        ss = rstack->menuSym;
        assert(ss);
        ss->s.references = olcxCopyRefList(rstack->references);
        rstack->actual = rstack->references;
        if (refactoringOptions.refactoringRegime==RegimeRefactory) {
            refactoryDisplayResolutionDialog(
                                             "These references inside moved class are refering to symbols which will be inaccessible at new class location. You should adjust their access first. (Each symbol is listed only once)",
                                             PPCV_BROWSER_TYPE_WARNING, CONTINUATION_DISABLED);
            refactoryAskForReallyContinueConfirmation();
        }
        return false;
    }
    olStackDeleteSymbol(sessionData.browserStack.top);
    // O.K. now check symbols defined inside the class
    olcxPushEmptyStackItem(&sessionData.browserStack);
    tpCheckFutureAccessibilitiesOfSymbolsDefinedInsideMovedClass(dd);
    rstack = sessionData.browserStack.top;
    if (rstack->menuSym!=NULL) {
        olcxRecomputeSelRefs(rstack);
        // TODO, synchronize this with emacs, but how?
        rstack->refsFilterLevel = RFilterDefinitions;
        if (refactoringOptions.refactoringRegime==RegimeRefactory) {
            refactoryDisplayResolutionDialog(
                                             "These symbols defined inside moved class and used outside the class will be inaccessible at new class location. You should adjust their access first.",
                                             PPCV_BROWSER_TYPE_WARNING, CONTINUATION_DISABLED);
            refactoryAskForReallyContinueConfirmation();
        }
        return false;
    }
    olStackDeleteSymbol(sessionData.browserStack.top);
    return true;
}

static bool tpCheckSourceIsNotInnerClass(void) {
    OlcxReferences *rstack;
    SymbolsMenu *menu;

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    menu = rstack->hkSelectedSym;
    assert(menu);

    // I can rely that it is a class
    int index = getClassNumFromClassLinkName(menu->s.name, noFileIndex);
    //& target = options.moveTargetClass;
    //& assert(target!=NULL);

    int directEnclosingInstanceIndex = getFileItem(index)->directEnclosingInstance;
    if (directEnclosingInstanceIndex != -1 && directEnclosingInstanceIndex != noFileIndex && (menu->s.access&AccessInterface)==0) {
        char tmpBuff[TMP_BUFF_SIZE];
        // If there exists a direct enclosing instance, it is an inner class
        sprintf(tmpBuff, "This is an inner class. Current version of C-xrefactory can only move top level classes and nested classes that are declared 'static'. If the class does not depend on its enclosing instances, you should declare it 'static' and then move it.");
        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
        errorMessage(ERR_ST, tmpBuff);
        return false;
    }
    return true;
}

static void tpCheckSpecialReferencesMapFun(ReferencesItem *ri, void *ddd) {
    S_tpCheckSpecialReferencesData *dd;

    dd = (S_tpCheckSpecialReferencesData *) ddd;
    assert(sessionData.browserStack.top);
    // todo make supermethod symbol special type
    //&fprintf(dumpOut,"! checking %s\n", ri->name);
    if (strcmp(ri->name, dd->symbolToTest)!=0)
        return;
    for (Reference *rr=ri->references; rr!=NULL; rr=rr->next) {
        if (dm_isBetween(cxMemory, rr, dd->mm.minMemi, dd->mm.maxMemi)) {
            // a super method reference
            dd->foundSpecialRefItem = ri;
            dd->foundSpecialR = rr;
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

static void initTpCheckSpecialReferencesData(S_tpCheckSpecialReferencesData *referencesData,
                                               int minMemi,
                                               int maxMemi,
                                               char *symbolToTest,
                                               int classToTest) {
    referencesData->mm.minMemi = minMemi;
    referencesData->mm.maxMemi = maxMemi;
    referencesData->symbolToTest = symbolToTest;
    referencesData->classToTest = classToTest;
    referencesData->foundSpecialRefItem = NULL;
    referencesData->foundSpecialR = NULL;
    referencesData->foundRefToTestedClass = NULL;
    referencesData->foundRefNotToTestedClass = NULL;
    referencesData->foundOuterScopeRef = NULL;
}


static bool tpCheckSuperMethodReferencesInit(S_tpCheckSpecialReferencesData *rr) {
    SymbolsMenu *ss;
    int scl;
    OlcxReferences *rstack;

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    ss = rstack->hkSelectedSym;
    assert(ss);
    scl = javaGetSuperClassNumFromClassNum(ss->s.vApplClass);
    if (scl == noFileIndex) {
        errorMessage(ERR_ST, "no super class, something is going wrong");
        return false;;
    }
    initTpCheckSpecialReferencesData(rr, s_cps.cxMemoryIndexAtMethodBegin,
                                     s_cps.cxMemoryIndexAtMethodEnd,
                                     LINK_NAME_SUPER_METHOD_ITEM, scl);
    mapOverReferenceTableWithPointer(tpCheckSpecialReferencesMapFun, rr);
    return true;
}

static bool tpCheckSuperMethodReferencesForPullUp(void) {
    S_tpCheckSpecialReferencesData      rr;
    OlcxReferences                    *rstack;
    SymbolsMenu                     *ss;
    int                                 tmp;
    char                                tt[TMP_STRING_SIZE];
    char                                ttt[MAX_CX_SYMBOL_SIZE];

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    ss = rstack->hkSelectedSym;
    assert(ss);
    tmp = tpCheckSuperMethodReferencesInit(&rr);
    if (!tmp)
        return false;

    // synthetize an answer
    if (rr.foundRefToTestedClass!=NULL) {
        char tmpBuff[TMP_BUFF_SIZE];
        linkNamePrettyPrint(ttt, ss->s.name, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
        javaGetClassNameFromFileIndex(rr.foundRefToTestedClass->vFunClass, tt, DOTIFY_NAME);
        sprintf(tmpBuff,"'%s' invokes another method using the keyword \"super\" and this invocation is refering to class '%s', i.e. to the class where '%s' will be moved. In consequence, it is not possible to ensure behaviour preseving pulling-up of this method.", ttt, tt, ttt);
        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
        errorMessage(ERR_ST, tmpBuff);
        return false;
    }
    return true;
}

static bool tpCheckSuperMethodReferencesAfterPushDown(void) {
    S_tpCheckSpecialReferencesData rr;
    OlcxReferences *rstack;
    SymbolsMenu *ss;
    char ttt[MAX_CX_SYMBOL_SIZE];

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    ss = rstack->hkSelectedSym;
    assert(ss);
    if (!tpCheckSuperMethodReferencesInit(&rr))
        return false;

    // synthetize an answer
    if (rr.foundRefToTestedClass!=NULL) {
        char tmpBuff[TMP_BUFF_SIZE];
        linkNamePrettyPrint(ttt, ss->s.name, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
        sprintf(tmpBuff,"'%s' invokes another method using the keyword \"super\" and the invoked method is also defined in current class. After pushing down, the reference will be misrelated. In consequence, it is not possible to ensure behaviour preseving pushing-down of this method.", ttt);
        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
        fprintf(communicationChannel,":[warning] %s", tmpBuff);
        //&errorMessage(ERR_ST, tmpBuff);
        return false;
    }
    return true;
}

static bool tpCheckSuperMethodReferencesForDynToSt(void) {
    S_tpCheckSpecialReferencesData rr;

    if (!tpCheckSuperMethodReferencesInit(&rr))
        return false;

    // synthetize an answer
    if (rr.foundSpecialRefItem!=NULL) {
        char tmpBuff[TMP_BUFF_SIZE];
        if (options.xref2) ppcGotoPosition(&rr.foundSpecialR->position);
        sprintf(tmpBuff,"This method invokes another method using the keyword \"super\". Current version of C-xrefactory does not know how to make it static.");
        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
        errorMessage(ERR_ST, tmpBuff);
        return false;
    }
    return true;
}

static bool tpCheckOuterScopeUsagesForDynToSt(void) {
    S_tpCheckSpecialReferencesData  rr;
    SymbolsMenu                     *ss;
    OlcxReferences                    *rstack;

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    ss = rstack->hkSelectedSym;
    assert(ss);
    initTpCheckSpecialReferencesData(&rr, s_cps.cxMemoryIndexAtMethodBegin,
                                     s_cps.cxMemoryIndexAtMethodEnd,
                                     LINK_NAME_MAYBE_THIS_ITEM, ss->s.vApplClass);
    mapOverReferenceTableWithPointer(tpCheckSpecialReferencesMapFun, &rr);
    if (rr.foundOuterScopeRef!=NULL) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff,"Inner class method is using symbols from outer scope. Current version of C-xrefactory does not know how to make it static.");
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
    Reference *rr;
    SymbolsMenu *ss,*mm;
    Symbol *target;
    int srccn;
    char                ttt[MAX_CX_SYMBOL_SIZE];
    int targetcn;
    UNUSED targetcn;

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    ss = rstack->hkSelectedSym;
    assert(ss);
    srccn = ss->s.vApplClass;
    target = getMoveTargetClass();
    assert(target!=NULL && target->u.structSpec);
    targetcn = target->u.structSpec->classFileIndex;
    for (mm=rstack->menuSym; mm!=NULL; mm=mm->next) {
        if (isSameCxSymbol(&ss->s, &mm->s)) {
            if (javaIsSuperClass(mm->s.vApplClass, srccn)) {
                // finally check there is some other reference than super.method()
                // and definition
                for (rr=mm->s.references; rr!=NULL; rr=rr->next) {
                    if ((! IS_DEFINITION_OR_DECL_USAGE(rr->usage.kind))
                        && rr->usage.kind != UsageMethodInvokedViaSuper) {
                        // well there is, issue warning message and finish
                        char tmpBuff[TMP_BUFF_SIZE];
                        linkNamePrettyPrint(ttt, ss->s.name, MAX_CX_SYMBOL_SIZE, SHORT_NAME);
                        sprintf(tmpBuff,"%s is defined also in superclass and there are invocations syntactically refering to one of superclasses. Under some circumstances this may cause that pulling up of this method will not be behaviour preserving.", ttt);
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
    OlcxReferences *rstack;
    SymbolsMenu *ss;
    char ttt[TMP_STRING_SIZE];
    char tt[TMP_STRING_SIZE];
    ClassHierarchyReference *cl;
    Symbol *target;
    bool found = false;

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    ss = rstack->hkSelectedSym;
    assert(ss);
    target = getMoveTargetClass();
    if (target==NULL) {
        errorMessage(ERR_ST, "moving to NULL target class?");
        return false;
    }

    scanForClassHierarchy();
    assert(target->u.structSpec!=NULL&&target->u.structSpec->classFileIndex!=noFileIndex);
    FileItem *fileItem = getFileItem(ss->s.vApplClass);
    if (flag == REQ_SUBCLASS)
        cl=fileItem->inferiorClasses;
    else
        cl=fileItem->superClasses;
    for (; cl!=NULL; cl=cl->next) {
        log_trace("!checking %d(%s) <-> %d(%s)", cl->superClass, getFileItem(cl->superClass)->name,
                  target->u.structSpec->classFileIndex, getFileItem(target->u.structSpec->classFileIndex)->name);
        if (cl->superClass == target->u.structSpec->classFileIndex) {
            found = true;
            break;
        }
    }
    if (found)
        return true;

    javaGetClassNameFromFileIndex(target->u.structSpec->classFileIndex, tt, DOTIFY_NAME);
    javaGetClassNameFromFileIndex(ss->s.vApplClass, ttt, DOTIFY_NAME);
    char tmpBuff[TMP_BUFF_SIZE];
    sprintf(tmpBuff,"Class %s is not direct %s of %s. This refactoring provides moving to direct %ses only.", tt, subOrSuper, ttt, subOrSuper);
    formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
    errorMessage(ERR_ST,tmpBuff);
    return false;
}

bool tpPullUpFieldLastPreconditions(void) {
    OlcxReferences *rstack;
    SymbolsMenu *ss,*mm;
    char ttt[TMP_STRING_SIZE];
    Symbol *target;
    int pcharFlag;
    char tmpBuff[TMP_BUFF_SIZE];

    pcharFlag = 0;
    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    ss = rstack->hkSelectedSym;
    assert(ss);
    target = getMoveTargetClass();
    assert(target!=NULL);
    assert(target->u.structSpec!=NULL&&target->u.structSpec->classFileIndex!=noFileIndex);
    for (mm=rstack->menuSym; mm!=NULL; mm=mm->next) {
        if (isSameCxSymbol(&ss->s,&mm->s)
            && mm->s.vApplClass == target->u.structSpec->classFileIndex) goto cont2;
    }
    // it is O.K. no item found
    return true;
 cont2:
    // an item found, it must be empty
    if (mm->s.references == NULL)
        return true;
    javaGetClassNameFromFileIndex(target->u.structSpec->classFileIndex, ttt, DOTIFY_NAME);
    if (IS_DEFINITION_OR_DECL_USAGE(mm->s.references->usage.kind) && mm->s.references->next==NULL) {
        if (pcharFlag==0) {pcharFlag=1; fprintf(communicationChannel,":[warning] ");}
        sprintf(tmpBuff, "%s is already defined in the superclass %s.  Pulling up will do nothing, but removing the definition from the subclass. You should make sure that both fields are initialized to the same value.", mm->s.name, ttt);
        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
        warningMessage(ERR_ST, tmpBuff);
        return false;
    }
    sprintf(tmpBuff,"There are already references of the field %s syntactically applied on the superclass %s, pulling up this field would cause confusion!", mm->s.name, ttt);
    formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
    errorMessage(ERR_ST, tmpBuff);
    return false;
}

bool tpPushDownFieldLastPreconditions(void) {
    OlcxReferences *rstack;
    SymbolsMenu *ss, *sourcesm, *targetsm;
    char ttt[TMP_STRING_SIZE];
    Reference *rr;
    Symbol *target;
    int thisclassi;
    bool res;
    char tmpBuff[TMP_BUFF_SIZE];

    res = true;
    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    ss = rstack->hkSelectedSym;
    assert(ss);
    thisclassi = ss->s.vApplClass;
    target = getMoveTargetClass();
    assert(target!=NULL);
    assert(target->u.structSpec!=NULL&&target->u.structSpec->classFileIndex!=noFileIndex);
    sourcesm = targetsm = NULL;
    for (SymbolsMenu *mm=rstack->menuSym; mm!=NULL; mm=mm->next) {
        if (isSameCxSymbol(&ss->s,&mm->s)) {
            if (mm->s.vApplClass == target->u.structSpec->classFileIndex) targetsm = mm;
            if (mm->s.vApplClass == thisclassi) sourcesm = mm;
        }
    }
    if (targetsm != NULL) {
        rr = getDefinitionRef(targetsm->s.references);
        if (rr!=NULL && IS_DEFINITION_OR_DECL_USAGE(rr->usage.kind)) {
            javaGetClassNameFromFileIndex(target->u.structSpec->classFileIndex, ttt, DOTIFY_NAME);
            sprintf(tmpBuff,"The field %s is already defined in %s!",
                    targetsm->s.name, ttt);
            formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
            errorMessage(ERR_ST, tmpBuff);
            return false;
        }
    }
    if (sourcesm != NULL) {
        if (sourcesm->s.references!=NULL && sourcesm->s.references->next!=NULL) {
            //& if (pcharFlag==0) {pcharFlag=1; fprintf(communicationChannel,":[warning] ");}
            javaGetClassNameFromFileIndex(thisclassi, ttt, DOTIFY_NAME);
            sprintf(tmpBuff, "There are several references of %s syntactically applied on %s. This may cause that the refactoring will not be behaviour preserving!", sourcesm->s.name, ttt);
            formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
            warningMessage(ERR_ST, tmpBuff);
            res = false;
        }
    }
    return res;
}

// ---------------------------------------------------------------------------------


static bool refactoryHandleSafetyCheckDifferenceLists(
    EditorMarkerList *diff1, EditorMarkerList *diff2, OlcxReferences *diffrefs
) {
    if (diff1!=NULL || diff2!=NULL) {
        //&editorDumpMarkerList(diff1);
        //&editorDumpMarkerList(diff2);
        for (SymbolsMenu *mm=diffrefs->menuSym; mm!=NULL; mm=mm->next) {
            mm->selected = true; mm->visible = true;
            mm->ooBits = 07777777;
            // hack, freeing now all diffs computed by old method
            olcxFreeReferences(mm->s.references); mm->s.references = NULL;
        }
        //&editorDumpMarkerList(diff1);
        //& safetyCheckFailPrepareRefStack();
        //& refactoryPushMarkersAsReferences(&diff1, diffrefs, LINK_NAME_SAFETY_CHECK_LOST);
        //& refactoryPushMarkersAsReferences(&diff2, diffrefs, LINK_NAME_SAFETY_CHECK_FOUND);
        refactoryPushMarkersAsReferences(&diff1, diffrefs, LINK_NAME_SAFETY_CHECK_MISSED);
        refactoryPushMarkersAsReferences(&diff2, diffrefs, LINK_NAME_SAFETY_CHECK_MISSED);
        editorFreeMarkerListNotMarkers(diff1);
        editorFreeMarkerListNotMarkers(diff2);
        olcxPopOnly();
        if (refactoringOptions.theRefactoring == AVR_RENAME_PACKAGE) {
            refactoryDisplayResolutionDialog(
                                             "The package exists and is referenced in the original project. Renaming will join two packages without possibility of inverse refactoring",
                                             PPCV_BROWSER_TYPE_WARNING, CONTINUATION_ENABLED);
        } else {
            refactoryDisplayResolutionDialog(
                                             "These references may be misinterpreted after refactoring",
                                             PPCV_BROWSER_TYPE_WARNING, CONTINUATION_ENABLED);
        }
        return false;
    } else
        return true;
}


static bool refactoryMakeSafetyCheckAndUndo(EditorMarker *point,
    EditorMarkerList **occs, EditorUndo *startPoint,
    EditorUndo **redoTrack
) {
    bool result;
    EditorMarkerList *chks;
    EditorMarker *defin;
    EditorMarkerList *diff1, *diff2;
    OlcxReferences *refs, *origrefs, *newrefs, *diffrefs;
    int pbflag;
    UNUSED pbflag;

    // safety check

    defin = point;
    // find definition reference? why this was there?
    //&for(dd= *occs; dd!=NULL; dd=dd->next) {
    //& if (IS_DEFINITION_USAGE(dd->usg.base)) break;
    //&}
    //&if (dd != NULL) defin = dd->d;

    olcxPushSpecialCheckMenuSym(LINK_NAME_SAFETY_CHECK_MISSED);
    refactorySafetyCheck(refactoringOptions.project, defin->buffer, defin);

    chks = editorReferencesToMarkers(sessionData.browserStack.top->references,filter0, NULL);

    editorMarkersDifferences(occs, &chks, &diff1, &diff2);

    editorFreeMarkersAndMarkerList(chks);

    editorUndoUntil(startPoint, redoTrack);

    origrefs = newrefs = diffrefs = NULL;
    SAFETY_CHECK2_GET_SYM_LISTS(refs,origrefs,newrefs,diffrefs, pbflag);
    assert(origrefs!=NULL && newrefs!=NULL && diffrefs!=NULL);
    result = refactoryHandleSafetyCheckDifferenceLists(diff1, diff2, diffrefs);
    return result;
}

void refactoryAskForReallyContinueConfirmation(void) {
    ppcGenRecord(PPC_ASK_CONFIRMATION,"The refactoring may change program behaviour, really continue?");
}

static void refactoryPreCheckThatSymbolRefsCorresponds(char *oldName, EditorMarkerList *occs) {
    char *cid;
    int off1, off2;
    EditorMarker *pos, *pp;

    for (EditorMarkerList *ll=occs; ll!=NULL; ll=ll->next) {
        pos = ll->marker;
        // first check that I have updated reference
        cid = refactoryGetIdentifierOnMarker_st(pos);
        if (strcmp(cid, oldName)!=0) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff,
                    "something goes wrong: expecting %s instead of %s at %s, offset:%d",
                    oldName, cid, simpleFileName(getRealFileName_static(pos->buffer->name)),
                    pos->offset);
            errorMessage(ERR_INTERNAL, tmpBuff);
            return;
        }
        // O.K. check also few characters around
        off1 = pos->offset - RRF_CHARS_TO_PRE_CHECK_AROUND;
        off2 = pos->offset + strlen(oldName) + RRF_CHARS_TO_PRE_CHECK_AROUND;
        if (off1 < 0)
            off1 = 0;
        if (off2 >= pos->buffer->allocation.bufferSize)
            off2 = pos->buffer->allocation.bufferSize-1;
        pp = editorCreateNewMarker(pos->buffer, off1);
        ppcPreCheck(pp, off2-off1);
        editorFreeMarker(pp);
    }
}

static void refactoryMakeSyntaxPassOnSource(EditorMarker *point) {
    refactoryEditServerParseBuffer(refactoringOptions.project, point->buffer,
                                   point, NULL, "-olcxsyntaxpass",NULL);
    olStackDeleteSymbol(sessionData.browserStack.top);
}

static EditorMarker *refactoryCrNewMarkerForExpressionBegin(EditorMarker *marker, int kind) {
    Position *pos;
    refactoryEditServerParseBuffer(refactoringOptions.project, marker->buffer,
                                   marker ,NULL, "-olcxprimarystart", NULL);
    olStackDeleteSymbol(sessionData.browserStack.top);
    if (kind == GET_PRIMARY_START) {
        pos = &s_primaryStartPosition;
    } else if (kind == GET_STATIC_PREFIX_START) {
        pos = &s_staticPrefixStartPosition;
    } else {
        pos = NULL;
        assert(0);
    }
    if (pos->file == noFileIndex) {
        if (kind == GET_STATIC_PREFIX_START) {
            refactoryFatalErrorOnPosition(marker, ERR_ST, "Can't determine static prefix. Maybe non-static reference to a static object? Make this invocation static before refactoring.");
        } else {
            refactoryFatalErrorOnPosition(marker, ERR_INTERNAL, "Can't determine beginning of primary expression");
        }
        return NULL;
    } else {
        EditorBuffer *buffer = editorGetOpenedAndLoadedBuffer(getFileItem(pos->file)->name);
        EditorMarker *newMarker = editorCreateNewMarker(buffer, 0);
        editorMoveMarkerToLineCol(newMarker, pos->line, pos->col);
        assert(newMarker->buffer == marker->buffer);
        assert(newMarker->offset <= marker->offset);
        return newMarker;
    }
}


static void refactoryCheckedRenameBuffer(EditorBuffer *buff, char *newName, EditorUndo **undo) {
    struct stat stat;
    if (editorFileStatus(newName, &stat)==0) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff, "Renaming buffer %s to an existing file.\nCan I do this?", buff->name);
        ppcGenRecord(PPC_ASK_CONFIRMATION, tmpBuff);
    }
    editorRenameBuffer(buff, newName, undo);
}

static void javaSlashifyDotName(char *ss) {
    char *s;
    for (s=ss; *s; s++) {
        if (*s == '.') *s = FILE_PATH_SEPARATOR;
    }
}

static void refactoryMoveFileAndDirForPackageRename(char *currentPath, EditorMarker *lld, char *symLinkName) {
    char newfile[2*MAX_FILE_NAME_SIZE];
    char packdir[2*MAX_FILE_NAME_SIZE];
    char newpackdir[2*MAX_FILE_NAME_SIZE];
    char path[MAX_FILE_NAME_SIZE];
    int plen;
    strcpy(path, currentPath);
    plen = strlen(path);
    if (plen>0 && (path[plen-1]=='/'||path[plen-1]=='\\')) {
        plen--;
        path[plen]=0;
    }
    sprintf(packdir, "%s%c%s", path, FILE_PATH_SEPARATOR, symLinkName);
    sprintf(newpackdir, "%s%c%s", path, FILE_PATH_SEPARATOR, refactoringOptions.renameTo);
    javaSlashifyDotName(newpackdir+strlen(path));
    sprintf(newfile, "%s%s", newpackdir, lld->buffer->name+strlen(packdir));
    refactoryCheckedRenameBuffer(lld->buffer, newfile, &editorUndo);
}


static bool refactoryRenamePackageFileMove(char *currentPath, EditorMarkerList *ll,
                                           char *symLinkName, int slnlen) {
    int pathLength;
    bool res = false;

    pathLength = strlen(currentPath);
    log_trace("checking %s<->%s, %s<->%s", ll->marker->buffer->name, currentPath,
              ll->marker->buffer->name+pathLength+1, symLinkName);
    if (filenameCompare(ll->marker->buffer->name, currentPath, pathLength) == 0 &&
        ll->marker->buffer->name[pathLength] == FILE_PATH_SEPARATOR &&
        filenameCompare(ll->marker->buffer->name + pathLength + 1, symLinkName, slnlen) == 0)
    {
        refactoryMoveFileAndDirForPackageRename(currentPath, ll->marker, symLinkName);
        res = true;
        goto fini;
    }
 fini:
    return res;
}

static void refactorySimplePackageRenaming(EditorMarkerList *occs, char *symname, char *symLinkName) {
    char rtpack[MAX_FILE_NAME_SIZE];
    char rtprefix[MAX_FILE_NAME_SIZE];
    char *ss;
    int snlen, slnlen;
    bool mvfile;
    EditorMarker      *pp;

    // get original and new directory, but how?
    snlen = strlen(symname);
    slnlen = strlen(symLinkName);
    strcpy(rtprefix, refactoringOptions.renameTo);
    ss = lastOccurenceInString(rtprefix, '.');
    if (ss == NULL) {
        strcpy(rtpack, rtprefix);
        rtprefix[0]  = 0;
    } else {
        strcpy(rtpack, ss+1);
        *(ss+1)=0;
    }
    for (EditorMarkerList *ll=occs; ll!=NULL; ll=ll->next) {
        pp = refactoryCrNewMarkerForExpressionBegin(ll->marker, GET_STATIC_PREFIX_START);
        if (pp!=NULL) {
            refactoryRemoveNonCommentCode(pp, ll->marker->offset - pp->offset);
            // make attention here, so that markers still points
            // to the package name, the best would be to replace
            // package name per single names, ...
            refactoryCheckedReplaceString(pp, snlen, symname, rtpack);
            refactoryReplaceString(pp, 0, rtprefix);
        }
        editorFreeMarker(pp);
    }
    for (EditorMarkerList *ll=occs; ll!=NULL; ll=ll->next) {
        if (ll->next == NULL || ll->next->marker->buffer!=ll->marker->buffer) {
            // O.K. verify whether I should move the file
            MapOnPaths(javaSourcePaths, {
                    mvfile = refactoryRenamePackageFileMove(currentPath, ll, symLinkName,
                                                            slnlen);
                    if (mvfile) goto moved;
                });
        moved:;
        }
    }
}

static void refactorySimpleRenaming(EditorMarkerList *occs, EditorMarker *point,
                                    char *symname,char *symLinkName, int symtype) {
    char                nfile[MAX_FILE_NAME_SIZE];
    char                *ss;

    if (symtype == TypeStruct && LANGUAGE(LANG_JAVA)
        && refactoringOptions.theRefactoring != AVR_RENAME_CLASS) {
        errorMessage(ERR_INTERNAL, "Use Rename Class to rename classes");
    }
    if (symtype == TypePackage && LANGUAGE(LANG_JAVA)
        && refactoringOptions.theRefactoring != AVR_RENAME_PACKAGE) {
        errorMessage(ERR_INTERNAL, "Use Rename Package to rename packages");
    }

    if (refactoringOptions.theRefactoring == AVR_RENAME_PACKAGE) {
        refactorySimplePackageRenaming(occs, symname, symLinkName);
    } else {
        for (EditorMarkerList *ll=occs; ll!=NULL; ll=ll->next) {
            refactoryRenameTo(ll->marker, symname, refactoringOptions.renameTo);
        }
        ppcGotoMarker(point);
        if (refactoringOptions.theRefactoring == AVR_RENAME_CLASS) {
            if (strcmp(simpleFileNameWithoutSuffix_st(point->buffer->name), symname)==0) {
                // O.K. file name equals to class name, rename file
                strcpy(nfile, point->buffer->name);
                ss = lastOccurenceOfSlashOrBackslash(nfile);
                if (ss==NULL) ss=nfile;
                else ss++;
                sprintf(ss, "%s.java", refactoringOptions.renameTo);
                assert(strlen(nfile) < MAX_FILE_NAME_SIZE-1);
                if (strcmp(nfile, point->buffer->name)!=0) {
                    // O.K. I should move file
                    refactoryCheckedRenameBuffer(point->buffer, nfile, &editorUndo);
                }
            }
        }
    }
}

static EditorMarkerList *refactoryGetReferences(EditorBuffer *buf, EditorMarker *point,
                                                char *resolveMessage, int messageType
) {
    EditorMarkerList *occs;
    refactoryPushReferences(buf, point, "-olcxrename", resolveMessage, messageType);
    assert(sessionData.browserStack.top && sessionData.browserStack.top->hkSelectedSym);
    occs = editorReferencesToMarkers(sessionData.browserStack.top->references, filter0, NULL);
    return occs;
}


static EditorMarkerList *refactoryPushGetAndPreCheckReferences(EditorBuffer *buf, EditorMarker *point, char *nameOnPoint,
                                                               char *resolveMessage, int messageType
) {
    EditorMarkerList *occs;
    occs = refactoryGetReferences(buf, point, resolveMessage, messageType);
    refactoryPreCheckThatSymbolRefsCorresponds(nameOnPoint, occs);
    return occs;
}


static EditorMarker *refactoryFindModifierAndCrMarker(EditorMarker *point, char *modifier, int limitIndex) {
    int             i, mlen, blen, mini;
    char            *text;
    EditorMarker  *mm, *mb, *me;

    text = point->buffer->allocation.text;
    blen = point->buffer->allocation.bufferSize;
    mlen = strlen(modifier);
    refactoryMakeSyntaxPassOnSource(point);
    if (s_spp[limitIndex].file == noFileIndex) {
        warningMessage(ERR_INTERNAL, "cant get field declaration");
        mini = point->offset;
        while (mini>0 && text[mini]!='\n') mini --;
        i = point->offset;
    } else {
        mb = editorCreateNewMarkerForPosition(&s_spp[limitIndex]);
        // TODO, this limitIndex+2 should be done more semantically
        me = editorCreateNewMarkerForPosition(&s_spp[limitIndex+2]);
        mini = mb->offset;
        i = me->offset;
        editorFreeMarker(mb);
        editorFreeMarker(me);
    }
    while (i>=mini) {
        if (strncmp(text+i, modifier, mlen)==0
            && (i==0 || isspace(text[i-1]))
            && (i+mlen==blen || isspace(text[i+mlen]))) {
            mm = editorCreateNewMarker(point->buffer, i);
            return mm;
        }
        i--;
    }
    return NULL;
}

static void refactoryRemoveModifier(EditorMarker *point, int limitIndex, char *modifier) {
    int i, j, mlen;
    char *text;
    EditorMarker  *mm;

    mlen = strlen(modifier);
    text = point->buffer->allocation.text;
    mm = refactoryFindModifierAndCrMarker(point, modifier, limitIndex);
    if (mm!=NULL) {
        i = mm->offset;
        for (j=i+mlen; isspace(text[j]); j++) ;
        refactoryReplaceString(mm, j-i, "");
    }
    editorFreeMarker(mm);
}

static void refactoryAddModifier(EditorMarker *point, int limit, char *modifier) {
    char            modifSpace[TMP_STRING_SIZE];
    EditorMarker  *mm;
    refactoryMakeSyntaxPassOnSource(point);
    if (s_spp[limit].file == noFileIndex) {
        errorMessage(ERR_INTERNAL, "cant find beginning of field declaration");
    }
    mm = editorCreateNewMarkerForPosition(&s_spp[limit]);
    sprintf(modifSpace, "%s ", modifier);
    refactoryReplaceString(mm, 0, modifSpace);
    editorFreeMarker(mm);
}

static void refactoryChangeAccessModifier(
                                          EditorMarker *point, int limitIndex, char *modifier
                                          ) {
    EditorMarker *mm;
    mm = refactoryFindModifierAndCrMarker(point, modifier, limitIndex);
    if (mm == NULL) {
        refactoryRemoveModifier(point, limitIndex, "private");
        refactoryRemoveModifier(point, limitIndex, "protected");
        refactoryRemoveModifier(point, limitIndex, "public");
        if (*modifier) refactoryAddModifier(point, limitIndex, modifier);
    } else {
        editorFreeMarker(mm);
    }
}

static void refactoryRestrictAccessibility(EditorMarker *point, int limitIndex, int minAccess) {
    int accessIndex, access;

    minAccess &= ACCESS_PPP_MODIFER_MASK;
    for (accessIndex=0; accessIndex<MAX_REQUIRED_ACCESS; accessIndex++) {
        if (javaRequiredAccessibilityTable[accessIndex] == minAccess) break;
    }

    // must update, because usualy they are out of date here
    refactoryUpdateReferences(refactoringOptions.project);

    refactoryPushReferences(point->buffer, point, "-olcxrename", NULL, 0);
    assert(sessionData.browserStack.top && sessionData.browserStack.top->menuSym);

    for (Reference *rr=sessionData.browserStack.top->references; rr!=NULL; rr=rr->next) {
        if (! IS_DEFINITION_OR_DECL_USAGE(rr->usage.kind)) {
            if (rr->usage.requiredAccess < accessIndex) {
                accessIndex = rr->usage.requiredAccess;
            }
        }
    }

    olcxPopOnly();

    access = javaRequiredAccessibilityTable[accessIndex];

    if (access == AccessPublic) refactoryChangeAccessModifier(point, limitIndex, "public");
    else if (access == AccessProtected) refactoryChangeAccessModifier(point, limitIndex, "protected");
    else if (access == AccessDefault) refactoryChangeAccessModifier(point, limitIndex, "");
    else if (access == AccessPrivate) refactoryChangeAccessModifier(point, limitIndex, "private");
    else errorMessage(ERR_INTERNAL, "No access modifier could be computed");
}


static void multipleReferencesInSamePlaceMessage(Reference *r) {
    char tmpBuff[TMP_BUFF_SIZE];
    ppcGotoPosition(&r->position);
    sprintf(tmpBuff, "The reference at this place refers to multiple symbols. The refactoring will probably damage your program. Do you really want to continue?");
    formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
    ppcGenRecord(PPC_ASK_CONFIRMATION, tmpBuff);
}

static void refactoryCheckForMultipleReferencesInSamePlace(OlcxReferences *rstack, SymbolsMenu *ccms) {
    ReferencesItem *p, *sss;
    SymbolsMenu *cms;
    bool pushed;

    p = &ccms->s;
    assert(rstack && rstack->menuSym);
    sss = &rstack->menuSym->s;
    pushed = itIsSymbolToPushOlReferences(p, rstack, &cms, DEFAULT_VALUE);
    // TODO, this can be simplified, as ccms == cms.
    log_trace(":checking %s to %s (%d)", p->name, sss->name, pushed);
    if (!pushed && olcxIsSameCxSymbol(p, sss)) {
        log_trace("checking %s references", p->name);
        for (Reference *r=p->references; r!=NULL; r=r->next) {
            if (refOccursInRefs(r, rstack->references)) {
                multipleReferencesInSamePlaceMessage(r);
            }
        }
    }
}

static void refactoryMultipleOccurencesSafetyCheck(void) {
    OlcxReferences    *rstack;

    rstack = sessionData.browserStack.top;
    olProcessSelectedReferences(rstack, refactoryCheckForMultipleReferencesInSamePlace);
}

// -------------------------------------------- Rename

static void refactoryRename(EditorBuffer *buf, EditorMarker *point) {
    char nameOnPoint[TMP_STRING_SIZE];
    char *symLinkName, *message;
    Type symtype;
    EditorMarkerList *occs;
    EditorUndo *undoStartPoint, *redoTrack;
    SymbolsMenu *csym;

    if (refactoringOptions.renameTo == NULL) {
        errorMessage(ERR_ST, "this refactoring requires -renameto=<new name> option");
    }

    refactoryUpdateReferences(refactoringOptions.project);

    if (LANGUAGE(LANG_JAVA)) {
        message = STANDARD_SELECT_SYMBOLS_MESSAGE;
    } else {
        message = STANDARD_C_SELECT_SYMBOLS_MESSAGE;
    }
    // rename
    strcpy(nameOnPoint, refactoryGetIdentifierOnMarker_st(point));
    assert(strlen(nameOnPoint) < TMP_STRING_SIZE-1);
    occs = refactoryPushGetAndPreCheckReferences(buf, point, nameOnPoint, message,PPCV_BROWSER_TYPE_INFO);
    csym =  sessionData.browserStack.top->hkSelectedSym;
    symtype = csym->s.type;
    symLinkName = csym->s.name;
    undoStartPoint = editorUndo;
    if (!LANGUAGE(LANG_JAVA)) {
        refactoryMultipleOccurencesSafetyCheck();
    }
    refactorySimpleRenaming(occs, point, nameOnPoint, symLinkName, symtype);
    //&editorDumpBuffers();
    redoTrack = NULL;
    if (!refactoryMakeSafetyCheckAndUndo(point, &occs, undoStartPoint, &redoTrack)) {
        refactoryAskForReallyContinueConfirmation();
    }

    editorApplyUndos(redoTrack, NULL, NULL, GEN_FULL_OUTPUT);

    // finish where you have started
    ppcGotoMarker(point);

    editorFreeMarkersAndMarkerList(occs);  // O(n^2)!

    if (refactoringOptions.theRefactoring==AVR_RENAME_PACKAGE) {
        ppcGenRecord(PPC_INFORMATION, "\nDone.\nDo not forget to remove .class files of former package");
    } else if (refactoringOptions.theRefactoring==AVR_RENAME_CLASS
               && strcmp(simpleFileNameWithoutSuffix_st(point->buffer->name), refactoringOptions.renameTo)==0) {
        ppcGenRecord(PPC_INFORMATION, "\nDone.\nDo not forget to remove .class file of former class");
    }
}

static void clearParamPositions(void) {
    s_paramPosition = noPosition;
    s_paramBeginPosition = noPosition;
    s_paramEndPosition = noPosition;
}

static int refactoryGetParamNamePosition(EditorMarker *pos, char *fname, int argn) {
    char            pushOpt[TMP_STRING_SIZE];
    char            *actName;
    int             res;
    actName = refactoryGetIdentifierOnMarker_st(pos);
    clearParamPositions();
    assert(strcmp(actName, fname)==0);
    sprintf(pushOpt, "-olcxgotoparname%d", argn);
    refactoryEditServerParseBuffer(refactoringOptions.project, pos->buffer, pos, NULL, pushOpt, NULL);
    olcxPopOnly();
    if (s_paramPosition.file != noFileIndex) {
        res = RETURN_OK;
    } else {
        res = RETURN_ERROR;
    }
    return res;
}

static int refactoryGetParamPosition(EditorMarker *pos, char *fname, int argn) {
    char            pushOpt[TMP_STRING_SIZE];
    char            *actName;
    int             res;
    char tmpBuff[TMP_BUFF_SIZE];

    actName = refactoryGetIdentifierOnMarker_st(pos);
    if (! (strcmp(actName, fname)==0
           || strcmp(actName,"this")==0
           || strcmp(actName,"super")==0)) {
        ppcGotoMarker(pos);
        sprintf(tmpBuff, "This reference is not pointing to the function/method name. Maybe a composed symbol. Sorry, do not know how to handle this case.");
        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
        errorMessage(ERR_ST, tmpBuff);
    }

    clearParamPositions();
    sprintf(pushOpt, "-olcxgetparamcoord%d", argn);
    refactoryEditServerParseBuffer(refactoringOptions.project, pos->buffer, pos, NULL, pushOpt, NULL);
    olcxPopOnly();

    res = RETURN_OK;
    if (s_paramBeginPosition.file == noFileIndex
        || s_paramEndPosition.file == noFileIndex
        || s_paramBeginPosition.file == -1
        || s_paramEndPosition.file == -1
        ) {
        ppcGotoMarker(pos);
        errorMessage(ERR_INTERNAL, "Can't get end of parameter");
        res = RETURN_ERROR;
    }
    // check some logical preconditions,
    if (s_paramBeginPosition.file != s_paramEndPosition.file
        || s_paramBeginPosition.line > s_paramEndPosition.line
        || (s_paramBeginPosition.line == s_paramEndPosition.line
            && s_paramBeginPosition.col > s_paramEndPosition.col)) {
        ppcGotoMarker(pos);
        sprintf(tmpBuff, "Something goes wrong at this occurence, can't get reasonable parameter limites");
        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
        errorMessage(ERR_ST, tmpBuff);
        res = RETURN_ERROR;
    }
    if (! LANGUAGE(LANG_JAVA)) {
        // check preconditions to avoid cases like
        // #define PAR1 toto,
        // ... lot of text
        // #define PAR2 tutu,
        //    function(PAR1 PAR2 0);
        // Hmmm, but how to do it? TODO!!!!
    }
    return res;
}

// !!!!!!!!! pos and endm can be the same marker !!!!!!
static int refactoryAddStringAsParameter(EditorMarker *pos, EditorMarker *endm,
                                         char *fname, int argn, char *param) {
    char *text;
    char par[REFACTORING_TMP_STRING_SIZE];
    char *sep1, *sep2;
    int rr,insertionOffset;
    EditorMarker *mm;

    insertionOffset = -1;
    rr = refactoryGetParamPosition(pos, fname, argn);
    if (rr != RETURN_OK) {
        errorMessage(ERR_INTERNAL, "Problem while adding parameter");
        return insertionOffset;
    }
    text = pos->buffer->allocation.text;

    if (endm==NULL) {
        mm = editorCreateNewMarkerForPosition(&s_paramBeginPosition);
    } else {
        mm = endm;
        assert(mm->buffer->fileIndex == s_paramBeginPosition.file);
        editorMoveMarkerToLineCol(mm, s_paramBeginPosition.line, s_paramBeginPosition.col);
    }

    sep1=""; sep2="";
    if (positionsAreEqual(s_paramBeginPosition, s_paramEndPosition)) {
        if (text[mm->offset] == '(') {
            // function with no parameter
            mm->offset ++;
            sep1=""; sep2="";
        } else if (text[mm->offset] == ')') {
            // beyond limite
            sep1=", "; sep2="";
        } else {
            char tmpBuff[TMP_BUFF_SIZE];
            ppcGotoMarker(pos);
            sprintf(tmpBuff, "Something goes wrong, probably different parameter coordinates at different cpp passes.");
            formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
            fatalError(ERR_INTERNAL, tmpBuff, XREF_EXIT_ERR);
            assert(0);
        }
    } else {
        if (text[mm->offset] == '(') {
            sep1 = "";
            sep2 = ", ";
        } else {
            sep1 = " ";
            sep2 = ",";
        }
        mm->offset ++;
    }

    sprintf(par, "%s%s%s", sep1, param, sep2);
    assert(strlen(param) < REFACTORING_TMP_STRING_SIZE-1);

    insertionOffset = mm->offset;
    refactoryReplaceString(mm, 0, par);
    if (endm == NULL) {
        editorFreeMarker(mm);
    }
    // O.K. I hope that mm == endp is moved to
    // end of inserted parameter
    return insertionOffset;
}

static int refactoryIsThisSymbolUsed(EditorMarker *pos) {
    int refn;
    refactoryPushReferences(pos->buffer, pos, "-olcxpushforlm",STANDARD_SELECT_SYMBOLS_MESSAGE,PPCV_BROWSER_TYPE_INFO);
    LIST_LEN(refn, Reference, sessionData.browserStack.top->references);
    olcxPopOnly();
    return refn > 1;
}

static int refactoryIsParameterUsedExceptRecursiveCalls(EditorMarker *ppos, EditorMarker *fpos) {
    // for the moment
    return refactoryIsThisSymbolUsed(ppos);
}

static void refactoryCheckThatParameterIsUnused(EditorMarker *pos, char *fname,
                                                int argn, int checkfor) {
    char pname[TMP_STRING_SIZE];
    int rr;
    EditorMarker *mm;

    rr = refactoryGetParamNamePosition(pos, fname, argn);
    if (rr != RETURN_OK) {
        ppcGenRecord(PPC_ASK_CONFIRMATION, "Can not parse parameter definition, continue anyway?");
        return;
    }

    mm = editorCreateNewMarkerForPosition(&s_paramPosition);
    strncpy(pname, refactoryGetIdentifierOnMarker_st(mm), TMP_STRING_SIZE);
    pname[TMP_STRING_SIZE-1] = 0;
    if (refactoryIsParameterUsedExceptRecursiveCalls(mm, pos)) {
        char tmpBuff[TMP_BUFF_SIZE];
        if (checkfor==CHECK_FOR_ADD_PARAM) {
            sprintf(tmpBuff, "parameter '%s' clashes with an existing symbol, continue anyway?", pname);
            ppcGenRecord(PPC_ASK_CONFIRMATION, tmpBuff);
        } else if (checkfor==CHECK_FOR_DEL_PARAM) {
            sprintf(tmpBuff, "parameter '%s' is used, delete it anyway?", pname);
            ppcGenRecord(PPC_ASK_CONFIRMATION, tmpBuff);
        } else {
            assert(0);
        }
    }
    editorFreeMarker(mm);
}

static void refactoryAddParameter(EditorMarker *pos, char *fname,
                                  int argn, int usage) {
    if (IS_DEFINITION_OR_DECL_USAGE(usage)) {
        refactoryAddStringAsParameter(pos,NULL, fname, argn, refactoringOptions.refpar1);
        // now check that there is no conflict
        if (IS_DEFINITION_USAGE(usage)) {
            refactoryCheckThatParameterIsUnused(pos, fname, argn, CHECK_FOR_ADD_PARAM);
        }
    } else {
        refactoryAddStringAsParameter(pos,NULL, fname, argn, refactoringOptions.refpar2);
    }
}

static void refactoryDeleteParameter(EditorMarker *pos, char *fname,
                                     int argn, int usage) {
    char *text;
    int res;
    EditorMarker *m1, *m2;

    res = refactoryGetParamPosition(pos, fname, argn);
    if (res != RETURN_OK) return;

    m1 = editorCreateNewMarkerForPosition(&s_paramBeginPosition);
    m2 = editorCreateNewMarkerForPosition(&s_paramEndPosition);

    text = pos->buffer->allocation.text;

    if (positionsAreEqual(s_paramBeginPosition, s_paramEndPosition)) {
        if (text[m1->offset] == '(') {
            // function with no parameter
        } else if (text[m1->offset] == ')') {
            // beyond limite
        } else {
            fatalError(ERR_INTERNAL,"Something goes wrong, probably different parameter coordinates at different cpp passes.", XREF_EXIT_ERR);
            assert(0);
        }
        errorMessage(ERR_ST, "Parameter out of limit");
    } else {
        if (text[m1->offset] == '(') {
            m1->offset ++;
            if (text[m2->offset] == ',') {
                m2->offset ++;
                // here pass also blank symbols
            }
            editorMoveMarkerToNonBlank(m2, 1);
        }
        if (IS_DEFINITION_USAGE(usage)) {
            // this must be at the end, because it discards values
            // of s_paramBeginPosition and s_paramEndPosition
            refactoryCheckThatParameterIsUnused(pos, fname, argn, CHECK_FOR_DEL_PARAM);
        }

        assert(m1->offset <= m2->offset);
        refactoryReplaceString(m1, m2->offset - m1->offset, "");
    }
    editorFreeMarker(m1);
    editorFreeMarker(m2);
}

static void refactoryMoveParameter(EditorMarker *pos, char *fname,
                                   int argFrom, int argTo) {
    char *text;
    char par[REFACTORING_TMP_STRING_SIZE];
    int res, plen;
    EditorMarker *m1, *m2;

    res = refactoryGetParamPosition(pos, fname, argFrom);
    if (res != RETURN_OK) return;

    m1 = editorCreateNewMarkerForPosition(&s_paramBeginPosition);
    m2 = editorCreateNewMarkerForPosition(&s_paramEndPosition);

    text = pos->buffer->allocation.text;
    plen = 0;
    par[plen]=0;

    if (positionsAreEqual(s_paramBeginPosition, s_paramEndPosition)) {
        if (text[m1->offset] == '(') {
            // function with no parameter
        } else if (text[m1->offset] == ')') {
            // beyond limite
        } else {
            fatalError(ERR_INTERNAL,"Something goes wrong, probably different parameter coordinates at different cpp passes.", XREF_EXIT_ERR);
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
        par[plen]=0;
        refactoryDeleteParameter(pos, fname, argFrom, UsageUsed);
        refactoryAddStringAsParameter(pos, NULL, fname, argTo, par);
    }
    editorFreeMarker(m1);
    editorFreeMarker(m2);
}

static void refactoryApplyParamManip(char *functionName, EditorMarkerList *occs,
                                     int manip, int argn1, int argn2
) {
    int progressi, progressn;

    LIST_LEN(progressn, EditorMarkerList, occs); progressi=0;
    for (EditorMarkerList *ll=occs; ll!=NULL; ll=ll->next) {
        if (ll->usage.kind != UsageUndefinedMacro) {
            if (manip==PPC_AVR_ADD_PARAMETER) {
                refactoryAddParameter(ll->marker, functionName, argn1,
                                      ll->usage.kind);
            } else if (manip==PPC_AVR_DEL_PARAMETER) {
                refactoryDeleteParameter(ll->marker, functionName, argn1,
                                         ll->usage.kind);
            } else if (manip==PPC_AVR_MOVE_PARAMETER) {
                refactoryMoveParameter(ll->marker, functionName,
                                       argn1, argn2
                                       );
            } else {
                errorMessage(ERR_INTERNAL, "unknown parameter manipulation");
            }
        }
        writeRelativeProgress((progressi++)*100/progressn);
    }
    writeRelativeProgress(100);
}


// -------------------------------------- ParameterManipulations

static void refactoryApplyParameterManipulation(EditorBuffer *buf, EditorMarker *point,
                                                int manip, int argn1, int argn2) {
    char nameOnPoint[TMP_STRING_SIZE];
    int check;
    EditorMarkerList *occs;
    EditorUndo *startPoint, *redoTrack;

    refactoryUpdateReferences(refactoringOptions.project);

    strcpy(nameOnPoint, refactoryGetIdentifierOnMarker_st(point));
    refactoryPushReferences(buf, point, "-olcxargmanip",STANDARD_SELECT_SYMBOLS_MESSAGE,PPCV_BROWSER_TYPE_INFO);
    occs = editorReferencesToMarkers(sessionData.browserStack.top->references, filter0, NULL);
    startPoint = editorUndo;
    // first just check that loaded files are up to date
    //& refactoryPreCheckThatSymbolRefsCorresponds(nameOnPoint, occs);

    //&editorDumpBuffer(occs->marker->buffer);
    //&editorDumpMarkerList(occs);
    // for some error mesages it is more natural that cursor does not move
    ppcGotoMarker(point);
    redoTrack = NULL;
    refactoryApplyParamManip(nameOnPoint, occs,
                             manip, argn1, argn2
                             );
    if (LANGUAGE(LANG_JAVA)) {
        check = refactoryMakeSafetyCheckAndUndo(point, &occs, startPoint, &redoTrack);
        if (! check) refactoryAskForReallyContinueConfirmation();
        editorApplyUndos(redoTrack, NULL, &editorUndo, GEN_NO_OUTPUT);
    }
    editorFreeMarkersAndMarkerList(occs);  // O(n^2)!
}

static void refactoryParameterManipulation(EditorBuffer *buf, EditorMarker *point,
                                           int manip, int argn1, int argn2) {
    refactoryApplyParameterManipulation(buf, point, manip, argn1, argn2);
    // and generate output
    refactoryApplyWholeRefactoringFromUndo();
    ppcGotoMarker(point);
}

static int createMarkersForAllReferencesInRegions(SymbolsMenu *menu, EditorRegionList **regions) {
    int res, n;

    res = 0;
    for (SymbolsMenu *mm=menu; mm!=NULL; mm=mm->next) {
        assert(mm->markers==NULL);
        if (mm->selected && mm->visible) {
            mm->markers = editorReferencesToMarkers(mm->s.references, filter0, NULL);
            if (regions != NULL) {
                editorRestrictMarkersToRegions(&mm->markers, regions);
            }
            LIST_MERGE_SORT(EditorMarkerList, mm->markers, editorMarkerListLess);
            LIST_LEN(n, EditorMarkerList, mm->markers);
            res += n;
        }
    }
    return res;
}

// --------------------------------------- ExpandShortNames

static void refactoryApplyExpandShortNames(EditorBuffer *buf, EditorMarker *point) {
    char fqtName[MAX_FILE_NAME_SIZE];
    char fqtNameDot[2*MAX_FILE_NAME_SIZE];
    char *shortName;
    int shortNameLen;

    refactoryEditServerParseBuffer(refactoringOptions.project, buf, point, NULL, "-olcxnotfqt", NULL);
    olcxPushSpecial(LINK_NAME_NOT_FQT_ITEM, OLO_NOT_FQT_REFS);

    // Do it in two steps because when changing file the references
    // are not updated while markers are, so first I need to change
    // everything to markers
    createMarkersForAllReferencesInRegions(sessionData.browserStack.top->menuSym, NULL);
    // Hmm. what if one reference will be twice ? Is it possible?
    for (SymbolsMenu *mm = sessionData.browserStack.top->menuSym; mm!=NULL; mm=mm->next) {
        if (mm->selected && mm->visible) {
            javaGetClassNameFromFileIndex(mm->s.vApplClass, fqtName, DOTIFY_NAME);
            javaDotifyClassName(fqtName);
            sprintf(fqtNameDot, "%s.", fqtName);
            shortName = javaGetShortClassName(fqtName);
            shortNameLen = strlen(shortName);
            if (isdigit(shortName[0])) {
                goto cont;  // anonymous nested class, no expansion
            }
            for (EditorMarkerList *ppp=mm->markers; ppp!=NULL; ppp=ppp->next) {
                char tmpBuff[TMP_BUFF_SIZE];
                log_trace("expanding at %s:%d", ppp->marker->buffer->name, ppp->marker->offset);
                if (ppp->usage.kind == UsageNonExpandableNotFQTNameInClassOrMethod) {
                    ppcGotoMarker(ppp->marker);
                    sprintf(tmpBuff, "This occurence of %s would be misinterpreted after expansion to %s.\nNo action made at this place.", shortName, fqtName);
                    warningMessage(ERR_ST, tmpBuff);
                } else if (ppp->usage.kind == UsageNotFQFieldInClassOrMethod) {
                    refactoryReplaceString(ppp->marker, 0, fqtNameDot);
                } else if (ppp->usage.kind == UsageNotFQTypeInClassOrMethod) {
                    refactoryCheckedReplaceString(ppp->marker, shortNameLen, shortName, fqtName);
                }
            }
        }
    cont:;
        editorFreeMarkerListNotMarkers(mm->markers);
        mm->markers = NULL;
    }
    //&editorDumpBuffer(buf);
}

static void refactoryExpandShortNames(EditorBuffer *buf, EditorMarker *point) {
    refactoryApplyExpandShortNames(buf, point);
    refactoryApplyWholeRefactoringFromUndo();
    ppcGotoMarker(point);
}

static EditorMarker *refactoryReplaceStaticPrefix(EditorMarker *d, char *npref) {
    int                 ppoffset, npreflen;
    EditorMarker      *pp;
    char                pdot[MAX_FILE_NAME_SIZE];

    pp = refactoryCrNewMarkerForExpressionBegin(d, GET_STATIC_PREFIX_START);
    if (pp==NULL) {
        // this is an error, this is just to avoid possible core dump in the future
        pp = editorCreateNewMarker(d->buffer, d->offset);
    } else {
        ppoffset = pp->offset;
        refactoryRemoveNonCommentCode(pp, d->offset-pp->offset);
        if (*npref!=0) {
            npreflen = strlen(npref);
            strcpy(pdot, npref);
            pdot[npreflen] = '.'; pdot[npreflen+1]=0;
            refactoryReplaceString(pp, 0, pdot);
        }
        // return it back to beginning of fqt
        pp->offset = ppoffset;
    }
    return pp;
}

// -------------------------------------- ReduceLongNames

static void refactoryReduceLongReferencesInRegions(EditorMarker      *point,
                                                   EditorRegionList  **regions
) {
    EditorMarkerList *rli, *ri, *ro;
    int currentProgress, totalProgress;

    refactoryEditServerParseBuffer(refactoringOptions.project, point->buffer, point,NULL,
                                   "-olcxuselesslongnames", "-olallchecks");
    olcxPushSpecial(LINK_NAME_IMPORTED_QUALIFIED_ITEM, OLO_USELESS_LONG_NAME);
    rli = editorReferencesToMarkers(sessionData.browserStack.top->references, filter0, NULL);
    editorSplitMarkersWithRespectToRegions(&rli, regions, &ri, &ro);
    editorFreeMarkersAndMarkerList(ro);

    // a hack, as we cannot predict how many times this will be
    // invoked, adjust progress bar counter ratio

    progressFactor += 1;
    LIST_LEN(totalProgress, EditorMarkerList, ri);
    currentProgress=0;
    for (EditorMarkerList *rr=ri; rr!=NULL; rr=rr->next) {
        EditorMarker *m = refactoryReplaceStaticPrefix(rr->marker, "");
        editorFreeMarker(m);
        writeRelativeProgress((((currentProgress++)*100/totalProgress)/10)*10);
    }
    writeRelativeProgress(100);
}

// ------------------------------------------------------ Reduce Long Names In The File

static bool refactoryIsTheImportUsed(EditorMarker *point, int line, int col) {
    char importSymbolName[TMP_STRING_SIZE];
    bool used;

    strcpy(importSymbolName, javaImportSymbolName_st(point->buffer->fileIndex, line, col));
    refactoryEditServerParseBuffer(refactoringOptions.project, point->buffer, point, NULL,
                                    "-olcxpushfileunused", "-olallchecks");
    pushLocalUnusedSymbolsAction();
    used = true;
    for (SymbolsMenu *m=sessionData.browserStack.top->menuSym; m!=NULL; m=m->next) {
        if (m->visible && strcmp(m->s.name, importSymbolName)==0) {
            used = false;
            goto finish;
        }
    }
 finish:
    olcxPopOnly();
    return used;
}

static int refactoryPushFileUnimportedFqts(EditorMarker *point, EditorRegionList **regions) {
    char                pushOpt[TMP_STRING_SIZE];
    int                 lastImportLine;

    sprintf(pushOpt, "-olcxpushspecialname=%s", LINK_NAME_UNIMPORTED_QUALIFIED_ITEM);
    refactoryEditServerParseBuffer(refactoringOptions.project, point->buffer, point,NULL,pushOpt, "-olallchecks");
    lastImportLine = s_cps.lastImportLine;
    olcxPushSpecial(LINK_NAME_UNIMPORTED_QUALIFIED_ITEM, OLO_PUSH_SPECIAL_NAME);
    createMarkersForAllReferencesInRegions(sessionData.browserStack.top->menuSym, regions);
    return lastImportLine;
}

static int refactoryImportNeeded(EditorMarker *point, EditorRegionList **regions, int vApplCl) {
    int res;

    // check whether the symbol is reduced
    refactoryPushFileUnimportedFqts(point, regions);
    for (SymbolsMenu *mm=sessionData.browserStack.top->menuSym; mm!=NULL; mm=mm->next) {
        if (mm->s.vApplClass == vApplCl) {
            res = 1;
            goto fini;
        }
    }
    res = 0;
 fini:
    olcxPopOnly();
    return res;
}

static bool refactoryAddImport(EditorMarker *point, EditorRegionList **regions,
                              char *iname, int line, int vApplCl, int interactive) {
    char                istat[MAX_CX_SYMBOL_SIZE];
    char                icoll;
    char                *ld1, *ld2;
    EditorMarker      *mm;
    bool res;
    EditorUndo        *undoBase;
    EditorRegionList  *wholeBuffer;

    undoBase = editorUndo;
    sprintf(istat, "import %s;\n", iname);
    mm = editorCreateNewMarker(point->buffer, 0);
    // a little hack, make one free line after 'package'
    if (line > 1) {
        editorMoveMarkerToLineCol(mm, line-1, 0);
        if (strncmp(MARKER_TO_POINTER(mm), "package", 7) == 0) {
            // insert newline
            editorMoveMarkerToLineCol(mm, line, 0);
            refactoryReplaceString(mm, 0, "\n");
            line ++;
        }
    }
    // add the import
    editorMoveMarkerToLineCol(mm, line, 0);
    refactoryReplaceString(mm, 0, istat);

    res = true;

    // TODO verify that import is unused
    ld1 = NULL;
    ld2 = istat + strlen("import");
    for (char *ss=ld2; *ss; ss++) {
        if (*ss == '.') {
            ld1 = ld2;
            ld2 = ss;
        }
    }
    if (ld1 == NULL) {
        errorMessage(ERR_INTERNAL, "can't find imported package");
    } else {
        icoll = ld1 - istat + 1;
        if (refactoryIsTheImportUsed(mm, line, icoll)) {
            if (interactive == INTERACTIVE_YES) {
                ppcGenRecord(PPC_WARNING,
                             "Sorry, adding this import would cause misinterpretation of\nsome of classes used elsewhere it the file.");
            }
            res = false;
        } else {
            wholeBuffer = editorWholeBufferRegion(point->buffer);
            refactoryReduceLongReferencesInRegions(point, &wholeBuffer);
            editorFreeMarkersAndRegionList(wholeBuffer); wholeBuffer=NULL;
            if (refactoryImportNeeded(point, regions, vApplCl)) {
                if (interactive == INTERACTIVE_YES) {
                    ppcGenRecord(PPC_WARNING,
                                 "Sorry, this import will not help to reduce class references.");
                }
                res = false;
            }
        }
    }
    if (!res)
        editorUndoUntil(undoBase, NULL);

    editorFreeMarker(mm);
    return res;
}

static bool isInDisabledList(S_disabledList *list, int file, int vApplCl) {
    for (S_disabledList *ll=list; ll!=NULL; ll=ll->next) {
        if (ll->file == file && ll->clas == vApplCl)
            return true;
    }
    return false;
}

static int translatePassToAddImportAction(int pass) {
    switch (pass) {
    case 0: return RC_IMPORT_ON_DEMAND;
    case 1: return RC_IMPORT_SINGLE_TYPE;
    case 2: return RC_CONTINUE;
    default: errorMessage(ERR_INTERNAL, "wrong code for noninteractive add import");
    }
    return 0;
}

static int refactoryInteractiveAskForAddImportAction(EditorMarkerList *ppp, int defaultAction,
                                                     char *fqtName
                                                     ) {
    int action;
    refactoryApplyWholeRefactoringFromUndo();  // make current state visible
    ppcGotoMarker(ppp->marker);
    ppcValueRecord(PPC_ADD_TO_IMPORTS_DIALOG,defaultAction,fqtName);
    refactoryBeInteractive();
    action = options.continueRefactoring;
    return action;
}

static S_disabledList *newDisabledList(SymbolsMenu *menu, int cfile, S_disabledList *disabled) {
    S_disabledList *dl;

    ED_ALLOC(dl, S_disabledList);
    *dl = (S_disabledList){.file = cfile, .clas = menu->s.vApplClass, .next = disabled};

    return dl;
}


static void refactoryPerformReduceNamesAndAddImportsInSingleFile(EditorMarker *point, EditorRegionList **regions,
                                                                 int interactive) {
    EditorBuffer *b;
    int action, lastImportLine;
    bool keepAdding;
    int fileIndex;
    char fqtName[MAX_FILE_NAME_SIZE];
    char starName[MAX_FILE_NAME_SIZE];
    char *dd;
    S_disabledList *disabled, *dl;

    // just verify that all references are from single file
    if (regions!=NULL && *regions!=NULL) {
        b = (*regions)->region.begin->buffer;
        for (EditorRegionList *rl= *regions; rl!=NULL; rl=rl->next) {
            if (rl->region.begin->buffer != b || rl->region.end->buffer != b) {
                assert(0 && "[refactoryPerformAddImportsInSingleFile] region list contains multiple buffers!");
                break;
            }
        }
    }

    refactoryReduceLongReferencesInRegions(point, regions);

    disabled = NULL;
    keepAdding = true;
    while (keepAdding) {
        keepAdding = false;
        lastImportLine = refactoryPushFileUnimportedFqts(point, regions);
        for (SymbolsMenu *menu=sessionData.browserStack.top->menuSym; menu!=NULL; menu=menu->next) {
            EditorMarkerList *markers;
            markers=menu->markers;
            int defaultImportAction = refactoringOptions.defaultAddImportStrategy;
            while (markers!=NULL && !keepAdding
                   && !isInDisabledList(disabled, markers->marker->buffer->fileIndex, menu->s.vApplClass)) {
                fileIndex = markers->marker->buffer->fileIndex;
                javaGetClassNameFromFileIndex(menu->s.vApplClass, fqtName, DOTIFY_NAME);
                javaDotifyClassName(fqtName);
                if (interactive == INTERACTIVE_YES) {
                    action = refactoryInteractiveAskForAddImportAction(markers, defaultImportAction, fqtName);
                } else {
                    action = translatePassToAddImportAction(defaultImportAction);
                }
                //&sprintf(tmpBuff,"%s, %s, %d", simpleFileNameFromFileNum(markers->marker->buffer->fileIndex), fqtName, action); ppcBottomInformation(tmpBuff);
                switch (action) {
                case RC_IMPORT_ON_DEMAND:
                    strcpy(starName, fqtName);
                    dd = lastOccurenceInString(starName, '.');
                    if (dd!=NULL) {
                        sprintf(dd, ".*");
                        keepAdding = refactoryAddImport(point, regions, starName,
                                                        lastImportLine+1, menu->s.vApplClass,
                                                        interactive);
                    }
                    defaultImportAction = NID_IMPORT_ON_DEMAND;
                    break;
                case RC_IMPORT_SINGLE_TYPE:
                    keepAdding = refactoryAddImport(point, regions, fqtName,
                                                    lastImportLine+1, menu->s.vApplClass,
                                                    interactive);
                    defaultImportAction = NID_SINGLE_TYPE_IMPORT;
                    break;
                case RC_CONTINUE:
                    dl = newDisabledList(menu, fileIndex, disabled);
                    disabled = dl;
                    defaultImportAction = NID_KEPP_FQT_NAME;
                    break;
                default:
                    fatalError(ERR_INTERNAL, "wrong continuation code", XREF_EXIT_ERR);
                }
                if (defaultImportAction <= 1)  defaultImportAction ++;
            }
            editorFreeMarkersAndMarkerList(menu->markers);
            menu->markers = NULL;
        }
        olcxPopOnly();
    }
}

static void refactoryPerformReduceNamesAndAddImports(EditorRegionList **regions, int interactive) {
    EditorBuffer *cb;
    EditorRegionList **cr, **cl, *ncr;

    LIST_MERGE_SORT(EditorRegionList, *regions, editorRegionListLess);
    cr = regions;
    // split regions per files and add imports
    while (*cr!=NULL) {
        cl = cr;
        cb = (*cr)->region.begin->buffer;
        while (*cr!=NULL && (*cr)->region.begin->buffer == cb) cr = &(*cr)->next;
        ncr = *cr;
        *cr = NULL;
        refactoryPerformReduceNamesAndAddImportsInSingleFile((*cl)->region.begin, cl, interactive);
        // following line this was big bug, regions may be sortes, some may even be
        // even removed due to overlaps
        //& *cr = ncr;
        cr = cl;
        while (*cr!=NULL) cr = &(*cr)->next;
        *cr = ncr;
    }
}

// ------------------------------------------- ReduceLongReferencesAddImports

// this is reduction of all names within file
static void refactoryReduceLongNamesInTheFile(EditorBuffer *buf, EditorMarker *point) {
    EditorRegionList *wholeBuffer;
    wholeBuffer = editorWholeBufferRegion(buf);
    // don't be interactive, I am too lazy to write jEdit interface
    // for <add-import-dialog>
    refactoryPerformReduceNamesAndAddImportsInSingleFile(point, &wholeBuffer, INTERACTIVE_NO);
    editorFreeMarkersAndRegionList(wholeBuffer);wholeBuffer=NULL;
    refactoryApplyWholeRefactoringFromUndo();
    ppcGotoMarker(point);
}

// this is reduction of a single fqt, problem is with detection of applicable context
static void refactoryAddToImports(EditorMarker *point) {
    EditorMarker          *begin, *end;
    EditorRegionList      *regionList;

    begin = editorDuplicateMarker(point);
    end = editorDuplicateMarker(point);
    editorMoveMarkerBeyondIdentifier(begin, -1);
    editorMoveMarkerBeyondIdentifier(end, 1);

    regionList = newEditorRegionList(begin, end, NULL);
    refactoryPerformReduceNamesAndAddImportsInSingleFile(point, &regionList, INTERACTIVE_YES);

    editorFreeMarkersAndRegionList(regionList);
    regionList=NULL;

    refactoryApplyWholeRefactoringFromUndo();
    ppcGotoMarker(point);
}


static void refactoryPushAllReferencesOfMethod(EditorMarker *m1, char *specialOption) {
    refactoryEditServerParseBuffer(refactoringOptions.project, m1->buffer, m1,NULL, "-olcxpushallinmethod", specialOption);
    olPushAllReferencesInBetween(s_cps.cxMemoryIndexAtMethodBegin, s_cps.cxMemoryIndexAtMethodEnd);
}


static void moveFirstElementOfMarkerList(EditorMarkerList **l1, EditorMarkerList **l2) {
    EditorMarkerList  *mm;
    if (*l1!=NULL) {
        mm = *l1;
        *l1 = (*l1)->next;
        mm->next = NULL;
        LIST_APPEND(EditorMarkerList, *l2, mm);
    }
}

static void refactoryShowSafetyCheckFailingDialog(EditorMarkerList **totalDiff, char *message) {
    EditorUndo *redo;
    redo = NULL;
    editorUndoUntil(refactoringStartingPoint, &redo);
    olcxPushSpecialCheckMenuSym(LINK_NAME_SAFETY_CHECK_MISSED);
    refactoryPushMarkersAsReferences(totalDiff, sessionData.browserStack.top,
                                     LINK_NAME_SAFETY_CHECK_MISSED);
    refactoryDisplayResolutionDialog(message, PPCV_BROWSER_TYPE_WARNING, CONTINUATION_DISABLED);
    editorApplyUndos(redo, NULL, &editorUndo, GEN_NO_OUTPUT);
}


#define EACH_SYMBOL_ONCE 1

static void refactoryStaticMoveCheckCorrespondance(
                                                   SymbolsMenu *menu1,
                                                   SymbolsMenu *menu2,
                                                   ReferencesItem *theMethod
) {
    SymbolsMenu *mm1, *mm2;
    EditorMarkerList *diff1, *diff2, *totalDiff;

    totalDiff = NULL;
    mm1=menu1;
    while (mm1!=NULL) {
        // do not check recursive calls
        if (isSameCxSymbolIncludingFunctionClass(&mm1->s, theMethod)) goto cont;
        // nor local variables
        if (mm1->s.storage == StorageAuto) goto cont;
        // nor labels
        if (mm1->s.type == TypeLabel) goto cont;
        // do not check also any symbols from classes defined in inner scope
        if (isStrictlyEnclosingClass(mm1->s.vFunClass, theMethod->vFunClass)) goto cont;
        // (maybe I should not test any local symbols ???)
        // O.K. something to be checked, find correspondance in mm2
        //&fprintf(dumpOut, "Looking for correspondance to %s\n", mm1->s.name);
        for (mm2=menu2; mm2!=NULL; mm2=mm2->next) {
            log_trace("Checking '%s'", mm2->s.name);
            if (isSameCxSymbolIncludingApplicationClass(&mm1->s, &mm2->s)) break;
        }
        if (mm2==NULL) {
            // O(n^2) !!!
            //&fprintf(dumpOut, "Did not find correspondance to %s\n", mm1->s.name);
#           ifdef EACH_SYMBOL_ONCE
            // if each symbol reported only once
            moveFirstElementOfMarkerList(&mm1->markers, &totalDiff);
#           else
            LIST_APPEND(S_editorMarkerList, totalDiff, mm1->markers);
            // hack!
            mm1->markers = NULL;
#           endif
        } else {
            editorMarkersDifferences(&mm1->markers, &mm2->markers, &diff1, &diff2);
#           ifdef EACH_SYMBOL_ONCE
            // if each symbol reported only once
            if (diff1!=NULL) {
                moveFirstElementOfMarkerList(&diff1, &totalDiff);
                //&fprintf(dumpOut, "problem with symbol %s corr %s\n", mm1->s.name, mm2->s.name);
            } else {
                // no, do not put there new symbols, only lost are problems
                moveFirstElementOfMarkerList(&diff2, &totalDiff);
            }
            editorFreeMarkersAndMarkerList(diff1);
            editorFreeMarkersAndMarkerList(diff2);
#           else
            LIST_APPEND(S_editorMarkerList, totalDiff, diff1);
            LIST_APPEND(S_editorMarkerList, totalDiff, diff2);
#           endif
        }
        editorFreeMarkersAndMarkerList(mm1->markers);
        mm1->markers = NULL;
    cont:
        mm1=mm1->next;
    }
    for (mm2=menu2; mm2!=NULL; mm2=mm2->next) {
        editorFreeMarkersAndMarkerList(mm2->markers);
        mm2->markers = NULL;
    }
    if (totalDiff!=NULL) {
#       ifdef EACH_SYMBOL_ONCE
        refactoryShowSafetyCheckFailingDialog(&totalDiff,"These references will be  misinterpreted after refactoring. Fix them first. (each symbol is reported only once)");
#       else
        refactoryShowSafetyCheckFailingDialog(&totalDiff, "These references will be  misinterpreted after refactoring");
#       endif
        editorFreeMarkersAndMarkerList(totalDiff); totalDiff=NULL;
        refactoryAskForReallyContinueConfirmation();
    }
}

// make it public, because you update references after and some references can
// be lost, later you can restrict accessibility

static void refactoryPerformMovingOfStaticObjectAndMakeItPublic(
                                                                EditorMarker *mstart,
                                                                EditorMarker *point,
                                                                EditorMarker *mend,
                                                                EditorMarker *target,
                                                                char fqtname[],
                                                                unsigned *outAccessFlags,
                                                                int check,
                                                                int limitIndex
                                                                ) {
    char nameOnPoint[TMP_STRING_SIZE];
    int size;
    SymbolsMenu *mm1, *mm2;
    EditorMarker *pp, *ppp, *movedEnd;
    EditorMarkerList *occs;
    EditorRegionList *regions;
    ReferencesItem *theMethod;
    int progressi, progressn;


    movedEnd = editorDuplicateMarker(mend);
    movedEnd->offset --;

    //&editorDumpMarker(mstart);
    //&editorDumpMarker(movedEnd);

    size = mend->offset - mstart->offset;
    if (target->buffer == mstart->buffer
        && target->offset > mstart->offset
        && target->offset < mstart->offset+size) {
        ppcGenRecord(PPC_INFORMATION, "You can't move something into itself.");
        return;
    }

    // O.K. move
    refactoryApplyExpandShortNames(point->buffer, point);
    strcpy(nameOnPoint, refactoryGetIdentifierOnMarker_st(point));
    assert(strlen(nameOnPoint) < TMP_STRING_SIZE-1);
    occs = refactoryGetReferences(point->buffer, point,STANDARD_SELECT_SYMBOLS_MESSAGE,PPCV_BROWSER_TYPE_INFO);
    assert(sessionData.browserStack.top && sessionData.browserStack.top->hkSelectedSym);
    if (outAccessFlags!=NULL) {
        *outAccessFlags = sessionData.browserStack.top->hkSelectedSym->s.access;
    }
    //&refactoryEditServerParseBuffer(refactoringOptions.project, point->buffer, point, "-olcxrename");

    LIST_MERGE_SORT(EditorMarkerList, occs, editorMarkerListLess);
    LIST_LEN(progressn, EditorMarkerList, occs); progressi=0;
    regions = NULL;
    for (EditorMarkerList *ll=occs; ll!=NULL; ll=ll->next) {
        if ((! IS_DEFINITION_OR_DECL_USAGE(ll->usage.kind))
            && ll->usage.kind!=UsageConstructorDefinition) {
            pp = refactoryReplaceStaticPrefix(ll->marker, fqtname);
            ppp = editorCreateNewMarker(ll->marker->buffer, ll->marker->offset);
            editorMoveMarkerBeyondIdentifier(ppp, 1);
            regions = newEditorRegionList(pp, ppp, regions);
        }
        writeRelativeProgress((progressi++)*100/progressn);
    }
    writeRelativeProgress(100);

    size = mend->offset - mstart->offset;
    if (check==NO_CHECKS) {
        editorMoveBlock(target, mstart, size, &editorUndo);
        refactoryChangeAccessModifier(point, limitIndex, "public");
    } else {
        assert(sessionData.browserStack.top!=NULL && sessionData.browserStack.top->hkSelectedSym!=NULL);
        theMethod = &sessionData.browserStack.top->hkSelectedSym->s;
        refactoryPushAllReferencesOfMethod(point, "-olallchecks");
        createMarkersForAllReferencesInRegions(sessionData.browserStack.top->menuSym, NULL);
        editorMoveBlock(target, mstart, size, &editorUndo);
        refactoryChangeAccessModifier(point, limitIndex, "public");
        refactoryPushAllReferencesOfMethod(point, "-olallchecks");
        createMarkersForAllReferencesInRegions(sessionData.browserStack.top->menuSym, NULL);
        assert(sessionData.browserStack.top && sessionData.browserStack.top->previous);
        mm1 = sessionData.browserStack.top->previous->menuSym;
        mm2 = sessionData.browserStack.top->menuSym;
        refactoryStaticMoveCheckCorrespondance(mm1, mm2, theMethod);
    }

    //&editorDumpMarker(mstart);
    //&editorDumpMarker(movedEnd);

    // reduce long names in the method
    pp = editorDuplicateMarker(mstart);
    ppp = editorDuplicateMarker(movedEnd);
    regions = newEditorRegionList(pp, ppp, regions);

    refactoryPerformReduceNamesAndAddImports(&regions, INTERACTIVE_NO);

}

static EditorMarker *getTargetFromOptions(void) {
    EditorMarker  *target;
    EditorBuffer  *tb;
    int             tline;
    tb = editorFindFileCreate(normalizeFileName(refactoringOptions.moveTargetFile, cwd));
    target = editorCreateNewMarker(tb, 0);
    sscanf(refactoringOptions.refpar1, "%d", &tline);
    editorMoveMarkerToLineCol(target, tline, 0);
    return target;
}

static void refactoryGetMethodLimitsForMoving(EditorMarker *point,
                                              EditorMarker **_mstart,
                                              EditorMarker **_mend,
                                              int limitIndex
                                              ) {
    EditorMarker *mstart, *mend;
    // get method limites
    refactoryMakeSyntaxPassOnSource(point);
    if (s_spp[limitIndex].file==noFileIndex || s_spp[limitIndex+1].file==noFileIndex) {
        fatalError(ERR_INTERNAL, "Can't find declaration coordinates", XREF_EXIT_ERR);
    }
    mstart = editorCreateNewMarkerForPosition(&s_spp[limitIndex]);
    mend = editorCreateNewMarkerForPosition(&s_spp[limitIndex+1]);
    refactoryMoveMarkerToTheBeginOfDefinitionScope(mstart);
    refactoryMoveMarkerToTheEndOfDefinitionScope(mend);
    assert(mstart->buffer == mend->buffer);
    *_mstart = mstart;
    *_mend = mend;
}

static void refactoryGetNameOfTheClassAndSuperClass(EditorMarker *point, char *ccname, char *supercname) {
    refactoryEditServerParseBuffer(refactoringOptions.project, point->buffer,
                                   point, NULL, "-olcxcurrentclass", NULL);
    if (ccname != NULL) {
        if (s_cps.currentClassAnswer[0] == 0) {
            fatalError(ERR_ST, "can't get class on point", XREF_EXIT_ERR);
        }
        strcpy(ccname, s_cps.currentClassAnswer);
        javaDotifyClassName(ccname);
    }
    if (supercname != NULL) {
        if (s_cps.currentSuperClassAnswer[0] == 0) {
            fatalError(ERR_ST, "can't get superclass of class on point", XREF_EXIT_ERR);
        }
        strcpy(supercname, s_cps.currentSuperClassAnswer);
        javaDotifyClassName(supercname);
    }
}

// ---------------------------------------------------- MoveStaticMethod

static void refactoryMoveStaticFieldOrMethod(EditorMarker *point, int limitIndex) {
    char                targetFqtName[MAX_FILE_NAME_SIZE];
    int                 lines;
    unsigned            accFlags;
    EditorMarker      *target, *mstart, *mend;

    target = getTargetFromOptions();

    if (! validTargetPlace(target, "-olcxmmtarget")) return;
    refactoryUpdateReferences(refactoringOptions.project);
    refactoryGetNameOfTheClassAndSuperClass(target, targetFqtName, NULL);
    refactoryGetMethodLimitsForMoving(point, &mstart, &mend, limitIndex);
    lines = editorCountLinesBetweenMarkers(mstart, mend);

    // O.K. Now STARTING!
    refactoryPerformMovingOfStaticObjectAndMakeItPublic(mstart, point, mend, target,
                                                        targetFqtName,
                                                        &accFlags, APPLY_CHECKS, limitIndex);
    //&sprintf(tmpBuff,"original acc == %d", accFlags); ppcBottomInformation(tmpBuff);
    refactoryRestrictAccessibility(point, limitIndex, accFlags);

    // and generate output
    refactoryApplyWholeRefactoringFromUndo();
    ppcGotoMarker(point);
    ppcValueRecord(PPC_INDENT, lines, "");
}


static void refactoryMoveStaticMethod(EditorMarker *point) {
    refactoryMoveStaticFieldOrMethod(point, SPP_METHOD_DECLARATION_BEGIN_POSITION);
}

static void refactoryMoveStaticField(EditorMarker *point) {
    refactoryMoveStaticFieldOrMethod(point, SPP_FIELD_DECLARATION_BEGIN_POSITION);
}


// ---------------------------------------------------------- MoveField

static void refactoryMoveField(EditorMarker *point) {
    char targetFqtName[MAX_FILE_NAME_SIZE];
    char nameOnPoint[TMP_STRING_SIZE];
    char prefixDot[TMP_STRING_SIZE];
    int lines, size, check, accessFlags;
    EditorMarker *target, *mstart, *mend, *movedEnd;
    EditorMarkerList *occs;
    EditorMarker *pp, *ppp;
    EditorRegionList *regions;
    EditorUndo *undoStartPoint, *redoTrack;
    int progressi, progressn;

    target = getTargetFromOptions();
    if (refactoringOptions.refpar2!=NULL && *refactoringOptions.refpar2!=0) {
        sprintf(prefixDot, "%s.", refactoringOptions.refpar2);
    } else {
        ; // sprintf(prefixDot, "");
    }

    if (! validTargetPlace(target, "-olcxmmtarget")) return;
    refactoryUpdateReferences(refactoringOptions.project);
    refactoryGetNameOfTheClassAndSuperClass(target, targetFqtName, NULL);
    refactoryGetMethodLimitsForMoving(point, &mstart, &mend, SPP_FIELD_DECLARATION_BEGIN_POSITION);
    lines = editorCountLinesBetweenMarkers(mstart, mend);

    // O.K. Now STARTING!
    movedEnd = editorDuplicateMarker(mend);
    movedEnd->offset --;

    //&editorDumpMarker(mstart);
    //&editorDumpMarker(movedEnd);

    size = mend->offset - mstart->offset;
    if (target->buffer == mstart->buffer
        && target->offset > mstart->offset
        && target->offset < mstart->offset+size) {
        ppcGenRecord(PPC_INFORMATION, "You can't move something into itself.");
        return;
    }

    // O.K. move
    refactoryApplyExpandShortNames(point->buffer, point);
    strcpy(nameOnPoint, refactoryGetIdentifierOnMarker_st(point));
    assert(strlen(nameOnPoint) < TMP_STRING_SIZE-1);
    occs = refactoryGetReferences(point->buffer, point,STANDARD_SELECT_SYMBOLS_MESSAGE,PPCV_BROWSER_TYPE_INFO);
    assert(sessionData.browserStack.top && sessionData.browserStack.top->hkSelectedSym);
    accessFlags = sessionData.browserStack.top->hkSelectedSym->s.access;

    undoStartPoint = editorUndo;
    LIST_MERGE_SORT(EditorMarkerList, occs, editorMarkerListLess);
    LIST_LEN(progressn, EditorMarkerList, occs); progressi=0;
    regions = NULL;
    for (EditorMarkerList *ll=occs; ll!=NULL; ll=ll->next) {
        if (! IS_DEFINITION_OR_DECL_USAGE(ll->usage.kind)) {
            if (*prefixDot != 0) {
                refactoryReplaceString(ll->marker, 0, prefixDot);
            }
        }
        writeRelativeProgress((progressi++)*100/progressn);
    }
    writeRelativeProgress(100);

    size = mend->offset - mstart->offset;
    editorMoveBlock(target, mstart, size, &editorUndo);

    // reduce long names in the method
    pp = editorDuplicateMarker(mstart);
    ppp = editorDuplicateMarker(movedEnd);
    regions = newEditorRegionList(pp, ppp, regions);

    refactoryPerformReduceNamesAndAddImports(&regions, INTERACTIVE_NO);

    refactoryChangeAccessModifier(point, SPP_FIELD_DECLARATION_BEGIN_POSITION, "public");
    refactoryRestrictAccessibility(point, SPP_FIELD_DECLARATION_BEGIN_POSITION, accessFlags);

    redoTrack = NULL;
    check = refactoryMakeSafetyCheckAndUndo(point, &occs,
                                            undoStartPoint, &redoTrack);
    if (! check) {
        refactoryAskForReallyContinueConfirmation();
    }
    // and generate output
    editorApplyUndos(redoTrack, NULL, NULL, GEN_FULL_OUTPUT);


    ppcGotoMarker(point);
    ppcValueRecord(PPC_INDENT, lines, "");
}

// ---------------------------------------------------------- MoveClass

static void refactorySetMovingPrecheckStandardEnvironment(EditorMarker *point, char *targetFqtName) {
    SymbolsMenu *ss;
    refactoryEditServerParseBuffer(refactoringOptions.project, point->buffer,
                                   point, NULL, "-olcxtrivialprecheck", NULL);
    assert(sessionData.browserStack.top);
    olCreateSelectionMenu(sessionData.browserStack.top->command);
    options.moveTargetFile = refactoringOptions.moveTargetFile;
    options.moveTargetClass = targetFqtName;
    assert(sessionData.browserStack.top);
    ss = sessionData.browserStack.top->hkSelectedSym;
    assert(ss);
}

static void refactoryPerformMoveClass(EditorMarker *point,
                                      EditorMarker *target,
                                      EditorMarker **outstart,
                                      EditorMarker **outend
                                      ) {
    char spack[MAX_FILE_NAME_SIZE];
    char tpack[MAX_FILE_NAME_SIZE];
    char targetFqtName[MAX_FILE_NAME_SIZE];
    int targetIsNestedInClass;
    EditorMarker *mstart, *mend;
    SymbolsMenu *ss;
    TpCheckMoveClassData dd;

    *outstart = *outend = NULL;

    // get target place
    refactoryEditServerParseBuffer(refactoringOptions.project, target->buffer,
                                   target, NULL, "-olcxcurrentclass", NULL);
    if (s_cps.currentPackageAnswer[0] == 0) {
        errorMessage(ERR_ST, "Can't get target class or package");
        return;
    }
    if (s_cps.currentClassAnswer[0] == 0) {
        sprintf(targetFqtName, "%s", s_cps.currentPackageAnswer);
        targetIsNestedInClass = 0;
    } else {
        sprintf(targetFqtName, "%s", s_cps.currentClassAnswer);
        targetIsNestedInClass = 1;
    }
    javaDotifyClassName(targetFqtName);

    // get limits
    refactoryMakeSyntaxPassOnSource(point);
    if (s_spp[SPP_CLASS_DECLARATION_BEGIN_POSITION].file==noFileIndex
        ||s_spp[SPP_CLASS_DECLARATION_END_POSITION].file==noFileIndex) {
        fatalError(ERR_INTERNAL, "Can't find declaration coordinates", XREF_EXIT_ERR);
    }
    mstart = editorCreateNewMarkerForPosition(&s_spp[SPP_CLASS_DECLARATION_BEGIN_POSITION]);
    mend = editorCreateNewMarkerForPosition(&s_spp[SPP_CLASS_DECLARATION_END_POSITION]);
    refactoryMoveMarkerToTheBeginOfDefinitionScope(mstart);
    refactoryMoveMarkerToTheEndOfDefinitionScope(mend);

    assert(mstart->buffer == mend->buffer);

    *outstart = mstart;
    // put outend -1 to be updated during moving
    *outend = editorCreateNewMarker(mend->buffer, mend->offset-1);

    // prechecks
    refactorySetMovingPrecheckStandardEnvironment(point, targetFqtName);
    ss = sessionData.browserStack.top->hkSelectedSym;
    tpCheckFillMoveClassData(&dd, spack, tpack);
    tpCheckSourceIsNotInnerClass();
    tpCheckMoveClassAccessibilities();

    // O.K. Now STARTING!
    refactoryPerformMovingOfStaticObjectAndMakeItPublic(mstart, point, mend, target, targetFqtName, NULL, NO_CHECKS, SPP_CLASS_DECLARATION_BEGIN_POSITION);

    // recover end marker
    (*outend)->offset++;

    // finally fiddle modifiers
    if (ss->s.access & AccessStatic) {
        if (! targetIsNestedInClass) {
            // nested -> top level
            //&sprintf(tmpBuff,"removing modifier"); ppcBottomInformation(tmpBuff);
            refactoryRemoveModifier(point, SPP_CLASS_DECLARATION_BEGIN_POSITION, "static");
        }
    } else {
        if (targetIsNestedInClass) {
            // top level -> nested
            refactoryAddModifier(point, SPP_CLASS_DECLARATION_BEGIN_POSITION, "static");
        }
    }
    if (dd.transPackageMove) {
        // add public
        refactoryChangeAccessModifier(point, SPP_CLASS_DECLARATION_BEGIN_POSITION, "public");
    }
}

static void refactoryMoveClass(EditorMarker *point) {
    EditorMarker          *target, *mstart, *mend;
    int                     linenum;

    target = getTargetFromOptions();
    if (! validTargetPlace(target, "-olcxmctarget")) return;

    refactoryUpdateReferences(refactoringOptions.project);

    refactoryPerformMoveClass(point, target, &mstart, &mend);
    linenum = editorCountLinesBetweenMarkers(mstart, mend);

    // and generate output
    refactoryApplyWholeRefactoringFromUndo();
    ppcGotoMarker(point);
    ppcValueRecord(PPC_INDENT, linenum, "");

    ppcGenRecord(PPC_INFORMATION, "\nDone.\nDo not forget to remove .class files of former class.");
}

static void refactoryGetPackageNameFromMarkerFileName(EditorMarker *target, char *tclass) {
    char *dd;
    strcpy(tclass, javaCutSourcePathFromFileName(target->buffer->name));
    dd = lastOccurenceInString(tclass, '.');
    if (dd!=NULL) *dd=0;
    dd = lastOccurenceOfSlashOrBackslash(tclass);
    if (dd!=NULL) {
        *dd=0;
        javaDotifyFileName(tclass);
    } else {
        *tclass = 0;
    }
}


static void refactoryInsertPackageStatToNewFile(EditorMarker *target) {
    char tclass[MAX_FILE_NAME_SIZE];
    char pack[2*MAX_FILE_NAME_SIZE];

    refactoryGetPackageNameFromMarkerFileName(target, tclass);
    if (tclass[0] == 0) {
        sprintf(pack, "\n");
    } else {
        sprintf(pack, "package %s;\n", tclass);
    }
    sprintf(pack+strlen(pack), "\n\n");
    refactoryReplaceString(target, 0, pack);
    target->offset --;
}


static void refactoryMoveClassToNewFile(EditorMarker *point) {
    EditorMarker *target, *mstart, *mend, *npoint;
    EditorBuffer *buff;
    int linenum;

    buff = point->buffer;
    refactoryUpdateReferences(refactoringOptions.project);
    target = getTargetFromOptions();

    // insert package statement
    refactoryInsertPackageStatToNewFile(target);

    refactoryPerformMoveClass(point, target, &mstart, &mend);

    if (mstart==NULL || mend==NULL) return;
    //&editorDumpMarker(mstart);
    //&editorDumpMarker(mend);
    linenum = editorCountLinesBetweenMarkers(mstart, mend);

    // and generate output
    refactoryApplyWholeRefactoringFromUndo();

    // indentation must be at the end (undo, redo does not work with)
    ppcGotoMarker(point);
    ppcValueRecord(PPC_INDENT, linenum, "");

    // TODO check whether the original class was the only class in the file
    npoint = editorCreateNewMarker(buff, 0);
    // just to parse the file
    refactoryEditServerParseBuffer(refactoringOptions.project, npoint->buffer, npoint,NULL, "-olcxpushspecialname=", NULL);
    if (s_spp[SPP_LAST_TOP_LEVEL_CLASS_POSITION].file == noFileIndex) {
        ppcGotoMarker(npoint);
        ppcGenRecord(PPC_KILL_BUFFER_REMOVE_FILE, "This file does not contain classes anymore, can I remove it?");
    }
    ppcGenRecord(PPC_INFORMATION, "\nDone.\nDo not forget to remove .class files of former class.");
}

static void refactoryMoveAllClassesToNewFile(EditorMarker *point) {
    // TODO: this should really copy whole file, including commentaries
    // between classes, etc... Then update all references
}

static void addCopyOfMarkerToList(EditorMarkerList **ll, EditorMarker *mm, Usage usage) {
    EditorMarker          *nn;
    EditorMarkerList      *lll;
    nn = editorCreateNewMarker(mm->buffer, mm->offset);
    ED_ALLOC(lll, EditorMarkerList);
    *lll = (EditorMarkerList){.marker = nn, .usage = usage, .next = *ll};
    *ll = lll;
}

// ------------------------------------------ TurnDynamicToStatic

static void refactorVirtualToStatic(EditorMarker *point) {
    char nameOnPoint[TMP_STRING_SIZE];
    char primary[REFACTORING_TMP_STRING_SIZE];
    char fqstaticname[REFACTORING_TMP_STRING_SIZE];
    char fqthis[REFACTORING_TMP_STRING_SIZE];
    char pardecl[2*REFACTORING_TMP_STRING_SIZE];
    char parusage[REFACTORING_TMP_STRING_SIZE];
    char cid[TMP_STRING_SIZE];
    int plen, ppoffset, poffset;
    int progressi, progressj, progressn;
    EditorMarker *pp, *ppp, *nparamdefpos;
    EditorMarkerList *occs, *allrefs;
    EditorMarkerList *npoccs, *npadded, *diff1, *diff2;
    EditorRegionList *regions, **reglast, *lll;
    SymbolsMenu *csym;
    EditorUndo *undoStartPoint;
    Usage defaultUsage;

    nparamdefpos = NULL;
    refactoryUpdateReferences(refactoringOptions.project);
    strcpy(nameOnPoint, refactoryGetIdentifierOnMarker_st(point));
    assert(strlen(nameOnPoint) < TMP_STRING_SIZE-1);
    occs = refactoryPushGetAndPreCheckReferences(point->buffer, point, nameOnPoint,
                                                 "If you see this message it is highly probable that turning this virtual method into static will not be behaviour preserving! This refactoring is behaviour preserving only  if the method does not use mechanism of virtual invocations. In this dialog you should select the application classes which are refering to the method which will become static. If you can't unambiguously determine those references do not continue in this refactoring!",
                                                 PPCV_BROWSER_TYPE_WARNING);
    editorFreeMarkersAndMarkerList(occs);

    if (! tpCheckOuterScopeUsagesForDynToSt()) return;
    if (! tpCheckSuperMethodReferencesForDynToSt()) return;

    // Pass over all references and move primary prefix to first parameter
    // also insert new first parameter on definition
    csym =  sessionData.browserStack.top->hkSelectedSym;
    javaGetClassNameFromFileIndex(csym->s.vFunClass, fqstaticname, DOTIFY_NAME);
    javaDotifyClassName(fqstaticname);
    sprintf(pardecl, "%s %s", fqstaticname, refactoringOptions.refpar1);
    sprintf(fqstaticname+strlen(fqstaticname), ".");

    progressn = progressi = 0;
    for (SymbolsMenu *mm=sessionData.browserStack.top->menuSym; mm!=NULL; mm=mm->next) {
        if (mm->selected && mm->visible) {
            mm->markers = editorReferencesToMarkers(mm->s.references, filter0, NULL);
            LIST_MERGE_SORT(EditorMarkerList, mm->markers, editorMarkerListLess);
            LIST_LEN(progressj, EditorMarkerList, mm->markers);
            progressn += progressj;
        }
    }
    undoStartPoint = editorUndo;
    regions = NULL; reglast = &regions; allrefs = NULL;
    for (SymbolsMenu *mm=sessionData.browserStack.top->menuSym; mm!=NULL; mm=mm->next) {
        if (mm->selected && mm->visible) {
            javaGetClassNameFromFileIndex(mm->s.vApplClass, fqthis, DOTIFY_NAME);
            javaDotifyClassName(fqthis);
            sprintf(fqthis+strlen(fqthis), ".this");
            for (EditorMarkerList *ll=mm->markers; ll!=NULL; ll=ll->next) {
                addCopyOfMarkerToList(&allrefs, ll->marker, ll->usage);
                pp = NULL;
                if (IS_DEFINITION_OR_DECL_USAGE(ll->usage.kind)) {
                    pp = editorCreateNewMarker(ll->marker->buffer, ll->marker->offset);
                    pp->offset = refactoryAddStringAsParameter(ll->marker, ll->marker, nameOnPoint,
                                                               1, pardecl);
                    // remember definition position of new parameter
                    nparamdefpos = editorCreateNewMarker(pp->buffer, pp->offset+strlen(pardecl)-strlen(refactoringOptions.refpar1));
                } else {
                    pp = refactoryCrNewMarkerForExpressionBegin(ll->marker, GET_PRIMARY_START);
                    assert(pp!=NULL);
                    ppoffset = pp->offset;
                    plen = ll->marker->offset - pp->offset;
                    strncpy(primary, MARKER_TO_POINTER(pp), plen);
                    // delete pending dot
                    while (plen>0 && primary[plen-1]!='.') plen--;
                    if (plen>0) {
                        primary[plen-1] = 0;
                    } else {
                        if (javaLinkNameIsAnnonymousClass(mm->s.name)) {
                            strcpy(primary, "this");
                        } else {
                            strcpy(primary, fqthis);
                        }
                    }
                    refactoryReplaceString(pp, ll->marker->offset-pp->offset, fqstaticname);
                    refactoryAddStringAsParameter(ll->marker, ll->marker, nameOnPoint, 1, primary);
                    // return offset back to beginning of fqt
                    pp->offset = ppoffset;
                }
                ppp = editorCreateNewMarker(ll->marker->buffer, ll->marker->offset);
                editorMoveMarkerBeyondIdentifier(ppp, 1);

                /* TODO: clean up... */
                lll = newEditorRegionList(pp, ppp, NULL);
                *reglast = lll;
                reglast = &lll->next;
                writeRelativeProgress((progressi++)*100/progressn);
            }
            editorFreeMarkersAndMarkerList(mm->markers);
            mm->markers = NULL;
        }
    }
    writeRelativeProgress(100);

    // pop references
    sessionData.browserStack.top = sessionData.browserStack.top->previous;

    sprintf(parusage, "%s.", refactoringOptions.refpar1);

    refactoryEditServerParseBuffer(refactoringOptions.project, point->buffer, point, NULL, "-olcxmaybethis", NULL);
    olcxPushSpecial(LINK_NAME_MAYBE_THIS_ITEM, OLO_MAYBE_THIS);

    progressn = progressi = 0;
    assert(sessionData.browserStack.top && sessionData.browserStack.top->hkSelectedSym);
    for (SymbolsMenu *mm=sessionData.browserStack.top->menuSym; mm!=NULL; mm=mm->next) {
        if (mm->selected && mm->visible) {
            mm->markers = editorReferencesToMarkers(mm->s.references, filter0, NULL);
            LIST_MERGE_SORT(EditorMarkerList, mm->markers, editorMarkerListLess);
            LIST_LEN(progressj, EditorMarkerList, mm->markers);
            progressn += progressj;
        }
    }

    // passing references inside method and change them to the new parameter
    npadded = NULL;
    fillUsage(&defaultUsage, UsageDefined, 0);
    addCopyOfMarkerToList(&npadded, nparamdefpos, defaultUsage);

    for (SymbolsMenu *mm=sessionData.browserStack.top->menuSym; mm!=NULL; mm=mm->next) {
        if (mm->selected && mm->visible) {
            for (EditorMarkerList *ll=mm->markers; ll!=NULL; ll=ll->next) {
                if (ll->usage.kind == UsageMaybeQualifThisInClassOrMethod) {
                    editorUndoUntil(undoStartPoint,NULL);
                    ppcGotoMarker(ll->marker);
                    errorMessage(ERR_ST, "The method is using qualified this to access enclosed instance. Do not know how to make it static.");
                    return;
                } else if (ll->usage.kind == UsageMaybeThisInClassOrMethod) {
                    strncpy(cid, refactoryGetIdentifierOnMarker_st(ll->marker), TMP_STRING_SIZE);
                    cid[TMP_STRING_SIZE-1]=0;
                    poffset = ll->marker->offset;
                    //&sprintf(tmpBuff, "Checking %s", cid); ppcGenRecord(PPC_INFORMATION, tmpBuff);
                    if (strcmp(cid, "this")==0 || strcmp(cid, "super")==0) {
                        pp = refactoryReplaceStaticPrefix(ll->marker, "");
                        poffset = pp->offset;
                        editorFreeMarker(pp);
                        refactoryCheckedReplaceString(ll->marker, 4, cid, refactoringOptions.refpar1);
                    } else {
                        refactoryReplaceString(ll->marker, 0, parusage);
                    }
                    ll->marker->offset = poffset;
                    addCopyOfMarkerToList(&npadded, ll->marker, ll->usage);
                }
                writeRelativeProgress((progressi++)*100/progressn);
            }
        }
    }
    writeRelativeProgress(100);

    refactoryAddModifier(point, SPP_METHOD_DECLARATION_BEGIN_POSITION, "static");

    // reduce long names at the end because of recursive calls
    refactoryPerformReduceNamesAndAddImports(&regions, INTERACTIVE_NO);
    editorFreeMarkersAndRegionList(regions); regions=NULL;

    // safety check checking that new parameter has exactly
    // those references as expected (not hidden by a local variable and no
    // occurence of extra variable is resolved to parameter)
    npoccs = refactoryGetReferences(
                                    nparamdefpos->buffer, nparamdefpos,
                                    "Internal problem, during new parameter resolution",
                                    PPCV_BROWSER_TYPE_WARNING);
    editorMarkersDifferences(&npoccs, &npadded, &diff1, &diff2);
    LIST_APPEND(EditorMarkerList, diff1, diff2); diff2=NULL;
    if (diff1!=NULL) {
        ppcGotoMarker(point);
        refactoryShowSafetyCheckFailingDialog(&diff1, "The new parameter conflicts with existing symbols");
    }

    if (npoccs != NULL && npoccs->next == NULL) {
        // only one occurence, this must be the definition
        // but check it for being sure
        // maybe you should update references and delete the parameter
        // after, but for now, use computed references, it should work.
        if (IS_DEFINITION_USAGE(npoccs->usage.kind)) {
            refactoryApplyParamManip(nameOnPoint, allrefs, PPC_AVR_DEL_PARAMETER, 1, 1);
            //& refactoryDeleteParameter(point, nameOnPoint, 1, UsageDefined);
        }

    }

    // TODO!!! add safety checks, as changing the profile of the method
    // can introduce new conflicts

    editorFreeMarkersAndMarkerList(allrefs); allrefs=NULL;

    refactoryApplyWholeRefactoringFromUndo();
    ppcGotoMarker(point);
}

static int noSpaceChar(int c) {return !isspace(c);}

static void refactoryPushMethodSymbolsPlusThoseWithClearedRegion(EditorMarker *m1, EditorMarker *m2) {
    char spaces[REFACTORING_TMP_STRING_SIZE];
    EditorUndo *undoMark;
    int slen;

    assert(m1->buffer == m2->buffer);
    undoMark = editorUndo;
    refactoryPushAllReferencesOfMethod(m1,NULL);
    slen = m2->offset-m1->offset;
    assert(slen>=0 && slen<REFACTORING_TMP_STRING_SIZE);
    memset(spaces, ' ', slen);
    spaces[slen]=0;
    refactoryReplaceString(m1, slen, spaces);
    refactoryPushAllReferencesOfMethod(m1,NULL);
    editorUndoUntil(undoMark,NULL);
}

static int refactoryIsMethodPartRedundant(
                                          EditorMarker *m1,
                                          EditorMarker *m2
) {
    SymbolsMenu *mm1, *mm2;
    Reference *diff;
    EditorMarkerList *lll, *ll;
    bool res = true;

    refactoryPushMethodSymbolsPlusThoseWithClearedRegion(m1, m2);
    assert(sessionData.browserStack.top && sessionData.browserStack.top->previous);
    mm1 = sessionData.browserStack.top->menuSym;
    mm2 = sessionData.browserStack.top->previous->menuSym;
    while (mm1!=NULL && mm2!=NULL && res) {
        //&symbolRefItemDump(&mm1->s); dumpReferences(mm1->s.references);
        //&symbolRefItemDump(&mm2->s); dumpReferences(mm2->s.references);
        olcxReferencesDiff(&mm1->s.references, &mm2->s.references, &diff);
        if (diff!=NULL) {
            lll = editorReferencesToMarkers(diff, filter0, NULL);
            LIST_MERGE_SORT(EditorMarkerList, lll, editorMarkerListLess);
            for (ll=lll; ll!=NULL; ll=ll->next) {
                assert(ll->marker->buffer == m1->buffer);
                //&sprintf(tmpBuff, "checking diff %d", ll->marker->offset); ppcGenRecord(PPC_INFORMATION, tmpBuff);
                if (editorMarkerLess(ll->marker, m1) || editorMarkerLessOrEq(m2, ll->marker)) {
                    res=false;
                }
            }
            editorFreeMarkersAndMarkerList(lll);
            olcxFreeReferences(diff);
        }
        mm1 = mm1->next;
        mm2 = mm2->next;
    }
    olcxPopOnly();
    olcxPopOnly();

    return res;
}

static void refactoryRemoveMethodPartIfRedundant(EditorMarker *m, int len) {
    EditorMarker *mm;
    mm = editorCreateNewMarker(m->buffer, m->offset+len);
    if (refactoryIsMethodPartRedundant(m, mm)) {
        refactoryReplaceString(m, len, "");
    }
    editorFreeMarker(mm);
}

static int isMethodBeg(int c) { return c=='{'; }

static bool staticToDynCanBeThisOccurence(EditorMarker *pp, char *param, int *rlen) {
    char *pp2;
    EditorMarker  *mm;
    bool res = false;

    mm = editorCreateNewMarker(pp->buffer, pp->offset);
    pp2 = strchr(param, '.');
    if (pp2==NULL) {
        *rlen = strlen(param);
        res = strcmp(refactoryGetIdentifierOnMarker_st(pp), param)==0;
        goto fini;
    }
    // param.field so parse it
    if (strncmp(refactoryGetIdentifierOnMarker_st(mm), param, pp2-param)!=0) goto fini;
    mm->offset += (pp2-param);
    editorMoveMarkerToNonBlank(mm, 1);
    if (*(MARKER_TO_POINTER(mm)) != '.') goto fini;
    mm->offset ++;
    editorMoveMarkerToNonBlank(mm, 1);
    if (strcmp(refactoryGetIdentifierOnMarker_st(mm), pp2+1)!=0) goto fini;
    *rlen = mm->offset - pp->offset + strlen(pp2+1);
    res = true;
 fini:
    editorFreeMarker(mm);
    return res;
}

// ----------------------------------------------- TurnStaticToDynamic

static void refactoryTurnStaticToDynamic(EditorMarker *point) {
    char                nameOnPoint[TMP_STRING_SIZE];
    char                param[REFACTORING_TMP_STRING_SIZE];
    char                tparam[REFACTORING_TMP_STRING_SIZE];
    char                testi[2*REFACTORING_TMP_STRING_SIZE];
    int                 plen, tplen, rlen, res, argn, bi;
    int                 classnum, parclassnum;
    int                 progressi, progressn;
    EditorMarker      *mm, *m1, *m2, *pp;
    EditorMarkerList  *occs, *poccs;
    EditorUndo        *checkPoint;

    refactoryUpdateReferences(refactoringOptions.project);

    argn = 0;
    sscanf(refactoringOptions.refpar1, "%d", &argn);

    assert(argn!=0);

    strcpy(nameOnPoint, refactoryGetIdentifierOnMarker_st(point));
    res = refactoryGetParamNamePosition(point, nameOnPoint, argn);
    if (res != RETURN_OK) {
        ppcGotoMarker(point);
        errorMessage(ERR_INTERNAL, "Can't determine position of parameter");
        return;
    }
    mm = editorCreateNewMarkerForPosition(&s_paramPosition);
    if (refactoringOptions.refpar2[0]!=0) {
        sprintf(param, "%s.%s", refactoryGetIdentifierOnMarker_st(mm), refactoringOptions.refpar2);
    } else {
        sprintf(param, "%s", refactoryGetIdentifierOnMarker_st(mm));
    }
    plen = strlen(param);

    // TODO!!! precheck
    refactoryEditServerParseBuffer(refactoringOptions.project, point->buffer,
                                   point,NULL, "-olcxcurrentclass",NULL);
    if (s_cps.currentClassAnswer[0] == 0) {
        errorMessage(ERR_INTERNAL, "Can't get current class");
        return;
    }
    classnum = getClassNumFromClassLinkName(s_cps.currentClassAnswer, noFileIndex);
    if (classnum==noFileIndex) {
        errorMessage(ERR_INTERNAL, "Problem when getting current class");
        return;
    }

    checkPoint = editorUndo;
    pp = editorCreateNewMarker(point->buffer, point->offset);
    res = editorRunWithMarkerUntil(pp, isMethodBeg, 1);
    if (! res) {
        errorMessage(ERR_INTERNAL, "Can't find beginning of method");
        return;
    }
    pp->offset ++;
    sprintf(testi, "xxx(%s)", param);
    bi = pp->offset + 3 + plen;
    editorReplaceString(pp->buffer, pp->offset, 0, testi, strlen(testi), &editorUndo);
    pp->offset = bi;
    refactoryEditServerParseBuffer(refactoringOptions.project, pp->buffer,
                                   pp, NULL, "-olcxgetsymboltype", "-no-errors");
    // -no-errors is basically very dangerous in this context, recover it in s_opt
    options.noErrors = 0;
    if (!s_olstringServed) {
        errorMessage(ERR_ST, "Can't infer type for parameter/field");
        return;
    }
    parclassnum = getClassNumFromClassLinkName(s_olSymbolClassType, noFileIndex);
    if (parclassnum==noFileIndex) {
        errorMessage(ERR_INTERNAL, "Problem when getting parameter/field class");
        return;
    }
    if (! isSmallerOrEqClass(parclassnum, classnum)) {
        errorMessage(ERR_ST, "Type of parameter.field must be current class or its subclass");
        return;
    }

    editorUndoUntil(checkPoint, NULL);
    editorFreeMarker(pp);

    // O.K. turn it virtual

    // STEP 1) inspect all references and copy the parameter to application object
    strcpy(nameOnPoint, refactoryGetIdentifierOnMarker_st(point));
    assert(strlen(nameOnPoint) < TMP_STRING_SIZE-1);
    occs = refactoryPushGetAndPreCheckReferences(point->buffer, point, nameOnPoint,STANDARD_SELECT_SYMBOLS_MESSAGE,PPCV_BROWSER_TYPE_INFO);

    LIST_LEN(progressn, EditorMarkerList, occs); progressi=0;
    for (EditorMarkerList *ll=occs; ll!=NULL; ll=ll->next) {
        if (! IS_DEFINITION_OR_DECL_USAGE(ll->usage.kind)) {
            res = refactoryGetParamPosition(ll->marker, nameOnPoint, argn);
            if (res == RETURN_OK) {
                m1 = editorCreateNewMarkerForPosition(&s_paramBeginPosition);
                m1->offset ++;
                editorRunWithMarkerUntil(m1, noSpaceChar, 1);
                m2 = editorCreateNewMarkerForPosition(&s_paramEndPosition);
                m2->offset --;
                editorRunWithMarkerUntil(m2, noSpaceChar, -1);
                m2->offset ++;
                tplen = m2->offset - m1->offset;
                assert(tplen < REFACTORING_TMP_STRING_SIZE-1);
                strncpy(tparam, MARKER_TO_POINTER(m1), tplen);
                tparam[tplen] = 0;
                if (strcmp(tparam, "this")!=0) {
                    if (refactoringOptions.refpar2[0]!=0) {
                        sprintf(tparam+strlen(tparam), ".%s", refactoringOptions.refpar2);
                    }
                    pp = refactoryReplaceStaticPrefix(ll->marker, tparam);
                    editorFreeMarker(pp);
                }
                editorFreeMarker(m2);
                editorFreeMarker(m1);
            }
        }
        writeRelativeProgress((progressi++)*100/progressn);
    }
    writeRelativeProgress(100);
    // you can remove 'static' now, hope it is not virtual symbol,
    refactoryRemoveModifier(point, SPP_METHOD_DECLARATION_BEGIN_POSITION, "static");

    // TODO verify that new profile does not make clash


    // STEP 2) inspect all usages of parameter and replace them by 'this',
    // remove this this if useless
    poccs = refactoryGetReferences(
                                   mm->buffer, mm,
                                   STANDARD_SELECT_SYMBOLS_MESSAGE, PPCV_BROWSER_TYPE_INFO
                                   );
    for (EditorMarkerList *ll=poccs; ll!=NULL; ll=ll->next) {
        if (! IS_DEFINITION_OR_DECL_USAGE(ll->usage.kind)) {
            if (ll->marker->offset+plen <= ll->marker->buffer->allocation.bufferSize
                // TODO! do this at least little bit better, by skipping spaces, etc.
                && staticToDynCanBeThisOccurence(ll->marker, param, &rlen)) {
                refactoryReplaceString(ll->marker, rlen, "this");
                refactoryRemoveMethodPartIfRedundant(ll->marker, strlen("this."));
            }
        }
    }

    // STEP3) remove the parameter if not used anymore
    if (! refactoryIsThisSymbolUsed(mm)) {
        refactoryApplyParameterManipulation(point->buffer, point, PPC_AVR_DEL_PARAMETER, argn, 0);
    } else {
        // at least update the progress
        writeRelativeProgress(100);
    }

    // and generate output
    refactoryApplyWholeRefactoringFromUndo();
    ppcGotoMarker(point);

    // DONE!
}


// ------------------------------------------------------ ExtractMethod

static void refactoryExtractMethod(EditorMarker *point, EditorMarker *mark) {
    refactoryEditServerParseBuffer(refactoringOptions.project, point->buffer, point, mark,
                                   "-olcxextract", NULL);
}

static void refactoryExtractMacro(EditorMarker *point, EditorMarker *mark) {
    refactoryEditServerParseBuffer(refactoringOptions.project, point->buffer, point, mark,
                                   "-olcxextract", "-olexmacro");
}

// ------------------------------------------------------- Encapsulate

static Reference *refactoryCheckEncapsulateGetterSetterForExistingMethods(char *mname) {
    SymbolsMenu *hk;
    char clist[REFACTORING_TMP_STRING_SIZE];
    char cn[TMP_STRING_SIZE];
    Reference *anotherDefinition = NULL;

    clist[0] = 0;
    assert(sessionData.browserStack.top);
    assert(sessionData.browserStack.top->hkSelectedSym);
    assert(sessionData.browserStack.top->menuSym);
    hk = sessionData.browserStack.top->hkSelectedSym;
    for (SymbolsMenu *mm=sessionData.browserStack.top->menuSym; mm!=NULL; mm=mm->next) {
        if (isSameCxSymbol(&mm->s, &hk->s) && mm->defRefn != 0) {
            if (mm->s.vFunClass==hk->s.vFunClass) {
                // find definition of another function
                for (Reference *rr=mm->s.references; rr!=NULL; rr=rr->next) {
                    if (IS_DEFINITION_USAGE(rr->usage.kind)) {
                        if (positionsAreNotEqual(rr->position, hk->defpos)) {
                            anotherDefinition = rr;
                            goto refbreak;
                        }
                    }
                }
            refbreak:;
            } else {
                if (isSmallerOrEqClass(mm->s.vFunClass, hk->s.vFunClass)
                    || isSmallerOrEqClass(hk->s.vFunClass, mm->s.vFunClass)) {
                    linkNamePrettyPrint(cn,
                                        getShortClassNameFromClassNum_st(mm->s.vFunClass),
                                        TMP_STRING_SIZE,
                                        SHORT_NAME);
                    if (substringIndex(clist, cn) == -1) {
                        sprintf(clist+strlen(clist), " %s", cn);
                    }
                }
            }
        }
    }
    // O.K. now I have list of classes in clist
    char tmpBuff[TMP_BUFF_SIZE];
    if (clist[0] != 0) {
        sprintf(tmpBuff,
                "The method %s is also defined in the following related classes: %s. Its definition in current class may (under some circumstance) change your program behaviour. Do you really want to continue with this refactoring?", mname, clist);
        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
        ppcGenRecord(PPC_ASK_CONFIRMATION, tmpBuff);
    }
    if (anotherDefinition!=NULL) {
        sprintf(tmpBuff, "The method %s is yet defined in this class. C-xrefactory will not generate new method. Continue anyway?", mname);
        formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
        ppcGenRecord(PPC_ASK_CONFIRMATION, tmpBuff);
    }
    return anotherDefinition;
}

static void refactoryAddMethodToForbiddenRegions(Reference *methodRef,
                                                 EditorRegionList **forbiddenRegions
                                                 ) {
    EditorMarker *mm, *mb, *me;

    mm = editorCreateNewMarkerForPosition(&methodRef->position);
    refactoryMakeSyntaxPassOnSource(mm);
    mb = editorCreateNewMarkerForPosition(&s_spp[SPP_METHOD_DECLARATION_BEGIN_POSITION]);
    me = editorCreateNewMarkerForPosition(&s_spp[SPP_METHOD_DECLARATION_END_POSITION]);
    *forbiddenRegions = newEditorRegionList(mb, me, *forbiddenRegions);
    editorFreeMarker(mm);
}

static void refactoryPerformEncapsulateField(EditorMarker *point,
                                             EditorRegionList **forbiddenRegions
                                             ) {
    char nameOnPoint[TMP_STRING_SIZE];
    char upcasedName[TMP_STRING_SIZE];
    char getter[2*TMP_STRING_SIZE];
    char setter[2*TMP_STRING_SIZE];
    char cclass[TMP_STRING_SIZE];
    char getterBody[3*REFACTORING_TMP_STRING_SIZE];
    char setterBody[3*REFACTORING_TMP_STRING_SIZE];
    char declarator[REFACTORING_TMP_STRING_SIZE];
    char *scclass;
    int nameOnPointLen, declLen, indlines, indoffset;
    Reference *anotherGetter, *anotherSetter;
    unsigned accFlags;
    EditorMarkerList  *occs, *insiders, *outsiders;
    EditorMarker *dte, *dtb, *de;
    EditorMarker *getterm, *setterm, *tbeg, *tend;
    EditorUndo *beforeInsertionUndo;
    EditorMarker *eqm, *ee, *db;
    UNUSED db;

    strcpy(nameOnPoint, refactoryGetIdentifierOnMarker_st(point));
    nameOnPointLen = strlen(nameOnPoint);
    assert(nameOnPointLen < TMP_STRING_SIZE-1);
    occs = refactoryPushGetAndPreCheckReferences(point->buffer, point, nameOnPoint,
                                                 ERROR_SELECT_SYMBOLS_MESSAGE,
                                                 PPCV_BROWSER_TYPE_WARNING);
    for (EditorMarkerList *ll=occs; ll!=NULL; ll=ll->next) {
        if (ll->usage.kind == UsageAddrUsed) {
            char tmpBuff[TMP_BUFF_SIZE];
            ppcGotoMarker(ll->marker);
            sprintf(tmpBuff, "There is a combined l-value reference of the field. Current version of C-xrefactory doesn't  know how  to encapsulate such  assignment. Please, turn it into simple assignment (i.e. field = field 'op' ...;) first.");
            formatOutputLine(tmpBuff, ERROR_MESSAGE_STARTING_OFFSET);
            errorMessage(ERR_ST, tmpBuff);
            return;
        }
    }

    assert(sessionData.browserStack.top && sessionData.browserStack.top->hkSelectedSym);
    accFlags = sessionData.browserStack.top->hkSelectedSym->s.access;

    cclass[0] = 0; scclass = cclass;
    if (accFlags&AccessStatic) {
        refactoryGetNameOfTheClassAndSuperClass(point, cclass, NULL);
        scclass = lastOccurenceInString(cclass, '.');
        if (scclass == NULL) scclass = cclass;
        else scclass ++;
    }

    strcpy(upcasedName, nameOnPoint);
    upcasedName[0] = toupper(upcasedName[0]);
    sprintf(getter, "get%s", upcasedName);
    sprintf(setter, "set%s", upcasedName);

    // generate getter and setter bodies
    refactoryMakeSyntaxPassOnSource(point);
    db = editorCreateNewMarkerForPosition(&s_spp[SPP_FIELD_DECLARATION_BEGIN_POSITION]);
    dtb = editorCreateNewMarkerForPosition(&s_spp[SPP_FIELD_DECLARATION_TYPE_BEGIN_POSITION]);
    dte = editorCreateNewMarkerForPosition(&s_spp[SPP_FIELD_DECLARATION_TYPE_END_POSITION]);
    de = editorCreateNewMarkerForPosition(&s_spp[SPP_FIELD_DECLARATION_END_POSITION]);
    refactoryMoveMarkerToTheEndOfDefinitionScope(de);
    assert(dtb->buffer == dte->buffer);
    assert(dtb->offset <= dte->offset);
    declLen = dte->offset - dtb->offset;
    strncpy(declarator, MARKER_TO_POINTER(dtb), declLen);
    declarator[declLen] = 0;

    sprintf(getterBody, "public %s%s %s() {\nreturn %s;\n}\n",
            ((accFlags&AccessStatic)?"static ":""),
            declarator, getter, nameOnPoint);
    sprintf(setterBody, "public %s%s %s(%s %s) {\n%s.%s = %s;\nreturn %s;\n}\n",
            ((accFlags&AccessStatic)?"static ":""),
            declarator, setter, declarator, nameOnPoint,
            ((accFlags&AccessStatic)?scclass:"this"),
            nameOnPoint, nameOnPoint, nameOnPoint);

    beforeInsertionUndo = editorUndo;
    if (CHAR_BEFORE_MARKER(de) != '\n') refactoryReplaceString(de, 0, "\n");
    tbeg = editorDuplicateMarker(de);
    tbeg->offset --;

    getterm = setterm = NULL;
    getterm = editorCreateNewMarker(de->buffer, de->offset-1);
    refactoryReplaceString(de, 0, getterBody);
    getterm->offset += substringIndex(getterBody, getter)+1;

    if ((accFlags & AccessFinal) ==  0) {
        setterm = editorCreateNewMarker(de->buffer, de->offset-1);
        refactoryReplaceString(de, 0, setterBody);
        setterm->offset += substringIndex(setterBody, setter)+1;
    }
    tbeg->offset ++;
    tend = editorDuplicateMarker(de);

    // check if not yet defined or used
    anotherGetter = anotherSetter = NULL;
    refactoryPushReferences(getterm->buffer, getterm, "-olcxrename", NULL, 0);
    anotherGetter = refactoryCheckEncapsulateGetterSetterForExistingMethods(getter);
    editorFreeMarker(getterm);
    if ((accFlags & AccessFinal) ==  0) {
        refactoryPushReferences(setterm->buffer, setterm, "-olcxrename", NULL, 0);
        anotherSetter = refactoryCheckEncapsulateGetterSetterForExistingMethods(setter);
        editorFreeMarker(setterm);
    }
    if (anotherGetter!=NULL || anotherSetter!=NULL) {
        if (anotherGetter!=NULL) {
            refactoryAddMethodToForbiddenRegions(anotherGetter, forbiddenRegions);
        }
        if (anotherSetter!=NULL) {
            refactoryAddMethodToForbiddenRegions(anotherSetter, forbiddenRegions);
        }
        editorUndoUntil(beforeInsertionUndo, &editorUndo);
        de->offset = tbeg->offset;
        if (CHAR_BEFORE_MARKER(de) != '\n') refactoryReplaceString(de, 0, "\n");
        tbeg->offset --;
        if (! anotherGetter) {
            refactoryReplaceString(de, 0, getterBody);
        }
        if ((accFlags & AccessFinal) ==  0 && ! anotherSetter) {
            refactoryReplaceString(de, 0, setterBody);
        }
        tend->offset = de->offset;
        tbeg->offset ++;
    }
    // do not move this before, as anotherdef reference would be freed!
    if ((accFlags & AccessFinal) ==  0) olcxPopOnly();
    olcxPopOnly();

    // generate getter and setter invocations
    editorSplitMarkersWithRespectToRegions(&occs, forbiddenRegions, &insiders, &outsiders);
    for (EditorMarkerList *ll=outsiders; ll!=NULL; ll=ll->next) {
        if (ll->usage.kind == UsageLvalUsed) {
            refactoryMakeSyntaxPassOnSource(ll->marker);
            if (s_spp[SPP_ASSIGNMENT_OPERATOR_POSITION].file == noFileIndex) {
                errorMessage(ERR_INTERNAL, "Can't get assignment coordinates");
            } else {
                eqm = editorCreateNewMarkerForPosition(&s_spp[SPP_ASSIGNMENT_OPERATOR_POSITION]);
                ee = editorCreateNewMarkerForPosition(&s_spp[SPP_ASSIGNMENT_END_POSITION]);
                // make it in two steps to move the ll->d marker to the end
                refactoryCheckedReplaceString(ll->marker, nameOnPointLen, nameOnPoint, "");
                refactoryReplaceString(ll->marker, 0, setter);
                refactoryReplaceString(ll->marker, 0, "(");
                editorRemoveBlanks(ll->marker, 1, &editorUndo);
                refactoryCheckedReplaceString(eqm, 1, "=", "");
                editorRemoveBlanks(eqm, 0, &editorUndo);
                refactoryReplaceString(ee, 0, ")");
                ee->offset --;
                editorRemoveBlanks(ee, -1, &editorUndo);
                editorFreeMarker(eqm);
                editorFreeMarker(ee);
            }
        } else if (! IS_DEFINITION_OR_DECL_USAGE(ll->usage.kind)) {
            refactoryCheckedReplaceString(ll->marker, nameOnPointLen, nameOnPoint, "");
            refactoryReplaceString(ll->marker, 0, getter);
            refactoryReplaceString(ll->marker, 0, "()");
        }
    }

    refactoryRestrictAccessibility(point, SPP_FIELD_DECLARATION_BEGIN_POSITION, AccessPrivate);

    indoffset = tbeg->offset;
    indlines = editorCountLinesBetweenMarkers(tbeg, tend);

    // and generate output
    refactoryApplyWholeRefactoringFromUndo();

    // put it here, undo-redo sometimes shifts markers
    de->offset = indoffset;
    ppcGotoMarker(de);
    ppcValueRecord(PPC_INDENT, indlines, "");

    ppcGotoMarker(point);
}

static void refactorySelfEncapsulateField(EditorMarker *point) {
    EditorRegionList  *forbiddenRegions;
    forbiddenRegions = NULL;
    refactoryUpdateReferences(refactoringOptions.project);
    refactoryPerformEncapsulateField(point, &forbiddenRegions);
}

static void refactoryEncapsulateField(EditorMarker *point) {
    EditorRegionList  *forbiddenRegions;
    EditorMarker      *cb, *ce;

    refactoryUpdateReferences(refactoringOptions.project);

    //&editorDumpMarker(point);
    refactoryMakeSyntaxPassOnSource(point);
    //&editorDumpMarker(point);
    if (s_spp[SPP_CLASS_DECLARATION_BEGIN_POSITION].file == noFileIndex
        || s_spp[SPP_CLASS_DECLARATION_END_POSITION].file == noFileIndex) {
        fatalError(ERR_INTERNAL, "can't deetrmine class coordinates", XREF_EXIT_ERR);
    }

    cb = editorCreateNewMarkerForPosition(&s_spp[SPP_CLASS_DECLARATION_BEGIN_POSITION]);
    ce = editorCreateNewMarkerForPosition(&s_spp[SPP_CLASS_DECLARATION_END_POSITION]);

    forbiddenRegions = newEditorRegionList(cb, ce, NULL);

    //&editorDumpMarker(point);
    refactoryPerformEncapsulateField(point, &forbiddenRegions);
}

// -------------------------------------------------- pulling-up/pushing-down


static SymbolsMenu *refactoryFindSymbolCorrespondingToReferenceWrtPullUpPushDown(
                                                                                     SymbolsMenu *menu2, SymbolsMenu *mm1, EditorMarkerList *rr1
                                                                                     ) {
    SymbolsMenu *mm2;
    EditorMarkerList *rr2;

    // find corresponding reference
    for (mm2=menu2; mm2!=NULL; mm2=mm2->next) {
        if (mm1->s.type!=mm2->s.type && mm2->s.type!=TypeInducedError) continue;
        for (rr2=mm2->markers; rr2!=NULL; rr2=rr2->next) {
            if (MARKER_EQ(rr1->marker, rr2->marker)) goto breakrr2;
        }
    breakrr2:
        // check if symbols corresponds
        if (rr2!=NULL && symbolsCorrespondWrtMoving(mm1, mm2, OLO_PP_PRE_CHECK)) {
            goto breakmm2;;
        }
        //&fprintf(dumpOut, "Checking %s\n", mm2->s.name);
    }
 breakmm2:
    return mm2;
}

static bool refactoryIsMethodPartRedundantWrtPullUpPushDown(EditorMarker *m1, EditorMarker *m2) {
    SymbolsMenu *mm1, *mm2;
    bool res;
    EditorRegionList *regions;
    EditorBuffer *buf;

    assert(m1->buffer == m2->buffer);

    regions = NULL;
    buf = m1->buffer;
    regions = newEditorRegionList(editorCrMarkerForBufferBegin(buf),
                              editorDuplicateMarker(m1),
                              regions);
    regions = newEditorRegionList(editorDuplicateMarker(m2),
                                  editorCrMarkerForBufferEnd(buf),
                                  regions);

    refactoryPushMethodSymbolsPlusThoseWithClearedRegion(m1, m2);
    assert(sessionData.browserStack.top && sessionData.browserStack.top->previous);
    mm1 = sessionData.browserStack.top->menuSym;
    mm2 = sessionData.browserStack.top->previous->menuSym;
    createMarkersForAllReferencesInRegions(mm1, &regions);
    createMarkersForAllReferencesInRegions(mm2, &regions);

    res = true;
    while (mm1!=NULL) {
        for (EditorMarkerList *rr1=mm1->markers; rr1!=NULL; rr1=rr1->next) {
            if (refactoryFindSymbolCorrespondingToReferenceWrtPullUpPushDown(mm2, mm1, rr1)==NULL) {
                res = false;
                goto fini;
            }
        }
        mm1=mm1->next;
    }
 fini:
    editorFreeMarkersAndRegionList(regions);
    olcxPopOnly();
    olcxPopOnly();
    return res;
}

static EditorMarkerList *refactoryPullUpPushDownDifferences(
                                                              SymbolsMenu *menu1, SymbolsMenu *menu2, ReferencesItem *theMethod
                                                              ) {
    SymbolsMenu *mm1, *mm2;
    EditorMarkerList *rr, *diff;

    diff = NULL;
    mm1 = menu1;
    while (mm1!=NULL) {
        // do not check recursive calls
        if (isSameCxSymbolIncludingFunctionClass(&mm1->s, theMethod)) goto cont;
        // nor local variables
        if (mm1->s.storage == StorageAuto) goto cont;
        // nor labels
        if (mm1->s.type == TypeLabel) goto cont;
        // do not check also any symbols from classes defined in inner scope
        if (isStrictlyEnclosingClass(mm1->s.vFunClass, theMethod->vFunClass)) goto cont;
        // (maybe I should not test any local symbols ???)
        // O.K. something to be checked, find correspondance in mm2
        //&fprintf(dumpOut, "Looking for correspondance to %s\n", mm1->s.name);
        for (EditorMarkerList *rr1=mm1->markers; rr1!=NULL; rr1=rr1->next) {
            mm2 = refactoryFindSymbolCorrespondingToReferenceWrtPullUpPushDown(menu2, mm1, rr1);
            if (mm2==NULL) {
                ED_ALLOC(rr, EditorMarkerList);
                *rr = (EditorMarkerList){.marker = editorDuplicateMarker(rr1->marker), .usage = rr1->usage, .next = diff};
                diff = rr;
            }
        }
    cont:
        mm1=mm1->next;
    }
    return diff;
}

static void refactoryPullUpPushDownCheckCorrespondance(
                                                       SymbolsMenu *menu1, SymbolsMenu *menu2, ReferencesItem *theMethod
                                                       ) {
    EditorMarkerList *diff;

    diff = refactoryPullUpPushDownDifferences(menu1, menu2, theMethod);
    if (diff!=NULL) {
        refactoryShowSafetyCheckFailingDialog(&diff, "These references will be  misinterpreted after refactoring");
        editorFreeMarkersAndMarkerList(diff); diff=NULL;
        refactoryAskForReallyContinueConfirmation();
    }
}

static void refactoryReduceParenthesesAroundExpression(EditorMarker *mm, char *expression) {
    EditorMarker  *lp, *rp, *eb, *ee;
    int             elen;
    refactoryMakeSyntaxPassOnSource(mm);
    if (s_spp[SPP_PARENTHESED_EXPRESSION_LPAR_POSITION].file != noFileIndex) {
        assert(s_spp[SPP_PARENTHESED_EXPRESSION_RPAR_POSITION].file != noFileIndex);
        assert(s_spp[SPP_PARENTHESED_EXPRESSION_BEGIN_POSITION].file != noFileIndex);
        assert(s_spp[SPP_PARENTHESED_EXPRESSION_END_POSITION].file != noFileIndex);
        elen = strlen(expression);
        lp = editorCreateNewMarkerForPosition(&s_spp[SPP_PARENTHESED_EXPRESSION_LPAR_POSITION]);
        rp = editorCreateNewMarkerForPosition(&s_spp[SPP_PARENTHESED_EXPRESSION_RPAR_POSITION]);
        eb = editorCreateNewMarkerForPosition(&s_spp[SPP_PARENTHESED_EXPRESSION_BEGIN_POSITION]);
        ee = editorCreateNewMarkerForPosition(&s_spp[SPP_PARENTHESED_EXPRESSION_END_POSITION]);
        if (ee->offset - eb->offset == elen && strncmp(MARKER_TO_POINTER(eb), expression, elen)==0) {
            refactoryReplaceString(lp, 1, "");
            refactoryReplaceString(rp, 1, "");
        }
        editorFreeMarker(lp);
        editorFreeMarker(rp);
        editorFreeMarker(eb);
        editorFreeMarker(ee);
    }
}

static void refactoryRemoveRedundantParenthesesAroundThisOrSuper(EditorMarker *mm, char *keyword) {
    char *ss;

    ss = refactoryGetIdentifierOnMarker_st(mm);
    if (strcmp(ss, keyword)==0) {
        refactoryReduceParenthesesAroundExpression(mm, keyword);
    }
}

static void refactoryReduceCastedThis(EditorMarker *mm, char *superFqtName) {
    EditorMarker  *lp, *rp, *eb, *ee, *tb, *te, *rr, *dd;
    char            *ss;
    int             superFqtLen, castExprLen;
    char            castExpr[MAX_FILE_NAME_SIZE];
    superFqtLen = strlen(superFqtName);
    ss = refactoryGetIdentifierOnMarker_st(mm);
    if (strcmp(ss,"this")==0) {
        refactoryMakeSyntaxPassOnSource(mm);
        if (s_spp[SPP_CAST_LPAR_POSITION].file != noFileIndex) {
            assert(s_spp[SPP_CAST_RPAR_POSITION].file != noFileIndex);
            assert(s_spp[SPP_CAST_EXPRESSION_BEGIN_POSITION].file != noFileIndex);
            assert(s_spp[SPP_CAST_EXPRESSION_END_POSITION].file != noFileIndex);
            lp = editorCreateNewMarkerForPosition(&s_spp[SPP_CAST_LPAR_POSITION]);
            rp = editorCreateNewMarkerForPosition(&s_spp[SPP_CAST_RPAR_POSITION]);
            tb = editorCreateNewMarkerForPosition(&s_spp[SPP_CAST_TYPE_BEGIN_POSITION]);
            te = editorCreateNewMarkerForPosition(&s_spp[SPP_CAST_TYPE_END_POSITION]);
            eb = editorCreateNewMarkerForPosition(&s_spp[SPP_CAST_EXPRESSION_BEGIN_POSITION]);
            ee = editorCreateNewMarkerForPosition(&s_spp[SPP_CAST_EXPRESSION_END_POSITION]);
            rp->offset ++;
            if (ee->offset - eb->offset == 4 /*strlen("this")*/) {
                if (refactoryIsMethodPartRedundantWrtPullUpPushDown(lp, rp)) {
                    refactoryReplaceString(lp, rp->offset-lp->offset, "");
                } else if (te->offset - tb->offset == superFqtLen
                           && strncmp(MARKER_TO_POINTER(tb), superFqtName, superFqtLen) == 0) {
                    // a little bit hacked:  ((superfqt)this).  -> super
                    rr = editorDuplicateMarker(ee);
                    editorMoveMarkerToNonBlank(rr, 1);
                    if (CHAR_ON_MARKER(rr) == ')') {
                        dd = editorDuplicateMarker(rr);
                        dd->offset ++;
                        editorMoveMarkerToNonBlank(dd, 1);
                        if (CHAR_ON_MARKER(dd) == '.') {
                            castExprLen = ee->offset - lp->offset;
                            strncpy(castExpr, MARKER_TO_POINTER(lp), castExprLen);
                            castExpr[castExprLen]=0;
                            refactoryReduceParenthesesAroundExpression(mm, castExpr);
                            refactoryReplaceString(lp, ee->offset-lp->offset, "super");
                        }
                        editorFreeMarker(dd);
                    }
                    editorFreeMarker(rr);
                }
            }
            editorFreeMarker(lp);
            editorFreeMarker(rp);
            editorFreeMarker(tb);
            editorFreeMarker(te);
            editorFreeMarker(eb);
            editorFreeMarker(ee);
        }
    }
}

static bool refactoryIsThereACastOfThis(EditorMarker *mm) {
    EditorMarker *eb, *ee;
    char *ss;
    bool res = false;

    ss = refactoryGetIdentifierOnMarker_st(mm);
    if (strcmp(ss,"this")==0) {
        refactoryMakeSyntaxPassOnSource(mm);
        if (s_spp[SPP_CAST_LPAR_POSITION].file != noFileIndex) {
            assert(s_spp[SPP_CAST_RPAR_POSITION].file != noFileIndex);
            assert(s_spp[SPP_CAST_EXPRESSION_BEGIN_POSITION].file != noFileIndex);
            assert(s_spp[SPP_CAST_EXPRESSION_END_POSITION].file != noFileIndex);
            eb = editorCreateNewMarkerForPosition(&s_spp[SPP_CAST_EXPRESSION_BEGIN_POSITION]);
            ee = editorCreateNewMarkerForPosition(&s_spp[SPP_CAST_EXPRESSION_END_POSITION]);
            if (ee->offset - eb->offset == 4 /*strlen("this")*/) {
                res = true;
            }
            editorFreeMarker(eb);
            editorFreeMarker(ee);
        }
    }
    return res;
}

static void refactoryReduceRedundantCastedThissInMethod(EditorMarker *point, EditorRegionList **methodreg) {
    char superFqtName[MAX_FILE_NAME_SIZE];
    char *ss;

    refactoryGetNameOfTheClassAndSuperClass(point, NULL, superFqtName);
    refactoryEditServerParseBuffer(refactoringOptions.project, point->buffer, point,NULL, "-olcxmaybethis",NULL);
    olcxPushSpecial(LINK_NAME_MAYBE_THIS_ITEM, OLO_MAYBE_THIS);

    createMarkersForAllReferencesInRegions(sessionData.browserStack.top->menuSym, methodreg);
    for (SymbolsMenu *mm = sessionData.browserStack.top->menuSym; mm!=NULL; mm=mm->next) {
        if (mm->selected && mm->visible) {
            for (EditorMarkerList *ll=mm->markers; ll!=NULL; ll=ll->next) {
                // casted expression "((cast)this) -> this"
                // casted expression "((cast)this) -> super"
                ss = refactoryGetIdentifierOnMarker_st(ll->marker);
                if (strcmp(ss,"this")==0) {
                    refactoryReduceCastedThis(ll->marker, superFqtName);
                    refactoryRemoveRedundantParenthesesAroundThisOrSuper(ll->marker, "this");
                    //&refactoryRemoveRedundantParenthesesAroundThisOrSuper(ll->marker, "super");
                }
            }
        }
    }
}

static void refactoryExpandThissToCastedThisInTheMethod(EditorMarker *point,
                                                        char *thiscFqtName, char *supercFqtName,
                                                        EditorRegionList *methodreg
) {
    char thisCast[MAX_FILE_NAME_SIZE];
    char superCast[MAX_FILE_NAME_SIZE];

    sprintf(thisCast, "((%s)this)", thiscFqtName);
    sprintf(superCast, "((%s)this)", supercFqtName);

    refactoryEditServerParseBuffer(refactoringOptions.project, point->buffer, point,NULL, "-olcxmaybethis",NULL);
    olcxPushSpecial(LINK_NAME_MAYBE_THIS_ITEM, OLO_MAYBE_THIS);
    createMarkersForAllReferencesInRegions(sessionData.browserStack.top->menuSym, &methodreg);
    for (SymbolsMenu *mm = sessionData.browserStack.top->menuSym; mm!=NULL; mm=mm->next) {
        if (mm->selected && mm->visible) {
            for (EditorMarkerList *ll=mm->markers; ll!=NULL; ll=ll->next) {
                char *ss = refactoryGetIdentifierOnMarker_st(ll->marker);
                // add casts only if there is yet this or super
                if (strcmp(ss,"this")==0) {
                    // check whether there is yet a casted this
                    if (! refactoryIsThereACastOfThis(ll->marker)) {
                        refactoryCheckedReplaceString(ll->marker, 4, "this", "");
                        refactoryReplaceString(ll->marker, 0, thisCast);
                    }
                } else if (strcmp(ss,"super")==0) {
                    refactoryCheckedReplaceString(ll->marker, 5, "super", "");
                    refactoryReplaceString(ll->marker, 0, superCast);
                }
            }
        }
    }
}

static void refactoryPushDownPullUp(EditorMarker *point, PushPullDirection direction, int limitIndex) {
    char sourceFqtName[MAX_FILE_NAME_SIZE];
    char superFqtName[MAX_FILE_NAME_SIZE];
    char targetFqtName[MAX_FILE_NAME_SIZE];
    EditorMarker *target, *movedStart, *mend, *movedEnd, *startMarker, *endMarker;
    EditorRegionList *methodreg;
    SymbolsMenu *mm1, *mm2;
    ReferencesItem *theMethod;
    int size;
    int lines;
    UNUSED lines;

    target = getTargetFromOptions();
    if (! validTargetPlace(target, "-olcxmmtarget")) return;

    refactoryUpdateReferences(refactoringOptions.project);

    refactoryGetNameOfTheClassAndSuperClass(point, sourceFqtName, superFqtName);
    refactoryGetNameOfTheClassAndSuperClass(target, targetFqtName, NULL);
    refactoryGetMethodLimitsForMoving(point, &movedStart, &mend, limitIndex);
    lines = editorCountLinesBetweenMarkers(movedStart, mend);

    // prechecks
    refactorySetMovingPrecheckStandardEnvironment(point, targetFqtName);
    if (limitIndex == SPP_METHOD_DECLARATION_BEGIN_POSITION) {
        // method
        if (direction == PULLING_UP) {
            if (! (tpCheckTargetToBeDirectSubOrSuperClass(REQ_SUPERCLASS, "superclass")
                   && tpCheckSuperMethodReferencesForPullUp()
                   && tpCheckMethodReferencesWithApplOnSuperClassForPullUp())) {
                fatalError(ERR_INTERNAL, "A trivial precondition failed", XREF_EXIT_ERR);
            }
        } else {
            if (! (tpCheckTargetToBeDirectSubOrSuperClass(REQ_SUBCLASS, "subclass"))) {
                fatalError(ERR_INTERNAL, "A trivial precondition failed", XREF_EXIT_ERR);
            }
        }
    } else {
        // field
        if (direction == PULLING_UP) {
            if (! (tpCheckTargetToBeDirectSubOrSuperClass(REQ_SUPERCLASS, "superclass")
                   && tpPullUpFieldLastPreconditions())) {
                fatalError(ERR_INTERNAL, "A trivial precondition failed", XREF_EXIT_ERR);
            }
        } else {
            if (! (tpCheckTargetToBeDirectSubOrSuperClass(REQ_SUBCLASS, "subclass")
                   && tpPushDownFieldLastPreconditions())) {
                fatalError(ERR_INTERNAL, "A trivial precondition failed", XREF_EXIT_ERR);
            }
        }
    }

    methodreg = newEditorRegionList(movedStart, mend, NULL);

    refactoryExpandThissToCastedThisInTheMethod(point, sourceFqtName, superFqtName, methodreg);

    movedEnd = editorDuplicateMarker(mend);
    movedEnd->offset --;

    // perform moving
    refactoryApplyExpandShortNames(point->buffer, point);
    size = mend->offset - movedStart->offset;
    refactoryPushAllReferencesOfMethod(point, "-olallchecks");
    createMarkersForAllReferencesInRegions(sessionData.browserStack.top->menuSym, NULL);
    assert(sessionData.browserStack.top!=NULL && sessionData.browserStack.top->hkSelectedSym!=NULL);
    theMethod = &sessionData.browserStack.top->hkSelectedSym->s;
    editorMoveBlock(target, movedStart, size, &editorUndo);

    // recompute methodregion, maybe free old methodreg before!!
    startMarker = editorDuplicateMarker(movedStart);
    endMarker = editorDuplicateMarker(movedEnd);
    endMarker->offset++;

    methodreg = newEditorRegionList(startMarker, endMarker, NULL);

    // checks correspondance
    refactoryPushAllReferencesOfMethod(point, "-olallchecks");
    createMarkersForAllReferencesInRegions(sessionData.browserStack.top->menuSym, NULL);
    assert(sessionData.browserStack.top && sessionData.browserStack.top->previous);
    mm1 = sessionData.browserStack.top->previous->menuSym;
    mm2 = sessionData.browserStack.top->menuSym;

    refactoryPullUpPushDownCheckCorrespondance(mm1, mm2, theMethod);
    // push down super.method() check
    if (limitIndex == SPP_METHOD_DECLARATION_BEGIN_POSITION) {
        if (direction == PUSHING_DOWN) {
            refactorySetMovingPrecheckStandardEnvironment(point, targetFqtName);
            if (! tpCheckSuperMethodReferencesAfterPushDown()) {
                fatalError(ERR_INTERNAL, "A trivial precondition failed", XREF_EXIT_ERR);
            }
        }
    }

    // O.K. now repass maybethis and reduce casts on this
    refactoryReduceRedundantCastedThissInMethod(point, &methodreg);

    // reduce long names in the method
    refactoryPerformReduceNamesAndAddImports(&methodreg, INTERACTIVE_NO);

    // and generate output
    refactoryApplyWholeRefactoringFromUndo();
}


static void refactoryPullUpField(EditorMarker *point) {
    refactoryPushDownPullUp(point, PULLING_UP , SPP_FIELD_DECLARATION_BEGIN_POSITION);
}

static void refactoryPullUpMethod(EditorMarker *point) {
    refactoryPushDownPullUp(point, PULLING_UP , SPP_METHOD_DECLARATION_BEGIN_POSITION);
}

static void refactoryPushDownField(EditorMarker *point) {
    refactoryPushDownPullUp(point, PUSHING_DOWN , SPP_FIELD_DECLARATION_BEGIN_POSITION);
}

static void refactoryPushDownMethod(EditorMarker *point) {
    refactoryPushDownPullUp(point, PUSHING_DOWN , SPP_METHOD_DECLARATION_BEGIN_POSITION);
}

// --------------------------------------------------------------------


static char * refactoryComputeUpdateOptionForSymbol(EditorMarker *point) {
    EditorMarkerList  *occs;
    SymbolsMenu     *csym;
    int                 hasHeaderReferenceFlag, scope, cat, multiFileRefsFlag, fn;
    int                 symtype, storage, accflags;
    char                *res;

    assert(point!=NULL && point->buffer!=NULL);
    mainSetLanguage(point->buffer->name, &s_language);

    hasHeaderReferenceFlag = 0;
    multiFileRefsFlag = 0;
    occs = refactoryGetReferences(point->buffer, point, NULL, PPCV_BROWSER_TYPE_WARNING);
    csym =  sessionData.browserStack.top->hkSelectedSym;
    scope = csym->s.scope;
    cat = csym->s.category;
    symtype = csym->s.type;
    storage = csym->s.storage;
    accflags = csym->s.access;
    if (occs == NULL) {
        fn = noFileIndex;
    } else {
        assert(occs->marker!=NULL && occs->marker->buffer!=NULL);
        fn = occs->marker->buffer->fileIndex;
    }
    for (EditorMarkerList *o = occs; o!=NULL; o=o->next) {
        assert(o->marker!=NULL && o->marker->buffer!=NULL);
        FileItem *fileItem = getFileItem(o->marker->buffer->fileIndex);
        if (fn != o->marker->buffer->fileIndex) {
            multiFileRefsFlag = 1;
        }
        if (!fileItem->commandLineEntered) {
            hasHeaderReferenceFlag = 1;
        }
    }

    if (LANGUAGE(LANG_JAVA)) {
        if (cat == CategoryLocal) {
            // useless to update when there is nothing about the symbol in Tags
            res = "";
        } else if (symtype==TypeDefault
                   && (storage == StorageMethod || storage == StorageField)
                   && ((accflags & AccessPrivate) != 0)
                   ) {
            // private field or method,
            // no update makes renaming after extract method much faster
            res = "";
        } else {
            res = "-fastupdate";
        }
    } else {
        if (cat == CategoryLocal) {
            // useless to update when there is nothing about the symbol in Tags
            res = "";
        } else if (hasHeaderReferenceFlag) {
            // once it is in a header, full update is required
            res = "-update";
        } else if (scope==ScopeAuto || scope==ScopeFile) {
            // for example a local var or a static function not used in any header
            if (multiFileRefsFlag) {
                errorMessage(ERR_INTERNAL, "something goes wrong, a local symbol is used in several files");
                res = "-update";
            } else {
                res = "";
            }
        } else if (! multiFileRefsFlag) {
            // this is a little bit tricky. It may provoke a bug when
            // a new external function is not yet indexed, but used in another file.
            // But it is so practical, so take the risk.
            res = "";
        } else {
            // may seems too strong, but implicitly linked global functions
            // requires this (for example).
            res = "-fastupdate";
        }
    }

    editorFreeMarkersAndMarkerList(occs);
    occs = NULL;
    olcxPopOnly();

    return res;
}

// --------------------------------------------------------------------


void mainRefactory() {
    int fArgCount;
    char *file, *argumentFile;
    char inputFileName[MAX_FILE_NAME_SIZE];
    EditorBuffer *buf;
    EditorMarker *point, *mark;

    ENTER();

    copyOptions(&refactoringOptions, &options);       // save command line options !!!!
    // in general in this file:
    //   'refactoringOptions' are options passed to c-xrefactory
    //   'options' are options valid for interactive edit-server 'sub-task'
    copyOptions(&savedOptions, &options);

    // MAGIC, set the server operation to anything that just refreshes
    // or generates xrefs since we will be calling the "main task"
    // below
    refactoringOptions.serverOperation = OLO_LIST;

    mainOpenOutputFile(refactoringOptions.outputFileName);
    editorLoadAllOpenedBufferFiles();
    // initialise lastQuasySaveTime
    editorQuasiSaveModifiedBuffers();

    if (refactoringOptions.project==NULL) {
        fatalError(ERR_ST, "You have to specify active project with -p option",
                   XREF_EXIT_ERR);
    }

    fArgCount = 0;
    argumentFile = getNextInputFile(&fArgCount);
    if (argumentFile==NULL) {
        file = NULL;
    } else {
        strcpy(inputFileName, argumentFile);
        file = inputFileName;
    }

    buf = NULL;
    if (file==NULL)
        fatalError(ERR_ST, "no input file", XREF_EXIT_ERR);

    buf = editorFindFile(file);

    point = refactoryGetPointFromRefactoryOptions(buf);
    mark = refactoryGetMarkFromRefactoryOptions(buf);

    refactoringStartingPoint = editorUndo;

    // init subtask
    mainTaskEntryInitialisations(argument_count(refactoryEditServInitOptions),
                                 refactoryEditServInitOptions);
    refactoryXrefEditServerSubTaskFirstPass = true;

    progressFactor = 1;

    switch (refactoringOptions.theRefactoring) {
    case AVR_RENAME_SYMBOL:
    case AVR_RENAME_CLASS:
    case AVR_RENAME_PACKAGE:
        progressFactor = 3;
        refactoryUpdateOption = refactoryComputeUpdateOptionForSymbol(point);
        refactoryRename(buf, point);
        break;
    case AVR_EXPAND_NAMES:
        progressFactor = 1;
        refactoryExpandShortNames(buf, point);
        break;
    case AVR_REDUCE_NAMES:
        progressFactor = 1;
        refactoryReduceLongNamesInTheFile(buf, point);
        break;
    case AVR_ADD_ALL_POSSIBLE_IMPORTS:
        progressFactor = 2;
        refactoryReduceLongNamesInTheFile(buf, point);
        break;
    case AVR_ADD_TO_IMPORT:
        progressFactor = 2;
        refactoryAddToImports(point);
        break;
    case AVR_ADD_PARAMETER:
    case AVR_DEL_PARAMETER:
    case AVR_MOVE_PARAMETER:
        progressFactor = 3;
        refactoryUpdateOption = refactoryComputeUpdateOptionForSymbol(point);
        mainSetLanguage(file, &s_language);
        if (LANGUAGE(LANG_JAVA)) progressFactor++;
        refactoryParameterManipulation(buf, point, refactoringOptions.theRefactoring,
                                       refactoringOptions.olcxGotoVal, refactoringOptions.parnum2);
        break;
    case AVR_MOVE_FIELD:
        progressFactor = 6;
        refactoryMoveField(point);
        break;
    case AVR_MOVE_STATIC_FIELD:
        progressFactor = 4;
        refactoryMoveStaticField(point);
        break;
    case AVR_MOVE_STATIC_METHOD:
        progressFactor = 4;
        refactoryMoveStaticMethod(point);
        break;
    case AVR_MOVE_CLASS:
        progressFactor = 3;
        refactoryMoveClass(point);
        break;
    case AVR_MOVE_CLASS_TO_NEW_FILE:
        progressFactor = 3;
        refactoryMoveClassToNewFile(point);
        break;
    case AVR_MOVE_ALL_CLASSES_TO_NEW_FILE:
        progressFactor = 3;
        refactoryMoveAllClassesToNewFile(point);
        break;
    case AVR_PULL_UP_METHOD:
        progressFactor = 2;
        refactoryPullUpMethod(point);
        break;
    case AVR_PULL_UP_FIELD:
        progressFactor = 2;
        refactoryPullUpField(point);
        break;
    case AVR_PUSH_DOWN_METHOD:
        progressFactor = 2;
        refactoryPushDownMethod(point);
        break;
    case AVR_PUSH_DOWN_FIELD:
        progressFactor = 2;
        refactoryPushDownField(point);
        break;
    case AVR_TURN_STATIC_METHOD_TO_DYNAMIC:
        progressFactor = 6;
        refactoryTurnStaticToDynamic(point);
        break;
    case AVR_TURN_DYNAMIC_METHOD_TO_STATIC:
        progressFactor = 4;
        refactorVirtualToStatic(point);
        break;
    case AVR_EXTRACT_METHOD:
        progressFactor = 1;
        refactoryExtractMethod(point, mark);
        break;
    case AVR_EXTRACT_MACRO:
        progressFactor = 1;
        refactoryExtractMacro(point, mark);
        break;
    case AVR_SELF_ENCAPSULATE_FIELD:
        progressFactor = 3;
        refactorySelfEncapsulateField(point);
        break;
    case AVR_ENCAPSULATE_FIELD:
        progressFactor = 3;
        refactoryEncapsulateField(point);
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
        sprintf(tmpBuff, "s_progressOffset (%d) != s_progressFactor (%d)", progressOffset, progressFactor);
        ppcGenRecord(PPC_DEBUG_INFORMATION, tmpBuff);
    }

    // synchronisation, wait so files won't be saved with the same time
    editorQuasiSaveModifiedBuffers();

    closeMainOutputFile();
    ppcSynchronize();

    // exiting, put undefined, so that main will finish
    options.taskRegime = RegimeUndefined;

    LEAVE();
}
