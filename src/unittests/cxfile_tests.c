#include <cgreen/cgreen.h>

#include "cxfile.c"

S_memory *cxMemory=NULL;

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
