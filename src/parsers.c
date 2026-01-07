#include "parsers.h"

#include "parsing.h"
#include "yacc_parser.h"
#include "c_parser.h"
#include "parsing.h"


YYSTYPE *uniyylval = &c_yylval;


void callParser(int fileNumber, Language language) {
    currentFileNumber = fileNumber;
    if (language == LANG_YACC) {
        uniyylval = &yacc_yylval;
        yacc_yyparse();
    } else {
        uniyylval = &c_yylval;
        c_yyparse();
    }
}
