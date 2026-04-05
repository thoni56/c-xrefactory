#ifndef REFACTORY_H_INCLUDED
#define REFACTORY_H_INCLUDED

#include "options.h"


extern Options refactoringOptions;

extern void applyWholeRefactoringFromUndo(void);
extern void serverPerformRefactoring(void);

#endif
