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
%left EQ NE
%left '<' '>' LE GE
%left LS RS
%left '+' '-'
%left '*' '/' '%'
%right '!' '~' UMINUS
%left '(' '.'
%start start
%%

start:	e						{ return($1); }
    |	error					{ return(0); }
    ;

e:    e '*' e					{$$ = $1 * $3;}
    | e '/' e					{
        if ($3 == 0) $$ = $1;
        else $$ = $1 / $3;
    }
    | e '%' e					{
        if ($3 == 0) $$ = $1;
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
    | number					{$$= $1;}
    ;
%%
