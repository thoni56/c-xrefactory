#ifndef YACC_PARSER_H_INCLUDED
#define YACC_PARSER_H_INCLUDED

#include "proto.h"

extern int yacc_yyparse();
extern void makeYaccCompletions(char *s, int len, Position *pos);

#endif
