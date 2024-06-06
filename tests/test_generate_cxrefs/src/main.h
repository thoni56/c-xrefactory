#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include <stdbool.h>

#include "head.h" /* For Language type */


// xref.c, server.c
extern bool initializeFileProcessing(bool *firstPass, int argc, char **argv, int nargc, char **nargv,
                                     Language *outLanguage);

// main.c, refactory.c
extern void mainTaskEntryInitialisations(int argc, char **argv);

#endif
