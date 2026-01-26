%{
%}

%start file
%%
file
    : expr {
        $<typed>$ = 2;
    }
    ;

%%
