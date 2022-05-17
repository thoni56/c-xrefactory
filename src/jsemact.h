#ifndef JSEMACT_H_INCLUDED
#define JSEMACT_H_INCLUDED

#include "jsltypetab.h"
#include "proto.h"
#include "symbol.h"
#include "symboltable.h"

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
    struct currentlyParsedCl cp;              /* some parsing positions */
    int                      classFileIndex;  /* this file class index */
    struct javaStat         *next;            /* outer class */
} S_javaStat;

extern S_javaStat *s_javaStat;
extern S_javaStat s_initJavaStat;


extern void fill_nestedSpec(S_nestedSpec *nestedSpec, struct symbol *cl,
                            char membFlag, short unsigned  accFlags);
extern void fillJavaStat(S_javaStat *javaStat, IdList *className, TypeModifier *thisType, Symbol *thisClass,
                         int currentNestedIndex, char *currentPackage, char *unnamedPackageDir,
                         char *namedPackageDir, SymbolTable *locals, IdList *lastParsedName,
                         unsigned methodModifiers, S_currentlyParsedCl parsingPositions, int classFileIndex,
                         S_javaStat *next);
extern void javaCheckForPrimaryStart(Position *cpos, Position *pp);
extern void javaCheckForPrimaryStartInNameList(IdList *name, Position *pp);
extern void javaCheckForStaticPrefixStart(Position *cpos, Position *bpos);
extern void javaCheckForStaticPrefixInNameList(IdList *name, Position *pp);
extern Position *javaGetNameStartingPosition(IdList *name);
extern char *javaCreateComposedName(
                                    char			*prefix,
                                    IdList   *className,
                                    int             classNameSeparator,
                                    char            *name,
                                    char			*resBuff,
                                    int				resBufSize
                                    );
extern int findTopLevelName(
                            char                *name,
                            S_recFindStr        *resRfs,
                            Symbol			**resMemb,
                            int                 classif
                            );
extern int javaClassifySingleAmbigNameToTypeOrPack(IdList *name,
                                                   Symbol **str,
                                                   IncludeCxrefs cxrefFlag
                                                   );
extern void javaAddImportConstructionReference(Position *importPos, Position *pos, int usage);
extern int javaClassifyAmbiguousName(
                                     IdList *name,
                                     S_recFindStr *rfs,
                                     Symbol **str,
                                     TypeModifier **expr,
                                     Reference **oref,
                                     Reference **rdtoref, int allowUselesFqtRefs,
                                     int classif,
                                     int usage
                                     );
extern Reference *javaClassifyToTypeOrPackageName(IdList *tname, int usage, Symbol **str, int allowUselesFqtRefs);
extern Reference *javaClassifyToTypeName(IdList *tname, int usage, Symbol **str, int allowUselesFqtRefs);
extern Symbol * javaQualifiedThis(IdList *tname, Id *thisid);
extern void javaClassifyToPackageName( IdList *id );
extern void javaClassifyToPackageNameAndAddRefs(IdList *id, int usage);
extern char *javaImportSymbolName_st(int file, int line, int coll);
extern TypeModifier *javaClassifyToExpressionName(IdList *name,Reference **oref);
extern Symbol *javaTypeNameDefinition(IdList *tname);
extern void javaSetFieldLinkName(Symbol *d);
extern void javaAddPackageDefinition(IdList *id);
extern Symbol *javaAddType(IdList *class, Access access, Position *p);
extern Symbol *javaCreateNewMethod(char *name, Position *pos, int mem);
extern int javaTypeToString(TypeModifier *type, char *pp, int ppSize);
extern int javaIsYetInTheClass(
                               Symbol	*clas,
                               char		*lname,
                               Symbol	**eq
                               );
extern int javaSetFunctionLinkName(Symbol *clas, Symbol *decl, enum memoryClass mem);
extern Symbol * javaGetFieldClass(char *fieldLinkName, char **fieldAdr);
extern bool javaTypeFileExist(IdList *name);
extern Symbol *javaTypeSymbolDefinition(IdList *tname, int accessFlags,int addType);
extern Symbol *javaTypeSymbolUsage(IdList *tname, int accessFlags);
extern void javaReadSymbolFromSourceFileEnd(void);
extern void javaReadSymbolFromSourceFileInit( int sourceFileNum,
                                              JslTypeTab *typeTab );
extern void javaReadSymbolsFromSourceFileNoFreeing(char *fname, char *asfname);
extern void javaReadSymbolsFromSourceFile(char *fname);
extern int javaLinkNameIsAnnonymousClass(char *linkname);
extern int javaLinkNameIsANestedClass(char *cname);
extern int isANestedClass(Symbol *ss);
extern void addSuperMethodCxReferences(int classIndex, Position *pos);
extern Reference * addUselessFQTReference(int classIndex, Position *pos);
extern Reference *addUnimportedTypeLongReference(int classIndex, Position *pos);
extern void addThisCxReferences(int classIndex, Position *pos);
extern void javaLoadClassSymbolsFromFile(Symbol *memb);
extern Symbol *javaPrependDirectEnclosingInstanceArgument(Symbol *args);
extern void addMethodCxReferences(unsigned modif, Symbol *method, Symbol *clas);
extern Symbol *javaMethodHeader(unsigned modif, Symbol *type, Symbol *decl, int storage);
extern void javaAddMethodParametersToSymTable(Symbol *method);
extern void javaMethodBodyBeginning(Symbol *method);
extern void javaMethodBodyEnding(Position *pos);
extern Symbol *javaFQTypeSymbolDefinition(char *name, char *fqName);
extern TypeModifier *javaClassNameType(IdList *typeName);
extern TypeModifier *javaNewAfterName(IdList *name, Id *id, IdList *idl);
extern int javaIsInnerAndCanGetUnnamedEnclosingInstance(Symbol *name, Symbol **outEi);
extern TypeModifier *javaNestedNewType(Symbol *expr, Id *thenew, IdList *idl);
extern TypeModifier *javaArrayFieldAccess(Id *id);
extern TypeModifier *javaMethodInvocationN(
                                              IdList *name,
                                              S_typeModifierList *args
                                              );
extern TypeModifier *javaMethodInvocationT(	TypeModifier *tt,
                                                Id *name,
                                                S_typeModifierList *args
                                                );
extern TypeModifier *javaMethodInvocationS(	Id *super,
                                                Id *name,
                                                S_typeModifierList *args
                                                );
extern TypeModifier *javaConstructorInvocation(Symbol *class,
                                                  Position *pos,
                                                  S_typeModifierList *args
                                                  );
extern S_extRecFindStr *javaCrErfsForMethodInvocationN(IdList *name);
extern S_extRecFindStr *javaCrErfsForMethodInvocationT(TypeModifier *tt,Id *name);
extern S_extRecFindStr *javaCrErfsForMethodInvocationS(Id *super,Id *name);
extern S_extRecFindStr *javaCrErfsForConstructorInvocation(Symbol *clas, Position *pos);
extern int javaClassIsInCurrentPackage(Symbol *cl);
extern int javaFqtNamesAreFromTheSamePackage(char *classFqName, char *fqname2);
extern int javaMethodApplicability(Symbol *memb, char *actArgs);
extern Symbol *javaGetSuperClass(Symbol *cc);
extern Symbol *javaCurrentSuperClass(void);
extern TypeModifier *javaCheckNumeric(TypeModifier *tt);
extern TypeModifier *javaNumericPromotion(TypeModifier *tt);
extern TypeModifier *javaBinaryNumericPromotion(TypeModifier *t1,
                                                   TypeModifier *t2
                                                   );
extern TypeModifier *javaBitwiseLogicalPromotion(TypeModifier *t1,
                                                    TypeModifier *t2
                                                    );
extern TypeModifier *javaConditionalPromotion(TypeModifier *t1,
                                                 TypeModifier *t2
                                                 );
extern int javaIsStringType(TypeModifier *tt);
extern void javaTypeDump(TypeModifier *tt);
extern void javaAddJslReadedTopLevelClasses(JslTypeTab  *typeTab);
extern struct freeTrail * newAnonClassDefinitionBegin(Id *interfName);
extern void javaAddSuperNestedClassToSymbolTab( Symbol *cc);
extern struct freeTrail *newClassDefinitionBegin(Id *name, Access access, Symbol *anonInterf);
extern void newClassDefinitionEnd(FreeTrail *trail);
extern void javaInitArrayObject(void);
extern void javaParsedSuperClass(Symbol *s);
extern void javaSetClassSourceInformation(char *package, Id *cl);
extern void javaCheckIfPackageDirectoryIsInClassOrSourcePath(char *dir);

#endif
