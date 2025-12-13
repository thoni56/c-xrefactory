#ifndef STARTUP_H_INCLUDED
#define STARTUP_H_INCLUDED

#include <stdbool.h>

#include "head.h" /* For Language type */


/* High-level initialization functions called by various entry points */

extern void totalTaskEntryInitialisations(void);
extern void mainTaskEntryInitialisations(int argc, char **argv);

extern bool initializeFileProcessing(bool *firstPass, int argc, char **argv, int nargc, char **nargv,
                                     Language *outLanguage);

/* Memory checkpoint management for option file reloading */
extern void saveMemoryCheckPoint(void);
extern void restoreMemoryCheckPoint(void);

#endif
