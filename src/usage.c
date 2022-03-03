#include "usage.h"


const char *usageKindEnumName[] = {
    ALL_USAGE_ENUMS(GENERATE_ENUM_STRING)
};


void fillUsage(Usage *usage, UsageKind kind, AccessKind requiredAccess) {
    usage->kind = kind;
    usage->requiredAccess = requiredAccess;
}
