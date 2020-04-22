#ifndef JSEMACT_H
#define JSEMACT_H

#include "jsltypetab.h"
#include "symbol.h"
#include "symboltable.h"


typedef struct javaStat {
    struct idList               *className;			/* this class name */
    struct typeModifier		*thisType;			/* this class type */
    struct symbol				*thisClass;			/* this class definition */
    int							currentNestedIndex;	/* currently parsed nested class */
    char						*currentPackage;    /* current package */
    char						*unnamedPackagePath;	/* directory for unnamed package */
    char						*namedPackagePath;	/* inferred source-path for named package */
    struct symbolTable			*locals;			/* args and local variables */
    struct idList               *lastParsedName;
    unsigned					methodModifiers;		/* currently parsed method modifs */
    struct currentlyParsedCl	cp;					/* some parsing positions */
    int							classFileIndex;		/* this file class index */
    struct javaStat				*next;				/* outer class */
} S_javaStat;


extern S_javaStat *s_javaStat;
extern S_javaStat s_initJavaStat;


extern void fill_nestedSpec(S_nestedSpec *nestedSpec, struct symbol *cl,
                            char membFlag, short unsigned  accFlags);
extern void fillJavaStat(S_javaStat *javaStat, S_idList *className, S_typeModifier *thisType, Symbol *thisClass,
                         int currentNestedIndex, char *currentPackage, char *unnamedPackageDir,
                         char *namedPackageDir, S_symbolTable *locals, S_idList *lastParsedName,
                         unsigned methodModifiers, S_currentlyParsedCl parsingPositions, int classFileIndex,
                         S_javaStat *next);
extern void javaCheckForPrimaryStart(S_position *cpos, S_position *pp);
extern void javaCheckForPrimaryStartInNameList(S_idList *name, S_position *pp);
extern void javaCheckForStaticPrefixStart(S_position *cpos, S_position *bpos);
extern void javaCheckForStaticPrefixInNameList(S_idList *name, S_position *pp);
extern S_position *javaGetNameStartingPosition(S_idList *name);
extern char *javaCreateComposedName(
                                    char			*prefix,
                                    S_idList   *className,
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
extern int javaClassifySingleAmbigNameToTypeOrPack(S_idList *name,
                                                   Symbol **str,
                                                   int cxrefFlag
                                                   );
extern void javaAddImportConstructionReference(S_position *importPos, S_position *pos, int usage);
extern int javaClassifyAmbiguousName(
                                     S_idList *name,
                                     S_recFindStr *rfs,
                                     Symbol **str,
                                     S_typeModifier **expr,
                                     S_reference **oref,
                                     S_reference **rdtoref, int allowUselesFqtRefs,
                                     int classif,
                                     int usage
                                     );
extern S_reference *javaClassifyToTypeOrPackageName(S_idList *tname, int usage, Symbol **str, int allowUselesFqtRefs);
extern S_reference *javaClassifyToTypeName(S_idList *tname, int usage, Symbol **str, int allowUselesFqtRefs);
extern Symbol * javaQualifiedThis(S_idList *tname, S_id *thisid);
extern void javaClassifyToPackageName( S_idList *id );
extern void javaClassifyToPackageNameAndAddRefs(S_idList *id, int usage);
extern char *javaImportSymbolName_st(int file, int line, int coll);
extern S_typeModifier *javaClassifyToExpressionName(S_idList *name,S_reference **oref);
extern Symbol *javaTypeNameDefinition(S_idList *tname);
extern void javaSetFieldLinkName(Symbol *d);
extern void javaAddPackageDefinition(S_idList *id);
extern Symbol *javaAddType(S_idList *class, Access access, S_position *p);
extern Symbol *javaCreateNewMethod(char *name, S_position *pos, int mem);
extern int javaTypeToString(S_typeModifier *type, char *pp, int ppSize);
extern int javaIsYetInTheClass(
                               Symbol	*clas,
                               char		*lname,
                               Symbol	**eq
                               );
extern int javaSetFunctionLinkName(Symbol *clas, Symbol *decl, int mem);
extern Symbol * javaGetFieldClass(char *fieldLinkName, char **fieldAdr);
extern void javaAddNestedClassesAsTypeDefs(Symbol *cc,
                                           S_idList *oclassname, int accessFlags);
extern bool javaTypeFileExist(S_idList *name);
extern Symbol *javaTypeSymbolDefinition(S_idList *tname, int accessFlags,int addType);
extern Symbol *javaTypeSymbolUsage(S_idList *tname, int accessFlags);
extern void javaReadSymbolFromSourceFileEnd(void);
extern void javaReadSymbolFromSourceFileInit( int sourceFileNum,
                                              S_jslTypeTab *typeTab );
extern void javaReadSymbolsFromSourceFileNoFreeing(char *fname, char *asfname);
extern void javaReadSymbolsFromSourceFile(char *fname);
extern int javaLinkNameIsAnnonymousClass(char *linkname);
extern int javaLinkNameIsANestedClass(char *cname);
extern int isANestedClass(Symbol *ss);
extern void addSuperMethodCxReferences(int classIndex, S_position *pos);
extern S_reference * addUselessFQTReference(int classIndex, S_position *pos);
extern S_reference *addUnimportedTypeLongReference(int classIndex, S_position *pos);
extern void addThisCxReferences(int classIndex, S_position *pos);
extern void javaLoadClassSymbolsFromFile(Symbol *memb);
extern Symbol *javaPrependDirectEnclosingInstanceArgument(Symbol *args);
extern void addMethodCxReferences(unsigned modif, Symbol *method, Symbol *clas);
extern Symbol *javaMethodHeader(unsigned modif, Symbol *type, Symbol *decl, int storage);
extern void javaAddMethodParametersToSymTable(Symbol *method);
extern void javaMethodBodyBeginning(Symbol *method);
extern void javaMethodBodyEnding(S_position *pos);
extern void javaAddMapedTypeName(
                                 char *file,
                                 char *path,
                                 char *pack,
                                 Completions *c,
                                 void *vdirid,
                                 int  *storage
                                 );
extern Symbol *javaFQTypeSymbolDefinition(char *name, char *fqName);
extern S_typeModifier *javaClassNameType(S_idList *typeName);
extern S_typeModifier *javaNewAfterName(S_idList *name, S_id *id, S_idList *idl);
extern int javaIsInnerAndCanGetUnnamedEnclosingInstance(Symbol *name, Symbol **outEi);
extern S_typeModifier *javaNestedNewType(Symbol *expr, S_id *thenew, S_idList *idl);
extern S_typeModifier *javaArrayFieldAccess(S_id *id);
extern S_typeModifier *javaMethodInvocationN(
                                              S_idList *name,
                                              S_typeModifierList *args
                                              );
extern S_typeModifier *javaMethodInvocationT(	S_typeModifier *tt,
                                                S_id *name,
                                                S_typeModifierList *args
                                                );
extern S_typeModifier *javaMethodInvocationS(	S_id *super,
                                                S_id *name,
                                                S_typeModifierList *args
                                                );
extern S_typeModifier *javaConstructorInvocation(Symbol *class,
                                                  S_position *pos,
                                                  S_typeModifierList *args
                                                  );
extern S_extRecFindStr *javaCrErfsForMethodInvocationN(S_idList *name);
extern S_extRecFindStr *javaCrErfsForMethodInvocationT(S_typeModifier *tt,S_id *name);
extern S_extRecFindStr *javaCrErfsForMethodInvocationS(S_id *super,S_id *name);
extern S_extRecFindStr *javaCrErfsForConstructorInvocation(Symbol *clas, S_position *pos);
extern int javaClassIsInCurrentPackage(Symbol *cl);
extern int javaFqtNamesAreFromTheSamePackage(char *classFqName, char *fqname2);
extern int javaMethodApplicability(Symbol *memb, char *actArgs);
extern Symbol *javaGetSuperClass(Symbol *cc);
extern Symbol *javaCurrentSuperClass(void);
extern S_typeModifier *javaCheckNumeric(S_typeModifier *tt);
extern S_typeModifier *javaNumericPromotion(S_typeModifier *tt);
extern S_typeModifier *javaBinaryNumericPromotion(S_typeModifier *t1,
                                                   S_typeModifier *t2
                                                   );
extern S_typeModifier *javaBitwiseLogicalPromotion(S_typeModifier *t1,
                                                    S_typeModifier *t2
                                                    );
extern S_typeModifier *javaConditionalPromotion(S_typeModifier *t1,
                                                 S_typeModifier *t2
                                                 );
extern int javaIsStringType(S_typeModifier *tt);
extern void javaTypeDump(S_typeModifier *tt);
extern void javaAddJslReadedTopLevelClasses(S_jslTypeTab  *typeTab);
extern struct freeTrail * newAnonClassDefinitionBegin(S_id *interfName);
extern void javaAddSuperNestedClassToSymbolTab( Symbol *cc);
extern struct freeTrail *newClassDefinitionBegin(S_id *name, Access access, Symbol *anonInterf);
extern void newClassDefinitionEnd(S_freeTrail *trail);
extern void javaInitArrayObject(void);
extern void javaParsedSuperClass(Symbol *s);
extern void javaSetClassSourceInformation(char *package, S_id *cl);
extern void javaCheckIfPackageDirectoryIsInClassOrSourcePath(char *dir);

#endif
