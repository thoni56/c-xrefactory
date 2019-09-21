#ifndef SEMACT_H
#define SEMACT_H

#include "proto.h"

extern void unpackPointers(S_symbol *pp);
extern int displayingErrorMessages(void);
extern void deleteSymDef(void *p);
extern void addSymbol(S_symbol *pp, S_symTab *tab);
extern void recFindPush(S_symbol *sym, S_recFindStr *rfs);
extern S_recFindStr * iniFind(S_symbol *s, S_recFindStr *rfs);
extern int javaOuterClassAccessible(S_symbol *cl);
extern int javaRecordAccessible(S_recFindStr *rfs, S_symbol *applcl, S_symbol *funcl, S_symbol *rec, unsigned recAccessFlags);
extern int javaRecordVisibleAndAccessible(S_recFindStr *rfs, S_symbol *applCl, S_symbol *funCl, S_symbol *r);
extern int javaGetMinimalAccessibility(S_recFindStr *rfs, S_symbol *r);
extern int findStrRecordSym(	S_recFindStr *ss,
                                char *recname,
                                S_symbol **res,
                                int javaClassif,
                                int accessCheck,
                                int visibilityCheck
                                );
extern S_symbol *addNewSymbolDef(S_symbol *p, unsigned storage, S_symTab *tab, int usage);
extern S_symbol *addNewCopyOfSymbolDef(S_symbol *def, unsigned defaultStorage);
extern S_symbol *addNewDeclaration(S_symbol *btype, S_symbol *decl, S_idIdentList *idl,
                                   unsigned storage, S_symTab *tab);
extern int styyerror(char *s);
extern int styyErrorRecovery(void);
extern void setToNull(void *p);
extern void allocNewCurrentDefinition(void);
extern S_symbol *typeSpecifier1(unsigned t);
extern void declTypeSpecifier1(S_symbol *d, unsigned t);
extern S_symbol *typeSpecifier2(S_typeModifiers *t);
extern void declTypeSpecifier2(S_symbol *d, S_typeModifiers *t);
extern void declTypeSpecifier21(S_typeModifiers *t, S_symbol *d);
extern S_typeModifiers *appendComposedType(S_typeModifiers **d, unsigned t);
extern S_typeModifiers *prependComposedType(S_typeModifiers *d, unsigned t);
extern void completeDeclarator(S_symbol *t, S_symbol *d);
extern void addFunctionParameterToSymTable(S_symbol *function, S_symbol *p, int i, S_symTab *tab);
extern S_typeModifiers *crSimpleTypeModifier (unsigned t);
extern SymbolList *createDefinitionList(S_symbol *symbol);
extern S_symbol *createSimpleDefinition(unsigned storage, unsigned t, S_idIdent *id);
extern int findStrRecord(S_symbol	*s,
                         char		*recname,	/* can be NULL */
                         S_symbol	**res,
                         int		javaClassif
                         );
extern S_reference * findStrRecordFromSymbol(S_symbol *str,
                                             S_idIdent *record,
                                             S_symbol **res,
                                             int javaClassif,
                                             S_idIdent *super
                                             );
extern S_reference * findStrRecordFromType(S_typeModifiers *str,
                                           S_idIdent *record,
                                           S_symbol **res,
                                           int javaClassif
                                           );
extern int mergeArguments(S_symbol *id, S_symbol *ty);
extern S_typeModifiers *simpleStrUnionSpecifier(S_idIdent *typeName,
                                                S_idIdent *id,
                                                int usage
                                                );
extern S_typeModifiers *crNewAnnonymeStrUnion(S_idIdent *typeName);
extern void specializeStrUnionDef(S_symbol *sd, S_symbol *rec);
extern S_typeModifiers *simpleEnumSpecifier(S_idIdent *id, int usage);
extern void setGlobalFileDepNames(char *iname, S_symbol *pp, int memory);
extern S_typeModifiers *createNewAnonymousEnum(SymbolList *enums);
extern void appendPositionToList(S_positionList **list, S_position *pos);
extern void setParamPositionForFunctionWithoutParams(S_position *lpar);
extern void setParamPositionForParameter0(S_position *lpar);
extern void setParamPositionForParameterBeyondRange(S_position *rpar);
extern S_symbol *crEmptyField(void);
extern void handleDeclaratorParamPositions(S_symbol *decl, S_position *lpar,
                                           S_positionList *commas, S_position *rpar,
                                           int hasParam);
extern void handleInvocationParamPositions(S_reference *ref, S_position *lpar,
                                           S_positionList *commas, S_position *rpar,
                                           int hasParam);
extern void javaHandleDeclaratorParamPositions(S_position *sym, S_position *lpar,
                                               S_positionList *commas, S_position *rpar);
extern void setLocalVariableLinkName(struct symbol *p);
extern void labelReference(S_idIdent *id, int usage);

#endif
