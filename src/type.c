#include "type.h"

#include <stddef.h>

#include "commons.h"


const char *typeNamesTable[] = {
    ALL_TYPE_ENUMS(GENERATE_ENUM_STRING)
};

static const char *cTypeSpellings[MAX_CTYPE] = {
    [TypeChar]                = "char",
    [TypeUnsignedChar]        = "unsigned char",
    [TypeSignedChar]          = "signed char",
    [TypeInt]                 = "int",
    [TypeUnsignedInt]         = "unsigned int",
    [TypeSignedInt]           = "signed int",
    [TypeShortInt]            = "short int",
    [TypeUnsignedShortInt]    = "unsigned short int",
    [TypeSignedShortInt]      = "signed short int",
    [TypeLongInt]             = "long int",
    [TypeUnsignedLongInt]     = "unsigned long int",
    [TypeSignedLongInt]       = "signed long int",
    [TypeLongLongInt]         = "long long int",
    [TypeUnsignedLongLongInt] = "unsigned long long int",
    [TypeSignedLongLongInt]   = "signed long long int",
    [TypeFloat]               = "float",
    [TypeDouble]              = "double",
    [TypeVoid]                = "void",
    [TmodLong]                = "long",
    [TmodShort]               = "short",
    [TmodSigned]              = "signed",
    [TmodUnsigned]            = "unsigned",
    [TmodShortSigned]         = "signed short",
    [TmodShortUnsigned]       = "unsigned short",
    [TmodLongSigned]          = "signed long",
    [TmodLongUnsigned]        = "unsigned long",
    [TmodLongLong]            = "long long",
    [TmodLongLongUnsigned]    = "unsigned long long",
    [TmodLongLongSigned]      = "signed long long",
    [TypeLong]                = "long",
    [TypeBool]                = "bool",
};

/* ********************************************************************** */
/*                            real constants                              */
/* ********************************************************************** */

static Type withShort[MAX_TYPE];
static Type withLong[MAX_TYPE];
static Type withSigned[MAX_TYPE];
static Type withUnsigned[MAX_TYPE];

bool isModifier(Type type) {
    return type > MODIFIERS_START && type < TYPE_MODIFIERS_END;
}

/* How a type is written in C, or its name if it has no C spelling */
const char *cTypeSpelling(Type type) {
    if (type < MAX_CTYPE && cTypeSpellings[type] != NULL)
        return cTypeSpellings[type];
    return typeNamesTable[type];
}

Type combinedType(Type base, Type modifier) {
    switch (modifier) {
    case TmodLong:
        return withLong[base];
    case TmodShort:
        return withShort[base];
    case TmodSigned:
        return withSigned[base];
    case TmodUnsigned:
        return withUnsigned[base];
    case TmodShortSigned:
        return withShort[withSigned[base]];
    case TmodShortUnsigned:
        return withShort[withUnsigned[base]];
    case TmodLongSigned:
        return withLong[withSigned[base]];
    case TmodLongUnsigned:
        return withLong[withUnsigned[base]];
    case TmodLongLong:
        return withLong[withLong[base]];
    case TmodLongLongUnsigned:
        return withLong[withLong[withUnsigned[base]]];
    case TmodLongLongSigned:
        return withLong[withLong[withSigned[base]]];
    default:
        assert(0);
        return TypeDefault;
    }
}

typedef struct {
    int type;
    int modifiedUsingShort;
    int modifiedUsingLong;
    int modifiedUsingSigned;
    int modifiedUsingUnsigned;
} TypeModificationRow;

/* What each modifier makes of a type, -1 where it leaves it as it is */
static TypeModificationRow typeModifications[] = {
    //  type                 short                 long                     signed                 unsigned
    {TypeDefault,         TypeShortInt,         TypeLongInt,             TypeSignedInt,         TypeUnsignedInt},
    {TypeChar,            -1,                   -1,                      TypeSignedChar,        TypeUnsignedChar},
    {TypeInt,             TypeShortInt,         TypeLongInt,             TypeSignedInt,         TypeUnsignedInt},
    {TypeUnsignedInt,     TypeUnsignedShortInt, TypeUnsignedLongInt,     -1,                    -1},
    {TypeSignedInt,       TypeSignedShortInt,   TypeSignedLongInt,       -1,                    -1},
    {TypeShortInt,        -1,                   -1,                      TypeSignedShortInt,    TypeUnsignedShortInt},
    {TypeLongInt,         -1,                   TypeLongLongInt,         TypeSignedLongInt,     TypeUnsignedLongInt},
    {TypeUnsignedLongInt, -1,                   TypeUnsignedLongLongInt, -1,                    -1},
    {TypeSignedLongInt,   -1,                   TypeSignedLongLongInt,   -1,                    -1},
    {TypeLongLongInt,     -1,                   -1,                      TypeSignedLongLongInt, TypeUnsignedLongLongInt},
    {TmodLong,            -1,                   TmodLongLong,            TmodLongSigned,        TmodLongUnsigned},
    {TmodLongLong,        -1,                   -1,                      TmodLongLongSigned,    TmodLongLongUnsigned},
    {TmodLongUnsigned,    -1,                   TmodLongLongUnsigned,    -1,                    -1},
    {TmodLongSigned,      -1,                   TmodLongLongSigned,      -1,                    -1},
    {TmodShort,           -1,                   -1,                      TmodShortSigned,       TmodShortUnsigned},
    {TmodSigned,          TmodShortSigned,      TmodLongSigned,          -1,                    -1},
    {TmodUnsigned,        TmodShortUnsigned,    TmodLongUnsigned,        -1,                    -1},
    {-1,                  -1,                   -1,                      -1,                    -1}
};

static void initializeTypeModificationsToIdentity(void) {
    for (Type t = TypeDefault; t < MAX_TYPE; t++) {
        withShort[t] = t;
        withLong[t] = t;
        withSigned[t] = t;
        withUnsigned[t] = t;
    }
}

void initTypeModifications(void) {
    initializeTypeModificationsToIdentity();

    for (int i = 0; typeModifications[i].type != -1; i++) {
        TypeModificationRow *row = &typeModifications[i];
        if (row->modifiedUsingShort != -1)
            withShort[row->type] = row->modifiedUsingShort;
        if (row->modifiedUsingLong != -1)
            withLong[row->type] = row->modifiedUsingLong;
        if (row->modifiedUsingSigned != -1)
            withSigned[row->type] = row->modifiedUsingSigned;
        if (row->modifiedUsingUnsigned != -1)
            withUnsigned[row->type] = row->modifiedUsingUnsigned;
    }
}
