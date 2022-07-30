#include "parsers.h"

#include "java_parser.h"
#include "yacc_parser.h"
#include "c_parser.h"

#include "recyacc.h"
#include "caching.h"


int java_yydebug = 0;

YYSTYPE *uniyylval = &c_yylval;


void parseInputFile(Language language) {
    if (language == LANG_JAVA) {
        uniyylval = &s_yygstate->gyylval;
        java_yyparse();
    }
    else if (language == LANG_YACC) {
        uniyylval = &yacc_yylval;
        yacc_yyparse();
    }
    else {
        uniyylval = &c_yylval;
        c_yyparse();
    }
    cache.active = false;
}
