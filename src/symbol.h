#ifndef _SYMBOL_H_
#define _SYMBOL_H_

/* Dependencies: */
#include "proto.h"

/* Types: */

/* Should really have the symbol struct here also, but not until we don't need the FILL... */


/* Functions: */

/* NOTE These will not fill bits-field, has to be done after allocation */
extern S_symbol *newSymbol(char *name, char *linkName, struct position pos, S_symbol *next);
extern S_symbol *newSymbolKeyword(char *name, char *linkName, struct position pos, int keyWordVal, S_symbol *next);
extern S_symbol *newSymbolType(char *name, char *linkName, struct position pos, struct typeModifiers *type, S_symbol *next);

#endif
