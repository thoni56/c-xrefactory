#ifndef JSEMACT_H
#define JSEMACT_H

#include "jsltypetab.h"
#include "symbol.h"


extern S_javaStat *s_javaStat;
extern S_javaStat s_initJavaStat;


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
                                     S_typeModifiers **expr,
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
extern S_typeModifiers *javaClassifyToExpressionName(S_idList *name,S_reference **oref);
extern Symbol *javaTypeNameDefinition(S_idList *tname);
extern void javaSetFieldLinkName(Symbol *d);
extern void javaAddPackageDefinition(S_idList *id);
extern Symbol *javaAddType(S_idList *clas, int accessFlag, S_position *p);
extern Symbol *javaCreateNewMethod(char *name, S_position *pos, int mem);
extern int javaTypeToString(S_typeModifiers *type, char *pp, int ppSize);
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
                                 S_completions *c,
                                 void *vdirid,
                                 int  *storage
                                 );
extern Symbol *javaFQTypeSymbolDefinition(char *name, char *fqName);
extern S_typeModifiers *javaClassNameType(S_idList *typeName);
extern S_typeModifiers *javaNewAfterName(S_idList *name, S_id *id, S_idList *idl);
extern int javaIsInnerAndCanGetUnnamedEnclosingInstance(Symbol *name, Symbol **outEi);
extern S_typeModifiers *javaNestedNewType(Symbol *expr, S_id *thenew, S_idList *idl);
extern S_typeModifiers *javaArrayFieldAccess(S_id *id);
extern S_typeModifiers *javaMethodInvocationN(
                                              S_idList *name,
                                              S_typeModifiersList *args
                                              );
extern S_typeModifiers *javaMethodInvocationT(	S_typeModifiers *tt,
                                                S_id *name,
                                                S_typeModifiersList *args
                                                );
extern S_typeModifiers *javaMethodInvocationS(	S_id *super,
                                                S_id *name,
                                                S_typeModifiersList *args
                                                );
extern S_typeModifiers *javaConstructorInvocation(Symbol *class,
                                                  S_position *pos,
                                                  S_typeModifiersList *args
                                                  );
extern S_extRecFindStr *javaCrErfsForMethodInvocationN(S_idList *name);
extern S_extRecFindStr *javaCrErfsForMethodInvocationT(S_typeModifiers *tt,S_id *name);
extern S_extRecFindStr *javaCrErfsForMethodInvocationS(S_id *super,S_id *name);
extern S_extRecFindStr *javaCrErfsForConstructorInvocation(Symbol *clas, S_position *pos);
extern int javaClassIsInCurrentPackage(Symbol *cl);
extern int javaFqtNamesAreFromTheSamePackage(char *classFqName, char *fqname2);
extern int javaMethodApplicability(Symbol *memb, char *actArgs);
extern Symbol *javaGetSuperClass(Symbol *cc);
extern Symbol *javaCurrentSuperClass(void);
extern S_typeModifiers *javaCheckNumeric(S_typeModifiers *tt);
extern S_typeModifiers *javaNumericPromotion(S_typeModifiers *tt);
extern S_typeModifiers *javaBinaryNumericPromotion(S_typeModifiers *t1,
                                                   S_typeModifiers *t2
                                                   );
extern S_typeModifiers *javaBitwiseLogicalPromotion(S_typeModifiers *t1,
                                                    S_typeModifiers *t2
                                                    );
extern S_typeModifiers *javaConditionalPromotion(S_typeModifiers *t1,
                                                 S_typeModifiers *t2
                                                 );
extern int javaIsStringType(S_typeModifiers *tt);
extern void javaTypeDump(S_typeModifiers *tt);
extern void javaAddJslReadedTopLevelClasses(S_jslTypeTab  *typeTab);
extern struct freeTrail * newAnonClassDefinitionBegin(S_id *interfName);
extern void javaAddSuperNestedClassToSymbolTab( Symbol *cc);
extern struct freeTrail * newClassDefinitionBegin(S_id *name, int accessFlags, Symbol *anonInterf);
extern void newClassDefinitionEnd(S_freeTrail *trail);
extern void javaInitArrayObject(void);
extern void javaParsedSuperClass(Symbol *s);
extern void javaSetClassSourceInformation(char *package, S_id *cl);
extern void javaCheckIfPackageDirectoryIsInClassOrSourcePath(char *dir);

#endif
