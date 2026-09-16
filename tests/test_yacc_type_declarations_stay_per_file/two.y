%union {
    int ast_id;
}

%token TOK_two

%type <ast_id> common_rule

%start common_rule
%%

common_rule
    : TOK_two
    ;

%%
