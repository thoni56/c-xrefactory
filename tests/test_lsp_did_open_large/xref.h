#ifndef XREF_INCLUDED
#define XREF_INCLUDED
#include <stdbool.h>

#include "argumentsvector.h"
#include "options.h"


typedef struct {
    char *projectName;      /* Project name, or NULL to use options.project */
    UpdateType updateType;  /* UPDATE_DEFAULT, UPDATE_FAST, UPDATE_FULL, or UPDATE_CREATE */
    bool isRefactoring;     /* true when called from refactory mode */
} XrefConfig;


extern void checkExactPositionUpdate(UpdateType *updateType, bool printMessage);
extern void callXref(ArgumentsVector args, XrefConfig *config);
extern void xref(ArgumentsVector args);
#endif
