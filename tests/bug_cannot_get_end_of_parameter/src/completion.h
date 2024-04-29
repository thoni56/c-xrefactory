#ifndef COMPLETION_H_INCLUDED
#define COMPLETION_H_INCLUDED

#include "reference.h"
#include "symbol.h"
#include "type.h"
#include "constants.h"


/* ***************** COMPLETION STRUCTURES ********************** */

typedef struct completion {
    char                 *name;
    char                 *fullName;
    char                 *vclass;
    short int             jindent;
    short int             lineCount;
    char                  category; /* Global/Local TODO: enum!*/
    Type                  csymType; /* symtype of completion */
    struct reference      ref;
    struct referenceItem sym;
    struct completion  *next;
} Completion;

typedef struct completionLine {
    char          *string;
    struct symbol *symbol;
    Type           symbolType;
    short int      virtLevel;
    short int      margn;
    char         **margs;
    struct symbol *vFunClass;
} CompletionLine;

typedef struct completions {
    char                  idToProcess[MAX_FUN_NAME_SIZE];
    int                   idToProcessLen;
    struct position       idToProcessPos;
    bool                  fullMatchFlag;
    bool                  isCompleteFlag;
    bool                  noFocusOnCompletions;
    bool                  abortFurtherCompletions;
    char                  prefix[TMP_STRING_SIZE];
    int                   maxLen;
    struct completionLine alternatives[MAX_COMPLETIONS];
    int                   alternativeIndex;
} Completions;

extern Completion *newCompletion(char *name, char *fullName, char *vclass, short int jindent,
                                 short int lineCount, char category, char csymType,
                                 struct reference ref, struct referenceItem sym);
extern void olcxFreeCompletion(Completion *completion);
extern void olcxFreeCompletions(Completion *completions);

extern Completion  *completionListPrepend(Completion *completions, char *name, char *fullText,
                                          char *vclass, int jindent, Symbol *s, ReferenceItem *ri,
                                          Reference *dfpos, int symType, int vFunClass);

void tagSearchCompactShortResults(void);

#endif
