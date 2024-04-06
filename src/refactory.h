#ifndef REFACTORY_H_INCLUDED
#define REFACTORY_H_INCLUDED


/* This structure holds information about which symbols(?) to push after a refactoring.
   Shared between refactory.c and cxref.c */
typedef struct pushRange {
    int lowestIndex;
    int highestIndex;
} PushRange;


extern void refactory(void);

#endif
