#include "parsing.h"

#include "editorbuffer.h"
#include "editormarker.h"
#include "filetable.h"
#include "globals.h"
#include "options.h"
#include "position.h"


/* Global parsing configuration - set before parsing starts */
ParsingConfig parsingConfig;


ParserOperation getParserOperation(ServerOperation serverOp) {
    switch (serverOp) {
        case OLO_GET_FUNCTION_BOUNDS:
            return PARSE_TO_GET_FUNCTION_BOUNDS;
        case OLO_SET_MOVE_TARGET:
            return PARSE_TO_VALIDATE_MOVE_TARGET;
        case OLO_EXTRACT:
            return PARSE_TO_EXTRACT;
        case OLO_GOTO_PARAM_NAME:
        case OLO_GET_PARAM_COORDINATES:
            return PARSE_TO_TRACK_PARAMETERS;
        case OLO_COMPLETION:
            return PARSE_TO_COMPLETE;
        default:
            return PARSE_TO_CREATE_REFERENCES;
    }
}

void syncParsingConfigFromOptions(Options options) {
    parsingConfig.includeDirs = options.includeDirs;
    parsingConfig.defines = options.definitionStrings;
    parsingConfig.strictAnsi = options.strictAnsi;
    parsingConfig.operation = getParserOperation(options.serverOperation);
    parsingConfig.cursorOffset = options.cursorOffset;
    parsingConfig.markOffset = options.markOffset;
    parsingConfig.extractMode = options.extractMode;
    parsingConfig.targetParameterIndex = options.olcxGotoVal;
}

bool needsReferenceAtCursor(ParserOperation op) {
    return op != PARSE_TO_EXTRACT
        && op != PARSE_TO_COMPLETE
        && op != PARSE_TO_VALIDATE_MOVE_TARGET;
}

bool allowsDuplicateReferences(ParserOperation op) {
    return op == PARSE_TO_EXTRACT;
}

/* Bridge to existing implementation - temporarily uses external parseBufferUsingServer */
extern void parseBufferUsingServer(char *project, EditorMarker *point, EditorMarker *mark,
                                   char *pushOption, char *pushOption2);

bool isValidMoveTarget(EditorMarker *target) {
    MoveTargetValidationResult result = {
        .valid = false
    };

    /* Set up parsing configuration */
    syncParsingConfigFromOptions(options);
    parsingConfig.operation = PARSE_TO_VALIDATE_MOVE_TARGET;
    parsingConfig.cursorPosition = makePositionFromEditorMarker(target);

    /* Clear previous results */
    parsedInfo.moveTargetAccepted = false;

    /* Bridge: Still calls old infrastructure via magic string */
    parseBufferUsingServer(options.project, target, NULL, "-olcxmovetarget", NULL);

    /* Extract result from global state */
    result.valid = parsedInfo.moveTargetAccepted;

    return result.valid;
}

FunctionBoundariesResult getFunctionBoundaries(EditorMarker *marker) {
    FunctionBoundariesResult result = {
        .found = false,
        .functionBegin = noPosition,
        .functionEnd = noPosition
    };

    /* Set up parsing configuration */
    syncParsingConfigFromOptions(options);
    parsingConfig.operation = PARSE_TO_GET_FUNCTION_BOUNDS;
    parsingConfig.cursorPosition = makePositionFromEditorMarker(marker);

    /* Clear previous results */
    parsedPositions[IPP_FUNCTION_BEGIN] = noPosition;
    parsedPositions[IPP_FUNCTION_END] = noPosition;

    /* Bridge: Still calls old infrastructure via magic string */
    parseBufferUsingServer(options.project, marker, NULL, "-olcxgetfunctionbounds", NULL);

    /* Extract results from global state */
    if (parsedPositions[IPP_FUNCTION_BEGIN].file != NO_FILE_NUMBER &&
        parsedPositions[IPP_FUNCTION_END].file != NO_FILE_NUMBER) {
        result.found = true;
        result.functionBegin = parsedPositions[IPP_FUNCTION_BEGIN];
        result.functionEnd = parsedPositions[IPP_FUNCTION_END];
    }

    return result;
}
