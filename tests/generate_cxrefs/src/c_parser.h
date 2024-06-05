/*

  External declarations from c_parser-"module".

*/

#ifndef C_PARSER_H_INCLUDED
#define C_PARSER_H_INCLUDED

#include "proto.h"

extern int c_yyparse();
extern void makeCCompletions(char *s, int len, Position *pos);

#endif
