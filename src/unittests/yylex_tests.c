#include <cgreen/cgreen.h>

#include "yylex.h"

#include "filedescriptor.h"

#include "globals.mock"
#include "caching.mock"
#include "macroargumenttable.mock"
#include "symboltable.mock"
#include "parsers.mock"
#include "cexp_parser.mock"
#include "jslsemact.mock"
#include "jsemact.mock"
#include "reftab.mock"
#include "filetab.mock"
#include "editor.mock"
#include "symbol.mock"
#include "commons.mock"
#include "misc.mock"
#include "fileitem.mock"
#include "cxfile.mock"
#include "characterbuffer.mock"
#include "lex.mock"
#include "semact.mock"
#include "java_parser.mock"
#include "cxref.mock"


Describe(Yylex);
BeforeEach(Yylex) {}
AfterEach(Yylex) {}


Ensure(Yylex, add_a_cpp_definition_to_the_symbol_table) {
    initInput(NULL, NULL, "#define __x86_64__", NULL);
    cFile.lineNumber = 1;
    processDefine(false);
}
