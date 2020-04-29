#ifndef _PARSERS__H
#define _PARSERS__H

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

/* Include one of the generated parser definitions for common token codes */
#include "c_parser.tab.h"

extern YYSTYPE cyylval;
extern YYSTYPE cccyylval;
#ifdef YYDEBUG
extern int cyydebug;
#endif

extern YYSTYPE javayylval;
extern YYSTYPE javaslyylval;
#ifdef YYDEBUG
/* Java parser is recursive so we need to set this in the stack of parsers */
/* extern int javayydebug; */
#endif

extern YYSTYPE yaccyylval;
#ifdef YYDEBUG
extern int yaccyydebug;
#endif

extern YYSTYPE *uniyylval;

#endif
