#ifndef REFACTORINGS_H_INCLUDED
#define REFACTORINGS_H_INCLUDED

#include <stdbool.h>

typedef enum Refactorings {
    AVR_NO_REFACTORING = 0,
    AVR_RENAME_SYMBOL = 10,
    AVR_RENAME_CLASS = 20,
    AVR_RENAME_PACKAGE = 30,
    AVR_ADD_PARAMETER = 40,
    AVR_DEL_PARAMETER = 50,
    AVR_MOVE_PARAMETER = 60,
    AVR_TURN_DYNAMIC_METHOD_TO_STATIC = 100,
    AVR_TURN_STATIC_METHOD_TO_DYNAMIC = 110,
    AVR_PULL_UP_FIELD = 120,
    AVR_PULL_UP_METHOD = 130,
    AVR_PUSH_DOWN_FIELD = 140,
    AVR_PUSH_DOWN_METHOD = 150,
    AVR_MOVE_CLASS = 160,
    AVR_MOVE_CLASS_TO_NEW_FILE = 170,
    AVR_MOVE_ALL_CLASSES_TO_NEW_FILE = 180,
    AVR_ADD_TO_IMPORT = 210,
    AVR_EXTRACT_METHOD = 220,
    AVR_EXTRACT_FUNCTION = 230,
    AVR_EXTRACT_MACRO = 240,
    AVR_EXTRACT_VARIABLE = 250,
    AVR_ADD_ALL_POSSIBLE_IMPORTS = 280,
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
