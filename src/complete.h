#ifndef COMPLETE_H_INCLUDED
#define COMPLETE_H_INCLUDED

#include "proto.h"

typedef struct completionFunctionsTable {
    int token;
    void (*fun)(Completions*);
} CompletionFunctionsTable;


extern void fillCompletionLine(CompletionLine *cline, char *string, Symbol *symbol, Type symbolType,
                               short int virtualLevel, short int margn, char **margs,
                               Symbol *vFunClass);
extern void initCompletions(Completions *completions, int length, Position position);
extern void processName(char *name, CompletionLine *t, int orderFlag, Completions *c);
extern void completeForSpecial1(Completions *c);
extern void completeForSpecial2(Completions *c);
extern void completeUpFunProfile(Completions *c);
extern void completeTypes(Completions *c);
extern void completeStructs(Completions *c);
extern void completeRecNames(Completions *c);
extern void completeEnums(Completions *c);
extern void completeLabels(Completions *c);
extern void completeMacros(Completions *c);
extern void completeOthers(Completions *c);
extern void javaCompleteTypeSingleName(Completions *c);
extern void javaHintImportFqt(Completions *c);
extern void javaHintVariableName(Completions *c);
extern void javaHintCompleteNonImportedTypes(Completions *c);
extern void javaHintCompleteMethodParameters(Completions *c);
extern void javaCompleteTypeCompName(Completions *c);
extern void javaCompleteThisPackageName(Completions *c);
extern void javaCompletePackageSingleName(Completions *c);
extern void javaCompleteExprSingleName(Completions *c);
extern void javaCompleteUpMethodSingleName(Completions *c);
extern void javaCompleteFullInheritedMethodHeader(Completions *c);
extern void javaCompletePackageCompName(Completions *c);
extern void javaCompleteExprCompName(Completions *c);
extern void javaCompleteMethodCompName(Completions *c);
extern void javaCompleteHintForConstructSingleName(Completions *c);
extern void javaCompleteConstructSingleName(Completions *c);
extern void javaCompleteConstructCompName(Completions *c);
extern void javaCompleteConstructNestNameName(Completions *c);
extern void javaCompleteConstructNestPrimName(Completions *c);
extern void javaCompleteStrRecordPrimary(Completions *c);
extern void javaCompleteStrRecordSuper(Completions *c);
extern void javaCompleteStrRecordQualifiedSuper(Completions *c);
extern void javaCompleteClassDefinitionNameSpecial(Completions *c);
extern void javaCompleteClassDefinitionName(Completions *c);
extern void javaCompleteThisConstructor(Completions *c);
extern void javaCompleteSuperConstructor(Completions *c);
extern void javaCompleteSuperNestedConstructor(Completions *c);
extern void completeYaccLexem(Completions *c);

extern void olCompletionListInit(Position *originalPos);
extern void printCompletionsList(int noFocus);
extern void printCompletions(Completions *c);

#endif
