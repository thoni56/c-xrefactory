#ifndef CGRAM_X
#define CGRAM_X

#include "proto.h"

extern int cyyparse();
extern void makeCCompletions(char *s, int len, S_position *pos);

#endif

/* Local variables: */
/* Mode: c          */
/* End:             */
