#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

/*
  SYMBOL TYPES

  Because of the macro magic we can't have the comments close to the
  enum values so here are some explanations of some of the Type enum
  values:

  TypeCppInclude - the Cpp #include reference
  TypeMacro - a macro in the symbol table
  TypeCppIfElse - the Cpp #if #else #fi references
  TypeCppCollate - the Cpp ## joined string reference
  TypePackage - a package in java
  TypeYaccSymbol - for yacc grammar references
  TypeLabel - a label in the symbol tabl
  TypeKeyword - a keyword in the symbol table
  TypeToken - a token for completions
  TypeUndefMacro - an undefined macro in the symbol table
  TypeMacroArg - a macro argument
  TypeDefinedOp - the 'defined' keyword in #if directives
  TypeBlockMarker - block markers for extract
  TypeTryCatchMarker - block markers for extract
  TypeExpression - an ambig. name evaluated to expression in java
  TypePackedType - typemodif, when type is in linkname
  TypeSpecialComplet - special completion string (for(;xx!=NULL ..
  TypeNonImportedClass - for completion
  TypeInducedError - dummy in general
  TypeInheritedFullMethod - dummy for completion, complete whole definition
  TypeSpecialConstructorCompletion - dummy completion of constructor 'super'
  TypeUnknown - dummy for completion

 */

#include "enums.h"

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
        ENUM(TYPE_MODIFIERS_END)                                        \
        ENUM(TypeElipsis)                                               \
        ENUM(TypeByte)                                                  \
        ENUM(TypeShort)                                                 \
        ENUM(TypeLong)                                                  \
        ENUM(TypeBoolean)                                               \
        ENUM(TypeNull)                                                  \
        ENUM(MAX_CTYPE)                                                 \
        ENUM(TypeCppInclude)                                            \
        ENUM(TypeMacro)                                                 \
        ENUM(TypeCppIfElse)                                             \
        ENUM(TypeCppCollate)                                            \
        ENUM(TypeYaccSymbol)                                            \
        ENUM(TypeLabel)                                                 \
        ENUM(TypeKeyword)                                               \
        ENUM(TypeToken)                                                 \
        ENUM(TypeUndefMacro)                                            \
        ENUM(TypeMacroArg)                                              \
        ENUM(TypeDefinedOp)                                             \
        ENUM(TypeBlockMarker)                                           \
        ENUM(TypeExpression)                                            \
        ENUM(TypePackedType)                                            \
        ENUM(TypeSpecialComplete)                                        \
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


extern const char *typeNamesTable[];

#endif /* _TYPES_H_ */
