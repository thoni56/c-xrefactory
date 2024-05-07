#ifndef REFACTORINGS_H_INCLUDED
#define REFACTORINGS_H_INCLUDED

#include <stdbool.h>

typedef enum Refactorings {
    AVR_NO_REFACTORING = 0,
    AVR_RENAME_SYMBOL = 10,
    AVR_RENAME_MODULE = 30,
    AVR_ADD_PARAMETER = 40,
    AVR_DEL_PARAMETER = 50,
    AVR_MOVE_PARAMETER = 60,
    AVR_MOVE_FUNCTION = 70,
    AVR_EXTRACT_FUNCTION = 230,
    AVR_EXTRACT_MACRO = 240,
    AVR_EXTRACT_VARIABLE = 250,
    AVR_SET_MOVE_TARGET = 290,
    AVR_UNDO = 300,
    AVR_MAX_AVAILABLE_REFACTORINGS = 310
} Refactoring;


extern void clearAvailableRefactorings(void);
extern void makeRefactoringAvailable(Refactoring refactoring, char *option);
extern bool isRefactoringAvailable(Refactoring refactoring);
extern int  availableRefactoringsCount(void);
extern char *availableRefactoringOptionFor(Refactoring refactoring);

#endif
