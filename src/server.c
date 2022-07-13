#include "server.h"

#include "cxref.h"
#include "options.h"
#include "complete.h"
#include "globals.h"
#include "caching.h"

const char *operationNamesTable[] = {
    ALL_OPERATION_ENUMS(GENERATE_ENUM_STRING)
};
