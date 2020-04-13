#ifndef _TYPES_H_
#define _TYPES_H_

typedef enum type {
    TypeDefault,
    TypeChar,
    TypeUnsignedChar,
    TypeSignedChar,
    TypeInt,
    TypeUnsignedInt,
    TypeSignedInt,
    TypeShortInt,
    TypeShortUnsignedInt,
    TypeShortSignedInt,
    TypeLongInt,
    TypeLongUnsignedInt,
    TypeLongSignedInt,
    TypeFloat,
    TypeDouble,
    TypeStruct,
    TypeUnion,
    TypeEnum,
    TypeVoid,
    TypePointer,
    TypeArray,
    TypeFunction,
    TypeAnonymousField,
    TypeError,
    MODIFIERS_START,
    TmodLong,
    TmodShort,
    TmodSigned,
    TmodUnsigned,
    TmodShortSigned,
    TmodShortUnsigned,
    TmodLongSigned,
    TmodLongUnsigned,
    TypeReserve1,
    TypeReserve2,
    TypeReserve3,
    TypeReserve4,
    MODIFIERS_END,
    TypeElipsis,
    JAVA_TYPES,
    TypeByte,
    TypeShort,
    TypeLong,
    TypeBoolean,
    TypeNull,
    TypeOverloadedFunction,
    TypeReserve7,
    TypeReserve8,
    TypeReserve9,
    TypeReserve10,
    TypeReserve11,
    CCC_TYPES,
    TypeWchar_t,
    MAX_CTYPE,
    TypeCppInclude,		/* dummy, the Cpp #include reference */
    TypeMacro,			/* dummy, a macro in the symbol table */
    TypeCppIfElse,		/* dummy, the Cpp #if #else #fi references */
    TypeCppCollate,		/* dummy, the Cpp ## joined string reference */
    TypePackage,		/* dummy, a package in java */
    TypeYaccSymbol,		/* dummy, for yacc grammar references */
    MAX_HTML_LIST_TYPE, /* only types until this will be listed in html lists*/
    TypeLabel,			/* dummy, a label in the symbol table*/
    TypeKeyword,		/* dummy, a keyword in the symbol table, + html ref. */
    TypeToken,			/* dummy, a token for completions */
    TypeUndefMacro,		/* dummy, an undefined macro in the symbol table */
    TypeMacroArg,		/* dummy, a macro argument */
    TypeDefinedOp,		/* dummy, the 'defined' keyword in #if directives */
    TypeCppAny,			/* dummy, a Cpp reference (html only) */
    TypeBlockMarker,	/* dummy, block markers for extract */
    TypeTryCatchMarker,	/* dummy, block markers for extract */
    TypeComment,		/* dummy, a commentary reference (html only) */
    TypeExpression,		/* dummy, an ambig. name evaluated to expression in java */
    TypePackedType,		/* dummy, typemodif, when type is in linkname */
    TypeFunSep,			/* dummy, function separator for HTML */
    TypeSpecialComplet,	/* dummy special completion string (for(;xx!=NULL ..)*/
    TypeNonImportedClass,/* dummy for completion*/
    TypeInducedError,    /* dummy in general*/
    TypeInheritedFullMethod, /* dummy for completion, complete whole definition */
    TypeSpecialConstructorCompletion, /*dummy completion of constructor 'super'*/
    TypeUnknown,                      /* dummy for completion */
    MAX_TYPE,
    /* if this becomes greater than 256, increase SYMTYPES_LN !!!!!!!!!!!!! */
} Type;

#endif /* _TYPES_H_ */
