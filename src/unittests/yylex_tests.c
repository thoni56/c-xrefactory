#include <cgreen/cgreen.h>

#include "yylex.h"
/* Declare semi-private function */
void processDefine(bool argFlag);

#include "filedescriptor.h"
#include "filetab.h"
#include "fileitem.h"
#include "symboltable.h"
#include "macroargumenttable.h"
#include "symbol.h"

#include "globals.mock"
#include "caching.mock"
#include "parsers.mock"
#include "cexp_parser.mock"
#include "jslsemact.mock"
#include "jsemact.mock"
#include "reftab.mock"
#include "editor.mock"
#include "commons.mock"
#include "misc.mock"
#include "cxfile.mock"
#include "characterbuffer.mock"
#include "lex.mock"
#include "semact.mock"
#include "java_parser.mock"
#include "cxref.mock"
#include "extract.mock"
#include "c_parser.mock"
#include "yacc_parser.mock"
#include "fileio.mock"


Describe(Yylex);
BeforeEach(Yylex) {
    stackMemoryInit();
    ppMemInit();
    initFileTab(&s_fileTab);
    XX_ALLOC(s_symbolTable, S_symbolTable);
    symbolTableInit(s_symbolTable, 10);
}
AfterEach(Yylex) {}

static void setup_lexBuffer_for_reading(void *data) {
    /* Need to insert lexem-codes first ? */
    cFile.lexBuffer.chars[0] = '\275';
    cFile.lexBuffer.chars[1] = '\001';
    strcpy(&cFile.lexBuffer.chars[2], cFile.lexBuffer.buffer.chars);
    *strchr(&cFile.lexBuffer.chars[2], ' ') = '\0';
    cFile.lexBuffer.next = cFile.lexBuffer.chars;
    cFile.lexBuffer.end = strchr(cFile.lexBuffer.chars, '\0');
    cFile.lexBuffer.index = 2;
}

Ensure(Yylex, add_a_cpp_definition_to_the_symbol_table) {
#   define DEFINE "__x86_64__"
    char *definition = (char *)malloc(strlen(DEFINE)+1);
    strcpy(definition, DEFINE);

    expect(getLexBuf, when(buffer, is_equal_to(&cFile.lexBuffer)),
           will_return(1), with_side_effect(setup_lexBuffer_for_reading, NULL));
    expect(setGlobalFileDepNames, when(iname, is_equal_to_string(definition)),
           will_set_contents_of_parameter(pp_name, &definition, sizeof(char *)));
    expect(getLexBuf, when(buffer, is_equal_to(&cFile.lexBuffer)),
           will_return(0));

    /* This is the confirmation that there is a symbol p with a
     * field named name equal to DEFINE
     */
    expect(addCxReference, when(p_name, is_equal_to_string(DEFINE)));

    s_opt.taskRegime = RegimeXref;
    /* If the define does not have a body, add the value of "1" */
    initInput(NULL, NULL, "__x86_64__ 1", NULL);
    cFile.lineNumber = 1;
    processDefine(false);

    /* Inspect symboltable for the define */
}
