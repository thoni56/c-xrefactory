#include <cgreen/cgreen.h>

/* Must #include it since we are (for now) unittesting an internal macro (GetInt) */
#include "cxfile.c"

#include "misc.mock"

Describe(CxFile);
BeforeEach(CxFile) {}
AfterEach(CxFile) {}

static CharacterBuffer cxfBuf;

Ensure(CxFile, can_scan_int) {
    char ch = ' ';
    char *cc = cxfBuf.buffer;
    char *cfin = cxfBuf.fin;
    int recInfo;

    ScanInt(ch, cc, cfin, &cxfBuf, recInfo);
}
