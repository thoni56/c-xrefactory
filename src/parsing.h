#ifndef PARSING_H_INCLUDED
#define PARSING_H_INCLUDED

#include <stdbool.h>
#include "stringlist.h"

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
 * Create parse configuration from current global options.
 * Temporary bridge function until callers build ParseConfig themselves.
 */
extern ParseConfig createParseConfigFromOptions(void);

#endif
