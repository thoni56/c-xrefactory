#define TYPE_NAME 257
#define INC_OP 258
#define DEC_OP 259
#define LEFT_OP 260
#define RIGHT_OP 261
#define LE_OP 262
#define GE_OP 263
#define EQ_OP 264
#define NE_OP 265
#define AND_OP 266
#define OR_OP 267
#define MUL_ASSIGN 268
#define DIV_ASSIGN 269
#define MOD_ASSIGN 270
#define ADD_ASSIGN 271
#define SUB_ASSIGN 272
#define LEFT_ASSIGN 273
#define RIGHT_ASSIGN 274
#define AND_ASSIGN 275
#define XOR_ASSIGN 276
#define OR_ASSIGN 277
#define PTR_OP 278
#define ELLIPSIS 279
#define YACC_PERC 280
#define YACC_DPERC 281
#define STATIC 282
#define BREAK 283
#define CASE 284
#define CHAR 285
#define CONST 286
#define CONTINUE 287
#define DEFAULT 288
#define DO 289
#define DOUBLE 290
#define ELSE 291
#define FLOAT 292
#define FOR 293
#define GOTO 294
#define IF 295
#define INT 296
#define LONG 297
#define RETURN 298
#define SHORT 299
#define SWITCH 300
#define VOID 301
#define VOLATILE 302
#define WHILE 303
#define TYPEDEF 304
#define EXTERN 305
#define AUTO 306
#define REGISTER 307
#define SIGNED 308
#define UNSIGNED 309
#define STRUCT 310
#define UNION 311
#define ENUM 312
#define SIZEOF 313
#define RESTRICT 314
#define _ATOMIC 315
#define _BOOL 316
#define _THREADLOCAL 317
#define _NORETURN 318
#define INLINE 319
#define ASM_KEYWORD 320
#define ANONYMOUS_MODIFIER 321
#define TRUE_LITERAL 322
#define FALSE_LITERAL 323
#define TOKEN 324
#define TYPE 325
#define LABEL 326
#define COMPLETE_FOR_STATEMENT1 327
#define COMPLETE_FOR_STATEMENT2 328
#define COMPLETE_TYPE_NAME 329
#define COMPLETE_STRUCT_NAME 330
#define COMPLETE_STRUCT_MEMBER_NAME 331
#define COMPLETE_UP_FUN_PROFILE 332
#define COMPLETE_ENUM_NAME 333
#define COMPLETE_LABEL_NAME 334
#define COMPLETE_OTHER_NAME 335
#define COMPLETE_YACC_LEXEM_NAME 336
#define CPP_TOKENS_START 337
#define CPP_INCLUDE 338
#define CPP_INCLUDE_NEXT 339
#define CPP_DEFINE 340
#define CPP_IFDEF 341
#define CPP_IFNDEF 342
#define CPP_IF 343
#define CPP_ELSE 344
#define CPP_ENDIF 345
#define CPP_ELIF 346
#define CPP_UNDEF 347
#define CPP_PRAGMA 348
#define CPP_LINE 349
#define CPP_DEFINE0 350
#define CPP_TOKENS_END 351
#define CPP_COLLATION 352
#define CPP_DEFINED_OP 353
#define EOI_TOKEN 354
#define OL_MARKER_TOKEN 355
#define OL_MARKER_TOKEN1 356
#define OL_MARKER_TOKEN2 357
#define TMP_TOKEN1 358
#define TMP_TOKEN2 359
#define MULTI_TOKENS_START 360
#define IDENTIFIER 361
#define CONSTANT 362
#define LONG_CONSTANT 363
#define FLOAT_CONSTANT 364
#define DOUBLE_CONSTANT 365
#define STRING_LITERAL 366
#define CHAR_LITERAL 367
#define LINE_TOKEN 368
#define IDENT_TO_COMPLETE 369
#define CPP_MACRO_ARGUMENT 370
#define IDENT_NO_CPP_EXPAND 371
#define LAST_TOKEN 372
typedef union {
#include "yystype.h"
} YYSTYPE;
extern YYSTYPE c_yylval;