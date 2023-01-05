#ifndef COMPLETION_H_INCLUDED
#define COMPLETION_H_INCLUDED

#include "reference.h"
#include "symbol.h"
#include "type.h"

typedef struct completion {
    char                 *name;
    char                 *fullName;
    char                 *vclass;
    short int             jindent;
    short int             lineCount;
    char                  category; /* Global/Local TODO: enum!*/
    Type                  csymType; /* symtype of completion */
    struct reference      ref;
    struct referencesItem sym;
    struct completion  *next;
} Completion;


extern Completion *newCompletion(char *name, char *fullName, char *vclass, short int jindent,
                                 short int lineCount, char category, char csymType,
                                 struct reference ref, struct referencesItem sym);
extern void olcxFreeCompletion(Completion *completion);
extern void olcxFreeCompletions(Completion *completions);

extern Completion  *completionListPrepend(Completion *completions, char *name, char *fullText,
                                          char *vclass, int jindent, Symbol *s, ReferencesItem *ri,
                                          Reference *dfpos, int symType, int vFunClass);

void tagSearchCompactShortResults(void);

#endif
