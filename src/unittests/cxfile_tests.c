#include <cgreen/cgreen.h>

/* Must #include it since we are (for now) unittesting an internal macro (GetInt) */
#include "cxfile.c"

#include "misc.mock"

Describe(CxFile);
BeforeEach(CxFile) {}
AfterEach(CxFile) {}

static S_charBuf cxfBuf;

Ensure(CxFile, can_scan_int) {
    char ch = ' ';
    char *cc = cxfBuf.a;
    char *cfin = cxfBuf.fin;
    int recInfo;

    ScanInt(ch, cc, cfin, &cxfBuf, recInfo);
}
