#ifndef COMPLETION_H_INCLUDED
#define COMPLETION_H_INCLUDED

#include "visibility.h"
#include "referenceableitem.h"
#include "symbol.h"
#include "type.h"
#include "constants.h"


/* ***************** COMPLETION STRUCTURES ********************** */

typedef struct completion {
    char                 *name;
    char                 *fullName;
    short int             lineCount;
    Visibility            visibility; /* Global/Local */
    struct reference      reference;
    struct referenceableItem  referenceable;
    struct completion    *next;
} Completion;

typedef struct completionLine {
    char          *string;
    struct symbol *symbol;
    Type           type;
    short int      margn;
    char         **margs;
} CompletionLine;

typedef struct completions {
    char                  idToProcess[MAX_FUNCTION_NAME_LENGTH];
    int                   idToProcessLength;
    struct position       idToProcessPosition;
    bool                  fullMatchFlag;
    bool                  isCompleteFlag;
    bool                  noFocusOnCompletions;
    bool                  abortFurtherCompletions;
    char                  prefix[TMP_STRING_SIZE];
    int                   maxLen;
    struct completionLine alternatives[MAX_COMPLETIONS];
    int                   alternativeCount;
} Completions;

extern void freeCompletions(Completion *completions);

extern Completion *completionListPrepend(Completion *completions, char *name, char *fullText, Symbol *symbol,
                                         ReferenceableItem *ri, Reference *dfpos, int includedFileNumber);

void tagSearchCompactShortResults(void);

#endif
