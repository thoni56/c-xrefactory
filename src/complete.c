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

ExpressionTokenType s_forCompletionType;


void initCompletions(Completions *completions, int length, Position position) {
    completions->idToProcessLen = length;
    completions->idToProcessPos = position;
    completions->fullMatchFlag = false;
    completions->isCompleteFlag = false;
    completions->noFocusOnCompletions = false;
    completions->abortFurtherCompletions = false;
    completions->maxLen = 0;
    completions->alternativeIndex = 0;
}

void fillCompletionLine(CompletionLine *cline, char *string, Symbol *symbol, Type symbolType,
                        short int virtualLevel, short int margn, char **margs, Symbol *vFunClass) {
    cline->string = string;
    cline->symbol = symbol;
    cline->symbolType = symbolType;
    cline->virtLevel = virtualLevel;
    cline->margn = margn;
    cline->margs = margs;
    cline->vFunClass = vFunClass;                  \
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
    int size, l, vFunCl, cindent, tempLength;
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
    vFunCl = NO_FILE_NUMBER;
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
    } else if (completions->alternatives[index].symbolType == TypeInheritedFullMethod) {
        if (completions->alternatives[index].vFunClass!=NULL) {
            sprintf(tempString,"%s \t:%s", completions->alternatives[index].vFunClass->name, typeNamesTable[completions->alternatives[index].symbolType]);
            vFunCl = completions->alternatives[index].vFunClass->u.structSpec->classFileNumber;
            if (vFunCl == -1) vFunCl = NO_FILE_NUMBER;
        } else {
            sprintf(tempString,"%s", typeNamesTable[completions->alternatives[index].symbolType]);
        }
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
    int                 max;
    int                 tlen;

    LIST_LEN(max, Completion, olc);
    if (options.xref2) {
        tlen = 0;
        for (Completion *cc=olc; cc!=NULL; cc=cc->next) {
            tlen += strlen(cc->fullName);
            if (cc->next!=NULL) tlen++;
        }
        if (completionsWillPrintEllipsis(olc))
            tlen += 4;
        ppcBeginAllCompletions(noFocus, tlen);
    } else {
        fprintf(communicationChannel,";");
    }
}

static void printOneCompletion(Completion *olc) {
    fprintf(communicationChannel, "%s", olc->fullName);
}

static void printCompletionsEnding(Completion *olc) {
    if (completionsWillPrintEllipsis(olc)) {
        fprintf(communicationChannel,"\n...");
    }
    if (options.xref2) {
        ppcEnd(PPC_ALL_COMPLETIONS);
    }
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
    int indent, jindent, max, vFunCl;
    char *vclass;

    jindent = 0; vclass = NULL;
    // O.K. there will be a menu diplayed, clear the old one
    olCompletionListInit(&c->idToProcessPos);
    if (c->alternativeIndex == 0) {
        if (options.xref2) {
            if (options.serverOperation == OLO_SEARCH)
                ppcGenRecordWithNumeric(PPC_BOTTOM_INFORMATION, PPCA_BEEP, 0, "** No matches **");
            else
                ppcGenRecordWithNumeric(PPC_BOTTOM_INFORMATION, PPCA_BEEP, 0, "** No completion possible **");
        } else {
            fprintf(communicationChannel, "-");
        }
        goto finishWithoutMenu;
    }
    if (!c->fullMatchFlag && c->alternativeIndex==1) {
        if (options.xref2) {
            ppcGotoPosition(&sessionData.completionsStack.top->callerPosition);
            ppcGenRecord(PPC_SINGLE_COMPLETION, c->alternatives[0].string);
        } else {
            fprintf(communicationChannel, ".%s", c->prefix + c->idToProcessLen);
        }
        goto finishWithoutMenu;
    }
    if (!c->fullMatchFlag && strlen(c->prefix) > c->idToProcessLen) {
        if (options.xref2) {
            ppcGotoPosition(&sessionData.completionsStack.top->callerPosition);
            ppcGenRecord(PPC_SINGLE_COMPLETION, c->prefix);
            ppcGenRecordWithNumeric(PPC_BOTTOM_INFORMATION, PPCA_BEEP, 1, "Multiple completions");
        } else {
            fprintf(communicationChannel, ",%s", c->prefix + c->idToProcessLen);
        }
        goto finishWithoutMenu;
    }

    indent = c->maxLen;
    if (options.olineLen - indent < MIN_COMPLETION_INDENT_REST) {
        indent = options.olineLen - MIN_COMPLETION_INDENT_REST;
        if (indent < MIN_COMPLETION_INDENT) indent = MIN_COMPLETION_INDENT;
    }
    if (indent > MAX_COMPLETION_INDENT) indent = MAX_COMPLETION_INDENT;
    if (c->alternativeIndex > options.maxCompletions) max = options.maxCompletions;
    else max = c->alternativeIndex;
    for(int ii=0; ii<max; ii++) {
        sprintFullCompletionInfo(c, ii, indent);
        vFunCl = NO_FILE_NUMBER;
        Reference ref;
        sessionData.completionsStack.top->completions = completionListPrepend(
            sessionData.completionsStack.top->completions, c->alternatives[ii].string, ppcTmpBuff,
            vclass, jindent, c->alternatives[ii].symbol, NULL, &ref, c->alternatives[ii].symbolType,
            vFunCl);
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
    if (l1 == collectedCompletions.idToProcessLen && l2 != collectedCompletions.idToProcessLen)
        return -1;
    if (l1 != collectedCompletions.idToProcessLen && l2 == collectedCompletions.idToProcessLen)
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
    len = ci->idToProcessLen;
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
        fillCompletionLine(&compLine, symbol->name, symbol, symbol->type, 0, 0, NULL, NULL);
    } else {
        if (symbol->u.mbody == NULL) {
            fillCompletionLine(&compLine, symbol->name, symbol, TypeUndefMacro, 0, 0, NULL, NULL);
        } else {
            fillCompletionLine(&compLine, symbol->name, symbol, symbol->type, 0, symbol->u.mbody->argCount,
                               symbol->u.mbody->argumentNames, NULL);
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
    fillCompletionLine(&compLine, cn, r, TypeDefault, vlevel,0,NULL,vFunCl);
    processName(cn, &compLine, orderFlag, c);
}

// NOTE: Mapping function
static void symbolCompletionFunction(Symbol *symbol, void *c) {
    SymbolCompletionFunctionInfo *cc;
    CompletionLine completionLine;
    char    *completionName;
    cc = (SymbolCompletionFunctionInfo *) c;
    assert(symbol);
    if (symbol->type != TypeDefault) return;
    assert(symbol);
    if (cc->storage==StorageTypedef && symbol->storage!=StorageTypedef)
        return;
    completionName = symbol->name;
    if (completionName!=NULL) {
        if (symbol->type == TypeDefault && symbol->u.typeModifier!=NULL && symbol->u.typeModifier->type == TypeFunction) {
            completeFunctionOrMethodName(cc->res, true, 0, symbol, NULL);
        } else {
            fillCompletionLine(&completionLine, completionName, symbol, symbol->type,0, 0, NULL,NULL);
            processName(completionName, &completionLine, 1, cc->res);
        }
    }
}

void completeStructs(Completions *c) {
    SymbolCompletionInfo ii;

    fillCompletionSymInfo(&ii, c, TypeStruct);
    symbolTableMapWithPointer(symbolTable, completeFun, (void*) &ii);
    fillCompletionSymInfo(&ii, c, TypeUnion);
    symbolTableMapWithPointer(symbolTable, completeFun, (void*) &ii);
}

static void processSpecialInheritedFullCompletion(Completions *c, int orderFlag, int vlevel, Symbol *r, Symbol *vFunCl, char *cname) {
    int     size, ll;
    char    *fcc;
    char    tmp[MAX_CX_SYMBOL_SIZE];
    CompletionLine compLine;

    tmp[0]=0; ll=0; size=MAX_CX_SYMBOL_SIZE;
    typeSPrint(tmp+ll, &size, r->u.typeModifier, cname, ' ', true, SHORT_NAME, NULL);
    fcc = stackMemoryAlloc(strlen(tmp)+1);
    strcpy(fcc,tmp);
    fillCompletionLine(&compLine, fcc, r, TypeInheritedFullMethod, vlevel,0,NULL,vFunCl);
    processName(fcc, &compLine, orderFlag, c);
}

static void completeRecordsNames(
    Completions *completions,
    Symbol *symbol,
    int completionType,
    int vlevelOffset
) {
    CompletionLine completionLine;
    int vlevel;
    Symbol *r, *vFunCl;
    S_recFindStr rfs;
    char *cname;

    if (symbol==NULL)
        return;

    bool orderFlag = completions->idToProcess[0] != 0;

    assert(symbol->u.structSpec);
    iniFind(symbol, &rfs);

    for(;;) {
        // this is in fact about not cutting all records of the class,
        Result result = findStrRecordSym(&r, &rfs, NULL);
        if (result != RESULT_OK)
            break;

        /* because constructors are not inherited */
        assert(r);

        cname = r->name;
        if (cname!=NULL && *cname != 0 && r->type != TypeError
            // Hmm. I hope it will not filter out something important
            && (! symbolShouldBeHiddenFromSearchResults(r->linkName))
        ) {
            assert(rfs.currentClass && rfs.currentClass->u.structSpec);
            assert(r->type == TypeDefault);
            vFunCl = rfs.currentClass;
            if (vFunCl->u.structSpec->classFileNumber == -1) {
                vFunCl = NULL;
            }
            vlevel = rfs.superClassesCount + vlevelOffset;
            if (completionType == TypeInheritedFullMethod) {
                // TODO customizable completion level
                if (vlevel > 1
                    &&  (r->access & AccessPrivate)==0
                    &&  (r->access & AccessStatic)==0) {
                    processSpecialInheritedFullCompletion(completions,orderFlag,vlevel,
                                                          r, vFunCl, cname);
                }
                completions->prefix[0] = 0;
            } else if (completionType == TypeSpecialConstructorCompletion) {
                fillCompletionLine(&completionLine, completions->idToProcess, r, TypeDefault, vlevel,0,NULL,vFunCl);
                completionInsertName(completions->idToProcess, &completionLine, orderFlag, (void*) completions);
            } else {
                fillCompletionLine(&completionLine, cname, r, TypeDefault, vlevel, 0, NULL, vFunCl);
                processName(cname, &completionLine, orderFlag, completions);
            }
        }
    }
    if (completions->idToProcess[0] == 0)
        completions->prefix[0] = 0; // no common prefix completed
}


void completeRecNames(Completions *c) {
    TypeModifier *str;
    Symbol *s;
    assert(s_structRecordCompletionType);
    str = s_structRecordCompletionType;
    if (str->type == TypeStruct || str->type == TypeUnion) {
        s = str->u.t;
        assert(s);
        completeRecordsNames(c, s, TypeDefault, 0);
    }
    s_structRecordCompletionType = &errorModifier;
}

static void completeFromSymTab(Completions*c, unsigned storage){
    SymbolCompletionFunctionInfo  info;

    fillCompletionSymFunInfo(&info, c, storage);
    symbolTableMapWithPointer(symbolTable, symbolCompletionFunction, (void*) &info);
}

void completeEnums(Completions *c) {
    SymbolCompletionInfo ii;
    fillCompletionSymInfo(&ii, c, TypeEnum);
    symbolTableMapWithPointer(symbolTable, completeFun, (void*) &ii);
}

void completeLabels(Completions *c) {
    SymbolCompletionInfo ii;
    fillCompletionSymInfo(&ii, c, TypeLabel);
    symbolTableMapWithPointer(symbolTable, completeFun, (void*) &ii);
}

void completeMacros(Completions *c) {
    SymbolCompletionInfo ii;
    fillCompletionSymInfo(&ii, c, TypeMacro);
    symbolTableMapWithPointer(symbolTable, completeFun, (void*) &ii);
}

void completeTypes(Completions *c) {
    completeFromSymTab(c, StorageTypedef);
}

void completeOthers(Completions *c) {
    completeFromSymTab(c, StorageDefault);
    completeMacros(c);      /* handle macros as functions */
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
    S_recFindStr    rfs;
    Symbol        *r,*s;
    char *res;
    char            *cname;
    static char     *cnext="next";
    static char     *cprevious="previous";

    s = token->typeModifier->next->u.t;
    res = NULL;
    assert(s->u.structSpec);
    iniFind(s, &rfs);
    for(;;) {
        Result rr = findStrRecordSym(&r, &rfs, NULL);
        if (rr != RESULT_OK) break;
        assert(r);
        cname = r->name;
        if (cname!=NULL) {
            assert(rfs.currentClass && rfs.currentClass->u.structSpec);
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

static bool isForCompletionSymbol(
    Completions *completions,
    ExpressionTokenType *token,
    Symbol **symbolP,
    char **nextRecord
) {
    if (options.serverOperation != OLO_COMPLETION)
        return false;
    if (token->typeModifier==NULL)
        return false;
    if (completions->idToProcessLen != 0)
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

void completeForSpecial1(Completions* c) {
    static char         ss[TMP_STRING_SIZE];
    char                *rec;
    CompletionLine             compLine;
    Symbol            *sym;
    if (isForCompletionSymbol(c,&s_forCompletionType,&sym,&rec)) {
        sprintf(ss,"%s!=NULL; ", sym->name);
        fillCompletionLine(&compLine,ss,NULL,TypeSpecialComplet,0,0,NULL,NULL);
        completeName(ss, &compLine, 0, c);
    }
}

void completeForSpecial2(Completions* c) {
    static char         ss[TMP_STRING_SIZE];
    char                *rec;
    CompletionLine             compLine;
    Symbol            *sym;
    if (isForCompletionSymbol(c, &s_forCompletionType,&sym,&rec)) {
        if (rec!=NULL) {
            sprintf(ss,"%s=%s->%s) {", sym->name, sym->name, rec);
            fillCompletionLine(&compLine,ss,NULL,TypeSpecialComplet,0,0,NULL,NULL);
            completeName(ss, &compLine, 0, c);
        }
    }
}

void completeUpFunProfile(Completions *c) {
    if (s_upLevelFunctionCompletionType != NULL && c->idToProcess[0] == 0 && c->alternativeIndex == 0) {
        Symbol *dd = newSymbolAsType("    ", "    ", noPosition, s_upLevelFunctionCompletionType);

        fillCompletionLine(&c->alternatives[0], "    ", dd, TypeDefault, 0, 0, NULL, NULL);
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
    fillCompletionLine(&compLine, s->linkName, NULL, s->type,0, 0, NULL,NULL);
    processName(s->linkName, &compLine, 1, cc->completions);
}

void completeYaccLexem(Completions *c) {
    SymbolCompletionInfo info;
    fillCompletionSymInfo(&info, c, TypeYaccSymbol);
    mapOverReferenceTableWithPointer(completeFromXrefFun, (void*) &info);
}
