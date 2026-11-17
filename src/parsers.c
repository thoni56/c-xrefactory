#include "parsers.h"

#include "parsing.h"
#include "yacc_parser.h"
#include "c_parser.h"
#include "parsing.h"


YYSTYPE *uniyylval = &c_yylval;


void callParser(void) {
    currentFileNumber = parsingConfig.fileNumber;
    if (parsingConfig.language == LANG_YACC) {
        uniyylval = &yacc_yylval;
        yacc_yyparse();
    } else {
        uniyylval = &c_yylval;
        c_yyparse();
    }
}
