#include "complete.h"

#include "classfilereader.h"
#include "completion.h"
#include "cxfile.h"
#include "cxref.h"
#include "fileio.h"
#include "filetable.h"
#include "globals.h"
#include "jsemact.h"
#include "list.h"
#include "log.h"
#include "misc.h"
#include "options.h"
#include "parsers.h"
#include "protocol.h"
#include "reftab.h"
#include "semact.h"
#include "type.h"
#include "yylex.h"


#define FULL_COMPLETION_INDENT_CHARS 2

enum fqtCompletion {
    FQT_COMPLETE_DEFAULT,
    FQT_COMPLETE_ALSO_ON_PACKAGE
};


typedef struct {
    struct completions *completions;
    Type                type;
} SymbolCompletionInfo;

typedef struct {
    struct completions *res;
    Storage storage;
} SymbolCompletionFunctionInfo;

typedef struct {
    struct completions *res;
    int completionType;
} FqtMapCompletionInfo;


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

static void fillCompletionFqtMapInfo(FqtMapCompletionInfo *completionFqtMapInfo, Completions *completions,
                                     enum fqtCompletion completionType) {
    completionFqtMapInfo->res = completions;
    completionFqtMapInfo->completionType = completionType;
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

/* STATIC except for unittests */
int printJavaModifiers(char *buf, int *size, Access access) {
    int i = 0;

    if (access & AccessPublic) {
        sprintf(buf+i,"public "); i+=strlen(buf+i);
        assert(i< *size);
    }
    if (access & AccessPrivate) {
        sprintf(buf+i,"private "); i+=strlen(buf+i);
        assert(i< *size);
    }
    if (access & AccessProtected) {
        sprintf(buf+i,"protected "); i+=strlen(buf+i);
        assert(i< *size);
    }
    if (access & AccessStatic) {
        sprintf(buf+i,"static "); i+=strlen(buf+i);
        assert(i< *size);
    }
    if (access & AccessFinal) {
        sprintf(buf+i,"final "); i+=strlen(buf+i);
        assert(i< *size);
    }
    *size -= i;
    return i;
}

static char *getCompletionClassFieldString(CompletionLine *cl) {
    char *cname;

    if (cl->vFunClass!=NULL) {
        cname = javaGetShortClassName(cl->vFunClass->linkName);
    } else {
        assert(cl->symbol);
        cname = javaGetNudePreTypeName_static(cl->symbol->linkName, DONT_DISPLAY_NESTED_CLASSES);
    }
    return cname;
}


static void sprintFullCompletionInfo(Completions* completions, int index, int indent) {
    int size, l, vFunCl, cindent, tempLength;
    bool tdexpFlag;
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
        if (LANGUAGE(LANG_JAVA)) {
            char tmpBuff[TMP_BUFF_SIZE];
            char *cname = getCompletionClassFieldString(&completions->alternatives[index]);
            if (completions->alternatives[index].virtLevel>NEST_VIRT_COMPL_OFFSET) {
                sprintf(tmpBuff,"(%d.%d)%s: ",
                        completions->alternatives[index].virtLevel/NEST_VIRT_COMPL_OFFSET,
                        completions->alternatives[index].virtLevel%NEST_VIRT_COMPL_OFFSET,
                        cname);
            } else if (completions->alternatives[index].virtLevel>0) {
                sprintf(tmpBuff,"(%d)%s: ", completions->alternatives[index].virtLevel, cname);
            } else {
                sprintf(tmpBuff,"   : ");
            }
            cindent += strlen(tmpBuff);
            sprintf(ppc,"%s", tmpBuff);
            ppc += strlen(ppc);
        }
        tdexpFlag = 1;
        if (completions->alternatives[index].symbol->storage == StorageTypedef) {
            sprintf(tempString, "typedef ");
            l = strlen(tempString);
            size -= l;
            tdexpFlag = 0;
        }
        char *pname = completions->alternatives[index].symbol->name;
        if (LANGUAGE(LANG_JAVA)) {
            l += printJavaModifiers(tempString+l, &size, completions->alternatives[index].symbol->access);
            if (completions->alternatives[index].vFunClass!=NULL) {
                vFunCl = completions->alternatives[index].vFunClass->u.structSpec->classFileNumber;
                if (vFunCl == -1) vFunCl = NO_FILE_NUMBER;
            }
        }
        typeSPrint(tempString+l, &size, completions->alternatives[index].symbol->u.typeModifier, pname,' ', 0, tdexpFlag,SHORT_NAME, NULL);
        if (LANGUAGE(LANG_JAVA)
            && (completions->alternatives[index].symbol->storage == StorageMethod
                || completions->alternatives[index].symbol->storage == StorageConstructor)) {
            throwsSprintf(tempString+l+size, COMPLETION_STRING_SIZE-l-size, completions->alternatives[index].symbol->u.typeModifier->u.m.exceptions);
        }
    } else if (completions->alternatives[index].symbolType==TypeMacro) {
        macDefSPrintf(tempString, &size, "", completions->alternatives[index].string,
                      completions->alternatives[index].margn, completions->alternatives[index].margs, NULL);
    } else if (LANGUAGE(LANG_JAVA) && completions->alternatives[index].symbolType==TypeStruct ) {
        if (completions->alternatives[index].symbol!=NULL) {
            l += printJavaModifiers(tempString+l, &size, completions->alternatives[index].symbol->access);
            if (completions->alternatives[index].symbol->access & AccessInterface) {
                sprintf(tempString+l,"interface ");
            } else {
                sprintf(tempString+l,"class ");
            }
            l = strlen(tempString);
            javaTypeStringSPrint(tempString+l, completions->alternatives[index].symbol->linkName,LONG_NAME, NULL);
        } else {
            sprintf(tempString,"class ");
        }
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

static void sprintFullJeditCompletionInfo(Completions *c, int ii, int *nindent, char **vclass) {
    int size,ll,tdexpFlag;
    char *pname,*cname;
    static char vlevelBuff[TMP_STRING_SIZE];
    size = COMPLETION_STRING_SIZE;
    ll = 0;
    sprintf(vlevelBuff," ");
    if (vclass != NULL) *vclass = vlevelBuff;
    if (c->alternatives[ii].symbolType==TypeDefault) {
        assert(c->alternatives[ii].symbol && c->alternatives[ii].symbol->u.typeModifier);
        if (LANGUAGE(LANG_JAVA)) {
            cname = getCompletionClassFieldString(&c->alternatives[ii]);
            if (c->alternatives[ii].virtLevel>NEST_VIRT_COMPL_OFFSET) {
                sprintf(vlevelBuff,"  : (%d.%d) %s ",
                        c->alternatives[ii].virtLevel/NEST_VIRT_COMPL_OFFSET,
                        c->alternatives[ii].virtLevel%NEST_VIRT_COMPL_OFFSET,
                        cname);
            } else if (c->alternatives[ii].virtLevel>0) {
                sprintf(vlevelBuff,"  : (%d) %s ", c->alternatives[ii].virtLevel, cname);
            }
        }
        tdexpFlag = 1;
        if (c->alternatives[ii].symbol->storage == StorageTypedef) {
            sprintf(ppcTmpBuff,"typedef ");
            ll = strlen(ppcTmpBuff);
            size -= ll;
            tdexpFlag = 0;
        }
        pname = c->alternatives[ii].symbol->name;
        if (LANGUAGE(LANG_JAVA)) {
            ll += printJavaModifiers(ppcTmpBuff+ll, &size, c->alternatives[ii].symbol->access);
        }
        typeSPrint(ppcTmpBuff+ll, &size, c->alternatives[ii].symbol->u.typeModifier, pname,' ', 0, tdexpFlag,SHORT_NAME, nindent);
        *nindent += ll;
        if (LANGUAGE(LANG_JAVA)
            && (c->alternatives[ii].symbol->storage == StorageMethod
                || c->alternatives[ii].symbol->storage == StorageConstructor)) {
            throwsSprintf(ppcTmpBuff+ll+size, COMPLETION_STRING_SIZE-ll-size, c->alternatives[ii].symbol->u.typeModifier->u.m.exceptions);
        }
    } else if (c->alternatives[ii].symbolType==TypeMacro) {
        macDefSPrintf(ppcTmpBuff, &size, "", c->alternatives[ii].string,
                      c->alternatives[ii].margn, c->alternatives[ii].margs, nindent);
    } else if (LANGUAGE(LANG_JAVA) && c->alternatives[ii].symbolType==TypeStruct ) {
        if (c->alternatives[ii].symbol!=NULL) {
            ll += printJavaModifiers(ppcTmpBuff+ll, &size, c->alternatives[ii].symbol->access);
            if (c->alternatives[ii].symbol->access & AccessInterface) {
                sprintf(ppcTmpBuff+ll,"interface ");
            } else {
                sprintf(ppcTmpBuff+ll,"class ");
            }
            ll = strlen(ppcTmpBuff);
            javaTypeStringSPrint(ppcTmpBuff+ll, c->alternatives[ii].symbol->linkName, LONG_NAME, nindent);
            *nindent += ll;
        } else {
            sprintf(ppcTmpBuff,"class ");
            ll = strlen(ppcTmpBuff);
            *nindent = ll;
            sprintf(ppcTmpBuff+ll,"%s", c->alternatives[ii].string);
        }
    } else if (c->alternatives[ii].symbolType == TypeInheritedFullMethod) {
        sprintf(ppcTmpBuff,"%s", c->alternatives[ii].string);
        if (c->alternatives[ii].vFunClass!=NULL) {
            sprintf(vlevelBuff,"  : %s ", c->alternatives[ii].vFunClass->name);
        }
        *nindent = 0;
    } else {
        sprintf(ppcTmpBuff,"%s", c->alternatives[ii].string);
        *nindent = 0;
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
        if (options.editor == EDITOR_JEDIT) {
            ppcBeginWithTwoNumericAttributes(PPC_FULL_MULTIPLE_COMPLETIONS,
                                           PPCA_NUMBER, max,
                                           PPCA_NO_FOCUS, noFocus);
        } else {
            tlen = 0;
            for (Completion *cc=olc; cc!=NULL; cc=cc->next) {
                tlen += strlen(cc->fullName);
                if (cc->next!=NULL) tlen++;
            }
            if (completionsWillPrintEllipsis(olc))
                tlen += 4;
            ppcBeginAllCompletions(noFocus, tlen);
        }
    } else {
        fprintf(communicationChannel,";");
    }
}

static void printOneCompletion(Completion *olc) {
    if (options.editor == EDITOR_JEDIT) {
        fprintf(communicationChannel,"<%s %s=\"%s\" %s=%d %s=%ld>", PPC_MULTIPLE_COMPLETION_LINE,
                PPCA_VCLASS, olc->vclass,
                PPCA_VALUE, olc->jindent,
                PPCA_LEN, (unsigned long)strlen(olc->fullName));
        fprintf(communicationChannel, "%s", olc->fullName);
        fprintf(communicationChannel, "</%s>\n", PPC_MULTIPLE_COMPLETION_LINE);
    } else {
        fprintf(communicationChannel, "%s", olc->fullName);
    }
}

static void printCompletionsEnding(Completion *olc) {
    if (completionsWillPrintEllipsis(olc)) {
        if (options.editor == EDITOR_JEDIT) {
        } else {
            fprintf(communicationChannel,"\n...");
        }
    }
    if (options.xref2) {
        if (options.editor == EDITOR_JEDIT) {
            ppcEnd(PPC_FULL_MULTIPLE_COMPLETIONS);
        } else {
            ppcEnd(PPC_ALL_COMPLETIONS);
        }
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
    // this can't be ordered directly, because of overloading
    //&     if (LANGUAGE(LANG_JAVA)) reorderCompletionArray(c);
    indent = c->maxLen;
    if (options.olineLen - indent < MIN_COMPLETION_INDENT_REST) {
        indent = options.olineLen - MIN_COMPLETION_INDENT_REST;
        if (indent < MIN_COMPLETION_INDENT) indent = MIN_COMPLETION_INDENT;
    }
    if (indent > MAX_COMPLETION_INDENT) indent = MAX_COMPLETION_INDENT;
    if (c->alternativeIndex > options.maxCompletions) max = options.maxCompletions;
    else max = c->alternativeIndex;
    for(int ii=0; ii<max; ii++) {
        if (options.editor == EDITOR_JEDIT) {
            sprintFullJeditCompletionInfo(c, ii, &jindent, &vclass);
        } else {
            sprintFullCompletionInfo(c, ii, indent);
        }
        vFunCl = NO_FILE_NUMBER;
        if (LANGUAGE(LANG_JAVA)  && c->alternatives[ii].vFunClass!=NULL) {
            vFunCl = c->alternatives[ii].vFunClass->u.structSpec->classFileNumber;
            if (vFunCl == -1) vFunCl = NO_FILE_NUMBER;
        }
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
    /*fprintf(dumpOut,"st %d %d\n",c1->symType,c2->symType);*/
    if (c1->symbolType != c2->symbolType)
        return false;
    if (currentLanguage != LANG_JAVA)
        return true;
    if (c1->symbolType == TypeStruct) {
        if (c1->symbol!=NULL && c2->symbol!=NULL) {
            return strcmp(c1->symbol->linkName, c2->symbol->linkName)==0;
        }
    }
    if (c1->symbolType != TypeDefault)
        return true;
    assert(c1->symbol && c1->symbol->u.typeModifier);
    assert(c2->symbol && c2->symbol->u.typeModifier);
    /*fprintf(dumpOut,"tm %d %d\n",c1->t->u.type->m,c2->t->u.type->m);*/
    if (c1->symbol->u.typeModifier->kind != c2->symbol->u.typeModifier->kind)
        return false;
    if (c1->vFunClass != c2->vFunClass)
        return false;
    if (c2->symbol->u.typeModifier->kind != TypeFunction)
        return true;
    /*fprintf(dumpOut,"sigs %s %s\n",c1->t->u.type->u.sig,c2->t->u.type->u.sig);*/
    assert(c1->symbol->u.typeModifier->u.m.signature && c2->symbol->u.typeModifier->u.m.signature);
    if (strcmp(c1->symbol->u.typeModifier->u.m.signature,c2->symbol->u.typeModifier->u.m.signature))
        return false;
    return true;
}

static bool symbolIsInTab(CompletionLine *a, int ai, int *ii, char *s, CompletionLine *t) {
    int i,j;
    for(i= *ii-1; i>=0 && strcmp(a[i].string,s)==0; i--) ;
    for(j= *ii+1; j<ai && strcmp(a[j].string,s)==0; j++) ;
    /* from a[i] to a[j] symbols have the same names */
    for (i++; i<j; i++)
        if (isTheSameSymbol(&a[i],t))
            return true;
    return false;
}

static int compareCompletionClassName(CompletionLine *c1, CompletionLine *c2) {
    char    n1[TMP_STRING_SIZE];
    int     c;
    char    *n2;
    strcpy(n1, getCompletionClassFieldString(c1));
    n2 = getCompletionClassFieldString(c2);
    c = strcmp(n1, n2);
    if (c<0) return -1;
    if (c>0) return 1;
    return 0;
}

static int completionOrderCmp(CompletionLine *c1, CompletionLine *c2) {
    int     c, l1, l2;
    char    *s1, *s2;
    if (! LANGUAGE(LANG_JAVA)) {
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
    } else {
        if (c1->symbolType==TypeKeyword && c2->symbolType!=TypeKeyword)
            return 1;
        if (c1->symbolType!=TypeKeyword && c2->symbolType==TypeKeyword)
            return -1;
        if (c1->symbolType==TypeNonImportedClass && c2->symbolType!=TypeNonImportedClass)
            return 1;
        if (c1->symbolType!=TypeNonImportedClass && c2->symbolType==TypeNonImportedClass)
            return -1;
        if (c1->symbolType!=TypeInheritedFullMethod && c2->symbolType==TypeInheritedFullMethod)
            return 1;
        if (c1->symbolType==TypeInheritedFullMethod && c2->symbolType!=TypeInheritedFullMethod)
            return -1;
        if (c1->symbolType==TypeInheritedFullMethod && c2->symbolType==TypeInheritedFullMethod) {
            if (c1->symbol == NULL)
                return 1;    // "main"
            if (c2->symbol == NULL)
                return -1;
            c = compareCompletionClassName(c1, c2);
            if (c!=0)
                return c;
        }
        if (c1->symbolType!=TypeDefault && c2->symbolType==TypeDefault)
            return 1;
        if (c1->symbolType==TypeDefault && c2->symbolType!=TypeDefault)
            return -1;
        // exact matches goes first
        l1 = strlen(c1->string);
        l2 = strlen(c2->string);
        if (l1 == collectedCompletions.idToProcessLen && l2 != collectedCompletions.idToProcessLen)
            return -1;
        if (l1 != collectedCompletions.idToProcessLen && l2 == collectedCompletions.idToProcessLen)
            return 1;
        if (c1->symbolType==TypeDefault && c2->symbolType==TypeDefault) {
            c = c1->virtLevel - c2->virtLevel;
            if (c<0)
                return -1;
            if (c>0)
                return 1;
            c = compareCompletionClassName(c1, c2);
            if (c!=0)
                return c;
            // compare storages, fields goes first, then methods
            if (c1->symbol!=NULL && c2->symbol!=NULL) {
                if (c1->symbol->storage==StorageField && c2->symbol->storage==StorageMethod)
                    return -1;
                if (c1->symbol->storage==StorageMethod && c2->symbol->storage==StorageField)
                    return 1;
            }
        }
        if (c1->symbolType==TypeNonImportedClass && c2->symbolType==TypeNonImportedClass) {
            // order by class name, not package
            s1 = lastOccurenceInString(c1->string, '.');
            if (s1 == NULL) s1 = c1->string;
            s2 = lastOccurenceInString(c2->string, '.');
            if (s2 == NULL) s2 = c2->string;
            return strcmp(s1, s2);
        }
        return strcmp(c1->string, c2->string);
    }
}

static bool reallyInsert(CompletionLine *a, int *aip, char *s, CompletionLine *t, int orderFlag) {
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
            //&     c = strcmp(s, a[x].string);
            if (c == 0) { /* identifier still in completions */
                if (currentLanguage != LANG_JAVA)
                    return false; /* no overloading, so ... */
                if (symbolIsInTab(a, ai, &x, s, t))
                    return false;
                r = x;
                l = x + 1;
                break;
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

static void completionInsertName(char *name, CompletionLine *completionLine, int orderFlag,
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

static void completeName(char *name, CompletionLine *compLine, int orderFlag,
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

void processName(char *name, CompletionLine *line, int orderFlag, Completions *c) {
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
    processName(symbol->name, &compLine, 1, completionInfo->completions);
}

/* TODO: Meaning? Something about constructor... If either then clear completionN */
static void CONST_CONSTRUCT_NAME(Storage ccstorage, Storage sstorage, char **completionName) {
    if (ccstorage!=StorageConstructor && sstorage==StorageConstructor) {
        *completionName = NULL;
    }
    if (ccstorage==StorageConstructor && sstorage!=StorageConstructor) {
        *completionName = NULL;
    }
}

static void completeFunctionOrMethodName(Completions *c, int orderFlag, int vlevel, Symbol *r, Symbol *vFunCl) {
    CompletionLine         compLine;
    int             cnamelen;
    char            *cn, *cname, *psuff, *msig;

    cname = r->name;
    cnamelen = strlen(cname);
    if (!options.completeParenthesis) {
        cn = cname;
    } else {
        assert(r->u.typeModifier!=NULL);
        if (LANGUAGE(LANG_JAVA)) {
            msig = r->u.typeModifier->u.m.signature;
            assert(msig!=NULL);
            if (msig[0]=='(' && msig[1]==')') {
                psuff = "()";
            } else {
                psuff = "(";
            }
        } else {
            if (r->u.typeModifier!=NULL && r->u.typeModifier->u.f.args == NULL) {
                psuff = "()";
            } else {
                psuff = "(";
            }
        }
        cn = StackMemoryAllocC(cnamelen+strlen(psuff)+1, char);
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
    CONST_CONSTRUCT_NAME(cc->storage, symbol->storage, &completionName);
    if (completionName!=NULL) {
        if (symbol->type == TypeDefault && symbol->u.typeModifier!=NULL && symbol->u.typeModifier->kind == TypeFunction) {
            completeFunctionOrMethodName(cc->res, 1, 0, symbol, NULL);
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

static bool javaLinkable(Access access) {

    log_trace("testing linkability %x", access);
    if (currentLanguage != LANG_JAVA)
        return true;
    if (access == AccessAll)
        return true;
    if ((options.ooChecksBits & OOC_LINKAGE_CHECK) == 0)
        return true;
    if (access & AccessStatic)
        return (access & AccessStatic) != 0;
    return true;
}

static void processSpecialInheritedFullCompletion( Completions *c, int orderFlag, int vlevel, Symbol *r, Symbol *vFunCl, char *cname) {
    int     size, ll;
    char    *fcc;
    char    tt[MAX_CX_SYMBOL_SIZE];
    CompletionLine compLine;

    tt[0]=0; ll=0; size=MAX_CX_SYMBOL_SIZE;
    if (LANGUAGE(LANG_JAVA)) {
        ll+=printJavaModifiers(tt+ll, &size, r->access);
    }
    typeSPrint(tt+ll, &size, r->u.typeModifier, cname, ' ', 0, 1,SHORT_NAME, NULL);
    fcc = StackMemoryAllocC(strlen(tt)+1, char);
    strcpy(fcc,tt);
    //&fprintf(dumpOut,":adding %s\n",fcc);fflush(dumpOut);
    fillCompletionLine(&compLine, fcc, r, TypeInheritedFullMethod, vlevel,0,NULL,vFunCl);
    processName(fcc, &compLine, orderFlag, c);
}

static AccessibilityCheckYesNo calculateAccessCheckOption(void) {
    if (options.ooChecksBits & OOC_ACCESS_CHECK) {
        return ACCESSIBILITY_CHECK_YES;
    } else {
        return ACCESSIBILITY_CHECK_NO;
    }
}

static void completeRecordsNames(
    Completions *c,
    Symbol *symbol,
    int classification,
    int constructorOpt,
    int completionType,
    int vlevelOffset
) {
    CompletionLine completionLine;
    int orderFlag, vlevel, accessCheck, visibilityCheck;
    Symbol *r, *vFunCl;
    S_recFindStr rfs;
    char *cname;

    if (symbol==NULL) return;
    if (c->idToProcess[0] == 0 && currentLanguage!=LANG_JAVA) {
        orderFlag = 0;
    } else {
        orderFlag = 1;
    }
    assert(symbol->u.structSpec);
    iniFind(symbol, &rfs);
    //&fprintf(dumpOut,"checking records of %s\n", symbol->linkName);
    for(;;) {
        // this is in fact about not cutting all records of the class,
        // not about visibility checks
        visibilityCheck = VISIBILITY_CHECK_NO;
        accessCheck = calculateAccessCheckOption();
        //&if (options.ooChecksBits & OOC_VISIBILITY_CHECK) {
        //& visibilityCheck = VISIBILITY_CHECK_YES;
        //&} else {
        //& visibilityCheck = VISIBILITY_CHECK_NO;
        //&}
        Result rr = findStrRecordSym(&rfs, NULL, &r, classification, accessCheck, visibilityCheck);
        if (rr != RESULT_OK) break;
        if (constructorOpt == StorageConstructor && rfs.currentClass != symbol)
            break;
        /* because constructors are not inherited */
        assert(r);
        cname = r->name;
        CONST_CONSTRUCT_NAME(constructorOpt, r->storage, &cname);
        //&fprintf(dumpOut,"record %s\n", cname);
        if (    cname!=NULL
                && *cname != 0
                && r->type != TypeError
                // Hmm. I hope it will not filter out something important
                && (! symbolShouldBeHiddenFromSearchResults(r->linkName))
                //  I do not know whether to check linkability or not
                //  What is more natural ???
                && javaLinkable(r->access)) {
            //&fprintf(dumpOut,"passed\n", cname);
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
                    && vlevel <= options.completionOverloadWizardDepth+1
                    &&  (r->access & AccessPrivate)==0
                    &&  (r->access & AccessStatic)==0) {
                    processSpecialInheritedFullCompletion(c,orderFlag,vlevel,
                                                          r, vFunCl, cname);
                }
                c->prefix[0] = 0;
            } else if (completionType == TypeSpecialConstructorCompletion) {
                fillCompletionLine(&completionLine, c->idToProcess, r, TypeDefault, vlevel,0,NULL,vFunCl);
                completionInsertName(c->idToProcess, &completionLine, orderFlag, (void*) c);
            } else if (options.completeParenthesis
                       && (r->storage==StorageMethod || r->storage==StorageConstructor)) {
                completeFunctionOrMethodName(c, orderFlag, vlevel, r, vFunCl);
            } else {
                fillCompletionLine(&completionLine, cname, r, TypeDefault, vlevel, 0, NULL, vFunCl);
                processName(cname, &completionLine, orderFlag, c);
            }
        }
    }
    if (c->idToProcess[0] == 0 && currentLanguage!=LANG_JAVA)
        c->prefix[0] = 0; // no common prefix completed
}


void completeRecNames(Completions *c) {
    TypeModifier *str;
    Symbol *s;
    assert(s_structRecordCompletionType);
    str = s_structRecordCompletionType;
    if (str->kind == TypeStruct || str->kind == TypeUnion) {
        s = str->u.t;
        assert(s);
        completeRecordsNames(c, s, CLASS_TO_ANY, StorageDefault, TypeDefault, 0);
    }
    s_structRecordCompletionType = &errorModifier;
}

static void completeFromSymTab(Completions*c, unsigned storage){
    SymbolCompletionFunctionInfo  info;
    JavaStat              *cs;
    int                     vlevelOffset;

    fillCompletionSymFunInfo(&info, c, storage);
    if (currentLanguage == LANG_JAVA) {
        vlevelOffset = 0;
        for(cs=javaStat; cs!=NULL && cs->thisClass!=NULL ;cs=cs->next) {
            symbolTableMapWithPointer(cs->locals, symbolCompletionFunction, (void*) &info);
            completeRecordsNames(c, cs->thisClass, CLASS_TO_ANY, storage, TypeDefault, vlevelOffset);
            vlevelOffset += NEST_VIRT_COMPL_OFFSET;
        }
    } else {
        symbolTableMapWithPointer(symbolTable, symbolCompletionFunction, (void*) &info);
    }
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
    // TODO: Feels like an API function... getReferencesItemFor(Reference *r)
    for (int i=getNextExistingReferencesItem(0); i != -1; i = getNextExistingReferencesItem(i+1)) {
        ReferencesItem *referencesItem = getReferencesItem(i);
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
    Symbol        *ss1,*ss2;

    assert(t1 && t2);
    for(s1=t1,s2=t2; s1->next!=NULL&&s2->next!=NULL; s1=s1->next,s2=s2->next) {
        if (s1->kind!=s2->kind)
            return false;
    }
    if (s1->next!=NULL || s2->next!=NULL)
        return false;
    if (s1->kind != s2->kind)
        return false;
    if (s1->kind==TypeStruct || s1->kind==TypeUnion || s1->kind==TypeEnum) {
        if (s1->u.t != s2->u.t)
            return false;
    } else if (s1->kind==TypeFunction) {
        if (LANGUAGE(LANG_JAVA)) {
            if (strcmp(s1->u.m.signature,s2->u.m.signature)!=0)
                return false;
        } else {
            for(ss1=s1->u.f.args, ss2=s2->u.f.args;
                ss1!=NULL&&ss2!=NULL;
                ss1=ss1->next, ss2=ss2->next) {
                if (!isEqualType(ss1->u.typeModifier, ss2->u.typeModifier))
                    return false;
            }
            if (ss1!=NULL||ss2!=NULL)
                return false;
        }
    }
    return true;
}

static char *spComplFindNextRecord(ExprTokenType *tok) {
    S_recFindStr    rfs;
    Symbol        *r,*s;
    char *res;
    char            *cname;
    static char     *cnext="next";
    static char     *cprevious="previous";

    s = tok->typeModifier->next->u.t;
    res = NULL;
    assert(s->u.structSpec);
    iniFind(s, &rfs);
    for(;;) {
        Result rr = findStrRecordSym(&rfs, NULL, &r, CLASS_TO_ANY, ACCESSIBILITY_CHECK_YES, VISIBILITY_CHECK_YES);
        if (rr != RESULT_OK) break;
        assert(r);
        cname = r->name;
        CONST_CONSTRUCT_NAME(StorageDefault, r->storage, &cname);
        if (cname!=NULL && javaLinkable(r->access)){
            assert(rfs.currentClass && rfs.currentClass->u.structSpec);
            assert(r->type == TypeDefault);
            if (isEqualType(r->u.typeModifier, tok->typeModifier)) {
                // there is a record of the same type
                if (res == NULL) res = cname;
                else if (strcmp(cname,cnext)==0) res = cnext;
                else if (res!=cnext&&strcmp(cname,"previous")==0) res=cprevious;
            }
        }
    }
    return res;
}

static bool isForCompletionSymbol(
    Completions *c,
    ExprTokenType *token,
    Symbol **sym,
    char   **nextRecord
) {
    Symbol    *sy;

    if (options.serverOperation != OLO_COMPLETION)
        return false;
    if (token->typeModifier==NULL)
        return false;
    if (c->idToProcessLen != 0)
        return false;
    if (token->typeModifier->kind == TypePointer) {
        assert(token->typeModifier->next);
        if (token->typeModifier->next->kind == TypeStruct) {
            *sym = sy = getSymbolFromReference(token->reference);
            if (sy==NULL)
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

/* *************************** JAVA completions ********************** */

static Symbol *javaGetFileNameClass(char *fileName) {
    char *pp = javaCutClassPathFromFileName(fileName);
    return javaGetFieldClass(pp, NULL);
}

static void completeConstructorsFromFile(Completions *c, char *fname) {
    Symbol *memb;
    //&fprintf(dumpOut,"comp %s\n", fname);
    memb = javaGetFileNameClass(fname);
    //&fprintf(dumpOut,"comp %s <-> %s\n", memb->name, c->idToProcess);
    if (strcmp(memb->name, c->idToProcess)==0) {
        /* only when exact match, otherwise it would be too memory costly*/
        //&fprintf(dumpOut,"O.K. %s\n", memb->linkName);
        javaLoadClassSymbolsFromFile(memb);
        completeRecordsNames(c, memb,CLASS_TO_ANY, StorageConstructor,TypeDefault,0);
    }
}

static void completeJavaConstructors(Symbol *s, void *c) {
    if (s->type != TypeStruct) return;
    completeConstructorsFromFile((Completions *)c, s->linkName);
}

/* NOTE: Map-function */
static void javaPackageNameCompletion(
    char        *fname,
    char        *path,
    char        *pack,
    Completions *c,
    void        *idp,
    int         *pstorage
) {
    CompletionLine compLine;
    char *cname;

    if (strchr(fname,'.')!=NULL) return;        /* not very proper */
    cname = StackMemoryAllocC(strlen(fname)+1, char);
    strcpy(cname, fname);
    fillCompletionLine(&compLine, cname, NULL, TypePackage,0, 0 , NULL,NULL);
    processName(cname, &compLine, 1, c);
}

/* NOTE: Map-function */
static void javaTypeNameCompletion(
    char        *fname,
    char        *path,
    char        *pack,
    Completions *c,
    void        *idp,
    int         *pstorage
) {
    char cfname[MAX_FILE_NAME_SIZE];
    CompletionLine compLine;
    char *cname, *suff;
    int len, storage, complType;
    Symbol *memb = NULL;

    if (pstorage == NULL) storage = StorageDefault;
    else storage = *pstorage;
    len = strlen(fname);
    complType = TypePackage;
    if (strchr(fname,'.') != NULL) {        /* not very proper */
        if (strchr(fname,'$') != NULL) return;
        suff = strchr(fname,'.');
        if (suff == NULL) return;
        if (strcmp(suff,".java")==0) len -= 5;
        else if (strcmp(suff,".class")==0) len -= 6;
        else return;
        complType = TypeStruct;
        sprintf(cfname,"%s/%s", pack, fname);
        assert(strlen(cfname)+1 < MAX_FILE_NAME_SIZE);
        memb = javaGetFieldClass(cfname,NULL);
        if (storage == StorageConstructor) {
            sprintf(cfname,"%s/%s/%s", path, pack, fname);
            assert(strlen(cfname)+1 < MAX_FILE_NAME_SIZE);
            memb = javaGetFileNameClass(cfname);
            completeConstructorsFromFile(c, cfname);
        }
    }
    cname = StackMemoryAllocC(len+1, char);
    strncpy(cname, fname, len);
    cname[len]=0;
    fillCompletionLine(&compLine, cname, memb, complType,0, 0 , NULL,NULL);
    processName(cname, &compLine, 1, c);
}

static void javaCompleteNestedClasses(  Completions *c,
                                        Symbol *cclas,
                                        int storage
                                        ) {
    CompletionLine         compLine;
    int             i;
    Symbol        *memb;
    Symbol        *str;
    str = cclas;
    // TODO count inheritance and virtual levels (at least for nested classes)
    for(str=cclas;
        str!=NULL && str->u.structSpec->super!=NULL;
        str=str->u.structSpec->super->element) {
        assert(str && str->u.structSpec);
        for(i=0; i<str->u.structSpec->nestedCount; i++) {
            if (str->u.structSpec->nest[i].membFlag) {
                memb = str->u.structSpec->nest[i].cl;
                assert(memb);
                memb->access |= str->u.structSpec->nest[i].accFlags;  // hack!!!
                fillCompletionLine(&compLine, memb->name, memb, TypeStruct,0, 0 , NULL,NULL);
                processName(memb->name, &compLine, 1, c);
                if (storage == StorageConstructor) {
                    javaLoadClassSymbolsFromFile(memb);
                    completeRecordsNames(c, memb, CLASS_TO_ANY, StorageConstructor, TypeDefault, 0);
                }
            }
        }
    }
}

static void javaCompleteNestedClSingleName(Completions *cc) {
    JavaStat  *cs;

    for(cs=javaStat; cs!=NULL && cs->thisClass!=NULL; cs=cs->next) {
        javaCompleteNestedClasses(cc, cs->thisClass, StorageDefault);
    }
}


static void javaCompleteComposedName(Completions *c,
                                     int classif,
                                     int storage,
                                     int innerConstruct
) {
    Symbol *str;
    TypeModifier *expr;
    int nameType;
    char packageName[MAX_FILE_NAME_SIZE];
    Reference *orr;

    nameType = javaClassifyAmbiguousName(javaStat->lastParsedName,NULL,&str,
                                         &expr,&orr,NULL, USELESS_FQT_REFS_ALLOWED,classif,UsageUsed);
    if (innerConstruct && nameType != TypeExpression)
        return;
    if (nameType == TypeExpression) {
        if (expr->kind == TypeArray) expr = &s_javaArrayObjectSymbol.u.structSpec->stype;
        if (expr->kind != TypeStruct)
            return;
        str = expr->u.t;
    }
    /* complete packages and classes from file system */
    if (nameType==TypePackage) {
        javaCreateComposedName(NULL,javaStat->lastParsedName,'/',NULL,packageName,MAX_FILE_NAME_SIZE);
        javaMapOverDirectoryFiles1(packageName, javaTypeNameCompletion,
                               c,javaStat->lastParsedName, &storage);
    }
    /* complete inner classes */
    if (nameType==TypeStruct || innerConstruct){
        javaLoadClassSymbolsFromFile(str);
        javaCompleteNestedClasses(c, str, storage);
    }
    /* complete fields and methods */
    if (classif==CLASS_TO_EXPR && str!=NULL && storage!=StorageConstructor
        && (nameType==TypeStruct || nameType==TypeExpression)) {
        javaLoadClassSymbolsFromFile(str);
        completeRecordsNames(c, str, CLASS_TO_ANY, storage, TypeDefault, 0);
    }
}

void javaHintCompleteMethodParameters(Completions *c) {
    CompletionLine     compLine;
    Symbol             *r, *vFunCl;
    S_recFindStr       *rfs;
    S_typeModifierList *aaa;
    int                visibilityCheck, accessCheck, vlevel, actArgi;
    Result rr;
    char               *mname;
    char               actArg[MAX_PROFILE_SIZE];

    if (c->idToProcessLen != 0)
        return;
    if (parsedClassInfo.erfsForParameterCompletion==NULL)
        return;
    r = parsedClassInfo.erfsForParameterCompletion->memb;
    rfs = &parsedClassInfo.erfsForParameterCompletion->s;
    mname = r->name;
    visibilityCheck = VISIBILITY_CHECK_NO;
    accessCheck = calculateAccessCheckOption();
    // partial actual parameters

    *actArg = 0; actArgi = 0;
    for(aaa=parsedClassInfo.erfsForParameterCompletion->params; aaa!=NULL; aaa=aaa->next) {
        actArgi += javaTypeToString(aaa->d,actArg+actArgi,MAX_PROFILE_SIZE-actArgi);
    }
    do {
        assert(r != NULL);
        if (*actArg==0 || javaMethodApplicability(r,actArg)==PROFILE_PARTIALLY_APPLICABLE) {
            vFunCl = rfs->currentClass;
            if (vFunCl->u.structSpec->classFileNumber == -1) {
                vFunCl = NULL;
            }
            vlevel = rfs->superClassesCount;
            fillCompletionLine(&compLine, r->name, r, TypeDefault, vlevel,0,NULL,vFunCl);
            processName(r->name, &compLine, 0, c);
        }
        rr = findStrRecordSym(rfs, mname, &r, CLASS_TO_METHOD, accessCheck, visibilityCheck);
    } while (rr == RESULT_OK);
    if (c->alternativeIndex != 0) {
        c->prefix[0]=0;
        c->fullMatchFlag = true;
        c->noFocusOnCompletions = true;
    }
    if (options.serverOperation != OLO_SEARCH)
        collectedCompletions.abortFurtherCompletions = true;
}


void javaCompletePackageSingleName(Completions*c) {
    javaMapOverDirectoryFiles2(NULL, javaPackageNameCompletion, c, NULL, NULL);
}

void javaCompleteThisPackageName(Completions *c) {
    CompletionLine     compLine;
    static char cname[TMP_STRING_SIZE];
    char        *cc, *ss, *dd;
    if (c->idToProcessLen != 0) return;
    ss = javaCutSourcePathFromFileName(getRealFileName_static(getFileItem(olOriginalFileNumber)->name));
    strcpy(cname, ss);
    dd = lastOccurenceInString(cname, '.');
    if (dd!=NULL) *dd=0;
    javaDotifyFileName(cname);
    cc = lastOccurenceInString(cname, '.');
    if (cc==NULL) return;
    *cc++ = ';'; *cc = 0;
    fillCompletionLine(&compLine,cname,NULL,TypeSpecialComplet,0,0,NULL,NULL);
    completeName(cname, &compLine, 0, c);
}

static void javaCompleteThisClassDefinitionName(Completions*c) {
    CompletionLine     compLine;
    static char cname[TMP_STRING_SIZE];
    char        *cc;

    javaGetClassNameFromFileNumber(olOriginalFileNumber, cname, DOTIFY_NAME);
    cc = strchr(cname,0);
    assert(cc!=NULL);
    *cc++ = ' '; *cc=0;
    cc = lastOccurenceInString(cname, '.');
    if (cc==NULL)
        return;
    cc++;
    fillCompletionLine(&compLine,cc,NULL,TypeSpecialComplet,0,0,NULL,NULL);
    completeName(cc, &compLine, 0, c);
}

void javaCompleteClassDefinitionNameSpecial(Completions*c) {
    if (c->idToProcessLen != 0) return;
    javaCompleteThisClassDefinitionName(c);
}

void javaCompleteClassDefinitionName(Completions*c) {
    SymbolCompletionInfo ii;
    javaCompleteThisClassDefinitionName(c);
    // order is important because of hack in nestedcl Access modifs
    javaCompleteNestedClSingleName(c);
    fillCompletionSymInfo(&ii, c, TypeStruct);
    symbolTableMapWithPointer(symbolTable, completeFun, (void*) &ii);
}

void javaCompletePackageCompName(Completions*c) {
    javaClassifyToPackageNameAndAddRefs(javaStat->lastParsedName, UsageUsed);
    javaMapOverDirectoryFiles2(javaStat->lastParsedName,
                           javaPackageNameCompletion, c, javaStat->lastParsedName, NULL);
}

void javaCompleteTypeSingleName(Completions*c) {
    SymbolCompletionInfo ii;
    // order is important because of hack in nestedcl Access modifs
    javaCompleteNestedClSingleName(c);
    javaMapOverDirectoryFiles2(NULL, javaTypeNameCompletion, c, NULL, NULL);
    fillCompletionSymInfo(&ii, c, TypeStruct);
    symbolTableMapWithPointer(symbolTable, completeFun, (void*) &ii);
}

static void completeFqtFromFileName(char *file, void *cfmpi) {
    char                    ttt[MAX_FILE_NAME_SIZE];
    char                    sss[MAX_FILE_NAME_SIZE];
    char                    *suff, *sname, *ss;
    CompletionLine                 compLine;
    FqtMapCompletionInfo  *fmi;
    Completions           *c;
    Symbol                *memb;

    fmi = (FqtMapCompletionInfo *) cfmpi;
    c = fmi->res;
    suff = getFileSuffix(file);
    if (compareFileNames(suff, ".class")==0 || compareFileNames(suff, ".java")==0) {
        sprintf(ttt, "%s", file);
        assert(strlen(ttt) < MAX_FILE_NAME_SIZE-1);
        suff = lastOccurenceInString(ttt, '.');
        assert(suff);
        *suff = 0;
        sname = lastOccurenceOfSlashOrBackslash(ttt);
        if (sname == NULL) sname = ttt;
        else sname ++;
        if (pathncmp(c->idToProcess, sname, c->idToProcessLen, options.completionCaseSensitive)==0
            || (fmi->completionType == FQT_COMPLETE_ALSO_ON_PACKAGE
                && pathncmp(c->idToProcess, ttt, c->idToProcessLen, options.completionCaseSensitive)==0)) {
            memb = javaGetFieldClass(ttt, &sname);
            linkNamePrettyPrint(sss, ttt, TMP_STRING_SIZE, LONG_NAME);
            ss = StackMemoryAllocC(strlen(sss)+1, char);
            strcpy(ss, sss);
            sname = lastOccurenceInString(ss,'.');
            // do not complete names not containing dot (== not fqt)
            if (sname!=NULL) {
                sname++;
                fillCompletionLine(&compLine,ss,memb,TypeNonImportedClass,0,0,NULL,NULL);
                if (fmi->completionType == FQT_COMPLETE_ALSO_ON_PACKAGE) {
                    processName(ss, &compLine, 1, c);
                } else {
                    processName(sname, &compLine, 1, c);
                }
                // reset common prefix and full match in order to always show window
                c->prefix[0]     = 0;
                c->fullMatchFlag = true;
            }
        }
    }
}

/* NOTE: Map-function */
static void completeFqtClassFileFromZipArchiv(char *zip, char *file, void *cfmpi) {
    completeFqtFromFileName(file, cfmpi);
}

static bool isCurrentPackageName(char *fn) {
    int plen;
    if (s_javaThisPackageName!=NULL && s_javaThisPackageName[0]!=0) {
        plen = strlen(s_javaThisPackageName);
        if (filenameCompare(fn, s_javaThisPackageName, plen)==0)
            return true;
    }
    return false;
}

static void completeFqtClassFileFromFileTab(FileItem *fi, void *cfmpi) {
    char    *fn;
    assert(fi!=NULL);
    if (fi->name[0] == ZIP_SEPARATOR_CHAR) {
        // remove current package
        fn = fi->name+1;
        if (!isCurrentPackageName(fn)) {
            completeFqtFromFileName(fn, cfmpi);
        }
    }
}

//#define MAP_FUN_SIGNATURE char *file, char *a1, char *a2, Completions *a3, void *a4, int *a5
// Not using a3, a5
static void completeRecursivelyFqtNamesFromDirectory(MAP_FUN_SIGNATURE) {
    char *fname, *dir, *path;
    char fn[MAX_FILE_NAME_SIZE];
    int pathLength;

    fname = file;
    dir = a1;
    path = a2;
    if (strcmp(fname,".")==0 || strcmp(fname,"..")==0)
        return;

    // do not descent too deep
    if (strlen(dir)+strlen(fname) >= MAX_FILE_NAME_SIZE-50)
        return;
    sprintf(fn,"%s%c%s",dir,FILE_PATH_SEPARATOR,fname);

    if (directoryExists(fn)) {
        mapOverDirectoryFiles(fn, completeRecursivelyFqtNamesFromDirectory, DO_NOT_ALLOW_EDITOR_FILES,
                          fn, path, NULL, a4, NULL);
    } else if (fileExists(fn)) {
        // O.K. cut the path
        assert(path!=NULL);
        pathLength = strlen(path);
        assert(filenameCompare(fn, path, pathLength)==0);
        if (fn[pathLength]!=0 && !isCurrentPackageName(fn)) {
            completeFqtFromFileName(fn+pathLength+1, a4);
        }
    }
}

static void javaFqtCompletions(Completions *c, enum fqtCompletion completionType) {
    FqtMapCompletionInfo  info;

    fillCompletionFqtMapInfo(&info, c, completionType);
    if (options.fqtNameToCompletions == 0)
        return;

    // fqt from .jars
    for (int i=0; i<MAX_JAVA_ZIP_ARCHIVES && zipArchiveTable[i].fn[0]!=0; i++) {
        fsRecMapOnFiles(zipArchiveTable[i].dir, zipArchiveTable[i].fn,
                        "", completeFqtClassFileFromZipArchiv, &info);
    }
    if (options.fqtNameToCompletions <= 1)
        return;

    // fqt from filetab
    for (int i=getNextExistingFileNumber(0); i != -1; i = getNextExistingFileNumber(i+1)) {
        completeFqtClassFileFromFileTab(getFileItem(i), &info);
    }
    if (options.fqtNameToCompletions <= 2)
        return;

    // fqt from classpath
    for (StringList *path=javaClassPaths; path!=NULL; path=path->next) {
        mapOverDirectoryFiles(path->string, completeRecursivelyFqtNamesFromDirectory, DO_NOT_ALLOW_EDITOR_FILES,
                          path->string, path->string, NULL, &info, NULL);
    }
    if (options.fqtNameToCompletions <= 3)
        return;

    // fqt from sourcepath
    MapOverPaths(javaSourcePaths, {
        mapOverDirectoryFiles(currentPath, completeRecursivelyFqtNamesFromDirectory, DO_NOT_ALLOW_EDITOR_FILES,
                          currentPath, currentPath, NULL, &info, NULL);
    });
}

void javaHintCompleteNonImportedTypes(Completions*c) {
    javaFqtCompletions(c, FQT_COMPLETE_DEFAULT);
}

void javaHintImportFqt(Completions*c) {
    javaFqtCompletions(c, FQT_COMPLETE_ALSO_ON_PACKAGE);
}

void javaHintVariableName(Completions*c) {
    CompletionLine compLine;
    char ss[TMP_STRING_SIZE];
    char *name, *affect1, *affect2;

    if (! LANGUAGE(LANG_JAVA))
        return;
    if (c->idToProcessLen != 0)
        return;
    if (s_lastReturnedLexem != IDENTIFIER)
        return;

    sprintf(ss, "%s", uniyylval->ast_id.data->name);
    //&sprintf(ss, "%s", yytext);
    if (ss[0]!=0) ss[0] = tolower(ss[0]);
    name = StackMemoryAllocC(strlen(ss)+1, char);
    strcpy(name, ss);
    sprintf(ss, "%s = new %s", name, uniyylval->ast_id.data->name);
    //&sprintf(ss, "%s = new %s", name, yytext);
    affect1 = StackMemoryAllocC(strlen(ss)+1, char);
    strcpy(affect1, ss);
    sprintf(ss, "%s = null;", name);
    affect2 = StackMemoryAllocC(strlen(ss)+1, char);
    strcpy(affect2, ss);
    fillCompletionLine(&compLine, affect1, NULL, TypeSpecialComplet,0,0,NULL,NULL);
    processName(affect1, &compLine, 0, c);
    fillCompletionLine(&compLine, affect2, NULL, TypeSpecialComplet,0,0,NULL,NULL);
    processName(affect2, &compLine, 0, c);
    fillCompletionLine(&compLine, name, NULL, TypeSpecialComplet,0,0,NULL,NULL);
    processName(name, &compLine, 0, c);
    c->prefix[0] = 0;
}

void javaCompleteTypeCompName(Completions *c) {
    javaCompleteComposedName(c, CLASS_TO_TYPE, StorageDefault,0);
}

void javaCompleteConstructSingleName(Completions *c) {
    symbolTableMapWithPointer(symbolTable, completeJavaConstructors, c);
}

void javaCompleteHintForConstructSingleName(Completions *c) {
    CompletionLine     compLine;
    char        *name;
    if (c->idToProcessLen == 0 && options.serverOperation == OLO_COMPLETION) {
        // O.K. wizard completion
        if (parsedInfo.lastAssignmentStruct!=NULL) {
            name = parsedInfo.lastAssignmentStruct->name;
            fillCompletionLine(&compLine, name, NULL, TypeSpecialComplet,0,0,NULL,NULL);
            processName(name, &compLine, 0, c);
        }
    }
}

void javaCompleteConstructCompName(Completions*c) {
    javaCompleteComposedName(c, CLASS_TO_TYPE, StorageConstructor,0);
}

void javaCompleteConstructNestNameName(Completions*c) {
    javaCompleteComposedName(c, CLASS_TO_EXPR, StorageConstructor,1);
}

void javaCompleteConstructNestPrimName(Completions*c) {
    Symbol *memb;
    if (s_javaCompletionLastPrimary == NULL)
        return;
    if (s_javaCompletionLastPrimary->kind == TypeStruct) {
        memb = s_javaCompletionLastPrimary->u.t;
    } else
        return;
    assert(memb);
    javaCompleteNestedClasses(c, memb, StorageConstructor);
}

void javaCompleteExprSingleName(Completions*c) {
    SymbolCompletionInfo ii;
    javaMapOverDirectoryFiles1(NULL, javaTypeNameCompletion, c, NULL, NULL);
    fillCompletionSymInfo(&ii, c, TypeStruct);
    symbolTableMapWithPointer(symbolTable, completeFun, (void*) &ii);
    completeFromSymTab(c, StorageDefault);
}

void javaCompleteThisConstructor (Completions *c) {
    Symbol *memb;
    if (strcmp(c->idToProcess,"this")!=0)
        return;
    if (options.serverOperation == OLO_SEARCH)
        return;
    memb = javaStat->thisClass;
    javaLoadClassSymbolsFromFile(memb);
    completeRecordsNames(c, memb, CLASS_TO_ANY, StorageConstructor,
                         TypeSpecialConstructorCompletion,0);
}

void javaCompleteSuperConstructor (Completions *c) {
    Symbol *memb;
    if (strcmp(c->idToProcess,"super")!=0)
        return;
    if (options.serverOperation == OLO_SEARCH)
        return;
    memb = javaCurrentSuperClass();
    javaLoadClassSymbolsFromFile(memb);
    completeRecordsNames(c, memb, CLASS_TO_ANY, StorageConstructor,
                         TypeSpecialConstructorCompletion,0);
}

void javaCompleteSuperNestedConstructor (Completions *c) {
    if (strcmp(c->idToProcess,"super")!=0)
        return;
    //TODO!!!
}

void javaCompleteExprCompName(Completions *c) {
    javaCompleteComposedName(c, CLASS_TO_EXPR, StorageDefault,0);
}

void javaCompleteStrRecordPrimary(Completions *c) {
    Symbol *memb;
    if (s_javaCompletionLastPrimary == NULL)
        return;
    if (s_javaCompletionLastPrimary->kind == TypeStruct) {
        memb = s_javaCompletionLastPrimary->u.t;
    } else if (s_javaCompletionLastPrimary->kind == TypeArray) {
        memb = &s_javaArrayObjectSymbol;
    } else
        return;
    assert(memb);
    javaLoadClassSymbolsFromFile(memb);
    /*fprintf(dumpOut,": completing %s\n",memb->linkName);fflush(dumpOut);*/
    completeRecordsNames(c, memb, CLASS_TO_ANY, StorageDefault,TypeDefault,0);
}

void javaCompleteStrRecordSuper(Completions *c) {
    Symbol *memb;
    memb = javaCurrentSuperClass();
    if (memb == &errorSymbol || memb->type==TypeError)
        return;
    assert(memb);
    javaLoadClassSymbolsFromFile(memb);
    completeRecordsNames(c, memb, CLASS_TO_ANY, StorageDefault,TypeDefault,0);
}

void javaCompleteStrRecordQualifiedSuper(Completions *c) {
    Symbol *str;
    TypeModifier *expr;
    Reference *rr, *lastUselessRef;
    int ttype;

    lastUselessRef = NULL;
    ttype = javaClassifyAmbiguousName(javaStat->lastParsedName, NULL, &str, &expr, &rr,
                                      &lastUselessRef, USELESS_FQT_REFS_ALLOWED, CLASS_TO_TYPE, UsageUsed);
    if (ttype != TypeStruct)
        return;
    javaLoadClassSymbolsFromFile(str);
    str = javaGetSuperClass(str);
    if (str == &errorSymbol || str->type==TypeError)
        return;
    assert(str);
    completeRecordsNames(c, str,CLASS_TO_ANY, StorageDefault,TypeDefault,0);
}

void javaCompleteUpMethodSingleName(Completions *c) {
    if (javaStat!=NULL) {
        completeRecordsNames(c, javaStat->thisClass,CLASS_TO_ANY,
                             StorageDefault,TypeDefault,0);
    }
}

void javaCompleteFullInheritedMethodHeader(Completions *c) {
    if (c->idToProcessLen != 0) return;
    if (javaStat!=NULL) {
        completeRecordsNames(c,javaStat->thisClass,CLASS_TO_METHOD,
                             StorageDefault,TypeInheritedFullMethod,0);
    }
#if ZERO
    // completing main is sometimes very strange, especially
    // in case there is no inherited method from direct superclass
    maindecl = "public static void main(String args[])";
    fill_cline(&compLine, maindecl, NULL, TypeInheritedFullMethod, 0,0,NULL,NULL);
    processName(maindecl, &compLine, 0, (void*) c);
#endif
}

/* this is only used in completionsTab */
void javaCompleteMethodCompName(Completions *c) {
    javaCompleteExprCompName(c);
}


/* ************************** Yacc stuff ************************ */

static void completeFromXrefFun(ReferencesItem *s, void *c) {
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
