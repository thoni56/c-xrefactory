#include "parsers.h"

#include "yacc_parser.h"
#include "c_parser.h"

#include "caching.h"


YYSTYPE *uniyylval = &c_yylval;


void parseCurrentInputFile(Language language) {
    if (language == LANG_YACC) {
        uniyylval = &yacc_yylval;
        yacc_yyparse();
    } else {
        uniyylval = &c_yylval;
        c_yyparse();
    }
}
