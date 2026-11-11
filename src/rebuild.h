#pragma once

#include "argumentsvector.h"
#include "freshness.h"


#define MAX_CUS_TO_REPARSE 128


/* The CUs one request builds */
typedef struct buildList BuildList;

extern BuildList *newBuildList(int capacity);
extern void freeBuildList(BuildList *list);
extern bool addToBuildList(BuildList *list, int fileNumber);

extern int collectCUsIncluding(Prerequisites *prerequisites, int fileNumber, int cuFileNumbers[],
                               int cuCount, int maxCUs);

extern void collectOutOfDateHeldCUs(BuildList *list, Prerequisites *prerequisites);
extern void collectSiblingCUs(BuildList *list, Prerequisites *prerequisites, int requestFileNumber);
extern void buildKnowledgeOfAll(BuildList *list, ArgumentsVector baseArgs);
extern void collectRequestCUForStripping(BuildList *list, Prerequisites *prerequisites, int requestFileNumber);
extern void collectRequestCUToBuild(BuildList *list, Prerequisites *prerequisites, int requestFileNumber);
extern void stripRequestCU(BuildList *list, int requestFileNumber);
extern void collectOutOfDateCUs(BuildList *list, Prerequisites *prerequisites);
extern int buildListCount(BuildList *list);

/* A file's references are removed when its content changes (ADR-0035) */
extern void removeReferencesOfChangedFiles(Prerequisites *prerequisites);

/* Mark every compilation unit unparsed so the next entry refresh reparses it.
 * Called after a project-config change, which can invalidate any file's parse
 * (defines flip #ifdefs, -I re-resolves includes) with no cheap way to know
 * which - so treat it as a logical cold restart. Old refs are left in place
 * until buildKnowledgeOfCU clears and builds each CU as it is touched. */
extern void markAllCompilationUnitsStale(void);
