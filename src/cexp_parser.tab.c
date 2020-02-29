#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define yyclearin (yychar=(-1))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define YYPREFIX "yy"
#line 2 "cexp.y"


#include "cexp.x"

#include "yylex.h"
#include "semact.h"

#define YYSTYPE MAINYYSTYPE
#include "c_parser.tab.h"				/* tokens from grammars */
#undef  YYSTYPE

/* Redefine names which can clash with C parser */

#define yylval cexpyylval

#define yyparse cexpyyparse
#define yylex cexpyylex
#define yylhs cexpyylhs
#define yylen cexpyylen
#define yydefred cexpyydefred
#define yydgoto cexpyydgoto
#define yysindex cexpyysindex
#define yyrindex cexpyyrindex
#define yytable cexpyytable
#define yycheck cexpyycheck
#define yyname cexpyyname
#define yyrule cexpyyrule
#define yydebug cexpyydebug
#define yynerrs cexpyynerrs
#define yyerrflag cexpyyerrflag
#define yychar cexpyychar
#define lastyystate cexplastyystate
#define yyssp cexpyyssp
#define yyval cexpyyval
#define yyss cexpyyss
#define yyvs cexpyyvs
#define yygindex cexpyygindex
#define yyvsp cexpyyvsp

#define YYDEBUG 0
#define yyerror styyerror
#define yyErrorRecovery styyErrorRecovery

#line 53 "cexp.tab.c"
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
short yylhs[] = {                                        -1,
    0,    0,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,
};
short yylen[] = {                                         2,
    1,    1,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    5,    3,    2,    2,    2,    3,    1,
};
short yydefred[] = {                                      0,
    2,   27,    0,    0,    0,    0,    0,    0,   23,   24,
   25,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   26,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    3,    4,    5,    0,    0,
};
short yydgoto[] = {                                       7,
    8,
};
short yysindex[] = {                                    -29,
    0,    0,  -19,  -19,  -19,  -19,    0,  414,    0,    0,
    0,  338,  -19,  -19,  -19,  -19,  -19,  -19,  -19,  -19,
  -19,  -19,  -19,  -19,  -19,  -19,  -19,  -19,  -19,  -19,
  -19,  -19,    0,  480,  480,  -30,  -30,  -37,  -37,  468,
  454,  426,  366,  326,  326,  438,  -30,  -30,  -17,  -17,
    0,    0,    0,  -19,  426,
};
short yyrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,    0,    2,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  112,  128,   73,   83,   24,   36,  140,
  161,   34,    0,  182,  188,  155,   91,  101,    1,    9,
    0,    0,    0,    0,   35,
};
short yygindex[] = {                                      0,
  630,
};
#define YYTABLESIZE 744
short yytable[] = {                                      32,
    6,    1,    0,    4,   30,   28,   32,   29,    7,   31,
    6,   30,   28,    4,   29,    3,   31,    0,    0,   32,
    6,    0,    0,    8,   30,    3,    0,    0,    0,   31,
    0,    0,    0,   22,   21,    9,    0,    0,    6,    0,
    0,    6,    0,    6,    6,    6,    7,    0,    0,    7,
    0,    7,    7,    7,    0,    0,    0,    0,    6,    0,
    6,    8,    6,    6,    8,    0,    7,    8,    7,    0,
    7,    7,   12,    9,   22,   21,    9,   22,   21,    9,
    0,    8,   13,    8,    0,    8,    8,    0,    0,    0,
   10,   22,   21,    9,    6,    9,    5,    9,    9,    0,
   11,    0,    7,    0,    0,    0,    5,    0,    0,    0,
   12,   14,    0,   12,    0,    0,   12,    8,    0,    0,
   13,    0,    0,   13,    6,    0,   13,   15,   10,    9,
   12,   10,    7,    0,   10,   12,    0,    0,   11,   19,
   13,   11,    0,    0,   11,   13,    0,    8,   10,   14,
    0,    0,   14,   10,   16,   14,    0,    0,   11,    9,
   20,    0,    0,   11,    0,   15,   12,    0,   15,   14,
    0,   15,    0,    0,   14,    0,   13,    0,    0,    0,
   19,   18,    0,   19,   10,   15,    0,   17,    0,    0,
   15,    0,   16,    0,   11,   16,   12,   19,   16,    0,
    0,   20,   19,    0,   20,   14,   13,    0,    0,    0,
    0,    0,   16,    0,   10,    0,    0,   16,   20,    0,
    0,   15,   18,   20,   11,   18,    1,    2,   17,    0,
    0,   17,   17,   18,    0,   14,    0,    2,    0,   18,
    0,    0,    0,    0,   18,   17,    0,    0,   16,    0,
   17,   15,    0,    0,    0,    0,    0,    0,    0,    6,
    6,    6,    6,    6,    6,    6,    6,    7,    7,    7,
    7,    7,    7,    7,    7,   18,    0,    0,   16,    0,
    0,   17,    8,    8,    8,    8,    8,    8,    8,    8,
    0,    0,    0,    0,    9,    9,    9,    9,    9,    9,
    9,    9,    0,    0,    0,   18,    0,    0,    0,    0,
    0,   17,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   12,   12,    0,    0,    0,    0,   12,   12,    0,
    0,   13,   13,    0,    0,    0,    0,   13,   13,   10,
   10,    0,    0,    0,    0,   10,   10,    0,    0,   11,
   11,    0,   32,   25,    0,   11,   11,   30,   28,    0,
   29,    0,   31,    0,   32,   25,   14,   14,   33,   30,
   28,   21,   29,    0,   31,   26,    0,   27,    0,    0,
    0,    0,   15,   15,    0,    0,    0,   26,    0,   27,
   22,    0,   32,   25,   19,   19,    0,   30,   28,   21,
   29,    0,   31,    0,    0,    0,    0,    0,    0,   16,
   16,    0,    0,   54,    0,   26,   20,   27,   22,    0,
    0,   24,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   18,   18,    0,    0,
   32,   25,   17,   17,    0,   30,   28,   21,   29,   24,
   31,   23,   32,   25,    0,    0,    0,   30,   28,    0,
   29,    0,   31,   26,   32,   27,   22,    0,    0,   30,
   28,    0,   29,    0,   31,   26,    0,   27,   22,   23,
   32,   25,    0,    0,    0,   30,   28,   26,   29,   27,
   31,    0,    0,    0,   32,   25,    0,   24,    0,   30,
   28,    0,   29,   26,   31,   27,   32,    0,    0,   24,
    0,   30,   28,    0,   29,    0,   31,   26,    0,   27,
    0,    0,    0,    0,    0,    0,    0,   23,    0,   26,
    0,   27,    0,    0,    0,    0,    0,   24,    0,   23,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   24,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   23,    0,    0,
    0,    0,    0,    0,   13,   14,   15,   16,   17,   18,
    0,   23,    0,    0,    0,    0,   13,   14,   15,   16,
   17,   18,   19,   20,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   13,   14,   15,   16,   17,   18,
   19,   20,    9,   10,   11,   12,    0,    0,    0,    0,
    0,    0,   34,   35,   36,   37,   38,   39,   40,   41,
   42,   43,   44,   45,   46,   47,   48,   49,   50,   51,
   52,   53,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   13,   14,   15,   16,   17,   18,   19,   20,
    0,    0,    0,   55,   13,   14,   15,   16,   17,   18,
   19,   20,    0,    0,    0,    0,   13,   14,   15,   16,
   17,   18,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   13,   14,   15,   16,   17,   18,   19,    0,
    0,    0,    0,    0,    0,    0,   13,   14,   15,   16,
   17,   18,    0,    0,    0,    0,    0,    0,    0,    0,
   15,   16,   17,   18,
};
short yycheck[] = {                                      37,
    0,    0,   -1,   33,   42,   43,   37,   45,    0,   47,
   40,   42,   43,   33,   45,   45,   47,   -1,   -1,   37,
   40,   -1,   -1,    0,   42,   45,   -1,   -1,   -1,   47,
   -1,   -1,   -1,    0,    0,    0,   -1,   -1,   38,   -1,
   -1,   41,   -1,   43,   44,   45,   38,   -1,   -1,   41,
   -1,   43,   44,   45,   -1,   -1,   -1,   -1,   58,   -1,
   60,   38,   62,   63,   41,   -1,   58,   44,   60,   -1,
   62,   63,    0,   38,   41,   41,   41,   44,   44,   44,
   -1,   58,    0,   60,   -1,   62,   63,   -1,   -1,   -1,
    0,   58,   58,   58,   94,   60,  126,   62,   63,   -1,
    0,   -1,   94,   -1,   -1,   -1,  126,   -1,   -1,   -1,
   38,    0,   -1,   41,   -1,   -1,   44,   94,   -1,   -1,
   38,   -1,   -1,   41,  124,   -1,   44,    0,   38,   94,
   58,   41,  124,   -1,   44,   63,   -1,   -1,   38,    0,
   58,   41,   -1,   -1,   44,   63,   -1,  124,   58,   38,
   -1,   -1,   41,   63,    0,   44,   -1,   -1,   58,  124,
    0,   -1,   -1,   63,   -1,   38,   94,   -1,   41,   58,
   -1,   44,   -1,   -1,   63,   -1,   94,   -1,   -1,   -1,
   41,    0,   -1,   44,   94,   58,   -1,    0,   -1,   -1,
   63,   -1,   38,   -1,   94,   41,  124,   58,   44,   -1,
   -1,   41,   63,   -1,   44,   94,  124,   -1,   -1,   -1,
   -1,   -1,   58,   -1,  124,   -1,   -1,   63,   58,   -1,
   -1,   94,   41,   63,  124,   44,  256,  257,   41,   -1,
   -1,   44,  263,  264,   -1,  124,   -1,  257,   -1,   58,
   -1,   -1,   -1,   -1,   63,   58,   -1,   -1,   94,   -1,
   63,  124,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  259,
  260,  261,  262,  263,  264,  265,  266,  259,  260,  261,
  262,  263,  264,  265,  266,   94,   -1,   -1,  124,   -1,
   -1,   94,  259,  260,  261,  262,  263,  264,  265,  266,
   -1,   -1,   -1,   -1,  259,  260,  261,  262,  263,  264,
  265,  266,   -1,   -1,   -1,  124,   -1,   -1,   -1,   -1,
   -1,  124,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,  259,  260,   -1,   -1,   -1,   -1,  265,  266,   -1,
   -1,  259,  260,   -1,   -1,   -1,   -1,  265,  266,  259,
  260,   -1,   -1,   -1,   -1,  265,  266,   -1,   -1,  259,
  260,   -1,   37,   38,   -1,  265,  266,   42,   43,   -1,
   45,   -1,   47,   -1,   37,   38,  265,  266,   41,   42,
   43,   44,   45,   -1,   47,   60,   -1,   62,   -1,   -1,
   -1,   -1,  265,  266,   -1,   -1,   -1,   60,   -1,   62,
   63,   -1,   37,   38,  265,  266,   -1,   42,   43,   44,
   45,   -1,   47,   -1,   -1,   -1,   -1,   -1,   -1,  265,
  266,   -1,   -1,   58,   -1,   60,  266,   62,   63,   -1,
   -1,   94,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  265,  266,   -1,   -1,
   37,   38,  265,  266,   -1,   42,   43,   44,   45,   94,
   47,  124,   37,   38,   -1,   -1,   -1,   42,   43,   -1,
   45,   -1,   47,   60,   37,   62,   63,   -1,   -1,   42,
   43,   -1,   45,   -1,   47,   60,   -1,   62,   63,  124,
   37,   38,   -1,   -1,   -1,   42,   43,   60,   45,   62,
   47,   -1,   -1,   -1,   37,   38,   -1,   94,   -1,   42,
   43,   -1,   45,   60,   47,   62,   37,   -1,   -1,   94,
   -1,   42,   43,   -1,   45,   -1,   47,   60,   -1,   62,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  124,   -1,   60,
   -1,   62,   -1,   -1,   -1,   -1,   -1,   94,   -1,  124,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   94,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  124,   -1,   -1,
   -1,   -1,   -1,   -1,  259,  260,  261,  262,  263,  264,
   -1,  124,   -1,   -1,   -1,   -1,  259,  260,  261,  262,
  263,  264,  265,  266,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  259,  260,  261,  262,  263,  264,
  265,  266,    3,    4,    5,    6,   -1,   -1,   -1,   -1,
   -1,   -1,   13,   14,   15,   16,   17,   18,   19,   20,
   21,   22,   23,   24,   25,   26,   27,   28,   29,   30,
   31,   32,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  259,  260,  261,  262,  263,  264,  265,  266,
   -1,   -1,   -1,   54,  259,  260,  261,  262,  263,  264,
  265,  266,   -1,   -1,   -1,   -1,  259,  260,  261,  262,
  263,  264,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  259,  260,  261,  262,  263,  264,  265,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  259,  260,  261,  262,
  263,  264,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
  261,  262,  263,  264,
};
#define YYFINAL 7
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 268
#if YYDEBUG
char *yyname[] = {
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
char *yyrule[] = {
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
#ifndef RECURSIVE
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
#else
#include "recyacc.h"
#endif
#define yystacksize YYSTACKSIZE
#line 109 "cexp.y"


int cexpTranslateToken(int tok, int val) {
    if (tok == '\n') return(0);
    if (tok < 256) return(tok);
    switch (tok) {
    case CONSTANT: case LONG_CONSTANT:
        yylval = val;
/*fprintf(dumpOut,"reading constant %d\n",val);*/
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
#line 365 "cexp.tab.c"
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
#line 68 "cexp.y"
{ return(yyvsp[0]); }
break;
case 2:
#line 69 "cexp.y"
{ return(0); }
break;
case 3:
#line 72 "cexp.y"
{yyval = yyvsp[-2] * yyvsp[0];}
break;
case 4:
#line 73 "cexp.y"
{
        if (yyvsp[0] == 0) yyval = yyvsp[-2];
        else yyval = yyvsp[-2] / yyvsp[0];
    }
break;
case 5:
#line 77 "cexp.y"
{
        if (yyvsp[0] == 0) yyval = yyvsp[-2];
        else yyval = yyvsp[-2] % yyvsp[0];
    }
break;
case 6:
#line 81 "cexp.y"
{yyval = yyvsp[-2] + yyvsp[0];}
break;
case 7:
#line 82 "cexp.y"
{yyval = yyvsp[-2] - yyvsp[0];}
break;
case 8:
#line 83 "cexp.y"
{yyval = yyvsp[-2] << yyvsp[0];}
break;
case 9:
#line 84 "cexp.y"
{yyval = yyvsp[-2] >> yyvsp[0];}
break;
case 10:
#line 85 "cexp.y"
{yyval = yyvsp[-2] < yyvsp[0];}
break;
case 11:
#line 86 "cexp.y"
{yyval = yyvsp[-2] > yyvsp[0];}
break;
case 12:
#line 87 "cexp.y"
{yyval = yyvsp[-2] <= yyvsp[0];}
break;
case 13:
#line 88 "cexp.y"
{yyval = yyvsp[-2] >= yyvsp[0];}
break;
case 14:
#line 89 "cexp.y"
{yyval = yyvsp[-2] == yyvsp[0];}
break;
case 15:
#line 90 "cexp.y"
{yyval = yyvsp[-2] != yyvsp[0];}
break;
case 16:
#line 91 "cexp.y"
{yyval = yyvsp[-2] & yyvsp[0];}
break;
case 17:
#line 92 "cexp.y"
{yyval = yyvsp[-2] ^ yyvsp[0];}
break;
case 18:
#line 93 "cexp.y"
{yyval = yyvsp[-2] | yyvsp[0];}
break;
case 19:
#line 94 "cexp.y"
{yyval = yyvsp[-2] && yyvsp[0];}
break;
case 20:
#line 95 "cexp.y"
{yyval = yyvsp[-2] || yyvsp[0];}
break;
case 21:
#line 96 "cexp.y"
{yyval = yyvsp[-4] ? yyvsp[-2] : yyvsp[0];}
break;
case 22:
#line 97 "cexp.y"
{yyval = yyvsp[0];}
break;
case 23:
#line 98 "cexp.y"
{yyval = -yyvsp[0];}
break;
case 24:
#line 99 "cexp.y"
{yyval = !yyvsp[0];}
break;
case 25:
#line 100 "cexp.y"
{yyval = ~yyvsp[0];}
break;
case 26:
#line 101 "cexp.y"
{yyval = yyvsp[-1];}
break;
case 27:
#line 106 "cexp.y"
{yyval= yyvsp[0];}
break;
#line 618 "cexp.tab.c"
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
