#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>
#include <cgreen/mocks.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "lsp_utils.h"

Describe(LspUtils);
BeforeEach(LspUtils) {
    log_set_level(LOG_ERROR);
}
AfterEach(LspUtils) {}

Ensure(LspUtils, uriToFilePath_converts_file_uri) {
    char *result = uriToFilePath("file:///path/to/file.c");
    assert_that(result, is_equal_to_string("/path/to/file.c"));
}

Ensure(LspUtils, uriToFilePath_handles_non_file_uri) {
    char *result = uriToFilePath("/path/to/file.c");
    assert_that(result, is_equal_to_string("/path/to/file.c"));
}

Ensure(LspUtils, lspPositionToByteOffset_converts_coordinates) {
    /* Create a temporary test file with known content */
    char testFile[] = "/tmp/lsp_test_XXXXXX";
    int fd = mkstemp(testFile);
    assert_that(fd, is_not_equal_to(-1));
    
    const char *content = "line 0\nline 1\nline 2\n";
    write(fd, content, strlen(content));
    close(fd);
    
    /* Test conversion: line 1, character 3 should be at byte offset 10 
       (6 chars in line 0 + 1 newline + 3 chars in line 1) */
    int offset = lspPositionToByteOffset(testFile, 1, 3);
    assert_that(offset, is_equal_to(10));
    
    /* Clean up */
    unlink(testFile);
}

Ensure(LspUtils, byteOffsetToLspPosition_converts_coordinates) {
    /* Create a temporary test file with known content */
    char testFile[] = "/tmp/lsp_test_XXXXXX";
    int fd = mkstemp(testFile);
    assert_that(fd, is_not_equal_to(-1));
    
    const char *content = "line 0\nline 1\nline 2\n";
    write(fd, content, strlen(content));
    close(fd);
    
    /* Test conversion: byte offset 10 should be line 1, character 3 */
    int line, character;
    byteOffsetToLspPosition(testFile, 10, &line, &character);
    assert_that(line, is_equal_to(1));
    assert_that(character, is_equal_to(3));
    
    /* Clean up */
    unlink(testFile);
}