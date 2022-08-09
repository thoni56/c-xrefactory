#define TYPE_NAME 257
#define CLASS_NAME 258
#define TEMPLATE_NAME 259
#define CONVERSION_OP_ID_PREFIX 260
#define OPERATOR_IDENT 261
#define INC_OP 262
#define DEC_OP 263
#define LEFT_OP 264
#define RIGHT_OP 265
#define LE_OP 266
#define GE_OP 267
#define EQ_OP 268
#define NE_OP 269
#define AND_OP 270
#define OR_OP 271
#define MUL_ASSIGN 272
#define DIV_ASSIGN 273
#define MOD_ASSIGN 274
#define ADD_ASSIGN 275
#define SUB_ASSIGN 276
#define LEFT_ASSIGN 277
#define RIGHT_ASSIGN 278
#define AND_ASSIGN 279
#define XOR_ASSIGN 280
#define OR_ASSIGN 281
#define PTR_OP 282
#define ELIPSIS 283
#define URIGHT_OP 284
#define URIGHT_ASSIGN 285
#define YACC_PERC 286
#define YACC_DPERC 287
#define DPOINT 288
#define POINTM_OP 289
#define PTRM_OP 290
#define STATIC 291
#define BREAK 292
#define CASE 293
#define CHAR 294
#define CONST 295
#define CONTINUE 296
#define DEFAULT 297
#define DO 298
#define DOUBLE 299
#define ELSE 300
#define FLOAT 301
#define FOR 302
#define GOTO 303
#define IF 304
#define INT 305
#define LONG 306
#define RETURN 307
#define SHORT 308
#define SWITCH 309
#define VOID 310
#define VOLATILE 311
#define WHILE 312
#define TYPEDEF 313
#define EXTERN 314
#define AUTO 315
#define REGISTER 316
#define SIGNED 317
#define UNSIGNED 318
#define STRUCT 319
#define UNION 320
#define ENUM 321
#define SIZEOF 322
#define ANONYME_MOD 323
#define ABSTRACT 324
#define BOOLEAN 325
#define BYTE 326
#define CATCH 327
#define CLASS 328
#define EXTENDS 329
#define FINAL 330
#define FINALLY 331
#define IMPLEMENTS 332
#define IMPORT 333
#define INSTANCEOF 334
#define INTERFACE 335
#define NATIVE 336
#define NEW 337
#define PACKAGE 338
#define PRIVATE 339
#define PROTECTED 340
#define PUBLIC 341
#define SUPER 342
#define SYNCHRONIZED 343
#define THIS 344
#define THROW 345
#define THROWS 346
#define TRANSIENT 347
#define TRY 348
#define TRUE_LITERAL 349
#define FALSE_LITERAL 350
#define NULL_LITERAL 351
#define STRICTFP 352
#define ASSERT 353
#define FRIEND 354
#define OPERATOR 355
#define NAMESPACE 356
#define TEMPLATE 357
#define DELETE 358
#define MUTABLE 359
#define EXPLICIT 360
#define WCHAR_T 361
#define BOOL 362
#define USING 363
#define ASM_KEYWORD 364
#define EXPORT 365
#define VIRTUAL 366
#define INLINE 367
#define TYPENAME 368
#define DYNAMIC_CAST 369
#define STATIC_CAST 370
#define REINTERPRET_CAST 371
#define CONST_CAST 372
#define TYPEID 373
#define TOKEN 374
#define TYPE 375
#define LABEL 376
#define COMPL_FOR_SPECIAL1 377
#define COMPL_FOR_SPECIAL2 378
#define COMPL_THIS_PACKAGE_SPECIAL 379
#define COMPL_TYPE_NAME 380
#define COMPL_STRUCT_NAME 381
#define COMPL_STRUCT_REC_NAME 382
#define COMPL_UP_FUN_PROFILE 383
#define COMPL_ENUM_NAME 384
#define COMPL_LABEL_NAME 385
#define COMPL_OTHER_NAME 386
#define COMPL_CLASS_DEF_NAME 387
#define COMPL_FULL_INHERITED_HEADER 388
#define COMPL_TYPE_NAME0 389
#define COMPL_TYPE_NAME1 390
#define COMPL_PACKAGE_NAME0 391
#define COMPL_EXPRESSION_NAME0 392
#define COMPL_METHOD_NAME0 393
#define COMPL_PACKAGE_NAME1 394
#define COMPL_EXPRESSION_NAME1 395
#define COMPL_METHOD_NAME1 396
#define COMPL_CONSTRUCTOR_NAME0 397
#define COMPL_CONSTRUCTOR_NAME1 398
#define COMPL_CONSTRUCTOR_NAME2 399
#define COMPL_CONSTRUCTOR_NAME3 400
#define COMPL_STRUCT_REC_PRIM 401
#define COMPL_STRUCT_REC_SUPER 402
#define COMPL_QUALIF_SUPER 403
#define COMPL_SUPER_CONSTRUCTOR1 404
#define COMPL_SUPER_CONSTRUCTOR2 405
#define COMPL_THIS_CONSTRUCTOR 406
#define COMPL_IMPORT_SPECIAL 407
#define COMPL_VARIABLE_NAME_HINT 408
#define COMPL_CONSTRUCTOR_HINT 409
#define COMPL_METHOD_PARAM1 410
#define COMPL_METHOD_PARAM2 411
#define COMPL_METHOD_PARAM3 412
#define COMPL_YACC_LEXEM_NAME 413
#define CPP_TOKENS_START 414
#define CPP_INCLUDE 415
#define CPP_DEFINE 416
#define CPP_IFDEF 417
#define CPP_IFNDEF 418
#define CPP_IF 419
#define CPP_ELSE 420
#define CPP_ENDIF 421
#define CPP_ELIF 422
#define CPP_UNDEF 423
#define CPP_PRAGMA 424
#define CPP_LINE 425
#define CPP_DEFINE0 426
#define CPP_TOKENS_END 427
#define CPP_COLLATION 428
#define CPP_DEFINED_OP 429
#define EOI_TOKEN 430
#define CACHING1_TOKEN 431
#define OL_MARKER_TOKEN 432
#define OL_MARKER_TOKEN1 433
#define OL_MARKER_TOKEN2 434
#define TMP_TOKEN1 435
#define TMP_TOKEN2 436
#define CCC_OPER_PARENTHESIS 437
#define CCC_OPER_BRACKETS 438
#define MULTI_TOKENS_START 439
#define IDENTIFIER 440
#define CONSTANT 441
#define LONG_CONSTANT 442
#define FLOAT_CONSTANT 443
#define DOUBLE_CONSTANT 444
#define STRING_LITERAL 445
#define LINE_TOK 446
#define IDENT_TO_COMPLETE 447
#define CPP_MAC_ARG 448
#define IDENT_NO_CPP_EXPAND 449
#define CHAR_LITERAL 450
#define LAST_TOKEN 451
typedef union {
	int									integer;
	unsigned							unsign;
	S_symbol							*symbol;
	S_symbolList						*symbolList;
	S_typeModifiers						*typeModif;
	S_typeModifiersList					*typeModifList;
	S_freeTrail     					*trail;
	S_idIdent							*idIdent;
	S_idIdentList						*idlist;
	S_exprTokenType						exprType;
	S_intPair							intpair;
	S_whileExtractData					*whiledata;
	S_position							position;
	S_unsPositionPair					unsPositionPair;
	S_symbolPositionPair				symbolPositionPair;
	S_symbolPositionLstPair				symbolPositionLstPair;
	S_positionLst						*positionLst;
	S_typeModifiersListPositionLstPair 	typeModifiersListPositionLstPair;

	S_extRecFindStr							*erfs;

	S_bb_int								bbinteger;
	S_bb_unsigned							bbunsign;
	S_bb_symbol								bbsymbol;
	S_bb_symbolList							bbsymbolList;
	S_bb_typeModifiers						bbtypeModif;
	S_bb_typeModifiersList					bbtypeModifList;
	S_bb_freeTrail     						bbtrail;
	S_bb_idIdent							bbidIdent;
	S_bb_idIdentList						bbidlist;
	S_bb_exprTokenType						bbexprType;
	S_bb_intPair							bbintpair;
	S_bb_whileExtractData					bbwhiledata;
	S_bb_position							bbposition;
	S_bb_unsPositionPair					bbunsPositionPair;
	S_bb_symbolPositionPair					bbsymbolPositionPair;
	S_bb_symbolPositionLstPair				bbsymbolPositionLstPair;
	S_bb_positionLst						bbpositionLst;
	S_bb_typeModifiersListPositionLstPair 	bbtypeModifiersListPositionLstPair;
	S_bb_nestedConstrTokenType				bbnestedConstrTokenType;
} YYSTYPE;
extern YYSTYPE yylval;
