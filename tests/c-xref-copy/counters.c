#include "counters.h"

#include <memory.h>

#include "usage.h"


Counters counters;

/* Currently from semact.c ... */
extern void generateInternalLabelReference(int counter, int usage);

void resetAllCounters() {
    memset(&counters, 0, sizeof(Counters));
}

void resetLocalSymbolCounter(void) {
    counters.localVar = 0;
}

int nextGeneratedLocalSymbol(void) {
    return counters.localSym++;
}

int nextGeneratedLabelSymbol(void) {
    int n = counters.localSym;
    generateInternalLabelReference(counters.localSym, UsageDefined);
    counters.localSym++;
    return n;
}

int nextGeneratedGotoSymbol(void) {
    int n = counters.localSym;
    generateInternalLabelReference(counters.localSym, UsageUsed);
    counters.localSym++;
    return n;
}

int nextGeneratedForkSymbol(void) {
    int n = counters.localSym;
    generateInternalLabelReference(counters.localSym, UsageFork);
    counters.localSym++;
    return n;
}
