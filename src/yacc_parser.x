#ifndef _YACC_PARSER_X_
#define _YACC_PARSER_X_

#include "proto.h"

extern int yacc_yyparse();
extern void makeYaccCompletions(char *s, int len, Position *pos);

#endif

/* Local variables: */
/* Mode: c          */
/* End:             */
