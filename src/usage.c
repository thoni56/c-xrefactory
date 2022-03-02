#include "usage.h"


const char *usageKindEnumName[] = {
    ALL_USAGE_ENUMS(GENERATE_ENUM_STRING)
};


void fillUsageBits(UsageBits *usageBits, UsageKind kind, AccessKind requiredAccess) {
    usageBits->kind = kind;
    usageBits->requiredAccess = requiredAccess;
}
