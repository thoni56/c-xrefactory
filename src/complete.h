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
extern void collectForStatementCompletions1(Completions *c);
extern void collectForStatementCompletions2(Completions *c);
extern void completeUpFunProfile(Completions *c);
extern void collectTypesCompletions(Completions *c);
extern void collectStructsCompletions(Completions *c);
extern void collectStructMemberCompletions(Completions *c);
extern void collectEnumsCompletions(Completions *c);
extern void collectLabelsCompletions(Completions *c);
extern void completeMacros(Completions *c);
extern void collectOthersCompletions(Completions *c);
extern void collectYaccLexemCompletions(Completions *c);

extern void olCompletionListInit(Position *originalPos);
extern void printCompletionsList(int noFocus);
extern void printCompletions(Completions *c);

#endif
