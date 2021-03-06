#ifndef LEXEM_H_INCLUDED
#define LEXEM_H_INCLUDED

/**** DO NOT EDIT - generated from c_parser.tab.h by Makefile ****/

typedef enum lexem {
TYPE_NAME = 257,
CLASS_NAME = 258,
TEMPLATE_NAME = 259,
CONVERSION_OP_ID_PREFIX = 260,
OPERATOR_IDENT = 261,
INC_OP = 262,
DEC_OP = 263,
LEFT_OP = 264,
RIGHT_OP = 265,
LE_OP = 266,
GE_OP = 267,
EQ_OP = 268,
NE_OP = 269,
AND_OP = 270,
OR_OP = 271,
MUL_ASSIGN = 272,
DIV_ASSIGN = 273,
MOD_ASSIGN = 274,
ADD_ASSIGN = 275,
SUB_ASSIGN = 276,
LEFT_ASSIGN = 277,
RIGHT_ASSIGN = 278,
AND_ASSIGN = 279,
XOR_ASSIGN = 280,
OR_ASSIGN = 281,
PTR_OP = 282,
ELLIPSIS = 283,
URIGHT_OP = 284,
URIGHT_ASSIGN = 285,
YACC_PERC = 286,
YACC_DPERC = 287,
DPOINT = 288,
POINTM_OP = 289,
PTRM_OP = 290,
STATIC = 291,
BREAK = 292,
CASE = 293,
CHAR = 294,
CONST = 295,
CONTINUE = 296,
DEFAULT = 297,
DO = 298,
DOUBLE = 299,
ELSE = 300,
FLOAT = 301,
FOR = 302,
GOTO = 303,
IF = 304,
INT = 305,
LONG = 306,
RETURN = 307,
SHORT = 308,
SWITCH = 309,
VOID = 310,
VOLATILE = 311,
WHILE = 312,
TYPEDEF = 313,
EXTERN = 314,
AUTO = 315,
REGISTER = 316,
SIGNED = 317,
UNSIGNED = 318,
STRUCT = 319,
UNION = 320,
ENUM = 321,
SIZEOF = 322,
RESTRICT = 323,
_ATOMIC = 324,
_BOOL = 325,
_THREADLOCAL = 326,
_NORETURN = 327,
ANONYMOUS_MODIFIER = 328,
ABSTRACT = 329,
BOOLEAN = 330,
BYTE = 331,
CATCH = 332,
CLASS = 333,
EXTENDS = 334,
FINAL = 335,
FINALLY = 336,
IMPLEMENTS = 337,
IMPORT = 338,
INSTANCEOF = 339,
INTERFACE = 340,
NATIVE = 341,
NEW = 342,
PACKAGE = 343,
PRIVATE = 344,
PROTECTED = 345,
PUBLIC = 346,
SUPER = 347,
SYNCHRONIZED = 348,
THIS = 349,
THROW = 350,
THROWS = 351,
TRANSIENT = 352,
TRY = 353,
TRUE_LITERAL = 354,
FALSE_LITERAL = 355,
NULL_LITERAL = 356,
STRICTFP = 357,
ASSERT = 358,
FRIEND = 359,
OPERATOR = 360,
NAMESPACE = 361,
TEMPLATE = 362,
DELETE = 363,
MUTABLE = 364,
EXPLICIT = 365,
WCHAR_T = 366,
BOOL = 367,
USING = 368,
ASM_KEYWORD = 369,
EXPORT = 370,
VIRTUAL = 371,
INLINE = 372,
TYPENAME = 373,
DYNAMIC_CAST = 374,
STATIC_CAST = 375,
REINTERPRET_CAST = 376,
CONST_CAST = 377,
TYPEID = 378,
TOKEN = 379,
TYPE = 380,
LABEL = 381,
COMPL_FOR_SPECIAL1 = 382,
COMPL_FOR_SPECIAL2 = 383,
COMPL_THIS_PACKAGE_SPECIAL = 384,
COMPL_TYPE_NAME = 385,
COMPL_STRUCT_NAME = 386,
COMPL_STRUCT_REC_NAME = 387,
COMPL_UP_FUN_PROFILE = 388,
COMPL_ENUM_NAME = 389,
COMPL_LABEL_NAME = 390,
COMPL_OTHER_NAME = 391,
COMPL_CLASS_DEF_NAME = 392,
COMPL_FULL_INHERITED_HEADER = 393,
COMPL_TYPE_NAME0 = 394,
COMPL_TYPE_NAME1 = 395,
COMPL_PACKAGE_NAME0 = 396,
COMPL_EXPRESSION_NAME0 = 397,
COMPL_METHOD_NAME0 = 398,
COMPL_PACKAGE_NAME1 = 399,
COMPL_EXPRESSION_NAME1 = 400,
COMPL_METHOD_NAME1 = 401,
COMPL_CONSTRUCTOR_NAME0 = 402,
COMPL_CONSTRUCTOR_NAME1 = 403,
COMPL_CONSTRUCTOR_NAME2 = 404,
COMPL_CONSTRUCTOR_NAME3 = 405,
COMPL_STRUCT_REC_PRIM = 406,
COMPL_STRUCT_REC_SUPER = 407,
COMPL_QUALIF_SUPER = 408,
COMPL_SUPER_CONSTRUCTOR1 = 409,
COMPL_SUPER_CONSTRUCTOR2 = 410,
COMPL_THIS_CONSTRUCTOR = 411,
COMPL_IMPORT_SPECIAL = 412,
COMPL_VARIABLE_NAME_HINT = 413,
COMPL_CONSTRUCTOR_HINT = 414,
COMPL_METHOD_PARAM1 = 415,
COMPL_METHOD_PARAM2 = 416,
COMPL_METHOD_PARAM3 = 417,
COMPL_YACC_LEXEM_NAME = 418,
CPP_TOKENS_START = 419,
CPP_INCLUDE = 420,
CPP_INCLUDE_NEXT = 421,
CPP_DEFINE = 422,
CPP_IFDEF = 423,
CPP_IFNDEF = 424,
CPP_IF = 425,
CPP_ELSE = 426,
CPP_ENDIF = 427,
CPP_ELIF = 428,
CPP_UNDEF = 429,
CPP_PRAGMA = 430,
CPP_LINE = 431,
CPP_DEFINE0 = 432,
CPP_TOKENS_END = 433,
CPP_COLLATION = 434,
CPP_DEFINED_OP = 435,
EOI_TOKEN = 436,
CACHING1_TOKEN = 437,
OL_MARKER_TOKEN = 438,
OL_MARKER_TOKEN1 = 439,
OL_MARKER_TOKEN2 = 440,
TMP_TOKEN1 = 441,
TMP_TOKEN2 = 442,
CCC_OPER_PARENTHESIS = 443,
CCC_OPER_BRACKETS = 444,
MULTI_TOKENS_START = 445,
IDENTIFIER = 446,
CONSTANT = 447,
LONG_CONSTANT = 448,
FLOAT_CONSTANT = 449,
DOUBLE_CONSTANT = 450,
STRING_LITERAL = 451,
LINE_TOKEN = 452,
IDENT_TO_COMPLETE = 453,
CPP_MACRO_ARGUMENT = 454,
IDENT_NO_CPP_EXPAND = 455,
CHAR_LITERAL = 456,
LAST_TOKEN = 457,
} Lexem;

#endif
