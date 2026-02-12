#ifndef EXTRACT_H_INCLUDED
#define EXTRACT_H_INCLUDED

/**
 * @file extract.h
 * @brief Extract Refactoring
 *
 * Category: Parsing Infrastructure - Feature-Specific Semantic Actions
 *
 * Semantic actions for extract function/macro/variable refactoring.
 * Only active when parsingConfig.operation == PARSER_OP_EXTRACT.
 *
 * During parsing, collects:
 * - Block boundaries (begin/end markers in selected region)
 * - Variable usage patterns (input/output/local classification)
 * - Control flow (break/continue targets, returns)
 *
 * When containing function boundary is reached (actionsBeforeAfterExternalDefinition):
 * - Analyzes data flow to determine parameters
 * - Generates new function/macro definition
 * - Generates call site replacement
 * - Sets extractProcessedFlag (stops further parsing)
 *
 * Note: Extraction triggers during parse when containing function ends.
 * Ideally would happen after entire file is parsed (see longjmp comments).
 *
 * Called from: c_parser.y, yacc_parser.y (semantic action hooks)
 */


#include "symbol.h"


typedef enum extractModes {
    EXTRACT_FUNCTION,
    EXTRACT_MACRO,
    EXTRACT_VARIABLE
} ExtractMode;


extern Symbol *addContinueBreakLabelSymbol(int labn);
extern void actionsBeforeAfterExternalDefinition(void);
extern void extractActionOnBlockMarker(void);
extern void deleteContinueBreakLabelSymbol(void);
extern void generateContinueBreakReference(void);
extern void generateSwitchCaseFork(bool isLast);
extern void deleteContinueBreakSymbol(Symbol *symbol);

#endif
