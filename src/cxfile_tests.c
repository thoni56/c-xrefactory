#include <cgreen/assertions.h>
#include <cgreen/cgreen.h>
#include <cgreen/constraint.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/mocks.h>

#include "characterreader.h"
#include "cxfile.h"
#include "filetable.h"
#include "log.h"

#include "globals.mock"
#include "misc.mock"
#include "options.h"
#include "utils.mock"
#include "options.mock"
#include "commons.mock"
#include "caching.mock"

#include "olcxtab.mock"
#include "cxref.mock"
#include "reftab.mock"
#include "editor.mock"
#include "characterreader.mock"
#include "classhierarchy.mock"
#include "fileio.mock"


Describe(CxFile);
BeforeEach(CxFile) {
    log_set_level(LOG_DEBUG); /* Set to LOG_TRACE if needed */

    options.taskRegime = RegimeEditServer;
    initFileTable(100);
}
AfterEach(CxFile) {}

Ensure(CxFile, can_run_empty_test) {
    options.referenceFileCount = 1;
    assert_that(cxFileHashNumber(NULL), is_equal_to(0));
}

static bool trueValue  = true;
static bool falseValue = false;

/* The string should not start with a space, but it is assumed that
 * this string is delimited by some leading whitespace so it always
 * expects a skipWhitespace() first */
static void expect_string(char string[], bool eof) {
    int i;

    expect(skipWhiteSpace, will_return(string[0]));
    for (i = 1; string[i+1] != '\0'; i++) {
        if (isspace(string[i])) {
            expect(getChar, will_return(' '));
            if (string[i + 1] != '\0')
                expect(skipWhiteSpace, will_return(string[++i]));
        } else {
            expect(getChar, will_return(string[i]));
        }
    }
    if (eof)
        expect(getChar, will_return(string[i]),
               will_set_contents_of_parameter(isAtEOF, &trueValue, sizeof(bool)));
    else
        expect(getChar, will_return(string[i]),
               will_set_contents_of_parameter(isAtEOF, &falseValue, sizeof(bool)));
}


xEnsure(CxFile, can_do_normal_scan) {
    FILE *xfilesFilePointer = (FILE *)4654654645;
    FILE *sourceFilePointer = (FILE *)4679898745;

    options.cxrefsLocation = "./CXrefs";
    options.referenceFileCount = 1;

    expect(checkReferenceFileCountOption, will_return(10));

    expect(openFile, when(fileName, is_equal_to_string("./CXrefs/XFiles")), will_return(xfilesFilePointer));
    expect(initCharacterBuffer, when(file, is_equal_to(xfilesFilePointer)));

    /* Version marking always starts a file */
    expect_string("34v", false);
    expect(skipCharacters, when(count, is_equal_to(33)));
    expect(getChar, will_return(' '));

    /* Generation setttings, file count, ... */
    expect_string("21@mpfotulcsrhdeibnaAgk 10n 300000k ", false);

    expect_string("49976f 1646087914m 53:/home/thoni/Utveckling/c-xrefactory/src/extract.mock ", false);
    expect(normalizeFileName, when(name, is_equal_to_string("/home/thoni/Utveckling/c-xrefactory/src/extract.mock")),
           will_return("/home/thoni/Utveckling/c-xrefactory/src/extract.mock"));

    expect_string("19491f 1647180780p m1ia 73:/home/thoni/Utveckling/c-xrefactory/tests/test_single_xref_file/source.c ", true);
    expect(closeFile, when(file, is_equal_to(xfilesFilePointer)));

    expect(openFile, when(fileName, is_equal_to_string("/home/thoni/Utveckling/c-xrefactory/src/extract.mock")),
           will_return(sourceFilePointer));
    expect(checkFileModifiedTime);
    expect(closeFile, when(file, is_equal_to(sourceFilePointer)));

    normalScanReferenceFile("/XFiles");
}
