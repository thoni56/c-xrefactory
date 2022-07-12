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

#include "proto.h"

#include "lexem.h"
typedef union {
#include "yystype.h"
} YYSTYPE;

extern YYSTYPE c_yylval;
#ifdef YYDEBUG
extern int c_yydebug;
#endif

extern YYSTYPE java_yylval;
extern YYSTYPE javaslyylval;
#ifdef YYDEBUG
/* Java parser is recursive so we need to set this in the stack of
   parsers when we start parsing */
extern int java_yydebug;
#endif

extern YYSTYPE yacc_yylval;
#ifdef YYDEBUG
extern int yacc_yydebug;
#endif

extern YYSTYPE *uniyylval;

extern void reset_reference_usage(Reference *reference, UsageKind usage);

#endif
