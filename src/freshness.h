#ifndef FRESHNESS_H_INCLUDED
#define FRESHNESS_H_INCLUDED

#include <stdbool.h>

#include "includegraph.h"


extern bool knowledgeIsOutOfDate(IncludeGraph *graph, int fileNumber);

#endif
