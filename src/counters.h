#ifndef COUNTERS_H_INCLUDED
#define COUNTERS_H_INCLUDED

/* ***************** unique counters  *********************** */
typedef struct counters {
    int localSym;
    int localVar;
} Counters;

extern Counters counters;

extern void resetAllCounters(void);
extern void resetLocalSymbolCounter(void);
extern int nextGeneratedLocalSymbol(void);
extern int nextGeneratedLabelSymbol(void);
extern int nextGeneratedGotoSymbol(void);
extern int nextGeneratedForkSymbol(void);

#endif
