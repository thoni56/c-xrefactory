#include <cgreen/cgreen.h>
#include <cgreen/mocks.h>

#include "misc.h"

#include "memory.h"

#include "globals.mock"
#include "options.mock"
#include "caching.mock"
#include "filetable.mock"
#include "jsemact.mock"
#include "classfilereader.mock"
#include "cxref.mock"
#include "cxfile.mock"
#include "yylex.mock"
#include "commons.mock"
#include "editor.mock"
#include "ppc.mock"
#include "fileio.mock"


Describe(Misc);
BeforeEach(Misc) {}
AfterEach(Misc) {}


/* Protected */
extern char *concatDirectoryWithFileName(char *result, char *directoryName, char *packageFilename);

Ensure(Misc, can_concat_filename_with_directory) {
    char *dirname = "dirname";
    char *filename = "filename";
    char buffer[1000];
    char *result = concatDirectoryWithFileName(buffer, dirname, filename);

    /* TODO: FILE_PATH_SEPARATOR might be '\\' */
    assert_that(result, is_equal_to_string("dirname/filename"));
}
