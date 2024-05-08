#include "extract.h"

#include "classhierarchy.h"
#include "commons.h"
#include "cxref.h"
#include "filedescriptor.h"
#include "filetable.h"
#include "globals.h"
#include "list.h"
#include "log.h"
#include "misc.h"
#include "options.h"
#include "protocol.h"
#include "ppc.h"
#include "protocol.h"
#include "reftab.h"
#include "semact.h"
#include "stackmemory.h"


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
    CLASSIFIED_AS_LOCAL_VAR,
    CLASSIFIED_AS_VALUE_ARGUMENT,
    CLASSIFIED_AS_LOCAL_OUT_ARGUMENT,
    CLASSIFIED_AS_OUT_ARGUMENT,
    CLASSIFIED_AS_IN_OUT_ARGUMENT,
    CLASSIFIED_AS_ADDRESS_ARGUMENT,
    CLASSIFIED_AS_RESULT_VALUE,
    CLASSIFIED_AS_IN_RESULT_VALUE,
    CLASSIFIED_AS_LOCAL_RESULT_VALUE,
    CLASSIFIED_AS_NONE
} ExtractClassification;

typedef struct programGraphNode {
    struct reference *ref;		/* original reference of node */
    struct referenceItem *symRef;
    struct programGraphNode *jump;
    InspectionBits posBits;               /* INSIDE/OUSIDE block */
    InspectionBits stateBits;             /* visited + where set */
    ExtractClassification classification; /* resulting classification */
    struct programGraphNode *next;
} ProgramGraphNode;


static void dumpProgramToLog(ProgramGraphNode *program) {
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

/* Non-static because unused - use from debugger */
void dumpProgram(ProgramGraphNode *program) {
    printf("[ProgramDump begin]\n");
    for (ProgramGraphNode *p=program; p!=NULL; p=p->next) {
        printf("%p: %2d %2d %s %s\n", p,
                p->posBits, p->stateBits,
                p->symRef->linkName,
                usageKindEnumName[p->ref->usage.kind]+5);
        if (p->symRef->type==TypeLabel && p->ref->usage.kind!=UsageDefined) {
            printf("    Jump: %p\n", p->jump);
        }
    }
    printf("[ProgramDump end]\n");
}

Symbol *addContinueBreakLabelSymbol(int labn, char *name) {
    Symbol *symbol;

    if (options.serverOperation != OLO_EXTRACT)
        return NULL;

    symbol = newSymbolAsLabel(name, name, noPosition, labn);
    symbol->type = TypeLabel;
    symbol->storage = StorageAuto;


    addSymbolToTable(symbolTable, symbol);
    return(symbol);
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

void generateContinueBreakReference(char *name) {
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
    Reference *ref, ReferenceItem *symRef,
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

static void extractFunGraphRef(ReferenceItem *rr, void *prog) {
    Reference *r;
    ProgramGraphNode *p,**ap;
    ap = (ProgramGraphNode **) prog;
    for (r=rr->references; r!=NULL; r=r->next) {
        if (dm_isBetween(cxMemory,r,parsedInfo.cxMemoryIndexAtFunctionBegin,parsedInfo.cxMemoryIndexAtFunctionEnd)){
            p = newProgramGraphNode(r, rr, NULL, 0, 0, CLASSIFIED_AS_NONE, *ap);
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

static Reference *getDefinitionReference(ReferenceItem *lab) {
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
                                                ReferenceItem     *lab
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

static ProgramGraphNode *makeProgramGraph(void) {
    ProgramGraphNode *program, *p;
    program = NULL;
    mapOverReferenceTableWithPointer(extractFunGraphRef, ((void *) &program));
    LIST_SORT(ProgramGraphNode, program, linearOrder);
    dumpProgramToLog(program);
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
                                ReferenceItem *symRef,
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
    ReferenceItem     *symRef;
    unsigned    inUsages,outUsages,outUsageBothExists;
    symRef = varRef->symRef;
    for (p=program; p!=NULL; p=p->next) {
        p->stateBits = 0;
    }
    //&dumpProgramToLog(program);
    extSetSetStates(program, symRef, INSPECTION_VISITED);
    //&dumpProgramToLog(program);
    inUsages = outUsages = outUsageBothExists = 0;
    for (p=program; p!=NULL; p=p->next) {
        if (p->symRef == varRef->symRef && p->ref->usage.kind != UsageNone) {
            if (p->posBits==INSPECTION_INSIDE_BLOCK) {
                inUsages |= p->stateBits;
            } else if (p->posBits==INSPECTION_OUTSIDE_BLOCK) {
                outUsages |= p->stateBits;
            } else assert(0);
        }
    }

    // inUsages marks usages in the block (from inside, or from ouside)
    // outUsages marks usages out of block (from inside, or from ouside)
    if (varRef->posBits == INSPECTION_OUTSIDE_BLOCK) {
        // a variable defined outside of the block
        if (outUsages & INSPECTION_INSIDE_BLOCK) {
            // a value set in the block is used outside
            if ((inUsages & INSPECTION_INSIDE_REENTER) == 0
                && (inUsages & INSPECTION_OUTSIDE_BLOCK) == 0
                && (outUsages & INSPECTION_INSIDE_PASSING) == 0
            ) {
                return CLASSIFIED_AS_OUT_ARGUMENT;
            } else {
                return CLASSIFIED_AS_IN_OUT_ARGUMENT;
            }
        }
        if (inUsages & INSPECTION_INSIDE_REENTER)
            return CLASSIFIED_AS_IN_OUT_ARGUMENT;
        if (inUsages & INSPECTION_OUTSIDE_BLOCK)
            return CLASSIFIED_AS_VALUE_ARGUMENT;
        if (inUsages)
            return CLASSIFIED_AS_LOCAL_VAR;
        return CLASSIFIED_AS_NONE;
    } else {
        if (outUsages & INSPECTION_INSIDE_BLOCK
            || outUsages & INSPECTION_OUTSIDE_BLOCK
        ) {
            // a variable defined inside the region used outside
            return CLASSIFIED_AS_LOCAL_OUT_ARGUMENT;
        } else {
            return CLASSIFIED_AS_NONE;
        }
    }
}

static ExtractClassification classifyLocalVariableForExtraction(
    ProgramGraphNode *program,
    ProgramGraphNode *varRef
) {
    ExtractClassification classification;

    classification = classifyLocalVariableExtraction0(program, varRef);
    if (isStructOrUnion(varRef) && classification!=CLASSIFIED_AS_NONE && classification!=CLASSIFIED_AS_LOCAL_VAR) {
        return CLASSIFIED_AS_ADDRESS_ARGUMENT;
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

static void setInOutBlockFields(ProgramGraphNode *program) {
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

static bool areThereJumpsInOrOutOfBlock(ProgramGraphNode *program) {
    for (ProgramGraphNode *p=program; p!=NULL; p=p->next) {
        assert(p->symRef!=NULL);
        if (p->symRef->type==TypeLabel) {
            assert(p->ref!=NULL);
            if (p->ref->usage.kind==UsageUsed || p->ref->usage.kind==UsageFork) {
                assert(p->jump != NULL);
                if (p->posBits != p->jump->posBits) {
                    //&fprintf(dumpOut,"jump in/out at %s : %x\n",p->symRef->linkName, p);
                    return true;
                }
            }
        }
    }
    return false;
}

static bool isLocalVariable(ProgramGraphNode *node) {
    return node->ref->usage.kind==UsageDefined
        &&  node->symRef->type==TypeDefault
        &&  node->symRef->scope==ScopeAuto;
}

static void classifyLocalVariables(ProgramGraphNode *program) {
    ProgramGraphNode *p;
    for (p=program; p!=NULL; p=p->next) {
        if (isLocalVariable(p)) {
            p->classification = classifyLocalVariableForExtraction(program,p);
        }
    }
}

/* This should really be split into three different functions, one for
 * each result: name, declarator, declaration...
 * Name... DONE */
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

static void getLocalVariableDeclarationFromLinkName(char *linkName, char *declaration,
                                                   char *declarationPrefix, bool shouldCopyName) {
    char *src, *declarationP;

    log_trace("%s '%s'", __FUNCTION__, linkName);

    // linkName always starts with a space?
    // Type/Declarator:
    for (src=linkName+1, declarationP=declaration;
         *src!=0 && *src!=LINK_NAME_SEPARATOR;
         src++, declarationP++
    ) {
        *declarationP = *src;
    }
    assert(*src);

    for (src++; *src!=0 && *src!=LINK_NAME_SEPARATOR; src++, declarationP++) {
        *declarationP = *src;
    }
    assert(*src);

    // Declaration prefix
    declarationP = strmcpy(declarationP, declarationPrefix);

    // Name
    for (src++; *src!=0 && *src!=LINK_NAME_SEPARATOR; src++) {
        if (shouldCopyName)
            *declarationP++ = *src;
    }
    assert(*src);

    // What part is this?
    for (src++; *src!=0 && *src!=LINK_NAME_SEPARATOR; src++, declarationP++) {
        *declarationP = *src;
    }
    *declarationP = 0;
}

static void getLocalVariableNameFromLinkName(char *linkName, char *name) {
    char *src;

    log_trace("%s '%s'", __FUNCTION__, linkName);

    // linkName always starts with a space? Skip first part
    src = linkName+1;
    src = strchr(src, LINK_NAME_SEPARATOR);
    assert(src);
    assert(*src);

    // skip also second part
    src++;
    src = strchr(src, LINK_NAME_SEPARATOR);
    assert(src);
    assert(*src);

    // Copy the name
    char *nameP;
    for (nameP=name, src++; *src!=0 && *src!=LINK_NAME_SEPARATOR; src++, nameP++) {
        *nameP = *src;
    }
    assert(*src);
    *nameP = 0;
}

static void reclassifyInOutVariables(ProgramGraphNode *program) {
    ProgramGraphNode *op = NULL;
    bool uniqueOutFlag = true;

    for (ProgramGraphNode *p=program; p!=NULL; p=p->next) {
        if (options.extractMode == EXTRACT_FUNCTION) {
            if (p->classification == CLASSIFIED_AS_OUT_ARGUMENT
                || p->classification == CLASSIFIED_AS_LOCAL_OUT_ARGUMENT
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
        if (op->classification == CLASSIFIED_AS_LOCAL_OUT_ARGUMENT) {
            op->classification = CLASSIFIED_AS_LOCAL_RESULT_VALUE;
        } else {
            op->classification = CLASSIFIED_AS_RESULT_VALUE;
        }
        return;
    }

    op = NULL;
    uniqueOutFlag = true;
    for (ProgramGraphNode *p=program; p!=NULL; p=p->next) {
        if (options.extractMode == EXTRACT_FUNCTION) {
            if (p->classification == CLASSIFIED_AS_IN_OUT_ARGUMENT) {
                if (op == NULL)
                    op = p;
                else
                    uniqueOutFlag = false;
                // re-classify to in_out
            }
        }
    }

    if (op!=NULL && uniqueOutFlag) {
        op->classification = CLASSIFIED_AS_IN_RESULT_VALUE;
    }
}

/* ************************** macro ******************************* */
/* Preparing for extract function or macro inside expression */
/* But how do we know this... */
bool isInExpression = false;

static int strcatf(char *resultingString, char *format, ...) {
    va_list args;
    va_start(args, format);
    return vsprintf(resultingString+strlen(resultingString), format, args);
}

static void generateNewMacroCall(ProgramGraphNode *program, char *extractionName) {
    bool isFirstArgument = true;
    char resultingString[EXTRACT_GEN_BUFFER_SIZE+1] = "";

    if (isInExpression)
        strcatf(resultingString, "%s", extractionName);
    else

        strcatf(resultingString, "\t%s", extractionName);

    for (ProgramGraphNode *p=program; p!=NULL; p=p->next) {
        if (p->classification == CLASSIFIED_AS_VALUE_ARGUMENT
            ||  p->classification == CLASSIFIED_AS_IN_OUT_ARGUMENT
            ||  p->classification == CLASSIFIED_AS_ADDRESS_ARGUMENT
            ||  p->classification == CLASSIFIED_AS_OUT_ARGUMENT
            ||  p->classification == CLASSIFIED_AS_LOCAL_OUT_ARGUMENT
            ||  p->classification == CLASSIFIED_AS_RESULT_VALUE
            ||  p->classification == CLASSIFIED_AS_IN_RESULT_VALUE
            ||  p->classification == CLASSIFIED_AS_LOCAL_VAR
        ) {
            char name[TMP_STRING_SIZE];
            getLocalVariableNameFromLinkName(p->symRef->linkName, name);
            strcatf(resultingString, "%s%s", isFirstArgument?"(":", " , name);
            isFirstArgument = false;
        }
    }

    if (isInExpression)
        strcatf(resultingString, "%s)", isFirstArgument?"(":"");
    else
        strcatf(resultingString, "%s);\n", isFirstArgument?"(":"");

    assert(strlen(resultingString)<EXTRACT_GEN_BUFFER_SIZE-1);
    ppcGenRecord(PPC_STRING_VALUE, resultingString);
}

static void generateNewMacroHead(ProgramGraphNode *program, char *extractionName) {
    bool isFirstArgument = true;
    char resultingString[EXTRACT_GEN_BUFFER_SIZE+1] = "";

    strcatf(resultingString, "#define %s",extractionName);
    for (ProgramGraphNode *p=program; p!=NULL; p=p->next) {
        if (p->classification == CLASSIFIED_AS_VALUE_ARGUMENT
            ||  p->classification == CLASSIFIED_AS_IN_OUT_ARGUMENT
            ||  p->classification == CLASSIFIED_AS_OUT_ARGUMENT
            ||  p->classification == CLASSIFIED_AS_LOCAL_OUT_ARGUMENT
            ||  p->classification == CLASSIFIED_AS_ADDRESS_ARGUMENT
            ||  p->classification == CLASSIFIED_AS_RESULT_VALUE
            ||  p->classification == CLASSIFIED_AS_IN_RESULT_VALUE
            ||  p->classification == CLASSIFIED_AS_LOCAL_VAR
        ) {
            char name[MAX_EXTRACT_FUN_HEAD_SIZE];
            getLocalVariableNameFromLinkName(p->symRef->linkName, name);
            strcatf(resultingString, "%s%s", isFirstArgument?"(":"," , name);
            isFirstArgument = false;
        }
    }
    strcatf(resultingString, "%s) {\\\n", isFirstArgument?"(":"");
    assert(strlen(resultingString)<EXTRACT_GEN_BUFFER_SIZE-1);
    ppcGenRecord(PPC_STRING_VALUE, resultingString);
}

static void generateNewMacroTail(void) {
    ppcGenRecord(PPC_STRING_VALUE, "}\n\n");
}



/* ********************** C function **************************** */


static void generateNewFunctionCall(ProgramGraphNode *program, char *extractionName) {
    bool isFirstArgument = true;
    char resultingString[EXTRACT_GEN_BUFFER_SIZE+1] = "";

    ProgramGraphNode *p;
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == CLASSIFIED_AS_RESULT_VALUE
            || p->classification == CLASSIFIED_AS_LOCAL_RESULT_VALUE
            || p->classification == CLASSIFIED_AS_IN_RESULT_VALUE
        )
            break;
    }
    if (p!=NULL) {
        char name[TMP_STRING_SIZE];
        char declaration[TMP_STRING_SIZE];
        getLocalVariableNameFromLinkName(p->symRef->linkName, name);
        getLocalVariableDeclarationFromLinkName(p->symRef->linkName, declaration, "", true);
        if (p->classification == CLASSIFIED_AS_LOCAL_RESULT_VALUE) {
            strcatf(resultingString, "\t%s = ", declaration);
        } else {
            strcatf(resultingString,"\t%s = ", name);
        }
    } else {
        if (!isInExpression)
            strcat(resultingString, "\t");
    }
    strcatf(resultingString, "%s", extractionName);

    for (ProgramGraphNode *p=program; p!=NULL; p=p->next) {
        if (p->classification == CLASSIFIED_AS_VALUE_ARGUMENT
            || p->classification == CLASSIFIED_AS_IN_RESULT_VALUE
        ) {
            char name[TMP_STRING_SIZE];
            getLocalVariableNameFromLinkName(p->symRef->linkName, name);
            strcatf(resultingString, "%s%s", isFirstArgument?"(":", " , name);
            isFirstArgument = false;
        }
    }
    for (ProgramGraphNode *p=program; p!=NULL; p=p->next) {
        if (p->classification == CLASSIFIED_AS_IN_OUT_ARGUMENT
            || p->classification == CLASSIFIED_AS_ADDRESS_ARGUMENT
            || p->classification == CLASSIFIED_AS_OUT_ARGUMENT
            || p->classification == CLASSIFIED_AS_LOCAL_OUT_ARGUMENT
        ) {
            char name[TMP_STRING_SIZE];
            getLocalVariableNameFromLinkName(p->symRef->linkName, name);
            strcatf(resultingString, "%s&%s", isFirstArgument?"(":", " , name);
            isFirstArgument = false;
        }
    }

    if (isInExpression)         /* TODO - how can we know this...? */
        strcatf(resultingString, "%s)", isFirstArgument?"(":"");
    else
        strcatf(resultingString, "%s);\n", isFirstArgument?"(":"");
    assert(strlen(resultingString)<EXTRACT_GEN_BUFFER_SIZE-1);

    ppcGenRecord(PPC_STRING_VALUE, resultingString);
}

static void generateNewFunctionHead(ProgramGraphNode *program, char *extractionName) {
    char nhead[MAX_EXTRACT_FUN_HEAD_SIZE+2];
    int nhi, ldclaLen;
    char ldcla[TMP_STRING_SIZE];
    char declarator[TMP_STRING_SIZE];
    char declaration[MAX_EXTRACT_FUN_HEAD_SIZE];
    char name[MAX_EXTRACT_FUN_HEAD_SIZE];
    ProgramGraphNode  *p;
    int isFirstArgument = true;
    char resultingString[EXTRACT_GEN_BUFFER_SIZE+1] = "";

    /* function header */
    nhi = 0;
    sprintf(nhead+nhi,"%s",extractionName);
    nhi += strlen(nhead+nhi);
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == CLASSIFIED_AS_VALUE_ARGUMENT
            || p->classification == CLASSIFIED_AS_IN_RESULT_VALUE
        ) {
            getLocalVariableDeclarationFromLinkName(p->symRef->linkName, declaration, "", true);
            sprintf(nhead+nhi, "%s%s", isFirstArgument?"(":", " , declaration);
            nhi += strlen(nhead+nhi);
            isFirstArgument = false;
        }
    }
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == CLASSIFIED_AS_IN_OUT_ARGUMENT
            ||  p->classification == CLASSIFIED_AS_OUT_ARGUMENT
            ||  p->classification == CLASSIFIED_AS_LOCAL_OUT_ARGUMENT
        ) {
            getLocalVariableDeclarationFromLinkName(p->symRef->linkName, declaration, options.olExtractAddrParPrefix, true);
            sprintf(nhead+nhi, "%s%s", isFirstArgument?"(":", " , declaration);
            nhi += strlen(nhead+nhi);
            isFirstArgument = false;
        }
    }
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == CLASSIFIED_AS_ADDRESS_ARGUMENT) {
            getLocalVariableDeclarationFromLinkName(p->symRef->linkName, declaration, EXTRACT_REFERENCE_ARG_STRING, true);
            sprintf(nhead+nhi, "%s%s", isFirstArgument?"(":", " , declaration);
            nhi += strlen(nhead+nhi);
            isFirstArgument = false;
        }
    }
    sprintf(nhead+nhi, "%s)", isFirstArgument?"(":"");
    nhi += strlen(nhead+nhi);

    strcat(resultingString, "static ");

    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == CLASSIFIED_AS_RESULT_VALUE
            || p->classification == CLASSIFIED_AS_LOCAL_RESULT_VALUE
            || p->classification == CLASSIFIED_AS_IN_RESULT_VALUE)
            break;
    }
    if (p==NULL) {
        strcatf(resultingString,"void %s",nhead);
    } else {
        getLocalVariableDeclarationFromLinkName(p->symRef->linkName, declaration, nhead, false);
        strcatf(resultingString, "%s", declaration);
    }

    /* function body */
    strcat(resultingString, " {\n");
    ldcla[0] = 0;
    ldclaLen = 0;
    isFirstArgument = true;
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == CLASSIFIED_AS_IN_OUT_ARGUMENT
            || p->classification == CLASSIFIED_AS_LOCAL_VAR
            || p->classification == CLASSIFIED_AS_OUT_ARGUMENT
            || p->classification == CLASSIFIED_AS_RESULT_VALUE
            || (p->symRef->storage == StorageExtern
                && p->ref->usage.kind == UsageDeclared)
        ) {
            getLocalVariableNameFromLinkName(p->symRef->linkName, name);
            getLocalVarStringFromLinkName(p->symRef->linkName, name, declarator, declaration, "", true);
            if (strcmp(ldcla,declarator)==0) {
                strcatf(resultingString, ",%s",declaration+ldclaLen);
            } else {
                strcpy(ldcla,declarator); ldclaLen=strlen(ldcla);
                if (isFirstArgument)
                    strcatf(resultingString, "\t%s",declaration);
                else
                    strcatf(resultingString, ";\n\t%s",declaration);
            }
            if (p->classification == CLASSIFIED_AS_IN_OUT_ARGUMENT) {
                strcatf(resultingString, " = %s%s", options.olExtractAddrParPrefix, name);
            }
            isFirstArgument = false;
        }
    }
    if (!isFirstArgument)
        strcat(resultingString, ";\n");
    assert(strlen(resultingString)<EXTRACT_GEN_BUFFER_SIZE-1);
    ppcGenRecord(PPC_STRING_VALUE, resultingString);
}

static void generateNewFunctionTail(ProgramGraphNode *program) {
    char                name[TMP_STRING_SIZE];
    ProgramGraphNode  *p;
    char resultingString[EXTRACT_GEN_BUFFER_SIZE+1] = "";

    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == CLASSIFIED_AS_IN_OUT_ARGUMENT
            ||  p->classification == CLASSIFIED_AS_OUT_ARGUMENT
            ||  p->classification == CLASSIFIED_AS_LOCAL_OUT_ARGUMENT
        ) {
            getLocalVariableNameFromLinkName(p->symRef->linkName, name);
            strcatf(resultingString, "\t%s%s = %s;\n", options.olExtractAddrParPrefix,
                    name, name);
        }
    }
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == CLASSIFIED_AS_RESULT_VALUE
            || p->classification == CLASSIFIED_AS_IN_RESULT_VALUE
            || p->classification == CLASSIFIED_AS_LOCAL_RESULT_VALUE
            ) {
            getLocalVariableNameFromLinkName(p->symRef->linkName, name);
            strcatf(resultingString, "\n\treturn %s;\n", name);
        }
    }
    strcat(resultingString, "}\n\n");
    assert(strlen(resultingString)<EXTRACT_GEN_BUFFER_SIZE-1);
    ppcGenRecord(PPC_STRING_VALUE, resultingString);
}


/* ******************************************************************* */

static bool programStructureMismatch() {
    return parsedInfo.cxMemoryIndexAtFunctionBegin > parsedInfo.cxMemoryIndexAtBlockBegin
        || parsedInfo.cxMemoryIndexAtBlockBegin > parsedInfo.cxMemoryIndexAtBlockEnd
        || parsedInfo.cxMemoryIndexAtBlockEnd > parsedInfo.cxMemoryIndexAtFunctionEnd
        || parsedInfo.workMemoryIndexAtBlockBegin != parsedInfo.workMemoryIndexAtBlockEnd;
}

static void generateNewVariableAccess(ProgramGraphNode *program, char *extractionName) {
    ppcGenRecord(PPC_STRING_VALUE, extractionName);
}

static void generateNewVariableDeclaration(ProgramGraphNode *program, char *extractionName) {
    char resultingString[EXTRACT_GEN_BUFFER_SIZE+1] = "";
    /* TODO: How do we generate the correct type? This must already be done when extracting function? */
    sprintf(resultingString, "int %s = ", extractionName);
    ppcGenRecord(PPC_STRING_VALUE, resultingString);
}

static void generateNewVariableTail(void) {
    ppcGenRecord(PPC_STRING_VALUE, ";");
}


static void makeExtraction(void) {
    ProgramGraphNode *program;

    if (programStructureMismatch()) {
        errorMessage(ERR_ST, "Region / program structure mismatch");
        return;
    }

    log_trace("!cxMemories: funBegin, blockBegin, blockEnd, funEnd: %x, %x, %x, %x", parsedInfo.cxMemoryIndexAtFunctionBegin, parsedInfo.cxMemoryIndexAtBlockBegin, parsedInfo.cxMemoryIndexAtBlockEnd, parsedInfo.cxMemoryIndexAtFunctionEnd);
    assert(parsedInfo.cxMemoryIndexAtFunctionBegin);
    assert(parsedInfo.cxMemoryIndexAtBlockBegin);
    assert(parsedInfo.cxMemoryIndexAtBlockEnd);
    assert(parsedInfo.cxMemoryIndexAtFunctionEnd);

    program = makeProgramGraph();
    setInOutBlockFields(program);
    dumpProgramToLog(program);

    if (options.extractMode!=EXTRACT_MACRO && areThereJumpsInOrOutOfBlock(program)) {
        errorMessage(ERR_ST, "There are jumps in or out of region");
        return;
    }
    classifyLocalVariables(program);
    reclassifyInOutVariables(program);

    char *extractionName;
    if (options.extractMode == EXTRACT_VARIABLE)
        extractionName = "newVariable_";
    else if (options.extractMode==EXTRACT_MACRO) {
        extractionName = "NEW_MACRO_";
    } else
        extractionName = "newFunction_";

    ppcBeginWithStringAttribute(PPC_EXTRACTION_DIALOG, PPCA_TYPE, extractionName);

    if (options.extractMode==EXTRACT_MACRO) {
        generateNewMacroCall(program, extractionName);
        generateNewMacroHead(program, extractionName);
        generateNewMacroTail();
    } else if (options.extractMode == EXTRACT_VARIABLE) {
        generateNewVariableAccess(program, extractionName);
        generateNewVariableDeclaration(program, extractionName);
        generateNewVariableTail();
    } else {
        generateNewFunctionCall(program, extractionName);
        generateNewFunctionHead(program, extractionName);
        generateNewFunctionTail(program);
    }

    ppcValueRecord(PPC_INT_VALUE, parsedInfo.functionBeginPosition, "");
    ppcEnd(PPC_EXTRACTION_DIALOG);
}


void actionsBeforeAfterExternalDefinition(void) {
    if (parsedInfo.cxMemoryIndexAtBlockEnd != 0
        // you have to check for matching class method
        // i.e. for case 'void mmm() { //blockbeg; ...; //blockend; class X { mmm(){}!!}; }'
        && parsedInfo.cxMemoryIndexAtFunctionBegin != 0
        && parsedInfo.cxMemoryIndexAtFunctionBegin <= parsedInfo.cxMemoryIndexAtBlockBegin
        // is it an extraction action ?
        && options.serverOperation == OLO_EXTRACT
        && (! parsedInfo.extractProcessedFlag))
    {
        // O.K. make extraction
        parsedInfo.cxMemoryIndexAtFunctionEnd = cxMemory->index;
        makeExtraction();
        parsedInfo.extractProcessedFlag = true;
        /* here should be a longjmp to stop file processing !!!! */
        /* No, all this extraction should be after parsing ! */
    }
    parsedInfo.cxMemoryIndexAtFunctionBegin = cxMemory->index;
    if (includeStack.pointer > 0) {
        parsedInfo.functionBeginPosition = includeStack.stack[0].lineNumber+1;
    } else {
        parsedInfo.functionBeginPosition = currentFile.lineNumber+1;
    }
}


void extractActionOnBlockMarker(void) {
    Position pos;
    if (parsedInfo.cxMemoryIndexAtBlockBegin == 0) {
        parsedInfo.cxMemoryIndexAtBlockBegin = cxMemory->index;
        parsedInfo.workMemoryIndexAtBlockBegin = currentBlock->outerBlock;
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
