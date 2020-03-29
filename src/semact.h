#ifndef SEMACT_H
#define SEMACT_H

#include "symbol.h"
#include "symboltable.h"

#include "proto.h"              /* Requires at least S_recFindStr, probably others */


extern void unpackPointers(Symbol *pp);
extern int displayingErrorMessages(void);
extern void deleteSymDef(void *p);
extern void addSymbol(Symbol *pp, S_symbolTable *tab);
extern void recFindPush(Symbol *sym, S_recFindStr *rfs);
extern S_recFindStr * iniFind(Symbol *s, S_recFindStr *rfs);
extern int javaOuterClassAccessible(Symbol *cl);
extern int javaRecordAccessible(S_recFindStr *rfs, Symbol *applcl, Symbol *funcl, Symbol *rec, unsigned recAccessFlags);
extern int javaRecordVisibleAndAccessible(S_recFindStr *rfs, Symbol *applCl, Symbol *funCl, Symbol *r);
extern int javaGetMinimalAccessibility(S_recFindStr *rfs, Symbol *r);
extern int findStrRecordSym(	S_recFindStr *ss,
                                char *recname,
                                Symbol **res,
                                int javaClassif,
                                int accessCheck,
                                int visibilityCheck
                                );
extern Symbol *addNewSymbolDef(Symbol *p, unsigned storage, S_symbolTable *tab, int usage);
extern Symbol *addNewCopyOfSymbolDef(Symbol *def, unsigned defaultStorage);
extern Symbol *addNewDeclaration(Symbol *btype, Symbol *decl, S_idList *idl,
                                   unsigned storage, S_symbolTable *tab);
extern int styyerror(char *s);
extern int styyErrorRecovery(void);
extern void setToNull(void *p);
extern Symbol *typeSpecifier1(unsigned t);
extern void declTypeSpecifier1(Symbol *d, unsigned t);
extern Symbol *typeSpecifier2(S_typeModifiers *t);
extern void declTypeSpecifier2(Symbol *d, S_typeModifiers *t);
extern void declTypeSpecifier21(S_typeModifiers *t, Symbol *d);
extern S_typeModifiers *appendComposedType(S_typeModifiers **d, unsigned t);
extern S_typeModifiers *prependComposedType(S_typeModifiers *d, unsigned t);
extern void completeDeclarator(Symbol *t, Symbol *d);
extern void addFunctionParameterToSymTable(Symbol *function, Symbol *p, int i, S_symbolTable *tab);
extern S_typeModifiers *crSimpleTypeModifier (unsigned t);
extern SymbolList *createDefinitionList(Symbol *symbol);
extern Symbol *createSimpleDefinition(unsigned storage, unsigned t, S_id *id);
extern int findStrRecord(Symbol	*s,
                         char		*recname,	/* can be NULL */
                         Symbol	**res,
                         int		javaClassif
                         );
extern S_reference * findStrRecordFromSymbol(Symbol *str,
                                             S_id *record,
                                             Symbol **res,
                                             int javaClassif,
                                             S_id *super
                                             );
extern S_reference * findStrRecordFromType(S_typeModifiers *str,
                                           S_id *record,
                                           Symbol **res,
                                           int javaClassif
                                           );
extern int mergeArguments(Symbol *id, Symbol *ty);
extern S_typeModifiers *simpleStrUnionSpecifier(S_id *typeName,
                                                S_id *id,
                                                int usage
                                                );
extern S_typeModifiers *crNewAnnonymeStrUnion(S_id *typeName);
extern void specializeStrUnionDef(Symbol *sd, Symbol *rec);
extern S_typeModifiers *simpleEnumSpecifier(S_id *id, int usage);
extern void setGlobalFileDepNames(char *iname, Symbol *pp, int memory);
extern S_typeModifiers *createNewAnonymousEnum(SymbolList *enums);
extern void appendPositionToList(S_positionList **list, S_position *pos);
extern void setParamPositionForFunctionWithoutParams(S_position *lpar);
extern void setParamPositionForParameter0(S_position *lpar);
extern void setParamPositionForParameterBeyondRange(S_position *rpar);
extern Symbol *crEmptyField(void);
extern void handleDeclaratorParamPositions(Symbol *decl, S_position *lpar,
                                           S_positionList *commas, S_position *rpar,
                                           int hasParam);
extern void handleInvocationParamPositions(S_reference *ref, S_position *lpar,
                                           S_positionList *commas, S_position *rpar,
                                           int hasParam);
extern void javaHandleDeclaratorParamPositions(S_position *sym, S_position *lpar,
                                               S_positionList *commas, S_position *rpar);
extern void setLocalVariableLinkName(struct symbol *p);
extern void labelReference(S_id *id, int usage);

#endif
