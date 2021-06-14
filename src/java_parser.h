#ifndef JAVA_PARSER_H_INCLUDED
#define JAVA_PARSER_H_INCLUDED

#include "proto.h"

extern int java_yyparse();
extern void makeJavaCompletions(char *s, int len, Position *pos);

#endif
