#include "parsers.h"

#include "globals.h"
#include "java_parser.h"
#include "yacc_parser.h"
#include "c_parser.h"

#include "recyacc.h"
#include "caching.h"
#include "filedescriptor.h"

int java_yydebug = 0;

YYSTYPE *uniyylval = &c_yylval;

/*///////////////////////// parsing /////////////////////////////////// */
void parseInputFile(void) {
    if (currentLanguage == LANG_JAVA) {
        uniyylval = & s_yygstate->gyylval;
        java_yyparse();
    }
    else if (currentLanguage == LANG_YACC) {
        //printf("Parsing YACC-file\n");
        uniyylval = & yacc_yylval;
        yacc_yyparse();
    }
    else {
        uniyylval = & c_yylval;
        c_yyparse();
    }
    cache.active = false;
    currentFile.fileName = NULL;
}
