#include <cgreen/cgreen.h>

/* Must #include it since we are (for now) unittesting an internal macro (ScanInt) */
#include "cxfile.c"

/* From globals.c */
S_memory *cxMemory=NULL;
int s_language;
FILE *cxOut;
int s_input_file_number = -1;
time_t s_fileProcessStartTime;
S_position s_olcxByPassPos;
S_fileTab s_fileTab;
int s_olMacro2PassFile;
char tmpBuff[TMP_BUFF_SIZE];
S_position s_noPos = {-1, 0, 0};
S_options s_opt;        // current options
int s_olOriginalFileNumber = -1;
int s_noneFileIndex = -1;
S_userOlcx *s_olcxCurrentUser;
int s_wildCharSearch;
FILE *fIn;
z_stream s_defaultZStream = {NULL,};

/* From reftab.c */
S_refTab s_cxrefTab;

#include "commons.mock"
#include "misc.mock"
#include "cxref.mock"
#include "html.mock"
#include "reftab.mock"
#include "filetab.mock"
#include "options.mock"
#include "yylex.mock"
#include "lex.mock"
#include "editor.mock"
#include "memmac.mock"
#include "enumTxt.c"

Describe(CxFile);
BeforeEach(CxFile) {}
AfterEach(CxFile) {}

static CharacterBuffer characterBuffer;

Ensure(CxFile, can_scan_int) {
    CharacterBuffer *cb = &characterBuffer;
    char next = ' ';
    char *characters;
    char *end;
    int result;

    characters = cb->buffer;
    strcpy(cb->buffer, "123");
    cb->end = &cb->buffer[strlen("123")];
    end = cb->end;

    expect(getCharBuf,
           when(buffer, is_equal_to_hex(&characterBuffer)));

    ScanInt(next, characters, end, &characterBuffer, result);

    assert_that(result, is_equal_to(123));
}
