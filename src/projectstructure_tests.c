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

static void given_include_resolves(char *includeName, char *dir, char *resolvedPath, int fileNum) {
    expect(directoryName_static, will_return(dir));
    expect(normalizeFileName_static, when(name, is_equal_to_string(includeName)),
           will_return(resolvedPath));
    expect(addFileNameToFileTable, when(fileName, is_equal_to_string(resolvedPath)),
           will_return(fileNum));
    expect(addFileAsIncludeReference, when(filenum, is_equal_to(fileNum)));
}


Describe(ProjectStructure);
BeforeEach(ProjectStructure) {
    log_set_level(LOG_ERROR);
}
AfterEach(ProjectStructure) {}

Ensure(ProjectStructure, empty_project_directory_adds_nothing) {
    given_project_files(NULL);
    never_expect(addFileNameToFileTable);
    never_expect(addFileAsIncludeReference);

    scanProjectForFilesAndIncludes("/empty/dir");
}

Ensure(ProjectStructure, single_cu_without_includes_adds_file_only) {
    given_project_files(newStringList("/project/source.c", NULL));
    given_cu_without_includes("/project/source.c");
    never_expect(addFileAsIncludeReference);

    scanProjectForFilesAndIncludes("/project");
}

Ensure(ProjectStructure, single_cu_with_one_include_adds_file_and_reference) {
    given_project_files(newStringList("/project/source.c", NULL));
    given_cu_with_content("/project/source.c", 1, "#include \"header.h\"\n");
    given_include_resolves("header.h", "/project", "/project/header.h", 2);

    scanProjectForFilesAndIncludes("/project");
}

Ensure(ProjectStructure, single_cu_with_multiple_includes) {
    given_project_files(newStringList("/project/source.c", NULL));
    given_cu_with_content("/project/source.c", 1,
                          "#include \"alpha.h\"\n#include \"beta.h\"\n#include \"gamma.h\"\n");
    given_include_resolves("alpha.h", "/project", "/project/alpha.h", 2);
    given_include_resolves("beta.h", "/project", "/project/beta.h", 3);
    given_include_resolves("gamma.h", "/project", "/project/gamma.h", 4);

    scanProjectForFilesAndIncludes("/project");
}

Ensure(ProjectStructure, multiple_cus_each_scanned_independently) {
    given_project_files(newStringList("/project/main.c",
                        newStringList("/project/util.c", NULL)));
    given_cu_with_content("/project/main.c", 1, "#include \"main.h\"\n");
    given_include_resolves("main.h", "/project", "/project/main.h", 2);
    given_cu_with_content("/project/util.c", 3, "#include \"util.h\"\n");
    given_include_resolves("util.h", "/project", "/project/util.h", 4);

    scanProjectForFilesAndIncludes("/project");
}

Ensure(ProjectStructure, include_split_across_buffer_boundary) {
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
    given_include_resolves("header.h", "/project", "/project/header.h", 2);

    scanProjectForFilesAndIncludes("/project");
}

Ensure(ProjectStructure, angle_bracket_includes_are_ignored) {
    given_project_files(newStringList("/project/source.c", NULL));
    given_cu_with_content("/project/source.c", 1, "#include <stdio.h>\n");
    never_expect(addFileAsIncludeReference);

    scanProjectForFilesAndIncludes("/project");
}

Ensure(ProjectStructure, indented_include_is_recognised) {
    given_project_files(newStringList("/project/source.c", NULL));
    given_cu_with_content("/project/source.c", 1, "  #include \"header.h\"\n");
    given_include_resolves("header.h", "/project", "/project/header.h", 2);

    scanProjectForFilesAndIncludes("/project");
}

Ensure(ProjectStructure, space_between_hash_and_include_is_recognised) {
    given_project_files(newStringList("/project/source.c", NULL));
    given_cu_with_content("/project/source.c", 1, "#  include \"header.h\"\n");
    given_include_resolves("header.h", "/project", "/project/header.h", 2);

    scanProjectForFilesAndIncludes("/project");
}

Ensure(ProjectStructure, only_compilation_units_are_scanned) {
    given_project_files(newStringList("/project/main.c",
                        newStringList("/project/parser.y",
                        newStringList("/project/header.h",
                        newStringList("/project/readme.txt", NULL)))));

    given_cu_without_includes("/project/main.c");
    given_cu_without_includes("/project/parser.y");
    expect(isCompilationUnit, will_return(false)); /* header.h */
    expect(isCompilationUnit, will_return(false)); /* readme.txt */

    scanProjectForFilesAndIncludes("/project");
}

/* markMissingFilesAsDeleted tests */

Ensure(ProjectStructure, no_files_discovered_and_empty_table_marks_nothing) {
    expect(getNextExistingFileNumber, will_return(-1));
    never_expect(markFileAsDeleted);

    markMissingFilesAsDeleted(NULL);
}

Ensure(ProjectStructure, file_in_table_but_not_discovered_is_marked_deleted) {
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
