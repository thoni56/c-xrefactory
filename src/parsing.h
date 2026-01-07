#ifndef PARSING_H_INCLUDED
#define PARSING_H_INCLUDED

/**
 * @file parsing.h
 * @brief Parsing Subsystem Interface
 *
 * Parsing Subsystem Architecture
 * ==============================
 *
 * This module provides the public interface to the parsing subsystem,
 * decoupling parsing behavior from server operations.
 *
 * Flow during parsing:
 *
 *   server.c (orchestration)
 *     │ sets parsingConfig.operation
 *     ↓
 *   c_parser.y / yacc_parser.y (parsing infrastructure)
 *     │
 *     ├─→ yylex.c / lexer.c (tokenization)
 *     │   └─→ needsReferenceAtCursor() → collects the position of the reference the cursor is inside
 *     │
 *     ├─→ semact.c (core semantic actions - called from grammar rules)
 *     │   ├─→ symbol tables, type checking
 *     │   └─→ reference.c (manages reference lists)
 *     │       └─→ allowsDuplicateReferences()
 *     │
 *     ├─→ extract.c (feature semantic actions - if PARSER_OP_EXTRACT)
 *     │   └─→ track blocks, variables, control flow
 *     │
 *     └─→ complete.c (feature semantic actions - if PARSER_OP_COMPLETION)
 *         └─→ build completion candidates
 *
 * Module Categories:
 * - Parsing Infrastructure: Core parsing and lexical analysis
 * - Core Semantic Actions: General-purpose symbol/type handling (semact.c)
 * - Feature Semantic Actions: Operation-specific analysis (extract.c, complete.c)
 * - Parsing Support Libraries: Data structures and utilities (reference.c)
 * - Orchestration: High-level coordination (server.c, refactory.c, cxref.c)
 */

#include <stdbool.h>
#include "position.h"
#include "stringlist.h"
#include "editorbuffer.h"
#include "extract.h"
#include "server.h"
#include "options.h"


typedef struct editorMarker EditorMarker;

/**
 * Operation that determines parser behavior.
 * Decouples parsing from server operation types.
 */
typedef enum {
    PARSE_TO_CREATE_REFERENCES,    /* Just parse, create references */
    PARSE_TO_GET_FUNCTION_BOUNDS,  /* Record function boundaries */
    PARSE_TO_VALIDATE_MOVE_TARGET, /* Check if position is valid move target */
    PARSE_TO_EXTRACT,              /* Extract refactoring - track blocks/vars */
    PARSE_TO_TRACK_PARAMETERS,     /* Track parameter positions for navigation */
    PARSE_TO_COMPLETE              /* Complete code at cursor */
} ParserOperation;

/**
 * Parsing configuration including operation and context.
 * This is the input to the parsing subsystem - set before parsing starts.
 */
typedef struct {
    const char      *fileName;     /* File to parse */
    StringList      *includeDirs;  /* -I include directories */
    char            *defines;      /* -D preprocessor definitions */
    ParserOperation  operation;    /* What should parser do? */
    int              cursorOffset; /* Byte offset of cursor in the file sent by the
                                    * legacy client.. */
    Position         positionOfSelectedReference; /* ... which the lexer turns into a
                                                   * position by inspecting the lexems
                                                   * to find the one the offset is
                                                   * inside. The _beginning_ of that
                                                   * lexem becomes the position. */
    int              markOffset;   /* Byte offset of mark (selection end), -1 if no selection */
    ExtractMode      extractMode;  /* Which extraction type (function/macro/variable) */
    int              targetParameterIndex; /* Which parameter to track (for PARSE_TO_TRACK_PARAMETERS) */
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
 * The fileNumber for the "main" file that the parse started from. Is not a header file.
 */
extern int topLevelFileNumber;

/**
 * The filenumber for the currently parsed file, might be an include file.
 * Parsing internal use only.
 */
extern int currentFileNumber;

/**
 * Convert server operation to parser operation.
 * Maps high-level server requests to parser-specific behaviors.
 *
 * @param serverOp      Server operation from request
 * @return              Corresponding parser operation
 */
extern ParserOperation getParserOperation(ServerOperation serverOp);

extern void syncParsingConfigFromOptions(Options options);

/**
 * Check if parser operation needs to record reference at cursor position.
 *
 * @param op            Parser operation to check
 * @return              True if operation needs to know what symbol/reference cursor is on
 */
extern bool needsReferenceAtCursor(ParserOperation op);

/**
 * Check if parser operation allows duplicate references at same position.
 * Extract operation needs all references including duplicates to analyze data flow.
 *
 * @param op            Parser operation to check
 * @return              True if operation allows duplicate references
 */
extern bool allowsDuplicateReferences(ParserOperation op);

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

/**
 * Initialize the parsing subsystem.
 * Must be called once before any parsing operations.
 * Sets up minimal state needed for parsing to work.
 */
extern void initializeParsingSubsystem(void);

/**
 * Parse a file to create references in the reference database.
 * Handles both editor buffers and disk files: tries editor buffer first,
 * falls back to loading from disk if not found.
 * Works for all modes: LSP, xref batch processing, and refactoring.
 * Language is automatically determined from the filename extension.
 *
 * @param fileName      Absolute path to the file to parse
 */
extern void parseToCreateReferences(const char *fileName);

#endif
