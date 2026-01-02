#ifndef SEMACT_H_INCLUDED
#define SEMACT_H_INCLUDED

/**
 * @file semact.h
 * @brief Core Semantic Actions
 *
 * Category: Parsing Infrastructure - Semantic Actions (General)
 *
 * Semantic actions called by the parser for most/all operations during parsing.
 * These handle fundamental language constructs: symbol definitions, type checking,
 * scope management, parameter tracking.
 *
 * Uses parsingConfig.operation to adapt behavior:
 * - PARSER_OP_TRACK_PARAMETERS: Capture parameter positions for navigation
 * - PARSER_OP_EXTRACT: Special handling for extractable variables
 *
 * Called from: c_parser.y, yacc_parser.y (directly from grammar rules)
 * Calls: Symbol tables, reference tracking, type system
 */

#include "symbol.h"
#include "symboltable.h"
#include "usage.h"
#include "reference.h"
#include "id.h"
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
                                      Usage usage);
extern Symbol *addNewDeclaration(SymbolTable *table, Symbol *baseType, Symbol *declaration, IdList *idList,
                                 Storage storage);
extern int styyerror(char *s);
extern int styyErrorRecovery(void);
extern void setToNull(void *p);
extern Symbol *typeSpecifier1(unsigned t);
extern void declTypeSpecifier1(Symbol *d, Type type);
extern Symbol *typeSpecifier2(TypeModifier *t);
extern void declTypeSpecifier2(Symbol *d, TypeModifier *t);
extern TypeModifier *addComposedTypeToSymbol(Symbol *symbol, Type type);
extern TypeModifier *appendComposedType(TypeModifier **d, Type type);
extern void completeDeclarator(Symbol *t, Symbol *d);
extern void addFunctionParameterToSymbolTable(SymbolTable *tab, Symbol *function, Symbol *p, int i);
extern SymbolList *createDefinitionList(Symbol *symbol);
extern Symbol *createSimpleDefinition(Storage storage, Type type, Id *id);
extern int findStructureMember(Symbol *symbol, char *memberName, Symbol	**foundMemberSymbol);
extern Reference *findStuctureMemberFromSymbol(Symbol *str, Id *member, Symbol **res);
extern Reference *findStructureFieldFromType(TypeModifier *structure, Id *field, Symbol **resultingSymbol);
extern Result mergeArguments(Symbol *id, Symbol *ty);
extern TypeModifier *simpleStructOrUnionSpecifier(Id *typeName, Id *id, Usage usage);
extern TypeModifier *createNewAnonymousStructOrUnion(Id *typeName);
extern void specializeStructOrUnionDef(Symbol *sd, Symbol *rec);
extern TypeModifier *simpleEnumSpecifier(Id *id, Usage usage);
extern void setGlobalFileDepNames(char *iname, Symbol *symbol, MemoryClass memory);
extern TypeModifier *createNewAnonymousEnum(SymbolList *enums);
extern void appendPositionToList(PositionList **list, Position position);
extern void setParamPositionForFunctionWithoutParams(Position lpar);
extern void setParamPositionForParameterBeyondRange(Position rpar);
extern Symbol *createEmptyField(void);
extern void handleDeclaratorParamPositions(Symbol *decl, Position lpar,
                                           PositionList *commas, Position rpar,
                                           bool hasParam, bool isVoid);
extern void handleInvocationParameterPositions(Reference *ref, Position lpar,
                                           PositionList *commas, Position rpar,
                                           bool hasParam);
extern void setLocalVariableLinkName(struct symbol *p);
extern void labelReference(Id *id, Usage usage);
extern void generateInternalLabelReference(int counter, int usage);
extern void setDirectStructureCompletionType(TypeModifier *xxx);
extern void setIndirectStructureCompletionType(TypeModifier *xxx);

#endif
