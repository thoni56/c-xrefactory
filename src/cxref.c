#include "cxref.h"

#include "caching.h"
#include "visibility.h"
#include "characterreader.h"
#include "classhierarchy.h"
#include "commons.h"
#include "complete.h"
#include "cxfile.h"
#include "filedescriptor.h"
#include "filetable.h"
#include "globals.h"
#include "list.h"
#include "log.h"
#include "menu.h"
#include "misc.h"
#include "options.h"
#include "ppc.h"
#include "proto.h"
#include "protocol.h"
#include "refactorings.h"
#include "reftab.h"
#include "scope.h"
#include "server.h"
#include "session.h"
#include "storage.h"
#include "symbol.h"
#include "type.h"
#include "usage.h"


/* *********************************************************************** */

int olcxReferenceInternalLessFunction(Reference *r1, Reference *r2) {
    return SORTED_LIST_LESS(r1, (*r2));
}

static void renameCollationSymbols(SymbolsMenu *menu) {
    assert(menu);
    for (SymbolsMenu *m=menu; m!=NULL; m=m->next) {
        char *cs = strchr(m->references.linkName, LINK_NAME_COLLATE_SYMBOL);
        if (cs!=NULL && m->references.type==TypeCppCollate) {
            char *newName;
            int len = strlen(m->references.linkName);
            assert(len>=2);
            newName = malloc(len-1);
            int len1 = cs-m->references.linkName;
            strncpy(newName, m->references.linkName, len1);
            strcpy(newName+len1, cs+2);
            log_debug("renaming %s to %s", m->references.linkName, newName);
            free(m->references.linkName);
            m->references.linkName = newName;
        }
    }
}


/* *********************************************************************** */

static Reference *getDefinitionRef(Reference *reference) {
    Reference *definitionReference = NULL;

    for (Reference *r=reference; r!=NULL; r=r->next) {
        if (r->usage==UsageDefined || r->usage==UsageOLBestFitDefined) {
            definitionReference = r;
        }
        if (definitionReference==NULL && r->usage==UsageDeclared)
            definitionReference = r;
    }
    return definitionReference;
}

static void setAvailableRefactorings(Symbol *symbol) {
    switch (symbol->type) {
    case TypeStruct:
        makeRefactoringAvailable(PPC_AVR_RENAME_SYMBOL, "");
        break;
    case TypeMacroArg:
        makeRefactoringAvailable(PPC_AVR_RENAME_SYMBOL, "");
        break;
    case TypeLabel:
        makeRefactoringAvailable(PPC_AVR_RENAME_SYMBOL, "");
        break;
    case TypeMacro:
        makeRefactoringAvailable(PPC_AVR_RENAME_SYMBOL, "");
        makeRefactoringAvailable(PPC_AVR_ADD_PARAMETER, "macro");
        makeRefactoringAvailable(PPC_AVR_DEL_PARAMETER, "macro");
        makeRefactoringAvailable(PPC_AVR_MOVE_PARAMETER, "macro");
        break;
    case TypeCppInclude:
        break;
    case TypeChar:
    case TypeUnsignedChar:
    case TypeSignedChar:
    case TypeInt:
    case TypeUnsignedInt:
    case TypeSignedInt:
    case TypeShortInt:
    case TypeShortUnsignedInt:
    case TypeShortSignedInt:
    case TypeLongInt:
    case TypeLongUnsignedInt:
    case TypeLongSignedInt:
    case TypeFloat:
    case TypeDouble:
    case TypeUnion:
    case TypeEnum:
    case TypeVoid:
    case TypePointer:
    case TypeArray:
    case TypeAnonymousField:
    case TypeError:
    case TypeElipsis:
    case TypeByte:
    case TypeShort:
    case TypeLong:
    case TypeBoolean:
    case TypeNull:
    case TypeCppIfElse:
    case TypeCppCollate:
    case TypeYaccSymbol:
    case TypeKeyword:
    case TypeToken:
    case TypeUndefMacro:
    case TypeDefinedOp:
    case TypeBlockMarker:
    case TypeExpression:
    case TypePackedType:
    case TypeSpecialComplete:
    case TypeNonImportedClass:
    case TypeInducedError:
    case TypeInheritedFullMethod:
    case TypeFunction:
    case TypeSpecialConstructorCompletion:
    case TypeUnknown:
    case TypeDefault:
        makeRefactoringAvailable(PPC_AVR_RENAME_SYMBOL, "");

        if (symbol->u.typeModifier->type == TypeFunction || symbol->u.typeModifier->type == TypeMacro) {
            makeRefactoringAvailable(PPC_AVR_ADD_PARAMETER, "");
            makeRefactoringAvailable(PPC_AVR_DEL_PARAMETER, "");
            makeRefactoringAvailable(PPC_AVR_MOVE_PARAMETER, "");
            makeRefactoringAvailable(PPC_AVR_MOVE_FUNCTION, "");
        }
        break;
    case MODIFIERS_START:
    case TmodLong:
    case TmodShort:
    case TmodSigned:
    case TmodUnsigned:
    case TmodShortSigned:
    case TmodShortUnsigned:
    case TmodLongSigned:
    case TmodLongUnsigned:
    case TYPE_MODIFIERS_END:
    case MAX_CTYPE:
    case MAX_TYPE:
        FATAL_ERROR(ERR_INTERNAL, "unexpected case for symbol type in setAvailableRefactoringsInMenu()", XREF_EXIT_ERR);
    }
}

static void olGetAvailableRefactorings(void) {
    int count;

    assert(options.xref2);

    count = availableRefactoringsCount();

    if (count==0)
        makeRefactoringAvailable(PPC_AVR_SET_MOVE_TARGET, "");

    makeRefactoringAvailable(PPC_AVR_UNDO, "");

    if (options.olMarkOffset != -1 && options.olCursorOffset != options.olMarkOffset) {
        // region selected, TODO!!! some more prechecks for extract - Duh! Which ones!?!?!?!
        makeRefactoringAvailable(PPC_AVR_EXTRACT_VARIABLE, "");
        makeRefactoringAvailable(PPC_AVR_EXTRACT_FUNCTION, "");
        makeRefactoringAvailable(PPC_AVR_EXTRACT_MACRO, "");
    }
    ppcBegin(PPC_AVAILABLE_REFACTORINGS);
    for (int i=0; i<AVR_MAX_AVAILABLE_REFACTORINGS; i++) {
        if (isRefactoringAvailable(i)) {
            ppcValueRecord(PPC_INT_VALUE, i, availableRefactoringOptionFor(i));
        }
    }
    ppcEnd(PPC_AVAILABLE_REFACTORINGS);
}


static bool olcxOnlyParseNoPushing(int opt) {
    return opt==OLO_GLOBAL_UNUSED || opt==OLO_LOCAL_UNUSED;
}


/* ********************************************************************* */
Reference *addNewCxReference(Symbol *symbol, Position *position, Usage usage,
                             int vApplCl) {
    Visibility        visibility;
    Scope             scope;
    Storage           storage;
    Usage             defaultUsage;
    Reference       **place;
    Position         *defaultPosition;
    SymbolsMenu      *menu;

    // do not record references during prescanning
    // this is because of cxMem overflow during prescanning (for ex. with -html)
    // TODO: So is this relevant now that HTML is gone?
    if (symbol->linkName == NULL)
        return NULL;
    if (*symbol->linkName == 0)
        return NULL;
    if (symbol == &errorSymbol || symbol->type==TypeError)
        return NULL;
    if (position->file == NO_FILE_NUMBER)
        return NULL;

    assert(position->file<MAX_FILES);

    FileItem *fileItem = getFileItem(position->file);

    getSymbolCxrefProperties(symbol, &visibility, &scope, &storage);

    log_trace("adding reference on %s(%d) at %d,%d,%d (%s) (%s) (%s)",
              symbol->linkName, vApplCl, position->file, position->line,
              position->col, visibility==GlobalVisibility?"Global":"Local",
              usageKindEnumName[usage], storageEnumName[symbol->storage]);

    assert(options.mode);
    switch (options.mode) {
    case ServerMode:
        if (options.serverOperation == OLO_EXTRACT) {
            if (inputFileNumber != currentFile.characterBuffer.fileNumber)
                return NULL;
        } else {
            if (visibility==GlobalVisibility && symbol->type!=TypeCppInclude && options.serverOperation!=OLO_TAG_SEARCH) {
                // do not load references if not the currently edited file
                if (olOriginalFileNumber != position->file && options.noIncludeRefs)
                    return NULL;
                // do not load references if current file is an
                // included header, they will be reloaded from ref file
                //&fprintf(dumpOut,"%s comm %d\n", fileItem->name, fileItem->isArgument);
            }
        }
        break;
    case XrefMode:
        if (visibility == LocalVisibility)
            return NULL; /* dont cxref local symbols */
        if (!fileItem->cxLoading)
            return NULL;
        break;
    default:
        assert(0);              /* Should not happen */
        break;
    }

    ReferenceItem  referenceItem;
    ReferenceItem *foundMember;

    fillReferenceItem(&referenceItem, symbol->linkName, vApplCl, symbol->type, storage, scope, visibility);
    if (options.mode==ServerMode && options.serverOperation==OLO_TAG_SEARCH && options.searchKind==SEARCH_FULL) {
        Reference reference = makeReference(*position, UsageNone, NULL);
        searchSymbolCheckReference(&referenceItem, &reference);
        return NULL;
    }

    int index;
    if (!isMemberInReferenceTable(&referenceItem, &index, &foundMember)) {
        log_trace("allocating '%s'", symbol->linkName);
        char *linkName = cxAlloc(strlen(symbol->linkName)+1);
        strcpy(linkName, symbol->linkName);
        ReferenceItem *r = cxAlloc(sizeof(ReferenceItem));
        fillReferenceItem(r, linkName, vApplCl, symbol->type,
                           storage, scope, visibility);
        pushReferenceItem(r, index);
        foundMember = r;
    }

    place = addToReferenceList(&foundMember->references, *position, usage);
    log_trace("checking %s(%d),%d,%d <-> %s(%d),%d,%d == %d(%d), usage == %d, %s",
              getFileItem(cxRefPosition.file)->name, cxRefPosition.file, cxRefPosition.line, cxRefPosition.col,
              fileItem->name, position->file, position->line, position->col,
              memcmp(&cxRefPosition, position, sizeof(Position)), positionsAreEqual(cxRefPosition, *position),
              usage, symbol->linkName);

    if (options.mode == ServerMode
        && positionsAreEqual(cxRefPosition, *position)
        && usage<UsageMaxOLUsages
    ) {
        if (symbol->linkName[0] == ' ') {  // special symbols for internal use!
            if (strcmp(symbol->linkName, LINK_NAME_UNIMPORTED_QUALIFIED_ITEM)==0) {
                if (options.serverOperation == OLO_GET_AVAILABLE_REFACTORINGS) {
                    setAvailableRefactorings(symbol);
                }
            }
        } else {
            /* an on - line cxref action ?*/
            //&fprintf(dumpOut,"!got it %s !!!!!!!\n", foundMember->linkName);
            olstringServed = true;       /* olstring will be served */
            olstringUsage = usage;
            assert(sessionData.browserStack.top);
            olSetCallerPosition(position);
            defaultPosition = &noPosition;
            defaultUsage = UsageNone;
            if (symbol->type==TypeMacro && ! options.exactPositionResolve) {
                // a hack for macros
                defaultPosition = &symbol->pos;
                defaultUsage = UsageDefined;
            }
            if (defaultPosition->file!=NO_FILE_NUMBER)
                log_trace("getting definition position of %s at line %d", symbol->name, defaultPosition->line);
            if (! olcxOnlyParseNoPushing(options.serverOperation)) {
                menu = olAddBrowsedSymbolToMenu(&sessionData.browserStack.top->hkSelectedSym, foundMember,
                                                true, true, 0, usage, 0, defaultPosition, defaultUsage);
                // hack added for EncapsulateField
                // to determine whether there is already definitions of getter/setter
                if (isDefinitionUsage(usage)) {
                    menu->defpos = *position;
                    menu->defUsage = usage;
                }
                if (options.serverOperation == OLO_GET_AVAILABLE_REFACTORINGS) {
                    setAvailableRefactorings(symbol);
                }
            }
        }
    }

    /* Test for available space */
    assert(options.mode);
    if (options.mode==XrefMode) {
        if (!dm_enoughSpaceFor(cxMemory, CX_SPACE_RESERVE)) {
            longjmp(cxmemOverflow, LONGJUMP_REASON_REFERENCE_OVERFLOW);
        }
    }

    assert(place);
    log_trace("returning %x == %s %s:%d", *place, usageKindEnumName[(*place)->usage],
              getFileItem((*place)->position.file)->name, (*place)->position.line);
    return *place;
}

Reference *addCxReference(Symbol *symbol, Position *position, Usage usage,
                          int vApplClass) {
    return addNewCxReference(symbol, position, usage, vApplClass);
}

void addTrivialCxReference(char *name, int symType, int storage, Position position, Usage usage) {
    Symbol symbol = makeSymbol(name, name, position);
    symbol.type = symType;
    symbol.storage = storage;
    addCxReference(&symbol, &position, usage, NO_FILE_NUMBER);
}


/* ***************************************************************** */

static void deleteOlcxRefs(OlcxReferences **refsP, OlcxReferencesStack *stack) {
    OlcxReferences    *refs = *refsP;

    freeReferences(refs->references);
    freeCompletions(refs->completions);
    freeSymbolsMenuList(refs->hkSelectedSym);
    freeSymbolsMenuList(refs->menuSym);

    // if deleting second entry point, update it
    if (refs==stack->top) {
        stack->top = refs->previous;
    }
    // this is useless, but one never knows
    if (refs==stack->root) {
        stack->root = refs->previous;
    }
    *refsP = refs->previous;
    free(refs);
}


void olcxFreeOldCompletionItems(OlcxReferencesStack *stack) {
    OlcxReferences **references;

    references = &stack->top;
    if (*references == NULL)
        return;
    for (int i=1; i<MAX_COMPLETIONS_HISTORY_DEEP; i++) {
        references = &(*references)->previous;
        if (*references == NULL)
            return;
    }
    deleteOlcxRefs(references, stack);
}


static void freePoppedBrowserStackItems(OlcxReferencesStack *stack) {
    assert(stack);
    // delete all after top
    while (stack->root != stack->top) {
        //&fprintf(dumpOut,":freeing %s\n", stack->root->hkSelectedSym->references.linkName);
        deleteOlcxRefs(&stack->root, stack);
    }
}

static OlcxReferences *pushEmptyReference(OlcxReferencesStack *stack) {
    OlcxReferences *res;

    res  = malloc(sizeof(OlcxReferences));
    *res = (OlcxReferences){.references      = NULL,
                            .actual          = NULL,
                            .command         = options.serverOperation,
                            .accessTime      = fileProcessingStartTime,
                            .callerPosition  = noPosition,
                            .completions     = NULL,
                            .hkSelectedSym   = NULL,
                            .menuFilterLevel = DEFAULT_MENU_FILTER_LEVEL,
                            .refsFilterLevel = DEFAULT_REFS_FILTER_LEVEL,
                            .previous        = stack->top};
    return res;
}

void pushEmptySession(OlcxReferencesStack *stack) {
    OlcxReferences *res;
    freePoppedBrowserStackItems(stack);
    res = pushEmptyReference(stack);
    stack->top = stack->root = res;
}

static Reference *olcxCopyReference(Reference *reference) {
    Reference *r;
    r = malloc(sizeof(Reference));
    *r = *reference;
    r->next = NULL;
    return r;
}

static void olcxAppendReference(Reference *ref, OlcxReferences *refs) {
    Reference *rr;
    rr = olcxCopyReference(ref);
    LIST_APPEND(Reference, refs->references, rr);
    log_trace("olcx appending %s %s:%d:%d", usageKindEnumName[ref->usage],
              getFileItem(ref->position.file)->name, ref->position.line, ref->position.col);
}

/* fnum is file number of which references are added, can be ANY_FILE */
void olcxAddReferences(Reference *list, Reference **dlist,
                       int fnum, int bestMatchFlag) {
    Reference *revlist,*tmp;
    /* from now, you should add it to macros as REVERSE_LIST_MAP() */
    revlist = NULL;
    while (list!=NULL) {
        tmp = list->next; list->next = revlist;
        revlist = list;   list = tmp;
    }
    list = revlist;
    revlist = NULL;
    while (list!=NULL) {
        if (fnum==ANY_FILE || fnum==list->position.file) {
            olcxAddReference(dlist, list);
        }
        tmp = list->next; list->next = revlist;
        revlist = list;   list = tmp;
    }
    list = revlist;
}

static void olcxAddReferencesToSymbolsMenu(SymbolsMenu *menu, Reference *references, int bestFitFlag) {
    for (Reference *rr = references; rr != NULL; rr = rr->next) {
        olcxAddReferenceToSymbolsMenu(menu, rr);
    }
}

static int characterCodeForUsage(Usage usage) {
    switch (usage) {
    case UsageOLBestFitDefined: return '!';
    case UsageDefined:  return '*';
    case UsageDeclared: return '+';
    case UsageLvalUsed: return ',';
    case UsageAddrUsed: return '.';
        /* some specials for refactorings now */
    case UsageNotFQFieldInClassOrMethod: return '.';
        // Usage Constructor definition is for move class to not expand
        // and move constructor definition references
    case UsageConstructorDefinition: return '-';
    default: return ' ';
    }
    assert(0);
}

void gotoOnlineCxref(Position *pos, Usage usage, char *suffix)
{
    if (options.xref2) {
        ppcGotoPosition(pos);
    } else {
        fprintf(communicationChannel,"%s%s#*+*#%d %d :%c%s ;;\n", COLCX_GOTO_REFERENCE,
                getRealFileName_static(getFileItem(pos->file)->name),
                pos->line, pos->col, characterCodeForUsage(usage), suffix);
    }
}

static bool sessionHasReferencesValidForOperation(SessionData *session, OlcxReferences **refs, CheckNull checkNull) {
    assert(session);
    if (options.serverOperation==OLO_COMPLETION || options.serverOperation==OLO_CSELECT
        ||  options.serverOperation==OLO_CGOTO || options.serverOperation==OLO_TAG_SEARCH) {
        *refs = session->completionsStack.top;
    } else {
        *refs = session->browserStack.top;
    }
    if (checkNull==CHECK_NULL && *refs == NULL) {
        if (options.xref2) {
            ppcBottomWarning("Empty stack");
        } else {
            fprintf(communicationChannel, "=");
        }
        return false;
    }
    return true;
}


static void olcxRenameInit(void) {
    OlcxReferences *refs;

    if (!sessionHasReferencesValidForOperation(&sessionData, &refs, CHECK_NULL))
        return;
    refs->actual = refs->references;
    gotoOnlineCxref(&refs->actual->position, refs->actual->usage, "");
}


static bool referenceIsLessThan(Reference *r1, Reference *r2) {
    int fc;
    char *s1,*s2;
    s1 = simpleFileName(getFileItem(r1->position.file)->name);
    s2 = simpleFileName(getFileItem(r2->position.file)->name);
    fc=strcmp(s1, s2);
    if (fc<0) return true;
    if (fc>0) return false;
    if (r1->position.file < r2->position.file) return true;
    if (r1->position.file > r2->position.file) return false;
    if (r1->position.line < r2->position.line) return true;
    if (r1->position.line > r2->position.line) return false;
    if (r1->position.col < r2->position.col) return true;
    if (r1->position.col > r2->position.col) return false;
    return false;
}


static bool usageImportantInOrder(Reference *r1, Reference *r2) {
    return r1->usage==UsageDefined
        || r1->usage==UsageDeclared
        || r1->usage==UsageOLBestFitDefined
        || r2->usage==UsageDefined
        || r2->usage==UsageDeclared
        || r2->usage==UsageOLBestFitDefined;
}


static bool referenceIsLessThanOrderImportant(Reference *r1, Reference *r2) {
    if (usageImportantInOrder(r1, r2)) {
        // in definition, declaration usage is important
        if (r1->usage < r2->usage) return true;
        if (r1->usage > r2->usage) return false;
    }
    return referenceIsLessThan(r1, r2);
}


static void olcxNaturalReorder(OlcxReferences *refs) {
    LIST_MERGE_SORT(Reference, refs->references, referenceIsLessThanOrderImportant);
}

static void indicateNoReference(void) {
    if (options.xref2) {
        ppcBottomInformation("No reference");
    } else {
        fprintf(communicationChannel, "_");
    }
}

// references has to be ordered according internal file numbers order !!!!
static void olcxSetCurrentRefsOnCaller(OlcxReferences *refs) {
    Reference *rr;
    for (rr=refs->references; rr!=NULL; rr=rr->next){
        log_trace("checking %d:%d:%d to %d:%d:%d", rr->position.file, rr->position.line,rr->position.col,
                  refs->callerPosition.file,  refs->callerPosition.line,  refs->callerPosition.col);
        if (!positionIsLessThan(rr->position, refs->callerPosition))
            break;
    }
    // it should never be NULL, but one never knows - DUH! We have coverage to show that you are wrong
    if (rr == NULL) {
        refs->actual = refs->references;
    } else {
        refs->actual = rr;
    }
}

static void orderRefsAndGotoDefinition(OlcxReferences *refs) {
    olcxNaturalReorder(refs);
    if (refs->references == NULL) {
        refs->actual = refs->references;
        indicateNoReference();
    } else if (refs->references->usage <= UsageDeclared) {
        refs->actual = refs->references;
        gotoOnlineCxref(&refs->actual->position, refs->actual->usage, "");
    } else {
        if (options.xref2) {
            ppcWarning("Definition not found");
        } else {
            fprintf(communicationChannel,"*** Definition not found **");
        }
    }
}

static void olcxOrderRefsAndGotoDefinition(void) {
    OlcxReferences *refs;

    if (!sessionHasReferencesValidForOperation(&sessionData, &refs, CHECK_NULL))
        return;
    orderRefsAndGotoDefinition(refs);
}

static void getFileChar(int *chP, Position *position, CharacterBuffer *characterBuffer) {
    if (*chP=='\n') {
        position->line++;
        position->col=0;
    } else
        position->col++;
    *chP = getChar(characterBuffer);
}


static char listLine[MAX_REF_LIST_LINE_LEN+5];
static int listLineIndex = 0;

static void passSourcePutChar(int c, FILE *file) {
    if (options.xref2) {
        if (listLineIndex < MAX_REF_LIST_LINE_LEN) {
            listLine[listLineIndex++] = c;
            listLine[listLineIndex] = 0;
        } else {
            strcpy(listLine + listLineIndex, "...");
        }
    } else {
        fputc(c,file);
    }
}


static bool listableUsage(Reference *ref, int usages, int usageFilter) {
    return usages==USAGE_ANY || usages==ref->usage || (usages==USAGE_FILTER && ref->usage<usageFilter);
}


static void linePosProcess(FILE *outFile,
                           int usages,
                           int usageFilter, // only if usages==USAGE_FILTER
                           char *fname,
                           Reference **reference,
                           Position *callerPosition,
                           Position *positionP,
                           int *chP,
                           CharacterBuffer *cxfBuf
) {
    int        ch, linerefn;
    bool       pendingRefFlag;
    Reference *rr, *r;
    char      *fn;

    rr = *reference;
    ch = *chP;
    fn = simpleFileName(getRealFileName_static(fname));
    r = NULL;
    pendingRefFlag = false;
    linerefn = 0;
    listLineIndex = 0;
    listLine[listLineIndex] = 0;
    do {
        if (listableUsage(rr, usages, usageFilter)) {
            if (r==NULL || r->usage > rr->usage)
                r = rr;
            if (pendingRefFlag) {
                if (! options.xref2) fprintf(outFile,"\n");
            }
            if (options.xref2) {
                if (! pendingRefFlag) {
                    sprintf(listLine+listLineIndex, "%s:%d:", fn, rr->position.line);
                    listLineIndex += strlen(listLine+listLineIndex);
                }
            } else {
                if (positionsAreNotEqual(*callerPosition, rr->position)) fprintf(outFile, " ");
                else fprintf(outFile, ">");
                fprintf(outFile,"%c%s:%d:", characterCodeForUsage(rr->usage),fn,
                        rr->position.line);
            }
            linerefn++;
            pendingRefFlag = true;
        }
        rr=rr->next;
    } while (rr!=NULL && ((rr->position.file == positionP->file && rr->position.line == positionP->line)
                          || (rr->usage>UsageMaxOLUsages)));
    if (r!=NULL) {
        if (! cxfBuf->isAtEOF) {
            while (ch!='\n' && (! cxfBuf->isAtEOF)) {
                passSourcePutChar(ch,outFile);
                getFileChar(&ch, positionP, cxfBuf);
            }
        }
        if (! options.xref2) passSourcePutChar('\n',outFile);
    }
    if (options.xref2 && listLineIndex!=0) {
        ppcIndent();
        fprintf(outFile, "<%s %s=%d %s=%ld>%s</%s>\n",
                PPC_SRC_LINE, PPCA_REFN, linerefn,
                PPCA_LEN, (unsigned long)strlen(listLine),
                listLine, PPC_SRC_LINE);
    }
    *reference = rr;
    *chP = ch;
}

static Reference *passNonPrintableRefsForFile(Reference *references,
                                              int wantedFileNumber,
                                              int usages, int usageFilter) {
    Reference *r;

    for (r=references; r!=NULL && r->position.file == wantedFileNumber; r=r->next) {
        if (listableUsage(r, usages, usageFilter))
            return r;
    }
    return NULL;                   /* TODO: Why return the last one? */
}

static void passRefsThroughSourceFile(Reference **inOutReferences, Position *callerPosition,
                                      FILE *outputFile, int usages, int usageFilter) {
    Reference *references;
    int ch, fileNumber;
    EditorBuffer *ebuf;
    char *cofileName;
    Position position;
    CharacterBuffer cxfBuf;

    references = *inOutReferences;
    if (references==NULL)
        goto fin;
    fileNumber = references->position.file;

    cofileName = getFileItem(fileNumber)->name;
    references = passNonPrintableRefsForFile(references, fileNumber, usages, usageFilter);
    if (references==NULL || references->position.file != fileNumber)
        goto fin;
    if (options.referenceListWithoutSource) {
        ebuf = NULL;
    } else {
        ebuf = findEditorBufferForFile(cofileName);
        if (ebuf==NULL) {
            if (options.xref2) {
                char tmpBuff[TMP_BUFF_SIZE];
                sprintf(tmpBuff, "file '%s' not accessible", cofileName);
                errorMessage(ERR_ST, tmpBuff);
            } else {
                fprintf(outputFile,"!!! file '%s' is not accessible", cofileName);
            }
        }
    }
    ch = ' ';
    if (ebuf==NULL) {
        cxfBuf.isAtEOF = true;
    } else {
        fillCharacterBuffer(&cxfBuf, ebuf->allocation.text, ebuf->allocation.text+ebuf->allocation.bufferSize, NULL, ebuf->allocation.bufferSize, NO_FILE_NUMBER, ebuf->allocation.text);
        getFileChar(&ch, &position, &cxfBuf);
    }
    position = makePosition(references->position.file, 1, 0);
    Reference *oldrr=NULL;
    while (references!=NULL && references->position.file==position.file && references->position.line>=position.line) {
        assert(oldrr!=references);
        oldrr=references;    // because it is a dangerous loop
        while ((! cxfBuf.isAtEOF) && position.line<references->position.line) {
            while (ch!='\n' && ch!=EOF)
                ch = getChar(&cxfBuf);
            getFileChar(&ch, &position, &cxfBuf);
        }
        linePosProcess(outputFile, usages, usageFilter, cofileName,
                       &references, callerPosition, &position, &ch, &cxfBuf);
    }
    //&if (cofile != NULL) closeFile(cofile);
 fin:
    *inOutReferences = references;
}

/* ******************************************************************** */

bool ooBitsGreaterOrEqual(unsigned oo1, unsigned oo2) {
    if ((oo1&OOC_PROFILE_MASK) < (oo2&OOC_PROFILE_MASK)) {
        return false;
    }
    if ((oo1&OOC_VIRTUAL_MASK) < (oo2&OOC_VIRTUAL_MASK)) {
        return false;
    }
    return true;
}

static int getCurrentRefPosition(OlcxReferences *refs) {
    Reference     *rr;
    int             rlevel;
    int             actn = 0;
    rr = NULL;
    if (refs!=NULL) {
        rlevel = refListFilters[refs->refsFilterLevel];
        for (rr=refs->references; rr!=NULL && rr!=refs->actual; rr=rr->next) {
            if (rr->usage < rlevel)
                actn++;
        }
    }
    if (rr==NULL)
        actn = 0;
    return actn;
}

static void symbolHighlighNameSprint(char *output, SymbolsMenu *ss) {
    char *bb, *cc;
    int len, llen;
    sprintfSymbolLinkName(ss, output);
    cc = strchr(output, '(');
    if (cc != NULL) *cc = 0;
    len = strlen(output);
    output[len]=0;
    bb = lastOccurenceInString(output,'.');
    if (bb!=NULL) {
        bb++;
        llen = strlen(bb);
        memmove(output, bb, llen+1);
        output[llen]=0;
    }
}

static void olcxPrintRefList(char *commandString, OlcxReferences *refs) {
    Reference *rr;
    int         actn, len;
    char        ttt[MAX_CX_SYMBOL_SIZE];

    if (options.xref2) {
        actn = getCurrentRefPosition(refs);
        if (refs!=NULL && refs->menuSym != NULL) {
            ttt[0]='\"';
            symbolHighlighNameSprint(ttt+1, refs->menuSym);
            len = strlen(ttt);
            ttt[len]='\"';
            ttt[len+1]=0;
            ppcBeginWithNumericValueAndAttribute(PPC_REFERENCE_LIST, actn,
                                                 PPCA_SYMBOL, ttt);
        } else {
            ppcBeginWithNumericValue(PPC_REFERENCE_LIST, actn);
        }
    } else {
        fprintf(communicationChannel,"%s",commandString);/* communication char */
    }
    if (refs!=NULL) {
        rr=refs->references;
        while (rr != NULL) {
            passRefsThroughSourceFile(&rr, &refs->actual->position,
                                      communicationChannel, USAGE_FILTER,
                                      refListFilters[refs->refsFilterLevel]);
        }
    }
    if (options.xref2) {
        ppcEnd(PPC_REFERENCE_LIST);
        //& if (refs!=NULL && refs->actual!=NULL) ppcGotoPosition(&refs->actual->position);
    }
    fflush(communicationChannel);
}

static void olcxReferenceList(char *commandString) {
    OlcxReferences    *refs;
    if (!sessionHasReferencesValidForOperation(&sessionData, &refs, CHECK_NULL))
        return;
    olcxPrintRefList(commandString, refs);
}

static void olcxGenGotoActReference(OlcxReferences *refs) {
    if (refs->actual != NULL) {
        gotoOnlineCxref(&refs->actual->position, refs->actual->usage, "");
    } else {
        indicateNoReference();
    }
}

static void olcxPushOnly(void) {
    OlcxReferences    *refs;
    if (!sessionHasReferencesValidForOperation(&sessionData, &refs, CHECK_NULL))
        return;
    //&LIST_MERGE_SORT(Reference, refs->references, referenceIsLessThan);
    olcxGenGotoActReference(refs);
}

static void olcxPushAndCallMacro(void) {
    OlcxReferences    *refs;
    char                symbol[MAX_CX_SYMBOL_SIZE];

    if (!sessionHasReferencesValidForOperation(&sessionData, &refs, CHECK_NULL))
        return;
    LIST_MERGE_SORT(Reference, refs->references, referenceIsLessThan);
    LIST_REVERSE(Reference, refs->references);
    assert(options.xref2);
    symbolHighlighNameSprint(symbol, refs->hkSelectedSym);
    // precheck first
    for (Reference *rr=refs->references; rr!=NULL; rr=rr->next) {
        ppcReferencePreCheck(rr, symbol);
    }
    for (Reference *rr=refs->references; rr!=NULL; rr=rr->next) {
        ppcReferencePreCheck(rr, symbol);
        ppcGenRecord(PPC_CALL_MACRO, "");
    }
    LIST_REVERSE(Reference, refs->references);
}

static void olcxReferenceGotoRef(int refn) {
    OlcxReferences    *refs;
    Reference         *rr;
    int                 i,rfilter;

    if (!sessionHasReferencesValidForOperation(&sessionData, &refs, CHECK_NULL))
        return;
    rfilter = refListFilters[refs->refsFilterLevel];
    for (rr=refs->references,i=1; rr!=NULL && (i<refn||rr->usage>=rfilter); rr=rr->next){
        if (rr->usage < rfilter) i++;
    }
    refs->actual = rr;
    olcxGenGotoActReference(refs);
}

static Completion *olCompletionNthLineRef(Completion *cpls, int refn) {
    Completion *rr, *rrr;
    int i;

    for (rr=rrr=cpls, i=1; i<=refn && rrr!=NULL; rrr=rrr->next) {
        i += rrr->lineCount;
        rr = rrr;
    }
    return rr;
}

static void popSession(void) {
    sessionData.browserStack.top = sessionData.browserStack.top->previous;
}

static void popAndFreeSession(void) {
    popSession();
    freePoppedBrowserStackItems(&sessionData.browserStack);
}

static OlcxReferences *pushSession(void) {
    OlcxReferences *oldtop;
    oldtop = sessionData.browserStack.top;
    sessionData.browserStack.top = sessionData.browserStack.root;
    pushEmptySession(&sessionData.browserStack);
    return oldtop;
}

static void popAndFreeSessionsUntil(OlcxReferences *oldtop) {
    popAndFreeSession();
    // recover old top, but what if it was freed, hmm
    while (sessionData.browserStack.top!=NULL && sessionData.browserStack.top!=oldtop) {
        popSession();
    }
}

static void findAndGotoDefinition(ReferenceItem *sym) {
    OlcxReferences *refs, *oldtop;
    SymbolsMenu menu;

    // preserve popped items from browser first
    oldtop = pushSession();
    refs = sessionData.browserStack.top;
    fillSymbolsMenu(&menu, *sym, 1, true, 0, UsageUsed, 0, UsageNone, noPosition);
    refs->menuSym = &menu;
    fullScanFor(sym->linkName);
    orderRefsAndGotoDefinition(refs);
    refs->menuSym = NULL;
    // recover stack
    popAndFreeSessionsUntil(oldtop);
}

static void olcxReferenceGotoCompletion(int refn) {
    OlcxReferences *refs;
    Completion *completion;

    assert(refn > 0);
    if (!sessionHasReferencesValidForOperation(&sessionData, &refs,CHECK_NULL))
        return;
    completion = olCompletionNthLineRef(refs->completions, refn);
    if (completion != NULL) {
        if (completion->visibility == LocalVisibility /*& || refs->command == OLO_TAG_SEARCH &*/) {
            if (completion->ref.usage != UsageClassFileDefinition
                && completion->ref.usage != UsageClassTreeDefinition
                && positionsAreNotEqual(completion->ref.position, noPosition)) {
                gotoOnlineCxref(&completion->ref.position, UsageDefined, "");
            } else {
                indicateNoReference();
            }
        } else {
            findAndGotoDefinition(&completion->sym);
        }
    } else {
        indicateNoReference();
    }
}

static void olcxReferenceGotoTagSearchItem(int refn) {
    Completion *rr;

    assert(refn > 0);
    assert(sessionData.retrieverStack.top);
    rr = olCompletionNthLineRef(sessionData.retrieverStack.top->completions, refn);
    if (rr != NULL) {
        if (rr->ref.usage != UsageClassFileDefinition
            && rr->ref.usage != UsageClassTreeDefinition
            && positionsAreNotEqual(rr->ref.position, noPosition)) {
            gotoOnlineCxref(&rr->ref.position, UsageDefined, "");
        } else {
            indicateNoReference();
        }
    } else {
        indicateNoReference();
    }
}

static void olcxSetActReferenceToFirstVisible(OlcxReferences *refs, Reference *r) {
    int                 rlevel;
    rlevel = refListFilters[refs->refsFilterLevel];
    while (r!=NULL && r->usage>=rlevel) r = r->next;
    if (r != NULL) {
        refs->actual = r;
    } else {
        if (options.xref2) {
            ppcBottomInformation("Moving to the first reference");
        }
        r = refs->references;
        while (r!=NULL && r->usage>=rlevel) r = r->next;
        refs->actual = r;
    }
}

static void olcxReferencePlus(void) {
    OlcxReferences    *refs;
    Reference         *r;
    if (!sessionHasReferencesValidForOperation(&sessionData, &refs, CHECK_NULL))
        return;
    if (refs->actual == NULL)
        refs->actual = refs->references;
    else {
        r = refs->actual->next;
        olcxSetActReferenceToFirstVisible(refs, r);
    }
    olcxGenGotoActReference(refs);
}

static void olcxReferenceMinus(void) {
    OlcxReferences    *refs;
    Reference         *r,*l,*act;
    int                 rlevel;
    if (!sessionHasReferencesValidForOperation(&sessionData,  &refs, CHECK_NULL))
        return;
    rlevel = refListFilters[refs->refsFilterLevel];
    if (refs->actual == NULL) refs->actual = refs->references;
    else {
        act = refs->actual;
        l = NULL;
        for (r=refs->references; r!=act && r!=NULL; r=r->next) {
            if (r->usage < rlevel)
                l = r;
        }
        if (l==NULL) {
            if (options.xref2) {
                ppcBottomInformation("Moving to the last reference");
            }
            for (; r!=NULL; r=r->next) {
                if (r->usage < rlevel)
                    l = r;
            }
        }
        refs->actual = l;
    }
    olcxGenGotoActReference(refs);
}

static void olcxReferenceGotoDef(void) {
    OlcxReferences    *refs;
    Reference         *dr;

    if (!sessionHasReferencesValidForOperation(&sessionData, &refs,CHECK_NULL))
        return;
    dr = getDefinitionRef(refs->references);
    if (dr != NULL) refs->actual = dr;
    else refs->actual = refs->references;
    //&fprintf(dumpOut,"goto ref %d %d\n", refs->actual->position.line, refs->actual->position.col);
    olcxGenGotoActReference(refs);
}

static void olcxReferenceGotoCurrent(void) {
    OlcxReferences    *refs;
    if (!sessionHasReferencesValidForOperation(&sessionData, &refs,CHECK_NULL))
        return;
    olcxGenGotoActReference(refs);
}

static void olcxReferenceGetCurrentRefn(void) {
    OlcxReferences    *refs;
    int                 n;
    if (!sessionHasReferencesValidForOperation(&sessionData, &refs,CHECK_NULL))
        return;
    n = getCurrentRefPosition(refs);
    assert(options.xref2);
    ppcValueRecord(PPC_UPDATE_CURRENT_REFERENCE, n, "");
}

static void olcxReferenceGotoCaller(void) {
    OlcxReferences    *refs;
    if (!sessionHasReferencesValidForOperation(&sessionData, &refs,CHECK_NULL))
        return;
    if (refs->callerPosition.file != NO_FILE_NUMBER) {
        gotoOnlineCxref(&refs->callerPosition, UsageUsed, "");

    } else {
        indicateNoReference();
    }
}

#define MAX_SYMBOL_MESSAGE_LEN 50

static void olcxPrintSymbolName(OlcxReferences *refs) {
    char ttt[MAX_CX_SYMBOL_SIZE+MAX_SYMBOL_MESSAGE_LEN];
    SymbolsMenu *ss;
    if (refs==NULL) {
        if (options.xref2) {
            ppcBottomInformation("stack is now empty");
        } else {
            fprintf(communicationChannel, "*stack is now empty");
        }
        //&     fprintf(communicationChannel, "*");
    } else if (refs->hkSelectedSym==NULL) {
        if (options.xref2) {
            ppcBottomInformation("Current top symbol: <empty>");
        } else {
            fprintf(communicationChannel, "*Current top symbol: <empty>");
        }
    } else {
        ss = refs->hkSelectedSym;
        sprintf(ttt, "Current top symbol: ");
        assert(strlen(ttt) < MAX_SYMBOL_MESSAGE_LEN);
        sprintfSymbolLinkName(ss, ttt+strlen(ttt));
        ppcBottomInformation(ttt);
    }
}


SymbolsMenu *olCreateSpecialMenuItem(char *fieldName, int cfi, Storage storage){
    SymbolsMenu     *res;
    ReferenceItem     ss;

    fillReferenceItem(&ss, fieldName, cfi, TypeDefault, storage, GlobalScope, GlobalVisibility);
    res = olCreateNewMenuItem(&ss, ss.vApplClass, ss.vApplClass, &noPosition, UsageNone,
                              1, 1, OOC_VIRT_SAME_APPL_FUN_CLASS,
                              UsageUsed, 0);
    return res;
}

bool isSameCxSymbol(ReferenceItem *p1, ReferenceItem *p2) {
    if (p1 == p2)
        return true;
    if (p1->visibility != p2->visibility)
        return false;
    if (p1->type!=TypeCppCollate && p2->type!=TypeCppCollate && p1->type!=p2->type)
        return false;
    if (p1->storage!=p2->storage)
        return false;

    if (strcmp(p1->linkName, p2->linkName) != 0)
        return false;
    return true;
}

bool olcxIsSameCxSymbol(ReferenceItem *p1, ReferenceItem *p2) {
    int n1len, n2len;
    char *n1start, *n2start;

    getBareName(p1->linkName, &n1start, &n1len);
    getBareName(p2->linkName, &n2start, &n2len);
    if (n1len != n2len)
        return false;
    if (strncmp(n1start, n2start, n1len))
        return false;
    return true;
}

void olStackDeleteSymbol(OlcxReferences *refs) {
    OlcxReferences **rr;
    for (rr= &sessionData.browserStack.root; *rr!=NULL&&*rr!=refs; rr= &(*rr)->previous)
        ;
    assert(*rr != NULL);
    deleteOlcxRefs(rr, &sessionData.browserStack);
}

static void olcxMenuInspectDef(SymbolsMenu *menu) {
    SymbolsMenu *ss;

    for (ss=menu; ss!=NULL; ss=ss->next) {
        //&sprintf(tmpBuff,"checking line %d", ss->outOnLine); ppcBottomInformation(tmpBuff);
        int line = SYMBOL_MENU_FIRST_LINE + ss->outOnLine;
        if (line == options.olcxMenuSelectLineNum)
            goto breakl;
    }
 breakl:
    if (ss == NULL) {
        indicateNoReference();
    } else {
        if (ss->defpos.file>=0 && ss->defpos.file!=NO_FILE_NUMBER) {
            gotoOnlineCxref(&ss->defpos, UsageDefined, "");
        } else {
            indicateNoReference();
        }
    }
}

static void olcxSymbolMenuInspectDef(void) {
    OlcxReferences    *refs;
    if (!sessionHasReferencesValidForOperation(&sessionData, &refs,CHECK_NULL))
        return;
    olcxMenuInspectDef(refs->menuSym);
}

void olProcessSelectedReferences(OlcxReferences *rstack,
                                 void (*referencesMapFun)(OlcxReferences *rstack, SymbolsMenu *menu)) {
    if (rstack->menuSym == NULL)
        return;

    LIST_MERGE_SORT(Reference, rstack->references, olcxReferenceInternalLessFunction);
    for (SymbolsMenu *m = rstack->menuSym; m != NULL; m = m->next) {
        referencesMapFun(rstack, m);
    }
    olcxSetCurrentRefsOnCaller(rstack);
    LIST_MERGE_SORT(Reference, rstack->references, referenceIsLessThan);
}

void olcxRecomputeSelRefs(OlcxReferences *refs) {
    freeReferences(refs->references); refs->references = NULL;
    olProcessSelectedReferences(refs, genOnLineReferences);
}

static void olcxMenuToggleSelect(void) {
    OlcxReferences    *refs;
    SymbolsMenu     *ss;
    int                 line;

    if (!sessionHasReferencesValidForOperation(&sessionData, &refs, CHECK_NULL))
        return;
    for (ss=refs->menuSym; ss!=NULL; ss=ss->next) {
        line = SYMBOL_MENU_FIRST_LINE + ss->outOnLine;
        if (line == options.olcxMenuSelectLineNum) {
            ss->selected = !ss->selected; // WTF! Was: ss->selected = ss->selected ^ 1;
            olcxRecomputeSelRefs(refs);
            break;
        }
    }
    if (ss!=NULL) {
        olcxPrintRefList(";", refs);
    }
}

static void olcxMenuSelectOnly(void) {
    OlcxReferences *refs;
    SymbolsMenu *selection;

    if (!sessionHasReferencesValidForOperation(&sessionData, &refs, CHECK_NULL))
        return;

    selection = NULL;
    for (SymbolsMenu *menu=refs->menuSym; menu!=NULL; menu=menu->next) {
        menu->selected = false;
        int line = SYMBOL_MENU_FIRST_LINE + menu->outOnLine;
        if (line == options.olcxMenuSelectLineNum) {
            menu->selected = true;
            selection = menu;
        }
    }

    if (selection==NULL) {
        ppcBottomWarning("No Symbol");
        return;
    }
    olcxRecomputeSelRefs(refs);

    Reference *definition = getDefinitionRef(refs->references);
    if (definition != NULL) {
        refs->actual = definition;
        olcxPrintRefList(";", refs);
        ppcGotoPosition(&refs->actual->position);
    } else
        ppcBottomWarning("Definition not found");
}


// Map function
static void selectUnusedSymbols(SymbolsMenu *menu, void *p1) {
    int filter, *flp;

    flp = (int *)p1;
    filter = *flp;
    for (SymbolsMenu *m=menu; m!=NULL; m=m->next) {
        m->visible = true; m->selected = false;
    }
    for (SymbolsMenu *m=menu; m!=NULL; m=m->next) {
        if (m->defRefn!=0 && m->refn==0)
            m->selected = true;
    }
    for (SymbolsMenu *m=menu; m!=NULL; m=m->next) {
        if (m->selected)
            goto fini2;
    }
    //nothing selected, make the symbol unvisible
    for (SymbolsMenu *m=menu; m!=NULL; m=m->next) {
        m->visible = false;
    }
 fini2:
    if (filter>0) {
        // make all unselected unvisible
        for (SymbolsMenu *m=menu; m!=NULL; m=m->next) {
            if (!m->selected)
                m->visible = false;
        }
    }
    return;
}


static void olcxMenuSelectAll(bool selected) {
    OlcxReferences *refs;

    if (!sessionHasReferencesValidForOperation(&sessionData, &refs, CHECK_NULL))
        return;
    if (refs->command == OLO_GLOBAL_UNUSED) {
        if (options.xref2) {
            ppcGenRecord(PPC_WARNING, "The browser does not display project unused symbols anymore");
        }
    }
    for (SymbolsMenu *menu=refs->menuSym; menu!=NULL; menu=menu->next) {
        if (menu->visible)
            menu->selected = selected;
    }
    olcxRecomputeSelRefs(refs);
    if (options.xref2) {
        olcxPrintRefList(";", refs);
    } else {
        fprintf(communicationChannel, "*Done");
    }
}

static void setDefaultSelectedVisibleItems(SymbolsMenu *menu,
                                           unsigned ooVisible,
                                           unsigned ooSelected
) {
    for (SymbolsMenu *m=menu; m!=NULL; m=m->next) {
        unsigned ooBits = m->ooBits;
        bool visible = ooBitsGreaterOrEqual(ooBits, ooVisible);
        bool selected = false;
        if (visible) {
            selected=ooBitsGreaterOrEqual(ooBits, ooSelected);
            if (m->references.type==TypeCppCollate)
                selected=false;
        }
        m->selected = selected;
        m->visible = visible;
    }
}

static bool isRenameMenuSelection(int command) {
    return command == OLO_RENAME
        || command == OLO_ENCAPSULATE
        || command == OLO_ARG_MANIP
        || command == OLO_PUSH_FOR_LOCALM
        || command == OLO_SAFETY_CHECK2
        || options.manualResolve == RESOLVE_DIALOG_NEVER
        ;
}

static void setSelectedVisibleItems(SymbolsMenu *menu, int command, int filterLevel) {
    unsigned ooselected, oovisible;
    if (command == OLO_GLOBAL_UNUSED) {
        splitMenuPerSymbolsAndMap(menu, selectUnusedSymbols, &filterLevel);
        goto sfini;
    }

    if (command == OLO_PUSH_NAME) {
        oovisible = 0;
        ooselected = 0;
    } else if (isRenameMenuSelection(command)) {
        oovisible = options.ooChecksBits;
        ooselected = RENAME_SELECTION_OO_BITS;
    } else {
        //&oovisible = options.ooChecksBits;
        oovisible = menuFilterOoBits[filterLevel];
        ooselected = DEFAULT_SELECTION_OO_BITS;
    }
    setDefaultSelectedVisibleItems(menu, oovisible, ooselected);
sfini:
    return;
}

static void olcxMenuSelectPlusolcxMenuSelectFilterSet(int flevel) {
    OlcxReferences    *refs;

    if (!sessionHasReferencesValidForOperation(&sessionData, &refs, DONT_CHECK_NULL))
        return;
    if (refs!=NULL && flevel < MAX_MENU_FILTER_LEVEL && flevel >= 0) {
        if (refs->menuFilterLevel != flevel) {
            refs->menuFilterLevel = flevel;
            setSelectedVisibleItems(refs->menuSym, refs->command, refs->menuFilterLevel);
            olcxRecomputeSelRefs(refs);
        }
    }
    if (refs!=NULL) {
        olcxPrintSelectionMenu(refs->menuSym);
    } else {
        if (options.xref2) {
            olcxPrintSelectionMenu(NULL);
            olcxPrintRefList(";", NULL);
        } else {
            fprintf(communicationChannel, "=");
        }
    }
}

static void olcxReferenceFilterSet(int filterLevel) {
    OlcxReferences *refs;

    if (!sessionHasReferencesValidForOperation(&sessionData,  &refs, DONT_CHECK_NULL))
        return;
    if (refs!=NULL && filterLevel < MAX_REF_LIST_FILTER_LEVEL && filterLevel >= 0) {
        refs->refsFilterLevel = filterLevel;
    }
    if (options.xref2) {
        // move to the visible reference
        if (refs!=NULL)
            olcxSetActReferenceToFirstVisible(refs, refs->actual);
        olcxPrintRefList(";", refs);
    } else {
        fprintf(communicationChannel, "*");
    }
}

static OlcxReferences *getNextTopStackItem(OlcxReferencesStack *stack) {
    OlcxReferences *rr, *nextrr;
    nextrr = NULL;
    rr = stack->root;
    while (rr!=NULL && rr!=stack->top) {
        nextrr = rr;
        rr = rr->previous;
    }
    assert(rr==stack->top);
    return nextrr;
}

static void olcxReferenceRePush(void) {
    OlcxReferences *refs, *nextrr;

    if (!sessionHasReferencesValidForOperation(&sessionData, &refs, DONT_CHECK_NULL))
        return;
    nextrr = getNextTopStackItem(&sessionData.browserStack);
    if (nextrr != NULL) {
        sessionData.browserStack.top = nextrr;
        olcxGenGotoActReference(sessionData.browserStack.top);
        // TODO, replace this by follwoing since 1.6.1
        //& ppcGotoPosition(&sessionData->browserStack.top->callerPosition);
        olcxPrintSymbolName(sessionData.browserStack.top);
    } else {
        if (options.xref2) {
            ppcBottomWarning("You are on the top of browser stack.");
        } else {
            fprintf(communicationChannel, "*** Complete stack, no pop-ed references");
        }
    }
}

static void olcxReferencePop(void) {
    OlcxReferences *refs;
    if (!sessionHasReferencesValidForOperation(&sessionData, &refs, CHECK_NULL))
        return;
    if (refs->callerPosition.file != NO_FILE_NUMBER) {
        gotoOnlineCxref(&refs->callerPosition, UsageUsed, "");
    } else {
        indicateNoReference();
    }
    //& olStackDeleteSymbol(refs);  // this was before non deleting pop
    sessionData.browserStack.top = refs->previous;
    olcxPrintSymbolName(sessionData.browserStack.top);
}

void olcxPopOnly(void) {
    OlcxReferences *refs;

    if (!sessionHasReferencesValidForOperation(&sessionData, &refs, CHECK_NULL))
        return;
    if (!options.xref2)
        fprintf(communicationChannel, "*");
    //& olStackDeleteSymbol(refs);
    sessionData.browserStack.top = refs->previous;
}

static void safetyCheckAddDiffRef(Reference *r, OlcxReferences *diffrefs,
                                  int mode) {
    int prefixchar;
    prefixchar = ' ';
    if (diffrefs->references == NULL) {
        fprintf(communicationChannel, "%s", COLCX_LIST);
        prefixchar = '>';
    }
    if (mode == DIFF_MISSING_REF) {
        fprintf(communicationChannel, "%c %s:%d missing reference\n", prefixchar,
                simpleFileNameFromFileNum(r->position.file), r->position.line);
    } else if (mode == DIFF_UNEXPECTED_REF) {
        fprintf(communicationChannel, "%c %s:%d unexpected new reference\n", prefixchar,
                simpleFileNameFromFileNum(r->position.file), r->position.line);
    } else {
        assert(0);
    }
    olcxAppendReference(r, diffrefs);
}

static void safetyCheckDiff(Reference **anr1,
                            Reference **aor2,
                            OlcxReferences *diffrefs
                            ) {
    Reference *r, *nr1, *or2;
    int mode;
    LIST_MERGE_SORT(Reference, *anr1, olcxReferenceInternalLessFunction);
    LIST_MERGE_SORT(Reference, *aor2, olcxReferenceInternalLessFunction);
    nr1 = *anr1; or2 = *aor2;
    while (nr1!=NULL && or2!=NULL) {
        if (nr1->position.file==or2->position.file && nr1->position.line==or2->position.line) {
            nr1 = nr1->next; or2=or2->next;
        } else {
            if (SORTED_LIST_LESS(nr1, *or2)) {
                safetyCheckAddDiffRef(nr1, diffrefs, DIFF_UNEXPECTED_REF);
                nr1=nr1->next;
            } else {
                safetyCheckAddDiffRef(or2, diffrefs, DIFF_MISSING_REF);
                or2=or2->next;
            }
        }
    }
    if (nr1!=NULL || or2!=NULL) {
        if (nr1!=NULL) {
            r = nr1;
            mode = DIFF_UNEXPECTED_REF;
        } else {
            r = or2;
            mode = DIFF_MISSING_REF;
        }
        for (; r!=NULL; r=r->next) {
            safetyCheckAddDiffRef(r, diffrefs, mode);
        }
    }
    diffrefs->actual = diffrefs->references;
    if (diffrefs->references!=NULL) {
        assert(diffrefs->menuSym);
        olcxAddReferencesToSymbolsMenu(diffrefs->menuSym, diffrefs->references, 0);
    }
}

int getFileNumberFromName(char *name) {
    char *normalizedName;
    int fileNumber;

    normalizedName = normalizeFileName_static(name, cwd);
    if ((fileNumber = lookupFileTable(normalizedName)) != -1) {
        return fileNumber;
    } else {
        return NO_FILE_NUMBER;
    }
}

static Reference *olcxCreateFileShiftedRefListForCheck(Reference *reference) {
    Reference *res, **resa;
    int ofn, nfn, fmline, lmline;

    if (options.checkFileMovedFrom==NULL) return NULL;
    if (options.checkFileMovedTo==NULL) return NULL;
    //&fprintf(dumpOut,"!shifting %s --> %s\n", options.checkFileMovedFrom, options.checkFileMovedTo);
    ofn = getFileNumberFromName(options.checkFileMovedFrom);
    nfn = getFileNumberFromName(options.checkFileMovedTo);
    //&fprintf(dumpOut,"!shifting %d --> %d\n", ofn, nfn);
    if (ofn==NO_FILE_NUMBER) return NULL;
    if (nfn==NO_FILE_NUMBER) return NULL;
    fmline = options.checkFirstMovedLine;
    lmline = options.checkFirstMovedLine + options.checkLinesMoved;
    res = NULL; resa = &res;
    for (Reference *r=reference; r!=NULL; r=r->next) {
        Reference *tt = malloc(sizeof(Reference));
        *tt = *r;
        if (tt->position.file==ofn && tt->position.line>=fmline && tt->position.line<lmline) {
            tt->position.file = nfn;
            //&fprintf(dumpOut,"!shifting %d:%d to %d (%d %d %d)\n", tt->position.file, tt->position.line, options.checkNewLineNumber + (tt->position.line - fmline), options.checkFirstMovedLine, options.checkLinesMoved, options.checkNewLineNumber);
            tt->position.line = options.checkNewLineNumber + (tt->position.line - fmline);
        }
        *resa=tt; tt->next=NULL; resa= &(tt->next);
    }
    LIST_MERGE_SORT(Reference, res, olcxReferenceInternalLessFunction);
    return res;
}

static void olcxSafetyCheck2(void) {
    OlcxReferences *refs, *origrefs, *newrefs, *diffrefs;
    Reference *shifted;
    int pbflag=0;
    origrefs = newrefs = diffrefs = NULL;
    SAFETY_CHECK2_GET_SYM_LISTS(refs,origrefs,newrefs,diffrefs, pbflag);
    assert(origrefs && newrefs && diffrefs);
    if (pbflag) return;
    shifted = olcxCreateFileShiftedRefListForCheck(origrefs->references);
    if (shifted != NULL) {
        safetyCheckDiff(&newrefs->references, &shifted, diffrefs);
        freeReferences(shifted);
    } else {
        safetyCheckDiff(&newrefs->references, &origrefs->references, diffrefs);
    }
    if (diffrefs->references == NULL) {
        // no need to free here, as popings are not freed
        sessionData.browserStack.top = sessionData.browserStack.top->previous;
        sessionData.browserStack.top = sessionData.browserStack.top->previous;
        sessionData.browserStack.top = sessionData.browserStack.top->previous;
        fprintf(communicationChannel, "*Done. No conflicts detected.");
    } else {
        assert(diffrefs->menuSym);
        sessionData.browserStack.top = sessionData.browserStack.top->previous;
        fprintf(communicationChannel, " ** Some misinterpreted references detected. Please, undo last refactoring.");
    }
    fflush(communicationChannel);
}

static void olCompletionSelect(void) {
    OlcxReferences    *refs;
    Completion      *rr;
    if (!sessionHasReferencesValidForOperation(&sessionData, &refs, CHECK_NULL))
        return;
    rr = olCompletionNthLineRef(refs->completions, options.olcxGotoVal);
    if (rr==NULL) {
        errorMessage(ERR_ST, "selection out of range.");
        return;
    }
    if (options.xref2) {
        assert(sessionData.completionsStack.root!=NULL);
        ppcGotoPosition(&sessionData.completionsStack.root->callerPosition);
        ppcGenRecord(PPC_SINGLE_COMPLETION, rr->name);
    } else {
        gotoOnlineCxref(&refs->callerPosition, UsageUsed, rr->name);
    }
}

static void olcxReferenceSelectTagSearchItem(int refn) {
    Completion      *rr;
    OlcxReferences    *refs;
    char                ttt[MAX_FUNCTION_NAME_LENGTH];
    assert(refn > 0);
    assert(sessionData.retrieverStack.top);
    refs = sessionData.retrieverStack.top;
    rr = olCompletionNthLineRef(refs->completions, refn);
    if (rr == NULL) {
        errorMessage(ERR_ST, "selection out of range.");
        return;
    }
    assert(sessionData.retrieverStack.root!=NULL);
    ppcGotoPosition(&sessionData.retrieverStack.root->callerPosition);
    sprintf(ttt, " %s", rr->name);
    ppcGenRecord(PPC_SINGLE_COMPLETION, ttt);
}

static void olCompletionBack(void) {
    OlcxReferences    *top;

    top = sessionData.completionsStack.top;
    if (top != NULL && top->previous != NULL) {
        sessionData.completionsStack.top = sessionData.completionsStack.top->previous;
        ppcGotoPosition(&sessionData.completionsStack.top->callerPosition);
        printCompletionsList(false);
    }
}

static void olCompletionForward(void) {
    OlcxReferences    *top;

    top = getNextTopStackItem(&sessionData.completionsStack);
    if (top != NULL) {
        sessionData.completionsStack.top = top;
        ppcGotoPosition(&sessionData.completionsStack.top->callerPosition);
        printCompletionsList(false);
    }
}

static void olcxNoSymbolFoundErrorMessage(void) {
    if (options.serverOperation == OLO_PUSH_NAME) {
        ppcGenRecord(PPC_ERROR,"No symbol found.");
    } else {
        ppcGenRecord(PPC_ERROR,"No symbol found, please position the cursor on a program symbol.");
    }
}


static bool olcxCheckSymbolExists(void) {
    if (sessionData.browserStack.top!=NULL
        && sessionData.browserStack.top->menuSym==NULL) {
        return false;
    }
    return true;
}

static SymbolsMenu *firstVisibleSymbol(SymbolsMenu *first) {
    SymbolsMenu *fvisible = NULL;

    for (SymbolsMenu *ss=first; ss!=NULL; ss=ss->next) {
        if (ss->visible) {
            fvisible = ss;
            break;
        }
    }
    return fvisible;
}


bool olcxShowSelectionMenu(void) {
    SymbolsMenu *first, *fvisible;

    // decide whether to show manual resolution menu
    assert(sessionData.browserStack.top);
    if (options.serverOperation == OLO_PUSH_FOR_LOCALM) {
        // never ask for resolution for local motion symbols
        return false;
    }
    if (options.serverOperation == OLO_SAFETY_CHECK2) {
        // safety check showing of menu is resolved by safetyCheck2ShouldWarn
        return false;
    }
    // first if just zero or one symbol, no resolution
    first = sessionData.browserStack.top->menuSym;
    if (first == NULL) {
        //&fprintf(dumpOut,"no resolve, no symbol\n"); fflush(dumpOut);
        return false; // no symbol
    }
    fvisible = firstVisibleSymbol(first);
    if (fvisible==NULL) {
        //&fprintf(dumpOut,"no resolve, no visible\n"); fflush(dumpOut);
        return false; // no visible
    }
    first = NULL;
    if (options.serverOperation==OLO_PUSH
        || options.serverOperation==OLO_PUSH_ONLY
        || options.serverOperation==OLO_PUSH_AND_CALL_MACRO
        || options.serverOperation==OLO_RENAME
        || options.serverOperation==OLO_ARG_MANIP
    ) {
        // manually only if different
        for (SymbolsMenu *ss=sessionData.browserStack.top->menuSym; ss!=NULL; ss=ss->next) {
            if (ss->selected) {
                if (first == NULL) {
                    first = ss;
                } else if ((! isSameCxSymbol(&first->references, &ss->references))
                           || first->references.vApplClass!=ss->references.vApplClass) {
                    return true;
                }
            }
        }
    } else {
        for (SymbolsMenu *ss=sessionData.browserStack.top->menuSym; ss!=NULL; ss=ss->next) {
            if (ss->visible) {
                if (first!=NULL) {
                    return true;
                }
                first = ss;
            }
        }
    }
    return false;
}

static bool olMenuHashFileNumLess(SymbolsMenu *s1, SymbolsMenu *s2) {
    int fi1, fi2;
    fi1 = cxFileHashNumber(s1->references.linkName);
    fi2 = cxFileHashNumber(s2->references.linkName);
    if (fi1 < fi2) return true;
    if (fi1 > fi2) return false;
    if (s1->references.visibility == LocalVisibility) return true;
    if (s1->references.visibility == LocalVisibility) return false;
    // both files and categories equals ?
    return false;
}

void getLineAndColumnCursorPositionFromCommandLineOptions(int *l, int *c) {
    assert(options.olcxlccursor!=NULL);
    sscanf(options.olcxlccursor,"%d:%d", l, c);
}

static bool refItemsOrderLess(SymbolsMenu *menu1, SymbolsMenu *menu2) {
    ReferenceItem *r1, *r2;
    char *name1, *name2;
    int len1; UNUSED len1;
    int len2; UNUSED len2;

    r1 = &menu1->references;
    r2 = &menu2->references;
    getBareName(r1->linkName, &name1, &len1);
    getBareName(r2->linkName, &name2, &len2);
    int cmp = strcmp(name1, name2);
    if (cmp!=0)
        return cmp<0;
    cmp = strcmp(r1->linkName, r2->linkName);
    return cmp<0;
}

void olCreateSelectionMenu(int command) {
    OlcxReferences    *rstack;
    SymbolsMenu     *ss;
    int                 fnum;

    // I think this ordering is useless

    // The above comment was in the original code, by Marian perhaps?
    // At least the tests pass if this sorting is removed but I'm
    // unsure we have all tests required to be sure
    LIST_MERGE_SORT(SymbolsMenu,
                    sessionData.browserStack.top->hkSelectedSym,
                    refItemsOrderLess);

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    ss = rstack->hkSelectedSym;
    if (ss == NULL)
        return;

    renameCollationSymbols(ss);
    LIST_SORT(SymbolsMenu, rstack->hkSelectedSym, olMenuHashFileNumLess);

    ss = rstack->hkSelectedSym;
    while (ss!=NULL) {
        scanReferencesToCreateMenu(ss->references.linkName);
        fnum = cxFileHashNumber(ss->references.linkName);
        while (ss!=NULL && fnum==cxFileHashNumber(ss->references.linkName))
            ss = ss->next;
    }

    mapOverReferenceTable(mapCreateSelectionMenu);
    mapOverReferenceTable(putOnLineLoadedReferences);
    setSelectedVisibleItems(rstack->menuSym, command, rstack->menuFilterLevel);
    assert(rstack->references==NULL);
    olProcessSelectedReferences(rstack, genOnLineReferences);

    // isn't ordering useless ?
    // Again, the above comment was in the original code, by Marian
    // perhaps?  But here some test fail if this sorting is removed
    // because they come out in the wrong order, but if the editor
    // client sorts them anyway (does it?) that would not matter
    LIST_MERGE_SORT(SymbolsMenu,
                    sessionData.browserStack.top->menuSym,
                    refItemsOrderLess);
}

void olcxPushSpecialCheckMenuSym(char *symname) {
    OlcxReferences *rstack;

    pushEmptySession(&sessionData.browserStack);
    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    rstack->hkSelectedSym = olCreateSpecialMenuItem(symname, NO_FILE_NUMBER, StorageDefault);
    rstack->menuSym = olCreateSpecialMenuItem(symname, NO_FILE_NUMBER, StorageDefault);
}

static void olcxProcessGetRequest(void) {
    char *name, *value;

    name = options.variableToGet;
    //&fprintf(dumpOut,"![get] looking for %s\n", name);
    value = getOptionVariable(name);
    if (value != NULL) {
        // O.K. this is a special case, if input file is given
        // then make additional 'predefined' replacements
        ppcGenRecord(PPC_SET_INFO, expandPredefinedSpecialVariables_static(value, inputFileName));
    } else {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff,"No \"-set %s <command>\" option specified for active project", name);
        errorMessage(ERR_ST, tmpBuff);
    }
}

void olcxPrintPushingAction(ServerOperation operation) {
    switch (operation) {
    case OLO_PUSH:
        if (olcxCheckSymbolExists()) {
            olcxOrderRefsAndGotoDefinition();
        } else {
            // to auto repush symbol by name, but I do not like it.
            //& if (options.xref2) ppcGenRecord(PPC_NO_SYMBOL, "");
            //& else
            olcxNoSymbolFoundErrorMessage();
            olStackDeleteSymbol(sessionData.browserStack.top);
        }
        break;
    case OLO_PUSH_NAME:
        if (olcxCheckSymbolExists()) {
            olcxOrderRefsAndGotoDefinition();
        } else {
            olcxNoSymbolFoundErrorMessage();
            olStackDeleteSymbol(sessionData.browserStack.top);
        }
        break;
    case OLO_GLOBAL_UNUSED:
    case OLO_LOCAL_UNUSED:
        // no output for dead code detection ???
        break;
    case OLO_LIST:
        if (olcxCheckSymbolExists()) {
            olcxReferenceList(";");
        } else {
            olcxNoSymbolFoundErrorMessage();
            olStackDeleteSymbol(sessionData.browserStack.top);
        }
        break;
    case OLO_PUSH_ONLY:
        if (olcxCheckSymbolExists()) {
            olcxPushOnly();
        } else {
            olcxNoSymbolFoundErrorMessage();
            olStackDeleteSymbol(sessionData.browserStack.top);
        }
        break;
    case OLO_PUSH_AND_CALL_MACRO:
        if (olcxCheckSymbolExists()) {
            olcxPushAndCallMacro();
        } else {
            olcxNoSymbolFoundErrorMessage();
            olStackDeleteSymbol(sessionData.browserStack.top);
        }
        break;
    case OLO_PUSH_FOR_LOCALM:
        if (olcxCheckSymbolExists())
            olcxPushOnly();
        else
            olcxNoSymbolFoundErrorMessage();
        break;
    case OLO_RENAME:
    case OLO_ARG_MANIP:
    case OLO_ENCAPSULATE:
        if (olcxCheckSymbolExists())
            olcxRenameInit();
        else
            olcxNoSymbolFoundErrorMessage();
        break;
    case OLO_SAFETY_CHECK2:
        if (olcxCheckSymbolExists())
            olcxSafetyCheck2();
        else
            olcxNoSymbolFoundErrorMessage();
        break;
    default:
        assert(0);
    }
}

#ifdef DUMP_SELECTION_MENU
static void olcxDumpSelectionMenu(SymbolsMenu *menu) {
    for (SymbolsMenu *s=menu; s!=NULL; s=s->next) {
        log_trace">> %d/%d %s %s %d", s->defRefn, s->refn, s->references.linkName,
            simpleFileName(getFileItem(s->references.vApplClass)->name),
            s->outOnLine);
    }
}
#endif

static void mainAnswerReferencePushingAction(ServerOperation operation) {
    assert(requiresCreatingRefs(operation));
    //&olcxPrintSelectionMenu(sessionData->browserStack.top->hkSelectedSym);
    //&olcxPrintSelectionMenu(sessionData->browserStack.top->hkSelectedSym);
    olCreateSelectionMenu(operation);
    //&olcxPrintSelectionMenu(sessionData->browserStack.top->hkSelectedSym);

#ifdef DUMP_SELECTION_MENU
    olcxDumpSelectionMenu(sessionData->browserStack.top->menuSym);
#endif
    if (options.manualResolve == RESOLVE_DIALOG_ALWAYS
        || (olcxShowSelectionMenu()
            && options.manualResolve != RESOLVE_DIALOG_NEVER)) {
        if (options.xref2) {
            ppcGenRecord(PPC_DISPLAY_OR_UPDATE_BROWSER, "");
        } else {
            olcxPrintSelectionMenu(sessionData.browserStack.top->menuSym);
        }
    } else {
        assert(sessionData.browserStack.top);
        //&olProcessSelectedReferences(sessionData->browserStack.top, genOnLineReferences);
        olcxPrintPushingAction(options.serverOperation);
    }
}

static void mapAddLocalUnusedSymbolsToHkSelection(ReferenceItem *ss) {
    bool used = false;
    Reference *definitionReference = NULL;

    if (ss->visibility != LocalVisibility)
        return;
    for (Reference *r = ss->references; r!=NULL; r=r->next) {
        if (isDefinitionOrDeclarationUsage(r->usage)) {
            if (r->position.file == inputFileNumber) {
                if (isDefinitionUsage(r->usage)) {
                    definitionReference = r;
                }
                if (definitionReference == NULL)
                    definitionReference = r;
            }
        } else {
            used = true;
            break;
        }
    }
    if (!used && definitionReference!=NULL) {
        olAddBrowsedSymbolToMenu(&sessionData.browserStack.top->hkSelectedSym, ss,
                                 true, true, 0, UsageDefined, 0, &definitionReference->position,
                                 definitionReference->usage);
    }
}

static void pushLocalUnusedSymbolsAction(void) {
    OlcxReferences    *rstack;
    SymbolsMenu     *ss;

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    ss = rstack->hkSelectedSym;
    assert(ss == NULL);
    mapOverReferenceTable(mapAddLocalUnusedSymbolsToHkSelection);
    olCreateSelectionMenu(options.serverOperation);
}

static void answerPushLocalUnusedSymbolsAction(void) {
    pushLocalUnusedSymbolsAction();
    assert(options.xref2);
    ppcGenRecord(PPC_DISPLAY_OR_UPDATE_BROWSER, "");
}

static void answerPushGlobalUnusedSymbolsAction(void) {
    OlcxReferences    *rstack;
    SymbolsMenu     *ss;

    assert(sessionData.browserStack.top);
    rstack = sessionData.browserStack.top;
    ss = rstack->hkSelectedSym;
    assert(ss == NULL);
    scanForGlobalUnused(options.cxrefsLocation);
    olCreateSelectionMenu(options.serverOperation);
    assert(options.xref2);
    ppcGenRecord(PPC_DISPLAY_OR_UPDATE_BROWSER, "");
}

static void getCallerPositionFromCommandLineOption(Position *opos) {
    int file, line, col;

    assert(opos != NULL);
    file = olOriginalFileNumber;
    getLineAndColumnCursorPositionFromCommandLineOptions(&line, &col);
    *opos = makePosition(file, line, col);
}

static void pushSymbolByName(char *name) {
    OlcxReferences *rstack;
    if (cache.index > 0) {
        int spass;
        spass = currentPass; currentPass=1;
        recoverCachePointZero();
        currentPass = spass;
    }
    rstack = sessionData.browserStack.top;
    rstack->hkSelectedSym = olCreateSpecialMenuItem(name, NO_FILE_NUMBER, StorageDefault);
    getCallerPositionFromCommandLineOption(&rstack->callerPosition);
}

void answerEditAction(void) {
    OlcxReferences *rstack, *nextrr;

    ENTER();
    assert(communicationChannel);

    log_trace("Server operation = %s(%d)", operationNamesTable[options.serverOperation], options.serverOperation);
    switch (options.serverOperation) {
    case OLO_COMPLETION:
    case OLO_SEARCH:
        printCompletions(&collectedCompletions);
        break;
    case OLO_EXTRACT:
        if (! parsedInfo.extractProcessedFlag) {
            fprintf(communicationChannel,"*** No function/method enclosing selected block found **");
        }
        break;
    case OLO_TAG_SEARCH: {
        Position givenPosition;
        getCallerPositionFromCommandLineOption(&givenPosition);
        //&olCompletionListInit(&givenPosition);
        if (!options.xref2)
            fprintf(communicationChannel,";");
        pushEmptySession(&sessionData.retrieverStack);
        sessionData.retrieverStack.top->callerPosition = givenPosition;

        scanForSearch(options.cxrefsLocation);
        printTagSearchResults();
        break;
    }
    case OLO_TAG_SEARCH_BACK:
        if (sessionData.retrieverStack.top!=NULL &&
            sessionData.retrieverStack.top->previous!=NULL) {
            sessionData.retrieverStack.top = sessionData.retrieverStack.top->previous;
            ppcGotoPosition(&sessionData.retrieverStack.top->callerPosition);
            printTagSearchResults();
        }
        break;
    case OLO_TAG_SEARCH_FORWARD:
        nextrr = getNextTopStackItem(&sessionData.retrieverStack);
        if (nextrr != NULL) {
            sessionData.retrieverStack.top = nextrr;
            ppcGotoPosition(&sessionData.retrieverStack.top->callerPosition);
            printTagSearchResults();
        }
        break;
    case OLO_ACTIVE_PROJECT:
        if (options.project != NULL) {
            ppcGenRecord(PPC_SET_INFO, options.project);
        } else {
            if (olOriginalComFileNumber == NO_FILE_NUMBER) {
                ppcGenRecord(PPC_ERROR, "No source file to identify project");
            } else {
                char standardOptionsFileName[MAX_FILE_NAME_SIZE];
                char standardOptionsSectionName[MAX_FILE_NAME_SIZE];
                char *inputFileName = getFileItem(olOriginalComFileNumber)->name;
                log_trace("inputFileName = %s", inputFileName);
                searchStandardOptionsFileAndProjectForFile(inputFileName, standardOptionsFileName, standardOptionsSectionName);
                if (standardOptionsFileName[0]==0 || standardOptionsSectionName[0]==0) {
                    if (!options.noErrors) {
                        ppcGenRecord(PPC_NO_PROJECT, inputFileName);
                    }
                } else {
                    ppcGenRecord(PPC_SET_INFO, standardOptionsSectionName);
                }
            }
        }
        break;
    case OLO_GET_ENV_VALUE:
        olcxProcessGetRequest();
        break;
    case OLO_NEXT:
        olcxReferencePlus();
        break;
    case OLO_PREVIOUS:
        olcxReferenceMinus();
        break;
    case OLO_GOTO_DEF:
        olcxReferenceGotoDef();
        break;
    case OLO_GOTO_CALLER:
        olcxReferenceGotoCaller();
        break;
    case OLO_GOTO_CURRENT:
        olcxReferenceGotoCurrent();
        break;
    case OLO_GET_CURRENT_REFNUM:
        olcxReferenceGetCurrentRefn();
        break;
    case OLO_CSELECT:
        olCompletionSelect();
        break;
    case OLO_COMPLETION_BACK:
        olCompletionBack();
        break;
    case OLO_COMPLETION_FORWARD:
        olCompletionForward();
        break;
    case OLO_GOTO:
        olcxReferenceGotoRef(options.olcxGotoVal);
        break;
    case OLO_CGOTO:
        olcxReferenceGotoCompletion(options.olcxGotoVal);
        break;
    case OLO_TAGGOTO:
        olcxReferenceGotoTagSearchItem(options.olcxGotoVal);
        break;
    case OLO_TAGSELECT:
        olcxReferenceSelectTagSearchItem(options.olcxGotoVal);
        break;
    case OLO_REF_FILTER_SET:
        olcxReferenceFilterSet(options.filterValue);
        break;
    case OLO_REPUSH:
        olcxReferenceRePush();
        break;
    case OLO_POP:
        olcxReferencePop();
        break;
    case OLO_POP_ONLY:
        olcxPopOnly();
        break;
    case OLO_MENU_INSPECT_DEF:
        olcxSymbolMenuInspectDef();
        break;
    case OLO_MENU_SELECT:
        olcxMenuToggleSelect();
        break;
    case OLO_MENU_SELECT_ONLY:
        olcxMenuSelectOnly();
        break;
    case OLO_MENU_SELECT_ALL:
        olcxMenuSelectAll(true);
        break;
    case OLO_MENU_SELECT_NONE:
        olcxMenuSelectAll(false);
        break;
    case OLO_MENU_FILTER_SET:
        olcxMenuSelectPlusolcxMenuSelectFilterSet(options.filterValue);
        break;
    case OLO_SET_MOVE_FUNCTION_TARGET:
        assert(options.xref2);
        if (!parsedInfo.moveTargetAccepted) {
            ppcGenRecord(PPC_ERROR, "Invalid target place");
        }
        break;
    case OLO_GET_METHOD_COORD:
        if (parsedInfo.methodCoordEndLine!=0) {
            fprintf(communicationChannel,"*%d", parsedInfo.methodCoordEndLine);
        } else {
            errorMessage(ERR_ST, "No method found.");
        }
        break;
    case OLO_GOTO_PARAM_NAME:
        // I hope this is not used anymore, put there assert(0); Well,
        // it is used from refactory, but that probably only executes
        // the parsers and server and not cxref...
        if (olstringServed && parameterPosition.file != NO_FILE_NUMBER) {
            gotoOnlineCxref(&parameterPosition, UsageDefined, "");
            olStackDeleteSymbol(sessionData.browserStack.top);
        } else {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff, "Parameter %d not found.", options.olcxGotoVal);
            errorMessage(ERR_ST, tmpBuff);
        }
        break;
    case OLO_GET_PRIMARY_START:
        if (olstringServed && primaryStartPosition.file != NO_FILE_NUMBER) {
            gotoOnlineCxref(&primaryStartPosition, UsageDefined, "");
            olStackDeleteSymbol(sessionData.browserStack.top);
        } else {
            errorMessage(ERR_ST, "Begin of primary expression not found.");
        }
        break;
    case OLO_GET_AVAILABLE_REFACTORINGS:
        olGetAvailableRefactorings();
        olStackDeleteSymbol(sessionData.browserStack.top);
        break;
    case OLO_PUSH_NAME:
        pushSymbolByName(options.pushName);
        mainAnswerReferencePushingAction(options.serverOperation);
        break;
    case OLO_GLOBAL_UNUSED:
        answerPushGlobalUnusedSymbolsAction();
        break;
    case OLO_LOCAL_UNUSED:
        answerPushLocalUnusedSymbolsAction();
        break;
    case OLO_ARG_MANIP:
        rstack = sessionData.browserStack.top;
        assert(rstack!=NULL);
        if (rstack->hkSelectedSym == NULL) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff,"Cursor (point) has to be positioned on a method or constructor name before invocation of this refactoring, not on the parameter itself. Please move the cursor onto the method (constructor) name and reinvoke the refactoring.");
            errorMessage(ERR_ST, tmpBuff);
        } else {
            mainAnswerReferencePushingAction(options.serverOperation);
        }
        break;
    case OLO_PUSH:
    case OLO_PUSH_ONLY:
        mainAnswerReferencePushingAction(options.serverOperation);
        break;
    case OLO_GET_LAST_IMPORT_LINE:
        if (options.xref2) {
            char tmpBuff[TMP_BUFF_SIZE];
            sprintf(tmpBuff, "%d", parsedInfo.lastImportLine);
            ppcGenRecord(PPC_SET_INFO, tmpBuff);
        } else {
            fprintf(communicationChannel,"*%d", parsedInfo.lastImportLine);
        }
        break;
    case OLO_NOOP:
        break;
    default:
        log_fatal("unexpected default case for server operation %s", operationNamesTable[options.serverOperation]);
    } // switch

    fflush(communicationChannel);
    inputFileName = NULL;
    LEAVE();
}

int itIsSymbolToPushOlReferences(ReferenceItem *p,
                               OlcxReferences *rstack,
                               SymbolsMenu **rss,
                               int checkSelFlag) {
    for (SymbolsMenu *ss=rstack->menuSym; ss!=NULL; ss=ss->next) {
        if ((ss->selected || checkSelFlag==DO_NOT_CHECK_IF_SELECTED)
            && ss->references.vApplClass == p->vApplClass
            && isSameCxSymbol(p, &ss->references)) {
            *rss = ss;
            if (IS_BEST_FIT_MATCH(ss)) {
                return 2;
            } else {
                return 1;
            }
        }
    }
    *rss = NULL;
    return 0;
}


void putOnLineLoadedReferences(ReferenceItem *p) {
    int ols;
    SymbolsMenu *cms;

    ols = itIsSymbolToPushOlReferences(p,sessionData.browserStack.top,
                                       &cms, DO_NOT_CHECK_IF_SELECTED);
    if (ols > 0) {
        assert(cms);
        for (Reference *rr=p->references; rr!=NULL; rr=rr->next) {
            olcxAddReferenceToSymbolsMenu(cms, rr);
        }
    }
}

void genOnLineReferences(OlcxReferences *rstack, SymbolsMenu *cms) {
    if (cms->selected) {
        assert(cms);
        olcxAddReferences(cms->references.references, &rstack->references, ANY_FILE,
                          IS_BEST_FIT_MATCH(cms));
    }
}

static unsigned olcxOoBits(SymbolsMenu *menu, ReferenceItem *referenceItem) {
    assert(olcxIsSameCxSymbol(&menu->references, referenceItem));
    unsigned ooBits = 0;

    if (menu->references.type!=TypeCppCollate) {
        if (menu->references.type != referenceItem->type)
            goto fini;
        if (menu->references.storage != referenceItem->storage)
            goto fini;
        if (menu->references.visibility != referenceItem->visibility)
            goto fini;
    }
    if (strcmp(menu->references.linkName,referenceItem->linkName)==0) {
        ooBits |= OOC_PROFILE_EQUAL;
    }
    if (referenceItem->vApplClass == menu->references.vApplClass)
        ooBits |= OOC_VIRT_SAME_APPL_FUN_CLASS;
 fini:
    return ooBits;
}

static unsigned ooBitsMax(unsigned oo1, unsigned oo2) {
    unsigned res;
    res = 0;
    if ((oo1&OOC_PROFILE_MASK) > (oo2&OOC_PROFILE_MASK)) {
        res |= (oo1&OOC_PROFILE_MASK);
    } else {
        res |= (oo2&OOC_PROFILE_MASK);
    }
    if ((oo1&OOC_VIRTUAL_MASK) > (oo2&OOC_VIRTUAL_MASK)) {
        res |= (oo1&OOC_VIRTUAL_MASK);
    } else {
        res |= (oo2&OOC_VIRTUAL_MASK);
    }
    return res;
}

SymbolsMenu *createSelectionMenu(ReferenceItem *references) {
    OlcxReferences *rstack;
    Position *defpos;
    unsigned ooBits, oo;
    bool found = false;
    int vlev, vlevel, defusage;

    SymbolsMenu *result = NULL;

    rstack = sessionData.browserStack.top;
    ooBits = 0; vlevel = 0;
    defpos = &noPosition; defusage = UsageNone;

    log_trace("ooBits for '%s'", getFileItem(references->vApplClass)->name);

    for (SymbolsMenu *menu=rstack->hkSelectedSym; menu!=NULL; menu=menu->next) {
        if (olcxIsSameCxSymbol(references, &menu->references)) {
            found = true;
            oo = olcxOoBits(menu, references);
            ooBits = ooBitsMax(oo, ooBits);
            if (defpos->file == NO_FILE_NUMBER) {
                defpos = &menu->defpos;
                defusage = menu->defUsage;
                log_trace(": propagating defpos (line %d) to menusym", defpos->line);
            }
            vlev = 0;
            if (vlevel==0 || ABS(vlevel)>ABS(vlev))
                vlevel = vlev;
            log_trace("ooBits for %s <-> %s %o %o", getFileItem(menu->references.vApplClass)->name, references->linkName, oo, ooBits);
        }
    }
    if (found) {
        int select = 0, visible = 0;  // for debug would be better 1 !
        result = olAddBrowsedSymbolToMenu(&rstack->menuSym, references, select, visible, ooBits, USAGE_ANY, vlevel, defpos,
                                          defusage);
    }
    return result;
}

void mapCreateSelectionMenu(ReferenceItem *p) {
    createSelectionMenu(p);
}


/* ********************************************************************** */

void olSetCallerPosition(Position *pos) {
    assert(sessionData.browserStack.top);
    sessionData.browserStack.top->callerPosition = *pos;
}


void olCompletionListReverse(void) {
    LIST_REVERSE(Completion, sessionData.completionsStack.top->completions);
}


#define maxOf(a, b) (((a) > (b)) ? (a) : (b))

static char *createTagSearchLine_static(char *name, Position *position,
                                        int *len1, int *len2, int *len3) {
    static char line[2*COMPLETION_STRING_SIZE];
    char file[TMP_STRING_SIZE];
    char dir[TMP_STRING_SIZE];
    char *ffname;
    int l1,l2,l3,fl, dl;

    l1 = l2 = l3 = 0;
    l1 = strlen(name);

    FileItem *fileItem = getFileItem(position->file);
    ffname = fileItem->name;
    assert(ffname);
    ffname = getRealFileName_static(ffname);
    fl = strlen(ffname);
    l3 = strmcpy(file,simpleFileName(ffname)) - file;

    dl = fl /*& - l3 &*/ ;
    strncpy(dir, ffname, dl);
    dir[dl]=0;

    *len1 = maxOf(*len1, l1);
    *len2 = maxOf(*len2, l2);
    *len3 = maxOf(*len3, l3);

    if (options.searchKind == SEARCH_DEFINITIONS_SHORT
        || options.searchKind==SEARCH_FULL_SHORT) {
        sprintf(line, "%s", name);
    } else {
        sprintf(line, "%-*s :%-*s :%s", *len1, name, *len3, file, dir);
    }
    return line;                /* static! */
}

void printTagSearchResults(void) {
    int len1, len2, len3, len;
    char *ls;

    len1 = len2 = len3 = 0;
    tagSearchCompactShortResults();

    // the first loop is counting the length of fields
    assert(sessionData.retrieverStack.top);
    for (Completion *cc=sessionData.retrieverStack.top->completions; cc!=NULL; cc=cc->next) {
        ls = createTagSearchLine_static(cc->name, &cc->ref.position,
                                   &len1, &len2, &len3);
    }
    if (options.olineLen >= 50000) {
        /* TODO: WTF? 50k??!?! */
        if (len1 > MAX_TAG_SEARCH_INDENT)
            len1 = MAX_TAG_SEARCH_INDENT;
    } else {
        if (len1 > (options.olineLen*MAX_TAG_SEARCH_INDENT_RATIO)/100) {
            len1 = (options.olineLen*MAX_TAG_SEARCH_INDENT_RATIO)/100;
        }
    }
    len = len1;

    // the second is writing
    if (options.xref2)
        ppcBegin(PPC_SYMBOL_LIST);
    assert(sessionData.retrieverStack.top);
    for (Completion *cc=sessionData.retrieverStack.top->completions; cc!=NULL; cc=cc->next) {
        ls = createTagSearchLine_static(cc->name, &cc->ref.position,
                                   &len1, &len2, &len3);
        if (options.xref2) {
            ppcGenRecord(PPC_STRING_VALUE, ls);
        } else {
            fprintf(communicationChannel,"%s\n", ls);
        }
        len1 = len;
    }
    if (options.xref2)
        ppcEnd(PPC_SYMBOL_LIST);
}

void generateReferences(void) {
    static bool updateFlag = false;  /* TODO: WTF - why do we need a
                                        static updateFlag? Maybe we
                                        need to know that we have
                                        generated from scratch so now
                                        we can just update? */

    if (options.cxrefsLocation == NULL)
        return;
    if (!updateFlag && options.update == UPDATE_DEFAULT) {
        writeReferenceFile(false, options.cxrefsLocation);
        updateFlag = true;
    } else {
        writeReferenceFile(true, options.cxrefsLocation);
    }
}
