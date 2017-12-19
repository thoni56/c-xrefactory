#ifndef EXTRACT_H
#define EXTRACT_H

#include "proto.h"

/* TODO: push the conditions into the functions for these macros */
#define GenInternalLabelReference(count,usage) {\
    if (s_opt.cxrefs == OLO_EXTRACT) genInternalLabelReference(count, usage);\
}
#define AddContinueBreakLabelSymbol(count, name, res) {\
    if (s_opt.cxrefs == OLO_EXTRACT) res = addContinueBreakLabelSymbol(count, name);\
}
#define DeleteContinueBreakLabelSymbol(name) {\
    if (s_opt.cxrefs == OLO_EXTRACT) deleteContinueBreakLabelSymbol(name);\
}

#define GenContBreakReference(name) {\
    if (s_opt.cxrefs == OLO_EXTRACT) genContinueBreakReference(name);\
}

#define GenSwitchCaseFork(lastFlag) {\
    if (s_opt.cxrefs == OLO_EXTRACT) genSwitchCaseFork(lastFlag);\
}

extern S_symbol * addContinueBreakLabelSymbol(int labn, char *name);
extern void actionsBeforeAfterExternalDefinition();
extern void extractActionOnBlockMarker();
extern void genInternalLabelReference(int counter, int usage);
extern S_symbol * addContinueBreakLabelSymbol(int labn, char *name);
extern void deleteContinueBreakLabelSymbol(char *name);
extern void genInternalLabelReference(int counter, int usage);
extern void genContinueBreakReference(char *name);
extern void genSwitchCaseFork(int lastFlag);

#endif
