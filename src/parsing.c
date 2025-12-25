#include "parsing.h"

#include "editorbuffer.h"
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

FunctionBoundariesResult parseToGetFunctionBoundaries(
    EditorBuffer *buffer,
    ParseConfig  *config,
    Position      cursorPos
) {
    /* Stub implementation - always returns "not found" */
    (void)buffer;
    (void)config;
    (void)cursorPos;

    FunctionBoundariesResult result = {
        .found = false,
        .functionBegin = noPosition,
        .functionEnd = noPosition
    };
    return result;
}
