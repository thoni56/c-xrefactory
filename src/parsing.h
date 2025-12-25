#ifndef PARSING_H_INCLUDED
#define PARSING_H_INCLUDED

#include <stdbool.h>
#include "position.h"
#include "stringlist.h"
#include "editorbuffer.h"

typedef struct editorMarker EditorMarker;

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
