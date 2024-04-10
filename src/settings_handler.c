#include "settings_handler.h"

#include "proto.h"
#include "fileio.h"

/*
  Handle settings in a project local .c-xrefrc file.

  The new settings file should, like settings for other modern
  "project" tool, like git, be placed at the project root and be
  checked into the repo.

  As opposed to the old ~/.c-xrefrc it only contains settings for the
  single project that it covers and it will therefore not have any
  "sections".  The implication of this is that the project name cannot
  be set and will be the name of the project root directory.

  It can still point to directories outside the project tree for
  including source for external components like libraries. TBD how to
  make these portable?
*/

#define SETTINGS_FILENAME ".c-xrefrc"

static void concatToPath(char path[], char string[]) {
    if (path[strlen(path) - 1] != '/')
        strcat(path, "/");
    strcat(path, string);
}

static void truncateSegmentsFromPath(char path[], int segmentCount) {
    char *last_slash;

    for (int i = segmentCount; i>0; i--) {
        last_slash = strrchr(path, '/');
        if (last_slash != NULL)
            *last_slash = '\0';
    }
}

/* Look up the directory path for a .c-xrefrc file, the directory for
 * it will be the project name. */
int find_project_settings(char *project_name) {
    char path[PATH_MAX];

    getCwd(path, PATH_MAX);
    do {
        concatToPath(path, SETTINGS_FILENAME);

        if (fileExists(path)) {
            truncateSegmentsFromPath(path, 1);
            strcpy(project_name, path);
            return RESULT_OK;
        }

        /* So, not found, go up to parent directory */
        truncateSegmentsFromPath(path, 2);
    } while (strlen(path) != 0);

    project_name[0] = '\0';
    return RESULT_NOT_FOUND;
}
