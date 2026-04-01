#ifndef REFACTORY_H_INCLUDED
#define REFACTORY_H_INCLUDED

#include "options.h"


extern Options refactoringOptions;

extern void applyWholeRefactoringFromUndo(void);
extern void ensureReferencesAreUpdated(char *project);
extern void serverPerformRefactoring(void);

#endif
