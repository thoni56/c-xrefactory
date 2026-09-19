#ifndef PARSERS_H_INCLUDED
#define PARSERS_H_INCLUDED

#ifdef __WIN32__

#ifdef CONST
#undef CONST
#endif

#ifdef VOID
#undef VOID
#endif

#ifdef THIS
#undef THIS
#endif

#ifdef DELETE
#undef DELETE
#endif

#endif

#include "ast.h"
#include "head.h"               /* For Language type */

#include "stackmemory.h"


typedef union {
#include "yystype.h"
} YYSTYPE;


extern YYSTYPE c_yylval;
#ifdef YYDEBUG
extern int c_yydebug;
#endif

extern YYSTYPE yacc_yylval;
#ifdef YYDEBUG
extern int yacc_yydebug;
#endif

extern YYSTYPE *uniyylval;

void callParser(int fileNumber, Language language);

#endif
