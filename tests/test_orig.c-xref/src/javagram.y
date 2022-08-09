/*
../../../byacc-1.9/yacc: 2 shift/reduce conflicts.
*/
%{

#define RECURSIVE

#define yylval javayylval

#define yyparse javayyparse
#define yylhs javayylhs
#define yylen javayylen
#define yydefred javayydefred
#define yydgoto javayydgoto
#define yysindex javayysindex
#define yyrindex javayyrindex
#define yytable javayytable
#define yycheck javayycheck
#define yyname javayyname
#define yyrule javayyrule
#define yydebug javayydebug
#define yynerrs javayynerrs
#define yyerrflag javayyerrflag
#define yychar javayychar
#define lastyystate javalastyystate
#define yyssp javayyssp
#define yyval javayyval
#define yyss javayyss
#define yyvs javayyvs
#define yygindex javayygindex
#define yyvsp javayyvsp


#include "stdinc.h"
#include "head.h"
#include "proto.h"      /*SBD*/
#include "protocol.h"
/* */

#define YYDEBUG 0
#define yyerror styyerror
#define yyErrorRecovery styyErrorRecovery

#define JslAddComposedType(ddd, ttt) jslAppendComposedType(&ddd->u.type, ttt)

#define JslImportSingleDeclaration(iname) {\
	S_symbol *sym;\
	jslClassifyAmbiguousTypeName(iname, &sym);\
	jslTypeSymbolDefinition(iname->idi.name, iname->next, TYPE_ADD_YES,ORDER_PREPEND, 1);\
}

/* Import on demand has to solve following situation (not handled by JSL) */
/* In case that there is a class and package differing only in letter case in name */
/* then even if classified to type it should be reclassified dep on the case */

#define JslImportOnDemandDeclaration(iname) {\
	S_symbol *sym;\
	int st;\
	char *origname;\
	st = jslClassifyAmbiguousTypeName(iname, &sym);\
	if (st == TypeStruct) {\
		javaLoadClassSymbolsFromFile(sym);\
		jslAddNestedClassesToJslTypeTab(sym, ORDER_APPEND);\
	} else {\
		javaMapDirectoryFiles2(iname,jslAddMapedImportTypeName,NULL,iname,NULL);\
	}\
}

#define CrTypeModifier(xxx,ttt) {\
		xxx = StackMemAlloc(S_typeModifiers);\
		FILLF_typeModifiers(xxx, ttt,f,( NULL,NULL) ,NULL,NULL);\
}

#define PrependModifier(xxx,ttt) {\
		S_typeModifiers *p;\
		p = StackMemAlloc(S_typeModifiers);\
		FILLF_typeModifiers(p, ttt,f,(NULL,NULL) ,NULL,xxx);\
		xxx = p;\
}

#define SetPrimitiveTypePos(res, typ) {\
		if (1 || SyntaxPassOnly()) {\
			XX_ALLOC(res, S_position);\
			*res = typ->p;\
		}\
}
#define SetAssignmentPos(res, tok) {\
		if (1 || SyntaxPassOnly()) {\
			XX_ALLOC(res.p, S_position);\
			*res.p = tok;\
		}\
}
#define PropagateBorns(res, beg, end) {res.b=beg.b; res.e=end.e;}
#define PropagateBornsIfRegularSyntaxPass(res, beg, end) {\
	if (RegularPass()) {\
		if (SyntaxPassOnly()) {PropagateBorns(res, beg, end);}\
	}\
}
#define SetNullBorns(res) {res.b=s_noPos; res.e=s_noPos;}

#define NULL_POS NULL

#define AddComposedType(ddd, ttt) appendComposedType(&ddd->u.type, ttt)
#define AllocIdCopy(copy, ident) {copy = StackMemAlloc(S_idIdent); *(copy) = *(ident);}


#define RegularPass() (s_jsl==NULL)
#define InSecondJslPass(xxxx) {if ((! RegularPass()) && s_jsl->pass==2) {xxxx}}
#define InFirstJslPass(xxxx) {if ((! RegularPass()) && s_jslPass==1) {xxxx}}

#define SyntaxPassOnly() (s_opt.cxrefs==OLO_GET_PRIMARY_START || s_opt.cxrefs==OLO_GET_PARAM_COORDINATES || s_opt.cxrefs==OLO_SYNTAX_PASS_ONLY || s_javaPreScanOnly)

#define ComputingPossibleParameterCompletion() (RegularPass() && (! SyntaxPassOnly()) && s_opt.taskRegime==RegimeEditServer && s_opt.cxrefs==OLO_COMPLETION)



%}
/*
   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
         Token definition part must be the same in all grammars
   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*/

/* ************************* SPECIALS ****************************** */
/* c+c++ */

%token TYPE_NAME 

/* c++-only */
%token CLASS_NAME TEMPLATE_NAME
%token CONVERSION_OP_ID_PREFIX OPERATOR_IDENT

/* ************************* OPERATORS ****************************** */
/* common */
%token INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token XOR_ASSIGN OR_ASSIGN

/* c-only */
%token PTR_OP ELIPSIS

/* java -only */
%token URIGHT_OP URIGHT_ASSIGN

/* yacc-only */
%token YACC_PERC YACC_DPERC

/* c++-only */
%token DPOINT POINTM_OP PTRM_OP

/* ************************** KEYWORDS ******************************** */

%token STATIC BREAK CASE CHAR CONST CONTINUE DEFAULT DO DOUBLE ELSE 
%token FLOAT FOR GOTO IF INT LONG RETURN SHORT SWITCH VOID VOLATILE WHILE

/* c-special */
%token TYPEDEF EXTERN AUTO REGISTER SIGNED UNSIGNED STRUCT UNION ENUM
%token SIZEOF
/* hmm */
%token ANONYME_MOD

/* java-special */
%token ABSTRACT BOOLEAN BYTE CATCH CLASS EXTENDS FINAL FINALLY 
%token IMPLEMENTS IMPORT INSTANCEOF INTERFACE NATIVE NEW
%token PACKAGE PRIVATE PROTECTED PUBLIC SUPER
%token SYNCHRONIZED THIS THROW THROWS TRANSIENT TRY
%token TRUE_LITERAL FALSE_LITERAL NULL_LITERAL
%token STRICTFP ASSERT

/* c++-special */
%token FRIEND OPERATOR NAMESPACE TEMPLATE DELETE MUTABLE EXPLICIT
%token WCHAR_T BOOL USING ASM_KEYWORD EXPORT VIRTUAL INLINE TYPENAME
%token DYNAMIC_CAST STATIC_CAST REINTERPRET_CAST CONST_CAST TYPEID

/* yacc-special */
%token TOKEN TYPE

/* gcc specials */
%token LABEL


/* ******************** COMPLETION SPECIAL TOKENS ******************** */

%token COMPL_FOR_SPECIAL1 COMPL_FOR_SPECIAL2
%token COMPL_THIS_PACKAGE_SPECIAL

/* c-only */
%token COMPL_TYPE_NAME
%token COMPL_STRUCT_NAME COMPL_STRUCT_REC_NAME COMPL_UP_FUN_PROFILE
%token COMPL_ENUM_NAME COMPL_LABEL_NAME COMPL_OTHER_NAME

/* java-only */
%token COMPL_CLASS_DEF_NAME COMPL_FULL_INHERITED_HEADER
%token COMPL_TYPE_NAME0
%token COMPL_TYPE_NAME1
%token COMPL_PACKAGE_NAME0 COMPL_EXPRESSION_NAME0 COMPL_METHOD_NAME0
%token COMPL_PACKAGE_NAME1 COMPL_EXPRESSION_NAME1 COMPL_METHOD_NAME1
%token COMPL_CONSTRUCTOR_NAME0 COMPL_CONSTRUCTOR_NAME1
%token COMPL_CONSTRUCTOR_NAME2 COMPL_CONSTRUCTOR_NAME3
%token COMPL_STRUCT_REC_PRIM COMPL_STRUCT_REC_SUPER COMPL_QUALIF_SUPER
%token COMPL_SUPER_CONSTRUCTOR1 COMPL_SUPER_CONSTRUCTOR2
%token COMPL_THIS_CONSTRUCTOR COMPL_IMPORT_SPECIAL
%token COMPL_VARIABLE_NAME_HINT COMPL_CONSTRUCTOR_HINT
%token COMPL_METHOD_PARAM1 COMPL_METHOD_PARAM2 COMPL_METHOD_PARAM3

/* yacc-special */
%token COMPL_YACC_LEXEM_NAME

/* ************************** CPP-TOKENS ****************************** */
/* c-only */
%token CPP_TOKENS_START
%token CPP_INCLUDE CPP_DEFINE CPP_IFDEF CPP_IFNDEF CPP_IF CPP_ELSE CPP_ENDIF
%token CPP_ELIF CPP_UNDEF
%token CPP_PRAGMA CPP_LINE 
%token CPP_DEFINE0      /* macro with no argument */
%token CPP_TOKENS_END

%token CPP_COLLATION	/* ## in macro body */
%token CPP_DEFINED_OP	/* defined(xxx) in #if */

/* ******************************************************************** */
/* special token signalizing end of program */
%token EOI_TOKEN

/* ******************************************************************** */
/* special tokens used for nontrivial caching !!!! not used */
%token CACHING1_TOKEN

/* ******************************************************************** */
%token OL_MARKER_TOKEN OL_MARKER_TOKEN1 OL_MARKER_TOKEN2

/* ******************************************************************** */
%token TMP_TOKEN1 TMP_TOKEN2
%token CCC_OPER_PARENTHESIS CCC_OPER_BRACKETS

/* ************************ MULTI TOKENS START ************************ */
%token MULTI_TOKENS_START

/* commons */
%token IDENTIFIER CONSTANT LONG_CONSTANT
%token FLOAT_CONSTANT DOUBLE_CONSTANT
%token STRING_LITERAL 
%token LINE_TOK
%token IDENT_TO_COMPLETE		/* identifier under cursor */

/* c-only */
%token CPP_MAC_ARG IDENT_NO_CPP_EXPAND

/* java-only */
%token CHAR_LITERAL

/* c++-only */

/* ****************************************************************** */

%token LAST_TOKEN


/* *************************************************************** */

%union {
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
}


/* ********************************************************* */

%type <bbidIdent> Identifier This Super New Package Import Throw Try Catch
%type <bbidIdent> ClassDeclaration InterfaceDeclaration
%type <bbinteger> DimExprs Dims_opt Dims
%type <bbinteger> _ncounter_ _nlabel_ _ngoto_ _nfork_ IfThenElseStatementPrefix
%type <bbintpair> ForStatementPrefix
%type <bbwhiledata> WhileStatementPrefix
%type <bbunsPositionPair> PrimitiveType NumericType IntegralType FloatingPointType
%type <bbunsPositionPair> AssignmentOperator
%type <bbunsign> Modifier Modifiers Modifiers_opt
%type <bbidlist> Name SimpleName QualifiedName PackageDeclaration_opt NewName
%type <bbidlist> SingleTypeImportDeclaration TypeImportOnDemandDeclaration
%type <bbsymbol> Type AssignementType ReferenceType ClassOrInterfaceType 
%type <bbsymbol> ExtendClassOrInterfaceType
%type <bbsymbol> ClassType InterfaceType
%type <bbsymbol> FormalParameter
%type <bbsymbol> VariableDeclarator VariableDeclaratorId
%type <bbsymbol> MethodHeader MethodDeclarator AbstractMethodDeclaration
%type <bbsymbol> FieldDeclaration ConstantDeclaration
%type <bbsymbol> ConstructorDeclarator
%type <bbsymbol> VariableDeclarators
%type <bbsymbol> LocalVariableDeclaration LocalVarDeclUntilInit
%type <bbsymbolList> Throws_opt ClassTypeList
%type <bbtypeModifiersListPositionLstPair> ArgumentList_opt ArgumentList
%type <bbsymbolPositionLstPair> FormalParameterList_opt FormalParameterList

%type <bbexprType> Primary PrimaryNoNewArray ClassInstanceCreationExpression
%type <bbexprType> ArrayCreationExpression FieldAccess MethodInvocation
%type <bbexprType> ArrayAccess PostfixExpression PostIncrementExpression
%type <bbexprType> PostDecrementExpression UnaryExpression
%type <bbexprType> PreIncrementExpression PreDecrementExpression
%type <bbexprType> UnaryExpressionNotPlusMinus CastExpression
%type <bbexprType> MultiplicativeExpression AdditiveExpression
%type <bbexprType> ShiftExpression RelationalExpression EqualityExpression
%type <bbexprType> AndExpression ExclusiveOrExpression InclusiveOrExpression
%type <bbexprType> ConditionalAndExpression ConditionalOrExpression
%type <bbexprType> ConditionalExpression AssignmentExpression Assignment
%type <bbexprType> LeftHandSide Expression ConstantExpression
%type <bbexprType> StatementExpression
%type <bbexprType> Literal VariableInitializer ArrayInitializer 
%type <bbsymbolPositionPair> ArrayType
%type <bbposition> MethodBody Block

%type <bbidIdent> IDENTIFIER THIS SUPER NEW PACKAGE IMPORT CLASS VOID
%type <bbidIdent> BOOLEAN CHAR LONG INT SHORT BYTE DOUBLE FLOAT THROW TRY CATCH
%type <bbidIdent> NULL_LITERAL
%type <bbinteger> CONSTANT

%type <bbposition> STRING_LITERAL '(' ',' ')' ';' '{' '}' '[' ']' ':' '+' '-' '~' '!' '=' 
%type <bbposition> INC_OP DEC_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN SUB_ASSIGN 
%type <bbposition> LEFT_ASSIGN RIGHT_ASSIGN URIGHT_ASSIGN AND_ASSIGN XOR_ASSIGN OR_ASSIGN 

%type <bbposition> TRUE_LITERAL FALSE_LITERAL LONG_CONSTANT FLOAT_CONSTANT
%type <bbposition> DOUBLE_CONSTANT CHAR_LITERAL STRING_LITERAL

%type <bbposition> ImportDeclarations ImportDeclarations_opt 
%type <bbposition> LabelDefininigIdentifier LabelUseIdentifier
%type <bbposition> TypeDeclarations_opt TypeDeclarations TypeDeclaration
%type <bbposition> PUBLIC PROTECTED PRIVATE STATIC ABSTRACT FINAL NATIVE
%type <bbposition> SYNCHRONIZED STRICTFP TRANSIENT VOLATILE EXTENDS
%type <bbposition> IMPLEMENTS THROWS INTERFACE ASSERT IF ELSE SWITCH
%type <bbposition> CASE DEFAULT WHILE DO FOR BREAK CONTINUE RETURN
%type <bbposition> FINALLY 
%type <bbposition> ClassBody FunctionInnerClassDeclaration Super_opt 
%type <bbposition> Interfaces_opt InterfaceTypeList ClassBodyDeclarations
%type <bbposition> ClassBodyDeclaration ClassMemberDeclaration ClassInitializer
%type <bbposition> ConstructorDeclaration ConstructorBody ExplicitConstructorInvocation
%type <bbposition> MethodDeclaration InterfaceMemberDeclarations InterfaceBody
%type <bbposition> ExtendsInterfaces_opt ExtendsInterfaces InterfaceMemberDeclaration
%type <bbposition> BlockStatements BlockStatement LocalVariableDeclarationStatement
%type <bbposition> FunctionInnerClassDeclaration Statement StatementWithoutTrailingSubstatement
%type <bbposition> LabeledStatement IfThenStatement IfThenElseStatement WhileStatement
%type <bbposition> ForStatement StatementNoShortIf LabeledStatementNoShortIf
%type <bbposition> IfThenElseStatementNoShortIf WhileStatementNoShortIf ForStatementNoShortIf
%type <bbposition> EmptyStatement ExpressionStatement SwitchStatement DoStatement
%type <bbposition> BreakStatement ContinueStatement ReturnStatement SynchronizedStatement
%type <bbposition> ThrowStatement TryStatement AssertStatement
%type <bbposition> SwitchBlockStatementGroups SwitchBlockStatementGroup SwitchLabels
%type <bbposition> SwitchLabel ForInit_opt StatementExpressionList
%type <bbposition> ForUpdate_opt TryCatches Catches Finally CatchClause
%type <bbposition> VariableInitializers SwitchBlock MaybeExpression ForKeyword
%type <bbposition> ForStatementBody ForStatementNoShortIfBody StatementExpressionList
%type <bbposition> ForUpdate_opt DimExpr
%type <bbnestedConstrTokenType> NestedConstructorInvocation

%type <erfs> _erfs_


%start Goal

%%

Goal:	CompilationUnit
	;

/* *************************** Literals ********************************* */

Literal:
		TRUE_LITERAL		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					CrTypeModifier($$.d.t,TypeBoolean);
					$$.d.r = NULL; 
				} else {
					$$.d.pp = &s_noPos;
					PropagateBorns($$, $1, $1);
				}
			}
		}
	|	FALSE_LITERAL		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					CrTypeModifier($$.d.t,TypeBoolean);
					$$.d.r = NULL; 
				} else {
					$$.d.pp = &s_noPos;
					PropagateBorns($$, $1, $1);
				}
			}
		}
	|	CONSTANT			{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					CrTypeModifier($$.d.t,TypeInt);
					$$.d.r = NULL; 
				} else {
					$$.d.pp = &s_noPos;
					PropagateBorns($$, $1, $1);
				}
			}
		}
	|	LONG_CONSTANT		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					CrTypeModifier($$.d.t,TypeLong);
					$$.d.r = NULL; 
				} else {
					$$.d.pp = &s_noPos;
					PropagateBorns($$, $1, $1);
				}
			}
		}
	|	FLOAT_CONSTANT		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					CrTypeModifier($$.d.t,TypeFloat);
					$$.d.r = NULL; 
				} else {
					$$.d.pp = &s_noPos;
					PropagateBorns($$, $1, $1);
				}
			}
		}
	|	DOUBLE_CONSTANT		{  
			if (RegularPass()) {
				CrTypeModifier($$.d.t,TypeDouble);
				$$.d.r = NULL; 
				$$.d.pp = &s_noPos;
				if (SyntaxPassOnly()) {PropagateBorns($$, $1, $1);}
			}
		}
	|	CHAR_LITERAL		{  
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					CrTypeModifier($$.d.t,TypeChar);
					$$.d.r = NULL; 
				} else {
					$$.d.pp = &s_noPos;
					PropagateBorns($$, $1, $1);
				}
			}
		}
	|	STRING_LITERAL		{  
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = &s_javaStringModifier;
					$$.d.r = NULL; 
				} else {
					XX_ALLOC($$.d.pp, S_position);
					*$$.d.pp = $1.d;
					PropagateBorns($$, $1, $1);
				}
			}
		}
	|	NULL_LITERAL		{  
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					CrTypeModifier($$.d.t,TypeNull);
					$$.d.r = NULL; 
				} else {
					XX_ALLOC($$.d.pp, S_position);
					*$$.d.pp = $1.d->p;
					PropagateBorns($$, $1, $1);
				}
			}
		}
	;

/* ************************* Types, Values, Variables ******************* */

Type:
		PrimitiveType	{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d = typeSpecifier1($1.d.u);
					s_cps.lastDeclaratorType = NULL;
				} else {
					PropagateBorns($$, $1, $1);
				}
			};
			InSecondJslPass({$$.d = jslTypeSpecifier1($1.d.u);});
		}
	|	ReferenceType	{
			$$.d = $1.d;
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	;

PrimitiveType:
		NumericType		/* $$ = $1 */ 
	|	BOOLEAN			{ 
			$$.d.u  = TypeBoolean; 
			if (RegularPass()) {
				SetPrimitiveTypePos($$.d.p, $1.d);
				PropagateBornsIfRegularSyntaxPass($$, $1, $1);
			}
		}
	;

NumericType:
		IntegralType			/* $$ = $1 */ 
	|	FloatingPointType		/* $$ = $1 */
	;

IntegralType:
		BYTE			{    
			$$.d.u  = TypeByte;
			if (RegularPass()) SetPrimitiveTypePos($$.d.p, $1.d);
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	|	SHORT			{    
			$$.d.u  = TypeShort;
			if (RegularPass()) SetPrimitiveTypePos($$.d.p, $1.d);
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	|	INT				{    
			$$.d.u  = TypeInt;
			if (RegularPass()) SetPrimitiveTypePos($$.d.p, $1.d);
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	|	LONG			{    
			$$.d.u  = TypeLong;
			if (RegularPass()) SetPrimitiveTypePos($$.d.p, $1.d);
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	|	CHAR			{    
			$$.d.u  = TypeChar;
			if (RegularPass()) SetPrimitiveTypePos($$.d.p, $1.d);
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	;

FloatingPointType:
		FLOAT			{    
			$$.d.u  = TypeFloat;
			if (RegularPass()) SetPrimitiveTypePos($$.d.p, $1.d);
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	|	DOUBLE			{    
			$$.d.u  = TypeDouble;
			if (RegularPass()) SetPrimitiveTypePos($$.d.p, $1.d);
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	;

ReferenceType:
		ClassOrInterfaceType		/* { $$.d = $1.d; } */
	|	ArrayType					{
			$$.d = $1.d.s; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	;

ClassOrInterfaceType:
		Name			{   
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					javaClassifyToTypeName($1.d,UsageUsed, &$$.d, USELESS_FQT_REFS_ALLOWED);
					$$.d = javaTypeNameDefinition($1.d);
					assert($$.d->u.type);
					s_cps.lastDeclaratorType = $$.d->u.type->u.t;
				} else {
					PropagateBorns($$, $1, $1);
				}
			};
			InSecondJslPass({
				S_symbol *str;
				jslClassifyAmbiguousTypeName($1.d, &str);
				$$.d = jslTypeNameDefinition($1.d);
			});
		}
	|	CompletionTypeName	{ /* rule never reduced */ }
	;

/* following is the same, just to distinguish type after EXTEND keyword */
ExtendClassOrInterfaceType:
		Name			{   
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					javaClassifyToTypeName($1.d, USAGE_EXTEND_USAGE, &$$.d, USELESS_FQT_REFS_ALLOWED);
					$$.d = javaTypeNameDefinition($1.d);
				} else {
					PropagateBorns($$, $1, $1);
				}
			};
			InSecondJslPass({
				S_symbol *str;
				jslClassifyAmbiguousTypeName($1.d, &str);
				$$.d = jslTypeNameDefinition($1.d);
			});
		}
	|	CompletionTypeName	{ /* rule never reduced */ }
	;

ClassType:
		ClassOrInterfaceType		/* { $$.d = $1.d; } */
	;

InterfaceType:
		ClassOrInterfaceType		/* { $$.d = $1.d; } */
	;

ArrayType:
		PrimitiveType '[' ']'		{   
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.s = typeSpecifier1($1.d.u);
					$$.d.s->u.type = prependComposedType($$.d.s->u.type, TypeArray);
				} else {
					PropagateBorns($$, $1, $3);
				}
				$$.d.p = $1.d.p;
				s_cps.lastDeclaratorType = NULL;
			};
			InSecondJslPass({
				$$.d.s = jslTypeSpecifier1($1.d.u);
				$$.d.s->u.type = jslPrependComposedType($$.d.s->u.type, TypeArray);
			});
		}
	|	Name '[' ']'				{   
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					javaClassifyToTypeName($1.d,UsageUsed, &($$.d.s), USELESS_FQT_REFS_ALLOWED);
					$$.d.s = javaTypeNameDefinition($1.d);
					assert($$.d.s && $$.d.s->u.type);
					s_cps.lastDeclaratorType = $$.d.s->u.type->u.t;
					$$.d.s->u.type = prependComposedType($$.d.s->u.type, TypeArray);
				} else {
					PropagateBorns($$, $1, $3);
				}
				$$.d.p = javaGetNameStartingPosition($1.d);
			};
			InSecondJslPass({
				S_symbol *ss;
				jslClassifyAmbiguousTypeName($1.d, &ss);
				$$.d.s = jslTypeNameDefinition($1.d);
				$$.d.s->u.type = jslPrependComposedType($$.d.s->u.type, TypeArray);
			});
		}
	|	ArrayType '[' ']'			{   
			if (RegularPass()) {
				$$.d = $1.d;
				if (! SyntaxPassOnly()) {
					$$.d.s->u.type = prependComposedType($$.d.s->u.type, TypeArray);
				} else {
					PropagateBorns($$, $1, $3);
				}
			};
			InSecondJslPass({
				$$.d = $1.d;
				$$.d.s->u.type = jslPrependComposedType($$.d.s->u.type, TypeArray);
			});
		}
	| 	CompletionTypeName '[' ']'	{ /* rule never used */ }
	;

/* ****************************** Names **************************** */

Identifier:	IDENTIFIER			{
				if (RegularPass()) AllocIdCopy($$.d,$1.d);
				PropagateBornsIfRegularSyntaxPass($$, $1, $1);
			};
This:		THIS				{
				if (RegularPass()) AllocIdCopy($$.d,$1.d);
				PropagateBornsIfRegularSyntaxPass($$, $1, $1);
			};
Super:		SUPER				{
				if (RegularPass()) AllocIdCopy($$.d,$1.d);
				PropagateBornsIfRegularSyntaxPass($$, $1, $1);
			};
New:		NEW					{
				if (RegularPass()) AllocIdCopy($$.d,$1.d);
				PropagateBornsIfRegularSyntaxPass($$, $1, $1);
			};
Import:		IMPORT				{
				if (RegularPass()) AllocIdCopy($$.d,$1.d);
				PropagateBornsIfRegularSyntaxPass($$, $1, $1);
			};
Package:	PACKAGE				{
				if (RegularPass()) AllocIdCopy($$.d,$1.d);
				PropagateBornsIfRegularSyntaxPass($$, $1, $1);
			};
Throw:		THROW				{
				if (RegularPass()) AllocIdCopy($$.d,$1.d);
				PropagateBornsIfRegularSyntaxPass($$, $1, $1);
			};
Try:		TRY					{
				if (RegularPass()) AllocIdCopy($$.d,$1.d);
				PropagateBornsIfRegularSyntaxPass($$, $1, $1);
			};
Catch:		CATCH				{
				if (RegularPass()) AllocIdCopy($$.d,$1.d);
				PropagateBornsIfRegularSyntaxPass($$, $1, $1);
			};


Name:
		SimpleName				{
			$$.d = $1.d;
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					assert(s_javaStat);
					s_javaStat->lastParsedName = $1.d; 
				} else {
					PropagateBorns($$, $1, $1);
					javaCheckForPrimaryStart(&$1.d->idi.p, &$1.d->idi.p);
					javaCheckForStaticPrefixStart(&$1.d->idi.p, &$1.d->idi.p);
				}
			};
		}
	|	QualifiedName			{
			$$.d = $1.d;
			if (RegularPass()) { 
				if (! SyntaxPassOnly()) {
					assert(s_javaStat);
					s_javaStat->lastParsedName = $1.d; 
				} else {
					PropagateBorns($$, $1, $1);
					javaCheckForPrimaryStartInNameList($1.d, javaGetNameStartingPosition($1.d));
					javaCheckForStaticPrefixInNameList($1.d, javaGetNameStartingPosition($1.d));
				}
			};
		}
	;

SimpleName:
		IDENTIFIER				{
			$$.d = StackMemAlloc(S_idIdentList);
			FILL_idIdentList($$.d, *$1.d, $1.d->name, TypeDefault, NULL);
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	;

QualifiedName:
		Name '.' IDENTIFIER		{
			$$.d = StackMemAlloc(S_idIdentList);
			FILL_idIdentList($$.d, *$3.d, $3.d->name, TypeDefault, $1.d);
			PropagateBornsIfRegularSyntaxPass($$, $1, $3);
		}
	;

/* following rules are never reduced, they keep completion information */

CompletionTypeName:
		COMPL_TYPE_NAME0
	|	Name '.' COMPL_TYPE_NAME1
	;

CompletionFQTypeName:
		COMPL_PACKAGE_NAME0
	|	Name '.' COMPL_TYPE_NAME1
	;

CompletionPackageName:
		COMPL_PACKAGE_NAME0
	|	Name '.' COMPL_PACKAGE_NAME1
	;

CompletionExpressionName:
		COMPL_EXPRESSION_NAME0
	|	Name '.' COMPL_EXPRESSION_NAME1
	;

CompletionConstructorName:
		COMPL_CONSTRUCTOR_NAME0
	|	Name '.' COMPL_CONSTRUCTOR_NAME1
	;

/*
CompletionMethodName:
		COMPL_METHOD_NAME0
	|	Name '.' COMPL_METHOD_NAME1
	;
*/

LabelDefininigIdentifier:
		IDENTIFIER 			{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					labelReference($1.d,UsageDefined);
				} else {
					PropagateBorns($$, $1, $1);
				}
			};
		}
	|	COMPL_LABEL_NAME		{ assert(0); /* token never used */ }
	;

LabelUseIdentifier:
		IDENTIFIER 			{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					labelReference($1.d,UsageUsed);
				} else {
					PropagateBorns($$, $1, $1);
				}
			};
		}
	|	COMPL_LABEL_NAME		{ assert(0); /* token never used */ }
	;

/* ****************************** packages ************************* */

CompilationUnit: {
			if (RegularPass()) {
				assert(s_javaStat);
				*s_javaStat = s_initJavaStat;
				s_javaThisPackageName = "";      // preset for case if copied somewhere
			};
		} PackageDeclaration_opt {
			if (RegularPass()) {
				if ($2.d == NULL) {	/* anonymous package */
					s_javaThisPackageName = "";
				} else {
					javaClassifyToPackageName($2.d);
					s_javaThisPackageName = javaCreateComposedName(NULL,$2.d,'/',
																   NULL,NULL,0);
				}
				s_javaStat->currentPackage = s_javaThisPackageName;
				if (! SyntaxPassOnly()) {

					int 			i,j,packlen;
					char 			*cdir, *fname;
					S_jslTypeTab	*jsltypeTab;
					struct stat 	st;
					// it is important to know the package before everything
					// else, as it must be set on class adding in order to set
					// isinCurrentPackage field. !!!!!!!!!!!!!!!!
					// this may be problem for CACHING !!!!
					if ($2.d == NULL) {	/* anonymous package */
						s_javaStat->className = NULL;
						for(i=0,j=0; cFile.fileName[i]; i++) {
							if (cFile.fileName[i] == SLASH) j=i;
						}
						XX_ALLOCC(cdir, j+1, char);  // I prefer this
						//&SM_ALLOCC(ftMemory, cdir, j+1, char);  // will exhauste ftmemory
						strncpy(cdir,cFile.fileName,j); cdir[j]=0;
						s_javaStat->unNamedPackageDir = cdir;
						javaCheckIfPackageDirectoryIsInClassOrSourcePath(cdir);
					} else {
						javaAddPackageDefinition($2.d);
						s_javaStat->className = $2.d;
						for(i=0,j=0; cFile.fileName[i]; i++) {
							if (cFile.fileName[i] == SLASH) j=i;
						}
						packlen = strlen(s_javaThisPackageName);
						if (j>packlen && fnnCmp(s_javaThisPackageName,&cFile.fileName[j-packlen],packlen)==0){
							XX_ALLOCC(cdir, j-packlen, char); // I prefer this
							//&SM_ALLOCC(ftMemory, cdir, j-packlen, char);  // will exhauste ftmemory
							strncpy(cdir, cFile.fileName, j-packlen-1); cdir[j-packlen-1]=0;
							s_javaStat->namedPackageDir = cdir;
							s_javaStat->currentPackage = "";
							javaCheckIfPackageDirectoryIsInClassOrSourcePath(cdir);
						} else {
							if (s_opt.taskRegime != RegimeEditServer) {
								warning(ERR_ST, "package name does not match directory name");
							}
						}
					}
					javaParsingInitializations();
					// make first and second pass through file
					assert(s_jsl == NULL); // no nesting
					XX_ALLOC(jsltypeTab, S_jslTypeTab);
					jslTypeTabInit(jsltypeTab, MAX_JSL_SYMBOLS);
					javaReadSymbolFromSourceFileInit(s_olOriginalFileNumber, 
													 jsltypeTab);
	
					fname = s_fileTab.tab[s_olOriginalFileNumber]->name;
#if 1 //I_DO_NOT_KNOW, to take also symbols from lastly saved file???
					if (s_opt.taskRegime == RegimeEditServer 
						&& s_ropt.refactoringRegime!=RegimeRefactory) {
						// this must be before reading 's_olOriginalComFile' !!!
						if (statb(fname, &st)==0) {
							javaReadSymbolsFromSourceFileNoFreeing(fname, fname);
						}
					}
#endif
					// this must be last reading of this class before parsing
					if (statb(s_fileTab.tab[s_olOriginalComFileNumber]->name, &st)==0) {
						javaReadSymbolsFromSourceFileNoFreeing(
							s_fileTab.tab[s_olOriginalComFileNumber]->name, fname);
					}
	
					javaReadSymbolFromSourceFileEnd();
					javaAddJslReadedTopLevelClasses(jsltypeTab);
					assert(s_jsl == NULL);
				}
				// -----------------------------------------------
			} else { 
				// -----------------------------------------------
				// jsl pass initialisation
				char 			*pname;
				S_jslClassStat 	*ss;
				char			ppp[MAX_FILE_NAME_SIZE];
				s_jsl->waitList = NULL;
				if ($2.d != NULL) {
					javaClassifyToPackageName($2.d);
				}
				javaCreateComposedName(NULL,$2.d,'/',NULL,ppp,MAX_FILE_NAME_SIZE);
				XX_ALLOCC(pname, strlen(ppp)+1, char);
				strcpy(pname, ppp);
				XX_ALLOC(ss, S_jslClassStat);
				FILL_jslClassStat(ss, $2.d, NULL, pname, 0, 0, NULL);
				s_jsl->classStat = ss;
				InSecondJslPass({
					char 		cdir[MAX_FILE_NAME_SIZE];
					int 		i;
					int			j;
					/* add this package types */
					if ($2.d == NULL) {	/* anonymous package */
						for(i=0,j=0; cFile.fileName[i]; i++) {
							if (cFile.fileName[i] == SLASH) j=i;
						}
						strncpy(cdir,cFile.fileName,j); 
						cdir[j]=0;
						mapDirectoryFiles(cdir,
									jslAddMapedImportTypeName,ALLOW_EDITOR_FILES, "",
									"", NULL, NULL, NULL);
						// why this is there, it makes problem when moving a class
						// it stays in fileTab and there is a clash!
						// [2/8/2003] Maybe I should put it out
						jslAddAllPackageClassesFromFileTab(NULL);
					} else {
						javaMapDirectoryFiles2($2.d,jslAddMapedImportTypeName,
												NULL,$2.d,NULL);
						// why this is there, it makes problem when moving a class
						// it stays in fileTab and there is a clash!
						// [2/8/2003] Maybe I should put it out
						jslAddAllPackageClassesFromFileTab($2.d);
					}
				});
				InSecondJslPass({
					/* add java/lang package types */
					javaMapDirectoryFiles2(s_javaLangName, 
							jslAddMapedImportTypeName, NULL, s_javaLangName, NULL);
				});
/*&fprintf(dumpOut," [jsl] current package == '%s'\n", pname);&*/
			}
		} ImportDeclarations_opt {
			if (RegularPass()) {
				/* add this package types after imports! */
			} else {
				// jsl pass initialisation
				/* there were original this package types add, but now this is
    	           a useless semantic action */
			}
		}
		TypeDeclarations_opt
	|	InCachingRecovery
	;

ImportDeclarations_opt:						{
			SetNullBorns($$);
		}
	|	ImportDeclarations					/* $$ = $1; */
	;

ImportDeclarations:
		SingleTypeImportDeclaration			{
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
			InSecondJslPass({
				JslImportSingleDeclaration($1.d);
			})
		}
	|	TypeImportOnDemandDeclaration			{
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
			InSecondJslPass({
				JslImportOnDemandDeclaration($1.d);
			})
		}
	|	ImportDeclarations SingleTypeImportDeclaration			{
			PropagateBornsIfRegularSyntaxPass($$, $1, $2);
			InSecondJslPass({
				JslImportSingleDeclaration($2.d);
			})
		}
	|	ImportDeclarations TypeImportOnDemandDeclaration		{
			PropagateBornsIfRegularSyntaxPass($$, $1, $2);
			InSecondJslPass({
				JslImportOnDemandDeclaration($2.d);
			})
		}
	;

/* this is original from JSL
ImportDeclarations:
		ImportDeclaration
	|	ImportDeclarations ImportDeclaration		{
			PropagateBornsIfRegularSyntaxPass($$, $1, $2);
		}
	;

ImportDeclaration:
		SingleTypeImportDeclaration
	|	TypeImportOnDemandDeclaration
	;
*/

SingleTypeImportDeclaration:
		Import Name ';'						{
			$$.d = $2.d;
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					S_reference *lastUselessRef;
					S_symbol *str;
					// it was type or packege, but I thing this would be better
					lastUselessRef = javaClassifyToTypeName($2.d, UsageUsed, &str, USELESS_FQT_REFS_DISALLOWED);
					// last useless reference is not useless here!
					if (lastUselessRef!=NULL) lastUselessRef->usg = s_noUsage;
					s_cps.lastImportLine = $1.d->p.line;
					if ($2.d->next!=NULL) {
						javaAddImportConstructionReference(&$2.d->next->idi.p, &$1.d->p, UsageDefined);
					}
				} else {
					PropagateBorns($$, $1, $3);
				}
			}
		}
	|	Import CompletionFQTypeName ';'		{ /* rule never used */ }
	|	Import COMPL_IMPORT_SPECIAL ';'		{ /* rule never used */ }
	;

TypeImportOnDemandDeclaration:
		Import Name '.' '*' ';'				{
			$$.d = $2.d;
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					S_symbol	*str;
					S_typeModifiers		*expr;
					S_idIdentList		*ii;
					S_reference			*rr, *lastUselessRef;
					int					st;
					st = javaClassifyAmbiguousName($2.d, NULL,&str,&expr,&rr,
												   &lastUselessRef, USELESS_FQT_REFS_DISALLOWED,
												   CLASS_TO_TYPE,UsageUsed);
					if (lastUselessRef!=NULL) lastUselessRef->usg = s_noUsage;
					s_cps.lastImportLine = $1.d->p.line;
					javaAddImportConstructionReference(&$2.d->idi.p, &$1.d->p, UsageDefined);
				} else {
					PropagateBorns($$, $1, $5);
				}
			}
		}
	|	Import CompletionPackageName '.' '*' ';'	{ /* rule never used */ }
	;

TypeDeclarations_opt:							{
			SetNullBorns($$);
		}
	|	TypeDeclarations _bef_					{
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	;

TypeDeclarations:
		_bef_ TypeDeclaration					{
			PropagateBornsIfRegularSyntaxPass($$, $2, $2);
		}
	|	TypeDeclarations _bef_ TypeDeclaration					{
			PropagateBornsIfRegularSyntaxPass($$, $1, $3);
		}
	;

PackageDeclaration_opt:							{ 
			$$.d = NULL;
			if (RegularPass()) {
				s_cps.lastImportLine = 0;
				SetNullBorns($$);
			}
		}
	|	Package Name ';'						{ 
			$$.d = $2.d;
			if (RegularPass()) {
				s_cps.lastImportLine = $1.d->p.line;
				PropagateBornsIfRegularSyntaxPass($$, $1, $3);
			}
		}
	|	Package error							{ 
			$$.d = NULL; 
			if (RegularPass()) {
				s_cps.lastImportLine = $1.d->p.line;
				PropagateBornsIfRegularSyntaxPass($$, $1, $1);
			}
		}
	|	Package CompletionPackageName ';'		{ /* rule never used */ }
	|	PACKAGE COMPL_THIS_PACKAGE_SPECIAL ';'		{ /* rule never used */ }
	;

TypeDeclaration:
		ClassDeclaration		{
			if (RegularPass()) {
				javaSetClassSourceInformation(s_javaThisPackageName, $1.d);
				PropagateBornsIfRegularSyntaxPass($$, $1, $1);
			}
		}
	|	InterfaceDeclaration	{
			if (RegularPass()) {
				javaSetClassSourceInformation(s_javaThisPackageName, $1.d);
				PropagateBornsIfRegularSyntaxPass($$, $1, $1);
			}
		}
	|	';'						{}
	|	error					{}
	;

/* ************************ LALR special ************************** */

Modifiers_opt:					{ 
			$$.d = ACC_DEFAULT;
			SetNullBorns($$);
		}
	|	Modifiers				{ 
			$$.d = $1.d; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	;

Modifiers:
		Modifier				/* { $$ = $1; } */
	|	Modifiers Modifier		{ 
			$$.d = $1.d | $2.d; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $2);
		}
	;

Modifier:
		PUBLIC			{ $$.d = ACC_PUBLIC; PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	PROTECTED		{ $$.d = ACC_PROTECTED; PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	PRIVATE			{ $$.d = ACC_PRIVATE; PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	STATIC			{ $$.d = ACC_STATIC; PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	ABSTRACT		{ $$.d = ACC_ABSTRACT; PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	FINAL			{ $$.d = ACC_FINAL; PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	NATIVE			{ $$.d = ACC_NATIVE; PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	SYNCHRONIZED	{ $$.d = ACC_SYNCHRONIZED; PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	STRICTFP		{ $$.d = 0; PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	TRANSIENT		{ $$.d = ACC_TRANSIENT; PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	VOLATILE		{ $$.d = 0; PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	;

/* *************************** Classes *************************** */

InCachingRecovery:
		CACHING1_TOKEN ClassBody
	;

/* **************** Class Declaration ****************** */

/* !!!!!!!! here is a problem if there are two in the same unit !!!! 
TopLevelClassDeclaration:
		Modifiers_opt CLASS Identifier 
			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						$<trail>$ = newClassDefinitionBegin($3.d, $1.d, NULL);
					}
				}
			}
		Super_opt Interfaces_opt ClassBody		{
			PropagateBornsIfRegularSyntaxPass($$, $1, $7);
		}
	|	Modifiers_opt CLASS error ClassBody		{
			PropagateBornsIfRegularSyntaxPass($$, $1, $4);
		}
	;
*/

ClassDeclaration:
		Modifiers_opt CLASS Identifier {
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						$<trail>$ = newClassDefinitionBegin($3.d, $1.d, NULL);
					}
				} else {
					jslNewClassDefinitionBegin($3.d, $1.d, NULL, CPOS_ST);
					jslAddDefaultConstructor(s_jsl->classStat->thisClass);
				}
			} Super_opt Interfaces_opt {
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						javaAddSuperNestedClassToSymbolTab(s_javaStat->thisClass);
					}
				} else {
					jslAddSuperNestedClassesToJslTypeTab(s_jsl->classStat->thisClass);
				}
			} ClassBody {
				if (RegularPass()) {
					$$.d = $3.d;
					if (! SyntaxPassOnly()) {
						newClassDefinitionEnd($<trail>4);
					} else {
						PropagateBorns($$, $1, $8);
						if ($$.b.file == s_noneFileIndex) PropagateBorns($$, $2, $$);
						if (POSITION_IS_BETWEEN_IN_THE_SAME_FILE($$.b, s_cxRefPos, $$.e)
							&& s_spp[SPP_CLASS_DECLARATION_BEGIN_POSITION].file == s_noneFileIndex) {
							s_spp[SPP_CLASS_DECLARATION_BEGIN_POSITION] = $$.b;
							s_spp[SPP_CLASS_DECLARATION_TYPE_BEGIN_POSITION] = $2.b;
							s_spp[SPP_CLASS_DECLARATION_TYPE_END_POSITION] = $2.e;
							s_spp[SPP_CLASS_DECLARATION_END_POSITION] = $$.e;
						}
					}
				} else {
					jslNewClassDefinitionEnd();
				}
			}
	|	Modifiers_opt CLASS Identifier 
			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						$<trail>$ = newClassDefinitionBegin($3.d, $1.d, NULL);
					}
				} else {
					jslNewClassDefinitionBegin($3.d, $1.d, NULL, CPOS_ST);
				}
			}
		error ClassBody
			{
				if (RegularPass()) {
					$$.d = $3.d;
					if (! SyntaxPassOnly()) {
						newClassDefinitionEnd($<trail>4);
					} else {
						PropagateBorns($$, $1, $6);
						if ($$.b.file == s_noneFileIndex) PropagateBorns($$, $2, $6);
					}
				} else {
					jslNewClassDefinitionEnd();
				}
			}
	|	Modifiers_opt CLASS COMPL_CLASS_DEF_NAME	{ /* never used */ }
	;


FunctionInnerClassDeclaration:
		Modifiers_opt CLASS Identifier {
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						$<trail>$ = newClassDefinitionBegin($3.d, $1.d, NULL);
					}
				} else {
					jslNewClassDefinitionBegin($3.d, $1.d, NULL, CPOS_FUNCTION_INNER);
				}
			} Super_opt Interfaces_opt {
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						javaAddSuperNestedClassToSymbolTab(s_javaStat->thisClass);
					}
				} else {
					jslAddSuperNestedClassesToJslTypeTab(s_jsl->classStat->thisClass);
				}
			} ClassBody {
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						newClassDefinitionEnd($<trail>4);
					} else {
						PropagateBorns($$, $1, $8);
						if ($$.b.file == s_noneFileIndex) PropagateBorns($$, $2, $8);
						if (POSITION_EQ(s_cxRefPos, $3.d->p)) {
							s_spp[SPP_CLASS_DECLARATION_BEGIN_POSITION] = $$.b;
							s_spp[SPP_CLASS_DECLARATION_END_POSITION] = $$.e;
						}
					}
				} else {
					jslNewClassDefinitionEnd();
				}
			}
	|	Modifiers_opt CLASS Identifier 
			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						$<trail>$ = newClassDefinitionBegin($3.d, $1.d, NULL);
					}
				} else {
					jslNewClassDefinitionBegin($3.d, $1.d, NULL, CPOS_FUNCTION_INNER);
				}
			}
		error ClassBody
			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						newClassDefinitionEnd($<trail>4);
					} else {
						PropagateBorns($$, $1, $6);
						if ($$.b.file == s_noneFileIndex) PropagateBorns($$, $2, $6);
					}
				} else {
					jslNewClassDefinitionEnd();
				}
			}
	|	Modifiers_opt CLASS COMPL_CLASS_DEF_NAME	{ /* never used */ }
	;



Super_opt:											{
			InSecondJslPass({
				if (strcmp(s_jsl->classStat->thisClass->linkName,
							s_javaLangObjectLinkName)!=0) {
					// add to any except java/lang/Object itself
					jslAddSuperClassOrInterfaceByName(
						s_jsl->classStat->thisClass, s_javaLangObjectLinkName);
				}
			});
			SetNullBorns($$);
		}
	|	EXTENDS ExtendClassOrInterfaceType			{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					assert($2.d && $2.d->b.symType == TypeDefault && $2.d->u.type);
					assert($2.d->u.type->m == TypeStruct);
					javaParsedSuperClass($2.d->u.type->u.t);
				} else {
					PropagateBorns($$, $1, $2);
				}
			}
			InSecondJslPass({
				assert($2.d && $2.d->b.symType == TypeDefault && $2.d->u.type);
				assert($2.d->u.type->m == TypeStruct);
				jslAddSuperClassOrInterface(s_jsl->classStat->thisClass, 
											$2.d->u.type->u.t);
			});
		}
	;

Interfaces_opt:								{
			SetNullBorns($$);
		}
	|	IMPLEMENTS InterfaceTypeList		{
			PropagateBornsIfRegularSyntaxPass($$, $1, $2);
		}
	;

InterfaceTypeList:
		InterfaceType							{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					assert($1.d && $1.d->b.symType == TypeDefault && $1.d->u.type);
					assert($1.d->u.type->m == TypeStruct);
					javaParsedSuperClass($1.d->u.type->u.t);
				} else {
					PropagateBorns($$, $1, $1);
				}
			}
			InSecondJslPass({
				assert($1.d && $1.d->b.symType == TypeDefault && $1.d->u.type);
				assert($1.d->u.type->m == TypeStruct);
				jslAddSuperClassOrInterface(s_jsl->classStat->thisClass, 
											$1.d->u.type->u.t);
			});
		}
	|	InterfaceTypeList ',' InterfaceType		{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					assert($3.d && $3.d->b.symType == TypeDefault && $3.d->u.type);
					assert($3.d->u.type->m == TypeStruct);
					javaParsedSuperClass($3.d->u.type->u.t);
				} else {
					PropagateBorns($$, $1, $3);
				}
			}
			InSecondJslPass({
				assert($3.d && $3.d->b.symType == TypeDefault && $3.d->u.type);
				assert($3.d->u.type->m == TypeStruct);
				jslAddSuperClassOrInterface(s_jsl->classStat->thisClass, 
											$3.d->u.type->u.t);
			});
		}
	;

_bef_:	{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					// TODO, REDO all this stuff around class/method borns !!!!!!
					if (s_opt.taskRegime == RegimeEditServer) {
						if (s_cp.parserPassedMarker && !s_cp.thisMethodMemoriesStored){
							s_cps.cxMemiAtMethodBeginning = s_cp.cxMemiAtFunBegin;
							s_cps.cxMemiAtMethodEnd = cxMemory->i;
//&sprintf(tmpBuff,"setting %s, %d,%d   %d,%d", olcxOptionsName[s_opt.cxrefs], s_cp.parserPassedMarker, s_cp.thisMethodMemoriesStored, s_cps.cxMemiAtMethodBeginning,s_cps.cxMemiAtMethodEnd),ppcGenRecord(PPC_BOTTOM_INFORMATION,tmpBuff,"\n");
							s_cp.thisMethodMemoriesStored = 1;
							if (s_opt.cxrefs == OLO_MAYBE_THIS) {
								changeMethodReferencesUsages(LINK_NAME_MAYBE_THIS_ITEM, 
															 CatLocal, cFile.lb.cb.fileNumber,
															 s_javaStat->thisClass);
							} else if (s_opt.cxrefs == OLO_NOT_FQT_REFS) {
								changeMethodReferencesUsages(LINK_NAME_NOT_FQT_ITEM, 
															 CatLocal,cFile.lb.cb.fileNumber,
															 s_javaStat->thisClass);
							} else if (s_opt.cxrefs == OLO_USELESS_LONG_NAME) {
								changeMethodReferencesUsages(LINK_NAME_IMPORTED_QUALIFIED_ITEM,
															 CatGlobal,cFile.lb.cb.fileNumber, 
															 s_javaStat->thisClass);
							}
							s_cps.cxMemiAtClassBeginning = s_cp.cxMemiAtClassBegin;
							s_cps.cxMemiAtClassEnd = cxMemory->i;
							s_cps.classCoordEndLine = cFile.lineNumber+1;
//&fprintf(dumpOut,"!setting class end line to %d, cb==%d, ce==%d\n", s_cps.classCoordEndLine, s_cps.cxMemiAtClassBeginning, s_cps.cxMemiAtClassEnd);
							if (s_opt.cxrefs == OLO_NOT_FQT_REFS_IN_CLASS) {
								changeClassReferencesUsages(LINK_NAME_NOT_FQT_ITEM, 
															CatLocal,cFile.lb.cb.fileNumber,
															s_javaStat->thisClass);
							} else if (s_opt.cxrefs == OLO_USELESS_LONG_NAME_IN_CLASS) {
								changeClassReferencesUsages(LINK_NAME_IMPORTED_QUALIFIED_ITEM,
															CatGlobal,cFile.lb.cb.fileNumber, 
															s_javaStat->thisClass);
							}
						}
					}
					s_cp.cxMemiAtClassBegin = cxMemory->i;
//&fprintf(dumpOut,"!setting class begin memory %d\n", s_cp.cxMemiAtClassBegin);
					actionsBeforeAfterExternalDefinition();
				}
			}
		}
	;

ClassBody:
		_bef_ '{' '}'								{
			PropagateBornsIfRegularSyntaxPass($$, $2, $3);
		}
	|	_bef_ '{' ClassBodyDeclarations _bef_ '}' 	{
			PropagateBornsIfRegularSyntaxPass($$, $2, $5);
		}
	;

ClassBodyDeclarations:
		ClassBodyDeclaration 									{
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	|	ClassBodyDeclarations _bef_ ClassBodyDeclaration		{
			PropagateBornsIfRegularSyntaxPass($$, $1, $3);
		}
	;

ClassBodyDeclaration:
		ClassMemberDeclaration
	|	ClassInitializer
	|	ConstructorDeclaration
	|	';'
	|	error							{SetNullBorns($$);}
	;

ClassMemberDeclaration:
		ClassDeclaration				{PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	InterfaceDeclaration			{PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	FieldDeclaration				{PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	MethodDeclaration				{PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	;

/* ****************** Field Declarations ****************** */

AssignementType:
		Type					{
			$$.d = $1.d;
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					s_cps.lastAssignementStruct = $1.d;
				} else {
					PropagateBorns($$, $1, $1);
				}
			}
	}
	;

FieldDeclaration:
		Modifiers_opt AssignementType VariableDeclarators ';'		{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					S_symbol *s,*p,*pp,*memb,*clas;
					int vClass;
					S_recFindStr    rfs;
					s_cps.lastAssignementStruct = NULL;
					clas = s_javaStat->thisClass;
					assert(clas != NULL);
					for(p=$3.d; p!=NULL; p=pp) {
						pp = p->next;
						p->next = NULL;
						if (p->b.symType == TypeError) continue;
						assert(p->b.symType == TypeDefault);
						completeDeclarator($2.d, p);
						vClass = s_javaStat->classFileInd;
						p->b.accessFlags = $1.d;
						p->b.storage = StorageField;
						if (clas->b.accessFlags&ACC_INTERFACE) {
							// set interface default access flags
							p->b.accessFlags |= (ACC_PUBLIC | ACC_STATIC | ACC_FINAL);
						}
						//&javaSetFieldLinkName(p);
						iniFind(clas, &rfs);				
						if (findStrRecordSym(&rfs, p->name, &memb, CLASS_TO_ANY,
											 ACC_CHECK_NO,VISIB_CHECK_NO) == RETURN_NOT_FOUND) {
							assert(clas->u.s);
							LIST_APPEND(S_symbol, clas->u.s->records, p);
						}
						addCxReference(p, &p->pos, UsageDefined, vClass, vClass);
						htmlAddJavaDocReference(p, &p->pos, vClass, vClass);
					}
					$$.d = $3.d;
					if (s_opt.taskRegime == RegimeEditServer
						&& s_cp.parserPassedMarker 
						&& !s_cp.thisMethodMemoriesStored){
						s_cps.methodCoordEndLine = cFile.lineNumber+1;
					}
				} else {
					PropagateBorns($$, $1, $4);
					if ($$.b.file == s_noneFileIndex) PropagateBorns($$, $2, $4);
					if (POSITION_IS_BETWEEN_IN_THE_SAME_FILE($$.b, s_cxRefPos, $$.e)
						&& s_spp[SPP_FIELD_DECLARATION_BEGIN_POSITION].file==s_noneFileIndex) {
						s_spp[SPP_FIELD_DECLARATION_BEGIN_POSITION] = $$.b;
						s_spp[SPP_FIELD_DECLARATION_TYPE_BEGIN_POSITION] = $2.b;
						s_spp[SPP_FIELD_DECLARATION_TYPE_END_POSITION] = $2.e;
						s_spp[SPP_FIELD_DECLARATION_END_POSITION] = $$.e;
					}
				}
			}
			InSecondJslPass({
				S_symbol *p;
				S_symbol *pp;
				S_symbol *clas;
				S_recFindStr    rfs;
				int		vClass;
				clas = s_jsl->classStat->thisClass;
				assert(clas != NULL);
				for(p=$3.d; p!=NULL; p=pp) {
					pp = p->next;
					p->next = NULL;
					if (p->b.symType == TypeError) continue;
					assert(p->b.symType == TypeDefault);
					assert(clas->u.s);
					vClass = clas->u.s->classFile;
					jslCompleteDeclarator($2.d, p);
					p->b.accessFlags = $1.d;
					p->b.storage = StorageField;
					if (clas->b.accessFlags&ACC_INTERFACE) {
						// set interface default access flags
						p->b.accessFlags |= (ACC_PUBLIC|ACC_STATIC|ACC_FINAL);
					}
					DPRINTF3("[jsl] adding field %s to %s\n",
							p->name,clas->linkName);
					LIST_APPEND(S_symbol, clas->u.s->records, p);
					assert(vClass!=s_noneFileIndex);
					if (p->pos.file!=s_olOriginalFileNumber && s_opt.cxrefs==OLO_PUSH) {
						// pre load of saved file akes problem on move field/method, ...
						addCxReference(p, &p->pos, UsageDefined, vClass, vClass);
					}
				}
				$$.d = $3.d;
			});
		}
	;

VariableDeclarators:
		VariableDeclarator								{ 
			$$.d = $1.d;
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					assert($$.d->b.symType == TypeDefault || $$.d->b.symType == TypeError);
				} else {
					PropagateBorns($$, $1, $1);
				}
			}
		}
	|	VariableDeclarators ',' VariableDeclarator		{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					S_symbol *p;
					assert($1.d && $3.d);
					if ($3.d->b.storage == StorageError) {
						$$.d = $1.d;
					} else {
						$$.d = $3.d;
						$$.d->next = $1.d;
					}
					assert($$.d->b.symType == TypeDefault || $$.d->b.symType == TypeError);
				} else {
					PropagateBorns($$, $1, $3);
				}
			}
			InSecondJslPass({
				S_symbol *p;
				assert($1.d && $3.d);
				if ($3.d->b.storage == StorageError) {
					$$.d = $1.d;
				} else {
					$$.d = $3.d;
					$$.d->next = $1.d;
				}
				assert($$.d->b.symType==TypeDefault || $$.d->b.symType==TypeError);
			});
		}
	;

VariableDeclarator:
		VariableDeclaratorId							/*	{ $$ = $1; } */
	|	VariableDeclaratorId '=' VariableInitializer	{ 
			$$.d = $1.d; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $3);
		}
	|	error											{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					XX_ALLOC($$.d, S_symbol);
					*$$.d = s_errorSymbol;
				} else {
					SetNullBorns($$);
				}
			}
			InSecondJslPass({
				CF_ALLOC($$.d, S_symbol);
				*$$.d = s_errorSymbol;
			});
		}
	;

VariableDeclaratorId:
		Identifier							{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d = StackMemAlloc(S_symbol);
					FILL_symbolBits(&$$.d->b,0,0,0,0,0,TypeDefault,StorageDefault,0);
					FILL_symbol($$.d,$1.d->name,$1.d->name,$1.d->p,$$.d->b,type,NULL,NULL);
					$$.d->u.type = NULL;
				} else {
					PropagateBorns($$, $1, $1);
				}
			}
			InSecondJslPass({
				char *name;
				CF_ALLOCC(name, strlen($1.d->name)+1, char);
				strcpy(name, $1.d->name);
				CF_ALLOC($$.d, S_symbol);
				FILL_symbolBits(&$$.d->b,0,0,0,0,0,TypeDefault,StorageDefault,0);
				FILL_symbol($$.d,name,name,$1.d->p,$$.d->b,type,NULL,NULL);
				$$.d->u.type = NULL;
			});
		}
	|	VariableDeclaratorId '[' ']'		{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					assert($1.d);
					$$.d = $1.d;
					AddComposedType($$.d, TypeArray);
				} else {
					PropagateBorns($$, $1, $3);
				}
			}
			InSecondJslPass({
				assert($1.d);
				$$.d = $1.d;
				JslAddComposedType($$.d, TypeArray);
			});
		}
	|	COMPL_VARIABLE_NAME_HINT {/* rule never used */}
	;

VariableInitializer:
		Expression
	|	ArrayInitializer
	;

/* **************** Method Declarations **************** */

MethodDeclaration:
		MethodHeader Start_block 
			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						javaMethodBodyBeginning($1.d);
					}
				}
			}
		MethodBody Stop_block 
			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						javaMethodBodyEnding(&$4.d);
					} else {
						PropagateBorns($$, $1, $4);
						if (POSITION_IS_BETWEEN_IN_THE_SAME_FILE($1.b, s_cxRefPos, $1.e)) {
							s_spp[SPP_METHOD_DECLARATION_BEGIN_POSITION] = $$.b;
							s_spp[SPP_METHOD_DECLARATION_END_POSITION] = $$.e;
						}
					}
				}
			}
	;

MethodHeader:
		Modifiers_opt AssignementType MethodDeclarator Throws_opt	{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					s_cps.lastAssignementStruct = NULL;
					$$.d = javaMethodHeader($1.d,$2.d,$3.d, StorageMethod);
				} else {
					PropagateBorns($$, $1, $4);
					if ($$.b.file == s_noneFileIndex) PropagateBorns($$, $2, $$);
					if ($$.e.file == s_noneFileIndex) PropagateBorns($$, $$, $3);
					if (POSITION_IS_BETWEEN_IN_THE_SAME_FILE($$.b, s_cxRefPos, $3.e)) {
						s_spp[SPP_METHOD_DECLARATION_TYPE_BEGIN_POSITION] = $2.b;
						s_spp[SPP_METHOD_DECLARATION_TYPE_END_POSITION] = $2.e;
					}
				}
			}
			InSecondJslPass({
				$$.d = jslMethodHeader($1.d,$2.d,$3.d,StorageMethod, $4.d);
			});
		}
	|	Modifiers_opt VOID MethodDeclarator Throws_opt	{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d = javaMethodHeader($1.d,&s_defaultVoidDefinition,$3.d,StorageMethod);
				} else {
					PropagateBorns($$, $1, $4);
					if ($$.b.file == s_noneFileIndex) PropagateBorns($$, $2, $$);
					if ($$.e.file == s_noneFileIndex) PropagateBorns($$, $$, $3);
					if (POSITION_IS_BETWEEN_IN_THE_SAME_FILE($$.b, s_cxRefPos, $3.e)) {
						s_spp[SPP_METHOD_DECLARATION_TYPE_BEGIN_POSITION] = $2.b;
						s_spp[SPP_METHOD_DECLARATION_TYPE_END_POSITION] = $2.e;
					}
				}
			}
			InSecondJslPass({
				$$.d = jslMethodHeader($1.d,&s_defaultVoidDefinition,$3.d,StorageMethod, $4.d);
			});
		}
	|	COMPL_FULL_INHERITED_HEADER		{assert(0);}
	;

MethodDeclarator:
		Identifier 
			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						$<symbol>$ = javaCreateNewMethod($1.d->name, &($1.d->p), MEM_XX);
					}
				}
				InSecondJslPass({
					$<symbol>$ = javaCreateNewMethod($1.d->name,&($1.d->p), MEM_CF);
				});
			}
		'(' FormalParameterList_opt ')'
			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						$$.d = $<symbol>2;
						assert($$.d && $$.d->u.type && $$.d->u.type->m == TypeFunction);
						FILL_funTypeModif(&$$.d->u.type->u.f , $4.d.s, NULL);
					} else {
						javaHandleDeclaratorParamPositions(&$1.d->p, &$3.d, $4.d.p, &$5.d);
						PropagateBorns($$, $1, $5);
					}
				}
				InSecondJslPass({
					$$.d = $<symbol>2;
					assert($$.d && $$.d->u.type && $$.d->u.type->m == TypeFunction);
					FILL_funTypeModif(&$$.d->u.type->u.f , $4.d.s, NULL);
				});
			}
	|	MethodDeclarator '[' ']'						{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d = $1.d;
					AddComposedType($$.d, TypeArray);
				} else {
					PropagateBorns($$, $1, $3);
				}
			}
			InSecondJslPass({
				$$.d = $1.d;
				JslAddComposedType($$.d, TypeArray);
			});
		}
	|	COMPL_METHOD_NAME0					{ assert(0);}
	;

FormalParameterList_opt:					{
			$$.d.s = NULL;
			$$.d.p = NULL;
			SetNullBorns($$);
		}
	|	FormalParameterList					/* {$$ = $1;} */
	;

FormalParameterList:
		FormalParameter								{
			if (! SyntaxPassOnly()) {
				$$.d.s = $1.d;
			} else {
				$$.d.p = NULL;
				appendPositionToList(&$$.d.p, &s_noPos);
				PropagateBorns($$, $1, $1);
			}
		}
	|	FormalParameterList ',' FormalParameter		{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d = $1.d;
					LIST_APPEND(S_symbol, $$.d.s, $3.d);
				} else {
					appendPositionToList(&$$.d.p, &$2.d);
					PropagateBorns($$, $1, $3);
				}
			}
			InSecondJslPass({
				$$.d = $1.d;
				LIST_APPEND(S_symbol, $$.d.s, $3.d);
			});
		}
	;

FormalParameter:
		Type VariableDeclaratorId			{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d = $2.d;
					completeDeclarator($1.d, $2.d);
				} else {
					PropagateBorns($$, $1, $2);
				}
			}
			InSecondJslPass({
				$$.d = $2.d;
				completeDeclarator($1.d, $2.d);
			});
		}
	|	FINAL Type VariableDeclaratorId		{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d = $3.d;
					completeDeclarator($2.d, $3.d);
				} else {
					PropagateBorns($$, $1, $3);
				}
			}
			InSecondJslPass({
				$$.d = $3.d;
				completeDeclarator($2.d, $3.d);
			});
		}
	|	error								{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					XX_ALLOC($$.d, S_symbol);
					*$$.d = s_errorSymbol;
				} else {
					SetNullBorns($$);
				}
			}
			InSecondJslPass({
				CF_ALLOC($$.d, S_symbol);
				*$$.d = s_errorSymbol;
			});
		}
	;

Throws_opt:								{ 
			$$.d = NULL; 
			SetNullBorns($$);
		}
	|	THROWS ClassTypeList			{ 
			$$.d = $2.d; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $2);
		}
	;

ClassTypeList:
		ClassType						{ 
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
			InSecondJslPass({
				assert($1.d && $1.d->b.symType == TypeDefault && $1.d->u.type);
				assert($1.d->u.type->m == TypeStruct);
				CF_ALLOC($$.d, S_symbolList);
				FILL_symbolList($$.d, $1.d->u.type->u.t, NULL);
			});
		}
	|	ClassTypeList ',' ClassType		{
			PropagateBornsIfRegularSyntaxPass($$, $1, $3);
			InSecondJslPass({
				assert($3.d && $3.d->b.symType == TypeDefault && $3.d->u.type);
				assert($3.d->u.type->m == TypeStruct);
				CF_ALLOC($$.d, S_symbolList);
				FILL_symbolList($$.d, $3.d->u.type->u.t, $1.d);
			});
		}
	;

MethodBody:
		Block				/* { $$ = $1; } */
	|	';'					{ 
			$$.d = $1.d; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	;

/* ************** Static Initializers ************** */

ClassInitializer:
		STATIC Block		{
			PropagateBornsIfRegularSyntaxPass($$, $1, $2);
		}
	|	Block				{
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	;

ConstructorDeclaration:
		Modifiers_opt ConstructorDeclarator Throws_opt
			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						S_symbol *p,*pa, *pp, *mh, *args;
						int i;
						args = $2.d;
/*&
  if (! ($1.d & ACC_STATIC)) {
  args = javaPrependDirectEnclosingInstanceArgument($2.d);
  }
  &*/
						mh=javaMethodHeader($1.d, &s_errorSymbol, args, StorageConstructor);
						// TODO! Merge this with 'javaMethodBodyBeginning'!
						assert(mh->u.type && mh->u.type->m == TypeFunction);
						stackMemoryBlockStart();  // in order to remove arguments
						s_cp.function = mh; /* added for set-target-position checks */
						/* also needed for pushing label reference */
						GenInternalLabelReference(-1, UsageDefined);
						s_count.localVar = 0;
						assert($2.d && $2.d->u.type);
						javaAddMethodParametersToSymTable($2.d);
						mh->u.type->u.m.sig = strchr(mh->linkName, '(');
						s_javaStat->cpMethodMods = $1.d;
					}
				}
				InSecondJslPass({
					S_symbol *args;
					args = $2.d;
/*&
					if (! ($1.d & ACC_STATIC)) {
						args = jslPrependDirectEnclosingInstanceArgument($2.d);
					}
&*/
					jslMethodHeader($1.d,&s_defaultVoidDefinition,args,
									StorageConstructor, $3.d);
				});
			}
		 Start_block ConstructorBody Stop_block { 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					stackMemoryBlockFree();
					if (s_opt.taskRegime == RegimeHtmlGenerate) {
						htmlAddFunctionSeparatorReference();
					} else {
						PropagateBorns($$, $1, $6);
						if ($$.b.file == s_noneFileIndex) PropagateBorns($$, $2, $$);
					}
				}
				s_cp.function = NULL; /* added for set-target-position checks */
			}
		}
	;

ConstructorDeclarator:
		Identifier 
			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						if (strcmp($1.d->name, s_javaStat->thisClass->name)==0) {
							addCxReference(s_javaStat->thisClass, &$1.d->p, 
										   UsageConstructorDefinition,s_noneFileIndex, s_noneFileIndex);
							$<symbol>$ = javaCreateNewMethod($1.d->name,//JAVA_CONSTRUCTOR_NAME1, 
															 &($1.d->p), MEM_XX);
						} else {
							// a type forgotten for a method?
							$<symbol>$ = javaCreateNewMethod($1.d->name,&($1.d->p),MEM_XX);
						}
					}
				}
				InSecondJslPass({
					if (strcmp($1.d->name, s_jsl->classStat->thisClass->name)==0) {
						$<symbol>$ = javaCreateNewMethod(
										$1.d->name, //JAVA_CONSTRUCTOR_NAME1, 
										&($1.d->p),
										MEM_CF);
					} else {
						// a type forgotten for a method?
						$<symbol>$ = javaCreateNewMethod($1.d->name, &($1.d->p), MEM_CF);
					}
				});
			}
		'(' FormalParameterList_opt ')'
			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						$$.d = $<symbol>2;
						assert($$.d && $$.d->u.type && $$.d->u.type->m == TypeFunction);
						FILL_funTypeModif(&$$.d->u.type->u.f , $4.d.s, NULL);
					} else {
						javaHandleDeclaratorParamPositions(&$1.d->p, &$3.d, $4.d.p, &$5.d);
						PropagateBorns($$, $1, $5);
					}
				}
				InSecondJslPass({
					$$.d = $<symbol>2;
					assert($$.d && $$.d->u.type && $$.d->u.type->m == TypeFunction);
					FILL_funTypeModif(&$$.d->u.type->u.f , $4.d.s, NULL);
				});
			}
	;

ConstructorBody:
		'{' Start_block ExplicitConstructorInvocation BlockStatements Stop_block '}'	{
			PropagateBornsIfRegularSyntaxPass($$, $1, $6);
		}
	|	'{' Start_block ExplicitConstructorInvocation Stop_block '}'					{
			PropagateBornsIfRegularSyntaxPass($$, $1, $5);
		}
	|	'{' Start_block BlockStatements Stop_block '}'									{
			PropagateBornsIfRegularSyntaxPass($$, $1, $5);
		}
	|	'{' '}'																			{
			PropagateBornsIfRegularSyntaxPass($$, $1, $2);
		}
	;

/* TO FINISH the constructors signatures */
ExplicitConstructorInvocation:
		This _erfs_ 
			{
				if (ComputingPossibleParameterCompletion()) {
					s_cp.erfsForParamsComplet = javaCrErfsForConstructorInvocation(s_javaStat->thisClass, &$1.d->p);
				}
			} '(' ArgumentList_opt ')'			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						javaConstructorInvocation(s_javaStat->thisClass, &($1.d->p), $5.d.t);
						s_cp.erfsForParamsComplet = $2;
					} else {
						javaHandleDeclaratorParamPositions(&$1.d->p, &$4.d, $5.d.p, &$6.d);
						PropagateBorns($$, $1, $6);
					}
				}
			}
	|	Super _erfs_
			{
				if (ComputingPossibleParameterCompletion()) {
					s_cp.erfsForParamsComplet = javaCrErfsForConstructorInvocation(javaCurrentSuperClass(), &$1.d->p);
				}
			}   '(' ArgumentList_opt ')'			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						S_symbol *ss;
						ss = javaCurrentSuperClass();
						javaConstructorInvocation(ss, &($1.d->p), $5.d.t);
						s_cp.erfsForParamsComplet = $2;
					} else {
						javaHandleDeclaratorParamPositions(&$1.d->p, &$4.d, $5.d.p, &$6.d);
						PropagateBorns($$, $1, $6);
					}
				}
			}
	|	Primary  '.' Super _erfs_
			{
				if (ComputingPossibleParameterCompletion()) {
					s_cp.erfsForParamsComplet = javaCrErfsForConstructorInvocation(javaCurrentSuperClass(), &($3.d->p));
				}
			} '(' ArgumentList_opt ')'		{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						S_symbol *ss;
						ss = javaCurrentSuperClass();
						javaConstructorInvocation(ss, &($3.d->p), $7.d.t);
						s_cp.erfsForParamsComplet = $4;
					} else {
						javaHandleDeclaratorParamPositions(&$3.d->p, &$6.d, $7.d.p, &$8.d);
						PropagateBorns($$, $1, $8);
					}
				}
			}
	|	This error					{SetNullBorns($$);}
	|	Super error					{SetNullBorns($$);}
	|	Primary error				{SetNullBorns($$);}
	|	COMPL_SUPER_CONSTRUCTOR1						{assert(0);}
	|	COMPL_THIS_CONSTRUCTOR							{assert(0);}
	|	Primary '.' COMPL_SUPER_CONSTRUCTOR1			{assert(0);}
	;

/* ******************************* Interfaces ******************** */		
/* ************************ Interface Declarations *************** */

InterfaceDeclaration:
		Modifiers_opt INTERFACE Identifier 			{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$<trail>$=newClassDefinitionBegin($3.d,($1.d|ACC_INTERFACE),NULL);
				}
			} else {
				jslNewClassDefinitionBegin($3.d, ($1.d|ACC_INTERFACE), NULL, CPOS_ST);
			}
		} ExtendsInterfaces_opt {
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					javaAddSuperNestedClassToSymbolTab(s_javaStat->thisClass);
				}
			} else {
				jslAddSuperNestedClassesToJslTypeTab(s_jsl->classStat->thisClass);
			}
		} InterfaceBody {
			if (RegularPass()) {
				$$.d = $3.d;
				if (! SyntaxPassOnly()) {
					newClassDefinitionEnd($<trail>4);
				} else {
					PropagateBorns($$, $1, $7);
					if ($$.b.file == s_noneFileIndex) PropagateBorns($$, $2, $$);
					if (POSITION_IS_BETWEEN_IN_THE_SAME_FILE($$.b, s_cxRefPos, $$.e)
						&& s_spp[SPP_CLASS_DECLARATION_BEGIN_POSITION].file == s_noneFileIndex) {
						s_spp[SPP_CLASS_DECLARATION_BEGIN_POSITION] = $$.b;
						s_spp[SPP_CLASS_DECLARATION_TYPE_BEGIN_POSITION] = $2.b;
						s_spp[SPP_CLASS_DECLARATION_TYPE_END_POSITION] = $2.e;
						s_spp[SPP_CLASS_DECLARATION_END_POSITION] = $$.e;
					}
				}
			} else {
				jslNewClassDefinitionEnd();
			}
		}
	|	Modifiers_opt INTERFACE Identifier 
			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						$<trail>$=newClassDefinitionBegin($3.d,($1.d|ACC_INTERFACE),NULL);
					}
				} else {
					jslNewClassDefinitionBegin($3.d, ($1.d|ACC_INTERFACE), NULL, CPOS_ST);
				}
			}
		error InterfaceBody 
			{
				if (RegularPass()) {
					$$.d = $3.d;
					if (! SyntaxPassOnly()) {
						newClassDefinitionEnd($<trail>4);
					} else {
						PropagateBorns($$, $1, $6);
						if ($$.b.file == s_noneFileIndex) PropagateBorns($$, $2, $$);
					}
				} else {
					jslNewClassDefinitionEnd();
				}
			}
	|	Modifiers_opt INTERFACE COMPL_CLASS_DEF_NAME	{ /* never used */ }
	;

ExtendsInterfaces_opt:					{
			SetNullBorns($$);
			InSecondJslPass({
				jslAddSuperClassOrInterfaceByName(s_jsl->classStat->thisClass, 
												s_javaLangObjectLinkName);
			});
		}
	|	ExtendsInterfaces
	;

ExtendsInterfaces:
		EXTENDS ExtendClassOrInterfaceType		{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					assert($2.d && $2.d->b.symType == TypeDefault && $2.d->u.type);
					assert($2.d->u.type->m == TypeStruct);
					javaParsedSuperClass($2.d->u.type->u.t);
				} else {
					PropagateBorns($$, $1, $2);
				}
			}
			InSecondJslPass({
				assert($2.d && $2.d->b.symType == TypeDefault && $2.d->u.type);
				assert($2.d->u.type->m == TypeStruct);
				jslAddSuperClassOrInterface(s_jsl->classStat->thisClass, 
											$2.d->u.type->u.t);
			})
		}
	|	ExtendsInterfaces ',' ExtendClassOrInterfaceType 		{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					assert($3.d && $3.d->b.symType == TypeDefault && $3.d->u.type);
					assert($3.d->u.type->m == TypeStruct);
					javaParsedSuperClass($3.d->u.type->u.t);
				} else {
					PropagateBorns($$, $1, $3);
				}
			}
			InSecondJslPass({
				assert($3.d && $3.d->b.symType == TypeDefault && $3.d->u.type);
				assert($3.d->u.type->m == TypeStruct);
				jslAddSuperClassOrInterface(s_jsl->classStat->thisClass, 
											$3.d->u.type->u.t);
			})
		}
	;

InterfaceBody:
		_bef_ '{' '}'											{
			PropagateBornsIfRegularSyntaxPass($$, $2, $3);
		}
	|	_bef_ '{' InterfaceMemberDeclarations _bef_ '}'			{
			PropagateBornsIfRegularSyntaxPass($$, $2, $5);
		}
	;

InterfaceMemberDeclarations:
		InterfaceMemberDeclaration
	|	InterfaceMemberDeclarations _bef_ InterfaceMemberDeclaration	{
			PropagateBornsIfRegularSyntaxPass($$, $1, $3);
		}
	;

InterfaceMemberDeclaration:
		ClassDeclaration				{PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	InterfaceDeclaration			{PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	ConstantDeclaration				{PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	AbstractMethodDeclaration		{PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	';'								{PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	error							{SetNullBorns($$);}
	;

ConstantDeclaration:
		FieldDeclaration				/* {$$=$1;} */
	;

AbstractMethodDeclaration:
		MethodHeader Start_block			
			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						javaMethodBodyBeginning($1.d);
					}
				}
			}
		';' Stop_block
			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						javaMethodBodyEnding(&$4.d);
					} else {
						PropagateBorns($$, $1, $4);
					}
				}
			}
	;

/* ***************************** Arrays *************************** */

ArrayInitializer:
		'{' VariableInitializers ',' '}'		{PropagateBornsIfRegularSyntaxPass($$, $1, $4);}
	|	'{' VariableInitializers     '}'		{PropagateBornsIfRegularSyntaxPass($$, $1, $3);}
	|	'{' ',' '}'								{PropagateBornsIfRegularSyntaxPass($$, $1, $3);}
	|	'{' '}'									{PropagateBornsIfRegularSyntaxPass($$, $1, $2);}
	;

VariableInitializers:
		VariableInitializer							{
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	|	VariableInitializers ',' VariableInitializer		{
			PropagateBornsIfRegularSyntaxPass($$, $1, $3);
		}
	;

/* *********************** Blocks and Statements ***************** */

Block:
		'{' Start_block BlockStatements Stop_block '}'		{ 
			$$.d = $5.d; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $5);
		}
	|	'{' '}'												{ 
			$$.d = $2.d; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $2);
		}
	;

BlockStatements:
		BlockStatement						/* {$$ = $1;} */
	|	BlockStatements BlockStatement		{
			PropagateBornsIfRegularSyntaxPass($$, $1, $2);
		}
	;

BlockStatement:
		LocalVariableDeclarationStatement		/* {$$ = $1;} */
	|	FunctionInnerClassDeclaration			/* {$$ = $1;} */
	|	Statement								/* {$$ = $1;} */
	|	error									{SetNullBorns($$);}
	;

LocalVariableDeclarationStatement:
		LocalVariableDeclaration ';'			{
			PropagateBornsIfRegularSyntaxPass($$, $1, $2);
		}
	;

LocalVarDeclUntilInit:
		Type VariableDeclaratorId							{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					addNewDeclaration($1.d,$2.d,StorageAuto,s_javaStat->locals);
					$$.d = $1.d;
				} else {
					PropagateBorns($$, $1, $2);
				}
			}
		}
	|	FINAL Type VariableDeclaratorId						{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					addNewDeclaration($2.d,$3.d,StorageAuto,s_javaStat->locals);
					$$.d = $2.d;
				} else {
					PropagateBorns($$, $1, $3);
				}
			}
		}
	|	LocalVariableDeclaration ',' VariableDeclaratorId	{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					if ($1.d->b.symType != TypeError) {
						addNewDeclaration($1.d,$3.d,StorageAuto,s_javaStat->locals);
					}
					$$.d = $1.d;
				} else {
					PropagateBorns($$, $1, $3);
				}
			}
		}
	;

LocalVariableDeclaration:
		LocalVarDeclUntilInit								{ 
			if (RegularPass()) $$.d = $1.d;
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	|	LocalVarDeclUntilInit {
 			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					s_cps.lastAssignementStruct = $1.d;
				}
			}
		} '=' VariableInitializer		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					s_cps.lastAssignementStruct = NULL;
					$$.d = $1.d;
				} else {
					PropagateBorns($$, $1, $4);
				}
			}
		}
	/*
	|   error			{
			$$.d = &s_errorSymbol;
			SetNullBorns($$);
		}
	*/
	;

Statement:
		StatementWithoutTrailingSubstatement
	|	LabeledStatement
	|	IfThenStatement
	|	IfThenElseStatement
	|	WhileStatement
	|	ForStatement
	;

StatementNoShortIf:
		StatementWithoutTrailingSubstatement
	|	LabeledStatementNoShortIf
	|	IfThenElseStatementNoShortIf
	|	WhileStatementNoShortIf
	|	ForStatementNoShortIf
	;

StatementWithoutTrailingSubstatement:
		Block
	|	EmptyStatement
	|	ExpressionStatement
	|	SwitchStatement
	|	DoStatement
	|	BreakStatement
	|	ContinueStatement
	|	ReturnStatement
	|	SynchronizedStatement
	|	ThrowStatement
	|	TryStatement
	|	AssertStatement
	;

AssertStatement:
		ASSERT Expression ';'						{
			PropagateBornsIfRegularSyntaxPass($$, $1, $3);
		}
	|	ASSERT Expression ':' Expression ';'		{
			PropagateBornsIfRegularSyntaxPass($$, $1, $5);
		}
	;

EmptyStatement:
		';'										{
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	;

LabeledStatement:
		LabelDefininigIdentifier ':' Statement		{
			PropagateBornsIfRegularSyntaxPass($$, $1, $3);
		}
	;

LabeledStatementNoShortIf:
		LabelDefininigIdentifier ':' StatementNoShortIf 		{
			PropagateBornsIfRegularSyntaxPass($$, $1, $3);
		}
	;

ExpressionStatement:
		StatementExpression ';'		{
			PropagateBornsIfRegularSyntaxPass($$, $1, $2);
		}
	;

StatementExpression:
		Assignment						{PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	PreIncrementExpression			{PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	PreDecrementExpression			{PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	PostIncrementExpression			{PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	PostDecrementExpression			{PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	MethodInvocation				{PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	|	ClassInstanceCreationExpression	{PropagateBornsIfRegularSyntaxPass($$, $1, $1);}
	;

_ncounter_:	 {if (RegularPass()) EXTRACT_COUNTER_SEMACT($$.d);}
	;

_nlabel_:	{if (RegularPass()) EXTRACT_LABEL_SEMACT($$.d);}
	;

_ngoto_:	{if (RegularPass()) EXTRACT_GOTO_SEMACT($$.d);}
	;

_nfork_:	{if (RegularPass()) EXTRACT_FORK_SEMACT($$.d);}
	;


IfThenStatement:
		IF '(' Expression ')' _nfork_ Statement 					{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					GenInternalLabelReference($5.d, UsageDefined);
				} else {
					PropagateBorns($$, $1, $6);
				}
			}
		}
	;

IfThenElseStatementPrefix:
		IF '(' Expression ')' _nfork_ StatementNoShortIf ELSE _ngoto_ {
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					GenInternalLabelReference($5.d, UsageDefined);
					$$.d = $8.d;
				} else {
					PropagateBorns($$, $1, $7);
				}
			}
		} 
	;

IfThenElseStatement:
		IfThenElseStatementPrefix Statement {
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					GenInternalLabelReference($1.d, UsageDefined);
				} else {
					PropagateBorns($$, $1, $2);
				}
			}
		}
	;

IfThenElseStatementNoShortIf:
		IfThenElseStatementPrefix StatementNoShortIf {
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					GenInternalLabelReference($1.d, UsageDefined);
				} else {
					PropagateBorns($$, $1, $2);
				}
			}
		}
	;

SwitchStatement:
		SWITCH '(' Expression ')' /*5*/ _ncounter_ 	{/*6*/
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					AddContinueBreakLabelSymbol(1000*$5.d,SWITCH_LABEL_NAME,$<symbol>$);
				}
			}
		} {/*7*/
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					AddContinueBreakLabelSymbol($5.d, BREAK_LABEL_NAME, $<symbol>$);
					GenInternalLabelReference($5.d, UsageFork);
				}
			}
		}   SwitchBlock 					{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					GenSwitchCaseFork(1);
					ExtrDeleteContBreakSym($<symbol>7);
					ExtrDeleteContBreakSym($<symbol>6);
					GenInternalLabelReference($5.d, UsageDefined);
				} else {
					PropagateBorns($$, $1, $8);
				}
			}
		}
	;

SwitchBlock:
		'{' Start_block SwitchBlockStatementGroups SwitchLabels Stop_block '}'	{
			PropagateBornsIfRegularSyntaxPass($$, $1, $6);
		}
	|	'{' Start_block SwitchBlockStatementGroups Stop_block '}'				{
			PropagateBornsIfRegularSyntaxPass($$, $1, $5);
		}
	|	'{' Start_block SwitchLabels Stop_block '}'								{
			PropagateBornsIfRegularSyntaxPass($$, $1, $5);
		}
	|	'{' '}'																	{
			PropagateBornsIfRegularSyntaxPass($$, $1, $2);
		}
	;

SwitchBlockStatementGroups:
		SwitchBlockStatementGroup								/* {$$=$1;} */
	|	SwitchBlockStatementGroups SwitchBlockStatementGroup	{
			PropagateBornsIfRegularSyntaxPass($$, $1, $2);
		}
	;

SwitchBlockStatementGroup:
		SwitchLabels 							{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					GenSwitchCaseFork(0);
				}
			}
		} BlockStatements						{
			PropagateBornsIfRegularSyntaxPass($$, $1, $3);
		}
	;

SwitchLabels:
		SwitchLabel								/* {$$=$1;} */
	|	SwitchLabels SwitchLabel				{
			PropagateBornsIfRegularSyntaxPass($$, $1, $2);
		}
	|	error									{
			SetNullBorns($$);
		}
	;

SwitchLabel:
		CASE ConstantExpression ':'				{
			PropagateBornsIfRegularSyntaxPass($$, $1, $3);
		}
	|	DEFAULT ':'								{
			PropagateBornsIfRegularSyntaxPass($$, $1, $2);
		}
	;

WhileStatementPrefix:
		WHILE _nlabel_ '(' Expression ')' /*6*/ _nfork_ {
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					if (s_opt.cxrefs == OLO_EXTRACT) {
						S_symbol *cl, *bl;
						cl = bl = NULL;        // just to avoid warning message
						AddContinueBreakLabelSymbol($2.d, CONTINUE_LABEL_NAME, cl);
						AddContinueBreakLabelSymbol($6.d, BREAK_LABEL_NAME, bl);
						XX_ALLOC($$.d, S_whileExtractData);
						FILL_whileExtractData($$.d, $2.d, $6.d, cl, bl);
					} else {
						$$.d = NULL;
					}
				} else {
					PropagateBorns($$, $1, $5);
				}
			}
		}
	;

WhileStatement:
		WhileStatementPrefix Statement					{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					if ($1.d != NULL) {
						ExtrDeleteContBreakSym($1.d->i4);
						ExtrDeleteContBreakSym($1.d->i3);
						GenInternalLabelReference($1.d->i1, UsageUsed);
						GenInternalLabelReference($1.d->i2, UsageDefined);
					}
				} else {
					PropagateBorns($$, $1, $2);
				}
			}
		}
	;

WhileStatementNoShortIf:
		WhileStatementPrefix StatementNoShortIf			{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					if ($1.d != NULL) {
						ExtrDeleteContBreakSym($1.d->i4);
						ExtrDeleteContBreakSym($1.d->i3);
						GenInternalLabelReference($1.d->i1, UsageUsed);
						GenInternalLabelReference($1.d->i2, UsageDefined);
					}
				} else {
					PropagateBorns($$, $1, $2);
				}
			}
		}
	;

DoStatement:
		DO _nlabel_ _ncounter_ _ncounter_ { /*5*/
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					AddContinueBreakLabelSymbol($3.d, CONTINUE_LABEL_NAME, $<symbol>$);
				}
			}
		} {/*6*/
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					AddContinueBreakLabelSymbol($4.d, BREAK_LABEL_NAME, $<symbol>$);
				}
			}
		} Statement WHILE {
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					ExtrDeleteContBreakSym($<symbol>6);
					ExtrDeleteContBreakSym($<symbol>5);
					GenInternalLabelReference($3.d, UsageDefined);
				}
			}
		} '(' Expression ')' ';'			{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					GenInternalLabelReference($2.d, UsageFork);
					GenInternalLabelReference($4.d, UsageDefined);
				} else {
					PropagateBorns($$, $1, $13);
				}
			}
		}
	;

MaybeExpression:							{
			SetNullBorns($$);
		}
		| Expression						{
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
		;

ForKeyword:
		FOR			{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					stackMemoryBlockStart();
				} else {
					PropagateBorns($$, $1, $1);
				}
			}
		}
	;

ForStatementPrefix:
		'(' ForInit_opt ';' /*4*/ _nlabel_  
		MaybeExpression ';' /*7*/_ngoto_
		/*8*/ _nlabel_  ForUpdate_opt ')' /*11*/ _nfork_    { 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					S_symbol *ss;
					GenInternalLabelReference($4.d, UsageUsed);
					GenInternalLabelReference($7.d, UsageDefined);
					AddContinueBreakLabelSymbol($8.d, CONTINUE_LABEL_NAME, ss);
					AddContinueBreakLabelSymbol($11.d, BREAK_LABEL_NAME, ss);
					$$.d.i1 = $8.d;
					$$.d.i2 = $11.d;
				} else {
					PropagateBorns($$, $1, $10);
				}
			}
		} 
	;

ForStatementBody: 
		ForStatementPrefix  Statement		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					DeleteContinueBreakLabelSymbol(BREAK_LABEL_NAME);
					DeleteContinueBreakLabelSymbol(CONTINUE_LABEL_NAME);
					GenInternalLabelReference($1.d.i1, UsageUsed);
					GenInternalLabelReference($1.d.i2, UsageDefined);
				} else {
					PropagateBorns($$, $1, $2);
				}
			}
		}
	;

ForStatementNoShortIfBody: 
		ForStatementPrefix  StatementNoShortIf		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					DeleteContinueBreakLabelSymbol(BREAK_LABEL_NAME);
					DeleteContinueBreakLabelSymbol(CONTINUE_LABEL_NAME);
					GenInternalLabelReference($1.d.i1, UsageUsed);
					GenInternalLabelReference($1.d.i2, UsageDefined);
				} else {
					PropagateBorns($$, $1, $2);
				}
			}
		}
	;

ForStatement:
		ForKeyword ForStatementBody {
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					stackMemoryBlockFree();
				} else {
					PropagateBorns($$, $1, $2);
				}
			}
		}
	|	ForKeyword error {
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					stackMemoryBlockFree();
				} else {
					SetNullBorns($$);
				}
			}
		}
	;

ForStatementNoShortIf:
		ForKeyword ForStatementNoShortIfBody {
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					stackMemoryBlockFree();
				} else {
					PropagateBorns($$, $1, $2);
				}
			}
		}
	|	ForKeyword error {
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					stackMemoryBlockFree();
				} else {
					SetNullBorns($$);
				}
			}
		}
	;


ForInit_opt:							{
			SetNullBorns($$);
		}
	|	StatementExpressionList			{
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	|	LocalVariableDeclaration		{
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	;

ForUpdate_opt:							{
			SetNullBorns($$);
		}
	|	StatementExpressionList			{
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	;

StatementExpressionList:
		StatementExpression									{
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	|	StatementExpressionList ',' StatementExpression		{
			PropagateBornsIfRegularSyntaxPass($$, $1, $3);
		}
	;

BreakStatement:
		BREAK LabelUseIdentifier ';'		{
			PropagateBornsIfRegularSyntaxPass($$, $1, $3);
		}
	|	BREAK ';'						{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					GenContBreakReference(BREAK_LABEL_NAME);
				} else {
					PropagateBorns($$, $1, $2);
				}
			}
		}
	;

ContinueStatement:
		CONTINUE LabelUseIdentifier ';'		{
			PropagateBornsIfRegularSyntaxPass($$, $1, $3);
		}
	|	CONTINUE ';'					{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					GenContBreakReference(CONTINUE_LABEL_NAME);
				} else {
					PropagateBorns($$, $1, $2);
				}
			}
		}
	;

ReturnStatement:
		RETURN Expression ';'			{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					GenInternalLabelReference(-1, UsageUsed);
				} else {
					PropagateBorns($$, $1, $3);
				}
			}
		}
	|	RETURN ';'						{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					GenInternalLabelReference(-1, UsageUsed);
				} else {
					PropagateBorns($$, $1, $2);
				}
			}
		}
	;

ThrowStatement:
		Throw Expression ';'		{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					if (s_opt.cxrefs==OLO_EXTRACT) {
						addCxReference($2.d.t->u.t, &$1.d->p, UsageThrown, s_noneFileIndex, s_noneFileIndex);
					}
				} else {
					PropagateBorns($$, $1, $3);
				}
			}
		}
	;

SynchronizedStatement:
		SYNCHRONIZED '(' Expression ')' Block				{
			PropagateBornsIfRegularSyntaxPass($$, $1, $5);
		}
	;

TryCatches:
		Catches				/* $$ = $1; */
	|	Finally				/* $$ = $1; */
	|	Catches Finally		{
			PropagateBornsIfRegularSyntaxPass($$, $1, $2);
		}
	;

TryStatement:
		Try  _nfork_ 
			{
				if (s_opt.cxrefs == OLO_EXTRACT) {
					addTrivialCxReference("TryCatch", TypeTryCatchMarker,StorageDefault, 
											&$1.d->p, UsageTryCatchBegin);
				}
			}
		Block 
			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						GenInternalLabelReference($2.d, UsageDefined);
					}
				}
			} 
		TryCatches		{
			PropagateBornsIfRegularSyntaxPass($$, $1, $6);
			if (s_opt.cxrefs == OLO_EXTRACT) {
				addTrivialCxReference("TryCatch", TypeTryCatchMarker,StorageDefault, 
										&$1.d->p, UsageTryCatchEnd);
			}
		}

	;

Catches:
		CatchClause						/* $$ = $1; */
	|	Catches CatchClause		{
			PropagateBornsIfRegularSyntaxPass($$, $1, $2);
		}
	;

CatchClause:
		Catch '(' FormalParameter ')' _nfork_ Start_block
			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						if ($3.d->b.symType != TypeError) {
							addNewSymbolDef($3.d, StorageAuto, s_javaStat->locals, 
											UsageDefined);
							if (s_opt.cxrefs == OLO_EXTRACT) {
								assert($3.d->b.symType==TypeDefault);
								addCxReference($3.d->u.type->u.t, &$1.d->p, UsageCatched, s_noneFileIndex, s_noneFileIndex);
							}
						}
					}
				}
			}
		Block Stop_block		
			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						GenInternalLabelReference($5.d, UsageDefined);
					} else {
						PropagateBorns($$, $1, $8);
					}
				}
			}
	|	Catch '(' FormalParameter ')' ';'		{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					if (s_opt.cxrefs == OLO_EXTRACT) {
						assert($3.d->b.symType==TypeDefault);
						addCxReference($3.d->u.type->u.t, &$1.d->p, UsageCatched, s_noneFileIndex, s_noneFileIndex);
					}
				} else {
					PropagateBorns($$, $1, $5);
				}
			}
		}
	;

Finally:
		FINALLY _nfork_ Block {
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					GenInternalLabelReference($2.d, UsageDefined);
				} else {
					PropagateBorns($$, $1, $3);
				}
			}
		}
	;

/* *********************** expressions ************************** */

Primary:
		PrimaryNoNewArray					{ 
			if (RegularPass()) {
				$$.d = $1.d;
				if (! SyntaxPassOnly()) {
					s_javaCompletionLastPrimary = s_structRecordCompletionType = $$.d.t;
				} else {
					PropagateBorns($$, $1, $1);
				}
			}
		}
	|	ArrayCreationExpression				{ 
			if (RegularPass()) {
				$$.d = $1.d; 
				if (! SyntaxPassOnly()) {
					s_javaCompletionLastPrimary = s_structRecordCompletionType = $$.d.t;
				} else {
					PropagateBorns($$, $1, $1);
				}
			}
		}
	;

PrimaryNoNewArray:
		Literal								/* $$ = $1; */
	|	This								{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					assert(s_javaStat && s_javaStat->thisType);
//fprintf(dumpOut,"this == %s\n",s_javaStat->thisType->u.t->linkName);
					$$.d.t = s_javaStat->thisType;
					addThisCxReferences(s_javaStat->classFileInd, &$1.d->p);
					$$.d.r = NULL;
				} else {
					$$.d.pp = &$1.d->p;
					javaCheckForStaticPrefixStart(&$1.d->p, &$1.d->p);
					PropagateBorns($$, $1, $1);
				}
			}
		}
	|	Name '.' THIS						{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					javaQualifiedThis($1.d, $3.d);
					$$.d.t = javaClassNameType($1.d);
					$$.d.r = NULL;
				} else {
					$$.d.pp = javaGetNameStartingPosition($1.d);
					javaCheckForStaticPrefixStart(&$3.d->p, javaGetNameStartingPosition($1.d));
					PropagateBorns($$, $1, $3);
				}
			}
		}
	|	PrimitiveType '.' CLASS				{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = &s_javaClassModifier;
					$$.d.r = NULL;
				} else {
					$$.d.pp = $1.d.p;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	|	Name '.' CLASS						{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					S_symbol *str;
					javaClassifyToTypeName($1.d,UsageUsed, &str, USELESS_FQT_REFS_ALLOWED);
					$$.d.t = &s_javaClassModifier;
					$$.d.r = NULL;
				} else {
					$$.d.pp = javaGetNameStartingPosition($1.d);
					PropagateBorns($$, $1, $3);
				}
			}
		}
	|	ArrayType '.' CLASS					{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = &s_javaClassModifier;
					$$.d.r = NULL;
				} else {
					$$.d.pp = $1.d.p;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	|	VOID '.' CLASS						{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = &s_javaClassModifier;
					$$.d.r = NULL;
				} else {
					SetPrimitiveTypePos($$.d.pp, $1.d);
					PropagateBorns($$, $1, $3);
				}
			}
		}
	|	'(' Expression ')'					{ 
			if (RegularPass()) {
				$$.d = $2.d; 
				if (SyntaxPassOnly()) {
					XX_ALLOC($$.d.pp, S_position);
					*$$.d.pp = $1.d;
					PropagateBorns($$, $1, $3);
					if (POSITION_IS_BETWEEN_IN_THE_SAME_FILE($$.b, s_cxRefPos, $$.e)
						&& s_spp[SPP_PARENTHESED_EXPRESSION_LPAR_POSITION].file == s_noneFileIndex) {
						s_spp[SPP_PARENTHESED_EXPRESSION_LPAR_POSITION] = $1.b;
						s_spp[SPP_PARENTHESED_EXPRESSION_RPAR_POSITION] = $3.b;
						s_spp[SPP_PARENTHESED_EXPRESSION_BEGIN_POSITION] = $2.b;
						s_spp[SPP_PARENTHESED_EXPRESSION_END_POSITION] = $2.e;
					}
				}
			}
		}
	|	ClassInstanceCreationExpression		/* { $$.d = $1.d' } */
	|	FieldAccess							/* { $$.d = $1.d' } */
	|	MethodInvocation					/* { $$.d = $1.d' } */
	|	ArrayAccess							/* { $$.d = $1.d' } */
	|	CompletionTypeName '.'		{ assert(0); /* rule never used */ }
	;

_erfs_:		{
			$$ = s_cp.erfsForParamsComplet;
		}
	;

NestedConstructorInvocation:
		Primary '.' New Name _erfs_ 
			{
				if (ComputingPossibleParameterCompletion()) {
					S_typeModifiers *mm;
					s_cp.erfsForParamsComplet = NULL;
					if ($1.d.t->m == TypeStruct) {
						mm = javaNestedNewType($1.d.t->u.t, $3.d, $4.d);
						if (mm->m != TypeError) {
							s_cp.erfsForParamsComplet = javaCrErfsForConstructorInvocation(mm->u.t, &($4.d->idi.p));
						}
					}
				}
			} 
		'(' ArgumentList_opt ')'				{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					s_cp.erfsForParamsComplet = $5;
					if ($1.d.t->m == TypeStruct) {
						$$.d.t = javaNestedNewType($1.d.t->u.t, $3.d, $4.d);
					} else {
						$$.d.t = &s_errorModifier;
					}
					javaHandleDeclaratorParamPositions(&$4.d->idi.p, &$7.d, $8.d.p, &$9.d);
					assert($$.d.t);
					$$.d.nid = $4.d;
					if ($$.d.t->m != TypeError) {
						javaConstructorInvocation($$.d.t->u.t, &($4.d->idi.p), $8.d.t);
					}
				} else {
					$$.d.pp = $1.d.pp;
					PropagateBorns($$, $1, $9);
				}
			}
		}
	|	Name '.' New Name _erfs_ 
			{
				if (ComputingPossibleParameterCompletion()) {
					S_typeModifiers *mm;
					s_cp.erfsForParamsComplet = NULL;
					mm = javaNewAfterName($1.d, $3.d, $4.d);
					if (mm->m != TypeError) {
						s_cp.erfsForParamsComplet = javaCrErfsForConstructorInvocation(mm->u.t, &($4.d->idi.p));
					}
				}
			} 
		'(' ArgumentList_opt ')'				{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					s_cp.erfsForParamsComplet = $5;
					$$.d.t = javaNewAfterName($1.d, $3.d, $4.d);
					$$.d.nid = $4.d;
					if ($$.d.t->m != TypeError) {
						javaConstructorInvocation($$.d.t->u.t, &($4.d->idi.p), $8.d.t);
					}
				} else {
					$$.d.pp = javaGetNameStartingPosition($1.d);
					javaHandleDeclaratorParamPositions(&$4.d->idi.p, &$7.d, $8.d.p, &$9.d);
					PropagateBorns($$, $1, $9);
				}
			}
		}
	;
	
NewName:
		Name	{
			if (ComputingPossibleParameterCompletion()) {
				S_symbol 			*ss;
				S_symbol			*str;
				S_typeModifiers		*expr;
				S_reference			*rr, *lastUselessRef;
				javaClassifyAmbiguousName($1.d, NULL,&str,&expr,&rr, &lastUselessRef, USELESS_FQT_REFS_ALLOWED,
										  CLASS_TO_TYPE,UsageUsed);
				$1.d->nameType = TypeStruct;
				ss = javaTypeSymbolUsage($1.d, ACC_DEFAULT);
				s_cp.erfsForParamsComplet = javaCrErfsForConstructorInvocation(ss, &($1.d->idi.p));
			}
			$$ = $1;
		}
	;

ClassInstanceCreationExpression:
		New _erfs_ NewName '(' ArgumentList_opt ')'							{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					S_symbol *ss, *tt, *ei;
					S_symbol	*str;
					S_typeModifiers		*expr;
					S_idIdentList		*ii;
					S_reference			*rr, *lastUselessRef;
					s_cp.erfsForParamsComplet = $2;
					lastUselessRef = NULL;
					javaClassifyAmbiguousName($3.d, NULL,&str,&expr,&rr, &lastUselessRef, USELESS_FQT_REFS_ALLOWED,
											  CLASS_TO_TYPE,UsageUsed);
					$3.d->nameType = TypeStruct;
					ss = javaTypeSymbolUsage($3.d, ACC_DEFAULT);
					if (isANestedClass(ss)) {
						if (javaIsInnerAndCanGetUnnamedEnclosingInstance(ss, &ei)) {
							// before it was s_javaStat->classFileInd, but be more precise
							// in reality you should keep both to discover references
							// to original class from class nested in method.
							addThisCxReferences(ei->u.s->classFile, &$1.d->p);
							// I have removed following because it makes problems when
							// expanding to FQT names, WHY IT WAS HERE ???
							//&addSpecialFieldReference(LINK_NAME_NOT_FQT_ITEM,StorageField,
							//&				 s_javaStat->classFileInd, &$1.d->p,
							//&				 UsageNotFQField);
						} else {
							// here I should annulate class reference, as it is an error
							// because can't get enclosing instance, this is sufficient to
							// pull-up/down to report a problem
							// BERK, It was completely wrong, because it is completely legal
							// and annulating of reference makes class renaming wrong!
							// Well, it is legal only for static nested classes.
							// But for security reasons, I will keep it in comment,
							//&if (! (ss->b.accessFlags&ACC_STATIC)) {
							//&	if (rr!=NULL) rr->usg.base = s_noUsage;
							//&}
						}
					}
					javaConstructorInvocation(ss, &($3.d->idi.p), $5.d.t);
					tt = javaTypeNameDefinition($3.d);
					$$.d.t = tt->u.type;
					$$.d.r = NULL;
				} else {
					javaHandleDeclaratorParamPositions(&$3.d->idi.p, &$4.d, $5.d.p, &$6.d);
					$$.d.pp = &$1.d->p;
					PropagateBorns($$, $1, $6);
				}
			}
		}
	|	New _erfs_ NewName '(' ArgumentList_opt ')' 
			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						S_symbol *ss;
						s_cp.erfsForParamsComplet = $2;
						javaClassifyToTypeName($3.d,UsageUsed, &ss, USELESS_FQT_REFS_ALLOWED);
						$<symbol>$ = javaTypeNameDefinition($3.d);
						ss = javaTypeSymbolUsage($3.d, ACC_DEFAULT);
						javaConstructorInvocation(ss, &($3.d->idi.p), $5.d.t);
					} else {
						javaHandleDeclaratorParamPositions(&$3.d->idi.p, &$4.d, $5.d.p, &$6.d);
						// seems that there is no problem like in previous case,
						// interfaces are never inner.
					}
				} else {
					S_symbol *str, *cls;
					jslClassifyAmbiguousTypeName($3.d, &str);
					cls = jslTypeNameDefinition($3.d);
					jslNewClassDefinitionBegin(&s_javaAnonymousClassName, 
												ACC_DEFAULT, cls, CPOS_ST);
				}
			}
			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						$<trail>$ = newClassDefinitionBegin(&s_javaAnonymousClassName,ACC_DEFAULT, $<symbol>6);
					}
				}
			}
		ClassBody			{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					newClassDefinitionEnd($<trail>8);
					assert($<symbol>7 && $<symbol>7->u.type);
					$$.d.t = $<symbol>7->u.type;
					$$.d.r = NULL;
				} else {
					$$.d.pp = &$1.d->p;
					PropagateBorns($$, $1, $9);
				}
			} else {
				jslNewClassDefinitionEnd();
			}
		}
	|	NestedConstructorInvocation								{
			$$.d.t = $1.d.t;
			$$.d.pp = $1.d.pp;
			$$.d.r = NULL;
			PropagateBorns($$, $1, $1);
		}
	|	NestedConstructorInvocation
			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						$$.d.t = $1.d.t;
						$$.d.pp = $1.d.pp;
						$$.d.r = NULL;
						if ($$.d.t->m != TypeError) {
							$<trail>$ = newClassDefinitionBegin(&s_javaAnonymousClassName, ACC_DEFAULT, $$.d.t->u.t);
						} else {
							$<trail>$ = newAnonClassDefinitionBegin(& $1.d.nid->idi);
						}
					} else {
						$$.d.pp = $1.d.pp;
					}
				} else {
					jslNewAnonClassDefinitionBegin(& $1.d.nid->idi);
				}
			}
		ClassBody			
			{
				if (RegularPass()) {
					if (! SyntaxPassOnly()) {
						newClassDefinitionEnd($<trail>2);
					} else {
						PropagateBorns($$, $1, $3);
					}
				} else {
					jslNewClassDefinitionEnd();
				}
		}
	|	New _erfs_ CompletionConstructorName '(' 				{
			assert(0); /* rule never used */
		}
	|	Primary '.' New  COMPL_CONSTRUCTOR_NAME3 '(' 	{
			assert(0); /* rule never used */
		}
	|	Name '.' New  COMPL_CONSTRUCTOR_NAME2 '(' 		{
			assert(0); /* rule never used */
		}

/* JLS
	|	Primary '.' NEW Identifier '(' ArgumentList_opt ')'
	|	Primary '.' NEW Identifier '(' ArgumentList_opt ')' ClassBody
	|	Name '.' NEW Identifier '(' ArgumentList_opt ')'
	|	Name '.' NEW Identifier '(' ArgumentList_opt ')' ClassBody
*/
	;

ArgumentList_opt:				{ 
			$$.d.t = NULL; 
			$$.d.p = NULL;
			SetNullBorns($$);
		}
	| ArgumentList				/* { $$.d = $1.d; } */
	;

ArgumentList:
		Expression									{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					XX_ALLOC($$.d.t, S_typeModifiersList);
					FILL_typeModifiersList($$.d.t, $1.d.t, NULL);
					if (s_cp.erfsForParamsComplet!=NULL) {
						s_cp.erfsForParamsComplet->params = $$.d.t;
					}
				} else {
					$$.d.p = NULL;
					appendPositionToList(&$$.d.p, &s_noPos);
					PropagateBorns($$, $1, $1);
				}
			}
		}
	|	ArgumentList ',' Expression					{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					S_typeModifiersList *p;
					$$.d = $1.d;
					XX_ALLOC(p, S_typeModifiersList);
					FILL_typeModifiersList(p, $3.d.t, NULL);
					LIST_APPEND(S_typeModifiersList, $$.d.t, p);
					if (s_cp.erfsForParamsComplet!=NULL) s_cp.erfsForParamsComplet->params = $$.d.t;
				} else {
					appendPositionToList(&$$.d.p, &$2.d);
					PropagateBorns($$, $1, $3);
				}
			}
		}
	| COMPL_UP_FUN_PROFILE							{assert(0);}
	| ArgumentList ',' COMPL_UP_FUN_PROFILE			{assert(0);}
	;


ArrayCreationExpression:
		New _erfs_ PrimitiveType DimExprs Dims_opt			{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					int i;
					CrTypeModifier($$.d.t,$3.d.u);
					for(i=0; i<$4.d; i++) PrependModifier($$.d.t, TypeArray);
					$$.d.r = NULL;
				} else {
					$$.d.pp = &$1.d->p;
					PropagateBorns($$, $1, $5);
					if ($$.e.file == s_noneFileIndex) PropagateBorns($$, $$, $4);
				}
			}
		}
	|	New _erfs_ PrimitiveType Dims ArrayInitializer		{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					int i;
					CrTypeModifier($$.d.t,$3.d.u);
					for(i=0; i<$4.d; i++) PrependModifier($$.d.t, TypeArray);
					$$.d.r = NULL;
				} else {
					$$.d.pp = &$1.d->p;
					PropagateBorns($$, $1, $5);
				}
			}
		}
	|	New _erfs_ ClassOrInterfaceType DimExprs Dims_opt			{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					int i;
					assert($3.d && $3.d->u.type);
					$$.d.t = $3.d->u.type;
					for(i=0; i<$4.d; i++) PrependModifier($$.d.t, TypeArray);
					$$.d.r = NULL;
				} else {
					$$.d.pp = &$1.d->p;
					PropagateBorns($$, $1, $5);
					if ($$.e.file == s_noneFileIndex) PropagateBorns($$, $$, $4);
				}
			}
		}
	|	New _erfs_ ClassOrInterfaceType Dims ArrayInitializer		{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					int i;
					assert($3.d && $3.d->u.type);
					$$.d.t = $3.d->u.type;
					for(i=0; i<$4.d; i++) PrependModifier($$.d.t, TypeArray);
					$$.d.r = NULL;
				} else {
					$$.d.pp = &$1.d->p;
					PropagateBorns($$, $1, $5);
				}
			}
		}
	;


DimExprs:
		DimExpr						{ 
			if (RegularPass()) $$.d = 1; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	|	DimExprs DimExpr			{ 
			if (RegularPass()) $$.d = $1.d+1; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $2);
		}
	;

DimExpr:
		'[' Expression ']'			{
			PropagateBornsIfRegularSyntaxPass($$, $1, $3);
		}
	;

Dims_opt:							{ 
			if (RegularPass()) $$.d = 0; 
			SetNullBorns($$);
		}
	|	Dims						/* { $$ = $1; } */
	;

Dims:
		'[' ']'						{ 
			if (RegularPass()) $$.d = 1; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $2);
		}
	|	Dims '[' ']'				{ 
			if (RegularPass()) $$.d = $1.d+1; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $3);
		}
	;

FieldAccess:
 		Primary '.' Identifier					{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					S_symbol *rec=NULL;
					assert($1.d.t);
					$$.d.r = NULL;
					$$.d.pp = $1.d.pp;
					if ($1.d.t->m == TypeStruct) {
						javaLoadClassSymbolsFromFile($1.d.t->u.t);
						$$.d.r = findStrRecordFromType($1.d.t, $3.d, &rec, CLASS_TO_EXPR);
						assert(rec);
						$$.d.t = rec->u.type;
					} else if (s_language == LAN_JAVA) {
						$$.d.t = javaArrayFieldAccess($3.d);
					} else {
						$$.d.t = &s_errorModifier;
					}
					assert($$.d.t);
				} else {
					$$.d.pp = $1.d.pp;
					javaCheckForPrimaryStart(&$3.d->p, $$.d.pp);
					PropagateBorns($$, $1, $3);
				}
			}
		}
	|	Super '.' Identifier					{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					S_symbol *ss,*rec=NULL;
					S_recFindStr rfs;
					$$.d.r = NULL;
					$$.d.pp = &$1.d->p;
					ss = javaCurrentSuperClass();
					if (ss != &s_errorSymbol && ss->b.symType!=TypeError) {
						javaLoadClassSymbolsFromFile(ss);
						$$.d.r = findStrRecordFromSymbol(ss, $3.d, &rec, CLASS_TO_EXPR, $1.d);
						assert(rec);
						$$.d.t = rec->u.type;
					} else {
						$$.d.t = &s_errorModifier;
					}
					assert($$.d.t);
				} else {
					$$.d.pp = &$1.d->p;
					javaCheckForPrimaryStart(&$3.d->p, $$.d.pp);
					javaCheckForStaticPrefixStart(&$3.d->p, $$.d.pp);
					PropagateBorns($$, $1, $3);
				}
			}
		}
	|	Name '.' Super '.' Identifier					{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					S_symbol *ss,*rec=NULL;
					S_recFindStr rfs;
					ss = javaQualifiedThis($1.d, $3.d);
					if (ss != &s_errorSymbol && ss->b.symType!=TypeError) {
						javaLoadClassSymbolsFromFile(ss);
						ss = javaGetSuperClass(ss);
						$$.d.r = findStrRecordFromSymbol(ss, $5.d, &rec, CLASS_TO_EXPR, NULL);
						assert(rec);
						$$.d.t = rec->u.type;
					} else {
						$$.d.t = &s_errorModifier;
					}
					$$.d.r = NULL;
					assert($$.d.t);
				} else {
					$$.d.pp = javaGetNameStartingPosition($1.d);
					javaCheckForPrimaryStart(&$3.d->p, $$.d.pp);
					javaCheckForPrimaryStart(&$5.d->p, $$.d.pp);
					javaCheckForStaticPrefixStart(&$3.d->p, $$.d.pp);
					javaCheckForStaticPrefixStart(&$5.d->p, $$.d.pp);
					PropagateBorns($$, $1, $5);
				}
			}
		}
	|	Primary '.' COMPL_STRUCT_REC_PRIM		{ assert(0); }
	|	Super '.' COMPL_STRUCT_REC_SUPER		{ assert(0); }
	|	Name '.' Super '.' COMPL_QUALIF_SUPER	{ assert(0); }
	;

MethodInvocation:
		Name _erfs_ {
			if (ComputingPossibleParameterCompletion()) {
				s_cp.erfsForParamsComplet = javaCrErfsForMethodInvocationN($1.d);
			}
		} '(' ArgumentList_opt ')'					{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaMethodInvocationN($1.d,$5.d.t);
					$$.d.r = NULL;
					s_cp.erfsForParamsComplet = $2;
				} else {
					$$.d.pp = javaGetNameStartingPosition($1.d);
					javaCheckForPrimaryStartInNameList($1.d, $$.d.pp);
					javaCheckForStaticPrefixInNameList($1.d, $$.d.pp);
					javaHandleDeclaratorParamPositions(&$1.d->idi.p, &$4.d, $5.d.p, &$6.d);
					PropagateBorns($$, $1, $6);
				}
			}
		}
	|	Primary '.' Identifier _erfs_ {
			if (ComputingPossibleParameterCompletion()) {
				s_cp.erfsForParamsComplet = javaCrErfsForMethodInvocationT($1.d.t, $3.d);
			}
		} '(' ArgumentList_opt ')'	{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaMethodInvocationT($1.d.t,$3.d,$7.d.t);
					$$.d.r = NULL;
					s_cp.erfsForParamsComplet = $4;
				} else {
					$$.d.pp = $1.d.pp;
					javaCheckForPrimaryStart(&$3.d->p, $$.d.pp);
					javaHandleDeclaratorParamPositions(&$3.d->p, &$6.d, $7.d.p, &$8.d);
					PropagateBorns($$, $1, $8);
				}
			}
		}
	|	Super '.' Identifier _erfs_ {
			if (ComputingPossibleParameterCompletion()) {
				s_cp.erfsForParamsComplet = javaCrErfsForMethodInvocationS($1.d, $3.d);
			}
		} '(' ArgumentList_opt ')'	{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaMethodInvocationS($1.d, $3.d, $7.d.t);
					$$.d.r = NULL;
					s_cp.erfsForParamsComplet = $4;
				} else {
					$$.d.pp = &$1.d->p;
					javaCheckForPrimaryStart(&$1.d->p, $$.d.pp);
					javaCheckForPrimaryStart(&$3.d->p, $$.d.pp);
					javaHandleDeclaratorParamPositions(&$3.d->p, &$6.d, $7.d.p, &$8.d);
					PropagateBorns($$, $1, $8);
				}
			}
		}

/* Served by field acces
	|	CompletionExpressionName '('				{ assert(0); }
	|	Primary '.' COMPL_METHOD_PRIMARY			{ assert(0); }
	|	SUPER '.' COMPL_METHOD_SUPER				{ assert(0); }
*/
	;

ArrayAccess:
		Name '[' Expression ']'							{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					S_typeModifiers *tt;
					tt = javaClassifyToExpressionName($1.d, &($$.d.r));
					if (tt->m==TypeArray) $$.d.t=tt->next;
					else $$.d.t = &s_errorModifier;
					assert($$.d.t);
					$$.d.r = NULL;
				} else {
					$$.d.pp = javaGetNameStartingPosition($1.d);
					PropagateBorns($$, $1, $4);
				}
			}
		}
	|	PrimaryNoNewArray '[' Expression ']'		{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					if ($1.d.t->m==TypeArray) $$.d.t=$1.d.t->next;
					else $$.d.t = &s_errorModifier;
					assert($$.d.t);
					$$.d.r = NULL;
				} else {
					$$.d.pp = $1.d.pp;
					PropagateBorns($$, $1, $4);
				}
			}
		}
	|	CompletionExpressionName '['				{ /* rule never used */ }
	;

PostfixExpression:
		Primary
	|	Name											{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaClassifyToExpressionName($1.d, &($$.d.r));
				} else {
					$$.d.pp = javaGetNameStartingPosition($1.d);
					javaCheckForPrimaryStartInNameList($1.d, $$.d.pp);
					javaCheckForStaticPrefixInNameList($1.d, $$.d.pp);
					PropagateBorns($$, $1, $1);
				}
			}
		}
	|	PostIncrementExpression
	|	PostDecrementExpression
	|	CompletionExpressionName 					{ /* rule never used */ }
	;

PostIncrementExpression:
		PostfixExpression INC_OP		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaCheckNumeric($1.d.t); 
					RESET_REFERENCE_USAGE($1.d.r, UsageAddrUsed);
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $2);
				}
			}
		}
	;

PostDecrementExpression:
		PostfixExpression DEC_OP		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaCheckNumeric($1.d.t); 
					RESET_REFERENCE_USAGE($1.d.r, UsageAddrUsed);
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $2);
				}
			}
		}
	;

UnaryExpression:
		PreIncrementExpression
	|	PreDecrementExpression
	|	'+' UnaryExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaNumericPromotion($2.d.t); 
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $2);
				}
			}
		}
	|	'-' UnaryExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaNumericPromotion($2.d.t); 
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $2);
				}
			}
		}
	|	UnaryExpressionNotPlusMinus
	;

PreIncrementExpression:
		INC_OP UnaryExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaCheckNumeric($2.d.t); 
					RESET_REFERENCE_USAGE($2.d.r, UsageAddrUsed);
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $2);
				}
			}
		}
	;

PreDecrementExpression:
		DEC_OP UnaryExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaCheckNumeric($2.d.t);
					RESET_REFERENCE_USAGE($2.d.r, UsageAddrUsed);
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $2);
				}
			}
		}
	;

UnaryExpressionNotPlusMinus:
		PostfixExpression
	|	'~' UnaryExpression		{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaNumericPromotion($2.d.t);
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $2);
				}
			}
		}
	|	'!' UnaryExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					if ($2.d.t->m == TypeBoolean) $$.d.t = $2.d.t;
					else $$.d.t = &s_errorModifier; 
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $2);
				}
			}
		}
	|	CastExpression
	;

CastExpression:
		'(' ArrayType ')' UnaryExpression					{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					assert($2.d.s && $2.d.s->u.type);
					$$.d.t = $2.d.s->u.type; 
					$$.d.r = NULL;
					assert($$.d.t->m == TypeArray);
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $4);
					if (POSITION_IS_BETWEEN_IN_THE_SAME_FILE($4.b, s_cxRefPos, $4.e)
						&& s_spp[SPP_CAST_LPAR_POSITION].file == s_noneFileIndex) {
						s_spp[SPP_CAST_LPAR_POSITION] = $1.b;
						s_spp[SPP_CAST_RPAR_POSITION] = $3.b;
						s_spp[SPP_CAST_TYPE_BEGIN_POSITION] = $2.b;
						s_spp[SPP_CAST_TYPE_END_POSITION] = $2.e;
						s_spp[SPP_CAST_EXPRESSION_BEGIN_POSITION] = $4.b;
						s_spp[SPP_CAST_EXPRESSION_END_POSITION] = $4.e;
					}
				}
			}
		}
	|	'(' PrimitiveType ')' UnaryExpression				{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					XX_ALLOC($$.d.t, S_typeModifiers);
					FILLF_typeModifiers($$.d.t,$2.d.u,t,NULL,NULL,NULL);
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $4);
					if (POSITION_IS_BETWEEN_IN_THE_SAME_FILE($4.b, s_cxRefPos, $4.e)
						&& s_spp[SPP_CAST_LPAR_POSITION].file == s_noneFileIndex) {
						s_spp[SPP_CAST_LPAR_POSITION] = $1.b;
						s_spp[SPP_CAST_RPAR_POSITION] = $3.b;
						s_spp[SPP_CAST_TYPE_BEGIN_POSITION] = $2.b;
						s_spp[SPP_CAST_TYPE_END_POSITION] = $2.e;
						s_spp[SPP_CAST_EXPRESSION_BEGIN_POSITION] = $4.b;
						s_spp[SPP_CAST_EXPRESSION_END_POSITION] = $4.e;
					}
				}
			}
		}
	|	'(' Expression ')' UnaryExpressionNotPlusMinus		{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = $2.d.t; 
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $4);
					if (POSITION_IS_BETWEEN_IN_THE_SAME_FILE($4.b, s_cxRefPos, $4.e)
						&& s_spp[SPP_CAST_LPAR_POSITION].file == s_noneFileIndex) {
						s_spp[SPP_CAST_LPAR_POSITION] = $1.b;
						s_spp[SPP_CAST_RPAR_POSITION] = $3.b;
						s_spp[SPP_CAST_TYPE_BEGIN_POSITION] = $2.b;
						s_spp[SPP_CAST_TYPE_END_POSITION] = $2.e;
						s_spp[SPP_CAST_EXPRESSION_BEGIN_POSITION] = $4.b;
						s_spp[SPP_CAST_EXPRESSION_END_POSITION] = $4.e;
					}
				}
			}
		}
/*	this is original from the JLS
	|	'(' Name Dims ')' UnaryExpressionNotPlusMinus			{
			if (RegularPass()) {
			if (! SyntaxPassOnly()) {
			javaClassifyToTypeName($1.d); 
			} else {
				PropagateBorns($$, $1, $4);
			}
			}
		}
*/
	;

MultiplicativeExpression:
		UnaryExpression
	|	MultiplicativeExpression '*' UnaryExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaBinaryNumericPromotion($1.d.t,$3.d.t); 
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	|	MultiplicativeExpression '/' UnaryExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaBinaryNumericPromotion($1.d.t,$3.d.t); 
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	|	MultiplicativeExpression '%' UnaryExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaBinaryNumericPromotion($1.d.t,$3.d.t); 
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	;

AdditiveExpression:
		MultiplicativeExpression
	|	AdditiveExpression '+' MultiplicativeExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					int st1, st2;
					st1 = javaIsStringType($1.d.t);
					st2 = javaIsStringType($3.d.t);
					if (st1 && st2) {
						$$.d.t = $1.d.t;
					} else if (st1) {
						$$.d.t = $1.d.t;
						// TODO add reference to 'toString' on $3.d
					} else if (st2) {
						$$.d.t = $3.d.t;
						// TODO add reference to 'toString' on $1.d
					} else {
						$$.d.t = javaBinaryNumericPromotion($1.d.t,$3.d.t);
					}
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	|	AdditiveExpression '-' MultiplicativeExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaBinaryNumericPromotion($1.d.t, $3.d.t); 
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	;

ShiftExpression:
		AdditiveExpression
	|	ShiftExpression LEFT_OP AdditiveExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaNumericPromotion($1.d.t);
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	|	ShiftExpression RIGHT_OP AdditiveExpression		{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaNumericPromotion($1.d.t); 
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	|	ShiftExpression URIGHT_OP AdditiveExpression	{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaNumericPromotion($1.d.t); 
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	;

RelationalExpression:
		ShiftExpression
	|	RelationalExpression '<' ShiftExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					CrTypeModifier($$.d.t,TypeBoolean);
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	|	RelationalExpression '>' ShiftExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					CrTypeModifier($$.d.t,TypeBoolean);
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	|	RelationalExpression LE_OP ShiftExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					CrTypeModifier($$.d.t,TypeBoolean);
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	|	RelationalExpression GE_OP ShiftExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					CrTypeModifier($$.d.t,TypeBoolean);
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	|	RelationalExpression INSTANCEOF ReferenceType		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					CrTypeModifier($$.d.t,TypeBoolean);
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	;

EqualityExpression:
		RelationalExpression
	|	EqualityExpression EQ_OP RelationalExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					CrTypeModifier($$.d.t,TypeBoolean);
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	|	EqualityExpression NE_OP RelationalExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					CrTypeModifier($$.d.t,TypeBoolean);
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	;

AndExpression:
		EqualityExpression
	|	AndExpression '&' EqualityExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaBitwiseLogicalPromotion($1.d.t,$3.d.t);
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	;

ExclusiveOrExpression:
		AndExpression
	|	ExclusiveOrExpression '^' AndExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaBitwiseLogicalPromotion($1.d.t,$3.d.t);
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	;

InclusiveOrExpression:
		ExclusiveOrExpression
	|	InclusiveOrExpression '|' ExclusiveOrExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaBitwiseLogicalPromotion($1.d.t,$3.d.t);
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	;

ConditionalAndExpression:
		InclusiveOrExpression
	|	ConditionalAndExpression AND_OP InclusiveOrExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					CrTypeModifier($$.d.t,TypeBoolean);
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	;

ConditionalOrExpression:
		ConditionalAndExpression
	|	ConditionalOrExpression OR_OP ConditionalAndExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					CrTypeModifier($$.d.t,TypeBoolean);
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $3);
				}
			}
		}
	;

ConditionalExpression:
		ConditionalOrExpression
	|	ConditionalOrExpression '?' Expression ':' ConditionalExpression	{
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = javaConditionalPromotion($3.d.t, $5.d.t);
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					PropagateBorns($$, $1, $5);
				}
			}
		}
	;

AssignmentExpression:
		ConditionalExpression
	|	Assignment
	;

Assignment:
		LeftHandSide {
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					if ($1.d.t!=NULL && $1.d.t->m==TypeStruct) {
						s_cps.lastAssignementStruct = $1.d.t->u.t;
					}
				}
				$$.d = $1.d;
			}
		} AssignmentOperator AssignmentExpression		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					s_cps.lastAssignementStruct = NULL;
					if ($1.d.r != NULL && s_opt.cxrefs == OLO_EXTRACT) {
						S_reference *rr;
						rr = duplicateReference($1.d.r);
						$1.d.r->usg = s_noUsage;
						if ($3.d.u == '=') {
							RESET_REFERENCE_USAGE(rr, UsageLvalUsed);
						} else {
							RESET_REFERENCE_USAGE(rr, UsageAddrUsed);
						}
					} else {
						if ($3.d.u == '=') {
							RESET_REFERENCE_USAGE($1.d.r, UsageLvalUsed);
						} else {
							RESET_REFERENCE_USAGE($1.d.r, UsageAddrUsed);
						}
						$$.d.t = $1.d.t;
						$$.d.r = NULL;	
						/*
						  fprintf(dumpOut,": java Type Dump\n"); fflush(dumpOut);
						  javaTypeDump($1.d.t);
						  fprintf(dumpOut,"\n = \n"); fflush(dumpOut);
						  javaTypeDump($4.d.t);
						  fprintf(dumpOut,"\ndump end\n"); fflush(dumpOut);
						*/
					}
				} else {
					PropagateBorns($$, $1, $4);
					if (s_opt.taskRegime == RegimeEditServer) {
						if (POSITION_IS_BETWEEN_IN_THE_SAME_FILE($1.b, s_cxRefPos, $1.e)) {
							s_spp[SPP_ASSIGNMENT_OPERATOR_POSITION] = $3.b;
							s_spp[SPP_ASSIGNMENT_END_POSITION] = $4.e;
						}
					}
					$$.d.pp = NULL_POS;
				}
			}
		}
	;

LeftHandSide:
		Name					{
			if (RegularPass()) {
				$$.d.pp = javaGetNameStartingPosition($1.d);
				if (! SyntaxPassOnly()) {
					S_reference *rr;
					$$.d.t = javaClassifyToExpressionName($1.d, &rr);
					$$.d.r = rr;
				} else {
					PropagateBorns($$, $1, $1);
				}
			}
		}
	|	FieldAccess
	|	ArrayAccess
	|	CompletionExpressionName 					{ /* rule never used */ }
	;

AssignmentOperator:
		'='					{
			if (RegularPass()) $$.d.u = '='; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	| 	MUL_ASSIGN 			{
			if (RegularPass()) $$.d.u = MUL_ASSIGN; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	| 	DIV_ASSIGN			{
			if (RegularPass()) $$.d.u = DIV_ASSIGN; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	| 	MOD_ASSIGN			{
			if (RegularPass()) $$.d.u = MOD_ASSIGN; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	| 	ADD_ASSIGN			{
			if (RegularPass()) $$.d.u = ADD_ASSIGN; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	| 	SUB_ASSIGN			{
			if (RegularPass()) $$.d.u = SUB_ASSIGN; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	| 	LEFT_ASSIGN			{
			if (RegularPass()) $$.d.u = LEFT_ASSIGN; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	| 	RIGHT_ASSIGN		{
			if (RegularPass()) $$.d.u = RIGHT_ASSIGN; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	| 	URIGHT_ASSIGN		{
			if (RegularPass()) $$.d.u = URIGHT_ASSIGN; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	| 	AND_ASSIGN			{
			if (RegularPass()) $$.d.u = AND_ASSIGN; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	| 	XOR_ASSIGN			{
			if (RegularPass()) $$.d.u = XOR_ASSIGN; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	| 	OR_ASSIGN			{
			if (RegularPass()) $$.d.u = OR_ASSIGN; 
			PropagateBornsIfRegularSyntaxPass($$, $1, $1);
		}
	;

Expression:
		AssignmentExpression
	|	error					{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					$$.d.t = &s_errorModifier; 
					$$.d.r = NULL;
				} else {
					$$.d.pp = NULL_POS;
					SetNullBorns($$);
				}
			}
		}
	;

ConstantExpression:
		Expression
	;

/* ****************************************************************** */


Start_block:	{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					stackMemoryBlockStart(); 
				}
			}
		}
	;

Stop_block:		{ 
			if (RegularPass()) {
				if (! SyntaxPassOnly()) {
					stackMemoryBlockFree(); 
				}
			}
		}
	;


/* ****************************************************************** */
/* ****************************************************************** */
%%

void javaParsingInitializations() {
			S_symbol *ss;
			//&javaMapDirectoryFiles2(s_javaLangName, 
			//&			javaAddMapedTypeName, NULL, s_javaLangName, NULL);
			ss = javaTypeSymbolDefinition(s_javaLangObjectName,ACC_DEFAULT, TYPE_ADD_NO);
			s_javaObjectSymbol = ss;
			FILLF_typeModifiers(&s_javaObjectModifier,TypeStruct,t,ss,NULL,NULL);
			s_javaObjectModifier.u.t = ss;

			ss = javaTypeSymbolDefinition(s_javaLangStringName,ACC_DEFAULT, TYPE_ADD_NO);
			s_javaStringSymbol = ss;
			FILLF_typeModifiers(&s_javaStringModifier,TypeStruct,t,ss,NULL,NULL);
			s_javaStringModifier.u.t = ss;

			ss = javaTypeSymbolDefinition(s_javaLangClassName,ACC_DEFAULT, TYPE_ADD_NO);
			FILLF_typeModifiers(&s_javaClassModifier,TypeStruct,t,ss,NULL,NULL);
			s_javaClassModifier.u.t = ss;
			s_javaCloneableSymbol = javaTypeSymbolDefinition(s_javaLangCloneableName,ACC_DEFAULT, TYPE_ADD_NO);
			s_javaIoSerializableSymbol = javaTypeSymbolDefinition(s_javaIoSerializableName,ACC_DEFAULT, TYPE_ADD_NO);

			javaInitArrayObject();
}

static S_completionFunTab spCompletionsTab[]  = {
	{ COMPL_THIS_PACKAGE_SPECIAL,	javaCompleteThisPackageName },
	{ COMPL_CLASS_DEF_NAME,			javaCompleteClassDefinitionNameSpecial },
	{ COMPL_FULL_INHERITED_HEADER,	javaCompleteFullInheritedMethodHeader },
	{ COMPL_CONSTRUCTOR_NAME0,		javaCompleteHintForConstructSingleName },
	{ COMPL_VARIABLE_NAME_HINT,		javaHintVariableName },
	{ COMPL_UP_FUN_PROFILE,			javaHintCompleteMethodParameters },
	{0,NULL}
};

static S_completionFunTab completionsTab[]  = {
	{ COMPL_TYPE_NAME0,				javaCompleteTypeSingleName },
	{ COMPL_TYPE_NAME1,				javaCompleteTypeCompName },
	{ COMPL_PACKAGE_NAME0,			javaCompletePackageSingleName },
	{ COMPL_PACKAGE_NAME1,			javaCompletePackageCompName },
	{ COMPL_EXPRESSION_NAME0,		javaCompleteExprSingleName },
	{ COMPL_EXPRESSION_NAME1,		javaCompleteExprCompName },
	{ COMPL_CONSTRUCTOR_NAME0,		javaCompleteConstructSingleName },
	{ COMPL_CONSTRUCTOR_NAME1,		javaCompleteConstructCompName },
	{ COMPL_CONSTRUCTOR_NAME2,		javaCompleteConstructNestNameName },
	{ COMPL_CONSTRUCTOR_NAME3,		javaCompleteConstructNestPrimName },
	{ COMPL_STRUCT_REC_PRIM,		javaCompleteStrRecordPrimary },
	{ COMPL_STRUCT_REC_SUPER,		javaCompleteStrRecordSuper },
	{ COMPL_QUALIF_SUPER,			javaCompleteStrRecordQualifiedSuper },
	{ COMPL_CLASS_DEF_NAME,			javaCompleteClassDefinitionName },
	{ COMPL_THIS_CONSTRUCTOR,		javaCompleteThisConstructor },
	{ COMPL_SUPER_CONSTRUCTOR1,		javaCompleteSuperConstructor },
	{ COMPL_SUPER_CONSTRUCTOR2,		javaCompleteSuperNestedConstructor },
  	{ COMPL_METHOD_NAME0,			javaCompleteUpMethodSingleName },
	{ COMPL_METHOD_NAME1,			javaCompleteMethodCompName },
	{0,NULL}
};


static S_completionFunTab hintCompletionsTab[]  = {
	{ COMPL_TYPE_NAME0,				javaHintCompleteNonImportedTypes },
// Finally I do not know if this is practical
//&	{ COMPL_IMPORT_SPECIAL,			javaHintImportFqt },
	{0,NULL}
};

void makeJavaCompletions(char *s, int len, S_position *pos) {
	int tok, yyn, i;
	S_cline compLine;
/*fprintf(dumpOut,": completing \"%s\" in state %d\n",s,lastyystate);*/
	LICENSE_CHECK();
	strncpy(s_completions.idToProcess, s, MAX_FUN_NAME_SIZE);
	s_completions.idToProcess[MAX_FUN_NAME_SIZE-1] = 0;
	FILL_completions(&s_completions, len, *pos, 0, 0, 0, 0, 0, 0);
	/* special wizard completions */
	for (i=0;(tok=spCompletionsTab[i].token)!=0; i++) {
	    if (((yyn = yysindex[lastyystate]) && (yyn += tok) >= 0 &&
				yyn <= YYTABLESIZE && yycheck[yyn] == tok) ||
			((yyn = yyrindex[lastyystate]) && (yyn += tok) >= 0 &&
		        yyn <= YYTABLESIZE && yycheck[yyn] == tok)) {
/*fprintf(stderr,"completing %d==%s v stave %d\n",i,yyname[tok],lastyystate);*/
				(*spCompletionsTab[i].fun)(&s_completions);
				if (s_completions.abortFurtherCompletions) return;
		}
	}

	/* If there is a wizard completion, RETURN now */
	if (s_completions.ai != 0 && s_opt.cxrefs != OLO_SEARCH) return;
	for (i=0;(tok=completionsTab[i].token)!=0; i++) {
	    if (((yyn = yysindex[lastyystate]) && (yyn += tok) >= 0 &&
				yyn <= YYTABLESIZE && yycheck[yyn] == tok) ||
			((yyn = yyrindex[lastyystate]) && (yyn += tok) >= 0 &&
		        yyn <= YYTABLESIZE && yycheck[yyn] == tok)) {
/*&fprintf(dumpOut,":completing %d==%s v stave %d\n",i,yyname[tok],lastyystate);fflush(dumpOut);&*/
				(*completionsTab[i].fun)(&s_completions);
/*fprintf(dumpOut,":completed\n");fflush(dumpOut);*/
				if (s_completions.abortFurtherCompletions) return;
		}
	}

	/* basic language tokens */
	for (tok=0; tok<LAST_TOKEN; tok++) {
		if (tok==IDENTIFIER) continue;
	    if (((yyn = yysindex[lastyystate]) && (yyn += tok) >= 0 &&
				yyn <= YYTABLESIZE && yycheck[yyn] == tok) ||
			((yyn = yyrindex[lastyystate]) && (yyn += tok) >= 0 &&
		        yyn <= YYTABLESIZE && yycheck[yyn] == tok)) {
				if (s_tokenName[tok]!= NULL) {
					if (isalpha(*s_tokenName[tok]) || *s_tokenName[tok]=='_') {
						FILL_cline(&compLine, s_tokenName[tok], NULL, TypeKeyword,0, 0, NULL,NULL);
						processName(s_tokenName[tok], &compLine, 0, &s_completions);
					} else {
						//& FILL_cline(&compLine, s_tokenName[tok], NULL, TypeToken,0, 0, NULL,NULL);
					}
				}
		}
	}

	/* If the completon window is shown, or there is no completion, 
       add also hints (should be optionally) */
	//&if (s_completions.comPrefix[0]!=0  && (s_completions.ai != 0)
	//&	&& s_opt.cxrefs != OLO_SEARCH) return;

	for (i=0;(tok=hintCompletionsTab[i].token)!=0; i++) {
	    if (((yyn = yysindex[lastyystate]) && (yyn += tok) >= 0 &&
				yyn <= YYTABLESIZE && yycheck[yyn] == tok) ||
			((yyn = yyrindex[lastyystate]) && (yyn += tok) >= 0 &&
		        yyn <= YYTABLESIZE && yycheck[yyn] == tok)) {
				(*hintCompletionsTab[i].fun)(&s_completions);
				if (s_completions.abortFurtherCompletions) return;
		}
	}
	LICENSE_CHECK();
}





