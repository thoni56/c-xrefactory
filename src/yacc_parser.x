#ifndef YACCGRAM_X
#define YACCGRAM_X

#include "proto.h"

extern int yaccyyparse();
extern void makeYaccCompletions(char *s, int len, Position *pos);

#endif

/* Local variables: */
/* Mode: c          */
/* End:             */
