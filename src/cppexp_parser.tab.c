#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define yyclearin (yychar=(-1))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define yyparse cppexp_yyparse
#define yylex cppexp_yylex
#define yyerror cppexp_yyerror
#define yychar cppexp_yychar
#define lastyystate cppexp_yylastyystate
#define yyval cppexp_yyval
#define yylval cppexp_yylval
#define yydebug cppexp_yydebug
#define yynerrs cppexp_yynerrs
#define yyerrflag cppexp_yyerrflag
#define yyss cppexp_yyss
#define yyssp cppexp_yyssp
#define yyvs cppexp_yyvs
#define yyvsp cppexp_yyvsp
#define yylhs cppexp_yylhs
#define yylen cppexp_yylen
#define yydefred cppexp_yydefred
#define yydgoto cppexp_yydgoto
#define yysindex cppexp_yysindex
#define yyrindex cppexp_yyrindex
#define yygindex cppexp_yygindex
#define yytable cppexp_yytable
#define yycheck cppexp_yycheck
#define yyname cppexp_yyname
#define yyrule cppexp_yyrule
#define YYPREFIX "cppexp_yy"
#line 2 "cppexp_parser.y"

#include <stdlib.h>

#include "cppexp_parser.h"

#include "yylex.h"
#include "semact.h"
#include "log.h"

#include "stackmemory.h"
#include "ast.h"

#define YYSTYPE CEXPYYSTYPE
#include "c_parser.tab.h"				/* tokens from grammars and overridden YYSTYPE */
#undef  YYSTYPE

#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define cppexp_yyerror styyerror
#define yyErrorRecovery styyErrorRecovery

#line 57 "cppexp_parser.tab.c"
#define number 257
#define DEFINED 258
#define EQ 259
#define NE 260
#define LE 261
#define GE 262
#define LS 263
#define RS 264
#define ANDAND 265
#define OROR 266
#define UNKNOWN 267
#define UMINUS 268
#define YYERRCODE 256
short cppexp_yylhs[] = {                                        -1,
    0,    0,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,
};
short cppexp_yylen[] = {                                         2,
    1,    1,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    5,    3,    2,    2,    2,    3,    1,
};
short cppexp_yydefred[] = {                                      0,
    2,   27,    0,    0,    0,    0,    0,    0,   23,   24,
   25,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   26,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    3,    4,    5,    0,    0,
};
short cppexp_yydgoto[] = {                                       7,
    8,
};
short cppexp_yysindex[] = {                                    -20,
    0,    0,   64,   64,   64,   64,    0,  409,    0,    0,
    0,  285,   64,   64,   64,   64,   64,   64,   64,   64,
   64,   64,   64,   64,   64,   64,   64,   64,   64,   64,
   64,   64,    0,  477,  477,  -10,  -10,  169,  169,  516,
  463,  436,  368,  -31,  -31,  160,  -10,  -10,  -37,  -37,
    0,    0,    0,   64,  436,
};
short cppexp_yyrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,    0,    2,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  276,  305,   76,   91,   24,   36,  100,
   49,   34,    0,  115,  250,  131,  124,  183,    1,    9,
    0,    0,    0,    0,  123,
};
short cppexp_yygindex[] = {                                      0,
  632,
};
#define YYTABLESIZE 780
short cppexp_yytable[] = {                                      32,
    6,    1,    0,    0,   30,   32,   25,    0,    7,   31,
   30,   28,    4,   29,    0,   31,    0,    0,    0,    6,
    0,    0,    0,    8,    3,    0,   32,    0,   26,    0,
   27,   30,   28,   22,   29,    9,   31,    0,    6,    0,
    0,    6,    0,    6,    6,    6,    7,    0,   20,    7,
    0,    7,    7,    7,    0,    0,    0,    0,    6,    0,
    6,    8,    6,    6,    8,    0,    7,    8,    7,    0,
    7,    7,    0,    9,   22,   12,    9,   22,    0,    9,
    0,    8,    0,    8,    0,    8,    8,    0,    0,   20,
   13,   22,   20,    9,    6,    9,    4,    9,    9,   19,
    0,    0,    7,    6,    0,    5,   20,    0,    3,    0,
    0,   20,    0,   12,   18,    0,   12,    8,    0,   12,
    0,    0,   21,   10,    6,    0,    0,    0,   13,    9,
   16,   13,    7,   12,   13,   12,    0,   12,   12,    0,
   19,    0,    0,   19,    0,    0,    0,    8,   13,    0,
   13,    0,   13,   13,    0,   18,    0,   19,   18,    9,
    0,   10,   19,   21,   10,    0,   21,   10,   16,   12,
    0,   16,   18,    0,   16,    0,    0,   18,    0,    0,
   21,   10,   11,   10,   13,   10,   10,    0,   16,    5,
    0,    0,    0,   16,    0,    0,   32,    0,    0,   12,
    0,   30,   28,    0,   29,   32,   31,    0,   18,    0,
   30,   28,    0,   29,   13,   31,    0,   10,    0,   26,
   11,   27,    0,   11,   16,    0,   11,   13,   14,   15,
   16,   17,   18,    0,    0,    1,    2,    0,   18,    0,
   11,    0,   11,    0,   11,   11,    0,   10,    0,   17,
    0,    0,   17,   18,   16,    0,    0,    0,    0,    6,
    6,    6,    6,    6,    6,    6,    6,    7,    7,    7,
    7,    7,    7,    7,    7,   14,   11,    0,    0,    0,
    0,    0,    8,    8,    8,    8,    8,    8,    8,    8,
   17,    0,    0,   17,    9,    9,    9,    9,    9,    9,
    9,    9,    0,    0,   15,    0,   11,   17,    0,    0,
    0,    0,   17,   14,   20,    0,   14,    0,    0,   14,
    2,   32,   25,    0,    0,   33,   30,   28,   21,   29,
    0,   31,    0,   14,   12,   12,   12,   12,   14,    0,
   12,   12,   15,   17,   26,   15,   27,   22,   15,   13,
   13,   13,   13,    0,    0,   13,   13,    0,    0,    0,
    0,    0,   15,    0,   19,   19,    0,   15,    0,   14,
    0,    0,    0,   17,    0,    0,    0,    0,   24,   18,
   18,    0,   10,   10,   10,   10,    0,    0,   10,   10,
    0,    0,    0,    0,    0,   16,   16,    0,   15,   14,
    0,    0,    0,    0,   32,   25,    0,    0,   23,   30,
   28,   21,   29,    0,   31,    0,    0,    0,   13,   14,
   15,   16,   17,   18,    0,   54,    0,   26,   15,   27,
   22,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   11,   11,   11,   11,   32,   25,   11,   11,    0,
   30,   28,   21,   29,    0,   31,    0,    0,    0,    0,
    0,   24,    0,    0,    0,    0,    0,    0,   26,    0,
   27,   22,   32,   25,    0,    0,    0,   30,   28,    0,
   29,    0,   31,    0,    0,    0,    0,    0,    0,    0,
    0,   23,    0,    0,    0,   26,    0,   27,   22,   32,
   25,    0,   24,    0,   30,   28,    0,   29,    0,   31,
    0,    0,    0,   32,   17,   17,    0,    0,   30,   28,
    0,   29,   26,   31,   27,    0,    0,    0,    0,   24,
    0,    0,   23,    0,   14,   14,   26,    0,   27,    0,
   14,   14,    0,   13,   14,   15,   16,   17,   18,   19,
   20,    0,   32,   25,    0,    0,   24,   30,   28,   23,
   29,    0,   31,   15,   15,    0,    0,    0,    0,   15,
   15,    0,    0,    0,    0,   26,    0,   27,    0,    0,
    0,    0,    0,    0,    0,    0,   23,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   24,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   13,   14,   15,   16,
   17,   18,   19,   20,    9,   10,   11,   12,    0,   23,
    0,    0,    0,    0,   34,   35,   36,   37,   38,   39,
   40,   41,   42,   43,   44,   45,   46,   47,   48,   49,
   50,   51,   52,   53,    0,    0,    0,   13,   14,   15,
   16,   17,   18,   19,   20,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   55,    0,    0,    0,    0,
    0,    0,    0,    0,   13,   14,   15,   16,   17,   18,
   19,   20,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   13,   14,   15,   16,   17,   18,   19,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   15,   16,   17,
   18,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   13,   14,   15,   16,   17,   18,
};
short cppexp_yycheck[] = {                                      37,
    0,    0,   -1,   -1,   42,   37,   38,   -1,    0,   47,
   42,   43,   33,   45,   -1,   47,   -1,   -1,   -1,   40,
   -1,   -1,   -1,    0,   45,   -1,   37,   -1,   60,   -1,
   62,   42,   43,    0,   45,    0,   47,   -1,   38,   -1,
   -1,   41,   -1,   43,   44,   45,   38,   -1,    0,   41,
   -1,   43,   44,   45,   -1,   -1,   -1,   -1,   58,   -1,
   60,   38,   62,   63,   41,   -1,   58,   44,   60,   -1,
   62,   63,   -1,   38,   41,    0,   41,   44,   -1,   44,
   -1,   58,   -1,   60,   -1,   62,   63,   -1,   -1,   41,
    0,   58,   44,   58,   94,   60,   33,   62,   63,    0,
   -1,   -1,   94,   40,   -1,  126,   58,   -1,   45,   -1,
   -1,   63,   -1,   38,    0,   -1,   41,   94,   -1,   44,
   -1,   -1,    0,    0,  124,   -1,   -1,   -1,   38,   94,
    0,   41,  124,   58,   44,   60,   -1,   62,   63,   -1,
   41,   -1,   -1,   44,   -1,   -1,   -1,  124,   58,   -1,
   60,   -1,   62,   63,   -1,   41,   -1,   58,   44,  124,
   -1,   38,   63,   41,   41,   -1,   44,   44,   38,   94,
   -1,   41,   58,   -1,   44,   -1,   -1,   63,   -1,   -1,
   58,   58,    0,   60,   94,   62,   63,   -1,   58,  126,
   -1,   -1,   -1,   63,   -1,   -1,   37,   -1,   -1,  124,
   -1,   42,   43,   -1,   45,   37,   47,   -1,   94,   -1,
   42,   43,   -1,   45,  124,   47,   -1,   94,   -1,   60,
   38,   62,   -1,   41,   94,   -1,   44,  259,  260,  261,
  262,  263,  264,   -1,   -1,  256,  257,   -1,  124,   -1,
   58,   -1,   60,   -1,   62,   63,   -1,  124,   -1,    0,
   -1,   -1,  263,  264,  124,   -1,   -1,   -1,   -1,  259,
  260,  261,  262,  263,  264,  265,  266,  259,  260,  261,
  262,  263,  264,  265,  266,    0,   94,   -1,   -1,   -1,
   -1,   -1,  259,  260,  261,  262,  263,  264,  265,  266,
   41,   -1,   -1,   44,  259,  260,  261,  262,  263,  264,
  265,  266,   -1,   -1,    0,   -1,  124,   58,   -1,   -1,
   -1,   -1,   63,   38,  266,   -1,   41,   -1,   -1,   44,
  257,   37,   38,   -1,   -1,   41,   42,   43,   44,   45,
   -1,   47,   -1,   58,  259,  260,  261,  262,   63,   -1,
  265,  266,   38,   94,   60,   41,   62,   63,   44,  259,
  260,  261,  262,   -1,   -1,  265,  266,   -1,   -1,   -1,
   -1,   -1,   58,   -1,  265,  266,   -1,   63,   -1,   94,
   -1,   -1,   -1,  124,   -1,   -1,   -1,   -1,   94,  265,
  266,   -1,  259,  260,  261,  262,   -1,   -1,  265,  266,
   -1,   -1,   -1,   -1,   -1,  265,  266,   -1,   94,  124,
   -1,   -1,   -1,   -1,   37,   38,   -1,   -1,  124,   42,
   43,   44,   45,   -1,   47,   -1,   -1,   -1,  259,  260,
  261,  262,  263,  264,   -1,   58,   -1,   60,  124,   62,
   63,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  259,  260,  261,  262,   37,   38,  265,  266,   -1,
   42,   43,   44,   45,   -1,   47,   -1,   -1,   -1,   -1,
   -1,   94,   -1,   -1,   -1,   -1,   -1,   -1,   60,   -1,
   62,   63,   37,   38,   -1,   -1,   -1,   42,   43,   -1,
   45,   -1,   47,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  124,   -1,   -1,   -1,   60,   -1,   62,   63,   37,
   38,   -1,   94,   -1,   42,   43,   -1,   45,   -1,   47,
   -1,   -1,   -1,   37,  265,  266,   -1,   -1,   42,   43,
   -1,   45,   60,   47,   62,   -1,   -1,   -1,   -1,   94,
   -1,   -1,  124,   -1,  259,  260,   60,   -1,   62,   -1,
  265,  266,   -1,  259,  260,  261,  262,  263,  264,  265,
  266,   -1,   37,   38,   -1,   -1,   94,   42,   43,  124,
   45,   -1,   47,  259,  260,   -1,   -1,   -1,   -1,  265,
  266,   -1,   -1,   -1,   -1,   60,   -1,   62,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  124,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   94,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  259,  260,  261,  262,
  263,  264,  265,  266,    3,    4,    5,    6,   -1,  124,
   -1,   -1,   -1,   -1,   13,   14,   15,   16,   17,   18,
   19,   20,   21,   22,   23,   24,   25,   26,   27,   28,
   29,   30,   31,   32,   -1,   -1,   -1,  259,  260,  261,
  262,  263,  264,  265,  266,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   54,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  259,  260,  261,  262,  263,  264,
  265,  266,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  259,  260,  261,  262,  263,  264,  265,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  261,  262,  263,
  264,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  259,  260,  261,  262,  263,  264,
};
#define YYFINAL 7
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 268
#if YYDEBUG
char *cppexp_yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"'!'",0,0,0,"'%'","'&'",0,"'('","')'","'*'","'+'","','","'-'","'.'","'/'",0,0,0,
0,0,0,0,0,0,0,"':'",0,"'<'","'='","'>'","'?'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,"'^'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,"'|'",0,"'~'",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"number","DEFINED","EQ","NE","LE",
"GE","LS","RS","ANDAND","OROR","UNKNOWN","UMINUS",
};
char *cppexp_yyrule[] = {
"$accept : start",
"start : e",
"start : error",
"e : e '*' e",
"e : e '/' e",
"e : e '%' e",
"e : e '+' e",
"e : e '-' e",
"e : e LS e",
"e : e RS e",
"e : e '<' e",
"e : e '>' e",
"e : e LE e",
"e : e GE e",
"e : e EQ e",
"e : e NE e",
"e : e '&' e",
"e : e '^' e",
"e : e '|' e",
"e : e ANDAND e",
"e : e OROR e",
"e : e '?' e ':' e",
"e : e ',' e",
"e : '-' e",
"e : '!' e",
"e : '~' e",
"e : '(' e ')'",
"e : number",
};
#endif
#ifndef YYSTYPE
typedef int YYSTYPE;
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 500
#define YYMAXDEPTH 500
#endif
#endif
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
int lastyystate;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short yyss[YYSTACKSIZE];
YYSTYPE yyvs[YYSTACKSIZE];
#define yystacksize YYSTACKSIZE
#line 84 "cppexp_parser.y"


int cppexpTranslateToken(int tok, int val) {
    if (tok == '\n') return(0);
    if (tok < 256) return(tok);
    switch (tok) {
    case CONSTANT: case LONG_CONSTANT:
        yylval = val;
        log_trace("reading constant %d", val);
        return(number);
    case EQ_OP:     return(EQ);
    case NE_OP:     return(NE);
    case LE_OP:     return(LE);
    case GE_OP:     return(GE);
    case LEFT_OP:   return(LS);
    case RIGHT_OP:  return(RS);
    case AND_OP:    return(ANDAND);
    case OR_OP:     return(OROR);
    }
/*	warning(ERR_ST,"unrecognized token in constant expression"); */
    yylval = 0;
    return(number);
    return(UNKNOWN);
}
#line 371 "cppexp_parser.tab.c"
#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
#define ERR_RECOVERY_ON 1
int
yyparse()
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register char *yys;

    if ((yys = getenv("YYDEBUG")))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if ((yyn = yydefred[yystate])) goto yyreduce;
    if (yychar < 0)
    {
        lastyystate = yystate;
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yyss + yystacksize - 1)
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag == 1)  yyErrorRecovery();
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
    goto yynewerror;
yynewerror:
    yyerror("syntax error");
    goto yyerrlab;
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < ERR_RECOVERY_ON)
    {
        yyerrflag = ERR_RECOVERY_ON;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yyss + yystacksize - 1)
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 1:
#line 47 "cppexp_parser.y"
{ return(yyvsp[0]); }
break;
case 2:
#line 48 "cppexp_parser.y"
{ return(0); }
break;
case 3:
#line 51 "cppexp_parser.y"
{yyval = yyvsp[-2] * yyvsp[0];}
break;
case 4:
#line 52 "cppexp_parser.y"
{
        if (yyvsp[0] == 0) yyval = yyvsp[-2];
        else yyval = yyvsp[-2] / yyvsp[0];
    }
break;
case 5:
#line 56 "cppexp_parser.y"
{
        if (yyvsp[0] == 0) yyval = yyvsp[-2];
        else yyval = yyvsp[-2] % yyvsp[0];
    }
break;
case 6:
#line 60 "cppexp_parser.y"
{yyval = yyvsp[-2] + yyvsp[0];}
break;
case 7:
#line 61 "cppexp_parser.y"
{yyval = yyvsp[-2] - yyvsp[0];}
break;
case 8:
#line 62 "cppexp_parser.y"
{yyval = yyvsp[-2] << yyvsp[0];}
break;
case 9:
#line 63 "cppexp_parser.y"
{yyval = yyvsp[-2] >> yyvsp[0];}
break;
case 10:
#line 64 "cppexp_parser.y"
{yyval = yyvsp[-2] < yyvsp[0];}
break;
case 11:
#line 65 "cppexp_parser.y"
{yyval = yyvsp[-2] > yyvsp[0];}
break;
case 12:
#line 66 "cppexp_parser.y"
{yyval = yyvsp[-2] <= yyvsp[0];}
break;
case 13:
#line 67 "cppexp_parser.y"
{yyval = yyvsp[-2] >= yyvsp[0];}
break;
case 14:
#line 68 "cppexp_parser.y"
{yyval = yyvsp[-2] == yyvsp[0];}
break;
case 15:
#line 69 "cppexp_parser.y"
{yyval = yyvsp[-2] != yyvsp[0];}
break;
case 16:
#line 70 "cppexp_parser.y"
{yyval = yyvsp[-2] & yyvsp[0];}
break;
case 17:
#line 71 "cppexp_parser.y"
{yyval = yyvsp[-2] ^ yyvsp[0];}
break;
case 18:
#line 72 "cppexp_parser.y"
{yyval = yyvsp[-2] | yyvsp[0];}
break;
case 19:
#line 73 "cppexp_parser.y"
{yyval = yyvsp[-2] && yyvsp[0];}
break;
case 20:
#line 74 "cppexp_parser.y"
{yyval = yyvsp[-2] || yyvsp[0];}
break;
case 21:
#line 75 "cppexp_parser.y"
{yyval = yyvsp[-4] ? yyvsp[-2] : yyvsp[0];}
break;
case 22:
#line 76 "cppexp_parser.y"
{yyval = yyvsp[0];}
break;
case 23:
#line 77 "cppexp_parser.y"
{yyval = -yyvsp[0];}
break;
case 24:
#line 78 "cppexp_parser.y"
{yyval = !yyvsp[0];}
break;
case 25:
#line 79 "cppexp_parser.y"
{yyval = ~yyvsp[0];}
break;
case 26:
#line 80 "cppexp_parser.y"
{yyval = yyvsp[-1];}
break;
case 27:
#line 81 "cppexp_parser.y"
{yyval= yyvsp[0];}
break;
#line 624 "cppexp_parser.tab.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
           lastyystate = yystate;
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yyss + yystacksize - 1)
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
