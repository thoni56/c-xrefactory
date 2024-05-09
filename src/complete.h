#ifndef COMPLETE_H_INCLUDED
#define COMPLETE_H_INCLUDED

#include "proto.h"              /* For ExpressionTokenType */
#include "completion.h"


typedef struct {
    int token;
    void (*fun)(Completions*);
} CompletionFunctionsTable;

extern ExpressionTokenType s_forCompletionType;

extern void fillCompletionLine(CompletionLine *cline, char *string, Symbol *symbol, Type symbolType,
                               short int virtualLevel, short int margn, char **margs,
                               Symbol *vFunClass);
extern void initCompletions(Completions *completions, int length, Position position);
extern void processName(char *name, CompletionLine *t, bool orderFlag, Completions *c);
extern void completeForSpecial1(Completions *c);
extern void completeForSpecial2(Completions *c);
extern void completeUpFunProfile(Completions *c);
extern void completeTypes(Completions *c);
extern void completeStructs(Completions *c);
extern void completeStructMemberNames(Completions *c);
extern void completeEnums(Completions *c);
extern void completeLabels(Completions *c);
extern void completeMacros(Completions *c);
extern void completeOthers(Completions *c);
extern void javaHintImportFqt(Completions *c);
extern void javaHintCompleteMethodParameters(Completions *c);
extern void completeYaccLexem(Completions *c);

extern void olCompletionListInit(Position *originalPos);
extern void printCompletionsList(int noFocus);
extern void printCompletions(Completions *c);

#endif
