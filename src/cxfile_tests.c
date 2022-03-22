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
}
AfterEach(CxFile) {}

Ensure(CxFile, can_run_empty_test) {
    options.referenceFileCount = 1;
    assert_that(cxFileHashNumber(NULL), is_equal_to(0));
}



/* The string should not start with a space, but it is assumed that
 * this string is delimited by some leading whitespace so it always
 * expects a skipWhitespace() first */
static void expect_string(char string[]) {
    expect(skipWhiteSpace, will_return(string[0]));
    for (int i=1; string[i] != '\0'; i++) {
        if (isspace(string[i])) {
            expect(getChar, will_return(' '));
            if (string[i+1] != '\0')
                expect(skipWhiteSpace, will_return(string[++i]));
          } else
            expect(getChar, will_return(string[i]));
    }
}


#if CGREEN_VERSION_MINOR < 5
#include "cgreen_capture_parameter.c"
#endif

xEnsure(CxFile, can_do_normal_scan) {
    FILE *filePointer = (FILE *)4654654645;
    CharacterBuffer *buffer;

    options.cxrefsLocation = "./CXrefs";
    options.referenceFileCount = 10;

    expect(openFile, when(fileName, is_equal_to_string("./CXrefs/XFiles")), will_return(filePointer));
    expect(initCharacterBuffer, when(file, is_equal_to(filePointer)),
           will_capture_parameter(characterBuffer, buffer));

    /* Version marking always starts a file */
    expect_string("34v");
    expect(skipCharacters, when(count, is_equal_to(33)));
    expect(getChar, will_return(' '));

    /* Generation setttings, file count, ... */
    expect_string("21@mpfotulcsrhdeibnaAgk 10n 300000k ");

    expect_string("49976f 1646087914m 53:/home/thoni/Utveckling/c-xrefactory/src/extract.mock");
    expect(normalizeFileName, when(name, is_equal_to_string("/home/thoni/Utveckling/c-xrefactory/src/extract.mock")),
           will_return("/home/thoni/Utveckling/c-xrefactory/src/extract.mock"));


    normalScanReferenceFile("/XFiles");
}
