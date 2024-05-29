#include "usage.h"


const char *usageKindEnumName[] = {
    ALL_USAGE_ENUMS(GENERATE_ENUM_STRING)
};


void fillUsage(Usage *usage, UsageKind kind) {
    usage->kind = kind;
}


bool isVisibleUsage(UsageKind usageKind) {
    return usageKind < UsageMaxOLUsages;
}

bool isDefinitionUsage(UsageKind usageKind) {
    return usageKind == UsageDefined || usageKind == UsageOLBestFitDefined;
}

bool isDefinitionOrDeclarationUsage(UsageKind usageKind) {
    return isDefinitionUsage(usageKind) || usageKind == UsageDeclared;
}
