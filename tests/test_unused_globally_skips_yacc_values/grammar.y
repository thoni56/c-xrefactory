%{
/* `term` is also a file-scope static. The grammar symbol of the same name
   resolves to this symbol, so it is what addRuleLocalVariable() passes as the
   base type for the $N it declares for that rule position. */
static int term = 0;
%}

%union {
    int number;
}

%token NUMBER
%token PLUS

%type <number> expression

%start expression
%%

expression
    : term
    | expression PLUS term    { $$ = $1 + $3; }
    ;

%%

int unused_globally(void) {
    return 0;
}
