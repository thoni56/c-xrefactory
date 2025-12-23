#ifndef COMPLETION_H_INCLUDED
#define COMPLETION_H_INCLUDED

#include "constants.h"
#include "match.h"
#include "referenceableitem.h"
#include "symbol.h"


/* ***************** COMPLETION STRUCTURES ********************** */

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

extern void freeMatches(Match *completions);

extern Match *prependToMatches(Match *matches, char *name, char *fullText, Symbol *symbol,
                               ReferenceableItem *referenceableItem, Reference *dfpos, int includedFileNumber);

void tagSearchCompactShortResults(void);

#endif
