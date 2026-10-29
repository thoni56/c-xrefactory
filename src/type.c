#include "type.h"

#include <stddef.h>

const char *typeNamesTable[] = {
    ALL_TYPE_ENUMS(GENERATE_ENUM_STRING)
};

static const char *cTypeSpellings[MAX_CTYPE] = {
    [TypeChar]             = "char",
    [TypeUnsignedChar]     = "unsigned char",
    [TypeSignedChar]       = "signed char",
    [TypeInt]              = "int",
    [TypeUnsignedInt]      = "unsigned int",
    [TypeSignedInt]        = "signed int",
    [TypeShortInt]         = "short int",
    [TypeUnsignedShortInt] = "unsigned short int",
    [TypeSignedShortInt]   = "signed short int",
    [TypeLongInt]          = "long int",
    [TypeUnsignedLongInt]  = "unsigned long int",
    [TypeSignedLongInt]    = "signed long int",
    [TypeFloat]            = "float",
    [TypeDouble]           = "double",
    [TypeVoid]             = "void",
    [TmodLong]             = "long",
    [TmodShort]            = "short",
    [TmodSigned]           = "signed",
    [TmodUnsigned]         = "unsigned",
    [TmodShortSigned]      = "signed short",
    [TmodShortUnsigned]    = "unsigned short",
    [TmodLongSigned]       = "signed long",
    [TmodLongUnsigned]     = "unsigned long",
    [TypeLong]             = "long",
    [TypeBool]             = "bool",
};

/* How a type is written in C, or its name if it has no C spelling */
const char *cTypeSpelling(Type type) {
    if (type < MAX_CTYPE && cTypeSpellings[type] != NULL)
        return cTypeSpellings[type];
    return typeNamesTable[type];
}
