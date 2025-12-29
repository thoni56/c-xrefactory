#include "complete.h"

#include <string.h>
#include <ctype.h>

#include "commons.h"
#include "completion.h"
#include "filetable.h"
#include "globals.h"
#include "list.h"
#include "log.h"
#include "match.h"
#include "misc.h"
#include "options.h"
#include "parsing.h"
#include "ppc.h"
#include "protocol.h"
#include "referenceableitemtable.h"
#include "semact.h"
#include "session.h"
#include "session.h"
#include "stackmemory.h"
#include "type.h"
#include "yylex.h"


#define FULL_COMPLETION_INDENT_CHARS 2
#define MIN_COMPLETION_INDENT_REST 40     /* minimal columns for symbol informations */
#define MIN_COMPLETION_INDENT 20          /* minimal colums for symbol */
#define MAX_COMPLETION_INDENT 70          /* maximal completion indent with scroll bar */


typedef struct {
    struct completions *completions;
    Type                type;
} SymbolCompletionInfo;

typedef struct {
    struct completions *completions;
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
    completions->alternativeCount = 0;
}

CompletionLine makeCompletionLine(char *string, Symbol *symbol, Type symbolType, short int margn,
                                  char **margs) {
    CompletionLine line;
    line.string = string;
    line.symbol = symbol;
    line.type = symbolType;
    line.margn = margn;
    line.margs = margs;
    return line;
}

static SymbolCompletionInfo makeCompletionSymInfo(Completions *completions, Type type) {
    SymbolCompletionInfo info;
    info.completions = completions;
    info.type        = type;
    return info;
}

static SymbolCompletionFunctionInfo makeCompletionSymFunInfo(Completions *completions, Storage storage) {
    SymbolCompletionFunctionInfo info;
    info.completions = completions;
    info.storage = storage;
    return info;
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
    if (completions->alternatives[index].type == TypeCppUndefinedMacro)
        return;

    // remove parenthesis (if any)
    strcpy(tempString, completions->alternatives[index].string);
    tempLength = strlen(tempString);
    if (tempLength>0 && tempString[tempLength-1]==')' && completions->alternatives[index].type!=TypeInheritedFullMethod) {
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
    if (completions->alternatives[index].type==TypeDefault) {
        assert(completions->alternatives[index].symbol && completions->alternatives[index].symbol->typeModifier);
        typeDefinitionExpressionFlag = true;
        if (completions->alternatives[index].symbol->storage == StorageTypedef) {
            sprintf(tempString, "typedef ");
            l = strlen(tempString);
            size -= l;
            typeDefinitionExpressionFlag = false;
        }
        char *pname = completions->alternatives[index].symbol->name;
        prettyPrintType(tempString+l, &size, completions->alternatives[index].symbol->typeModifier, pname, ' ',
                   typeDefinitionExpressionFlag);
    } else if (completions->alternatives[index].type==TypeMacro) {
        prettyPrintMacroDefinition(tempString, &size, completions->alternatives[index].string,
                                   completions->alternatives[index].margn, completions->alternatives[index].margs);
    } else {
        assert(completions->alternatives[index].type>=0 && completions->alternatives[index].type<MAX_TYPE);
        sprintf(tempString,"%s", typeNamesTable[completions->alternatives[index].type]);
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

static void completionListInit(Position originalPos) {
    freeOldCompletionStackEntries(&sessionData.completionStack);
    pushEmptySession(&sessionData.completionStack);
    sessionData.completionStack.top->callerPosition = originalPos;
}


static int completionsWillPrintEllipsis(Match *match) {
    int max, ellipsis;
    LIST_LEN(max, Match, match);
    ellipsis = 0;
    if (max >= MAX_COMPLETIONS - 2 || max == options.maxCompletions) {
        ellipsis = 1;
    }
    return ellipsis;
}


static void printCompletionsBeginning(Match *match, int noFocus) {
    int tlen = 0;
    for (Match *cc=match; cc!=NULL; cc=cc->next) {
        tlen += strlen(cc->fullName);
        if (cc->next!=NULL) tlen++;
    }
    if (completionsWillPrintEllipsis(match))
        tlen += 4;
    ppcBeginAllCompletions(noFocus, tlen);
}

static void printOneCompletion(Match *match) {
    fprintf(outputFile, "%s", match->fullName);
}

static void printCompletionsEnding(Match *match) {
    if (completionsWillPrintEllipsis(match)) {
        fprintf(outputFile,"\n...");
    }
    ppcEnd(PPC_ALL_COMPLETIONS);
}

void printCompletionsList(bool noFocus) {
    Match *completions = sessionData.completionStack.top->matches;

    printCompletionsBeginning(completions, noFocus);
    for(Match *c=completions; c!=NULL; c=c->next) {
        printOneCompletion(c);
        if (c->next!=NULL)
            fprintf(outputFile,"\n");
    }
    printCompletionsEnding(completions);
}

static void reverseCompletionList(void) {
    LIST_REVERSE(Match, sessionData.completionStack.top->matches);
}

void printCompletions(Completions *completions) {
    int indent, max;

    // O.K. there will be a menu diplayed, clear the old one
    completionListInit(completions->idToProcessPosition);
    if (completions->alternativeCount == 0) {
        ppcGenRecordWithNumeric(PPC_BOTTOM_INFORMATION, PPCA_BEEP, 0, "** No completion possible **");
        goto finishWithoutMenu;
    }
    if (!completions->fullMatchFlag && completions->alternativeCount==1) {
        ppcGotoPosition(sessionData.completionStack.top->callerPosition);
        ppcGenRecord(PPC_SINGLE_COMPLETION, completions->alternatives[0].string);
        goto finishWithoutMenu;
    }
    if (!completions->fullMatchFlag && strlen(completions->prefix) > completions->idToProcessLength) {
        ppcGotoPosition(sessionData.completionStack.top->callerPosition);
        ppcGenRecord(PPC_SINGLE_COMPLETION, completions->prefix);
        ppcGenRecordWithNumeric(PPC_BOTTOM_INFORMATION, PPCA_BEEP, 1, "Multiple completions");
        goto finishWithoutMenu;
    }

    indent = completions->maxLen;
    if (options.olineLen - indent < MIN_COMPLETION_INDENT_REST) {
        indent = options.olineLen - MIN_COMPLETION_INDENT_REST;
        if (indent < MIN_COMPLETION_INDENT) indent = MIN_COMPLETION_INDENT;
    }
    if (indent > MAX_COMPLETION_INDENT)
        indent = MAX_COMPLETION_INDENT;
    if (completions->alternativeCount > options.maxCompletions)
        max = options.maxCompletions;
    else
        max = completions->alternativeCount;
    for(int i=0; i<max; i++) {
        sprintFullCompletionInfo(completions, i, indent);
        Reference r;
        sessionData.completionStack.top->matches = prependToMatches(
            sessionData.completionStack.top->matches, completions->alternatives[i].string, ppcTmpBuff,
            completions->alternatives[i].symbol, NULL, &r, NO_FILE_NUMBER);
    }
    reverseCompletionList();
    printCompletionsList(completions->noFocusOnCompletions);
    fflush(outputFile);
    return;
 finishWithoutMenu:
    sessionData.completionStack.top = sessionData.completionStack.top->previous;
    fflush(outputFile);
}

/* *********************************************************************** */

static bool isTheSameSymbol(CompletionLine *c1, CompletionLine *c2) {
    if (strcmp(c1->string, c2->string) != 0)
        return false;
    if (c1->type != c2->type)
        return false;
    return true;
}

static int completionOrderComparer(CompletionLine *c1, CompletionLine *c2) {
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
            c = completionOrderComparer(t, &a[x]);
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

static void computeCompletionPrefix(char *d, char *s) {
    while (*d == *s) {
        if (*d == 0)
            return;
        d++; s++;
    }
    *d = 0;
}

static bool completionTestPrefix(Completions *completions, char *s) {
    char *d;
    d = completions->idToProcess;
    while (*d == *s || (!options.completionCaseSensitive && tolower(*d)==tolower(*s))) {
        if (*d == 0) {
            completions->isCompleteFlag = true;    /* complete, but maybe not unique*/
            return 0;
        }
        d++; s++;
    }
    if (*d == 0)
        return false;
    return true;
}

static void completionInsertName(char *name, CompletionLine *completionLine, bool orderFlag,
                                 Completions *ci) {
    int len,l;

    name = completionLine->string;
    len = ci->idToProcessLength;
    if (ci->alternativeCount == 0) {
        strcpy(ci->prefix, name);
        ci->alternatives[ci->alternativeCount] = *completionLine;
        ci->alternatives[ci->alternativeCount].string = name/*+len*/;
        ci->alternativeCount++;
        ci->maxLen = strlen(name/*+len*/);
        ci->fullMatchFlag = ((len==ci->maxLen)
                             || (len==ci->maxLen-1 && name[len]=='(')
                             || (len==ci->maxLen-2 && name[len]=='('));
    } else {
        assert(ci->alternativeCount < MAX_COMPLETIONS-1);
        if (reallyInsert(ci->alternatives, &ci->alternativeCount, name/*+len*/, completionLine, orderFlag)) {
            ci->fullMatchFlag = false;
            l = strlen(name/*+len*/);
            if (l > ci->maxLen) ci->maxLen = l;
            computeCompletionPrefix(ci->prefix, name);
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

void processName(char *name, CompletionLine *line, bool orderFlag, Completions *completions) {
    completeName(name, line, orderFlag, completions);
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
        compLine = makeCompletionLine(symbol->name, symbol, symbol->type, 0, NULL);
    } else {
        if (symbol->mbody == NULL) {
            compLine = makeCompletionLine(symbol->name, symbol, TypeCppUndefinedMacro, 0, NULL);
        } else {
            compLine = makeCompletionLine(symbol->name, symbol, symbol->type, symbol->mbody->argCount,
                               symbol->mbody->argumentNames);
        }
    }
    processName(symbol->name, &compLine, true, completionInfo->completions);
}

static void completeFunctionOrMethodName(Completions *c, bool orderFlag, Symbol *r, Symbol *vFunCl) {
    CompletionLine compLine;
    int cnamelen;
    char *cn, *cname, *psuff;

    cname = r->name;
    cnamelen = strlen(cname);
    if (!options.completeParenthesis) {
        cn = cname;
    } else {
        assert(r->typeModifier!=NULL);
        if (r->typeModifier!=NULL && r->typeModifier->args == NULL) {
            psuff = "()";
        } else {
            psuff = "(";
        }
        cn = stackMemoryAlloc(cnamelen+strlen(psuff)+1);
        strcpy(cn, cname);
        strcpy(cn+cnamelen, psuff);
    }
    compLine = makeCompletionLine(cn, r, TypeDefault,0,NULL);
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
        if (symbol->type == TypeDefault && symbol->typeModifier!=NULL && symbol->typeModifier->type == TypeFunction) {
            completeFunctionOrMethodName(info->completions, true, symbol, NULL);
        } else {
            completionLine = makeCompletionLine(completionName, symbol, symbol->type, 0, NULL);
            processName(completionName, &completionLine, 1, info->completions);
        }
    }
}

void collectStructsCompletions(Completions *completions) {
    SymbolCompletionInfo info;

    info = makeCompletionSymInfo(completions, TypeStruct);
    symbolTableMapWithPointer(symbolTable, completeFun, (void*) &info);
    info = makeCompletionSymInfo(completions, TypeUnion);
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

    assert(symbol->structSpec);
    initFind(symbol, &info);

    for(;;) {
        // this is in fact about not cutting all members of the struct,
        Result result = findStructureMemberSymbol(&foundSymbol, &info, NULL);
        if (result != RESULT_OK)
            break;

        /* because constructors are not inherited */
        assert(foundSymbol);

        name = foundSymbol->name;
        if (name!=NULL && *name != 0 && foundSymbol->type != TypeError) {
            assert(info.currentStructure && info.currentStructure->structSpec);
            assert(foundSymbol->type == TypeDefault);
            completionLine = makeCompletionLine(name, foundSymbol, TypeDefault, 0, NULL);
            processName(name, &completionLine, orderFlag, completions);
        }
    }
    if (completions->idToProcess[0] == 0)
        completions->prefix[0] = 0; // no common prefix completed
}


void collectStructMemberCompletions(Completions *completions) {
    TypeModifier *structure;
    assert(structMemberCompletionType);
    structure = structMemberCompletionType;
    if (structure->type == TypeStruct || structure->type == TypeUnion) {
        Symbol *symbol = structure->typeSymbol;
        assert(symbol);
        completeMemberNames(completions, symbol);
    }
    structMemberCompletionType = &errorModifier;
}

static void completeFromSymTab(Completions *completions, unsigned storage){
    SymbolCompletionFunctionInfo  info;

    info = makeCompletionSymFunInfo(completions, storage);
    symbolTableMapWithPointer(symbolTable, symbolCompletionFunction, (void*) &info);
}

void collectEnumsCompletions(Completions *completions) {
    SymbolCompletionInfo info;

    info = makeCompletionSymInfo(completions, TypeEnum);
    symbolTableMapWithPointer(symbolTable, completeFun, (void*) &info);
}

void collectLabelsCompletions(Completions *completions) {
    SymbolCompletionInfo info;

    info = makeCompletionSymInfo(completions, TypeLabel);
    symbolTableMapWithPointer(symbolTable, completeFun, (void*) &info);
}

void completeMacros(Completions *completions) {
    SymbolCompletionInfo info;

    info = makeCompletionSymInfo(completions, TypeMacro);
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
    // TODO: Feels like an API function... getReferenceableItemFor(Reference *r)
    for (int i=getNextExistingReferenceableItem(0); i != -1; i = getNextExistingReferenceableItem(i+1)) {
        ReferenceableItem *referencesItem = getReferenceableItem(i);
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
        if (s1->typeSymbol != s2->typeSymbol)
            return false;
    } else if (s1->type==TypeFunction) {
        Symbol *ss1, *ss2;
        for (ss1=s1->args, ss2=s2->args;
             ss1!=NULL && ss2!=NULL;
             ss1=ss1->next, ss2=ss2->next) {
            if (!isEqualType(ss1->typeModifier, ss2->typeModifier))
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

    s = token->typeModifier->next->typeSymbol;
    res = NULL;
    assert(s->structSpec);
    initFind(s, &rfs);
    for(;;) {
        Result rr = findStructureMemberSymbol(&r, &rfs, NULL);
        if (rr != RESULT_OK) break;
        assert(r);
        cname = r->name;
        if (cname!=NULL) {
            assert(rfs.currentStructure && rfs.currentStructure->structSpec);
            assert(r->type == TypeDefault);
            if (isEqualType(r->typeModifier, token->typeModifier)) {
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
    if (parsingConfig.operation != PARSER_OP_COMPLETION)
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
        compLine = makeCompletionLine(string, NULL, TypeSpecialComplete, 0, NULL);
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
            compLine = makeCompletionLine(ss, NULL, TypeSpecialComplete, 0, NULL);
            completeName(ss, &compLine, 0, c);
        }
    }
}

void completeUpFunProfile(Completions *c) {
    if (upLevelFunctionCompletionType != NULL && c->idToProcess[0] == 0 && c->alternativeCount == 0) {
        Symbol *dd = newSymbolAsType("    ", "    ", noPosition, upLevelFunctionCompletionType);

        c->alternatives[0] = makeCompletionLine("    ", dd, TypeDefault, 0, NULL);
        c->fullMatchFlag = true;
        c->prefix[0]  = 0;
        c->alternativeCount++;
    }
}

/* ************************** Yacc stuff ************************ */

static void completeFromXrefFun(ReferenceableItem *s, void *c) {
    SymbolCompletionInfo *cc;
    CompletionLine compLine;
    cc = (SymbolCompletionInfo *) c;
    assert(s && cc);
    if (s->type != cc->type)
        return;
    compLine = makeCompletionLine(s->linkName, NULL, s->type, 0, NULL);
    processName(s->linkName, &compLine, 1, cc->completions);
}

void collectYaccLexemCompletions(Completions *c) {
    SymbolCompletionInfo info;
    info = makeCompletionSymInfo(c, TypeYaccSymbol);
    mapOverReferenceableItemTableWithPointer(completeFromXrefFun, (void*) &info);
}
