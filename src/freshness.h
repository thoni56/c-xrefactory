#ifndef FRESHNESS_H_INCLUDED
#define FRESHNESS_H_INCLUDED

#include <stdbool.h>

#include "includegraph.h"


/* What the knowledge of a CU is built from, collected once per request */
typedef struct inputs Inputs;

extern Inputs *collectInputs(void);
extern void freeInputs(Inputs *inputs);

extern bool knowledgeIsOutOfDate(Inputs *inputs, int fileNumber);

#endif
