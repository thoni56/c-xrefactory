#include "jsemact.h"

#include "commons.h"
#include "globals.h"
#include "options.h"
#include "misc.h"
#include "proto.h"
#include "usage.h"
#include "yylex.h"
#include "semact.h"
#include "cxref.h"
#include "classfilereader.h"
#include "filedescriptor.h"
#include "editor.h"
#include "symbol.h"
#include "javafqttab.h"
#include "list.h"
#include "fileio.h"
#include "filetable.h"
#include "stackmemory.h"

#include "parsers.h"
#include "recyacc.h"
#include "jslsemact.h"

#include "log.h"


typedef enum {
    RESULT_IS_CLASS_FILE,
    RESULT_IS_JAVA_FILE,
    RESULT_NO_FILE_FOUND
} FindJavaFileResult;


JavaStat *javaStat;
JavaStat s_initJavaStat;


#define IsJavaReferenceType(m) (m==TypeStruct || m==TypeArray)



void fillJavaStat(JavaStat *javaStat, IdList *className, TypeModifier *thisType, Symbol *thisClass,
                  int currentNestedIndex, char *currentPackage, char *unnamedPackagePath,
                  char *namedPackagePath, SymbolTable *locals, IdList *lastParsedName,
                  unsigned methodModifiers, CurrentlyParsedClassInfo parsingPositions, int classFileNumber,
                  JavaStat *next) {

    javaStat->className = className;
    javaStat->thisType = thisType;
    javaStat->thisClass = thisClass;
    javaStat->currentNestedIndex = currentNestedIndex;
    javaStat->currentPackage = currentPackage;
    javaStat->unnamedPackagePath = unnamedPackagePath;
    javaStat->namedPackagePath = namedPackagePath;
    javaStat->locals = locals;
    javaStat->lastParsedName = lastParsedName;
    javaStat->methodModifiers = methodModifiers;
    javaStat->cp = parsingPositions;
    javaStat->classFileNumber = classFileNumber;
    javaStat->next = next;
}


static int javaFqtNameIsFromThePackage(char *cpack, char *classFqName) {
    char   *p1,*p2;

    for(p1=cpack, p2=classFqName; *p1 == *p2; p1++,p2++) ;
    if (*p1 != 0) return false;
    if (*p2 == 0) return false;
    //& if (*p2 != '/') return false;
    for(p2++; *p2; p2++) if (*p2 == '/') return false;
    return true;
}

static int javaFqtNamesAreFromTheSamePackage(char *nn1, char *nn2) {
    char   *p1,*p2;

    if (nn1==NULL || nn2==NULL) return false;
//&fprintf(dumpOut,"checking equal package %s %s\n", nn1, nn2);
    for(p1=nn1, p2=nn2; *p1 == *p2 && *p1 && *p2; p1++,p2++) ;
    for(; *p1; p1++) if (*p1 == '/') return false;
    for(; *p2; p2++) if (*p2 == '/') return false;
//*fprintf(dumpOut,"YES EQUALS\n");
    return true;
}

int javaClassIsInCurrentPackage(Symbol *cl) {
    if (s_jsl!=NULL) {
        if (s_jsl->classStat->thisClass == NULL) {
            // probably import class.*; (nested subclasses)
            return javaFqtNameIsFromThePackage(s_jsl->classStat->thisPackage,
                                               cl->linkName);
        } else {
            return javaFqtNamesAreFromTheSamePackage(cl->linkName,
                                                     s_jsl->classStat->thisClass->linkName);
        }
    } else {
        if (javaStat->thisClass == NULL) {
            // probably import class.*; (nested subclasses)
            return javaFqtNameIsFromThePackage(javaStat->currentPackage,
                                               cl->linkName);
        } else {
            return javaFqtNamesAreFromTheSamePackage(cl->linkName,
                                                     javaStat->thisClass->linkName);
        }
    }
}

static Symbol *javaFQTypeSymbolDefinitionCreate(char *name, char *fqName) {
    Symbol *memb;
    SymbolList *pppl;
    char *lname1, *sname;

    sname = cfAllocc(strlen(name)+1, char);
    strcpy(sname, name);

    lname1 = cfAllocc(strlen(fqName)+1, char);
    strcpy(lname1, fqName);

    memb = cfAlloc(Symbol);
    fillSymbol(memb, sname, lname1, noPosition);

    memb->type = TypeStruct;
    memb->storage = StorageNone;

    memb->u.structSpec = cfAlloc(S_symStructSpec);

    initSymStructSpec(memb->u.structSpec, /*.records=*/NULL);
    TypeModifier *type = &memb->u.structSpec->type;
    /* Assumed to be Struct/Union/Enum? */
    initTypeModifierAsStructUnionOrEnum(type, /*.kind=*/TypeStruct, /*.u.t=*/memb,
                                            /*.typedefSymbol=*/NULL, /*.next=*/NULL);
    TypeModifier *ptrtype = &memb->u.structSpec->ptrtype;
    initTypeModifierAsPointer(ptrtype, &memb->u.structSpec->type);

    pppl = cfAlloc(SymbolList);
    /* REPLACED: FILL_symbolList(pppl, memb, NULL); with compound literal */
    *pppl = (SymbolList){.element = memb, .next = NULL};

    javaFqtTableAdd(&javaFqtTable, pppl);

    // I think this can be there, as it is very used
    javaCreateClassFileItem(memb);
    // this would be too strong, javaLoadClassSymbolsFromFile(memb);
    /* so, this table is not freed when freeing FrameAllocations */
    //&if (stringContainsSubstring(fqName, "ComboBoxTreeFilter")) {fprintf(dumpOut,"\nAAAAAAAAAAAAA : %s %s\n\n", name, fqName);} if (strcmp(fqName, "ComboBoxTreeFilter")==0) assert(0);
    return memb;
}

Symbol *javaFQTypeSymbolDefinition(char *name, char *fqName) {
    Symbol symbol, *member;
    SymbolList ppl, *pppl;

    /* This probably creates a SymbolList element so ..IsMember() can be used */
    /* TODO: SymbolList is here used as a FQT, create that type... */
    fillSymbol(&symbol, name, fqName, noPosition);
    symbol.type = TypeStruct;
    symbol.storage = StorageNone;

    /* REPLACED: FILL_symbolList(&ppl, &symbol, NULL); with compound literal */
    ppl = (SymbolList){.element = &symbol, .next = NULL};

    if (javaFqtTableIsMember(&javaFqtTable, &ppl, NULL, &pppl)) {
        member = pppl->element;
    } else {
        member = javaFQTypeSymbolDefinitionCreate(name, fqName);
    }
    return member;
}

char *javaImportSymbolName_st(int file, int line, int coll) {
    static char res[MAX_CX_SYMBOL_SIZE];
    sprintf(res, "%s:%x:%x:%x%cimport on line %3d", LINK_NAME_IMPORT_STATEMENT,
            file, line, coll,
            LINK_NAME_SEPARATOR,
            /*& simpleFileName(getRealFileName_static(getFileItem(file)->name)), &*/
            line);
    return res;
}

int javaLinkNameIsAnnonymousClass(char *linkname) {
    char *ss;
    // this is a very hack too!!   TODO do this seriously
    ss = linkname;
    while ((ss=strchr(ss, '$'))!=NULL) {
        ss++;
        if (isdigit(*ss)) return true;
    }
    return false;
}

void addThisCxReferences(int classIndex, Position *pos) {
    int usage;
    if (classIndex == javaStat->classFileNumber) {
        usage = UsageMaybeThis;
    } else {
        usage = UsageMaybeQualifiedThis;
    }
    addSpecialFieldReference(LINK_NAME_MAYBE_THIS_ITEM,StorageField,
                             classIndex, pos, usage);
}
