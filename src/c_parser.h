/*

  External declarations from c_parser-"module".

*/

#ifndef _C_PARSER_H_
#define _C_PARSER_H_

#include "proto.h"

extern int c_yyparse();
extern void makeCCompletions(char *s, int len, Position *pos);

#endif
