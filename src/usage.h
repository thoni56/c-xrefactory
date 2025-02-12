#ifndef USAGE_H_INCLUDED
#define USAGE_H_INCLUDED

#include <stdbool.h>


#include "enums.h"
#define ALL_USAGE_ENUMS(ENUM)                   \
/* filter3  == all filters */                   \
    ENUM(UsageOLBestFitDefined)                 \
    ENUM(UsageDefined)                          \
    ENUM(UsageDeclared)                         \
/* filter2 */                                   \
    ENUM(UsageLvalUsed)                         \
/* filter1 */                                   \
    ENUM(UsageAddrUsed)                         \
/* filter0 */                                   \
    ENUM(UsageUsed)                             \
    ENUM(UsageUndefinedMacro)                   \
    ENUM(UsageFork)                             \
/* INVISIBLE USAGES */                          \
    ENUM(UsageMaxOnLineVisibleUsages)           \
    ENUM(UsageNone)                             \
    ENUM(UsageMacroBaseFileUsage)               \
    ENUM(MAX_USAGES)                            \
    ENUM(USAGE_ANY)                             \
    ENUM(USAGE_FILTER)

typedef enum {
    ALL_USAGE_ENUMS(GENERATE_ENUM_VALUE)
} Usage;


extern const char *usageKindEnumName[];

extern bool isVisibleUsage(Usage usage);
extern bool isDefinitionUsage(Usage usage);
extern bool isDefinitionOrDeclarationUsage(Usage usage);

#endif
