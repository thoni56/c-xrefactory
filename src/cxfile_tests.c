#include <cgreen/assertions.h>
#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/mocks.h>

#include "characterreader.h"
#include "cxfile.h"
#include "log.h"

#include "globals.mock"
#include "misc.mock"
#include "utils.mock"
#include "options.mock"
#include "commons.mock"

#include "olcxtab.mock"
#include "cxref.mock"
#include "reftab.mock"
#include "filetable.mock"
#include "yylex.mock"           /* For addFiletabItem */
#include "editor.mock"
#include "characterreader.mock"
#include "classhierarchy.mock"
#include "fileio.mock"


Describe(CxFile);
BeforeEach(CxFile) {
    log_set_level(LOG_DEBUG); /* Set to LOG_TRACE if needed */
}
AfterEach(CxFile) {}

Ensure(CxFile, can_run_empty_test) {
    options.referenceFileCount = 1;
    assert_that(cxFileHashNumber(NULL), is_equal_to(0));
}


#include "cgreen_capture_parameter.c"


xEnsure(CxFile, can_do_normal_scan) {
    FILE *filePointer = (FILE *)4654654645;
    CharacterBuffer *buffer;

    cgreen_mocks_are(learning_mocks);
    options.cxrefsLocation = "./CXrefs";

    expect(openFile, when(fileName, is_equal_to_string("./CXrefs/XFiles")), will_return(filePointer));
    expect(initCharacterBuffer, when(file, is_equal_to(filePointer)),
           will_capture_parameter(characterBuffer, buffer));

    /* Version marking always starts a file */
    expect(skipWhiteSpace, will_return('3'));
    expect(getChar, will_return('4'));
    expect(getChar, will_return('v'));
    expect(getChar, will_return(' '));
    expect(skipCharacters, when(count, is_equal_to(33)));

    /* Generation setttings, file count, ... */
    expect(getChar, will_return('2'));
    expect(getChar, will_return('1'));
    expect(getChar, will_return('@'));
    expect(getChar, will_return(' '));

    normalScanReferenceFile("/XFiles");
}
