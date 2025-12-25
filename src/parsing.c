#include "parsing.h"

#include "options.h"

ParseConfig createParseConfigFromOptions(void) {
    ParseConfig config = {
        .includeDirs = options.includeDirs,
        .defines = options.definitionStrings,
        .strictAnsi = options.strictAnsi
    };
    return config;
}
