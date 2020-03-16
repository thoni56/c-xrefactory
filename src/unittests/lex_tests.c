#include <cgreen/cgreen.h>

/* Including the source since we are testing macros */
#include "lex.c"

Describe(Lex);
BeforeEach(Lex) {}
AfterEach(Lex) {}

/* From globals.c */
S_caching s_cache;
int s_language;
char s_olstring[MAX_FUN_NAME_SIZE];
S_currentlyParsedStatics s_cps;
S_position s_cxRefPos;
S_options s_opt;        // current options
char *s_editCommunicationString = "C@$./@mVeDitznAC";
int s_olOriginalFileNumber = -1;
int macroStackIndex=0;
int s_noneFileIndex = -1;
int inStacki=0;

/* From jslsemact.c */
S_jslStat *s_jsl;

#include "commons.mock"
#include "caching.mock"
#include "cxref.mock"
#include "utils.mock"
#include "charbuf.mock"

Ensure(Lex, can_get_char) {
    //struct lexBuf lexBuf;
    //cgreen_mocks_are(learning_mocks);

    //getLexBuf(&lexBuf);
}
