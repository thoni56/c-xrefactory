#include "parsing.h"

#include "editorbuffer.h"
#include "editormarker.h"
#include "filetable.h"
#include "globals.h"
#include "options.h"
#include "position.h"


ParseConfig createParseConfigFromOptions(void) {
    ParseConfig config = {
        .includeDirs = options.includeDirs,
        .defines = options.definitionStrings,
        .strictAnsi = options.strictAnsi
    };
    return config;
}

/* Bridge to existing implementation - temporarily uses external parseBufferUsingServer */
extern void parseBufferUsingServer(char *project, EditorMarker *point, EditorMarker *mark,
                                   char *pushOption, char *pushOption2);

FunctionBoundariesResult parseToGetFunctionBoundaries(
    EditorBuffer *buffer,
    ParseConfig  *config,
    Position      cursorPos
) {
    FunctionBoundariesResult result = {
        .found = false,
        .functionBegin = noPosition,
        .functionEnd = noPosition
    };

    /* Bridge implementation: Use existing infrastructure temporarily */
    (void)config; /* TODO: Apply config before parsing */

    /* Clear previous results */
    parsedPositions[IPP_FUNCTION_BEGIN] = noPosition;
    parsedPositions[IPP_FUNCTION_END] = noPosition;

    /* Create marker at cursor position for existing API */
    EditorMarker *marker = newEditorMarkerForPosition(cursorPos);
    if (marker->buffer != buffer) {
        /* Position doesn't match buffer - error */
        freeEditorMarker(marker);
        return result;
    }

    /* Call existing parsing infrastructure */
    parseBufferUsingServer(options.project, marker, NULL, "-olcxgetfunctionbounds", NULL);

    /* Extract results from global state */
    if (parsedPositions[IPP_FUNCTION_BEGIN].file != NO_FILE_NUMBER &&
        parsedPositions[IPP_FUNCTION_END].file != NO_FILE_NUMBER) {
        result.found = true;
        result.functionBegin = parsedPositions[IPP_FUNCTION_BEGIN];
        result.functionEnd = parsedPositions[IPP_FUNCTION_END];
    }

    freeEditorMarker(marker);
    return result;
}

    return result;
}
