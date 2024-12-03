#include "usage.h"


const char *usageKindEnumName[] = {
    ALL_USAGE_ENUMS(GENERATE_ENUM_STRING)
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
