#include "parsing.h"

#include "constants.h"
#include "editorbuffer.h"
#include "editormarker.h"
#include "filedescriptor.h"
#include "filetable.h"
#include "globals.h"
#include "init.h"
#include "log.h"
#include "macroargumenttable.h"
#include "memory.h"
#include "misc.h"
#include "options.h"
#include "parsers.h"
#include "position.h"
#include "referenceableitemtable.h"
#include "stackmemory.h"
#include "startup.h"
#include "symboltable.h"
#include "yylex.h"


/* Global parsing configuration - set before parsing starts */
ParsingConfig parsingConfig;


int currentFileNumber = -1;     /* Currently parsed file, maybe a header file */


ParserOperation getParserOperation(ServerOperation serverOp) {
    switch (serverOp) {
        case OP_INTERNAL_GET_FUNCTION_BOUNDS:
            return PARSE_TO_GET_FUNCTION_BOUNDS;
        case OP_INTERNAL_PARSE_TO_SET_MOVE_TARGET:
            return PARSE_TO_VALIDATE_MOVE_TARGET;
        case OP_INTERNAL_PARSE_TO_EXTRACT:
            return PARSE_TO_EXTRACT;
        case OP_INTERNAL_PARSE_TO_GOTO_PARAM_NAME:
        case OP_INTERNAL_PARSE_TO_GET_PARAM_COORDINATES:
            return PARSE_TO_TRACK_PARAMETERS;
        case OP_COMPLETION:
            return PARSE_TO_COMPLETE;
        default:
            return PARSE_TO_CREATE_REFERENCES;
    }
}


void syncParsingConfigFromOptions(Options options) {
    parsingConfig.operation = getParserOperation(options.serverOperation);

    parsingConfig.includeDirs = options.includeDirs;
    parsingConfig.defines = options.definitionStrings;

    parsingConfig.cursorOffset = options.cursorOffset;
    parsingConfig.markOffset = options.markOffset;
    parsingConfig.extractMode = options.extractMode;
    parsingConfig.targetParameterIndex = options.olcxGotoVal;

    parsingConfig.positionOfSelectedReference = NO_POSITION;
}

void setupParsingConfig(int fileNumber) {
  syncParsingConfigFromOptions(options);
  parsingConfig.fileNumber = fileNumber;
  parsingConfig.fileName = getFileItemWithFileNumber(fileNumber)->name;
  parsingConfig.language = getLanguageFor(parsingConfig.fileName);
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
    parsingConfig.positionOfSelectedReference = makePositionFromEditorMarker(target);

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
        .functionBegin = NO_POSITION,
        .functionEnd = NO_POSITION
    };

    /* Set up parsing configuration */
    syncParsingConfigFromOptions(options);
    parsingConfig.operation = PARSE_TO_GET_FUNCTION_BOUNDS;
    parsingConfig.positionOfSelectedReference = makePositionFromEditorMarker(marker);

    /* Clear previous results */
    parsedPositions[IPP_FUNCTION_BEGIN] = NO_POSITION;
    parsedPositions[IPP_FUNCTION_END] = NO_POSITION;

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

void initializeParsingSubsystem(void) {
    initializeCoreSubsystems();

    /* Copy preset options as baseline (file suffixes, defaults, etc.)
     * This gives us the same baseline as XRef mode without the expensive
     * project discovery and compiler interrogation that initializeFileProcessing does. */
    deepCopyOptionsFromTo(&presetOptions, &options);

    /* Override mode to ServerMode - required by parser assertions */
    options.mode = ServerMode;

    /* Prevent lowercasing of filenames on case-sensitive filesystems.
     * This ensures buffer lookups match how files were stored during didOpen. */
    options.fileNamesCaseSensitive = true;
}

void parseToCreateReferences(const char *fileName) {
    EditorBuffer *buffer;

    /* Try to get editor buffer first (preferred for LSP, refactoring, xref with open editors) */
    buffer = getOpenedAndLoadedEditorBuffer((char *)fileName);

    /* Fall back to disk file if editor buffer not found (common in xref batch mode) */
    if (buffer == NULL) {
        buffer = findOrCreateAndLoadEditorBufferForFile((char *)fileName);
        if (buffer == NULL) {
            log_error("parseToCreateReferences: Could not open '%s'", fileName);
            return;
        }
    }

    /* Setup input for parsing */
    initInput(NULL, buffer, "\n", (char *)fileName);

    setupParsingConfig(currentFile.characterBuffer.fileNumber);
    parsingConfig.operation = PARSE_TO_CREATE_REFERENCES;

    log_trace("parseToCreateReferences: Parsing '%s' as %s", fileName,
              parsingConfig.language == LANG_YACC ? "YACC" : "C");

    /* Parse the file - populating the ReferenceableItemTable */
    callParser(parsingConfig.fileNumber, parsingConfig.language);

    log_trace("parseToCreateReferences: Completed parsing '%s'", fileName);
}
