#ifndef STORAGE_H_INCLUDED
#define STORAGE_H_INCLUDED

/* Because of the macro magic we can't comment near the actual values
   so here are some descriptions of some of the Usage values
   (duplication so remember to change in here too):

   StorageConstant       - enumerator definition
   // some "artificial" Java storages
   StorageMethod         - storage for class methods

*/

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
    ENUM(StorageMutable)                    \
    ENUM(StorageRegister)                   \
    ENUM(StorageMethod)                     \
    ENUM(MAX_STORAGE_NAMES)
    /* If this becomes more than 32 increase STORAGES_LN !!!!!!!! */

#define STORAGES_LN 5		/* logarithm of MAX_STORAGE */


typedef enum storage {
    ALL_STORAGE_ENUMS(GENERATE_ENUM_VALUE)
} Storage;


extern const char *storageEnumName[];

#endif
