#include "commandlogger.h"

#include <string.h>
#include <stdlib.h>

#include "constants.h"
#include "fileio.h"
#include "options.h"


static FILE *commandsLogfile = NULL;

static char *removePattern(char *source, const char *pattern) {
    char* matchPosition;
    int lenPattern = strlen(pattern);
    char *modifiableCopy = strcpy(malloc(strlen(source)+1), source);

    while ((matchPosition = strstr(modifiableCopy, pattern)) != NULL) {
        // Move the rest of the string to overwrite the pattern
        memmove(matchPosition, matchPosition + lenPattern, strlen(matchPosition + lenPattern) + 1);
    }
    return modifiableCopy;
}

void logCommands(int argc, char *argv[]) {
    if (!options.commandlog)
        return;

    if (commandsLogfile == NULL)
        commandsLogfile = openFile(options.commandlog, "w");

    char cwd[MAX_FILE_NAME_SIZE];
    getCwd(cwd, sizeof(cwd));
    strcat(cwd, "/");

    for (int i=0; i<argc-1; i++) {
        if (argv[i] != NULL) {
            char *cleaned = removePattern(argv[i], cwd);
            writeFile(commandsLogfile, cleaned, strlen(cleaned), 1);
            writeFile(commandsLogfile, " ", 1, 1);
        }
    }
    char *cleaned = removePattern(argv[argc-1], cwd);
    writeFile(commandsLogfile, cleaned, strlen(cleaned), 1);
    writeFile(commandsLogfile, "\n", 1, 1);
}
