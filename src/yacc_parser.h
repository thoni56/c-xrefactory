#ifndef _YACC_PARSER_H_
#define _YACC_PARSER_H_

#include "proto.h"

extern int yacc_yyparse();
extern void makeYaccCompletions(char *s, int len, Position *pos);

#endif
