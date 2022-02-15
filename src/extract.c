#include "extract.h"

#include "commons.h"
#include "globals.h"
#include "options.h"
#include "misc.h"
#include "protocol.h"
#include "semact.h"
#include "cxref.h"
#include "reftab.h"
#include "list.h"
#include "jsemact.h"
#include "filedescriptor.h"
#include "log.h"
#include "protocol.h"


#define EXTRACT_GEN_BUFFER_SIZE 500000

typedef struct programGraphNode {
    struct reference            *ref;		/* original reference of node */
    struct symbolReferenceItem  *symRef;
    struct programGraphNode		*jump;
    char						posBits;		/* INSIDE/OUSIDE block */
    char						stateBits;		/* visited + where setted */
    char						classifBits;	/* resulting classification */
    struct programGraphNode		*next;
} ProgramGraphNode;


static unsigned s_javaExtractFromFunctionMods=AccessDefault;
static char *rb;
static char *s_extractionName;

/* Public only to avoid "unused" warning */
void dumpProgram(ProgramGraphNode *program) {
    ProgramGraphNode *p;

    log_trace("[ProgramDump begin]");
    for(p=program; p!=NULL; p=p->next) {
        log_trace("%p: %2d %2d %s %s", p,
                p->posBits, p->stateBits,
                p->symRef->name,
                usageEnumName[p->ref->usage.base]+5);
        if (p->symRef->b.symType==TypeLabel && p->ref->usage.base!=UsageDefined) {
            log_trace("    Jump: %p", p->jump);
        }
    }
    log_trace("[ProgramDump end]");
}


void generateInternalLabelReference(int counter, int usage) {
    char labelName[TMP_STRING_SIZE];
    Id labelId;
    Position position;

    if (options.server_operation != OLO_EXTRACT)
        return;

    snprintf(labelName, TMP_STRING_SIZE, "%%L%d", counter);

    position = (Position){.file = currentFile.lexBuffer.buffer.fileNumber, .line = 0, .col = 0};
    fillId(&labelId, labelName, NULL, position);

    if (usage != UsageDefined)
        labelId.position.line++;
    // line == 0 or 1 , (hack to get definition first)
    labelReference(&labelId, usage);
}


Symbol *addContinueBreakLabelSymbol(int labn, char *name) {
    Symbol *s;

    if (options.server_operation != OLO_EXTRACT)
        return NULL;

    s = newSymbolAsLabel(name, name, noPosition, labn);
    fillSymbolBits(&s->bits, AccessDefault, TypeLabel, StorageAuto);

    AddSymbolNoTrail(s, symbolTable);
    return(s);
}


void deleteContinueBreakLabelSymbol(char *name) {
    Symbol ss,*memb;

    if (options.server_operation != OLO_EXTRACT)
        return;

    fillSymbolWithLabel(&ss, name, name, noPosition, 0);
    fillSymbolBits(&ss.bits, AccessDefault, TypeLabel, StorageAuto);
    if (symbolTableIsMember(symbolTable, &ss, NULL, &memb)) {
        deleteContinueBreakSymbol(memb);
    } else {
        assert(0);
    }
}

void genContinueBreakReference(char *name) {
    Symbol ss,*memb;

    if (options.server_operation != OLO_EXTRACT)
        return;

    fillSymbolWithLabel(&ss, name, name, noPosition, 0);
    fillSymbolBits(&ss.bits, AccessDefault, TypeLabel, StorageAuto);

    if (symbolTableIsMember(symbolTable, &ss, NULL, &memb)) {
        generateInternalLabelReference(memb->u.labelIndex, UsageUsed);
    }
}

void generateSwitchCaseFork(bool isLast) {
    Symbol symbol, *found_member_pointer;

    if (options.server_operation != OLO_EXTRACT)
        return;

    fillSymbolWithLabel(&symbol, SWITCH_LABEL_NAME, SWITCH_LABEL_NAME, noPosition, 0);
    fillSymbolBits(&symbol.bits, AccessDefault, TypeLabel, StorageAuto);
    if (symbolTableIsMember(symbolTable, &symbol, NULL, &found_member_pointer)) {
        generateInternalLabelReference(found_member_pointer->u.labelIndex, UsageDefined);
        if (!isLast) {
            found_member_pointer->u.labelIndex++;
            generateInternalLabelReference(found_member_pointer->u.labelIndex, UsageFork);
        }
    }
}

static ProgramGraphNode *newProgramGraphNode(
    Reference *ref, SymbolReferenceItem *symRef,
    ProgramGraphNode *jump, char posBits,
    char stateBits,
    char classifBits,
    ProgramGraphNode *next
) {
    ProgramGraphNode *programGraph;

    CX_ALLOC(programGraph, ProgramGraphNode);

    programGraph->ref = ref;
    programGraph->symRef = symRef;
    programGraph->jump = jump;
    programGraph->posBits = posBits;
    programGraph->stateBits = stateBits;
    programGraph->classifBits = classifBits;
    programGraph->next = next;

    return programGraph;
}

static void extractFunGraphRef(SymbolReferenceItem *rr, void *prog) {
    Reference *r;
    ProgramGraphNode *p,**ap;
    ap = (ProgramGraphNode **) prog;
    for(r=rr->refs; r!=NULL; r=r->next) {
        if (DM_IS_BETWEEN(cxMemory,r,s_cp.cxMemoryIndexAtFunctionBegin,s_cp.cxMemoryIndexAtFunctionEnd)){
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
    for(p=program; res==NULL && p!=NULL; p=p->next) {
        if (p->ref == ref) res = p;
    }
    return(res);
}

static Reference *getDefinitionReference(SymbolReferenceItem *lab) {
    Reference *res;
    for(res=lab->refs; res!=NULL && res->usage.base!=UsageDefined; res=res->next) ;
    if (res == NULL) {
        char tmpBuff[TMP_BUFF_SIZE];
        sprintf(tmpBuff,"jump to unknown label '%s'",lab->name);
        errorMessage(ERR_ST,tmpBuff);
    }
    return(res);
}

static ProgramGraphNode *getLabelGraphAddress(ProgramGraphNode *program,
                                                SymbolReferenceItem     *lab
                                                ) {
    ProgramGraphNode  *res;
    Reference         *defref;
    assert(lab->b.symType == TypeLabel);
    defref = getDefinitionReference(lab);
    res = getGraphAddress(program, defref);
    return(res);
}

static int linearOrder(ProgramGraphNode *n1, ProgramGraphNode *n2) {
    return(n1->ref < n2->ref);
}

static ProgramGraphNode * extMakeProgramGraph(void) {
    ProgramGraphNode *program,*p;
    program = NULL;
    refTabMap5(&referenceTable, extractFunGraphRef, ((void *) &program));
    LIST_SORT(ProgramGraphNode, program, linearOrder);
    dumpProgram(program);
    for(p=program; p!=NULL; p=p->next) {
        if (p->symRef->b.symType==TypeLabel && p->ref->usage.base!=UsageDefined) {
            // resolve the jump
            p->jump = getLabelGraphAddress(program, p->symRef);
        }
    }
    return(program);
}

static bool isStructOrUnion(ProgramGraphNode *node) {
    return node->symRef->name[0]==LINK_NAME_EXTRACT_STR_UNION_TYPE_FLAG;
}

static void extSetSetStates(    ProgramGraphNode *p,
                                SymbolReferenceItem *symRef,
                                unsigned cstate
                                ) {
    unsigned cpos,oldStateBits;
    for(; p!=NULL; p=p->next) {
    cont:
        if (p->stateBits == cstate)
            return;
        oldStateBits = p->stateBits;
        cstate = p->stateBits = (cstate | oldStateBits | INSP_VISITED);
        cpos = p->posBits | INSP_VISITED;
        if (p->symRef == symRef) {          // the examined variable
            if (p->ref->usage.base == UsageAddrUsed) {
                cstate = cpos;
                // change only state, so usage is kept
            } else if (p->ref->usage.base == UsageLvalUsed
                       || (p->ref->usage.base == UsageDefined
                           && ! isStructOrUnion(p))
                       ) {
                // change also current value, because there is no usage
                p->stateBits = cstate = cpos;
            }
        } else if (p->symRef->b.symType==TypeBlockMarker &&
                   (p->posBits&INSP_INSIDE_BLOCK) == 0) {
            // leaving the block
            if (cstate & INSP_INSIDE_BLOCK) {
                // leaving and value is set from inside, preset for possible
                // reentering
                cstate |= INSP_INSIDE_REENTER;
            }
            if (cstate & INSP_OUTSIDE_BLOCK) {
                // leaving and value is set from outside, set value passing flag
                cstate |= INSP_INSIDE_PASSING;
            }
        } else if (p->symRef->b.symType==TypeLabel) {
            if (p->ref->usage.base==UsageUsed) {  // goto
                p = p->jump;
                goto cont;
            } else if (p->ref->usage.base==UsageFork) {   // branching
                extSetSetStates(p->jump, symRef, cstate);
            }
        }
    }
}

static ExtractCategory categorizeLocalVariableExtraction0(
    ProgramGraphNode *program,
    ProgramGraphNode *varRef
) {
    ProgramGraphNode *p;
    SymbolReferenceItem     *symRef;
    unsigned    inUsages,outUsages,outUsageBothExists;
    symRef = varRef->symRef;
    for(p=program; p!=NULL; p=p->next) {
        p->stateBits = 0;
    }
    //&dumpProgram(program);
    extSetSetStates(program, symRef, INSP_VISITED);
    //&dumpProgram(program);
    inUsages = outUsages = outUsageBothExists = 0;
    for(p=program; p!=NULL; p=p->next) {
        if (p->symRef == varRef->symRef && p->ref->usage.base != UsageNone) {
            if (p->posBits==INSP_INSIDE_BLOCK) {
                inUsages |= p->stateBits;
            } else if (p->posBits==INSP_OUTSIDE_BLOCK) {
                outUsages |= p->stateBits;
                //&if ( (p->stateBits & INSP_INSIDE_BLOCK)
                //& &&  (p->stateBits & INSP_OUTSIDE_BLOCK)) {
                // a reference outside using values set in both in and out
                //&     outUsageBothExists = 1;
                //&}
            } else assert(0);
        }
    }
    //&fprintf(dumpOut,"%% ** variable '%s' ", varRef->symRef->name);fprintf(dumpOut,"in, out Usages %o %o\n",inUsages,outUsages);
    // inUsages marks usages in the block (from inside, or from ouside)
    // outUsages marks usages out of block (from inside, or from ouside)
    if (varRef->posBits == INSP_OUTSIDE_BLOCK) {
        // a variable defined outside of the block
        if (outUsages & INSP_INSIDE_BLOCK) {
            // a value set in the block is used outside
            //&sprintf(tmpBuff,"testing %s: %d %d %d\n", varRef->symRef->name, (inUsages & INSP_INSIDE_REENTER),(inUsages & INSP_OUTSIDE_BLOCK), outUsageBothExists);ppcGenRecord(PPC_INFORMATION, tmpBuff);
            if ((inUsages & INSP_INSIDE_REENTER) == 0
                && (inUsages & INSP_OUTSIDE_BLOCK) == 0
                /*& && outUsageBothExists == 0 &*/
                && (outUsages & INSP_INSIDE_PASSING) == 0
                ) {
                return(EXTRACT_OUT_ARGUMENT);
            } else {
                return(EXTRACT_IN_OUT_ARGUMENT);
            }
        }
        if (inUsages & INSP_INSIDE_REENTER)
            return(EXTRACT_IN_OUT_ARGUMENT);
        if (inUsages & INSP_OUTSIDE_BLOCK)
            return(EXTRACT_VALUE_ARGUMENT);
        if (inUsages)
            return(EXTRACT_LOCAL_VAR);
        return(EXTRACT_NONE);
    } else {
        if (    outUsages & INSP_INSIDE_BLOCK
                ||  outUsages & INSP_OUTSIDE_BLOCK) {
            // a variable defined inside the region used outside
            //&fprintf(dumpOut,"%% ** variable '%s' defined inside the region used outside", varRef->symRef->name);
            return(EXTRACT_LOCAL_OUT_ARGUMENT);
        } else {
            return(EXTRACT_NONE);
        }
    }
}

static ExtractCategory categorizeLocalVariableExtraction(
    ProgramGraphNode *program,
    ProgramGraphNode *varRef
) {
    ExtractCategory category;

    category = categorizeLocalVariableExtraction0(program, varRef);
    //&log_trace("extraction categorized to %s", miscellaneousName[category]);
    if (isStructOrUnion(varRef)
        && category!=EXTRACT_NONE && category!=EXTRACT_LOCAL_VAR) {
        return(EXTRACT_ADDRESS_ARGUMENT);
    }
    return(category);
}

static unsigned toogleInOutBlock(unsigned *pos) {
    if (*pos == INSP_INSIDE_BLOCK)
        *pos = INSP_OUTSIDE_BLOCK;
    else if (*pos == INSP_OUTSIDE_BLOCK)
        *pos = INSP_INSIDE_BLOCK;
    else
        assert(0);

    return *pos;
}

static void extSetInOutBlockFields(ProgramGraphNode *program) {
    ProgramGraphNode *p;
    unsigned    pos;
    pos = INSP_OUTSIDE_BLOCK;
    for(p=program; p!=NULL; p=p->next) {
        if (p->symRef->b.symType == TypeBlockMarker) {
            toogleInOutBlock(&pos);
        }
        p->posBits = pos;
    }
}

static int extIsJumpInOutBlock(ProgramGraphNode *program) {
    ProgramGraphNode *p;
    for(p=program; p!=NULL; p=p->next) {
        assert(p->symRef!=NULL)
            if (p->symRef->b.symType==TypeLabel) {
                assert(p->ref!=NULL);
                if (p->ref->usage.base==UsageUsed || p->ref->usage.base==UsageFork) {
                    assert(p->jump != NULL);
                    if (p->posBits != p->jump->posBits) {
                        //&fprintf(dumpOut,"jump in/out at %s : %x\n",p->symRef->name, p);
                        return(1);
                    }
                }
            }
    }
    return(0);
}

#define EXT_LOCAL_VAR_REF(ppp) (                                        \
                                ppp->ref->usage.base==UsageDefined        \
                                &&  ppp->symRef->b.symType==TypeDefault \
                                &&  ppp->symRef->b.scope==ScopeAuto     \
                                )

static void extClassifyLocalVariables(ProgramGraphNode *program) {
    ProgramGraphNode *p;
    for(p=program; p!=NULL; p=p->next) {
        if (EXT_LOCAL_VAR_REF(p)) {
            p->classifBits = categorizeLocalVariableExtraction(program,p);
        }
    }
}

/*  linkName -> oName  == name of the variable
    -> oDecl  == declaration of the variable
    (decl contains declStar string(pt_), before the name)
    -> oDecla == declarator
    ( if cpName == 0, then do not copy the original var name )
*/
#define GetLocalVarStringFromLinkName(linkName,oDecla,oName,oDecl,declStar,cpName) { \
        char *s,*d,*dn,*dd;                                             \
                                                                        \
        log_trace("GetLocalVarStringFromLinkName '%s'",linkName);       \
        for(    s=linkName+1,d=oDecl,dd=oDecla;                         \
                *s!=0 && *s!=LINK_NAME_SEPARATOR;                       \
                s++,d++,dd++) {                                         \
            *d = *dd = *s;                                              \
        }                                                               \
        *dd = 0;                                                        \
        assert(*s);                                                     \
        for(s++; *s!=0 && *s!=LINK_NAME_SEPARATOR; s++,d++) {           \
            *d = *s;                                                    \
        }                                                               \
        assert(*s);                                                     \
        d = strmcpy(d, declStar);                                       \
        for(dn=oName,s++; *s!=0 && *s!=LINK_NAME_SEPARATOR; s++,dn++) { \
            *dn = *s;                                                   \
            if (cpName) *d++ = *s;                                      \
        }                                                               \
        assert(*s);                                                     \
        *dn = 0;                                                        \
        assert(*s);                                                     \
        for(s++; *s!=0 && *s!=LINK_NAME_SEPARATOR; s++,d++) {           \
            *d = *s;                                                    \
        }                                                               \
        *d = 0;                                                         \
    }

static void extReClassifyIOVars(ProgramGraphNode *program) {
    ProgramGraphNode  *p,*op;
    int uniqueOutFlag;

    op = NULL; uniqueOutFlag = 1;
    for(p=program; p!=NULL; p=p->next) {
        if (options.extractMode == EXTRACT_FUNCTION_ADDRESS_ARGS) {
            if (p->classifBits == EXTRACT_OUT_ARGUMENT
                ||  p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT
                ||  p->classifBits == EXTRACT_IN_OUT_ARGUMENT
                ) {
                p->classifBits = EXTRACT_ADDRESS_ARGUMENT;
            }
        } else if (options.extractMode == EXTRACT_FUNCTION) {
            if (p->classifBits == EXTRACT_OUT_ARGUMENT
                || p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT
                ) {
                if (op == NULL) op = p;
                else uniqueOutFlag = 0;
                // re-classify to in_out
            }
        }
    }

    if (op!=NULL && uniqueOutFlag) {
        if (op->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT) {
            op->classifBits = EXTRACT_LOCAL_RESULT_VALUE;
        } else {
            op->classifBits = EXTRACT_RESULT_VALUE;
        }
        return;
    }

    op = NULL; uniqueOutFlag = 1;
    for(p=program; p!=NULL; p=p->next) {
        if (options.extractMode == EXTRACT_FUNCTION) {
            if (p->classifBits == EXTRACT_IN_OUT_ARGUMENT) {
                if (op == NULL) op = p;
                else uniqueOutFlag = 0;
                // re-classify to in_out
            }
        }
    }

    if (op!=NULL && uniqueOutFlag) {
        op->classifBits = EXTRACT_IN_RESULT_VALUE;
        return;
    }

}

/* ************************** macro ******************************* */

static void generateNewMacroCall(ProgramGraphNode *program) {
    char dcla[TMP_STRING_SIZE];
    char decl[TMP_STRING_SIZE];
    char name[TMP_STRING_SIZE];
    ProgramGraphNode *p;
    int fFlag=1;

    rb[0]=0;

    sprintf(rb+strlen(rb),"\t%s",s_extractionName);

    for(p=program; p!=NULL; p=p->next) {
        if (    p->classifBits == EXTRACT_VALUE_ARGUMENT
                ||  p->classifBits == EXTRACT_IN_OUT_ARGUMENT
                ||  p->classifBits == EXTRACT_ADDRESS_ARGUMENT
                ||  p->classifBits == EXTRACT_OUT_ARGUMENT
                ||  p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT
                ||  p->classifBits == EXTRACT_RESULT_VALUE
                ||  p->classifBits == EXTRACT_IN_RESULT_VALUE
                ||  p->classifBits == EXTRACT_LOCAL_VAR) {
            GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
            sprintf(rb+strlen(rb), "%s%s", fFlag?"(":", " , name);
            fFlag = 0;
        }
    }
    sprintf(rb+strlen(rb), "%s);\n", fFlag?"(":"");

    assert(strlen(rb)<EXTRACT_GEN_BUFFER_SIZE-1);
    if (options.xref2) {
        ppcGenRecord(PPC_STRING_VALUE, rb);
    } else {
        fprintf(communicationChannel, "%s", rb);
    }
}

static void extGenNewMacroHead(ProgramGraphNode *program) {
    char dcla[TMP_STRING_SIZE];
    char decl[MAX_EXTRACT_FUN_HEAD_SIZE];
    char name[MAX_EXTRACT_FUN_HEAD_SIZE];
    ProgramGraphNode *p;
    int fFlag=1;

    rb[0]=0;

    sprintf(rb+strlen(rb),"#define %s",s_extractionName);
    for(p=program; p!=NULL; p=p->next) {
        if (    p->classifBits == EXTRACT_VALUE_ARGUMENT
                ||  p->classifBits == EXTRACT_IN_OUT_ARGUMENT
                ||  p->classifBits == EXTRACT_OUT_ARGUMENT
                ||  p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT
                ||  p->classifBits == EXTRACT_ADDRESS_ARGUMENT
                ||  p->classifBits == EXTRACT_RESULT_VALUE
                ||  p->classifBits == EXTRACT_IN_RESULT_VALUE
                ||  p->classifBits == EXTRACT_LOCAL_VAR) {
            GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
            sprintf(rb+strlen(rb), "%s%s", fFlag?"(":"," , name);
            fFlag = 0;
        }
    }
    sprintf(rb+strlen(rb), "%s) {\\\n", fFlag?"(":"");
    assert(strlen(rb)<EXTRACT_GEN_BUFFER_SIZE-1);
    if (options.xref2) {
        ppcGenRecord(PPC_STRING_VALUE, rb);
    } else {
        fprintf(communicationChannel, "%s", rb);
    }
}

static void generateNewMacroTail() {
    rb[0]=0;

    sprintf(rb+strlen(rb),"}\n\n");

    assert(strlen(rb)<EXTRACT_GEN_BUFFER_SIZE-1);
    if (options.xref2) {
        ppcGenRecord(PPC_STRING_VALUE, rb);
    } else {
        fprintf(communicationChannel, "%s", rb);
    }
}



/* ********************** C function **************************** */


static void generateNewFunctionCall(ProgramGraphNode *program) {
    char dcla[TMP_STRING_SIZE];
    char decl[TMP_STRING_SIZE];
    char name[TMP_STRING_SIZE];
    ProgramGraphNode *p;
    int fFlag=1;

    rb[0]=0;

    for(p=program; p!=NULL; p=p->next) {
        if (p->classifBits == EXTRACT_RESULT_VALUE
            || p->classifBits == EXTRACT_LOCAL_RESULT_VALUE
            || p->classifBits == EXTRACT_IN_RESULT_VALUE
            ) break;
    }
    if (p!=NULL) {
        GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
        if (p->classifBits == EXTRACT_LOCAL_RESULT_VALUE) {
            sprintf(rb+strlen(rb),"\t%s = ", decl);
        } else {
            sprintf(rb+strlen(rb),"\t%s = ", name);
        }
    } else {
        sprintf(rb+strlen(rb),"\t");
    }
    sprintf(rb+strlen(rb),"%s",s_extractionName);

    for(p=program; p!=NULL; p=p->next) {
        if (p->classifBits == EXTRACT_VALUE_ARGUMENT
            || p->classifBits == EXTRACT_IN_RESULT_VALUE) {
            GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
            sprintf(rb+strlen(rb), "%s%s", fFlag?"(":", " , name);
            fFlag = 0;
        }
    }
    for(p=program; p!=NULL; p=p->next) {
        if (    p->classifBits == EXTRACT_IN_OUT_ARGUMENT
                ||  p->classifBits == EXTRACT_ADDRESS_ARGUMENT
                ||  p->classifBits == EXTRACT_OUT_ARGUMENT
                ||  p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT
                ) {
            GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,
                                          options.olExtractAddrParPrefix,1);
            sprintf(rb+strlen(rb), "%s&%s", fFlag?"(":", " , name);
            fFlag = 0;
        }
    }
    sprintf(rb+strlen(rb), "%s);\n", fFlag?"(":"");
    assert(strlen(rb)<EXTRACT_GEN_BUFFER_SIZE-1);
    if (options.xref2) {
        ppcGenRecord(PPC_STRING_VALUE, rb);
    } else {
        fprintf(communicationChannel, "%s", rb);
    }
}

static void removeSymbolFromSymRefList(SymbolReferenceItemList **ll, SymbolReferenceItem *s) {
    SymbolReferenceItemList **r;
    r=ll;
    while (*r!=NULL) {
        if (isSmallerOrEqClass(getClassNumFromClassLinkName((*r)->d->name, noFileIndex),
                               getClassNumFromClassLinkName(s->name, noFileIndex))) {
            *r = (*r)->next;
        } else {
            r= &(*r)->next;
        }
    }
}

static SymbolReferenceItemList *concatRefItemList(SymbolReferenceItemList **ll, SymbolReferenceItem *s) {
    SymbolReferenceItemList *refItemList;
    CX_ALLOC(refItemList, SymbolReferenceItemList);
    refItemList->d = s;
    refItemList->next = *ll;
    return refItemList;
}


/* Public for unittesting */
void addSymbolToSymRefList(SymbolReferenceItemList **ll, SymbolReferenceItem *s) {
    SymbolReferenceItemList *r;

    r = *ll;
    while (r!=NULL) {
        if (isSmallerOrEqClass(getClassNumFromClassLinkName(s->name, noFileIndex),
                               getClassNumFromClassLinkName(r->d->name, noFileIndex))) {
            return;
        }
        r= r->next;
    }
    // first remove all superceeded by this one
    /* TODO: WTF, what does superceed mean here and why should we remove them? */
    removeSymbolFromSymRefList(ll, s);
    *ll = concatRefItemList(ll, s);
}

static SymbolReferenceItemList *computeExceptionsThrownBetween(ProgramGraphNode *bb,
                                                           ProgramGraphNode *ee
                                                           ) {
    SymbolReferenceItemList *res, *excs, *catched, *noncatched, **cl;
    ProgramGraphNode *p, *e;
    int depth;

    catched = NULL; noncatched = NULL; cl = &catched;
    for(p=bb; p!=NULL && p!=ee; p=p->next) {
        if (p->symRef->b.symType == TypeTryCatchMarker && p->ref->usage.base == UsageTryCatchBegin) {
            depth = 0;
            for(e=p; e!=NULL && e!=ee; e=e->next) {
                if (e->symRef->b.symType == TypeTryCatchMarker) {
                    if (e->ref->usage.base == UsageTryCatchBegin) depth++;
                    else depth --;
                    if (depth == 0) break;
                }
            }
            if (depth==0) {
                // add exceptions thrown in block
                for(excs=computeExceptionsThrownBetween(p->next,e); excs!=NULL; excs=excs->next){
                    addSymbolToSymRefList(cl, excs->d);
                }
            }
        }
        if (p->ref->usage.base == UsageThrown) {
            // thrown exception add it to list
            addSymbolToSymRefList(cl, p->symRef);
        } else if (p->ref->usage.base == UsageCatched) {
            // catched, remove it from list
            removeSymbolFromSymRefList(&catched, p->symRef);
            cl = &noncatched;
        }
    }
    res = catched;
    for(excs=noncatched; excs!=NULL; excs=excs->next) {
        addSymbolToSymRefList(&res, excs->d);
    }
    return(res);
}

static SymbolReferenceItemList *computeExceptionsThrownInBlock(ProgramGraphNode *program) {
    ProgramGraphNode  *pp, *ee;
    for(pp=program; pp!=NULL && pp->symRef->b.symType!=TypeBlockMarker; pp=pp->next) ;
    if (pp==NULL)
        return(NULL);
    for(ee=pp->next; ee!=NULL && ee->symRef->b.symType!=TypeBlockMarker; ee=ee->next) ;
    return(computeExceptionsThrownBetween(pp, ee));
}

static void extractSprintThrownExceptions(char *nhead, ProgramGraphNode *program) {
    SymbolReferenceItemList     *exceptions, *ee;
    int                     nhi;
    char                    *sname;
    nhi = 0;
    exceptions = computeExceptionsThrownInBlock(program);
    if (exceptions!=NULL) {
        sprintf(nhead+nhi, " throws");
        nhi += strlen(nhead+nhi);
        for(ee=exceptions; ee!=NULL; ee=ee->next) {
            sname = lastOccurenceInString(ee->d->name, '$');
            if (sname==NULL) sname = lastOccurenceInString(ee->d->name, '/');
            if (sname==NULL) {
                sname = ee->d->name;
            } else {
                sname ++;
            }
            sprintf(nhead+nhi, "%s%s", ee==exceptions?" ":", ", sname);
            nhi += strlen(nhead+nhi);
        }
    }
}

static void generateNewFunctionHead(ProgramGraphNode *program) {
    char nhead[MAX_EXTRACT_FUN_HEAD_SIZE];
    int nhi, ldclaLen;
    char ldcla[TMP_STRING_SIZE];
    char dcla[TMP_STRING_SIZE];
    char decl[MAX_EXTRACT_FUN_HEAD_SIZE];
    char name[MAX_EXTRACT_FUN_HEAD_SIZE];
    ProgramGraphNode  *p;
    int fFlag=1;

    rb[0]=0;

    /* function header */

    nhi = 0;
    sprintf(nhead+nhi,"%s",s_extractionName);
    nhi += strlen(nhead+nhi);
    for(p=program; p!=NULL; p=p->next) {
        if (p->classifBits == EXTRACT_VALUE_ARGUMENT
            || p->classifBits == EXTRACT_IN_RESULT_VALUE
            ) {
            GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
            sprintf(nhead+nhi, "%s%s", fFlag?"(":", " , decl);
            nhi += strlen(nhead+nhi);
            fFlag = 0;
        }
    }
    for(p=program; p!=NULL; p=p->next) {
        if (    p->classifBits == EXTRACT_IN_OUT_ARGUMENT
                ||  p->classifBits == EXTRACT_OUT_ARGUMENT
                ||  p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT
                ) {
            GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,
                                          options.olExtractAddrParPrefix,1);
            sprintf(nhead+nhi, "%s%s", fFlag?"(":", " , decl);
            nhi += strlen(nhead+nhi);
            fFlag = 0;
        }
    }
    for(p=program; p!=NULL; p=p->next) {
        if (p->classifBits == EXTRACT_ADDRESS_ARGUMENT) {
            GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,
                                          EXTRACT_REFERENCE_ARG_STRING, 1);
            sprintf(nhead+nhi, "%s%s", fFlag?"(":", " , decl);
            nhi += strlen(nhead+nhi);
            fFlag = 0;
        }
    }
    sprintf(nhead+nhi, "%s)", fFlag?"(":"");
    nhi += strlen(nhead+nhi);

    //throws
    extractSprintThrownExceptions(nhead+nhi, program);
    nhi += strlen(nhead+nhi);

    if (LANGUAGE(LANG_JAVA)) {
        // this makes renaming after extraction much faster
        sprintf(rb+strlen(rb), "private ");
    }

    if (LANGUAGE(LANG_JAVA) && (s_javaExtractFromFunctionMods&AccessStatic)==0) {
        ; // sprintf(rb+strlen(rb), "");
    } else {
        sprintf(rb+strlen(rb), "static ");
    }
    for(p=program; p!=NULL; p=p->next) {
        if (p->classifBits == EXTRACT_RESULT_VALUE
            || p->classifBits == EXTRACT_LOCAL_RESULT_VALUE
            || p->classifBits == EXTRACT_IN_RESULT_VALUE) break;
    }
    if (p==NULL) {
        sprintf(rb+strlen(rb),"void %s",nhead);
    } else {
        GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,nhead,0);
        sprintf(rb+strlen(rb), "%s", decl);
    }

    /* function body */

    sprintf(rb+strlen(rb), " {\n");
    ldcla[0] = 0; ldclaLen = 0; fFlag = 1;
    for(p=program; p!=NULL; p=p->next) {
        if (        p->classifBits == EXTRACT_IN_OUT_ARGUMENT
                    ||  p->classifBits == EXTRACT_LOCAL_VAR
                    ||  p->classifBits == EXTRACT_OUT_ARGUMENT
                    ||  p->classifBits == EXTRACT_RESULT_VALUE
                    ||  (   p->symRef->b.storage == StorageExtern
                            && p->ref->usage.base == UsageDeclared)
                    ) {
            GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
            if (strcmp(ldcla,dcla)==0) {
                sprintf(rb+strlen(rb), ",%s",decl+ldclaLen);
            } else {
                strcpy(ldcla,dcla); ldclaLen=strlen(ldcla);
                if (fFlag) sprintf(rb+strlen(rb), "\t%s",decl);
                else sprintf(rb+strlen(rb), ";\n\t%s",decl);
            }
            if (p->classifBits == EXTRACT_IN_OUT_ARGUMENT) {
                sprintf(rb+strlen(rb), " = %s%s", options.olExtractAddrParPrefix, name);
            }
            fFlag = 0;
        }
    }
    if (fFlag == 0) sprintf(rb+strlen(rb), ";\n");
    assert(strlen(rb)<EXTRACT_GEN_BUFFER_SIZE-1);
    if (options.xref2) {
        ppcGenRecord(PPC_STRING_VALUE, rb);
    } else {
        fprintf(communicationChannel, "%s", rb);
    }
}

static void generateNewFunctionTail(ProgramGraphNode *program) {
    char                dcla[TMP_STRING_SIZE];
    char                decl[TMP_STRING_SIZE];
    char                name[TMP_STRING_SIZE];
    ProgramGraphNode  *p;

    rb[0]=0;

    for(p=program; p!=NULL; p=p->next) {
        if (    p->classifBits == EXTRACT_IN_OUT_ARGUMENT
                ||  p->classifBits == EXTRACT_OUT_ARGUMENT
                ||  p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT
                ) {
            GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
            sprintf(rb+strlen(rb), "\t%s%s = %s;\n", options.olExtractAddrParPrefix,
                    name, name);
        }
    }
    for(p=program; p!=NULL; p=p->next) {
        if (p->classifBits == EXTRACT_RESULT_VALUE
            || p->classifBits == EXTRACT_IN_RESULT_VALUE
            || p->classifBits == EXTRACT_LOCAL_RESULT_VALUE
            ) {
            GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
            sprintf(rb+strlen(rb), "\n\treturn %s;\n", name);
        }
    }
    sprintf(rb+strlen(rb),"}\n\n");
    assert(strlen(rb)<EXTRACT_GEN_BUFFER_SIZE-1);
    if (options.xref2) {
        ppcGenRecord(PPC_STRING_VALUE, rb);
    } else {
        fprintf(communicationChannel, "%s", rb);
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
    char dcla[TMP_STRING_SIZE];
    char decl[TMP_STRING_SIZE];
    char name[TMP_STRING_SIZE];
    ProgramGraphNode *p;
    char *classname;
    int fFlag=1;

    rb[0]=0;

    classname = makeNewClassName();

    // constructor invocation
    sprintf(rb+strlen(rb),"\t\t%s %s = new %s",classname, s_extractionName,classname);
    fFlag = 1;
    for(p=program; p!=NULL; p=p->next) {
        if (p->classifBits == EXTRACT_IN_OUT_ARGUMENT) {
            GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
            sprintf(rb+strlen(rb), "%s%s", fFlag?"(":", " , name);
            fFlag = 0;
        }
    }
    sprintf(rb+strlen(rb), "%s);\n", fFlag?"(":"");

    // "perform" invocation
    for(p=program; p!=NULL; p=p->next) {
        if (p->classifBits == EXTRACT_RESULT_VALUE
            || p->classifBits == EXTRACT_IN_RESULT_VALUE
            || p->classifBits == EXTRACT_LOCAL_RESULT_VALUE
            ) break;
    }
    if (p!=NULL) {
        GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
        if (p->classifBits == EXTRACT_LOCAL_RESULT_VALUE) {
            sprintf(rb+strlen(rb),"\t\t%s = ", decl);
        } else {
            sprintf(rb+strlen(rb),"\t\t%s = ", name);
        }
    } else {
        sprintf(rb+strlen(rb),"\t\t");
    }
    sprintf(rb+strlen(rb),"%s.perform",s_extractionName);

    fFlag=1;
    for(p=program; p!=NULL; p=p->next) {
        if (p->classifBits == EXTRACT_VALUE_ARGUMENT
            || p->classifBits == EXTRACT_IN_RESULT_VALUE) {
            GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
            sprintf(rb+strlen(rb), "%s%s", fFlag?"(":", " , name);
            fFlag = 0;
        }
    }
    sprintf(rb+strlen(rb), "%s);\n", fFlag?"(":"");

    sprintf(rb+strlen(rb), "\t\t");
    // 'out' arguments value recovering
    for(p=program; p!=NULL; p=p->next) {
        if (    p->classifBits == EXTRACT_IN_OUT_ARGUMENT
                ||  p->classifBits == EXTRACT_OUT_ARGUMENT
                ||  p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT
                ) {
            GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,
                                          "",1);
            if (p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT) {
                sprintf(rb+strlen(rb), "%s=%s.%s; ", decl, s_extractionName, name);
            } else {
                sprintf(rb+strlen(rb), "%s=%s.%s; ", name, s_extractionName, name);
            }
            fFlag = 0;
        }
    }
    sprintf(rb+strlen(rb), "\n");
    sprintf(rb+strlen(rb),"\t\t%s = null;\n", s_extractionName);

    assert(strlen(rb)<EXTRACT_GEN_BUFFER_SIZE-1);
    if (options.xref2) {
        ppcGenRecord(PPC_STRING_VALUE, rb);
    } else {
        fprintf(communicationChannel, "%s", rb);
    }
}

static void extJavaGenNewClassHead(ProgramGraphNode *program) {
    char nhead[MAX_EXTRACT_FUN_HEAD_SIZE];
    int nhi, ldclaLen;
    char ldcla[TMP_STRING_SIZE];
    char dcla[TMP_STRING_SIZE];
    char *classname;
    char decl[MAX_EXTRACT_FUN_HEAD_SIZE];
    char name[MAX_EXTRACT_FUN_HEAD_SIZE];
    ProgramGraphNode *p;
    int fFlag=1;

    rb[0]=0;

    classname = makeNewClassName();

    // class header
    sprintf(rb+strlen(rb), "\t");
    if (s_javaExtractFromFunctionMods & AccessStatic){
        sprintf(rb+strlen(rb), "static ");
    }
    sprintf(rb+strlen(rb), "class %s {\n", classname);
    //sprintf(rb+strlen(rb), "\t\t// %s 'out' arguments\n", s_opt.extractName);
    for(p=program; p!=NULL; p=p->next) {
        if (p->classifBits == EXTRACT_OUT_ARGUMENT
            || p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT
            || p->classifBits == EXTRACT_IN_OUT_ARGUMENT
            ) {
            GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,
                                          "",1);
            sprintf(rb+strlen(rb), "\t\t%s;\n", decl);
        }
    }

    // the constructor
    //sprintf(rb+strlen(rb),"\t\t// constructor for %s 'in-out' args\n",s_opt.extractName);
    sprintf(rb+strlen(rb), "\t\t%s", classname);
    fFlag = 1;
    for(p=program; p!=NULL; p=p->next) {
        if (    p->classifBits == EXTRACT_IN_OUT_ARGUMENT){
            GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,
                                          "",1);
            sprintf(rb+strlen(rb), "%s%s", fFlag?"(":", " , decl);
            fFlag = 0;
        }
    }
    sprintf(rb+strlen(rb), "%s) {", fFlag?"(":"");
    fFlag = 1;
    for(p=program; p!=NULL; p=p->next) {
        if (    p->classifBits == EXTRACT_IN_OUT_ARGUMENT){
            GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,
                                          "",1);
            if (fFlag) sprintf(rb+strlen(rb), "\n\t\t\t");
            sprintf(rb+strlen(rb), "this.%s = %s; ", name, name);
            fFlag = 0;
        }
    }
    if (! fFlag) sprintf(rb+strlen(rb),"\n\t\t");
    sprintf(rb+strlen(rb),"}\n");
    //sprintf(rb+strlen(rb),"\t\t// perform with %s 'in' args\n",s_opt.extractName);

    // the "perform" method
    nhi = 0; fFlag = 1;
    sprintf(nhead+nhi,"%s", "perform");
    nhi += strlen(nhead+nhi);
    for(p=program; p!=NULL; p=p->next) {
        if (p->classifBits == EXTRACT_VALUE_ARGUMENT
            || p->classifBits == EXTRACT_IN_RESULT_VALUE) {
            GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,
                                          "", 1);
            sprintf(nhead+nhi, "%s%s", fFlag?"(":", " , decl);
            nhi += strlen(nhead+nhi);
            fFlag = 0;
        }
    }
    sprintf(nhead+nhi, "%s)", fFlag?"(":"");
    nhi += strlen(nhead+nhi);

    // throws
    extractSprintThrownExceptions(nhead+nhi, program);
    nhi += strlen(nhead+nhi);

    for(p=program; p!=NULL; p=p->next) {
        if (p->classifBits == EXTRACT_RESULT_VALUE
            || p->classifBits == EXTRACT_IN_RESULT_VALUE) break;
    }
    if (p==NULL) {
        sprintf(rb+strlen(rb),"\t\tvoid %s", nhead);
    } else {
        GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,nhead,0);
        sprintf(rb+strlen(rb), "\t\t%s", decl);
    }

    /* function body */

    sprintf(rb+strlen(rb), " {\n");
    ldcla[0] = 0; ldclaLen = 0; fFlag = 1;
    for(p=program; p!=NULL; p=p->next) {
        if (    p->classifBits == EXTRACT_LOCAL_VAR
                ||  p->classifBits == EXTRACT_RESULT_VALUE
                ||  (   p->symRef->b.storage == StorageExtern
                        && p->ref->usage.base == UsageDeclared)
                ) {
            GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
            if (strcmp(ldcla,dcla)==0) {
                sprintf(rb+strlen(rb), ",%s",decl+ldclaLen);
            } else {
                strcpy(ldcla,dcla); ldclaLen=strlen(ldcla);
                if (fFlag) sprintf(rb+strlen(rb), "\t\t\t%s",decl);
                else sprintf(rb+strlen(rb), ";\n\t\t\t%s",decl);
            }
            fFlag = 0;
        }
    }
    if (fFlag == 0) sprintf(rb+strlen(rb), ";\n");
    assert(strlen(rb)<EXTRACT_GEN_BUFFER_SIZE-1);
    if (options.xref2) {
        ppcGenRecord(PPC_STRING_VALUE, rb);
    } else {
        fprintf(communicationChannel, "%s", rb);
    }
}

static void extJavaGenNewClassTail(ProgramGraphNode *program) {
    char                dcla[TMP_STRING_SIZE];
    char                decl[TMP_STRING_SIZE];
    char                name[TMP_STRING_SIZE];
    ProgramGraphNode  *p;
    int                 fFlag;

    rb[0]=0;


    // local 'out' arguments value setting
    fFlag = 1;
    for(p=program; p!=NULL; p=p->next) {
        if (p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT) {
            GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,
                                          "",1);
            if (fFlag) sprintf(rb+strlen(rb), "\t\t\t");
            sprintf(rb+strlen(rb), "this.%s=%s; ", name, name);
            fFlag = 0;
        }
    }
    if (! fFlag) sprintf(rb+strlen(rb), "\n");


    for(p=program; p!=NULL; p=p->next) {
        if (p->classifBits == EXTRACT_RESULT_VALUE
            || p->classifBits == EXTRACT_LOCAL_RESULT_VALUE
            || p->classifBits == EXTRACT_IN_RESULT_VALUE
            ) {
            GetLocalVarStringFromLinkName(p->symRef->name,dcla,name,decl,"",1);
            sprintf(rb+strlen(rb), "\t\t\treturn(%s);\n", name);
        }
    }
    sprintf(rb+strlen(rb),"\t\t}\n\t}\n\n");

    assert(strlen(rb)<EXTRACT_GEN_BUFFER_SIZE-1);
    if (options.xref2) {
        ppcGenRecord(PPC_STRING_VALUE, rb);
    } else {
        fprintf(communicationChannel, "%s", rb);
    }
}

/* ******************************************************************* */

static bool extractJavaIsNewClassNecessary(ProgramGraphNode *program) {
    ProgramGraphNode  *p;
    for(p=program; p!=NULL; p=p->next) {
        if (p->classifBits == EXTRACT_OUT_ARGUMENT
            || p->classifBits == EXTRACT_LOCAL_OUT_ARGUMENT
            || p->classifBits == EXTRACT_IN_OUT_ARGUMENT
            ) break;
    }
    if (p==NULL)
        return false;
    return true;
}

static void makeExtraction(void) {
    ProgramGraphNode *program;
    bool needToExtractNewClass = false;

    if (s_cp.cxMemoryIndexAtFunctionBegin > s_cps.cxMemoryIndexAtBlockBegin
        || s_cps.cxMemoryIndexAtBlockBegin > s_cps.cxMemoryIndexAtBlockEnd
        || s_cps.cxMemoryIndexAtBlockEnd > s_cp.cxMemoryIndexAtFunctionEnd
        || s_cps.workMemoryIndexAtBlockBegin != s_cps.workMemiAtBlockEnd) {
        errorMessage(ERR_ST, "Region / program structure mismatch");
        return;
    }
    log_trace("!cxMemories: funBeg, blockBeb, blockEnd, funEnd: %x, %x, %x, %x", s_cp.cxMemoryIndexAtFunctionBegin, s_cps.cxMemoryIndexAtBlockBegin, s_cps.cxMemoryIndexAtBlockEnd, s_cp.cxMemoryIndexAtFunctionEnd);
    assert(s_cp.cxMemoryIndexAtFunctionBegin);
    assert(s_cps.cxMemoryIndexAtBlockBegin);
    assert(s_cps.cxMemoryIndexAtBlockEnd);
    assert(s_cp.cxMemoryIndexAtFunctionEnd);

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

    CX_ALLOCC(rb, EXTRACT_GEN_BUFFER_SIZE, char);

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
        ppcValueRecord(PPC_INT_VALUE, s_cp.funBegPosition, "");
    } else {
        fprintf(communicationChannel,"!%d!\n", s_cp.funBegPosition);
        fflush(communicationChannel);
    }

    if (options.xref2)
        ppcEnd(PPC_EXTRACTION_DIALOG);
}


void actionsBeforeAfterExternalDefinition(void) {
    if (s_cps.cxMemoryIndexAtBlockEnd != 0
        // you have to check for matching class method
        // i.e. for case 'void mmm() { //blockbeg; ...; //blockend; class X { mmm(){}!!}; }'
        && s_cp.cxMemoryIndexAtFunctionBegin != 0
        && s_cp.cxMemoryIndexAtFunctionBegin <= s_cps.cxMemoryIndexAtBlockBegin
        // is it an extraction action ?
        && options.server_operation == OLO_EXTRACT
        && (! s_cps.extractProcessedFlag))
    {
        // O.K. make extraction
        s_cp.cxMemoryIndexAtFunctionEnd = cxMemory->index;
        makeExtraction();
        s_cps.extractProcessedFlag = 1;
        /* here should be a longjmp to stop file processing !!!! */
        /* No, all this extraction should be after parsing ! */
    }
    s_cp.cxMemoryIndexAtFunctionBegin = cxMemory->index;
    if (includeStackPointer) {                     // ??????? burk ????
        s_cp.funBegPosition = includeStack[0].lineNumber+1;
    } else {
        s_cp.funBegPosition = currentFile.lineNumber+1;
    }
}


void extractActionOnBlockMarker(void) {
    Position pos;
    if (s_cps.cxMemoryIndexAtBlockBegin == 0) {
        s_cps.cxMemoryIndexAtBlockBegin = cxMemory->index;
        s_cps.workMemoryIndexAtBlockBegin = currentBlock->outerBlock;
        if (LANGUAGE(LANG_JAVA)) {
            s_javaExtractFromFunctionMods = s_javaStat->methodModifiers;
        }
    } else {
        assert(s_cps.cxMemoryIndexAtBlockEnd == 0);
        s_cps.cxMemoryIndexAtBlockEnd = cxMemory->index;
        s_cps.workMemiAtBlockEnd = currentBlock->outerBlock;
    }
    pos = makePosition(currentFile.lexBuffer.buffer.fileNumber, 0, 0);
    addTrivialCxReference("Block", TypeBlockMarker, StorageDefault, &pos, UsageUsed);
}

void deleteContinueBreakSymbol(Symbol *symbol) {
    if (options.server_operation == OLO_EXTRACT)
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
