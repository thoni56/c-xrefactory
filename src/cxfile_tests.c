#include <cgreen/assertions.h>
#include <cgreen/cgreen.h>
#include <cgreen/constraint.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/mocks.h>

#include <ctype.h>

#include "cxfile.h"
#include "log.h"

#include "browsermenu.mock"
#include "characterreader.mock"
#include "commons.mock"
#include "completion.mock"
#include "cxref.mock"
#include "editor.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "globals.mock"
#include "misc.mock"
#include "options.mock"
#include "reference.mock"
#include "referenceableitem.h"
#include "referenceableitemtable.mock"
#include "scope.h"
#include "session.h"
#include "session.mock"
#include "startup.mock"
#include "storage.h"
#include "visibility.h"

protected void normalScanCxFile(char *name);


Describe(CxFile);
BeforeEach(CxFile) {
    log_set_level(LOG_ERROR); /* Set to LOG_DEBUG if needed */

    options.mode = ServerMode;
}
AfterEach(CxFile) {}

Ensure(CxFile, will_return_no_hashnumber_for_single_cxfile) {
    options.cxFileCount = 1;
    assert_that(cxFileHashNumberForSymbol(NULL), is_equal_to(0));
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
    int      sourceFileNumber1 = 44;
    FileItem fileItem1        = {.name = sourceFileName1};

    // log_set_level(LOG_DEBUG);

    options.cxFileLocation     = "./CXrefs";
    options.cxFileCount = 1;

    expect(currentCxFileCountMatches, will_return(true));

    expect(openFile, when(fileName, is_equal_to_string("./CXrefs/XFiles")), will_return(xfilesFilePointer));

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

    expect(addFileNameToFileTable, when(fileName, is_equal_to_string(sourceFileName1)), will_return(sourceFileNumber1));

    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(sourceFileNumber1)), will_return(&fileItem1));

    expect(closeFile, when(file, is_equal_to(xfilesFilePointer)));

    normalScanCxFile("/XFiles");
}

SessionStackEntry *newEmptySessionStackEntry(void) {
    SessionStackEntry *entry  = malloc(sizeof(SessionStackEntry));
    *entry = (SessionStackEntry){
        .references      = NULL,
        .current         = NULL,
        .operation       = options.serverOperation,
        .callerPosition  = noPosition,
        .matches     = NULL,
        .hkSelectedSym   = NULL,
        .menuFilterLevel = 0,
        .refsFilterLevel = 0,
        .previous        = NULL};
    return entry;
}

Ensure(CxFile, can_check_references_for_referenceable_in_search) {
    ReferenceableItem item = makeReferenceableItem("item", TypeInt, StorageDefault, FileScope, VisibilityLocal, NO_FILE_NUMBER);
    Reference reference;
    SessionStackEntry *stackEntry = newEmptySessionStackEntry();

    options.olcxSearchString = "Sym";
    sessionData.searchingStack.top = stackEntry;

    expect(prettyPrintLinkName, will_set_contents_of_output_parameter(buffer, "SymbolName", 11));
    expect(containsWildcard, will_return(false));

    expect(prependToMatches, when(name, is_equal_to_string("SymbolName")));

    searchSymbolCheckReference(&item, &reference);
}
