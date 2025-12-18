#ifndef STARTUP_H_INCLUDED
#define STARTUP_H_INCLUDED

#include <stdbool.h>

#include "argumentsvector.h"
#include "head.h" /* For Language type */


/* High-level initialization functions called by various entry points */

extern void totalTaskEntryInitialisations(void);
extern void mainTaskEntryInitialisations(ArgumentsVector args);

extern bool initializeFileProcessing(ArgumentsVector args, ArgumentsVector nargs, Language *outLanguage,
                                     bool *firstPass);

/* Memory checkpoint management for option file reloading */
extern void saveMemoryCheckPoint(void);
extern void restoreMemoryCheckPoint(void);

#endif
