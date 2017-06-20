#ifndef _UNIGRAM__H
#define _UNIGRAM__H

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

#include "cgram.h"

extern YYSTYPE cyylval;
extern YYSTYPE javayylval;
extern YYSTYPE javaslyylval;
extern YYSTYPE cccyylval;
extern YYSTYPE yaccyylval;

extern YYSTYPE *uniyylval;

#endif
