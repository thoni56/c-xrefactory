#ifndef PARSING_H_INCLUDED
#define PARSING_H_INCLUDED

#include <stdbool.h>
#include "position.h"
#include "stringlist.h"
#include "editorbuffer.h"
#include "server.h"

typedef struct editorMarker EditorMarker;

/**
 * Operation that determines parser behavior.
 * Decouples parsing from server operation types.
 */
typedef enum {
    PARSER_OP_NORMAL,              /* Just parse, create references */
    PARSER_OP_GET_FUNCTION_BOUNDS, /* Record function boundaries */
    PARSER_OP_VALIDATE_MOVE_TARGET,/* Check if position is valid move target */
    PARSER_OP_EXTRACT,             /* Extract refactoring - track blocks/vars */
    PARSER_OP_TRACK_PARAMETERS,    /* Track parameter positions for navigation */
    PARSER_OP_COMPLETION           /* Code completion context */
} ParserOperation;

/**
 * Configuration for parsing - reusable across operations.
 * Contains preprocessor and language settings needed for parsing.
 */
typedef struct {
    StringList *includeDirs;     /* -I include directories */
    char       *defines;         /* -D preprocessor definitions */
    bool        strictAnsi;      /* ANSI C mode vs extensions */
} ParseConfig;

/**
 * Complete parsing configuration including operation and context.
 * This is the input to the parsing subsystem - set before parsing starts.
 */
typedef struct {
    StringList      *includeDirs;  /* -I include directories */
    char            *defines;      /* -D preprocessor definitions */
    bool             strictAnsi;   /* ANSI C mode vs extensions */
    ParserOperation  operation;    /* What should parser do? */
    Position         cursorPos;    /* Cursor position (for bounded operations) */
} ParsingConfig;

/**
 * Global parsing configuration.
 * Set this before calling parser to configure its behavior.
 */
extern ParsingConfig parsingConfig;

/**
 * Result of parsing to find function boundaries.
 */
typedef struct {
    bool     found;              /* True if function containing cursor was found */
    Position functionBegin;      /* Start of function (including comments/attributes) */
    Position functionEnd;        /* End of function (past closing brace and newline) */
} FunctionBoundariesResult;

/**
 * Result of parsing to validate a move target position.
 */
typedef struct {
    bool valid;                  /* True if position is valid for moving a function to */
} MoveTargetValidationResult;

/**
 * Create parse configuration from current global options.
 * Temporary bridge function until callers build ParseConfig themselves.
 */
extern ParseConfig createParseConfigFromOptions(void);

/**
 * Convert server operation to parser operation.
 * Maps high-level server requests to parser-specific behaviors.
 *
 * @param serverOp      Server operation from request
 * @return              Corresponding parser operation
 */
extern ParserOperation getParserOperation(ServerOperation serverOp);

/**
 * Check if parser operation needs to record reference at cursor position.
 *
 * @param op            Parser operation to check
 * @return              True if operation needs to know what symbol/reference cursor is on
 */
extern bool needsReferenceAtCursor(ParserOperation op);

/**
 * Check if a marker position is valid for moving a function to.
 * Convenience function that uses current global options.
 *
 * @param target        Editor marker at target position
 * @return              True if position is valid for moving a function to
 */
extern bool isValidMoveTarget(EditorMarker *target);

/**
 * Get the boundaries of the function containing the marker position.
 * Convenience function that uses current global options.
 *
 * @param marker        Editor marker at cursor position
 * @return              Result with function boundaries, or found=false if no function at cursor
 */
extern FunctionBoundariesResult getFunctionBoundaries(EditorMarker *marker);

#endif
