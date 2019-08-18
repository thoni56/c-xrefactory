#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

/* Must #include it since we are (for now) unittesting an internal macro (GetInt) */
#include "cxfile.c"

/* Dependencies (manually collected so may change): */
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
S_options s_opt;
S_refTab s_cxrefTab;
int s_olOriginalFileNumber = -1;
int s_noneFileIndex = -1;
S_userOlcx *s_olcxCurrentUser;
int s_wildCharSearch;
FILE *fIn;
z_stream s_defaultZStream = {NULL,};

#include "enumTxt.c"

#include "cxref.mock"
#include "commons.mock"
#include "html.mock"
#include "misc.mock"
#include "reftab.mock"
#include "options.mock"
#include "yylex.mock"
#include "lex.mock"
#include "filetab.mock"
#include "editor.mock"
#include "memmac.mock"


Describe(CxFile);
BeforeEach(CxFile) {}
AfterEach(CxFile) {}


Ensure(CxFile, can_scan_int) {
    char ch = ' ';
    char *cc = cxfBuf.a;
    char *cfin = cxfBuf.fin;
    int recInfo;

    ScanInt(ch, cc, cfin, &cxfBuf, recInfo);
}
