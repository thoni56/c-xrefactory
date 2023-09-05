#include "counters.h"
#include "extract.h"
#include "usage.h"

Counters counters;

/* Currently from extract ... */
extern void generateInternalLabelReference(int counter, int usage);

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
