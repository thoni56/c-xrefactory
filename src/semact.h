#ifndef SEMACT_H_INCLUDED
#define SEMACT_H_INCLUDED

#include "symbol.h"
#include "symboltable.h"
#include "usage.h"

#include "proto.h"


extern void initSymStructSpec(S_symStructSpec *symStruct, Symbol *records);
extern void fillRecFindStr(S_recFindStr *recFindStr, Symbol *baseClass, Symbol *currentClass, Symbol *nextRecord, unsigned recsClassCounter);
extern void unpackPointers(Symbol *pp);
extern bool displayingErrorMessages(void);
extern void noSuchFieldError(char *rec);
extern void deleteSymDef(void *p);
extern void addSymbol(SymbolTable *tab, Symbol *pp);
extern void recFindPush(Symbol *sym, S_recFindStr *rfs);
extern S_recFindStr * iniFind(Symbol *s, S_recFindStr *rfs);
extern bool javaOuterClassAccessible(Symbol *cl);
extern bool javaRecordAccessible(S_recFindStr *rfs, Symbol *applcl, Symbol *funcl, Symbol *rec, unsigned recAccessFlags);
extern bool javaRecordVisibleAndAccessible(S_recFindStr *rfs, Symbol *applCl, Symbol *funCl, Symbol *r);
extern int javaGetMinimalAccessibility(S_recFindStr *rfs, Symbol *r);
extern Result findStrRecordSym(S_recFindStr *ss,
                            char *recname,
                            Symbol **res,
                            int javaClassif,
                            AccessibilityCheckYesNo accessCheck,
                            VisibilityCheckYesNo visibilityCheck);
extern Symbol *addNewSymbolDefinition(SymbolTable *table, Symbol *symbol, Storage storage, UsageKind usage);
extern Symbol *addNewDeclaration(SymbolTable *table, Symbol *baseType, Symbol *declaration, IdList *idList,
                                 unsigned storage);
extern int styyerror(char *s);
extern int styyErrorRecovery(void);
extern void setToNull(void *p);
extern Symbol *typeSpecifier1(unsigned t);
extern void declTypeSpecifier1(Symbol *d, unsigned t);
extern Symbol *typeSpecifier2(TypeModifier *t);
extern void declTypeSpecifier2(Symbol *d, TypeModifier *t);
extern TypeModifier *addComposedTypeToSymbol(Symbol *symbol, Type type);
extern TypeModifier *appendComposedType(TypeModifier **d, unsigned t);
extern TypeModifier *prependComposedType(TypeModifier *d, unsigned t);
extern void completeDeclarator(Symbol *t, Symbol *d);
extern void addFunctionParameterToSymTable(SymbolTable *tab, Symbol *function, Symbol *p, int i);
extern SymbolList *createDefinitionList(Symbol *symbol);
extern Symbol *createSimpleDefinition(unsigned storage, unsigned t, Id *id);
extern int findStrRecord(Symbol	*s,
                         char   *recname,	/* can be NULL */
                         Symbol	**res,
                         int    javaClassif);
extern Reference *findStrRecordFromSymbol(Symbol *str,
                                          Id *record,
                                          Symbol **res,
                                          int javaClassif,
                                          Id *super);
extern Reference *findStructureFieldFromType(TypeModifier *structure,
                                             Id *field,
                                             Symbol **resultingSymbol,
                                             int javaClassifier);
extern Result mergeArguments(Symbol *id, Symbol *ty);
extern TypeModifier *simpleStrUnionSpecifier(Id *typeName,
                                             Id *id,
                                             UsageKind usage);
extern TypeModifier *createNewAnonymousStructOrUnion(Id *typeName);
extern void specializeStrUnionDef(Symbol *sd, Symbol *rec);
extern TypeModifier *simpleEnumSpecifier(Id *id, UsageKind usage);
extern void setGlobalFileDepNames(char *iname, Symbol *symbol, int memory);
extern TypeModifier *createNewAnonymousEnum(SymbolList *enums);
extern void appendPositionToList(PositionList **list, Position *pos);
extern void setParamPositionForFunctionWithoutParams(Position *lpar);
extern void setParamPositionForParameter0(Position *lpar);
extern void setParamPositionForParameterBeyondRange(Position *rpar);
extern Symbol *createEmptyField(void);
extern void handleDeclaratorParamPositions(Symbol *decl, Position *lpar,
                                           PositionList *commas, Position *rpar,
                                           bool hasParam, bool isVoid);
extern void handleInvocationParamPositions(Reference *ref, Position *lpar,
                                           PositionList *commas, Position *rpar,
                                           bool hasParam);
extern void javaHandleDeclaratorParamPositions(Position *sym, Position *lpar,
                                               PositionList *commas, Position *rpar);
extern void setLocalVariableLinkName(struct symbol *p);
extern void labelReference(Id *id, UsageKind usage);
extern void setDirectStructureCompletionType(TypeModifier *xxx);
extern void setIndirectStructureCompletionType(TypeModifier *xxx);

#endif
