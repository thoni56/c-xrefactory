#ifndef JSEMACT_H
#define JSEMACT_H

#include "jsltypetab.h"

extern void javaCheckForPrimaryStart(S_position *cpos, S_position *pp);
extern void javaCheckForPrimaryStartInNameList(S_idIdentList *name, S_position *pp);
extern void javaCheckForStaticPrefixStart(S_position *cpos, S_position *bpos);
extern void javaCheckForStaticPrefixInNameList(S_idIdentList *name, S_position *pp);
extern S_position *javaGetNameStartingPosition(S_idIdentList *name);
extern char *javaCreateComposedName(
                                    char			*prefix,
                                    S_idIdentList   *className,
                                    int             classNameSeparator,
                                    char            *name,
                                    char			*resBuff,
                                    int				resBufSize
                                    );
extern int findTopLevelName(
                            char                *name,
                            S_recFindStr        *resRfs,
                            S_symbol			**resMemb,
                            int                 classif
                            );
extern int javaClassifySingleAmbigNameToTypeOrPack(S_idIdentList *name,
                                                   S_symbol **str,
                                                   int cxrefFlag
                                                   );
extern void javaAddImportConstructionReference(S_position *importPos, S_position *pos, int usage);
extern int javaClassifyAmbiguousName(
                                     S_idIdentList *name,
                                     S_recFindStr *rfs,
                                     S_symbol **str,
                                     S_typeModifiers **expr,
                                     S_reference **oref,
                                     S_reference **rdtoref, int allowUselesFqtRefs,
                                     int classif,
                                     int usage
                                     );
extern S_reference *javaClassifyToTypeOrPackageName(S_idIdentList *tname, int usage, S_symbol **str, int allowUselesFqtRefs);
extern S_reference *javaClassifyToTypeName(S_idIdentList *tname, int usage, S_symbol **str, int allowUselesFqtRefs);
extern S_symbol * javaQualifiedThis(S_idIdentList *tname, S_idIdent *thisid);
extern void javaClassifyToPackageName( S_idIdentList *id );
extern void javaClassifyToPackageNameAndAddRefs(S_idIdentList *id, int usage);
extern char *javaImportSymbolName_st(int file, int line, int coll);
extern S_typeModifiers *javaClassifyToExpressionName(S_idIdentList *name,S_reference **oref);
extern S_symbol *javaTypeNameDefinition(S_idIdentList *tname);
extern void javaSetFieldLinkName(S_symbol *d);
extern void javaAddPackageDefinition(S_idIdentList *id);
extern S_symbol *javaAddType(S_idIdentList *clas, int accessFlag, S_position *p);
extern S_symbol *javaCreateNewMethod(char *name, S_position *pos, int mem);
extern int javaTypeToString(S_typeModifiers *type, char *pp, int ppSize);
extern int javaIsYetInTheClass(
                               S_symbol	*clas,
                               char		*lname,
                               S_symbol	**eq
                               );
extern int javaSetFunctionLinkName(S_symbol *clas, S_symbol *decl, int mem);
extern S_symbol * javaGetFieldClass(char *fieldLinkName, char **fieldAdr);
extern void javaAddNestedClassesAsTypeDefs(S_symbol *cc,
                                           S_idIdentList *oclassname, int accessFlags);
extern int javaTypeFileExist(S_idIdentList *name);
extern S_symbol *javaTypeSymbolDefinition(S_idIdentList *tname, int accessFlags,int addType);
extern S_symbol *javaTypeSymbolUsage(S_idIdentList *tname, int accessFlags);
extern void javaReadSymbolFromSourceFileEnd(void);
extern void javaReadSymbolFromSourceFileInit( int sourceFileNum,
                                              S_jslTypeTab *typeTab );
extern void javaReadSymbolsFromSourceFileNoFreeing(char *fname, char *asfname);
extern void javaReadSymbolsFromSourceFile(char *fname);
extern int javaLinkNameIsAnnonymousClass(char *linkname);
extern int javaLinkNameIsANestedClass(char *cname);
extern int isANestedClass(S_symbol *ss);
extern void addSuperMethodCxReferences(int classIndex, S_position *pos);
extern S_reference * addUselessFQTReference(int classIndex, S_position *pos);
extern S_reference *addUnimportedTypeLongReference(int classIndex, S_position *pos);
extern void addThisCxReferences(int classIndex, S_position *pos);
extern void javaLoadClassSymbolsFromFile(S_symbol *memb);
extern S_symbol *javaPrependDirectEnclosingInstanceArgument(S_symbol *args);
extern void addMethodCxReferences(unsigned modif, S_symbol *method, S_symbol *clas);
extern S_symbol *javaMethodHeader(unsigned modif, S_symbol *type, S_symbol *decl, int storage);
extern void javaAddMethodParametersToSymTable(S_symbol *method);
extern void javaMethodBodyBeginning(S_symbol *method);
extern void javaMethodBodyEnding(S_position *pos);
extern void javaAddMapedTypeName(
                                 char *file,
                                 char *path,
                                 char *pack,
                                 S_completions *c,
                                 void *vdirid,
                                 int  *storage
                                 );
extern S_symbol *javaFQTypeSymbolDefinition(char *name, char *fqName);
extern S_typeModifiers *javaClassNameType(S_idIdentList *typeName);
extern S_typeModifiers *javaNewAfterName(S_idIdentList *name, S_idIdent *id, S_idIdentList *idl);
extern int javaIsInnerAndCanGetUnnamedEnclosingInstance(S_symbol *name, S_symbol **outEi);
extern S_typeModifiers *javaNestedNewType(S_symbol *expr, S_idIdent *thenew, S_idIdentList *idl);
extern S_typeModifiers *javaArrayFieldAccess(S_idIdent *id);
extern S_typeModifiers *javaMethodInvocationN(
                                              S_idIdentList *name,
                                              S_typeModifiersList *args
                                              );
extern S_typeModifiers *javaMethodInvocationT(	S_typeModifiers *tt,
                                                S_idIdent *name,
                                                S_typeModifiersList *args
                                                );
extern S_typeModifiers *javaMethodInvocationS(	S_idIdent *super,
                                                S_idIdent *name,
                                                S_typeModifiersList *args
                                                );
extern S_typeModifiers *javaConstructorInvocation(S_symbol *class,
                                                  S_position *pos,
                                                  S_typeModifiersList *args
                                                  );
extern S_extRecFindStr *javaCrErfsForMethodInvocationN(S_idIdentList *name);
extern S_extRecFindStr *javaCrErfsForMethodInvocationT(S_typeModifiers *tt,S_idIdent *name);
extern S_extRecFindStr *javaCrErfsForMethodInvocationS(S_idIdent *super,S_idIdent *name);
extern S_extRecFindStr *javaCrErfsForConstructorInvocation(S_symbol *clas, S_position *pos);
extern int javaClassIsInCurrentPackage(S_symbol *cl);
extern int javaFqtNamesAreFromTheSamePackage(char *classFqName, char *fqname2);
extern int javaMethodApplicability(S_symbol *memb, char *actArgs);
extern S_symbol *javaGetSuperClass(S_symbol *cc);
extern S_symbol *javaCurrentSuperClass(void);
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
extern struct freeTrail * newAnonClassDefinitionBegin(S_idIdent *interfName);
extern void javaAddSuperNestedClassToSymbolTab( S_symbol *cc);
extern struct freeTrail * newClassDefinitionBegin(S_idIdent *name, int accessFlags, S_symbol *anonInterf);
extern void newClassDefinitionEnd(S_freeTrail *trail);
extern void javaInitArrayObject(void);
extern void javaParsedSuperClass(S_symbol *s);
extern void javaSetClassSourceInformation(char *package, S_idIdent *cl);
extern void javaCheckIfPackageDirectoryIsInClassOrSourcePath(char *dir);

#endif
