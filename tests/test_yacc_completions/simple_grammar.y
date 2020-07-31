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

start:	e					{ int func(p1, p2, p3); return($1); }
    |	error					{ void func2(a, ...); return a[3]; }
    |	err
    |   sta
    ;

e:    e '*' e					{$$ = $1 * $3;}
    | e '/' e					{
        char c = 'c';
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
    | number					{$$= $1;}
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
