#include "type.h"

#include <stddef.h>

#include "commons.h"
#include "proto.h"


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

static uchar typeShortChange[MAX_TYPE];
static uchar typeLongChange[MAX_TYPE];
static uchar typeSignedChange[MAX_TYPE];
static uchar typeUnsignedChange[MAX_TYPE];

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
    case TmodLongLong:
        r = typeLongChange[base];
        r = typeLongChange[r];
        break;
    case TmodLongLongUnsigned:
        r = typeUnsignedChange[base];
        r = typeLongChange[r];
        r = typeLongChange[r];
        break;
    case TmodLongLongSigned:
        r = typeSignedChange[base];
        r = typeLongChange[r];
        r = typeLongChange[r];
        break;
    default:
        assert(0);
    }
    return r;
}

typedef struct typeModificationsInit {
    int type;
    int modShort;
    int modLong;
    int modSigned;
    int modUnsigned;
} TypeModificationsInitTable;

/* What each modifier makes of a type, -1 where it leaves it as it is */
static TypeModificationsInitTable typeModificationsInitTable[] = {
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

void initTypeModifications(void) {
    for (int i = 0; i < MAX_TYPE; i++) {
        typeShortChange[i] = i;
        typeLongChange[i] = i;
        typeSignedChange[i] = i;
        typeUnsignedChange[i] = i;
    }
    for (int i = 0; typeModificationsInitTable[i].type >= 0; i++) {
        TypeModificationsInitTable *m = &typeModificationsInitTable[i];
        if (m->modShort >= 0)
            typeShortChange[m->type] = m->modShort;
        if (m->modLong >= 0)
            typeLongChange[m->type] = m->modLong;
        if (m->modSigned >= 0)
            typeSignedChange[m->type] = m->modSigned;
        if (m->modUnsigned >= 0)
            typeUnsignedChange[m->type] = m->modUnsigned;
    }
}
