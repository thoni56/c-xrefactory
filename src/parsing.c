#include "parsing.h"

#include "editorbuffer.h"
#include "editormarker.h"
#include "filedescriptor.h"
#include "filetable.h"
#include "globals.h"
#include "log.h"
#include "misc.h"
#include "options.h"
#include "parsers.h"
#include "position.h"
#include "startup.h"
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
    parsingConfig.targetParameterIndex = options.index;

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

/* The operation's own fields, e.g. the cursor offset, are set after this,
 * since the setup takes the rest from the options */
void startParsingBuffer(EditorBuffer *buffer, ParserOperation operation) {
    initInput(NULL, buffer, "\n", buffer->fileName);

    setupParsingConfig(buffer->fileNumber);
    parsingConfig.operation = operation;

    resetForCompilationUnit();
}

bool isValidMoveTarget(EditorMarker *target) {
    MoveTargetValidationResult result = {
        .valid = false
    };

    startParsingBuffer(target->buffer, PARSE_TO_VALIDATE_MOVE_TARGET);
    parsingConfig.cursorOffset = target->offset;

    /* Clear previous results */
    parsedInfo.moveTargetAccepted = false;

    callParser(target->buffer->fileNumber, parsingConfig.language);

    /* Extract result from global state */
    result.valid = parsedInfo.moveTargetAccepted;

    return result.valid;
}

static void parseToGetDefinitionBoundaries(EditorMarker *marker) {
    startParsingBuffer(marker->buffer, PARSE_TO_GET_FUNCTION_BOUNDS);
    parsingConfig.cursorOffset = marker->offset;

    /* Clear previous results */
    for (int i = 0; i < IPP_MAX; i++)
        parsedPositions[i] = NO_POSITION;

    callParser(marker->buffer->fileNumber, parsingConfig.language);
}

/* Extract results from global state */
static DefinitionBoundariesResult parsedBoundaries(ImportantParsedPositionKind begin,
                                                   ImportantParsedPositionKind end) {
    DefinitionBoundariesResult result = {
        .found = false,
        .begin = NO_POSITION,
        .end = NO_POSITION
    };
    if (parsedPositions[begin].file != NO_FILE_NUMBER &&
        parsedPositions[end].file != NO_FILE_NUMBER) {
        result.found = true;
        result.begin = parsedPositions[begin];
        result.end = parsedPositions[end];
    }
    return result;
}

DefinitionBoundariesResult getFunctionBoundaries(EditorMarker *marker) {
    parseToGetDefinitionBoundaries(marker);
    return parsedBoundaries(IPP_FUNCTION_BEGIN, IPP_FUNCTION_END);
}

DefinitionBoundariesResult getDefinitionBoundaries(EditorMarker *marker) {
    parseToGetDefinitionBoundaries(marker);
    DefinitionBoundariesResult result = parsedBoundaries(IPP_FUNCTION_BEGIN, IPP_FUNCTION_END);
    if (!result.found)
        result = parsedBoundaries(IPP_VARIABLE_BEGIN, IPP_VARIABLE_END);
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

    saveProjectBaseline();
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

    startParsingBuffer(buffer, PARSE_TO_CREATE_REFERENCES);

    log_trace("parseToCreateReferences: Parsing '%s' as %s", fileName,
              parsingConfig.language == LANG_YACC ? "YACC" : "C");

    /* Parse the file - populating the ReferenceableItemTable */
    callParser(parsingConfig.fileNumber, parsingConfig.language);

    log_trace("parseToCreateReferences: Completed parsing '%s'", fileName);
}
