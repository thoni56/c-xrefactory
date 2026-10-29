#include "type.h"

#include <stddef.h>

#include "commons.h"
#include "globals.h"


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

Type combinedType(Type base, Type modifier) {
    Type r = 0;
    switch (modifier) {
    case TmodLong:
        r = typeLongChange[base];
        break;
    case TmodShort:
        r = typeShortChange[base];
        break;
    case TmodSigned:
        r = typeSignedChange[base];
        break;
    case TmodUnsigned:
        r = typeUnsignedChange[base];
        break;
    case TmodShortSigned:
        r = typeSignedChange[base];
        r = typeShortChange[r];
        break;
    case TmodShortUnsigned:
        r = typeUnsignedChange[base];
        r = typeShortChange[r];
        break;
    case TmodLongSigned:
        r = typeSignedChange[base];
        r = typeLongChange[r];
        break;
    case TmodLongUnsigned:
        r = typeUnsignedChange[base];
        r = typeLongChange[r];
        break;
    default:
        assert(0);
    }
    return r;
}
