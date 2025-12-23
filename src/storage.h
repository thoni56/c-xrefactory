#ifndef STORAGE_H_INCLUDED
#define STORAGE_H_INCLUDED

#include "enums.h"

#define ALL_STORAGE_ENUMS(ENUM)             \
    ENUM(StorageDefault)                    \
    ENUM(StorageError)                      \
    ENUM(StorageAuto)                       \
    ENUM(StorageExtern)                     \
    ENUM(StorageConstant)                   \
    ENUM(StorageStatic)                     \
    ENUM(StorageThreadLocal)                \
    ENUM(StorageTypedef)                    \
    ENUM(StorageRegister)                   \
    ENUM(STORAGE_ENUMS_MAX)
    /* If this becomes more than 32 increase STORAGES_BITS !!!!!!!! */

#define STORAGES_BITS 5		/* logarithm of MAX_STORAGE */


typedef enum storage {
    ALL_STORAGE_ENUMS(GENERATE_ENUM_VALUE)
} Storage;


extern const char *storageEnumName[];

#endif
