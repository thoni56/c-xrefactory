%{

#include "file2.h"

%}
 
%%

rule: 'a' { $$ = func(); }
;

%%
