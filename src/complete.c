#include "complete.h"

#include "completion.h"
#include "cxfile.h"
#include "cxref.h"
#include "filetable.h"
#include "globals.h"
#include "list.h"
#include "log.h"
#include "misc.h"
#include "options.h"
#include "ppc.h"
#include "protocol.h"
#include "reftab.h"
#include "semact.h"
#include "stackmemory.h"
#include "type.h"
#include "session.h"
#include "yylex.h"


#define FULL_COMPLETION_INDENT_CHARS 2


typedef struct {
    struct completions *completions;
    Type                type;
} SymbolCompletionInfo;

typedef struct {
    struct completions *res;
    Storage storage;
} SymbolCompletionFunctionInfo;

ExpressionTokenType completionTypeForForStatement;


void initCompletions(Completions *completions, int length, Position position) {
    completions->idToProcessLength = length;
    completions->idToProcessPosition = position;
    completions->fullMatchFlag = false;
    completions->isCompleteFlag = false;
    completions->noFocusOnCompletions = false;
    completions->abortFurtherCompletions = false;
    completions->maxLen = 0;
    completions->alternativeIndex = 0;
}

void fillCompletionLine(CompletionLine *cline, char *string, Symbol *symbol, Type symbolType, short int margn, char **margs) {
    cline->string = string;
    cline->symbol = symbol;
    cline->symbolType = symbolType;
    cline->margn = margn;
    cline->margs = margs;
}

static void fillCompletionSymInfo(SymbolCompletionInfo *info, Completions *completions, Type type) {
    info->completions = completions;
    info->type        = type;
}

static void fillCompletionSymFunInfo(SymbolCompletionFunctionInfo *completionSymFunInfo, Completions *completions,
                                     enum storage storage) {
    completionSymFunInfo->res = completions;
    completionSymFunInfo->storage = storage;
}

static void formatFullCompletions(char *tt, int indent, int inipos) {
    int pos;
    char *nlpos, *p;

    log_trace("formatting '%s' indent==%d, inipos==%d, linelen==%d" ,tt, indent, inipos, options.olineLen);
    pos = inipos;
    nlpos=NULL;
    p = tt;
    for (;;) {
        while (pos<options.olineLen || nlpos==NULL) {
            p++; pos++;
            if (*p == 0)
                goto formatFullCompletionsEnd;
            if (*p ==' ' && pos>indent)
                nlpos = p;
        }
        *nlpos = '\n';
        pos = indent+(p-nlpos);
        nlpos=NULL;
    }
 formatFullCompletionsEnd:
    log_trace("result '%s'", tt);
    return;
}

static void sprintFullCompletionInfo(Completions* completions, int index, int indent) {
    int size, l, cindent, tempLength;
    bool typeDefinitionExpressionFlag;
    char tempString[COMPLETION_STRING_SIZE];
    char *ppc;

    ppc = ppcTmpBuff;
    if (completions->alternatives[index].symbolType == TypeUndefMacro)
        return;

    // remove parenthesis (if any)
    strcpy(tempString, completions->alternatives[index].string);
    tempLength = strlen(tempString);
    if (tempLength>0 && tempString[tempLength-1]==')' && completions->alternatives[index].symbolType!=TypeInheritedFullMethod) {
        tempLength--;
        tempString[tempLength]=0;
    }
    if (tempLength>0 && tempString[tempLength-1]=='(') {
        tempLength--;
        tempString[tempLength]=0;
    }
    sprintf(ppc, "%-*s:", indent+FULL_COMPLETION_INDENT_CHARS, tempString);
    cindent = strlen(ppc);
    ppc += strlen(ppc);
    size = COMPLETION_STRING_SIZE;
    l = 0;
    if (completions->alternatives[index].symbolType==TypeDefault) {
        assert(completions->alternatives[index].symbol && completions->alternatives[index].symbol->u.typeModifier);
        typeDefinitionExpressionFlag = true;
        if (completions->alternatives[index].symbol->storage == StorageTypedef) {
            sprintf(tempString, "typedef ");
            l = strlen(tempString);
            size -= l;
            typeDefinitionExpressionFlag = false;
        }
        char *pname = completions->alternatives[index].symbol->name;
        typeSPrint(tempString+l, &size, completions->alternatives[index].symbol->u.typeModifier, pname, ' ',
                   typeDefinitionExpressionFlag, SHORT_NAME, NULL);
    } else if (completions->alternatives[index].symbolType==TypeMacro) {
        macroDefinitionSPrintf(tempString, &size, "", completions->alternatives[index].string,
                      completions->alternatives[index].margn, completions->alternatives[index].margs, NULL);
    } else {
        assert(completions->alternatives[index].symbolType>=0 && completions->alternatives[index].symbolType<MAX_TYPE);
        sprintf(tempString,"%s", typeNamesTable[completions->alternatives[index].symbolType]);
    }

    formatFullCompletions(tempString, indent+FULL_COMPLETION_INDENT_CHARS+2, cindent);
    for (int i=0; tempString[i]; i++) {
        sprintf(ppc,"%c",tempString[i]);
        ppc += strlen(ppc);
        if (tempString[i] == '\n') {
            sprintf(ppc,"%-*s ",indent+FULL_COMPLETION_INDENT_CHARS, " ");
            ppc += strlen(ppc);
        }
    }
}

void olCompletionListInit(Position *originalPos) {
    olcxFreeOldCompletionItems(&sessionData.completionsStack);
    pushEmptySession(&sessionData.completionsStack);
    sessionData.completionsStack.top->callerPosition = *originalPos;
}


static int completionsWillPrintEllipsis(Completion *olc) {
    int max, ellipsis;
    LIST_LEN(max, Completion, olc);
    ellipsis = 0;
    if (max >= MAX_COMPLETIONS - 2 || max == options.maxCompletions) {
        ellipsis = 1;
    }
    return ellipsis;
}


static void printCompletionsBeginning(Completion *olc, int noFocus) {
    int tlen = 0;
    for (Completion *cc=olc; cc!=NULL; cc=cc->next) {
        tlen += strlen(cc->fullName);
        if (cc->next!=NULL) tlen++;
    }
    if (completionsWillPrintEllipsis(olc))
        tlen += 4;
    ppcBeginAllCompletions(noFocus, tlen);
}

static void printOneCompletion(Completion *olc) {
    fprintf(communicationChannel, "%s", olc->fullName);
}

static void printCompletionsEnding(Completion *olc) {
    if (completionsWillPrintEllipsis(olc)) {
        fprintf(communicationChannel,"\n...");
    }
    ppcEnd(PPC_ALL_COMPLETIONS);
}

void printCompletionsList(int noFocus) {
    Completion *cc, *olc;
    olc = sessionData.completionsStack.top->completions;
    printCompletionsBeginning(olc, noFocus);
    for(cc=olc; cc!=NULL; cc=cc->next) {
        printOneCompletion(cc);
        if (cc->next!=NULL) fprintf(communicationChannel,"\n");
    }
    printCompletionsEnding(olc);
}

void printCompletions(Completions* c) {
    int indent, max;

    // O.K. there will be a menu diplayed, clear the old one
    olCompletionListInit(&c->idToProcessPosition);
    if (c->alternativeIndex == 0) {
        if (options.serverOperation == OLO_SEARCH)
            ppcGenRecordWithNumeric(PPC_BOTTOM_INFORMATION, PPCA_BEEP, 0, "** No matches **");
        else
            ppcGenRecordWithNumeric(PPC_BOTTOM_INFORMATION, PPCA_BEEP, 0, "** No completion possible **");
        goto finishWithoutMenu;
    }
    if (!c->fullMatchFlag && c->alternativeIndex==1) {
        ppcGotoPosition(&sessionData.completionsStack.top->callerPosition);
        ppcGenRecord(PPC_SINGLE_COMPLETION, c->alternatives[0].string);
        goto finishWithoutMenu;
    }
    if (!c->fullMatchFlag && strlen(c->prefix) > c->idToProcessLength) {
        ppcGotoPosition(&sessionData.completionsStack.top->callerPosition);
        ppcGenRecord(PPC_SINGLE_COMPLETION, c->prefix);
        ppcGenRecordWithNumeric(PPC_BOTTOM_INFORMATION, PPCA_BEEP, 1, "Multiple completions");
        goto finishWithoutMenu;
    }

    indent = c->maxLen;
    if (options.olineLen - indent < MIN_COMPLETION_INDENT_REST) {
        indent = options.olineLen - MIN_COMPLETION_INDENT_REST;
        if (indent < MIN_COMPLETION_INDENT) indent = MIN_COMPLETION_INDENT;
    }
    if (indent > MAX_COMPLETION_INDENT)
        indent = MAX_COMPLETION_INDENT;
    if (c->alternativeIndex > options.maxCompletions)
        max = options.maxCompletions;
    else
        max = c->alternativeIndex;
    for(int ii=0; ii<max; ii++) {
        sprintFullCompletionInfo(c, ii, indent);
        Reference ref;
        sessionData.completionsStack.top->completions = completionListPrepend(
            sessionData.completionsStack.top->completions, c->alternatives[ii].string, ppcTmpBuff, c->alternatives[ii].symbol, NULL, &ref, c->alternatives[ii].symbolType,
            NO_FILE_NUMBER);
    }
    olCompletionListReverse();
    printCompletionsList(c->noFocusOnCompletions);
    fflush(communicationChannel);
    return;
 finishWithoutMenu:
    sessionData.completionsStack.top = sessionData.completionsStack.top->previous;
    fflush(communicationChannel);
}

/* *********************************************************************** */

static bool isTheSameSymbol(CompletionLine *c1, CompletionLine *c2) {
    if (strcmp(c1->string, c2->string) != 0)
        return false;
    if (c1->symbolType != c2->symbolType)
        return false;
    return true;
}

static int completionOrderCmp(CompletionLine *c1, CompletionLine *c2) {
    int     l1, l2;
    char    *s1, *s2;

    // exact matches goes first
    s1 = strchr(c1->string, '(');
    if (s1 == NULL) l1 = strlen(c1->string);
    else l1 = s1 - c1->string;
    s2 = strchr(c2->string, '(');
    if (s2 == NULL) l2 = strlen(c2->string);
    else l2 = s2 - c2->string;
    if (l1 == collectedCompletions.idToProcessLength && l2 != collectedCompletions.idToProcessLength)
        return -1;
    if (l1 != collectedCompletions.idToProcessLength && l2 == collectedCompletions.idToProcessLength)
        return 1;
    return strcmp(c1->string, c2->string);
}

static bool reallyInsert(CompletionLine *a, int *aip, char *s, CompletionLine *t, bool orderFlag) {
    int ai;
    int l, r, x, c;

    ai = *aip;
    l  = 0;
    r  = ai - 1;
    assert(t->string == s);
    if (orderFlag) {
        // binary search
        while (l <= r) {
            x = (l + r) / 2;
            c = completionOrderCmp(t, &a[x]);
            if (c == 0) { /* identifier still in completions */
                return false;
            }
            if (c < 0)
                r = x - 1;
            else
                l = x + 1;
        }
        assert(l == r + 1);
    } else {
        // linear search
        for (l = 0; l < ai; l++) {
            if (isTheSameSymbol(t, &a[l]))
                return false;
        }
    }
    if (orderFlag) {
        //& for(i=ai-1; i>=l; i--) a[i+1] = a[i];
        // should be faster, but frankly the weak point is not here.
        memmove(a + l + 1, a + l, sizeof(CompletionLine) * (ai - l));
    } else {
        l = ai;
    }

    a[l]        = *t;
    a[l].string = s;
    if (ai < MAX_COMPLETIONS - 2)
        ai++;
    *aip = ai;

    return true;
}

static void computeComPrefix(char *d, char *s) {
    while (*d == *s /*& && *s!='(' || (!options.completionCaseSensitive && tolower(*d)==tolower(*s)) &*/) {
        if (*d == 0) return;
        d++; s++;
    }
    *d = 0;
}

static bool completionTestPrefix(Completions *ci, char *s) {
    char *d;
    d = ci->idToProcess;
    while (*d == *s || (!options.completionCaseSensitive && tolower(*d)==tolower(*s))) {
        if (*d == 0) {
            ci->isCompleteFlag = true;    /* complete, but maybe not unique*/
            return 0;
        }
        d++; s++;
    }
    if (*d == 0)
        return false;
    return true;
}

static bool stringContainsCaseInsensitive(char *s1, char *s2) {
    char *p,*a,*b;
    for (p=s1; *p; p++) {
        for(a=p,b=s2; tolower(*a)==tolower(*b); a++,b++) ;
        if (*b==0)
            return true;
    }
    return false;
}

static void completionInsertName(char *name, CompletionLine *completionLine, bool orderFlag,
                                 Completions *ci) {
    int len,l;
    //&completionLine->string  = name;
    name = completionLine->string;
    len = ci->idToProcessLength;
    if (ci->alternativeIndex == 0) {
        strcpy(ci->prefix, name);
        ci->alternatives[ci->alternativeIndex] = *completionLine;
        ci->alternatives[ci->alternativeIndex].string = name/*+len*/;
        ci->alternativeIndex++;
        ci->maxLen = strlen(name/*+len*/);
        ci->fullMatchFlag = ((len==ci->maxLen)
                             || (len==ci->maxLen-1 && name[len]=='(')
                             || (len==ci->maxLen-2 && name[len]=='('));
    } else {
        assert(ci->alternativeIndex < MAX_COMPLETIONS-1);
        if (reallyInsert(ci->alternatives, &ci->alternativeIndex, name/*+len*/, completionLine, orderFlag)) {
            ci->fullMatchFlag = false;
            l = strlen(name/*+len*/);
            if (l > ci->maxLen) ci->maxLen = l;
            computeComPrefix(ci->prefix, name);
        }
    }
}

static void completeName(char *name, CompletionLine *compLine, bool orderFlag,
                         Completions *ci) {
    if (name == NULL)
        return;
    if (completionTestPrefix(ci, name))
        return;
    completionInsertName(name, compLine, orderFlag, ci);
}

static void searchName(char *name, CompletionLine *compLine, int orderFlag,
                       Completions *completions) {
    if (name == NULL)
        return;

    if (options.olcxSearchString==NULL || *options.olcxSearchString==0) {
        // old fashioned search
        if (!stringContainsCaseInsensitive(name, completions->idToProcess))
            return;
    } else {
        // the new one
        // since 1.6.0 this may not work, because switching to
        // regular expressions
        if (!searchStringMatch(name, strlen(name)))
            return;
    }
    //&compLine->string = name;
    name = compLine->string;
    if (completions->alternativeIndex == 0) {
        completions->fullMatchFlag = false;
        completions->prefix[0]=0;
        completions->alternatives[completions->alternativeIndex] = *compLine;
        completions->alternatives[completions->alternativeIndex].string = name;
        completions->alternativeIndex++;
        completions->maxLen = strlen(name);
    } else {
        assert(completions->alternativeIndex < MAX_COMPLETIONS-1);
        if (reallyInsert(completions->alternatives, &completions->alternativeIndex, name, compLine, 1)) {
            completions->fullMatchFlag = false;
            int l = strlen(name);
            if (l > completions->maxLen)
                completions->maxLen = l;
        }
    }
}

void processName(char *name, CompletionLine *line, bool orderFlag, Completions *c) {
    Completions *ci = (Completions *) c;
    if (options.serverOperation == OLO_SEARCH) {
        searchName(name, line, orderFlag, ci);
    } else {
        completeName(name, line, orderFlag, ci);
    }
}

// NOTE: Mapping function
static void completeFun(Symbol *symbol, void *c) {
    SymbolCompletionInfo *completionInfo = (SymbolCompletionInfo *)c;
    CompletionLine compLine;

    assert(symbol && completionInfo);
    if (symbol->type != completionInfo->type)
        return;
    log_trace("testing %s", symbol->linkName);
    if (symbol->type != TypeMacro) {
        fillCompletionLine(&compLine, symbol->name, symbol, symbol->type, 0, NULL);
    } else {
        if (symbol->u.mbody == NULL) {
            fillCompletionLine(&compLine, symbol->name, symbol, TypeUndefMacro, 0, NULL);
        } else {
            fillCompletionLine(&compLine, symbol->name, symbol, symbol->type, symbol->u.mbody->argCount,
                               symbol->u.mbody->argumentNames);
        }
    }
    processName(symbol->name, &compLine, true, completionInfo->completions);
}

static void completeFunctionOrMethodName(Completions *c, bool orderFlag, int vlevel, Symbol *r, Symbol *vFunCl) {
    CompletionLine compLine;
    int cnamelen;
    char *cn, *cname, *psuff;

    cname = r->name;
    cnamelen = strlen(cname);
    if (!options.completeParenthesis) {
        cn = cname;
    } else {
        assert(r->u.typeModifier!=NULL);
        if (r->u.typeModifier!=NULL && r->u.typeModifier->u.f.args == NULL) {
            psuff = "()";
        } else {
            psuff = "(";
        }
        cn = stackMemoryAlloc(cnamelen+strlen(psuff)+1);
        strcpy(cn, cname);
        strcpy(cn+cnamelen, psuff);
    }
    fillCompletionLine(&compLine, cn, r, TypeDefault,0,NULL);
    processName(cn, &compLine, orderFlag, c);
}

// NOTE: Mapping function
static void symbolCompletionFunction(Symbol *symbol, void *i) {
    CompletionLine completionLine;
    char    *completionName;

    SymbolCompletionFunctionInfo *info = (SymbolCompletionFunctionInfo *) i;

    assert(symbol);
    if (symbol->type != TypeDefault)
        return;

    if (info->storage==StorageTypedef && symbol->storage!=StorageTypedef)
        return;

    completionName = symbol->name;
    if (completionName!=NULL) {
        if (symbol->type == TypeDefault && symbol->u.typeModifier!=NULL && symbol->u.typeModifier->type == TypeFunction) {
            completeFunctionOrMethodName(info->res, true, 0, symbol, NULL);
        } else {
            fillCompletionLine(&completionLine, completionName, symbol, symbol->type, 0, NULL);
            processName(completionName, &completionLine, 1, info->res);
        }
    }
}

void collectStructsCompletions(Completions *completions) {
    SymbolCompletionInfo info;

    fillCompletionSymInfo(&info, completions, TypeStruct);
    symbolTableMapWithPointer(symbolTable, completeFun, (void*) &info);
    fillCompletionSymInfo(&info, completions, TypeUnion);
    symbolTableMapWithPointer(symbolTable, completeFun, (void*) &info);
}

static void completeMemberNames(Completions *completions, Symbol *symbol) {
    CompletionLine completionLine;
    Symbol *foundSymbol;
    StructMemberFindInfo info;
    char *name;

    if (symbol==NULL)
        return;

    bool orderFlag = completions->idToProcess[0] != 0;

    assert(symbol->u.structSpec);
    initFind(symbol, &info);

    for(;;) {
        // this is in fact about not cutting all members of the struct,
        Result result = findStructureMemberSymbol(&foundSymbol, &info, NULL);
        if (result != RESULT_OK)
            break;

        /* because constructors are not inherited */
        assert(foundSymbol);

        name = foundSymbol->name;
        if (name!=NULL && *name != 0 && foundSymbol->type != TypeError
            // Hmm. I hope it will not filter out something important
            && !symbolShouldBeHiddenFromSearchResults(foundSymbol->linkName)
        ) {
            assert(info.currentStructure && info.currentStructure->u.structSpec);
            assert(foundSymbol->type == TypeDefault);
            fillCompletionLine(&completionLine, name, foundSymbol, TypeDefault, 0, NULL);
            processName(name, &completionLine, orderFlag, completions);
        }
    }
    if (completions->idToProcess[0] == 0)
        completions->prefix[0] = 0; // no common prefix completed
}


void collectStructMemberCompletions(Completions *completions) {
    TypeModifier *structure;
    Symbol *symbol;
    assert(structMemberCompletionType);
    structure = structMemberCompletionType;
    if (structure->type == TypeStruct || structure->type == TypeUnion) {
        symbol = structure->u.t;
        assert(symbol);
        completeMemberNames(completions, symbol);
    }
    structMemberCompletionType = &errorModifier;
}

static void completeFromSymTab(Completions *completions, unsigned storage){
    SymbolCompletionFunctionInfo  info;

    fillCompletionSymFunInfo(&info, completions, storage);
    symbolTableMapWithPointer(symbolTable, symbolCompletionFunction, (void*) &info);
}

void collectEnumsCompletions(Completions *completions) {
    SymbolCompletionInfo info;

    fillCompletionSymInfo(&info, completions, TypeEnum);
    symbolTableMapWithPointer(symbolTable, completeFun, (void*) &info);
}

void collectLabelsCompletions(Completions *completions) {
    SymbolCompletionInfo info;

    fillCompletionSymInfo(&info, completions, TypeLabel);
    symbolTableMapWithPointer(symbolTable, completeFun, (void*) &info);
}

void completeMacros(Completions *completions) {
    SymbolCompletionInfo info;

    fillCompletionSymInfo(&info, completions, TypeMacro);
    symbolTableMapWithPointer(symbolTable, completeFun, (void*) &info);
}

void collectTypesCompletions(Completions *completions) {
    completeFromSymTab(completions, StorageTypedef);
}

void collectOthersCompletions(Completions *completions) {
    completeFromSymTab(completions, StorageDefault);
    completeMacros(completions);      /* handle macros as functions */
}

/* very costly function in time !!!! */
static Symbol *getSymbolFromReference(Reference *reference) {
    char *symbolLinkName;

    // first visit all references, looking for symbol link name
    // TODO: Feels like an API function... getReferenceItemFor(Reference *r)
    for (int i=getNextExistingReferenceItem(0); i != -1; i = getNextExistingReferenceItem(i+1)) {
        ReferenceItem *referencesItem = getReferenceItem(i);
        if (referencesItem!=NULL) {
            for (Reference *r=referencesItem->references; r!=NULL; r=r->next) {
                if (r == reference) {
                    symbolLinkName = referencesItem->linkName;
                    goto found;
                }
            }
        }
    }
    return NULL;

 found:
    // now look symbol table to find the symbol, berk!
    for (int i=0; i<symbolTable->size; i++) {
        for (Symbol *symbol=symbolTable->tab[i]; symbol!=NULL; symbol=symbol->next) {
            if (strcmp(symbol->linkName, symbolLinkName)==0)
                return symbol;
        }
    }
    return NULL;
}

static bool isEqualType(TypeModifier *t1, TypeModifier *t2) {
    TypeModifier *s1,*s2;

    assert(t1 && t2);
    for (s1=t1,s2=t2; s1->next!=NULL && s2->next!=NULL; s1=s1->next,s2=s2->next) {
        if (s1->type!=s2->type)
            return false;
    }
    if (s1->next!=NULL || s2->next!=NULL)
        return false;
    if (s1->type != s2->type)
        return false;
    if (s1->type==TypeStruct || s1->type==TypeUnion || s1->type==TypeEnum) {
        if (s1->u.t != s2->u.t)
            return false;
    } else if (s1->type==TypeFunction) {
        Symbol *ss1, *ss2;
        for (ss1=s1->u.f.args, ss2=s2->u.f.args;
             ss1!=NULL && ss2!=NULL;
             ss1=ss1->next, ss2=ss2->next) {
            if (!isEqualType(ss1->u.typeModifier, ss2->u.typeModifier))
                return false;
        }
        if (ss1!=NULL||ss2!=NULL)
            return false;
    }
    return true;
}

static char *spComplFindNextRecord(ExpressionTokenType *token) {
    StructMemberFindInfo    rfs;
    Symbol        *r,*s;
    char *res;
    char            *cname;
    static char     *cnext="next";
    static char     *cprevious="previous";

    s = token->typeModifier->next->u.t;
    res = NULL;
    assert(s->u.structSpec);
    initFind(s, &rfs);
    for(;;) {
        Result rr = findStructureMemberSymbol(&r, &rfs, NULL);
        if (rr != RESULT_OK) break;
        assert(r);
        cname = r->name;
        if (cname!=NULL) {
            assert(rfs.currentStructure && rfs.currentStructure->u.structSpec);
            assert(r->type == TypeDefault);
            if (isEqualType(r->u.typeModifier, token->typeModifier)) {
                // there is a record of the same type
                if (res == NULL)
                    res = cname;
                else if (strcmp(cname,cnext)==0)
                    res = cnext;
                else if (res!=cnext&&strcmp(cname,"previous")==0)
                    res=cprevious;
            }
        }
    }
    return res;
}

static bool isForStatementCompletionSymbol(
    Completions *completions,
    ExpressionTokenType *token,
    Symbol **symbolP,
    char **nextRecord
) {
    if (options.serverOperation != OLO_COMPLETION)
        return false;
    if (token->typeModifier==NULL)
        return false;
    if (completions->idToProcessLength != 0)
        return false;
    if (token->typeModifier->type == TypePointer) {
        assert(token->typeModifier->next);
        if (token->typeModifier->next->type == TypeStruct) {
            if ((*symbolP = getSymbolFromReference(token->reference)) == NULL)
                return false;
            *nextRecord = spComplFindNextRecord(token);
            return true;
        }
    }
    return false;
}

void collectForStatementCompletions1(Completions* c) {
    static char         string[TMP_STRING_SIZE];
    char               *rec;
    CompletionLine      compLine;
    Symbol             *sym;

    if (isForStatementCompletionSymbol(c, &completionTypeForForStatement, &sym, &rec)) {
        sprintf(string,"%s!=NULL; ", sym->name);
        fillCompletionLine(&compLine, string, NULL, TypeSpecialComplete, 0, NULL);
        completeName(string, &compLine, 0, c);
    }
}

void collectForStatementCompletions2(Completions* c) {
    static char         ss[TMP_STRING_SIZE];
    char               *rec;
    CompletionLine      compLine;
    Symbol             *sym;

    if (isForStatementCompletionSymbol(c, &completionTypeForForStatement, &sym, &rec)) {
        if (rec != NULL) {
            sprintf(ss,"%s=%s->%s) {", sym->name, sym->name, rec);
            fillCompletionLine(&compLine, ss, NULL, TypeSpecialComplete, 0, NULL);
            completeName(ss, &compLine, 0, c);
        }
    }
}

void completeUpFunProfile(Completions *c) {
    if (upLevelFunctionCompletionType != NULL && c->idToProcess[0] == 0 && c->alternativeIndex == 0) {
        Symbol *dd = newSymbolAsType("    ", "    ", noPosition, upLevelFunctionCompletionType);

        fillCompletionLine(&c->alternatives[0], "    ", dd, TypeDefault, 0, NULL);
        c->fullMatchFlag = true;
        c->prefix[0]  = 0;
        c->alternativeIndex++;
    }
}

/* ************************** Yacc stuff ************************ */

static void completeFromXrefFun(ReferenceItem *s, void *c) {
    SymbolCompletionInfo *cc;
    CompletionLine compLine;
    cc = (SymbolCompletionInfo *) c;
    assert(s && cc);
    if (s->type != cc->type)
        return;
    /*&fprintf(dumpOut,"testing %s\n",s->linkName);fflush(dumpOut);&*/
    fillCompletionLine(&compLine, s->linkName, NULL, s->type, 0, NULL);
    processName(s->linkName, &compLine, 1, cc->completions);
}

void collectYaccLexemCompletions(Completions *c) {
    SymbolCompletionInfo info;
    fillCompletionSymInfo(&info, c, TypeYaccSymbol);
    mapOverReferenceTableWithPointer(completeFromXrefFun, (void*) &info);
}
