#include <cgreen/cgreen.h>

#include <string.h>

#include "fileio.h"
#include "log.h"


Describe(Fileio);
BeforeEach(Fileio) {
    log_set_level(LOG_ERROR);
}
AfterEach(Fileio) {}


static bool listContains(StringList *list, const char *path) {
    for (StringList *l = list; l != NULL; l = l->next)
        if (strcmp(l->string, path) == 0)
            return true;
    return false;
}

Ensure(Fileio, can_see_if_exists) {
    assert_that(exists("."));
    assert_that(exists("some file that do not exist"), is_false);
}

Ensure(Fileio, can_create_directory_even_if_exists) {
    createDirectory(".");
}

#define FILEIO_TMP "/tmp/c-xref-fileio-test"

/* listFilesInDirectory is a true fileio primitive: it lists a single
 * directory level (files AND subdirectories) and does NOT recurse, prune, or
 * otherwise apply project policy. The recursive, prune-aware walk belongs to
 * the layer above (projectstructure). The temp tree lives in /tmp so the
 * source-tree test watcher is not retriggered by the created .c files. */
Ensure(Fileio, lists_entries_of_directory_without_recursing_into_subdirectories) {
    /* layout:
     *   <tmp>/top.c
     *   <tmp>/sub/nested.c
     */
    recursivelyDeleteDirectory(FILEIO_TMP);   /* clean any leftover from a prior run */
    createDirectory(FILEIO_TMP);
    createDirectory(FILEIO_TMP "/sub");
    closeFile(openFile(FILEIO_TMP "/top.c", "w"));
    closeFile(openFile(FILEIO_TMP "/sub/nested.c", "w"));

    StringList *entries = listFilesInDirectory(FILEIO_TMP);

    /* a file directly in the directory is returned ... */
    assert_that(listContains(entries, FILEIO_TMP "/top.c"), is_true);
    /* ... a subdirectory is returned as an entry (so the walker above can
     * decide whether to descend) ... */
    assert_that(listContains(entries, FILEIO_TMP "/sub"), is_true);
    /* ... but files inside a subdirectory are NOT — no recursion. */
    assert_that(listContains(entries, FILEIO_TMP "/sub/nested.c"), is_false);

    freeStringList(entries);
    recursivelyDeleteDirectory(FILEIO_TMP);
}
