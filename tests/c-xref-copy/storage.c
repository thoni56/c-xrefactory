#include "storage.h"

const char *storageEnumName[] = {
    ALL_STORAGE_ENUMS(GENERATE_ENUM_STRING)
};

char *storageNamesTable[STORAGE_ENUMS_MAX];
