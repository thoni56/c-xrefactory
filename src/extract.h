#ifndef EXTRACT_H
#define EXTRACT_H

#include "proto.h"
#include "symbol.h"


/* TODO: push the conditions into the functions for these macros */

#define ExtrDeleteContBreakSym(sym) {                               \
    if (options.server_operation == OLO_EXTRACT) deleteSymDef(sym);\
}

#define EXTRACT_COUNTER_SEMACT(rescount) {\
        rescount = counters.localSym;\
        counters.localSym++;\
}

#define EXTRACT_LABEL_SEMACT(rescount) {\
        rescount = counters.localSym;\
        generateInternalLabelReference(counters.localSym, UsageDefined);\
        counters.localSym++;\
}

#define EXTRACT_GOTO_SEMACT(rescount) {\
        rescount = counters.localSym;\
        generateInternalLabelReference(counters.localSym, UsageUsed);\
        counters.localSym++;\
}

#define EXTRACT_FORK_SEMACT(rescount) {\
        rescount = counters.localSym;\
        generateInternalLabelReference(counters.localSym, UsageFork);\
        counters.localSym++;\
}


enum extractModes {
    EXTRACT_FUNCTION,
    EXTRACT_FUNCTION_ADDRESS_ARGS,
    EXTRACT_MACRO,
};


extern Symbol *addContinueBreakLabelSymbol(int labn, char *name);
extern void actionsBeforeAfterExternalDefinition(void);
extern void extractActionOnBlockMarker(void);
extern void generateInternalLabelReference(int counter, int usage);
extern void deleteContinueBreakLabelSymbol(char *name);
extern void genContinueBreakReference(char *name);
extern void generateSwitchCaseFork(bool isLast);

#endif
