#ifndef COMPLETE_H_INCLUDED
#define COMPLETE_H_INCLUDED

#include "proto.h"              /* For ExpressionTokenType */
#include "completion.h"


typedef struct {
    int token;
    void (*fun)(Completions*);
} CompletionFunctionsTable;

extern ExpressionTokenType completionTypeForForStatement;

extern void initCompletions(Completions *completions, int length, Position position);
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

extern CompletionLine makeCompletionLine(char *string, Symbol *symbol, Type symbolType, short int margn,
                                         char **margs);
extern void processName(char *name, CompletionLine *line, bool orderFlag, Completions *completions);

extern void printCompletionsList(bool noFocus);
extern void printCompletions(Completions *c);

#endif
