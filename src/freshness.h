#ifndef FRESHNESS_H_INCLUDED
#define FRESHNESS_H_INCLUDED

#include <stdbool.h>

#include "timestamp.h"


/* Which files the knowledge of a CU depends on, and when they changed, as a request sees them */
typedef struct prerequisites Prerequisites;

typedef enum {
    KNOWLEDGE_CURRENT,
    KNOWLEDGE_MISSING,
    CU_CHANGED,
    PREREQUISITE_CHANGED,
    CLOSURE_TOO_LARGE,
} OutOfDateReason;

/* Why the knowledge of a CU is out of date, and the prerequisite that made it so */
typedef struct {
    OutOfDateReason reason;
    int             fileNumber;
} OutOfDate;

/* The change time of a change the server makes itself, now (ADR-0035) */
extern FileTimestamp changeTimeNow(void);

extern Prerequisites *collectPrerequisites(void);
extern void freePrerequisites(Prerequisites *prerequisites);

extern OutOfDate whyKnowledgeIsOutOfDate(Prerequisites *prerequisites, int fileNumber);
extern bool knowledgeIsOutOfDate(Prerequisites *prerequisites, int fileNumber);
extern int collectIncludeClosureOf(Prerequisites *prerequisites, int fileNumber, int fileNumbers[],
                                   int maxFileNumbers);

#endif
