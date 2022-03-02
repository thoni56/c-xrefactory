#include "usage.h"

const char *usageEnumName[] = {
    ALL_USAGE_ENUMS(GENERATE_ENUM_STRING)
};


void fillUsageBits(UsageBits *usageBits, unsigned base, unsigned requiredAccess) {
    usageBits->kind = base;
    usageBits->requiredAccess = requiredAccess;
}
