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

/* Look up the directory path for a .c-xrefrc file, the directory for
 * it will be the project name. */
int find_project_settings(char *project_name) {
    char *cwd = getCwd(NULL, 0);

    char cwdBuffer[PATH_MAX];
    strcpy(cwdBuffer, cwd);
    if (cwdBuffer[strlen(cwdBuffer)-1] != '/')
        strcat(cwdBuffer, "/.c-xrefrc");
    else
        strcat(cwdBuffer, ".c-xrefrc");

    if (fileExists(cwdBuffer)) {
        strcpy(project_name, cwd);
        return RESULT_OK;
    } else
        return RESULT_NOT_FOUND;
}
