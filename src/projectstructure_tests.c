#include "projectstructure.h"

/* Unittests */

#include <cgreen/cgreen.h>
#include <cgreen/constraint_syntax_helpers.h>

#include "log.h"

#include "commons.mock"
#include "fileio.mock"
#include "filetable.mock"
#include "misc.mock"
#include "yylex.mock"


static void given_project_files(StringList *files) {
    expect(listFilesInDirectory, will_return(files));
}

static void given_cu_with_content(char *path, int fileNum, char *content) {
    expect(isCompilationUnit, will_return(true));
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string(path)),
           will_return(fileNum));
    expect(openFile, when(fileName, is_equal_to_string(path)),
           will_return((FILE *)1));
    expect(readFile, will_set_contents_of_parameter(buffer, content, strlen(content)),
           will_return(strlen(content)));
    expect(readFile, will_return(0));
    expect(closeFile);
}

static void given_cu_without_includes(char *path) {
    expect(isCompilationUnit, will_return(true));
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string(path)));
    expect(openFile, will_return((FILE *)1));
    expect(readFile, will_return(0));
    expect(closeFile);
}

static void given_include_resolves(char *includeName, char *dir, char *resolvedPath,
                                   int includedFileNum, int includerFileNum) {
    expect(directoryName_static, will_return(dir));
    expect(normalizeFileName_static, when(name, is_equal_to_string(includeName)),
           will_return(resolvedPath));
    expect(fileExists, when(fullPath, is_equal_to_string(resolvedPath)),
           will_return(true));
    expect(existsInFileTable, when(fileName, is_equal_to_string(resolvedPath)),
           will_return(false));
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string(resolvedPath)),
           will_return(includedFileNum));
    expect(addIncludeReference, when(includedFileNumber, is_equal_to(includedFileNum)));
}


static void given_header_with_content(char *path, char *content) {
    expect(openFile, when(fileName, is_equal_to_string(path)),
           will_return((FILE *)1));
    expect(readFile, will_set_contents_of_parameter(buffer, content, strlen(content)),
           will_return(strlen(content)));
    expect(readFile, will_return(0));
    expect(closeFile);
}

static void given_header_without_includes(char *path) {
    (void)path; /* order of header scanning is not guaranteed */
    expect(openFile, will_return((FILE *)1));
    expect(readFile, will_return(0));
    expect(closeFile);
}

Describe(ProjectStructure);
BeforeEach(ProjectStructure) {
    log_set_level(LOG_ERROR);
}
AfterEach(ProjectStructure) {}

Ensure(ProjectStructure, adds_nothing_for_empty_project_directory) {
    given_project_files(NULL);
    never_expect(addFileNameToFileTable);
    never_expect(addIncludeReference);

    scanProjectForFilesAndIncludes("/empty/dir", NULL);
}

Ensure(ProjectStructure, adds_only_file_for_cu_without_includes) {
    given_project_files(newStringList("/project/source.c", NULL));
    given_cu_without_includes("/project/source.c");
    never_expect(addIncludeReference);

    scanProjectForFilesAndIncludes("/project", NULL);
}

Ensure(ProjectStructure, adds_file_and_reference_for_single_include) {
    given_project_files(newStringList("/project/source.c", NULL));
    given_cu_with_content("/project/source.c", 1, "#include \"header.h\"\n");
    given_include_resolves("header.h", "/project", "/project/header.h", 2, 1);
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/header.h")),
           will_return(2));
    given_header_without_includes("/project/header.h");

    scanProjectForFilesAndIncludes("/project", NULL);
}

Ensure(ProjectStructure, adds_references_for_multiple_includes) {
    given_project_files(newStringList("/project/source.c", NULL));
    given_cu_with_content("/project/source.c", 1,
                          "#include \"alpha.h\"\n#include \"beta.h\"\n#include \"gamma.h\"\n");
    given_include_resolves("alpha.h", "/project", "/project/alpha.h", 2, 1);
    given_include_resolves("beta.h", "/project", "/project/beta.h", 3, 1);
    given_include_resolves("gamma.h", "/project", "/project/gamma.h", 4, 1);
    /* Worklist is reversed: gamma, beta, alpha */
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/gamma.h")),
           will_return(4));
    given_header_without_includes("/project/gamma.h");
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/beta.h")),
           will_return(3));
    given_header_without_includes("/project/beta.h");
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/alpha.h")),
           will_return(2));
    given_header_without_includes("/project/alpha.h");

    scanProjectForFilesAndIncludes("/project", NULL);
}

Ensure(ProjectStructure, scans_multiple_cus_independently) {
    given_project_files(newStringList("/project/main.c",
                        newStringList("/project/util.c", NULL)));
    given_cu_with_content("/project/main.c", 1, "#include \"main.h\"\n");
    given_include_resolves("main.h", "/project", "/project/main.h", 2, 1);
    given_cu_with_content("/project/util.c", 3, "#include \"util.h\"\n");
    given_include_resolves("util.h", "/project", "/project/util.h", 4, 3);
    /* Worklist is reversed: util.h first, then main.h */
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/util.h")),
           will_return(4));
    given_header_without_includes("/project/util.h");
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/main.h")),
           will_return(2));
    given_header_without_includes("/project/main.h");

    scanProjectForFilesAndIncludes("/project", NULL);
}

Ensure(ProjectStructure, handles_include_split_across_buffer_boundary) {
    /* First read ends mid-line: "#include \"hea", second read has: "der.h"\n" */
    char *chunk1 = "#include \"hea";
    char *chunk2 = "der.h\"\n";

    given_project_files(newStringList("/project/source.c", NULL));
    expect(isCompilationUnit, will_return(true));
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/source.c")),
           will_return(1));
    expect(openFile, will_return((FILE *)1));
    expect(readFile, will_set_contents_of_parameter(buffer, chunk1, strlen(chunk1)),
           will_return(strlen(chunk1)));
    expect(readFile, will_set_contents_of_parameter(buffer, chunk2, strlen(chunk2)),
           will_return(strlen(chunk2)));
    expect(readFile, will_return(0));
    expect(closeFile);
    given_include_resolves("header.h", "/project", "/project/header.h", 2, 1);
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/header.h")),
           will_return(2));
    given_header_without_includes("/project/header.h");

    scanProjectForFilesAndIncludes("/project", NULL);
}

Ensure(ProjectStructure, ignores_angle_bracket_includes) {
    given_project_files(newStringList("/project/source.c", NULL));
    given_cu_with_content("/project/source.c", 1, "#include <stdio.h>\n");
    never_expect(addIncludeReference);

    scanProjectForFilesAndIncludes("/project", NULL);
}

Ensure(ProjectStructure, recognises_indented_include) {
    given_project_files(newStringList("/project/source.c", NULL));
    given_cu_with_content("/project/source.c", 1, "  #include \"header.h\"\n");
    given_include_resolves("header.h", "/project", "/project/header.h", 2, 1);
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/header.h")),
           will_return(2));
    given_header_without_includes("/project/header.h");

    scanProjectForFilesAndIncludes("/project", NULL);
}

Ensure(ProjectStructure, recognises_space_between_hash_and_include) {
    given_project_files(newStringList("/project/source.c", NULL));
    given_cu_with_content("/project/source.c", 1, "#  include \"header.h\"\n");
    given_include_resolves("header.h", "/project", "/project/header.h", 2, 1);
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/header.h")),
           will_return(2));
    given_header_without_includes("/project/header.h");

    scanProjectForFilesAndIncludes("/project", NULL);
}

Ensure(ProjectStructure, scans_only_compilation_units) {
    given_project_files(newStringList("/project/main.c",
                        newStringList("/project/parser.y",
                        newStringList("/project/header.h",
                        newStringList("/project/readme.txt", NULL)))));

    given_cu_without_includes("/project/main.c");
    given_cu_without_includes("/project/parser.y");
    expect(isCompilationUnit, will_return(false)); /* header.h */
    expect(isCompilationUnit, will_return(false)); /* readme.txt */

    scanProjectForFilesAndIncludes("/project", NULL);
}

/* Transitive include scanning tests */

Ensure(ProjectStructure, scans_header_includes_transitively) {
    /* source.c → header.h → deep.h */
    given_project_files(newStringList("/project/source.c", NULL));
    given_cu_with_content("/project/source.c", 1, "#include \"header.h\"\n");
    given_include_resolves("header.h", "/project", "/project/header.h", 2, 1);

    /* Phase 2: header.h popped from worklist */
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/header.h")),
           will_return(2));
    given_header_with_content("/project/header.h", "#include \"deep.h\"\n");
    given_include_resolves("deep.h", "/project", "/project/deep.h", 3, 2);

    /* Phase 2: deep.h popped from worklist */
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/deep.h")),
           will_return(3));
    given_header_with_content("/project/deep.h", "/* no includes */\n");

    scanProjectForFilesAndIncludes("/project", NULL);
}

Ensure(ProjectStructure, scans_transitive_includes_to_full_depth) {
    /* source.c → a.h → b.h → c.h */
    given_project_files(newStringList("/project/source.c", NULL));
    given_cu_with_content("/project/source.c", 1, "#include \"a.h\"\n");
    given_include_resolves("a.h", "/project", "/project/a.h", 2, 1);

    /* Phase 2: a.h */
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/a.h")),
           will_return(2));
    given_header_with_content("/project/a.h", "#include \"b.h\"\n");
    given_include_resolves("b.h", "/project", "/project/b.h", 3, 2);

    /* Phase 2: b.h */
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/b.h")),
           will_return(3));
    given_header_with_content("/project/b.h", "#include \"c.h\"\n");
    given_include_resolves("c.h", "/project", "/project/c.h", 4, 3);

    /* Phase 2: c.h */
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/c.h")),
           will_return(4));
    given_header_without_includes("/project/c.h");

    scanProjectForFilesAndIncludes("/project", NULL);
}

Ensure(ProjectStructure, skips_already_known_header_in_transitive_scan) {
    /* CU1 → h1 → shared.h, CU2 → shared.h
       shared.h should be scanned only once.
       Worklist order after CU phase: shared.h (from b.c, prepended last), h1.h */
    given_project_files(newStringList("/project/a.c",
                        newStringList("/project/b.c", NULL)));

    given_cu_with_content("/project/a.c", 1, "#include \"h1.h\"\n");
    given_include_resolves("h1.h", "/project", "/project/h1.h", 2, 1);

    given_cu_with_content("/project/b.c", 3, "#include \"shared.h\"\n");
    given_include_resolves("shared.h", "/project", "/project/shared.h", 4, 3);

    /* Phase 2: shared.h first (no includes) */
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/shared.h")),
           will_return(4));
    given_header_without_includes("/project/shared.h");

    /* Phase 2: h1.h includes shared.h, but shared.h is already known */
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/h1.h")),
           will_return(2));
    given_header_with_content("/project/h1.h", "#include \"shared.h\"\n");
    expect(directoryName_static, will_return("/project"));
    expect(normalizeFileName_static, when(name, is_equal_to_string("shared.h")),
           will_return("/project/shared.h"));
    expect(fileExists, when(fullPath, is_equal_to_string("/project/shared.h")),
           will_return(true));
    expect(existsInFileTable, when(fileName, is_equal_to_string("/project/shared.h")),
           will_return(true));
    /* Still records the include reference, but does NOT add to worklist */
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/shared.h")),
           will_return(4));
    expect(addIncludeReference, when(includedFileNumber, is_equal_to(4)));

    scanProjectForFilesAndIncludes("/project", NULL);
}

Ensure(ProjectStructure, records_include_but_skips_scanning_missing_file) {
    /* source.c → missing.h, but missing.h doesn't exist on disk */
    given_project_files(newStringList("/project/source.c", NULL));
    given_cu_with_content("/project/source.c", 1, "#include \"missing.h\"\n");

    /* Include resolution: try includer's dir — not found, no includeDirs, fallback */
    expect(directoryName_static, will_return("/project"));
    expect(normalizeFileName_static, when(name, is_equal_to_string("missing.h")),
           will_return("/project/missing.h"));
    expect(fileExists, when(fullPath, is_equal_to_string("/project/missing.h")),
           will_return(false));
    /* Fallback: return includer-relative path */
    expect(normalizeFileName_static, when(name, is_equal_to_string("missing.h")),
           will_return("/project/missing.h"));
    expect(directoryName_static, will_return("/project"));

    /* File gets registered despite not existing */
    expect(existsInFileTable, when(fileName, is_equal_to_string("/project/missing.h")),
           will_return(false));
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/missing.h")),
           will_return(2));
    expect(addIncludeReference, when(includedFileNumber, is_equal_to(2)));

    /* Phase 2: missing.h popped — openFile fails */
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/missing.h")),
           will_return(2));
    expect(openFile, will_return(NULL));
    /* No readFile or closeFile — file couldn't be opened */

    scanProjectForFilesAndIncludes("/project", NULL);
}

Ensure(ProjectStructure, handles_circular_includes_without_infinite_loop) {
    /* source.c → a.h → b.h → a.h (cycle) */
    given_project_files(newStringList("/project/source.c", NULL));
    given_cu_with_content("/project/source.c", 1, "#include \"a.h\"\n");
    given_include_resolves("a.h", "/project", "/project/a.h", 2, 1);

    /* Phase 2: a.h */
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/a.h")),
           will_return(2));
    given_header_with_content("/project/a.h", "#include \"b.h\"\n");
    given_include_resolves("b.h", "/project", "/project/b.h", 3, 2);

    /* Phase 2: b.h includes a.h, but a.h is already known — cycle broken */
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/b.h")),
           will_return(3));
    given_header_with_content("/project/b.h", "#include \"a.h\"\n");
    expect(directoryName_static, will_return("/project"));
    expect(normalizeFileName_static, when(name, is_equal_to_string("a.h")),
           will_return("/project/a.h"));
    expect(fileExists, when(fullPath, is_equal_to_string("/project/a.h")),
           will_return(true));
    expect(existsInFileTable, when(fileName, is_equal_to_string("/project/a.h")),
           will_return(true));
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/a.h")),
           will_return(2));
    expect(addIncludeReference, when(includedFileNumber, is_equal_to(2)));
    /* No further scanning — worklist is empty */

    scanProjectForFilesAndIncludes("/project", NULL);
}

Ensure(ProjectStructure, follows_includes_regardless_of_file_extension) {
    /* source.c → table.tc → table.th — scanner follows any quoted include */
    given_project_files(newStringList("/project/source.c", NULL));
    given_cu_with_content("/project/source.c", 1, "#include \"table.tc\"\n");
    given_include_resolves("table.tc", "/project", "/project/table.tc", 2, 1);

    /* Phase 2: table.tc */
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/table.tc")),
           will_return(2));
    given_header_with_content("/project/table.tc", "#include \"table.th\"\n");
    given_include_resolves("table.th", "/project", "/project/table.th", 3, 2);

    /* Phase 2: table.th */
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/table.th")),
           will_return(3));
    given_header_without_includes("/project/table.th");

    scanProjectForFilesAndIncludes("/project", NULL);
}

/* Include path resolution tests */

Ensure(ProjectStructure, prefers_includer_dir_over_includeDirs) {
    /* source.c includes "util.h", found in both /project/ and /project/lib/
       — includer's directory should win */
    StringList *includeDirs = newStringList("/project/lib", NULL);

    given_project_files(newStringList("/project/source.c", NULL));
    given_cu_with_content("/project/source.c", 1, "#include \"util.h\"\n");

    /* Include resolution: try includer's dir first — found */
    expect(directoryName_static, will_return("/project"));
    expect(normalizeFileName_static, when(name, is_equal_to_string("util.h")),
           will_return("/project/util.h"));
    expect(fileExists, when(fullPath, is_equal_to_string("/project/util.h")),
           will_return(true));

    /* Resolved to includer's dir, NOT /project/lib/util.h */
    expect(existsInFileTable, when(fileName, is_equal_to_string("/project/util.h")),
           will_return(false));
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/util.h")),
           will_return(2));
    expect(addIncludeReference, when(includedFileNumber, is_equal_to(2)));

    /* Phase 2: transitive scan of the discovered header */
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/util.h")),
           will_return(2));
    given_header_without_includes("/project/util.h");

    scanProjectForFilesAndIncludes("/project", includeDirs);
}

Ensure(ProjectStructure, resolves_include_via_includeDirs_when_not_in_includer_dir) {
    /* source.c includes "util.h", not found in /project/, found in /project/lib/ */
    StringList *includeDirs = newStringList("/project/lib", NULL);

    given_project_files(newStringList("/project/source.c", NULL));

    /* CU setup */
    expect(isCompilationUnit, will_return(true));
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/source.c")),
           will_return(1));
    expect(openFile, when(fileName, is_equal_to_string("/project/source.c")),
           will_return((FILE *)1));
    char *content = "#include \"util.h\"\n";
    expect(readFile, will_set_contents_of_parameter(buffer, content, strlen(content)),
           will_return(strlen(content)));
    expect(readFile, will_return(0));
    expect(closeFile);

    /* Include resolution: try includer's dir first — not found */
    expect(directoryName_static, will_return("/project"));
    expect(normalizeFileName_static, when(name, is_equal_to_string("util.h")),
           will_return("/project/util.h"));
    expect(fileExists, when(fullPath, is_equal_to_string("/project/util.h")),
           will_return(false));

    /* Try first includeDirs entry — found */
    expect(normalizeFileName_static, when(name, is_equal_to_string("util.h")),
           will_return("/project/lib/util.h"));
    expect(fileExists, when(fullPath, is_equal_to_string("/project/lib/util.h")),
           will_return(true));

    /* File added with the resolved path */
    expect(existsInFileTable, when(fileName, is_equal_to_string("/project/lib/util.h")),
           will_return(false));
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/lib/util.h")),
           will_return(2));
    expect(addIncludeReference, when(includedFileNumber, is_equal_to(2)));

    /* Phase 2: transitive scan of the discovered header */
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string("/project/lib/util.h")),
           will_return(2));
    given_header_without_includes("/project/lib/util.h");

    scanProjectForFilesAndIncludes("/project", includeDirs);
}

/* Return value tests */

static bool listContains(StringList *list, const char *name) {
    for (StringList *s = list; s != NULL; s = s->next)
        if (strcmp(s->string, name) == 0)
            return true;
    return false;
}

Ensure(ProjectStructure, returns_discovered_compilation_units) {
    given_project_files(newStringList("/project/main.c",
                        newStringList("/project/util.c",
                        newStringList("/project/header.h",
                        newStringList("/project/readme.txt", NULL)))));

    given_cu_without_includes("/project/main.c");
    given_cu_without_includes("/project/util.c");
    expect(isCompilationUnit, will_return(false)); /* header.h */
    expect(isCompilationUnit, will_return(false)); /* readme.txt */

    StringList *discovered = scanProjectForFilesAndIncludes("/project", NULL);

    assert_that(discovered, is_non_null);
    assert_that(listContains(discovered, "/project/main.c"), is_true);
    assert_that(listContains(discovered, "/project/util.c"), is_true);
    assert_that(listContains(discovered, "/project/header.h"), is_false);
    assert_that(listContains(discovered, "/project/readme.txt"), is_false);
    freeStringList(discovered);
}

Ensure(ProjectStructure, returns_null_for_empty_project) {
    given_project_files(NULL);
    never_expect(addFileNameToFileTable);

    StringList *discovered = scanProjectForFilesAndIncludes("/empty/dir", NULL);

    assert_that(discovered, is_null);
}

/* markMissingFilesAsDeleted tests */

Ensure(ProjectStructure, marks_nothing_when_no_files_and_empty_table) {
    expect(getNextExistingFileNumber, will_return(-1));
    never_expect(markFileAsDeleted);

    markMissingFilesAsDeleted(NULL);
}

Ensure(ProjectStructure, marks_undiscovered_file_as_deleted) {
    FileItem oldFile = { .name = "/project/gone.c" };

    expect(getNextExistingFileNumber, will_return(1));
    expect(getFileItemWithFileNumber, when(fileNumber, is_equal_to(1)),
           will_return(&oldFile));
    expect(markFileAsDeleted, when(fileNumber, is_equal_to(1)));
    expect(getNextExistingFileNumber, will_return(-1));

    markMissingFilesAsDeleted(NULL);
}

Ensure(ProjectStructure, marks_only_missing_files_in_mixed_table) {
    /* File table has: main.c(1), util.c(2), gone.c(3), old.c(4) */
    /* Discovered:     main.c, util.c                              */
    /* Expected:       gone.c and old.c marked as deleted          */
    StringList *discovered = newStringList("/project/main.c",
                             newStringList("/project/util.c", NULL));

    FileItem mainFile = { .name = "/project/main.c" };
    FileItem utilFile = { .name = "/project/util.c" };
    FileItem goneFile = { .name = "/project/gone.c" };
    FileItem oldFile  = { .name = "/project/old.c" };

    expect(getNextExistingFileNumber, will_return(1));
    expect(getFileItemWithFileNumber, will_return(&mainFile));

    expect(getNextExistingFileNumber, will_return(2));
    expect(getFileItemWithFileNumber, will_return(&utilFile));

    expect(getNextExistingFileNumber, will_return(3));
    expect(getFileItemWithFileNumber, will_return(&goneFile));
    expect(markFileAsDeleted, when(fileNumber, is_equal_to(3)));

    expect(getNextExistingFileNumber, will_return(4));
    expect(getFileItemWithFileNumber, will_return(&oldFile));
    expect(markFileAsDeleted, when(fileNumber, is_equal_to(4)));

    expect(getNextExistingFileNumber, will_return(-1));

    markMissingFilesAsDeleted(discovered);
}
