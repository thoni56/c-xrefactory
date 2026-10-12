#pragma once

#include "argumentsvector.h"
#include "freshness.h"


#define MAX_CUS_TO_REPARSE 128


/* The CUs one request builds */
typedef struct buildList BuildList;

extern BuildList *newBuildList(void);
extern void freeBuildList(BuildList *list);

extern int collectCUsIncluding(int fileNumber, int cuFileNumbers[], int cuCount, int maxCUs);

extern void collectOutOfDateHeldFiles(BuildList *list, Inputs *inputs);
extern void collectSiblingCUs(BuildList *list, Inputs *inputs, int requestFileNumber, bool requestIsParsed);
extern void buildKnowledgeOfAll(BuildList *list, ArgumentsVector baseArgs);
