#include "extract.h"

#include <string.h>

#include "commons.h"
#include "cxref.h"
#include "filedescriptor.h"
#include "globals.h"
#include "list.h"
#include "log.h"
#include "memory.h"
#include "misc.h"
#include "parsing.h"
#include "protocol.h"
#include "ppc.h"
#include "protocol.h"
#include "referenceableitemtable.h"
#include "semact.h"
#include "stackmemory.h"


#define EXTRACT_GEN_BUFFER_SIZE 500000
#define EXTRACT_REFERENCE_ARG_STRING "&"
#define EXTRACT_OUTPUT_PARAM_PREFIX "*_"


/* ********************** Data Flow analysis bits ********************* */

typedef enum {
    DATAFLOW_ANALYZED = 1,
    DATAFLOW_INSIDE_BLOCK = 2,
    DATAFLOW_OUTSIDE_BLOCK = 4,
    DATAFLOW_INSIDE_REENTER = 8,		/* value reenters the block             */
    DATAFLOW_INSIDE_PASSING = 16		/* a non-modified values pass via block */
} DataFlowBits;

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
    struct reference *reference;          /* original reference of node */
    struct referenceableItem *referenceableItem;
    struct programGraphNode *jump;
    DataFlowBits regionSide;              /* INSIDE/OUTSIDE block */
    DataFlowBits state;                   /* where value comes from + flow flags */
    bool visited;                         /* visited during dataflow traversal */
    ExtractClassification classification; /* resulting classification */
    struct programGraphNode *next;
} ProgramGraphNode;


static void dumpProgramToLog(ProgramGraphNode *program) {
    log_trace("[ProgramDump begin]");
    for (ProgramGraphNode *p=program; p!=NULL; p=p->next) {
        log_trace("%p: %2d %2d %s %s", p,
                p->regionSide, p->state,
                p->referenceableItem->linkName,
                usageKindEnumName[p->reference->usage]+5);
        if (p->referenceableItem->type==TypeLabel && p->reference->usage!=UsageDefined) {
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
                p->regionSide, p->state,
                p->referenceableItem->linkName,
                usageKindEnumName[p->reference->usage]+5);
        if (p->referenceableItem->type==TypeLabel && p->reference->usage!=UsageDefined) {
            printf("    Jump: %p\n", p->jump);
        }
    }
    printf("[ProgramDump end]\n");
}

Symbol *addContinueBreakLabelSymbol(int labn, char *name) {
    Symbol *symbol;

    if (parsingConfig.operation != PARSE_TO_EXTRACT)
        return NULL;

    symbol = newSymbolAsLabel(name, name, noPosition, labn);
    symbol->type = TypeLabel;
    symbol->storage = StorageAuto;


    addSymbolToTable(symbolTable, symbol);
    return symbol;
}


void deleteContinueBreakLabelSymbol(char *name) {
    Symbol symbol, *foundSymbol;

    if (parsingConfig.operation != PARSE_TO_EXTRACT)
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

    if (parsingConfig.operation != PARSE_TO_EXTRACT)
        return;

    fillSymbolWithLabel(&symbol, name, name, noPosition, 0);
    symbol.type = TypeLabel;
    symbol.storage = StorageAuto;

    if (symbolTableIsMember(symbolTable, &symbol, NULL, &foundSymbol)) {
        generateInternalLabelReference(foundSymbol->labelIndex, UsageUsed);
    }
}

void generateSwitchCaseFork(bool isLast) {
    Symbol symbol, *foundSymbol;

    if (parsingConfig.operation != PARSE_TO_EXTRACT)
        return;

    fillSymbolWithLabel(&symbol, SWITCH_LABEL_NAME, SWITCH_LABEL_NAME, noPosition, 0);
    symbol.type = TypeLabel;
    symbol.storage = StorageAuto;

    if (symbolTableIsMember(symbolTable, &symbol, NULL, &foundSymbol)) {
        generateInternalLabelReference(foundSymbol->labelIndex, UsageDefined);
        if (!isLast) {
            foundSymbol->labelIndex++;
            generateInternalLabelReference(foundSymbol->labelIndex, UsageFork);
        }
    }
}

static ProgramGraphNode *newProgramGraphNode(ReferenceableItem *referenceableItem,
                                             Reference *reference, ProgramGraphNode *jump,
                                             char regionSide, char state, char classifcation,
                                             ProgramGraphNode *next
) {
    ProgramGraphNode *programGraph = cxAlloc(sizeof(ProgramGraphNode));

    programGraph->reference = reference;
    programGraph->referenceableItem = referenceableItem;
    programGraph->jump = jump;
    programGraph->regionSide = regionSide;
    programGraph->state = state;
    programGraph->visited = false;
    programGraph->classification = classifcation;
    programGraph->next = next;

    return programGraph;
}

static void extractFunGraphRef(ReferenceableItem *referenceableItem, void *prog) {
    ProgramGraphNode **ap = (ProgramGraphNode **) prog;
    for (Reference *r=referenceableItem->references; r!=NULL; r=r->next) {
        if (cxMemoryPointerIsBetween(r, parsedInfo.cxMemoryIndexAtFunctionBegin,
                                     parsedInfo.cxMemoryIndexAtFunctionEnd)
            ){
            ProgramGraphNode *p = newProgramGraphNode(referenceableItem, r, NULL, 0, 0,
                                                      CLASSIFIED_AS_NONE, *ap);
            *ap = p;
        }
    }
}

static ProgramGraphNode *getGraphAddress(ProgramGraphNode *program, Reference *ref) {
    ProgramGraphNode *result = NULL;

    for (ProgramGraphNode *p=program; result==NULL && p!=NULL; p=p->next) {
        if (p->reference == ref)
            result = p;
    }
    return result;
}

static Reference *getDefinitionReference(ReferenceableItem *lab) {
    Reference *result;

    for (result=lab->references; result!=NULL && result->usage!=UsageDefined; result=result->next)
        ;
    if (result == NULL) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff,"jump to unknown label '%s'",lab->linkName);
        errorMessage(ERR_ST,tmpBuff);
    }
    return result;
}

static ProgramGraphNode *getLabelGraphAddress(ProgramGraphNode *program,
                                                ReferenceableItem     *lab
                                                ) {
    ProgramGraphNode  *res;
    Reference         *defref;
    assert(lab->type == TypeLabel);
    defref = getDefinitionReference(lab);
    res = getGraphAddress(program, defref);
    return res;
}

static bool linearOrder(ProgramGraphNode *n1, ProgramGraphNode *n2) {
    return n1->reference < n2->reference;
}

static ProgramGraphNode *makeProgramGraph(void) {
    ProgramGraphNode *program = NULL;

    mapOverReferenceableItemTableWithPointer(extractFunGraphRef, ((void *) &program));
    LIST_SORT(ProgramGraphNode, program, linearOrder);
    dumpProgramToLog(program);
    for (ProgramGraphNode *p=program; p!=NULL; p=p->next) {
        if (p->referenceableItem->type==TypeLabel && p->reference->usage!=UsageDefined) {
            // resolve the jump
            p->jump = getLabelGraphAddress(program, p->referenceableItem);
        }
    }
    return program;
}

static bool isStructOrUnion(ProgramGraphNode *node) {
    return node->referenceableItem->linkName[0] == LINK_NAME_EXTRACT_STR_UNION_TYPE_FLAG;
}

static bool hasBit(unsigned state, DataFlowBits bit) {
    return (state & bit) != 0;
}

static void analyzeVariableDataFlow(ProgramGraphNode *p, ReferenceableItem *referenceableItem,
                                    unsigned incomingState) {
    unsigned propagatingState = incomingState;  // State flowing through the for-loop

    for (; p!=NULL; p=p->next) {
    again:
        // === Avoid redundant processing ===
        if (p->visited && p->state == propagatingState)
            return;  // Fixed point reached; no new information
        p->visited = true;

        // === Accumulate state from incoming + node's stored state ===
        unsigned accumulatedState = (propagatingState | p->state);
        p->state = accumulatedState;

        // === Process node based on type and variable being analyzed ===
        unsigned nodeProcessingState = accumulatedState;
        unsigned valueStateIfAssignedHere = p->regionSide;

        if (p->referenceableItem == referenceableItem) {          // the examined variable
            if (p->reference->usage == UsageAddrUsed) {
                nodeProcessingState = valueStateIfAssignedHere;
                // change only state, so usage is kept
            } else if (p->reference->usage == UsageLvalUsed
                       || (p->reference->usage == UsageDefined && ! isStructOrUnion(p))
                ) {
                // variable is assigned here, so reset its stored state too
                p->state = nodeProcessingState = valueStateIfAssignedHere;
            }
        } else if (p->referenceableItem->type==TypeBlockMarker &&
                   (p->regionSide&DATAFLOW_INSIDE_BLOCK) == 0) {
            // === Region boundary crossing: add special flags ===
            // leaving the block
            if (hasBit(nodeProcessingState, DATAFLOW_INSIDE_BLOCK)) {
                // leaving and value is set from inside, preset for possible reentering
                nodeProcessingState |= DATAFLOW_INSIDE_REENTER;
            }
            if (hasBit(nodeProcessingState, DATAFLOW_OUTSIDE_BLOCK)) {
                // leaving and value is set from outside, set value passing flag
                nodeProcessingState |= DATAFLOW_INSIDE_PASSING;
            }
        }

        // === Handle control flow (labels, gotos, branches) ===
        if (p->referenceableItem->type==TypeLabel) {
            if (p->reference->usage==UsageUsed) {  // goto
                propagatingState = nodeProcessingState;
                p = p->jump;
                goto again;
            } else if (p->reference->usage==UsageFork) {   // branching
                analyzeVariableDataFlow(p->jump, referenceableItem, nodeProcessingState);
            }
        }

        // === Carry forward for next iteration ===
        propagatingState = nodeProcessingState;
    }
}

static bool isValueFromBlockUsedAfter(unsigned outUsages) {
    return hasBit(outUsages, DATAFLOW_INSIDE_BLOCK);
}

static bool hasSimpleOutflowOnly(unsigned inUsages, unsigned outUsages) {
    return !hasBit(inUsages, DATAFLOW_INSIDE_REENTER) && !hasBit(inUsages, DATAFLOW_OUTSIDE_BLOCK)
        && !hasBit(outUsages, DATAFLOW_INSIDE_PASSING);
}

static bool isValueUsedFromOutside(unsigned inUsages) {
    return hasBit(inUsages, DATAFLOW_INSIDE_REENTER);
}

static bool hasAnyUsageInBlock(unsigned inUsages) {
    return inUsages != 0;
}

static bool isVariableUsedOutsideBlock(unsigned outUsages) {
    return hasBit(outUsages, DATAFLOW_INSIDE_BLOCK) || hasBit(outUsages, DATAFLOW_OUTSIDE_BLOCK);
}

static ExtractClassification classifyLocalVariableExtraction0(ProgramGraphNode *program,
                                                              ProgramGraphNode *variableNode) {
    ProgramGraphNode *p;
    ReferenceableItem *referenceableItem;
    unsigned inUsages, outUsages, outUsageBothExists;

    referenceableItem = variableNode->referenceableItem;
    for (p=program; p!=NULL; p=p->next) {
        p->state = 0;
        p->visited = false;
    }
    //&dumpProgramToLog(program);
    analyzeVariableDataFlow(program, referenceableItem, DATAFLOW_ANALYZED);
    //&dumpProgramToLog(program);
    inUsages = outUsages = outUsageBothExists = 0;
    for (p=program; p!=NULL; p=p->next) {
        if (p->referenceableItem == variableNode->referenceableItem && p->reference->usage != UsageNone) {
            if (p->regionSide == DATAFLOW_INSIDE_BLOCK) {
                inUsages |= p->state;
            } else if (p->regionSide == DATAFLOW_OUTSIDE_BLOCK) {
                outUsages |= p->state;
            } else assert(0);
        }
    }

    // inUsages marks usages in the block (from inside, or from ouside)
    // outUsages marks usages out of block (from inside, or from ouside)
    if (variableNode->regionSide == DATAFLOW_OUTSIDE_BLOCK) {
        // a variable defined outside of the block
        if (isValueFromBlockUsedAfter(outUsages)) {
            // a value set in the block is used outside
            if (hasSimpleOutflowOnly(inUsages, outUsages)) {
                return CLASSIFIED_AS_OUT_ARGUMENT;
            } else {
                return CLASSIFIED_AS_IN_OUT_ARGUMENT;
            }
        }
        if (isValueUsedFromOutside(inUsages))
            return CLASSIFIED_AS_IN_OUT_ARGUMENT;
        if (hasBit(inUsages, DATAFLOW_OUTSIDE_BLOCK))
            return CLASSIFIED_AS_VALUE_ARGUMENT;
        if (hasAnyUsageInBlock(inUsages))
            return CLASSIFIED_AS_LOCAL_VAR;
        return CLASSIFIED_AS_NONE;
    } else {
        if (isVariableUsedOutsideBlock(outUsages)) {
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
    if (*pos == DATAFLOW_INSIDE_BLOCK)
        *pos = DATAFLOW_OUTSIDE_BLOCK;
    else if (*pos == DATAFLOW_OUTSIDE_BLOCK)
        *pos = DATAFLOW_INSIDE_BLOCK;
    else
        assert(0);

    return *pos;
}

static void setInOutBlockFields(ProgramGraphNode *program) {
    ProgramGraphNode *p;
    unsigned    pos;
    pos = DATAFLOW_OUTSIDE_BLOCK;
    for (p=program; p!=NULL; p=p->next) {
        if (p->referenceableItem->type == TypeBlockMarker) {
            toogleInOutBlock(&pos);
        }
        p->regionSide = pos;
    }
}

static bool areThereJumpsInOrOutOfBlock(ProgramGraphNode *program) {
    for (ProgramGraphNode *p=program; p!=NULL; p=p->next) {
        assert(p->referenceableItem!=NULL);
        if (p->referenceableItem->type==TypeLabel) {
            assert(p->reference!=NULL);
            if (p->reference->usage==UsageUsed || p->reference->usage==UsageFork) {
                assert(p->jump != NULL);
                if (p->regionSide != p->jump->regionSide) {
                    //&fprintf(dumpOut,"jump in/out at %s : %x\n",p->referenceableItem->linkName, p);
                    return true;
                }
            }
        }
    }
    return false;
}

static bool isLocalVariable(ProgramGraphNode *node) {
    return node->reference->usage==UsageDefined
        &&  node->referenceableItem->type==TypeDefault
        &&  node->referenceableItem->scope==AutoScope;
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
        if (parsingConfig.extractMode == EXTRACT_FUNCTION) {
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
        if (parsingConfig.extractMode == EXTRACT_FUNCTION) {
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

/* ************************** macro ********************************* */
/* Preparing for extract function or macro inside expression, e.g.    */
/*     if (some expression) ... and we want extract "some expression" */
/* But how do we know this...                                         */
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
            getLocalVariableNameFromLinkName(p->referenceableItem->linkName, name);
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

    strcatf(resultingString, "#define %s", extractionName);
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
            getLocalVariableNameFromLinkName(p->referenceableItem->linkName, name);
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
        getLocalVariableNameFromLinkName(p->referenceableItem->linkName, name);
        getLocalVariableDeclarationFromLinkName(p->referenceableItem->linkName, declaration, "", true);
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
            getLocalVariableNameFromLinkName(p->referenceableItem->linkName, name);
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
            getLocalVariableNameFromLinkName(p->referenceableItem->linkName, name);
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
            getLocalVariableDeclarationFromLinkName(p->referenceableItem->linkName, declaration, "", true);
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
            getLocalVariableDeclarationFromLinkName(p->referenceableItem->linkName, declaration, EXTRACT_OUTPUT_PARAM_PREFIX,
                                                    true);
            sprintf(nhead+nhi, "%s%s", isFirstArgument?"(":", " , declaration);
            nhi += strlen(nhead+nhi);
            isFirstArgument = false;
        }
    }
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == CLASSIFIED_AS_ADDRESS_ARGUMENT) {
            getLocalVariableDeclarationFromLinkName(p->referenceableItem->linkName, declaration, EXTRACT_REFERENCE_ARG_STRING, true);
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
        getLocalVariableDeclarationFromLinkName(p->referenceableItem->linkName, declaration, nhead, false);
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
            || (p->referenceableItem->storage == StorageExtern
                && p->reference->usage == UsageDeclared)
        ) {
            getLocalVariableNameFromLinkName(p->referenceableItem->linkName, name);
            getLocalVarStringFromLinkName(p->referenceableItem->linkName, name, declarator, declaration, "", true);
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
                strcatf(resultingString, " = %s%s", EXTRACT_OUTPUT_PARAM_PREFIX, name);
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
            getLocalVariableNameFromLinkName(p->referenceableItem->linkName, name);
            strcatf(resultingString, "\t%s%s = %s;\n", EXTRACT_OUTPUT_PARAM_PREFIX,
                    name, name);
        }
    }
    for (p=program; p!=NULL; p=p->next) {
        if (p->classification == CLASSIFIED_AS_RESULT_VALUE
            || p->classification == CLASSIFIED_AS_IN_RESULT_VALUE
            || p->classification == CLASSIFIED_AS_LOCAL_RESULT_VALUE
            ) {
            getLocalVariableNameFromLinkName(p->referenceableItem->linkName, name);
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
        || parsedInfo.blockAtBegin != parsedInfo.blockAtEnd;
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

static void generateLineNumber(int lineNumber) {
    ppcValueRecord(PPC_INT_VALUE, lineNumber, "");
}

static void makeExtraction(void) {
    ProgramGraphNode *program;

    if (programStructureMismatch()) {
        errorMessage(ERR_ST, "Region / program structure mismatch");
        return;
    }

    log_trace("!cxMemories: funBegin, blockBegin, blockEnd, funEnd: %x, %x, %x, %x",
              parsedInfo.cxMemoryIndexAtFunctionBegin,
              parsedInfo.cxMemoryIndexAtBlockBegin,
              parsedInfo.cxMemoryIndexAtBlockEnd, parsedInfo.cxMemoryIndexAtFunctionEnd);
    assert(parsedInfo.cxMemoryIndexAtFunctionBegin);
    assert(parsedInfo.cxMemoryIndexAtBlockBegin);
    assert(parsedInfo.cxMemoryIndexAtBlockEnd);
    assert(parsedInfo.cxMemoryIndexAtFunctionEnd);

    program = makeProgramGraph();
    setInOutBlockFields(program);
    dumpProgramToLog(program);

    if (parsingConfig.extractMode!=EXTRACT_MACRO && areThereJumpsInOrOutOfBlock(program)) {
        errorMessage(ERR_ST, "There are jumps in or out of region");
        return;
    }
    classifyLocalVariables(program);
    reclassifyInOutVariables(program);

    char *extractionName;
    if (parsingConfig.extractMode == EXTRACT_VARIABLE)
        extractionName = "newVariable_";
    else if (parsingConfig.extractMode==EXTRACT_MACRO) {
        extractionName = "NEW_MACRO_";
    } else
        extractionName = "newFunction_";

    ppcBeginWithStringAttribute(PPC_EXTRACTION_DIALOG, PPCA_TYPE, extractionName);

    if (parsingConfig.extractMode==EXTRACT_MACRO) {
        generateNewMacroCall(program, extractionName);
        generateNewMacroHead(program, extractionName);
        generateNewMacroTail();
        generateLineNumber(parsedInfo.functionBeginPosition);
    } else if (parsingConfig.extractMode == EXTRACT_VARIABLE) {
        generateNewVariableAccess(program, extractionName);
        generateNewVariableDeclaration(program, extractionName);
        generateNewVariableTail();
        generateLineNumber(currentFile.lineNumber);
    } else {
        generateNewFunctionCall(program, extractionName);
        generateNewFunctionHead(program, extractionName);
        generateNewFunctionTail(program);
        generateLineNumber(parsedInfo.functionBeginPosition);
    }

    ppcEnd(PPC_EXTRACTION_DIALOG);
}


void actionsBeforeAfterExternalDefinition(void) {
    if (parsedInfo.cxMemoryIndexAtBlockEnd != 0
        // you have to check for matching class method
        // i.e. for case 'void mmm() { //blockbeg; ...; //blockend; class X { mmm(){}!!}; }'
        && parsedInfo.cxMemoryIndexAtFunctionBegin != 0
        && parsedInfo.cxMemoryIndexAtFunctionBegin <= parsedInfo.cxMemoryIndexAtBlockBegin
        // is it an extraction action ?
        && parsingConfig.operation == PARSE_TO_EXTRACT
        && (! parsedInfo.extractProcessedFlag))
    {
        // O.K. make extraction
        parsedInfo.cxMemoryIndexAtFunctionEnd = cxMemory.index;
        makeExtraction();
        parsedInfo.extractProcessedFlag = true;
        /* here should be a longjmp to stop file processing !!!! */
        /* No, all this extraction should be after parsing ! */
    }
    parsedInfo.cxMemoryIndexAtFunctionBegin = cxMemory.index;
    if (includeStack.pointer > 0) {
        parsedInfo.functionBeginPosition = includeStack.stack[0].lineNumber+1;
    } else {
        parsedInfo.functionBeginPosition = currentFile.lineNumber+1;
    }
}


void extractActionOnBlockMarker(void) {
    if (parsedInfo.cxMemoryIndexAtBlockBegin == 0) {
        parsedInfo.cxMemoryIndexAtBlockBegin = cxMemory.index;
        parsedInfo.blockAtBegin = currentBlock->outerBlock;
    } else {
        assert(parsedInfo.cxMemoryIndexAtBlockEnd == 0);
        parsedInfo.cxMemoryIndexAtBlockEnd = cxMemory.index;
        parsedInfo.blockAtEnd = currentBlock->outerBlock;
    }
    Position pos = makePosition(currentFile.characterBuffer.fileNumber, 0, 0);
    addTrivialCxReference("Block", TypeBlockMarker, StorageDefault, pos, UsageUsed);
}

void deleteContinueBreakSymbol(Symbol *symbol) {
    if (parsingConfig.operation == PARSE_TO_EXTRACT)
        deleteSymDef(symbol);
}
