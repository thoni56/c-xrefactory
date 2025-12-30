#include "parsing.h"

#include "editorbuffer.h"
#include "editormarker.h"
#include "filetable.h"
#include "globals.h"
#include "options.h"
#include "position.h"


/* Global parsing configuration - set before parsing starts */
ParsingConfig parsingConfig;


ParseConfig createParseConfigFromOptions(void) {
    ParseConfig config = {
        .includeDirs = options.includeDirs,
        .defines = options.definitionStrings,
        .strictAnsi = options.strictAnsi
    };
    return config;
}

ParserOperation getParserOperation(ServerOperation serverOp) {
    switch (serverOp) {
        case OLO_GET_FUNCTION_BOUNDS:
            return PARSER_OP_GET_FUNCTION_BOUNDS;
        case OLO_SET_MOVE_TARGET:
            return PARSER_OP_VALIDATE_MOVE_TARGET;
        case OLO_EXTRACT:
            return PARSER_OP_EXTRACT;
        case OLO_GOTO_PARAM_NAME:
        case OLO_GET_PARAM_COORDINATES:
            return PARSER_OP_TRACK_PARAMETERS;
        case OLO_COMPLETION:
            return PARSER_OP_COMPLETION;
        default:
            return PARSER_OP_NORMAL;
    }
}

bool needsReferenceAtCursor(ParserOperation op) {
    return op != PARSER_OP_EXTRACT
        && op != PARSER_OP_COMPLETION
        && op != PARSER_OP_VALIDATE_MOVE_TARGET;
}

bool allowsDuplicateReferences(ParserOperation op) {
    return op == PARSER_OP_EXTRACT;
}

/* Bridge to existing implementation - temporarily uses external parseBufferUsingServer */
extern void parseBufferUsingServer(char *project, EditorMarker *point, EditorMarker *mark,
                                   char *pushOption, char *pushOption2);

static FunctionBoundariesResult parseToGetFunctionBoundaries(
    EditorBuffer *buffer,
    ParseConfig  *config,
    Position      cursorPos
) {
    FunctionBoundariesResult result = {
        .found = false,
        .functionBegin = noPosition,
        .functionEnd = noPosition
    };

    /* Set up parsing configuration for subsystem */
    parsingConfig.includeDirs = config->includeDirs;
    parsingConfig.defines = config->defines;
    parsingConfig.strictAnsi = config->strictAnsi;
    parsingConfig.operation = PARSER_OP_GET_FUNCTION_BOUNDS;
    parsingConfig.cursorPosition = cursorPos;

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

    /* Bridge: Still calls old infrastructure via magic string */
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

static MoveTargetValidationResult parseToValidateMoveTarget(
    EditorBuffer *buffer,
    ParseConfig  *config,
    Position      targetPosition
) {
    MoveTargetValidationResult result = {
        .valid = false
    };

    /* Set up parsing configuration for subsystem */
    parsingConfig.includeDirs = config->includeDirs;
    parsingConfig.defines = config->defines;
    parsingConfig.strictAnsi = config->strictAnsi;
    parsingConfig.operation = PARSER_OP_VALIDATE_MOVE_TARGET;
    parsingConfig.cursorPosition = targetPosition;

    /* Clear previous results */
    parsedInfo.moveTargetAccepted = false;

    /* Create marker at target position for existing API */
    EditorMarker *marker = newEditorMarkerForPosition(targetPosition);
    if (marker->buffer != buffer) {
        freeEditorMarker(marker);
        return result;
    }

    /* Bridge: Still calls old infrastructure via magic string */
    parseBufferUsingServer(options.project, marker, NULL, "-olcxmovetarget", NULL);

    /* Extract result from global state */
    result.valid = parsedInfo.moveTargetAccepted;

    freeEditorMarker(marker);
    return result;
}

bool isValidMoveTarget(EditorMarker *target) {
    ParseConfig config = createParseConfigFromOptions();
    Position targetPos = makePositionFromEditorMarker(target);
    MoveTargetValidationResult result = parseToValidateMoveTarget(target->buffer, &config, targetPos);
    return result.valid;
}

FunctionBoundariesResult getFunctionBoundaries(EditorMarker *marker) {
    ParseConfig config = createParseConfigFromOptions();
    Position cursorPos = makePositionFromEditorMarker(marker);
    return parseToGetFunctionBoundaries(marker->buffer, &config, cursorPos);
}
