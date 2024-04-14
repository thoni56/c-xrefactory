#ifndef JSEMACT_H_INCLUDED
#define JSEMACT_H_INCLUDED

#include "jsltypetab.h"
#include "proto.h"
#include "symbol.h"
#include "symboltable.h"
#include "stackmemory.h"


typedef struct javaStat {
    struct idList           *className;          /* this class name */
    struct typeModifier     *thisType;           /* this class type */
    struct symbol           *thisClass;          /* this class definition */
    int                      currentNestedIndex; /* currently parsed nested class */
    char                    *currentPackage;     /* current package */
    char                    *unnamedPackagePath; /* directory for unnamed package */
    char                    *namedPackagePath;   /* inferred source-path for named package */
    struct symbolTable      *locals;             /* args and local variables */
    struct idList           *lastParsedName;
    unsigned                 methodModifiers; /* currently parsed method modifs */
    struct currentlyParsedClassInfo cp;              /* some parsing positions */
    int                      classFileNumber;  /* this file class index */
    struct javaStat         *next;            /* outer class */
} JavaStat;

extern JavaStat *javaStat;
extern JavaStat s_initJavaStat;


extern void fill_nestedSpec(S_nestedSpec *nestedSpec, struct symbol *cl,
                            char membFlag, short unsigned  accFlags);
extern void fillJavaStat(JavaStat *javaStat, IdList *className, TypeModifier *thisType, Symbol *thisClass,
                         int currentNestedIndex, char *currentPackage, char *unnamedPackageDir,
                         char *namedPackageDir, SymbolTable *locals, IdList *lastParsedName,
                         unsigned methodModifiers, CurrentlyParsedClassInfo parsingPositions, int classFileIndex,
                         JavaStat *next);
extern void javaCheckForPrimaryStartInNameList(IdList *name, Position *pp);
extern void javaCheckForStaticPrefixInNameList(IdList *name, Position *pp);

extern char *javaImportSymbolName_st(int file, int line, int coll);
extern int javaIsYetInTheClass(
                               Symbol	*clas,
                               char		*lname,
                               Symbol	**eq
                               );
extern Symbol * javaGetFieldClass(char *fieldLinkName, char **fieldAdr);
extern int javaLinkNameIsAnnonymousClass(char *linkname);
extern void addThisCxReferences(int classIndex, Position *pos);
extern void javaLoadClassSymbolsFromFile(Symbol *memb);
extern Symbol *javaFQTypeSymbolDefinition(char *name, char *fqName);
extern TypeModifier *javaNewAfterName(IdList *name, Id *id, IdList *idl);
extern TypeModifier *javaMethodInvocationT(	TypeModifier *tt,
                                                Id *name,
                                                S_typeModifierList *args
                                                );
extern TypeModifier *javaMethodInvocationS(	Id *super,
                                                Id *name,
                                                S_typeModifierList *args
                                                );
extern int javaClassIsInCurrentPackage(Symbol *cl);

#endif
