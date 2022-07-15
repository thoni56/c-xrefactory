#include "xref.h"

#include "commons.h"
#include "globals.h"
#include "head.h"
#include "log.h"
#include "main.h"
#include "options.h"
#include "ppc.h"
#include "filetable.h"


void xref(int argc, char **argv) {
    ENTER();
    mainOpenOutputFile(options.outputFileName);
    editorLoadAllOpenedBufferFiles();

    mainCallXref(argc, argv);
    closeMainOutputFile();
    if (options.xref2) {
        ppcSynchronize();
    }
    //& fprintf(dumpOut, "\n\nDUMP\n\n"); fflush(dumpOut);
    //& mapOverReferenceTable(symbolRefItemDump);
    LEAVE();
}
