#ifndef COMPLETION_H_INCLUDED
#define COMPLETION_H_INCLUDED

#include "constants.h"
#include "symbol.h"


typedef struct completionLine {
    char          *string;
    struct symbol *symbol;
    Type           type;
    short int      margn;
    char         **margs;
} CompletionLine;

typedef struct completions {
    char           idToProcess[MAX_FUNCTION_NAME_LENGTH];
    int            idToProcessLength;
    Position       idToProcessPosition;
    bool           fullMatchFlag;
    bool           isCompleteFlag;
    bool           noFocusOnCompletions;
    bool           abortFurtherCompletions;
    char           prefix[TMP_STRING_SIZE];
    int            maxLen;
    CompletionLine alternatives[MAX_COMPLETIONS];
    int            alternativeCount;
} Completions;

#endif
