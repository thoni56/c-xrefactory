#ifndef FRESHNESS_H_INCLUDED
#define FRESHNESS_H_INCLUDED

#include <stdbool.h>

#include "includegraph.h"


/* What the knowledge of a CU is built from, collected once per request */
typedef struct inputs Inputs;

typedef enum {
    KNOWLEDGE_CURRENT,
    KNOWLEDGE_MISSING,
    CU_CHANGED,
    INPUT_CHANGED,
    INPUT_LEFT_BEHIND,
    CLOSURE_TOO_LARGE,
} OutOfDateReason;

/* Why the knowledge of a CU is out of date, and the input that made it so */
typedef struct {
    OutOfDateReason reason;
    int             fileNumber;
} OutOfDate;

extern Inputs *collectInputs(void);
extern void freeInputs(Inputs *inputs);

extern OutOfDate whyKnowledgeIsOutOfDate(Inputs *inputs, int fileNumber);
extern bool knowledgeIsOutOfDate(Inputs *inputs, int fileNumber);
extern bool changedAfterKnowledgeOf(Inputs *inputs, int fileNumber, int cuFileNumber);
extern int collectIncludeClosureOf(Inputs *inputs, int fileNumber, int fileNumbers[],
                                   int maxFileNumbers);

#endif
