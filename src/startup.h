#ifndef STARTUP_H_INCLUDED
#define STARTUP_H_INCLUDED

#include <stdbool.h>

#include "argumentsvector.h"
#include "optionsets.h"
#include "stringlist.h"


/* Project-level configuration, separate from per-request options */
typedef struct {
    StringList *sourceDirs;
    OptionSets optionSets;
} ProjectConfig;

extern ProjectConfig *getProjectConfig(void);

/* High-level initialization functions called by various entry points */

extern void initializeCoreSubsystems(void);
extern void totalTaskEntryInitialisations(void);
extern void mainTaskEntryInitialisations(ArgumentsVector args, StringList **operands);

extern bool initializeProjectContext(char *fileName, ArgumentsVector startupOptions,
                                     ArgumentsVector requestOptions);
extern bool initializeFileProcessing(ArgumentsVector args, ArgumentsVector nargs, char *fileName);
extern void startCompilationUnit(void);

extern int numberOfPasses(void);
extern void saveProjectBaseline(void);
extern void startCompilationUnitForPass(int pass);

/* Config-change detection: re-run scan when .c-xrefrc is edited mid-session */
extern bool isProjectConfigChanged(void);
extern void reloadProjectConfig(ArgumentsVector startupOptions, ArgumentsVector requestOptions);

#endif
