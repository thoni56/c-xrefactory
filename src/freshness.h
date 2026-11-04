#ifndef FRESHNESS_H_INCLUDED
#define FRESHNESS_H_INCLUDED

#include <stdbool.h>

#include "includegraph.h"
#include "timestamp.h"


/* What the knowledge of a CU is built from, collected once per request */
typedef struct inputs Inputs;

typedef enum {
    KNOWLEDGE_CURRENT,
    KNOWLEDGE_MISSING,
    CU_CHANGED,
    INPUT_CHANGED,
    CLOSURE_TOO_LARGE,
} OutOfDateReason;

/* Why the knowledge of a CU is out of date, and the input that made it so */
typedef struct {
    OutOfDateReason reason;
    int             fileNumber;
} OutOfDate;

/* The change time of a change the server makes itself, now (ADR-0035) */
extern FileTimestamp changeTimeNow(void);

extern Inputs *collectInputs(void);
extern void freeInputs(Inputs *inputs);

extern OutOfDate whyKnowledgeIsOutOfDate(Inputs *inputs, int fileNumber);
extern bool knowledgeIsOutOfDate(Inputs *inputs, int fileNumber);
extern int collectIncludeClosureOf(Inputs *inputs, int fileNumber, int fileNumbers[],
                                   int maxFileNumbers);

#endif
