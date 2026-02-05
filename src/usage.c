#include "usage.h"


const char *usageKindEnumName[] = {
    ALL_USAGE_ENUMS(GENERATE_ENUM_STRING)
};

/* These levels are also used in the Emacs UI */
int usageFilterLevels[] = {
    UsageMaxOnLineVisibleUsages,
    UsageUsed,
    UsageAddrUsed,
    UsageLvalUsed,
};


bool isVisibleUsage(Usage usage) {
    return usage < UsageMaxOnLineVisibleUsages;
}

bool isDefinitionUsage(Usage usage) {
    return usage == UsageDefined || usage == UsageOLBestFitDefined;
}

bool isDefinitionOrDeclarationUsage(Usage usage) {
    return isDefinitionUsage(usage) || usage == UsageDeclared;
}

bool isMoreImportantUsageThan(Usage usage1, Usage usage2) {
    /* Lower enum values represent more important usages (definitions > declarations > usages).
     * This will be reversed when we eventually flip the enum ordering. */
    return usage1 < usage2;
}

bool isLessImportantUsageThan(Usage usage1, Usage usage2) {
    return usage1 > usage2;
}

bool isAtMostAsImportantAs(Usage usage1, Usage usage2) {
    return usage1 >= usage2;
}
