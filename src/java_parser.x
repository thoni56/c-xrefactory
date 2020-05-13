#ifndef _JAVA_PARSER_X_
#define _JAVA_PARSER_X_

#include "proto.h"

extern int java_yyparse();
extern void makeJavaCompletions(char *s, int len, Position *pos);

#endif

/* Local variables: */
/* Mode: c          */
/* End:             */
