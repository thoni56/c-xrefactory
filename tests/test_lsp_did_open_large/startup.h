#ifndef STARTUP_H_INCLUDED
#define STARTUP_H_INCLUDED

#include <stdbool.h>

#include "argumentsvector.h"


/* High-level initialization functions called by various entry points */

extern void totalTaskEntryInitialisations(void);
extern void mainTaskEntryInitialisations(ArgumentsVector args);

extern bool initializeFileProcessing(ArgumentsVector args, ArgumentsVector nargs);

/* Restore to checkpoint after compiler discovery (clearing file-local macros).
 * Used by staleness refresh to reset macro state before re-parsing. */
extern void restoreMemoryCheckPoint(void);

#endif
