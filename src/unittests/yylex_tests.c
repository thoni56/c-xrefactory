#include <cgreen/cgreen.h>

#include "yylex.h"
/* Declare semi-private function */
void processDefine(bool argFlag);

#include "filedescriptor.h"
#include "filetable.h"
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
#include "characterreader.mock"
#include "lexer.mock"
#include "semact.mock"
#include "java_parser.mock"
#include "cxref.mock"
#include "extract.mock"
#include "c_parser.mock"
#include "yacc_parser.mock"
#include "fileio.mock"
#include "memory.mock"


Describe(Yylex);
BeforeEach(Yylex) {
    ppMemInit();
    initFileTable(&fileTable);
    XX_ALLOC(s_symbolTable, SymbolTable);
    symbolTableInit(s_symbolTable, 10);
}
AfterEach(Yylex) {}

static void setup_lexBuffer_for_reading(void *data) {
    /* Need to insert lexem-codes first ? */
    currentFile.lexBuffer.chars[0] = '\275';
    currentFile.lexBuffer.chars[1] = '\001';
    /* TODO: WTF This is mostly guesswork, no idea if this is how they are connected... */
    strcpy(&currentFile.lexBuffer.chars[2], currentFile.lexBuffer.buffer.chars);
    *strchr(&currentFile.lexBuffer.chars[2], ' ') = '\0';
    currentFile.lexBuffer.next = currentFile.lexBuffer.chars;
    currentFile.lexBuffer.end = strchr(currentFile.lexBuffer.chars, '\0');
    currentFile.lexBuffer.index = 2;
}

Ensure(Yylex, add_a_cpp_definition_to_the_symbol_table) {
#   define DEFINE "__x86_64__"
    char *definition = (char *)malloc(strlen(DEFINE)+1);
    strcpy(definition, DEFINE);

    expect(getLexem, when(buffer, is_equal_to(&currentFile.lexBuffer)),
           will_return(1), with_side_effect(setup_lexBuffer_for_reading, NULL));
    expect(setGlobalFileDepNames, when(iname, is_equal_to_string(definition)),
           will_set_contents_of_parameter(pp_name, &definition, sizeof(char *)));
    expect(getLexem, when(buffer, is_equal_to(&currentFile.lexBuffer)),
           will_return(0));

    /* This is the confirmation that there is a symbol p with a
     * field with name equal to DEFINE
     */
    expect(addCxReference, when(p_name, is_equal_to_string(DEFINE)));

    options.taskRegime = RegimeXref;
    /* If the define does not have a body, add the value of "1" */
    initInput(NULL, NULL, "__x86_64__ 1", NULL);
    currentFile.lineNumber = 1;
    processDefine(false);
}
