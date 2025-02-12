#ifndef USAGE_H_INCLUDED
#define USAGE_H_INCLUDED

#include <stdbool.h>


/* Because of the macro magic we can't comment near the actual values
   so here are some descriptions of some of the Usage values
   (duplication so remember to change in here too):

    UsageLvalUsed                       - == USAGE_EXTEND_USAGE
    UsageAddrUsed                       - == USAGE_TOP_LEVEL_USED
    UsageFork                           - only in program graph for branching on label
    UsageNone                           - also forgotten rval usage of lval usage ex. l=3;
    UsageMacroBaseFileUsage             - reference to input file expanding the macro

 */


// !!!!!!!!!!!!! All this stuff is to be removed, new way of defining usages
// !!!!!!!!!!!!! is to set various bits in usg structure
/* TODO: well, they are still used, so there must be more to this... */
/* And they need to be monotonically ordered since
 * reset_reference_usage() compares them... */

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
