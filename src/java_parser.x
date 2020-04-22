#ifndef JAVAGRAM_X
#define JAVAGRAM_X

#include "proto.h"

extern int javayyparse();
extern void makeJavaCompletions(char *s, int len, Position *pos);

#endif

/* Local variables: */
/* Mode: c          */
/* End:             */
