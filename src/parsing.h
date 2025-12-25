#ifndef PARSING_H_INCLUDED
#define PARSING_H_INCLUDED

#include <stdbool.h>
#include "position.h"
#include "stringlist.h"
#include "editorbuffer.h"

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
 * Create parse configuration from current global options.
 * Temporary bridge function until callers build ParseConfig themselves.
 */
extern ParseConfig createParseConfigFromOptions(void);

/**
 * Parse to find the boundaries of the function containing the cursor.
 *
 * Used by: Move Function refactoring
 *
 * @param buffer        Editor buffer to parse
 * @param config        Parsing configuration (include dirs, defines, etc.)
 * @param cursorPos     Position of cursor - determines which function to find
 * @return              Result with function boundaries, or found=false if no function at cursor
 */
extern FunctionBoundariesResult parseToGetFunctionBoundaries(
    EditorBuffer *buffer,
    ParseConfig  *config,
    Position      cursorPos
);

#endif
