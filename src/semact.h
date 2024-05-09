#ifndef SEMACT_H_INCLUDED
#define SEMACT_H_INCLUDED

#include "symbol.h"
#include "symboltable.h"
#include "usage.h"

#include "proto.h"


extern void initSymStructSpec(StructSpec *symStruct, Symbol *records);
extern void fillStructMemberFindInfo(StructMemberFindInfo *info, Symbol *currentStructure,
                                     Symbol *nextRecord, unsigned memberFindCount);
extern void unpackPointers(Symbol *pp);
extern bool displayingErrorMessages(void);
extern void noSuchMemberError(char *memberName);
extern void deleteSymDef(void *p);
extern void addSymbolToFrame(SymbolTable *tab, Symbol *pp);
extern void memberFindPush(Symbol *sym, StructMemberFindInfo *rfs);
extern StructMemberFindInfo * initFind(Symbol *s, StructMemberFindInfo *rfs);
extern Result findStructureMemberSymbol(Symbol **res, StructMemberFindInfo *ss, char *recname);
extern Symbol *addNewSymbolDefinition(SymbolTable *table, char *fileName, Symbol *symbol, Storage storage,
                                      UsageKind usage);
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
extern void completeDeclarator(Symbol *t, Symbol *d);
extern void addFunctionParameterToSymTable(SymbolTable *tab, Symbol *function, Symbol *p, int i);
extern SymbolList *createDefinitionList(Symbol *symbol);
extern Symbol *createSimpleDefinition(unsigned storage, unsigned t, Id *id);
extern int findStructureMember(Symbol *symbol, char *memberName, Symbol	**foundMemberSymbol);
extern Reference *findStuctureMemberFromSymbol(Symbol *str, Id *member, Symbol **res);
extern Reference *findStructureFieldFromType(TypeModifier *structure, Id *field, Symbol **resultingSymbol);
extern Result mergeArguments(Symbol *id, Symbol *ty);
extern TypeModifier *simpleStrUnionSpecifier(Id *typeName, Id *id, UsageKind usage);
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
extern void setLocalVariableLinkName(struct symbol *p);
extern void labelReference(Id *id, UsageKind usage);
extern void generateInternalLabelReference(int counter, int usage);
extern void setDirectStructureCompletionType(TypeModifier *xxx);
extern void setIndirectStructureCompletionType(TypeModifier *xxx);

#endif
