#include <cgreen/assertions.h>
#include <cgreen/cgreen.h>
#include <cgreen/constraint.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/mocks.h>

#include "cxfile.h"
#include "log.h"

#include "caching.mock"
#include "commons.mock"
#include "globals.mock"
#include "misc.mock"
#include "options.h"
#include "options.mock"

#include "characterreader.mock"
#include "classhierarchy.mock"
#include "cxref.mock"
#include "editor.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "reftab.mock"

Describe(CxFile);
BeforeEach(CxFile) {
    log_set_level(LOG_DEBUG); /* Set to LOG_TRACE if needed */

    options.mode = ServerMode;
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
static void expect_characters(char string[], bool eof) {
    int i;

    expect(skipWhiteSpace, will_return(string[0]));
    for (i = 1; string[i + 1] != '\0'; i++) {
        if (isspace(string[i])) {
            expect(getChar, will_return(' '));
            if (string[i + 1] != '\0')
                expect(skipWhiteSpace, will_return(string[++i]));
        } else {
            expect(getChar, will_return(string[i]));
        }
    }
    if (eof)
        expect(getChar, will_return(string[i]), will_set_contents_of_parameter(isAtEOF, &trueValue, sizeof(bool)));
    else
        expect(getChar, will_return(string[i]),
               will_set_contents_of_parameter(isAtEOF, &falseValue, sizeof(bool)));
}

Ensure(CxFile, can_do_normal_scan_with_only_a_single_file) {
    FILE *xfilesFilePointer = (FILE *)4654654645;

    char    *sourceFileName1  = "source1.c";
    int      sourceFileIndex1 = 44;
    FileItem fileItem1        = {.name = sourceFileName1};

    // log_set_level(LOG_TRACE);

    options.cxrefsLocation     = "./CXrefs";
    options.referenceFileCount = 1;

    expect(referenceFileCountMatches, will_return(true));

    expect(openFile, when(fileName, is_equal_to_string("./CXrefs/XFiles")), will_return(xfilesFilePointer));
    expect(initCharacterBuffer, when(file, is_equal_to(xfilesFilePointer)));

    /* Version marking always starts a file */
    expect_characters("34v", false);
    expect(skipCharacters, when(count, is_equal_to(33)));
    expect(getChar, will_return(' '));

    /* Generation setttings, file count, ... */
    expect_characters("21@mpfotulcsrhdeibnaAgk 10n 300000k ", false);

    /* First and only file, no other symbols */
    expect_characters("49976f 1646087914m 10: ", true);
    expect(getString, will_set_contents_of_parameter(string, sourceFileName1, strlen(sourceFileName1) + 1));

    expect(existsInFileTable, when(fileName, is_equal_to_string(sourceFileName1)), will_return(false));

    expect(addFileNameToFileTable, when(name, is_equal_to_string(sourceFileName1)), will_return(sourceFileIndex1));

    expect(getFileItem, when(fileIndex, is_equal_to(sourceFileIndex1)), will_return(&fileItem1));

    expect(closeFile, when(file, is_equal_to(xfilesFilePointer)));

    normalScanReferenceFile("/XFiles");
}
