#ifndef _SYMBOL_H_
#define _SYMBOL_H_

/* Dependencies: */
#include "proto.h"

/* Types: */

/* Should really have the symbol struct here also, but not until we
   don't need the FILL_symbol() macro because as long as it needs to
   be generated it has to live in proto.h */


/* Functions: */

/* NOTE These will not fill bits-field, has to be done after allocation */
extern S_symbol *newSymbol(char *name, char *linkName, struct position pos);
extern void fillSymbol(S_symbol *symbol, char *name, char *linkName, struct position pos);
extern S_symbol *newSymbolIsKeyword(char *name, char *linkName, struct position pos, int keyWordVal);
extern S_symbol *newSymbolIsType(char *name, char *linkName, struct position pos, struct typeModifiers *type);
extern S_symbol *newSymbolIsEnum(char *name, char *linkName, struct position pos, struct symbolList *enums);
extern S_symbol *newSymbolIsLabel(char *name, char *linkName, struct position pos, int labelIndex);

#endif
