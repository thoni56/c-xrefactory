#ifndef USAGE_H_INCLUDED
#define USAGE_H_INCLUDED

#include <stdbool.h>

/* These must be in this order for filtering to work since filtering compares
 * using LessThan. Also these are stored as the 'u' field in the cxfile "database" as
 * their corresponding numeric vales, so if you change this, you should bump
 * CXREF_FILE_FORMAT_VERSION */

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
extern bool isMoreImportantUsageThan(Usage usage1, Usage usage2);
extern bool isLessImportantUsageThan(Usage usage1, Usage usage2);
extern bool isAtMostAsImportantAs(Usage usage1, Usage usage2);

#endif
