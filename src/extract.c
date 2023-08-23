#include "extract.h"

#include "classhierarchy.h"
#include "commons.h"
#include "cxref.h"
#include "filedescriptor.h"
#include "filetable.h"
#include "globals.h"
#include "jsemact.h"
#include "list.h"
#include "log.h"
#include "misc.h"
#include "options.h"
#include "protocol.h"
#include "protocol.h"
#include "reftab.h"
#include "semact.h"


#define EXTRACT_GEN_BUFFER_SIZE 500000

/* ********************** code inspection state bits ********************* */

typedef enum {
    INSPECTION_VISITED = 1,
    INSPECTION_INSIDE_BLOCK = 2,
    INSPECTION_OUTSIDE_BLOCK = 4,
    INSPECTION_INSIDE_REENTER = 8,		/* value reenters the block             */
    INSPECTION_INSIDE_PASSING = 16		/* a non-modified values pass via block */
} InspectionBits;

typedef enum extractClassification {
    EXTRACT_LOCAL_VAR,
    EXTRACT_VALUE_ARGUMENT,
    EXTRACT_LOCAL_OUT_ARGUMENT,
    EXTRACT_OUT_ARGUMENT,
    EXTRACT_IN_OUT_ARGUMENT,
    EXTRACT_ADDRESS_ARGUMENT,
    EXTRACT_RESULT_VALUE,
    EXTRACT_IN_RESULT_VALUE,
    EXTRACT_LOCAL_RESULT_VALUE,
    EXTRACT_NONE,
    EXTRACT_ERROR
} ExtractClassification;

typedef struct programGraphNode {
    struct reference *ref;		/* original reference of node */
    struct referencesItem *symRef;
    struct programGraphNode *jump;
    InspectionBits posBits;               /* INSIDE/OUSIDE block */
    InspectionBits stateBits;             /* visited + where set */
    ExtractClassification classification; /* resulting classification */
    struct programGraphNode *next;
} ProgramGraphNode;


static unsigned s_javaExtractFromFunctionMods=AccessDefault;
static char *resultingString;
static char *s_extractionName;

static void dumpProgram(ProgramGraphNode *program) {
    log_trace("[ProgramDump begin]");
    for (ProgramGraphNode *p=program; p!=NULL; p=p->next) {
        log_trace("%p: %2d %2d %s %s", p,
                p->posBits, p->stateBits,
                p->symRef->linkName,
                usageKindEnumName[p->ref->usage.kind]+5);
        if (p->symRef->type==TypeLabel && p->ref->usage.kind!=UsageDefined) {
            log_trace("    Jump: %p", p->jump);
        }
    }
    log_trace("[ProgramDump end]");
}


void generateInternalLabelReference(int counter, int usage) {
    char labelName[TMP_STRING_SIZE];
    Id labelId;
    Position position;

    if (options.serverOperation != OLO_EXTRACT)
        return;

    snprintf(labelName, TMP_STRING_SIZE, "%%L%d", counter);

    position = (Position){.file = currentFile.characterBuffer.fileNumber, .line = 0, .col = 0};
    fillId(&labelId, labelName, NULL, position);

    if (usage != UsageDefined)
        labelId.position.line++;
    // line == 0 or 1 , (hack to get definition first)
    labelReference(&labelId, usage);
}


Symbol *addContinueBreakLabelSymbol(int labn, char *name) {
    Symbol *s;

    if (options.serverOperation != OLO_EXTRACT)
        return NULL;

    s = newSymbolAsLabel(name, name, noPosition, labn);
    s->type = TypeLabel;
    s->storage = StorageAuto;

    addSymbolNoTrail(symbolTable, s);
    return(s);
}


void deleteContinueBreakLabelSymbol(char *name) {
    Symbol symbol, *foundSymbol;

    if (options.serverOperation != OLO_EXTRACT)
        return;

    fillSymbolWithLabel(&symbol, name, name, noPosition, 0);
    symbol.type = TypeLabel;
    symbol.storage = StorageAuto;

    if (symbolTableIsMember(symbolTable, &symbol, NULL, &foundSymbol)) {
        deleteContinueBreakSymbol(foundSymbol);
    } else {
        assert(0);
    }
}

void genContinueBreakReference(char *name) {
    Symbol symbol, *foundSymbol;

    if (options.serverOperation != OLO_EXTRACT)
        return;

    fillSymbolWithLabel(&symbol, name, name, noPosition, 0);
    symbol.type = TypeLabel;
    symbol.storage = StorageAuto;

    if (symbolTableIsMember(symbolTable, &symbol, NULL, &foundSymbol)) {
        generateInternalLabelReference(foundSymbol->u.labelIndex, UsageUsed);
    }
}

void generateSwitchCaseFork(bool isLast) {
    Symbol symbol, *foundSymbol;

    if (options.serverOperation != OLO_EXTRACT)
        return;

    fillSymbolWithLabel(&symbol, SWITCH_LABEL_NAME, SWITCH_LABEL_NAME, noPosition, 0);
    symbol.type = TypeLabel;
    symbol.storage = StorageAuto;

    if (symbolTableIsMember(symbolTable, &symbol, NULL, &foundSymbol)) {
        generateInternalLabelReference(foundSymbol->u.labelIndex, UsageDefined);
        if (!isLast) {
            foundSymbol->u.labelIndex++;
            generateInternalLabelReference(foundSymbol->u.labelIndex, UsageFork);
        }
    }
}

static ProgramGraphNode *newProgramGraphNode(
    Reference *ref, ReferencesItem *symRef,
    ProgramGraphNode *jump, char posBits,
    char stateBits,
    char classifBits,
    ProgramGraphNode *next
) {
    ProgramGraphNode *programGraph;

    programGraph = cxAlloc(sizeof(ProgramGraphNode));

    programGraph->ref = ref;
    programGraph->symRef = symRef;
    programGraph->jump = jump;
    programGraph->posBits = posBits;
    programGraph->stateBits = stateBits;
    programGraph->classification = classifBits;
    programGraph->next = next;

    return programGraph;
}

static void extractFunGraphRef(ReferencesItem *rr, void *prog) {
    Reference *r;
    ProgramGraphNode *p,**ap;
    ap = (ProgramGraphNode **) prog;
    for (r=rr->references; r!=NULL; r=r->next) {
        if (dm_isBetween(cxMemory,r,parsedClassInfo.cxMemoryIndexAtFunctionBegin,parsedClassInfo.cxMemoryIndexAtFunctionEnd)){
            p = newProgramGraphNode(r, rr, NULL, 0, 0, EXTRACT_NONE, *ap);
            *ap = p;
        }
    }
}

static ProgramGraphNode *getGraphAddress( ProgramGraphNode  *program,
                                            Reference         *ref
                                            ) {
    ProgramGraphNode *p,*res;
    res = NULL;
    for (p=program; res==NULL && p!=NULL; p=p->next) {
        if (p->ref == ref) res = p;
    }
    return res;
}

static Reference *getDefinitionReference(ReferencesItem *lab) {
    Reference *res;
    for (res=lab->references; res!=NULL && res->usage.kind!=UsageDefined; res=res->next) ;
    if (res == NULL) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff,"jump to unknown label '%s'",lab->linkName);
        errorMessage(ERR_ST,tmpBuff);
    }
    return(res);
}

static ProgramGraphNode *getLabelGraphAddress(ProgramGraphNode *program,
                                                ReferencesItem     *lab
                                                ) {
    ProgramGraphNode  *res;
    Reference         *defref;
    assert(lab->type == TypeLabel);
    defref = getDefinitionReference(lab);
    res = getGraphAddress(program, defref);
    return(res);
}

static int linearOrder(ProgramGraphNode *n1, ProgramGraphNode *n2) {
    return(n1->ref < n2->ref);
}

static ProgramGraphNode *extMakeProgramGraph(void) {
    ProgramGraphNode *program, *p;
    program = NULL;
    mapOverReferenceTableWithPointer(extractFunGraphRef, ((void *) &program));
    LIST_SORT(ProgramGraphNode, program, linearOrder);
    dumpProgram(program);
    for (p=program; p!=NULL; p=p->next) {
        if (p->symRef->type==TypeLabel && p->ref->usage.kind!=UsageDefined) {
            // resolve the jump
            p->jump = getLabelGraphAddress(program, p->symRef);
        }
    }
    return(program);
}

static bool isStructOrUnion(ProgramGraphNode *node) {
    return node->symRef->linkName[0]==LINK_NAME_EXTRACT_STR_UNION_TYPE_FLAG;
}

static void extSetSetStates(    ProgramGraphNode *p,
                                ReferencesItem *symRef,
                                unsigned cstate
                                ) {
    unsigned cpos,oldStateBits;
    for (; p!=NULL; p=p->next) {
    cont:
        if (p->stateBits == cstate)
            return;
        oldStateBits = p->stateBits;
        cstate = p->stateBits = (cstate | oldStateBits | INSPECTION_VISITED);
        cpos = p->posBits | INSPECTION_VISITED;
        if (p->symRef == symRef) {          // the examined variable
            if (p->ref->usage.kind == UsageAddrUsed) {
                cstate = cpos;
                // change only state, so usage is kept
            } else if (p->ref->usage.kind == UsageLvalUsed
                       || (p->ref->usage.kind == UsageDefined
                           && ! isStructOrUnion(p))
                       ) {
                // change also current value, because there is no usage
                p->stateBits = cstate = cpos;
            }
        } else if (p->symRef->type==TypeBlockMarker &&
                   (p->posBits&INSPECTION_INSIDE_BLOCK) == 0) {
            // leaving the block
            if (cstate & INSPECTION_INSIDE_BLOCK) {
                // leaving and value is set from inside, preset for possible
                // reentering
                cstate |= INSPECTION_INSIDE_REENTER;
            }
            if (cstate & INSPECTION_OUTSIDE_BLOCK) {
                // leaving and value is set from outside, set value passing flag
                cstate |= INSPECTION_INSIDE_PASSING;
            }
        } else if (p->symRef->type==TypeLabel) {
            if (p->ref->usage.kind==UsageUsed) {  // goto
                p = p->jump;
                goto cont;
            } else if (p->ref->usage.kind==UsageFork) {   // branching
                extSetSetStates(p->jump, symRef, cstate);
            }
        }
    }
}

static ExtractClassification classifyLocalVariableExtraction0(
    ProgramGraphNode *program,
    ProgramGraphNode *varRef
) {
    ProgramGraphNode *p;
    ReferencesItem     *symRef;
    unsigned    inUsages,outUsages,outUsageBothExists;
    symRef = varRef->symRef;
    for (p=program; p!=NULL; p=p->next) {
        p->stateBits = 0;
    }
    //&dumpProgram(program);
    extSetSetStates(program, symRef, INSPECTION_VISITED);
    //&dumpProgram(program);
    inUsages = outUsages = outUsageBothExists = 0;
    for (p=program; p!=NULL; p=p->next) {
        if (p->symRef == varRef->symRef && p->ref->usage.kind != UsageNone) {
            if (p->posBits==INSPECTION_INSIDE_BLOCK) {
                inUsages |= p->stateBits;
            } else if (p->posBits==INSPECTION_OUTSIDE_BLOCK) {
                outUsages |= p->stateBits;
                //&if ( (p->stateBits & INSPECTION_INSIDE_BLOCK)
                //& &&  (p->stateBits & INSPECTION_OUTSIDE_BLOCK)) {
                // a reference outside using values set in both in and out
                //&     outUsageBothExists = 1;
                //&}
            } else assert(0);
        }
    }
    //&fprintf(dumpOut,"%% ** variable '%s' ", varRef->symRef->linkName);fprintf(dumpOut,"in, out Usages %o %o\n",inUsages,outUsages);
    // inUsages marks usages in the block (from inside, or from ouside)
    // outUsages marks usages out of block (from inside, or from ouside)
    if (varRef->posBits == INSPECTION_OUTSIDE_BLOCK) {
        // a variable defined outside of the block
        if (outUsages & INSPECTION_INSIDE_BLOCK) {
            // a value set in the block is used outside
            //&sprintf(tmpBuff,"testing %s: %d %d %d\n", varRef->symRef->linkName, (inUsages & INSPECTION_INSIDE_REENTER),(inUsages & INSPECTION_OUTSIDE_BLOCK), outUsageBothExists);ppcGenRecord(PPC_INFORMATION, tmpBuff);
            if ((inUsages & INSPECTION_INSIDE_REENTER) == 0
                && (inUsages & INSPECTION_OUTSIDE_BLOCK) == 0
                /*& && outUsageBothExists == 0 &*/
                && (outUsages & INSPECTION_INSIDE_PASSING) == 0
                ) {
                return EXTRACT_OUT_ARGUMENT;
            } else {
                return EXTRACT_IN_OUT_ARGUMENT;
            }
        }
        if (inUsages & INSPECTION_INSIDE_REENTER)
            return EXTRACT_IN_OUT_ARGUMENT;
        if (inUsages & INSPECTION_OUTSIDE_BLOCK)
            return EXTRACT_VALUE_ARGUMENT;
        if (inUsages)
            return EXTRACT_LOCAL_VAR;
        return EXTRACT_NONE;
    } else {
        if (    outUsages & INSPECTION_INSIDE_BLOCK
                ||  outUsages & INSPECTION_OUTSIDE_BLOCK) {
            // a variable defined inside the region used outside
            //&fprintf(dumpOut,"%% ** variable '%s' defined inside the region used outside", varRef->symRef->linkName);
            return EXTRACT_LOCAL_OUT_ARGUMENT;
        } else {
            return EXTRACT_NONE;
        }
    }
}

static ExtractClassification categorizeLocalVariableExtraction(
    ProgramGraphNode *program,
    ProgramGraphNode *varRef
) {
    ExtractClassification classification;

    classification = classifyLocalVariableExtraction0(program, varRef);
    if (isStructOrUnion(varRef) && classification!=EXTRACT_NONE && classification!=EXTRACT_LOCAL_VAR) {
        return EXTRACT_ADDRESS_ARGUMENT;
    }
    return classification;
}

static unsigned toogleInOutBlock(unsigned *pos) {
    if (*pos == INSPECTION_INSIDE_BLOCK)
        *pos = INSPECTION_OUTSIDE_BLOCK;
    else if (*pos == INSPECTION_OUTSIDE_BLOCK)
        *pos = INSPECTION_INSIDE_BLOCK;
    else
        assert(0);

    return *pos;
}

static void extSetInOutBlockFields(ProgramGraphNode *program) {
    ProgramGraphNode *p;
    unsigned    pos;
    pos = INSPECTION_OUTSIDE_BLOCK;
    for (p=program; p!=NULL; p=p->next) {
        if (p->symRef->type == TypeBlockMarker) {
            toogleInOutBlock(&pos);
        }
        p->posBits = pos;
    }
}

static int extIsJumpInOutBlock(ProgramGraphNode *program) {
    ProgramGraphNode *p;
    for (p=program; p!=NULL; p=p->next) {
        assert(p->symRef!=NULL)
            if (p->symRef->type==TypeLabel) {
                assert(p->ref!=NULL);
                if (p->ref->usage.kind==UsageUsed || p->ref->usage.kind==UsageFork) {
                    assert(p->jump != NULL);
                    if (p->posBits != p->jump->posBits) {
                        //&fprintf(dumpOut,"jump in/out at %s : %x\n",p->symRef->linkName, p);
                        return(1);
                    }
                }
            }
    }
    return(0);
}

static bool isLocalVariable(ProgramGraphNode *node) {
    return node->ref->usage.kind==UsageDefined
        &&  node->symRef->type==TypeDefault
        &&  node->symRef->scope==ScopeAuto;
}

static void extClassifyLocalVariables(ProgramGraphNode *program) {
    ProgramGraphNode *p;
    for (p=program; p!=NULL; p=p->next) {
        if (isLocalVariable(p)) {
            p->classification = categorizeLocalVariableExtraction(program,p);
        }
    }
}

/* This should really be split into three different functions, one for each result: name, declarator, declaration... */
static void getLocalVarStringFromLinkName(char *linkName, char *name, char *declarator, char *declaration,
                                          char *declStar, bool shouldCopyName) {
    char *src, *nameP, *declarationP, *declaratorP;

    log_trace("%s '%s'", __FUNCTION__, linkName);

    // linkName always starts with a space?
    for (src=linkName+1, declarationP=declaration, declaratorP=declarator;
         *src!=0 && *src!=LINK_NAME_SEPARATOR;
         src++, declarationP++, declaratorP++
    ) {
        *declarationP = *declaratorP = *src;
    }
    *declaratorP = 0;

    assert(*src);
    for (src++; *src!=0 && *src!=LINK_NAME_SEPARATOR; src++, declarationP++) {
        *declarationP = *src;
    }

    assert(*src);
    declarationP = strmcpy(declarationP, declStar);

    for (nameP=name, src++; *src!=0 && *src!=LINK_NAME_SEPARATOR; src++, nameP++) {
        *nameP = *src;
        if (shouldCopyName)
            *declarationP++ = *src;
    }
    assert(*src);
    *nameP = 0;

    assert(*src);
    for (src++; *src!=0 && *src!=LINK_NAME_SEPARATOR; src++, declarationP++) {
        *declarationP = *src;
    }
    *declarationP = 0;
}

static void extReClassifyIOVars(ProgramGraphNode *program) {
    ProgramGraphNode *op = NULL;
    bool uniqueOutFlag = true;

    for (ProgramGraphNode *p=program; p!=NULL; p=p->next) {
        if (options.extractMode == EXTRACT_FUNCTION_ADDRESS_ARGS) {
            if (p->classification == EXTRACT_OUT_ARGUMENT
                ||  p->classification == EXTRACT_LOCAL_OUT_ARGUMENT
                ||  p->classification == EXTRACT_IN_OUT_ARGUMENT
                ) {
                p->classification = EXTRACT_ADDRESS_ARGUMENT;
            }
        } else if (options.extractMode == EXTRACT_FUNCTION) {
            if (p->classification == EXTRACT_OUT_ARGUMENT
                || p->classification == EXTRACT_LOCAL_OUT_ARGUMENT
                ) {
                if (op == NULL)
                    op = p;
                else
                    uniqueOutFlag = false;
                // re-classify to in_out
            }
        }
    }

    if (op!=NULL && uniqueOutFlag) {
        if (op->classification == EXTRACT_LOCAL_OUT_ARGUMENT) {
            op->classification = EXTRACT_LOCAL_RESULT_VALUE;
        } else {
            op->classification = EXTRACT_RESULT_VALUE;
        }
        return;
    }

    op = NULL;
    uniqueOutFlag = true;
    for (ProgramGraphNode *p=program; p!=NULL; p=p->next) {
        if (options.extractMode == EXTRACT_FUNCTION) {
            if (p->classification == EXTRACT_IN_OUT_ARGUMENT) {
                if (op == NULL)
                    op = p;
                else
                    uniqueOutFlag = false;
                // re-classify to in_out
            }
        }
    }

    if (op!=NULL && uniqueOutFlag) {
        op->classification = EXTRACT_IN_RESULT_VALUE;
    }
}

/* ************************** macro ******************************* */

static void generateNewMacroCall(ProgramGraphNode *program) {
    char declarator[TMP_STRING_SIZE];
    char declaration[TMP_STRING_SIZE];

    char name[TMP_STRING_SIZE];
    ProgramGraphNode *p;
    bool isFirstArgument = true;

    resultingString[0]=0;

    sprintf(resultingString+strlen(resultingString),"\t%s",s_extractionName);

    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_VALUE_ARGUMENT
            ||  p->classification == EXTRACT_IN_OUT_ARGUMENT
            ||  p->classification == EXTRACT_ADDRESS_ARGUMENT
            ||  p->classification == EXTRACT_OUT_ARGUMENT
            ||  p->classification == EXTRACT_LOCAL_OUT_ARGUMENT
            ||  p->classification == EXTRACT_RESULT_VALUE
            ||  p->classification == EXTRACT_IN_RESULT_VALUE
            ||  p->classification == EXTRACT_LOCAL_VAR
        ) {
            getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, /* unused */declaration, "", true);
            sprintf(resultingString+strlen(resultingString), "%s%s", isFirstArgument?"(":", " , name);
            isFirstArgument = false;
        }
    }
    sprintf(resultingString+strlen(resultingString), "%s);\n", isFirstArgument?"(":"");

    assert(strlen(resultingString)<EXTRACT_GEN_BUFFER_SIZE-1);
    if (options.xref2) {
        ppcGenRecord(PPC_STRING_VALUE, resultingString);
    } else {
        fprintf(communicationChannel, "%s", resultingString);
    }
}

static void extGenNewMacroHead(ProgramGraphNode *program) {
    char declarator[TMP_STRING_SIZE];
    char declaration[MAX_EXTRACT_FUN_HEAD_SIZE];
    char name[MAX_EXTRACT_FUN_HEAD_SIZE];
    ProgramGraphNode *p;
    bool isFirstArgument = true;

    resultingString[0]=0;

    sprintf(resultingString+strlen(resultingString),"#define %s",s_extractionName);
    for (p=program; p!=NULL; p=p->next) {
        if (    p->classification == EXTRACT_VALUE_ARGUMENT
                ||  p->classification == EXTRACT_IN_OUT_ARGUMENT
                ||  p->classification == EXTRACT_OUT_ARGUMENT
                ||  p->classification == EXTRACT_LOCAL_OUT_ARGUMENT
                ||  p->classification == EXTRACT_ADDRESS_ARGUMENT
                ||  p->classification == EXTRACT_RESULT_VALUE
                ||  p->classification == EXTRACT_IN_RESULT_VALUE
                ||  p->classification == EXTRACT_LOCAL_VAR) {
            getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, /* unused */declaration, "", true);
            sprintf(resultingString+strlen(resultingString), "%s%s", isFirstArgument?"(":"," , name);
            isFirstArgument = false;
        }
    }
    sprintf(resultingString+strlen(resultingString), "%s) {\\\n", isFirstArgument?"(":"");
    assert(strlen(resultingString)<EXTRACT_GEN_BUFFER_SIZE-1);
    if (options.xref2) {
        ppcGenRecord(PPC_STRING_VALUE, resultingString);
    } else {
        fprintf(communicationChannel, "%s", resultingString);
    }
}

static void generateNewMacroTail() {
    resultingString[0]=0;

    sprintf(resultingString+strlen(resultingString),"}\n\n");

    assert(strlen(resultingString)<EXTRACT_GEN_BUFFER_SIZE-1);
    if (options.xref2) {
        ppcGenRecord(PPC_STRING_VALUE, resultingString);
    } else {
        fprintf(communicationChannel, "%s", resultingString);
    }
}



/* ********************** C function **************************** */


static void generateNewFunctionCall(ProgramGraphNode *program) {
    char declarator[TMP_STRING_SIZE];
    char declaration[TMP_STRING_SIZE];
    char name[TMP_STRING_SIZE];
    ProgramGraphNode *p;
    bool isFirstArgument = true;

    resultingString[0]=0;

    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_RESULT_VALUE
            || p->classification == EXTRACT_LOCAL_RESULT_VALUE
            || p->classification == EXTRACT_IN_RESULT_VALUE
            ) break;
    }
    if (p!=NULL) {
        getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, /* unused */declaration, "", true);
        if (p->classification == EXTRACT_LOCAL_RESULT_VALUE) {
            sprintf(resultingString+strlen(resultingString),"\t%s = ", declaration);
        } else {
            sprintf(resultingString+strlen(resultingString),"\t%s = ", name);
        }
    } else {
        sprintf(resultingString+strlen(resultingString),"\t");
    }
    sprintf(resultingString+strlen(resultingString),"%s",s_extractionName);

    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_VALUE_ARGUMENT
            || p->classification == EXTRACT_IN_RESULT_VALUE) {
            getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, /* unused */declaration, "", true);
            sprintf(resultingString+strlen(resultingString), "%s%s", isFirstArgument?"(":", " , name);
            isFirstArgument = false;
        }
    }
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_IN_OUT_ARGUMENT
            || p->classification == EXTRACT_ADDRESS_ARGUMENT
            || p->classification == EXTRACT_OUT_ARGUMENT
            || p->classification == EXTRACT_LOCAL_OUT_ARGUMENT
        ) {
            getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, /* unused */declaration, options.olExtractAddrParPrefix, true);
            sprintf(resultingString+strlen(resultingString), "%s&%s", isFirstArgument?"(":", " , name);
            isFirstArgument = false;
        }
    }
    sprintf(resultingString+strlen(resultingString), "%s);\n", isFirstArgument?"(":"");
    assert(strlen(resultingString)<EXTRACT_GEN_BUFFER_SIZE-1);
    if (options.xref2) {
        ppcGenRecord(PPC_STRING_VALUE, resultingString);
    } else {
        fprintf(communicationChannel, "%s", resultingString);
    }
}

static void removeSymbolFromSymRefList(ReferencesItemList **ll, ReferencesItem *s) {
    ReferencesItemList **r;
    r=ll;
    while (*r!=NULL) {
        if (isSmallerOrEqClass(getClassNumFromClassLinkName((*r)->item->linkName, NO_FILE_NUMBER),
                               getClassNumFromClassLinkName(s->linkName, NO_FILE_NUMBER))) {
            *r = (*r)->next;
        } else {
            r= &(*r)->next;
        }
    }
}

static ReferencesItemList *concatRefItemList(ReferencesItemList **list, ReferencesItem *items) {
    ReferencesItemList *refItemList = cxAlloc(sizeof(ReferencesItemList));
    refItemList->item = items;
    refItemList->next = *list;
    return refItemList;
}


/* Non-static for unittesting */
void addSymbolToSymRefList(ReferencesItemList **list, ReferencesItem *items) {
    ReferencesItemList *l;

    l = *list;
    while (l!=NULL) {
        if (isSmallerOrEqClass(getClassNumFromClassLinkName(items->linkName, NO_FILE_NUMBER),
                               getClassNumFromClassLinkName(l->item->linkName, NO_FILE_NUMBER))) {
            return;
        }
        l= l->next;
    }
    // first remove all superceeded by this one
    /* TODO: WTF, what does superceed mean here and why should we remove them? */
    removeSymbolFromSymRefList(list, items);
    *list = concatRefItemList(list, items);
}

static ReferencesItemList *computeExceptionsThrownBetween(ProgramGraphNode *bb,
                                                           ProgramGraphNode *ee
                                                           ) {
    ReferencesItemList *res, *excs, *catched, *noncatched, **cl;
    ProgramGraphNode *p, *e;
    int depth;

    catched = NULL; noncatched = NULL; cl = &catched;
    for (p=bb; p!=NULL && p!=ee; p=p->next) {
        if (p->symRef->type == TypeTryCatchMarker && p->ref->usage.kind == UsageTryCatchBegin) {
            depth = 0;
            for (e=p; e!=NULL && e!=ee; e=e->next) {
                if (e->symRef->type == TypeTryCatchMarker) {
                    if (e->ref->usage.kind == UsageTryCatchBegin) depth++;
                    else depth --;
                    if (depth == 0) break;
                }
            }
            if (depth==0) {
                // add exceptions thrown in block
                for (excs=computeExceptionsThrownBetween(p->next,e); excs!=NULL; excs=excs->next){
                    addSymbolToSymRefList(cl, excs->item);
                }
            }
        }
        if (p->ref->usage.kind == UsageThrown) {
            // thrown exception add it to list
            addSymbolToSymRefList(cl, p->symRef);
        } else if (p->ref->usage.kind == UsageCatched) {
            // catched, remove it from list
            removeSymbolFromSymRefList(&catched, p->symRef);
            cl = &noncatched;
        }
    }
    res = catched;
    for (excs=noncatched; excs!=NULL; excs=excs->next) {
        addSymbolToSymRefList(&res, excs->item);
    }
    return(res);
}

static ReferencesItemList *computeExceptionsThrownInBlock(ProgramGraphNode *program) {
    ProgramGraphNode  *pp, *ee;
    for (pp=program; pp!=NULL && pp->symRef->type!=TypeBlockMarker; pp=pp->next) ;
    if (pp==NULL)
        return(NULL);
    for (ee=pp->next; ee!=NULL && ee->symRef->type!=TypeBlockMarker; ee=ee->next) ;
    return(computeExceptionsThrownBetween(pp, ee));
}

static void extractSprintThrownExceptions(char *nhead, ProgramGraphNode *program) {
    ReferencesItemList     *exceptions, *ee;
    int                     nhi;
    char                    *sname;
    nhi = 0;
    exceptions = computeExceptionsThrownInBlock(program);
    if (exceptions!=NULL) {
        sprintf(nhead+nhi, " throws");
        nhi += strlen(nhead+nhi);
        for (ee=exceptions; ee!=NULL; ee=ee->next) {
            sname = lastOccurenceInString(ee->item->linkName, '$');
            if (sname==NULL)
                sname = lastOccurenceInString(ee->item->linkName, '/');
            if (sname==NULL) {
                sname = ee->item->linkName;
            } else {
                sname ++;
            }
            sprintf(nhead+nhi, "%s%s", ee==exceptions?" ":", ", sname);
            nhi += strlen(nhead+nhi);
        }
    }
}

static void generateNewFunctionHead(ProgramGraphNode *program) {
    char nhead[MAX_EXTRACT_FUN_HEAD_SIZE+2];
    int nhi, ldclaLen;
    char ldcla[TMP_STRING_SIZE];
    char declarator[TMP_STRING_SIZE];
    char declaration[MAX_EXTRACT_FUN_HEAD_SIZE];
    char name[MAX_EXTRACT_FUN_HEAD_SIZE];
    ProgramGraphNode  *p;
    int isFirstArgument = true;

    resultingString[0]=0;

    /* function header */
    nhi = 0;
    sprintf(nhead+nhi,"%s",s_extractionName);
    nhi += strlen(nhead+nhi);
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_VALUE_ARGUMENT
            || p->classification == EXTRACT_IN_RESULT_VALUE
        ) {
            getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, declaration, "", true);
            sprintf(nhead+nhi, "%s%s", isFirstArgument?"(":", " , declaration);
            nhi += strlen(nhead+nhi);
            isFirstArgument = false;
        }
    }
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_IN_OUT_ARGUMENT
            ||  p->classification == EXTRACT_OUT_ARGUMENT
            ||  p->classification == EXTRACT_LOCAL_OUT_ARGUMENT
        ) {
            getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, declaration, options.olExtractAddrParPrefix, true);
            sprintf(nhead+nhi, "%s%s", isFirstArgument?"(":", " , declaration);
            nhi += strlen(nhead+nhi);
            isFirstArgument = false;
        }
    }
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_ADDRESS_ARGUMENT) {
            getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, declaration, EXTRACT_REFERENCE_ARG_STRING, true);
            sprintf(nhead+nhi, "%s%s", isFirstArgument?"(":", " , declaration);
            nhi += strlen(nhead+nhi);
            isFirstArgument = false;
        }
    }
    sprintf(nhead+nhi, "%s)", isFirstArgument?"(":"");
    nhi += strlen(nhead+nhi);

    //throws
    extractSprintThrownExceptions(nhead+nhi, program);
    nhi += strlen(nhead+nhi);

    if (LANGUAGE(LANG_JAVA)) {
        // this makes renaming after extraction much faster
        sprintf(resultingString+strlen(resultingString), "private ");
    }

    if (LANGUAGE(LANG_JAVA) && (s_javaExtractFromFunctionMods&AccessStatic)==0) {
        ; // sprintf(rb+strlen(rb), "");
    } else {
        sprintf(resultingString+strlen(resultingString), "static ");
    }
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_RESULT_VALUE
            || p->classification == EXTRACT_LOCAL_RESULT_VALUE
            || p->classification == EXTRACT_IN_RESULT_VALUE)
            break;
    }
    if (p==NULL) {
        sprintf(resultingString+strlen(resultingString),"void %s",nhead);
    } else {
        getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, declaration, nhead, false);
        sprintf(resultingString+strlen(resultingString), "%s", declaration);
    }

    /* function body */
    sprintf(resultingString+strlen(resultingString), " {\n");
    ldcla[0] = 0;
    ldclaLen = 0;
    isFirstArgument = true;
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_IN_OUT_ARGUMENT
            || p->classification == EXTRACT_LOCAL_VAR
            || p->classification == EXTRACT_OUT_ARGUMENT
            || p->classification == EXTRACT_RESULT_VALUE
            || (p->symRef->storage == StorageExtern
                && p->ref->usage.kind == UsageDeclared)
        ) {
            getLocalVarStringFromLinkName(p->symRef->linkName, name, declarator, declaration, "", true);
            if (strcmp(ldcla,declarator)==0) {
                sprintf(resultingString+strlen(resultingString), ",%s",declaration+ldclaLen);
            } else {
                strcpy(ldcla,declarator); ldclaLen=strlen(ldcla);
                if (isFirstArgument)
                    sprintf(resultingString+strlen(resultingString), "\t%s",declaration);
                else
                    sprintf(resultingString+strlen(resultingString), ";\n\t%s",declaration);
            }
            if (p->classification == EXTRACT_IN_OUT_ARGUMENT) {
                sprintf(resultingString+strlen(resultingString), " = %s%s", options.olExtractAddrParPrefix, name);
            }
            isFirstArgument = false;
        }
    }
    if (!isFirstArgument)
        sprintf(resultingString+strlen(resultingString), ";\n");
    assert(strlen(resultingString)<EXTRACT_GEN_BUFFER_SIZE-1);
    if (options.xref2) {
        ppcGenRecord(PPC_STRING_VALUE, resultingString);
    } else {
        fprintf(communicationChannel, "%s", resultingString);
    }
}

static void generateNewFunctionTail(ProgramGraphNode *program) {
    char                declarator[TMP_STRING_SIZE];
    char                declaration[TMP_STRING_SIZE];
    char                name[TMP_STRING_SIZE];
    ProgramGraphNode  *p;

    resultingString[0]=0;

    for (p=program; p!=NULL; p=p->next) {
        if (    p->classification == EXTRACT_IN_OUT_ARGUMENT
                ||  p->classification == EXTRACT_OUT_ARGUMENT
                ||  p->classification == EXTRACT_LOCAL_OUT_ARGUMENT
                ) {
            getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, /* unused */declaration, "", true);
            sprintf(resultingString+strlen(resultingString), "\t%s%s = %s;\n", options.olExtractAddrParPrefix,
                    name, name);
        }
    }
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_RESULT_VALUE
            || p->classification == EXTRACT_IN_RESULT_VALUE
            || p->classification == EXTRACT_LOCAL_RESULT_VALUE
            ) {
            getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, /* unused */declaration, "", true);
            sprintf(resultingString+strlen(resultingString), "\n\treturn %s;\n", name);
        }
    }
    sprintf(resultingString+strlen(resultingString),"}\n\n");
    assert(strlen(resultingString)<EXTRACT_GEN_BUFFER_SIZE-1);
    if (options.xref2) {
        ppcGenRecord(PPC_STRING_VALUE, resultingString);
    } else {
        fprintf(communicationChannel, "%s", resultingString);
    }
}


/* ********************** java function **************************** */


static char * makeNewClassName(void) {
    static char classname[TMP_STRING_SIZE];
    if (s_extractionName[0]==0) {
        sprintf(classname,"NewClass");
    } else {
        sprintf(classname, "%c%s", toupper(s_extractionName[0]),
                s_extractionName+1);
    }
    return(classname);
}


static void extJavaGenNewClassCall(ProgramGraphNode *program) {
    char declarator[TMP_STRING_SIZE];
    char declaration[TMP_STRING_SIZE];
    char name[TMP_STRING_SIZE];
    ProgramGraphNode *p;
    char *classname;
    bool isFirstArgument = true;

    resultingString[0]=0;

    classname = makeNewClassName();

    // constructor invocation
    sprintf(resultingString+strlen(resultingString),"\t\t%s %s = new %s",classname, s_extractionName,classname);
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_IN_OUT_ARGUMENT) {
            getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, /* unused */declaration, "", true);
            sprintf(resultingString+strlen(resultingString), "%s%s", isFirstArgument?"(":", " , name);
            isFirstArgument = false;
        }
    }
    sprintf(resultingString+strlen(resultingString), "%s);\n", isFirstArgument?"(":"");

    // "perform" invocation
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_RESULT_VALUE
            || p->classification == EXTRACT_IN_RESULT_VALUE
            || p->classification == EXTRACT_LOCAL_RESULT_VALUE
            ) break;
    }
    if (p!=NULL) {
        getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, declaration, "", true);
        if (p->classification == EXTRACT_LOCAL_RESULT_VALUE) {
            sprintf(resultingString+strlen(resultingString),"\t\t%s = ", declaration);
        } else {
            sprintf(resultingString+strlen(resultingString),"\t\t%s = ", name);
        }
    } else {
        sprintf(resultingString+strlen(resultingString),"\t\t");
    }
    sprintf(resultingString+strlen(resultingString),"%s.perform",s_extractionName);

    isFirstArgument = true;
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_VALUE_ARGUMENT
            || p->classification == EXTRACT_IN_RESULT_VALUE
        ) {
            getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, /* unused */declaration, "", true);
            sprintf(resultingString+strlen(resultingString), "%s%s", isFirstArgument?"(":", " , name);
            isFirstArgument = false;
        }
    }
    sprintf(resultingString+strlen(resultingString), "%s);\n", isFirstArgument?"(":"");

    sprintf(resultingString+strlen(resultingString), "\t\t");
    // 'out' arguments value recovering
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_IN_OUT_ARGUMENT
            ||  p->classification == EXTRACT_OUT_ARGUMENT
            ||  p->classification == EXTRACT_LOCAL_OUT_ARGUMENT
        ) {
            getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, declaration, "", true);
            if (p->classification == EXTRACT_LOCAL_OUT_ARGUMENT) {
                sprintf(resultingString+strlen(resultingString), "%s=%s.%s; ", declaration, s_extractionName, name);
            } else {
                sprintf(resultingString+strlen(resultingString), "%s=%s.%s; ", name, s_extractionName, name);
            }
            isFirstArgument = false;
        }
    }
    sprintf(resultingString+strlen(resultingString), "\n");
    sprintf(resultingString+strlen(resultingString),"\t\t%s = null;\n", s_extractionName);

    assert(strlen(resultingString)<EXTRACT_GEN_BUFFER_SIZE-1);
    if (options.xref2) {
        ppcGenRecord(PPC_STRING_VALUE, resultingString);
    } else {
        fprintf(communicationChannel, "%s", resultingString);
    }
}

static void extJavaGenNewClassHead(ProgramGraphNode *program) {
    char nhead[MAX_EXTRACT_FUN_HEAD_SIZE+2];
    int nhi, ldclaLen;
    char ldcla[TMP_STRING_SIZE];
    char declarator[TMP_STRING_SIZE];
    char *classname;
    char declaration[MAX_EXTRACT_FUN_HEAD_SIZE];
    char name[MAX_EXTRACT_FUN_HEAD_SIZE];
    ProgramGraphNode *p;
    bool isFirstArgument = true;

    resultingString[0]=0;

    classname = makeNewClassName();

    // class header
    sprintf(resultingString+strlen(resultingString), "\t");
    if (s_javaExtractFromFunctionMods & AccessStatic){
        sprintf(resultingString+strlen(resultingString), "static ");
    }
    sprintf(resultingString+strlen(resultingString), "class %s {\n", classname);
    //sprintf(rb+strlen(rb), "\t\t// %s 'out' arguments\n", s_opt.extractName);
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_OUT_ARGUMENT
            || p->classification == EXTRACT_LOCAL_OUT_ARGUMENT
            || p->classification == EXTRACT_IN_OUT_ARGUMENT
            ) {
            getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, declaration, "", true);
            sprintf(resultingString+strlen(resultingString), "\t\t%s;\n", declaration);
        }
    }

    // the constructor
    //sprintf(rb+strlen(rb),"\t\t// constructor for %s 'in-out' args\n",s_opt.extractName);
    sprintf(resultingString+strlen(resultingString), "\t\t%s", classname);
    isFirstArgument = true;
    for (p=program; p!=NULL; p=p->next) {
        if (    p->classification == EXTRACT_IN_OUT_ARGUMENT){
            getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, declaration, "", true);
            sprintf(resultingString+strlen(resultingString), "%s%s", isFirstArgument?"(":", " , declaration);
            isFirstArgument = false;
        }
    }
    sprintf(resultingString+strlen(resultingString), "%s) {", isFirstArgument?"(":"");

    isFirstArgument = true;
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_IN_OUT_ARGUMENT) {
            getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, /* unused */declaration, "", true);
            if (isFirstArgument)
                sprintf(resultingString+strlen(resultingString), "\n\t\t\t");
            sprintf(resultingString+strlen(resultingString), "this.%s = %s; ", name, name);
            isFirstArgument = false;
        }
    }
    if (!isFirstArgument)
        sprintf(resultingString+strlen(resultingString),"\n\t\t");
    sprintf(resultingString+strlen(resultingString),"}\n");
    //sprintf(rb+strlen(rb),"\t\t// perform with %s 'in' args\n",s_opt.extractName);

    // the "perform" method
    nhi = 0;
    isFirstArgument = true;
    sprintf(nhead+nhi,"%s", "perform");
    nhi += strlen(nhead+nhi);
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_VALUE_ARGUMENT
            || p->classification == EXTRACT_IN_RESULT_VALUE
        ) {
            getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, declaration, "", true);
            sprintf(nhead+nhi, "%s%s", isFirstArgument?"(":", " , declaration);
            nhi += strlen(nhead+nhi);
            isFirstArgument = false;
        }
    }
    sprintf(nhead+nhi, "%s)", isFirstArgument?"(":"");
    nhi += strlen(nhead+nhi);

    // throws
    extractSprintThrownExceptions(nhead+nhi, program);
    nhi += strlen(nhead+nhi);

    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_RESULT_VALUE
            || p->classification == EXTRACT_IN_RESULT_VALUE) break;
    }
    if (p==NULL) {
        sprintf(resultingString+strlen(resultingString),"\t\tvoid %s", nhead);
    } else {
        getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, declaration, nhead, false);
        sprintf(resultingString+strlen(resultingString), "\t\t%s", declaration);
    }

    /* function body */
    sprintf(resultingString+strlen(resultingString), " {\n");
    ldcla[0] = 0;
    ldclaLen = 0;
    isFirstArgument = true;
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_LOCAL_VAR
            || p->classification == EXTRACT_RESULT_VALUE
            || (p->symRef->storage == StorageExtern
                && p->ref->usage.kind == UsageDeclared)
        ) {
            getLocalVarStringFromLinkName(p->symRef->linkName, name, declarator, declaration, "", true);
            if (strcmp(ldcla,declarator)==0) {
                sprintf(resultingString+strlen(resultingString), ",%s",declaration+ldclaLen);
            } else {
                strcpy(ldcla,declarator); ldclaLen=strlen(ldcla);
                if (isFirstArgument) sprintf(resultingString+strlen(resultingString), "\t\t\t%s",declaration);
                else sprintf(resultingString+strlen(resultingString), ";\n\t\t\t%s",declaration);
            }
            isFirstArgument = false;
        }
    }
    if (!isFirstArgument)
        sprintf(resultingString+strlen(resultingString), ";\n");
    assert(strlen(resultingString)<EXTRACT_GEN_BUFFER_SIZE-1);
    if (options.xref2) {
        ppcGenRecord(PPC_STRING_VALUE, resultingString);
    } else {
        fprintf(communicationChannel, "%s", resultingString);
    }
}

static void extJavaGenNewClassTail(ProgramGraphNode *program) {
    char declarator[TMP_STRING_SIZE]; /* Unused */
    char declaration[TMP_STRING_SIZE]; /* Unused */

    char name[TMP_STRING_SIZE];
    ProgramGraphNode  *p;
    bool isFirstArgument = true;

    resultingString[0]=0;

    // local 'out' arguments value setting
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_LOCAL_OUT_ARGUMENT) {
            getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, /* unused */declaration, "", true);
            if (isFirstArgument)
                sprintf(resultingString+strlen(resultingString), "\t\t\t");
            sprintf(resultingString+strlen(resultingString), "this.%s=%s; ", name, name);
            isFirstArgument = false;
        }
    }
    if (!isFirstArgument)
        sprintf(resultingString+strlen(resultingString), "\n");

    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_RESULT_VALUE
            || p->classification == EXTRACT_LOCAL_RESULT_VALUE
            || p->classification == EXTRACT_IN_RESULT_VALUE
        ) {
            getLocalVarStringFromLinkName(p->symRef->linkName, name, /* unused */declarator, /* unused */declaration, "", true);
            sprintf(resultingString+strlen(resultingString), "\t\t\treturn(%s);\n", name);
        }
    }
    sprintf(resultingString+strlen(resultingString),"\t\t}\n\t}\n\n");

    assert(strlen(resultingString)<EXTRACT_GEN_BUFFER_SIZE-1);
    if (options.xref2) {
        ppcGenRecord(PPC_STRING_VALUE, resultingString);
    } else {
        fprintf(communicationChannel, "%s", resultingString);
    }
}

/* ******************************************************************* */

static bool extractJavaIsNewClassNecessary(ProgramGraphNode *program) {
    ProgramGraphNode  *p;
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == EXTRACT_OUT_ARGUMENT
            || p->classification == EXTRACT_LOCAL_OUT_ARGUMENT
            || p->classification == EXTRACT_IN_OUT_ARGUMENT
            ) break;
    }
    if (p==NULL)
        return false;
    return true;
}

static void makeExtraction(void) {
    ProgramGraphNode *program;
    bool needToExtractNewClass = false;

    if (parsedClassInfo.cxMemoryIndexAtFunctionBegin > parsedInfo.cxMemoryIndexAtBlockBegin
        || parsedInfo.cxMemoryIndexAtBlockBegin > parsedInfo.cxMemoryIndexAtBlockEnd
        || parsedInfo.cxMemoryIndexAtBlockEnd > parsedClassInfo.cxMemoryIndexAtFunctionEnd
        || parsedInfo.workMemoryIndexAtBlockBegin != parsedInfo.workMemoryIndexAtBlockEnd) {
        errorMessage(ERR_ST, "Region / program structure mismatch");
        return;
    }
    log_trace("!cxMemories: funBeg, blockBeb, blockEnd, funEnd: %x, %x, %x, %x", parsedClassInfo.cxMemoryIndexAtFunctionBegin, parsedInfo.cxMemoryIndexAtBlockBegin, parsedInfo.cxMemoryIndexAtBlockEnd, parsedClassInfo.cxMemoryIndexAtFunctionEnd);
    assert(parsedClassInfo.cxMemoryIndexAtFunctionBegin);
    assert(parsedInfo.cxMemoryIndexAtBlockBegin);
    assert(parsedInfo.cxMemoryIndexAtBlockEnd);
    assert(parsedClassInfo.cxMemoryIndexAtFunctionEnd);

    program = extMakeProgramGraph();
    extSetInOutBlockFields(program);
    //&dumpProgram(program);

    if (options.extractMode!=EXTRACT_MACRO && extIsJumpInOutBlock(program)) {
        errorMessage(ERR_ST, "There are jumps in or out of region");
        return;
    }
    extClassifyLocalVariables(program);
    extReClassifyIOVars(program);

    if (LANGUAGE(LANG_JAVA))
        needToExtractNewClass = extractJavaIsNewClassNecessary(program);

    if (LANGUAGE(LANG_JAVA)) {
        if (needToExtractNewClass) s_extractionName = "newClass_";
        else s_extractionName = "newMethod_";
    } else {
        if (options.extractMode==EXTRACT_MACRO) s_extractionName = "NEW_MACRO_";
        else s_extractionName = "newFunction_";
    }

    if (options.xref2)
        ppcBeginWithStringAttribute(PPC_EXTRACTION_DIALOG, PPCA_TYPE, s_extractionName);

    resultingString = cxAlloc(EXTRACT_GEN_BUFFER_SIZE);

    if (!options.xref2) {
        fprintf(communicationChannel,
                "%%!\n------------------------ The Invocation ------------------------\n!\n");
    }
    if (options.extractMode==EXTRACT_MACRO)
        generateNewMacroCall(program);
    else if (needToExtractNewClass)
        extJavaGenNewClassCall(program);
    else
        generateNewFunctionCall(program);

    if (!options.xref2) {
        fprintf(communicationChannel,
                "!\n--------------------------- The Head ---------------------------\n!\n");
    }

    if (options.extractMode==EXTRACT_MACRO)
        extGenNewMacroHead(program);
    else if (needToExtractNewClass)
        extJavaGenNewClassHead(program);
    else
        generateNewFunctionHead(program);

    if (!options.xref2) {
        fprintf(communicationChannel,
                "!\n--------------------------- The Tail ---------------------------\n!\n");
    }

    if (options.extractMode==EXTRACT_MACRO)
        generateNewMacroTail();
    else if (needToExtractNewClass)
        extJavaGenNewClassTail(program);
    else
        generateNewFunctionTail(program);

    if (options.xref2) {
        ppcValueRecord(PPC_INT_VALUE, parsedClassInfo.functionBeginPosition, "");
    } else {
        fprintf(communicationChannel,"!%d!\n", parsedClassInfo.functionBeginPosition);
        fflush(communicationChannel);
    }

    if (options.xref2)
        ppcEnd(PPC_EXTRACTION_DIALOG);
}


void actionsBeforeAfterExternalDefinition(void) {
    if (parsedInfo.cxMemoryIndexAtBlockEnd != 0
        // you have to check for matching class method
        // i.e. for case 'void mmm() { //blockbeg; ...; //blockend; class X { mmm(){}!!}; }'
        && parsedClassInfo.cxMemoryIndexAtFunctionBegin != 0
        && parsedClassInfo.cxMemoryIndexAtFunctionBegin <= parsedInfo.cxMemoryIndexAtBlockBegin
        // is it an extraction action ?
        && options.serverOperation == OLO_EXTRACT
        && (! parsedInfo.extractProcessedFlag))
    {
        // O.K. make extraction
        parsedClassInfo.cxMemoryIndexAtFunctionEnd = cxMemory->index;
        makeExtraction();
        parsedInfo.extractProcessedFlag = true;
        /* here should be a longjmp to stop file processing !!!! */
        /* No, all this extraction should be after parsing ! */
    }
    parsedClassInfo.cxMemoryIndexAtFunctionBegin = cxMemory->index;
    if (includeStack.pointer > 0) {
        parsedClassInfo.functionBeginPosition = includeStack.stack[0].lineNumber+1;
    } else {
        parsedClassInfo.functionBeginPosition = currentFile.lineNumber+1;
    }
}


void extractActionOnBlockMarker(void) {
    Position pos;
    if (parsedInfo.cxMemoryIndexAtBlockBegin == 0) {
        parsedInfo.cxMemoryIndexAtBlockBegin = cxMemory->index;
        parsedInfo.workMemoryIndexAtBlockBegin = currentBlock->outerBlock;
        if (LANGUAGE(LANG_JAVA)) {
            s_javaExtractFromFunctionMods = javaStat->methodModifiers;
        }
    } else {
        assert(parsedInfo.cxMemoryIndexAtBlockEnd == 0);
        parsedInfo.cxMemoryIndexAtBlockEnd = cxMemory->index;
        parsedInfo.workMemoryIndexAtBlockEnd = currentBlock->outerBlock;
    }
    pos = makePosition(currentFile.characterBuffer.fileNumber, 0, 0);
    addTrivialCxReference("Block", TypeBlockMarker, StorageDefault, pos, UsageUsed);
}

void deleteContinueBreakSymbol(Symbol *symbol) {
    if (options.serverOperation == OLO_EXTRACT)
        deleteSymDef(symbol);
}

int nextGeneratedLocalSymbol(void) {
    return counters.localSym++;
}

int nextGeneratedLabelSymbol(void) {
    int n = counters.localSym;
    generateInternalLabelReference(counters.localSym, UsageDefined);
    counters.localSym++;
    return n;
}

int nextGeneratedGotoSymbol(void) {
    int n = counters.localSym;
    generateInternalLabelReference(counters.localSym, UsageUsed);
    counters.localSym++;
    return n;
}

int nextGeneratedForkSymbol(void) {
    int n = counters.localSym;
    generateInternalLabelReference(counters.localSym, UsageFork);
    counters.localSym++;
    return n;
}
