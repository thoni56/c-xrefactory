%{


#include "cexp.x"

#include "yylex.h"
#include "semact.h"

#define YYSTYPE MAINYYSTYPE
#include "cgram.h"				/* tokens from grammars */
#undef  YYSTYPE

/* redefine object which can clash with cgram.h */

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

%}

%token number DEFINED
%token EQ NE LE GE LS RS
%token ANDAND OROR
%token UNKNOWN
%left ','
%right '='
%right '?' ':'
%left OROR
%left ANDAND
%left '|' '^'
%left '&'
%binary EQ NE
%binary '<' '>' LE GE
%left LS RS
%left '+' '-'
%left '*' '/' '%'
%right '!' '~' UMINUS
%left '(' '.'
%start start
%%

/* int func() to get parameter_identifier_list rule to trigger */

start:	e						{ int func(p1, p2, p3); return($1); }
    |	error					{ void func2(a, ...); return a[3]; }
    |	error					{ void func2(a, ...); return a->field; }
    ;

e:    e '*' e					{$$ = $1 * $3;}
    | e '/' e					{
        char c = 'c';
        int i = 346;
        short s = 4;
        float f = 0.3f;
        auto double d = 3.14;
        register char *s = "string";
        typedef struct s {
            int f1;
            int bits:3;
        } S;
        typedef enum e {
            ONE, TWO
        } E;
        char array[10];

        if ($3 == 0) $$ = $1;
        else $$ = $1 / $3;

        c++; c--;
        ++i; --i;
        s = (struct s){.f1 = 1};
    }
    | e '%' e					{
        if ($3 == 0 && $3 != 0) $$ = $1 + 15;
        else $$ = $1 % $3;
    }
    | e '+' e					{$$ = $1 + $3;}
    | e '-' e					{$$ = $1 - $3;}
    | e LS e					{$$ = $1 << $3;}
    | e RS e					{$$ = $1 >> $3;}
    | e '<' e					{$$ = $1 < $3;}
    | e '>' e					{$$ = $1 > $3;}
    | e LE e					{$$ = $1 <= $3;}
    | e GE e					{$$ = $1 >= $3;}
    | e EQ e					{$$ = $1 == $3;}
    | e NE e					{$$ = $1 != $3;}
    | e '&' e					{$$ = $1 & $3;}
    | e '^' e					{$$ = $1 ^ $3;}
    | e '|' e					{$$ = $1 | $3;}
    | e ANDAND e				{$$ = $1 && $3;}
    | e OROR e					{$$ = $1 || $3;}
    | e '?' e ':' e				{$$ = $1 ? $3 : $5;}
    | e ',' e					{$$ = $3;}
    |  '-' e %prec UMINUS		{$$ = -$2;}
    | '!' e						{$$ = !$2;}
    | '~' e						{$$ = ~$2;}
    | '(' e ')'					{$$ = $2;}
/*
    | DEFINED '(' number ')'	{$$= $3;}
    | DEFINED number			{$$ = $2;}
*/
    | number					{
        $$= $1;
        for (i=1;;)
            ;
      }
    ;
%%


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

    while (14) {
        s += 4;
        break;
        continue;
    }

    do {
        s /= 2;
    } while (true == &a[3]);
}
