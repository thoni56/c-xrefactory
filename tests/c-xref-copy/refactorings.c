#include "refactorings.h"

// This should move to refactorings.c
typedef struct availableRefactoring {
    bool available;
    char *option;
} AvailableRefactoring;


static AvailableRefactoring refactorings[AVR_MAX_AVAILABLE_REFACTORINGS];

void clearAvailableRefactorings(void) {
    for (int i=0; i<AVR_MAX_AVAILABLE_REFACTORINGS; i++) {
        refactorings[i].available = false;
        refactorings[i].option = "";
    }
}

void makeRefactoringAvailable(Refactoring refactoring, char *option) {
    refactorings[refactoring].available = true;
    refactorings[refactoring].option = option;
}

bool isRefactoringAvailable(Refactoring refactoring) {
    return refactorings[refactoring].available;
}

int availableRefactoringsCount() {
    int count;
    count = 0;
    for (int i = 0; i < AVR_MAX_AVAILABLE_REFACTORINGS; i++) {
        if (refactorings[i].available)
            count++;
    }

    return count;
}

char *availableRefactoringOptionFor(Refactoring refactoring) {
    return refactorings[refactoring].option;
}
