#ifndef _STORAGE_H_
#define _STORAGE_H_

typedef enum storage {
    // standard C storage modes
    StorageError,
    StorageAuto,
    StorageGlobal,			/* not used anymore, backward compatibility */
    StorageDefault,
    StorageExtern,
    StorageConstant,		/* enumerator definition */
    StorageStatic,
    StorageThreadLocal,
    StorageTypedef,
    StorageMutable,
    StorageRegister,
    // some "artificial" Java storages
    StorageConstructor,		/* storage for class constructors */
    StorageField,			/* storage for class fields */
    StorageMethod,			/* storage for class methods */
    //
    StorageNone,
    MAX_STORAGE,
    /* If this becomes greater than 32 increase STORAGES_LN !!!!!!!! */
} Storage;

#define STORAGES_LN 5		/* logarithm of MAX_STORAGE */

#endif
