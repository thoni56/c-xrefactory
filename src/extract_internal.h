#ifndef EXTRACT_INTERNAL_H_INCLUDED
#define EXTRACT_INTERNAL_H_INCLUDED

/* Types shared between extract.c and extract_tests.c. Not part of the
 * public API; do not include from other modules. */

#include <stdbool.h>

#include "reference.h"
#include "referenceableitem.h"


typedef enum {
    DATAFLOW_ANALYZED = 1,
    DATAFLOW_INSIDE_BLOCK = 2,
    DATAFLOW_OUTSIDE_BLOCK = 4,
    DATAFLOW_INSIDE_REENTER = 8,            /* value reenters the block             */
    DATAFLOW_INSIDE_PASSING = 16            /* a non-modified values pass via block */
} DataFlowBits;

typedef enum {
    CLASSIFIED_AS_LOCAL_VAR,
    CLASSIFIED_AS_VALUE_ARGUMENT,
    CLASSIFIED_AS_LOCAL_OUT_ARGUMENT,
    CLASSIFIED_AS_OUT_ARGUMENT,
    CLASSIFIED_AS_IN_OUT_ARGUMENT,
    CLASSIFIED_AS_ADDRESS_ARGUMENT,
    CLASSIFIED_AS_RESULT_VALUE,
    CLASSIFIED_AS_IN_RESULT_VALUE,
    CLASSIFIED_AS_LOCAL_RESULT_VALUE,
    CLASSIFIED_AS_NONE
} ExtractClassification;

typedef struct programGraphNode {
    struct reference *reference;          /* original reference of node */
    struct referenceableItem *referenceableItem;
    struct programGraphNode *jump;
    DataFlowBits regionSide;              /* INSIDE/OUTSIDE block */
    DataFlowBits state;                   /* where value comes from + flow flags */
    bool visited;                         /* visited during dataflow traversal */
    ExtractClassification classification; /* resulting classification */
    struct programGraphNode *next;
} ProgramGraphNode;

#endif
