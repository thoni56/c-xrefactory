#include "commandlogger.h"

#include "fileio.h"

#include <string.h>

#include "options.h"


static FILE *commandsLogfile = NULL;

void logCommands(int argc, char *argv[]) {
    if (!options.commandlog)
        return;

    if (commandsLogfile == NULL)
        commandsLogfile = openFile("/tmp/c-xref-command-log", "w");

    for (int i=0; i<argc-1; i++) {
        if (argv[i] != NULL) {
            writeFile(argv[i], strlen(argv[i]), 1, commandsLogfile);
            writeFile(" ", 1, 1, commandsLogfile);
        }
    }
    writeFile(argv[argc-1], strlen(argv[argc-1]), 1, commandsLogfile);
    writeFile("\n", 1, 1, commandsLogfile);
}