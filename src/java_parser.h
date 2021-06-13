#ifndef _JAVA_PARSER_H_
#define _JAVA_PARSER_H_

#include "proto.h"

extern int java_yyparse();
extern void makeJavaCompletions(char *s, int len, Position *pos);

#endif
