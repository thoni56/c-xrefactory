#ifndef _TYPES_H_
#define _TYPES_H_

/*
  Because of the macro magic we can't have the comments close to the
  enum values so here are some explanations of some of the Type enum
  values:

  TypeCppInclude - dummy, the Cpp #include reference
  TypeMacro - dummy, a macro in the symbol table
  TypeCppIfElse - dummy, the Cpp #if #else #fi references
  TypeCppCollate - dummy, the Cpp ## joined string reference
  TypePackage - dummy, a package in java
  TypeYaccSymbol - dummy, for yacc grammar references
  MAX_HTML_LIST_TYPE - only types up to here will be listed in html list
  TypeLabel - dummy, a label in the symbol tabl
  TypeKeyword - dummy, a keyword in the symbol table, + html ref.
  TypeToken - dummy, a token for completions
  TypeUndefMacro - dummy, an undefined macro in the symbol table
  TypeMacroArg - dummy, a macro argument
  TypeDefinedOp - dummy, the 'defined' keyword in #if directives
  TypeCppAny - dummy, a Cpp reference (html only)
  TypeBlockMarker - dummy, block markers for extract
  TypeTryCatchMarker - dummy, block markers for extract
  TypeComment - dummy, a commentary reference (html only)
  TypeExpression - dummy, an ambig. name evaluated to expression in java
  TypePackedType - dummy, typemodif, when type is in linkname
  TypeFunSep - dummy, function separator for HTML
  TypeSpecialComplet - dummy special completion string (for(;xx!=NULL ..
  TypeNonImportedClass - dummy for completio
  TypeInducedError - dummy in genera
  TypeInheritedFullMethod - dummy for completion, complete whole definition
  TypeSpecialConstructorCompletion - dummy completion of constructor 'super'
  TypeUnknown - dummy for completion

 */

/* Some CPP magic to be able to print enums as strings: */
#define GENERATE_ENUM_VALUE(ENUM) ENUM,
#define GENERATE_ENUM_STRING(STRING) #STRING,

#define ALL_TYPE_ENUMS(ENUM)                                            \
    ENUM(TypeDefault)                                                   \
        ENUM(TypeChar)                                                  \
        ENUM(TypeUnsignedChar)                                          \
        ENUM(TypeSignedChar)                                            \
        ENUM(TypeInt)                                                   \
        ENUM(TypeUnsignedInt)                                           \
        ENUM(TypeSignedInt)                                             \
        ENUM(TypeShortInt)                                              \
        ENUM(TypeShortUnsignedInt)                                      \
        ENUM(TypeShortSignedInt)                                        \
        ENUM(TypeLongInt)                                               \
        ENUM(TypeLongUnsignedInt)                                       \
        ENUM(TypeLongSignedInt)                                         \
        ENUM(TypeFloat)                                                 \
        ENUM(TypeDouble)                                                \
        ENUM(TypeStruct)                                                \
        ENUM(TypeUnion)                                                 \
        ENUM(TypeEnum)                                                  \
        ENUM(TypeVoid)                                                  \
        ENUM(TypePointer)                                               \
        ENUM(TypeArray)                                                 \
        ENUM(TypeFunction)                                              \
        ENUM(TypeAnonymousField)                                        \
        ENUM(TypeError)                                                 \
        ENUM(MODIFIERS_START)                                           \
        ENUM(TmodLong)                                                  \
        ENUM(TmodShort)                                                 \
        ENUM(TmodSigned)                                                \
        ENUM(TmodUnsigned)                                              \
        ENUM(TmodShortSigned)                                           \
        ENUM(TmodShortUnsigned)                                         \
        ENUM(TmodLongSigned)                                            \
        ENUM(TmodLongUnsigned)                                          \
        ENUM(TypeReserve1)                                              \
        ENUM(TypeReserve2)                                              \
        ENUM(TypeReserve3)                                              \
        ENUM(TypeReserve4)                                              \
        ENUM(MODIFIERS_END)                                             \
        ENUM(TypeElipsis)                                               \
        ENUM(JAVA_TYPES)                                                \
        ENUM(TypeByte)                                                  \
        ENUM(TypeShort)                                                 \
        ENUM(TypeLong)                                                  \
        ENUM(TypeBoolean)                                               \
        ENUM(TypeNull)                                                  \
        ENUM(TypeOverloadedFunction)                                    \
        ENUM(TypeReserve7)                                              \
        ENUM(TypeReserve8)                                              \
        ENUM(TypeReserve9)                                              \
        ENUM(TypeReserve10)                                             \
        ENUM(TypeReserve11)                                             \
        ENUM(CCC_TYPES)                                                 \
        ENUM(TypeWchar_t)                                               \
        ENUM(MAX_CTYPE)                                                 \
        ENUM(TypeCppInclude)                                            \
        ENUM(TypeMacro)                                                 \
        ENUM(TypeCppIfElse)                                             \
        ENUM(TypeCppCollate)                                            \
        ENUM(TypePackage)                                               \
        ENUM(TypeYaccSymbol)                                            \
        ENUM(MAX_HTML_LIST_TYPE)                                        \
        ENUM(TypeLabel)                                                 \
        ENUM(TypeKeyword)                                               \
        ENUM(TypeToken)                                                 \
        ENUM(TypeUndefMacro)                                            \
        ENUM(TypeMacroArg)                                              \
        ENUM(TypeDefinedOp)                                             \
        ENUM(TypeCppAny)                                                \
        ENUM(TypeBlockMarker)                                           \
        ENUM(TypeTryCatchMarker)                                        \
        ENUM(TypeComment)                                               \
        ENUM(TypeExpression)                                            \
        ENUM(TypePackedType)                                            \
        ENUM(TypeFunSep)                                                \
        ENUM(TypeSpecialComplet)                                        \
        ENUM(TypeNonImportedClass)                                      \
        ENUM(TypeInducedError)                                          \
        ENUM(TypeInheritedFullMethod)                                   \
        ENUM(TypeSpecialConstructorCompletion)                          \
        ENUM(TypeUnknown)                                               \
        ENUM(MAX_TYPE)                                                  \
/* If these becomes more than 256, increase SYMTYPES_LN */

#define SYMTYPES_LN 7


typedef enum type {
    ALL_TYPE_ENUMS(GENERATE_ENUM_VALUE)
} Type;


extern const char *typeEnumName[];

#endif /* _TYPES_H_ */
