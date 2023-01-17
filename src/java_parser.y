/*
../../../byacc-1.9/yacc: 2 shift/reduce conflicts.
*/
%{

#define RECURSIVE

#define java_yylex yylex

#include "java_parser.h"

#include "globals.h"
#include "options.h"
#include "jslsemact.h"
#include "jsemact.h"
#include "editor.h"
#include "cxref.h"
#include "yylex.h"
#include "stdinc.h"
#include "head.h"
#include "misc.h"
#include "commons.h"
#include "complete.h"
#include "proto.h"
#include "protocol.h"
#include "extract.h"
#include "semact.h"
#include "symbol.h"
#include "list.h"
#include "filedescriptor.h"
#include "filetable.h"
#include "typemodifier.h"

#include "ast.h"

#include "log.h"


#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define java_yyerror styyerror
#define yyErrorRecovery styyErrorRecovery

#define JslAddComposedType(ddd, ttt) jslAppendComposedType(&ddd->u.typeModifier, ttt)

#define JslImportSingleDeclaration(iname) {\
    Symbol *sym;\
    jslClassifyAmbiguousTypeName(iname, &sym);\
    jslTypeSymbolDefinition(iname->id.name, iname->next, ADD_YES, ORDER_PREPEND, true);\
}

/* Import on demand has to solve following situation (not handled by JSL) */
/* In case that there is a class and package differing only in letter case in name */
/* then even if classified to type it should be reclassified dep on the case */

static void jslImportOnDemandDeclaration(struct idList *iname) {
    Symbol *sym;
    int st;
    st = jslClassifyAmbiguousTypeName(iname, &sym);
    if (st == TypeStruct) {
        javaLoadClassSymbolsFromFile(sym);
        jslAddNestedClassesToJslTypeTab(sym, ORDER_APPEND);
    } else {
        javaMapDirectoryFiles2(iname,jslAddMapedImportTypeName,NULL,iname,NULL);
    }
}

#define SetPrimitiveTypePos(res, typ) {         \
        if (1 || SyntaxPassOnly()) {            \
            res = StackMemoryAlloc(Position);   \
            *res = typ->position;                      \
        }                                       \
        else assert(0);                         \
    }

/* NOTE: These cannot be unmacrofied since the "node" can have different types */
#define PropagateBoundaries(node, startSymbol, endSymbol) {node.b=startSymbol.b; node.e=endSymbol.e;}
#define PropagateBoundariesIfRegularSyntaxPass(node, startSymbol, endSymbol) {         \
        if (regularPass()) {                                            \
            if (SyntaxPassOnly()) {PropagateBoundaries(node, startSymbol, endSymbol);} \
        }                                                               \
    }
#define SetNullBoundariesFor(node) {node.b=noPosition; node.e=noPosition;}

#define NULL_POS NULL

static bool regularPass(void) { return s_jsl == NULL; }
static bool inSecondJslPass() {
    return !regularPass() && s_jsl->pass==2;
}

#define SyntaxPassOnly() (options.serverOperation==OLO_GET_PRIMARY_START || options.serverOperation==OLO_GET_PARAM_COORDINATES || options.serverOperation==OLO_SYNTAX_PASS_ONLY || javaPreScanOnly)

#define ComputingPossibleParameterCompletion() (regularPass() && (! SyntaxPassOnly()) && options.mode==ServerMode && options.serverOperation==OLO_COMPLETION)

typedef struct whileExtractData {
    int				i1;
    int             i2;
    struct symbol	*i3;
    struct symbol	*i4;
} S_whileExtractData;

static S_whileExtractData *newWhileExtractData(int i1, int i2, Symbol *i3, Symbol *i4) {
    S_whileExtractData *whileExtractData;

    whileExtractData = StackMemoryAlloc(S_whileExtractData);
    whileExtractData->i1 = i1;
    whileExtractData->i2 = i2;
    whileExtractData->i3 = i3;
    whileExtractData->i4 = i4;

    return whileExtractData;
}


%}

/* Token definitions *must* be the same in all parsers. The following
   is a marker, it must be the same as in the Makefile check */
/* START OF COMMON TOKEN DEFINITIONS */

/* ************************* SPECIALS ****************************** */
/* c+c++ */

%token TYPE_NAME

/* ************************* OPERATORS ****************************** */
/* common */
%token INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token XOR_ASSIGN OR_ASSIGN

/* c-only */
%token PTR_OP ELLIPSIS

/* java -only */
%token URIGHT_OP URIGHT_ASSIGN

/* yacc-only */
%token YACC_PERC YACC_DPERC

/* ************************** KEYWORDS ******************************** */

%token STATIC BREAK CASE CHAR CONST CONTINUE DEFAULT DO DOUBLE ELSE
%token FLOAT FOR GOTO IF INT LONG RETURN SHORT SWITCH VOID VOLATILE WHILE

/* c-special */
%token TYPEDEF EXTERN AUTO REGISTER SIGNED UNSIGNED STRUCT UNION ENUM
%token SIZEOF RESTRICT _ATOMIC _BOOL _THREADLOCAL _NORETURN
/* hmm */
%token ANONYMOUS_MODIFIER

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
%token CPP_INCLUDE CPP_INCLUDE_NEXT CPP_DEFINE CPP_IFDEF CPP_IFNDEF CPP_IF CPP_ELSE CPP_ENDIF
%token CPP_ELIF CPP_UNDEF
%token CPP_PRAGMA CPP_LINE
%token CPP_DEFINE0       /* macro with no argument */
%token CPP_TOKENS_END

%token CPP_COLLATION     /* ## in macro body */
%token CPP_DEFINED_OP    /* defined(xxx) in #if */

/* ******************************************************************** */
/* special token signaling end of input */
%token EOI_TOKEN

/* ******************************************************************** */
/* special tokens used for nontrivial caching !!!! not used */
%token CACHING1_TOKEN

/* ******************************************************************** */
%token OL_MARKER_TOKEN OL_MARKER_TOKEN1 OL_MARKER_TOKEN2

/* ******************************************************************** */
%token TMP_TOKEN1 TMP_TOKEN2

/* ************************ MULTI TOKENS START ************************ */
%token MULTI_TOKENS_START

/* commons */
%token IDENTIFIER CONSTANT LONG_CONSTANT
%token FLOAT_CONSTANT DOUBLE_CONSTANT
%token STRING_LITERAL
%token LINE_TOKEN
%token IDENT_TO_COMPLETE        /* identifier under cursor */

/* c-only */
%token CPP_MACRO_ARGUMENT IDENT_NO_CPP_EXPAND

/* java-only */
%token CHAR_LITERAL

/* ****************************************************************** */

%token LAST_TOKEN

/* END OF COMMON TOKEN DEFINITIONS */
/* Token definitions *must* be the same in all parsers. The above
   is a marker, it must be the same as in the Makefile check */

%union {
#include "yystype.h"
}


/* ********************************************************* */

%type <ast_id> Identifier This Super New Package Import Throw Try Catch
%type <ast_id> ClassDeclaration InterfaceDeclaration
%type <ast_integer> DimExprs Dims_opt Dims
%type <ast_integer> _ncounter_ _nlabel_ _ngoto_ _nfork_ IfThenElseStatementPrefix
%type <ast_intPair> ForStatementPrefix
%type <ast_whileData> WhileStatementPrefix
%type <ast_unsignedPositionPair> PrimitiveType NumericType IntegralType FloatingPointType
%type <ast_unsignedPositionPair> AssignmentOperator
%type <ast_unsigned> Modifier Modifiers Modifiers_opt
%type <ast_idList> Name SimpleName QualifiedName PackageDeclaration_opt NewName
%type <ast_idList> SingleTypeImportDeclaration TypeImportOnDemandDeclaration
%type <ast_symbol> JavaType AssignmentType ReferenceType ClassOrInterfaceType
%type <ast_symbol> ExtendClassOrInterfaceType
%type <ast_symbol> ClassType InterfaceType
%type <ast_symbol> FormalParameter
%type <ast_symbol> VariableDeclarator VariableDeclaratorId
%type <ast_symbol> MethodHeader MethodDeclarator AbstractMethodDeclaration
%type <ast_symbol> FieldDeclaration ConstantDeclaration
%type <ast_symbol> ConstructorDeclarator
%type <ast_symbol> VariableDeclarators
%type <ast_symbol> LocalVariableDeclaration LocalVarDeclUntilInit
%type <ast_symbolList> Throws_opt ClassTypeList
%type <ast_typeModifiersListPositionListPair> ArgumentList_opt ArgumentList
%type <ast_symbolPositionListPair> FormalParameterList_opt FormalParameterList

%type <ast_expressionType> Primary PrimaryNoNewArray ClassInstanceCreationExpression
%type <ast_expressionType> ArrayCreationExpression FieldAccess MethodInvocation
%type <ast_expressionType> ArrayAccess PostfixExpression PostIncrementExpression
%type <ast_expressionType> PostDecrementExpression UnaryExpression
%type <ast_expressionType> PreIncrementExpression PreDecrementExpression
%type <ast_expressionType> UnaryExpressionNotPlusMinus CastExpression
%type <ast_expressionType> MultiplicativeExpression AdditiveExpression
%type <ast_expressionType> ShiftExpression RelationalExpression EqualityExpression
%type <ast_expressionType> AndExpression ExclusiveOrExpression InclusiveOrExpression
%type <ast_expressionType> ConditionalAndExpression ConditionalOrExpression
%type <ast_expressionType> ConditionalExpression AssignmentExpression Assignment
%type <ast_expressionType> LeftHandSide Expression ConstantExpression
%type <ast_expressionType> StatementExpression
%type <ast_expressionType> Literal VariableInitializer ArrayInitializer
%type <ast_symbolPositionPair> ArrayType
%type <ast_position> MethodBody Block

%type <ast_id> IDENTIFIER THIS SUPER NEW PACKAGE IMPORT CLASS VOID
%type <ast_id> BOOLEAN CHAR LONG INT SHORT BYTE DOUBLE FLOAT THROW TRY CATCH
%type <ast_id> NULL_LITERAL
%type <ast_integer> CONSTANT

%type <ast_position> STRING_LITERAL '(' ',' ')' ';' '{' '}' '[' ']' ':' '+' '-' '~' '!' '='
%type <ast_position> INC_OP DEC_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN SUB_ASSIGN
%type <ast_position> LEFT_ASSIGN RIGHT_ASSIGN URIGHT_ASSIGN AND_ASSIGN XOR_ASSIGN OR_ASSIGN

%type <ast_position> TRUE_LITERAL FALSE_LITERAL LONG_CONSTANT FLOAT_CONSTANT
%type <ast_position> DOUBLE_CONSTANT CHAR_LITERAL STRING_LITERAL

%type <ast_position> ImportDeclarations ImportDeclarations_opt
%type <ast_position> LabelDefininigIdentifier LabelUseIdentifier
%type <ast_position> TypeDeclarations_opt TypeDeclarations TypeDeclaration
%type <ast_position> PUBLIC PROTECTED PRIVATE STATIC ABSTRACT FINAL NATIVE
%type <ast_position> SYNCHRONIZED STRICTFP TRANSIENT VOLATILE EXTENDS
%type <ast_position> IMPLEMENTS THROWS INTERFACE ASSERT IF ELSE SWITCH
%type <ast_position> CASE DEFAULT WHILE DO FOR BREAK CONTINUE RETURN
%type <ast_position> FINALLY
%type <ast_position> ClassBody FunctionInnerClassDeclaration Super_opt
%type <ast_position> Interfaces_opt InterfaceTypeList ClassBodyDeclarations
%type <ast_position> ClassBodyDeclaration ClassMemberDeclaration ClassInitializer
%type <ast_position> ConstructorDeclaration ConstructorBody ExplicitConstructorInvocation
%type <ast_position> MethodDeclaration InterfaceMemberDeclarations InterfaceBody
%type <ast_position> ExtendsInterfaces_opt ExtendsInterfaces InterfaceMemberDeclaration
%type <ast_position> BlockStatements BlockStatement LocalVariableDeclarationStatement
%type <ast_position> FunctionInnerClassDeclaration Statement StatementWithoutTrailingSubstatement
%type <ast_position> LabeledStatement IfThenStatement IfThenElseStatement WhileStatement
%type <ast_position> ForStatement StatementNoShortIf LabeledStatementNoShortIf
%type <ast_position> IfThenElseStatementNoShortIf WhileStatementNoShortIf ForStatementNoShortIf
%type <ast_position> EmptyStatement ExpressionStatement SwitchStatement DoStatement
%type <ast_position> BreakStatement ContinueStatement ReturnStatement SynchronizedStatement
%type <ast_position> ThrowStatement TryStatement AssertStatement
%type <ast_position> SwitchBlockStatementGroups SwitchBlockStatementGroup SwitchLabels
%type <ast_position> SwitchLabel ForInit_opt StatementExpressionList
%type <ast_position> ForUpdate_opt TryCatches Catches Finally CatchClause
%type <ast_position> VariableInitializers SwitchBlock MaybeExpression ForKeyword
%type <ast_position> ForStatementBody ForStatementNoShortIfBody StatementExpressionList
%type <ast_position> ForUpdate_opt DimExpr
%type <ast_nestedConstrTokenType> NestedConstructorInvocation

%type <erfs> _erfs_


%start Goal

%%

Goal:	CompilationUnit
    ;

/* *************************** Literals ********************************* */

Literal
    :   TRUE_LITERAL		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = &noPosition;
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	FALSE_LITERAL		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = &noPosition;
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	CONSTANT			{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = newSimpleTypeModifier(TypeInt);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = &noPosition;
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	LONG_CONSTANT		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = newSimpleTypeModifier(TypeLong);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = &noPosition;
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	FLOAT_CONSTANT		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = newSimpleTypeModifier(TypeFloat);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = &noPosition;
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	DOUBLE_CONSTANT		{
            if (regularPass()) {
                $$.data.typeModifier = newSimpleTypeModifier(TypeDouble);
                $$.data.reference = NULL;
                $$.data.position = &noPosition;
                if (SyntaxPassOnly()) {PropagateBoundaries($$, $1, $1);}
            }
        }
    |	CHAR_LITERAL		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = newSimpleTypeModifier(TypeChar);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = &noPosition;
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	STRING_LITERAL		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = &s_javaStringModifier;
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = StackMemoryAlloc(Position);
                    *$$.data.position = $1.data;
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	NULL_LITERAL		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = newSimpleTypeModifier(TypeNull);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = StackMemoryAlloc(Position);
                    *$$.data.position = $1.data->position;
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    ;

/* ************************* Types, Values, Variables ******************* */

JavaType
    :   PrimitiveType	{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data = typeSpecifier1($1.data.u);
                    parsedInfo.lastDeclaratorType = NULL;
                } else {
                    PropagateBoundaries($$, $1, $1);
                }
            };
            if (inSecondJslPass()) {
                $$.data = jslTypeSpecifier1($1.data.u);
            }
        }
    |	ReferenceType	{
            $$.data = $1.data;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

PrimitiveType
    :   NumericType		/* $$ = $1 */
    |	BOOLEAN			{
            $$.data.u  = TypeBoolean;
            if (regularPass()) {
                SetPrimitiveTypePos($$.data.position, $1.data);
                PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
            }
        }
    ;

NumericType
    :   IntegralType			/* $$ = $1 */
    |	FloatingPointType		/* $$ = $1 */
    ;

IntegralType
    :   BYTE			{
            $$.data.u  = TypeByte;
            if (regularPass()) SetPrimitiveTypePos($$.data.position, $1.data);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |	SHORT			{
            $$.data.u  = TypeShort;
            if (regularPass()) SetPrimitiveTypePos($$.data.position, $1.data);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |	INT				{
            $$.data.u  = TypeInt;
            if (regularPass()) SetPrimitiveTypePos($$.data.position, $1.data);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |	LONG			{
            $$.data.u  = TypeLong;
            if (regularPass()) SetPrimitiveTypePos($$.data.position, $1.data);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |	CHAR			{
            $$.data.u  = TypeChar;
            if (regularPass()) SetPrimitiveTypePos($$.data.position, $1.data);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

FloatingPointType
    :   FLOAT			{
            $$.data.u  = TypeFloat;
            if (regularPass()) SetPrimitiveTypePos($$.data.position, $1.data);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |	DOUBLE			{
            $$.data.u  = TypeDouble;
            if (regularPass()) SetPrimitiveTypePos($$.data.position, $1.data);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

ReferenceType
    :   ClassOrInterfaceType		/*& { $$.data = $1.data; } &*/
    |	ArrayType					{
            $$.data = $1.data.symbol;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

ClassOrInterfaceType
    :   Name			{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    javaClassifyToTypeName($1.data,UsageUsed, &$$.data, USELESS_FQT_REFS_ALLOWED);
                    $$.data = javaTypeNameDefinition($1.data);
                    assert($$.data->u.typeModifier);
                    parsedInfo.lastDeclaratorType = $$.data->u.typeModifier->u.t;
                } else {
                    PropagateBoundaries($$, $1, $1);
                }
            };
            if (inSecondJslPass()) {
                Symbol *str;
                jslClassifyAmbiguousTypeName($1.data, &str);
                $$.data = jslTypeNameDefinition($1.data);
            }
        }
    |	CompletionTypeName	{ /* rule never reduced */ }
    ;

/* following is the same, just to distinguish type after EXTEND keyword */
ExtendClassOrInterfaceType
    :   Name			{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    javaClassifyToTypeName($1.data, USAGE_EXTEND_USAGE, &$$.data, USELESS_FQT_REFS_ALLOWED);
                    $$.data = javaTypeNameDefinition($1.data);
                } else {
                    PropagateBoundaries($$, $1, $1);
                }
            };
            if (inSecondJslPass()) {
                Symbol *str;
                jslClassifyAmbiguousTypeName($1.data, &str);
                $$.data = jslTypeNameDefinition($1.data);
            }
        }
    |	CompletionTypeName	{ /* rule never reduced */ }
    ;

ClassType
    :   ClassOrInterfaceType		/*& { $$.data = $1.data; } &*/
    ;

InterfaceType
    :   ClassOrInterfaceType		/*& { $$.data = $1.data; } &*/
    ;

ArrayType
    :   PrimitiveType '[' ']'		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.symbol = typeSpecifier1($1.data.u);
                    $$.data.symbol->u.typeModifier = prependComposedType($$.data.symbol->u.typeModifier, TypeArray);
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
                $$.data.position = $1.data.position;
                parsedInfo.lastDeclaratorType = NULL;
            };
            if (inSecondJslPass()) {
                $$.data.symbol = jslTypeSpecifier1($1.data.u);
                $$.data.symbol->u.typeModifier = jslPrependComposedType($$.data.symbol->u.typeModifier, TypeArray);
            }
        }
    |	Name '[' ']'				{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    javaClassifyToTypeName($1.data,UsageUsed, &($$.data.symbol), USELESS_FQT_REFS_ALLOWED);
                    $$.data.symbol = javaTypeNameDefinition($1.data);
                    assert($$.data.symbol && $$.data.symbol->u.typeModifier);
                    parsedInfo.lastDeclaratorType = $$.data.symbol->u.typeModifier->u.t;
                    $$.data.symbol->u.typeModifier = prependComposedType($$.data.symbol->u.typeModifier, TypeArray);
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
                $$.data.position = javaGetNameStartingPosition($1.data);
            };
            if (inSecondJslPass()) {
                Symbol *ss;
                jslClassifyAmbiguousTypeName($1.data, &ss);
                $$.data.symbol = jslTypeNameDefinition($1.data);
                $$.data.symbol->u.typeModifier = jslPrependComposedType($$.data.symbol->u.typeModifier, TypeArray);
            }
        }
    |	ArrayType '[' ']'			{
            if (regularPass()) {
                $$.data = $1.data;
                if (! SyntaxPassOnly()) {
                    $$.data.symbol->u.typeModifier = prependComposedType($$.data.symbol->u.typeModifier, TypeArray);
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            };
            if (inSecondJslPass()) {
                $$.data = $1.data;
                $$.data.symbol->u.typeModifier = jslPrependComposedType($$.data.symbol->u.typeModifier, TypeArray);
            }
        }
    |   CompletionTypeName '[' ']'	{ /* rule never used */ }
    ;

/* ****************************** Names **************************** */

Identifier:	IDENTIFIER			{
            if (regularPass()) $$.data = newCopyOfId($1.data);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;
This:		THIS				{
            if (regularPass()) $$.data = newCopyOfId($1.data);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;
Super:		SUPER				{
            if (regularPass()) $$.data = newCopyOfId($1.data);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;
New:		NEW					{
            if (regularPass()) $$.data = newCopyOfId($1.data);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;
Import:		IMPORT				{
            if (regularPass()) $$.data = newCopyOfId($1.data);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;
Package:	PACKAGE				{
            if (regularPass()) $$.data = newCopyOfId($1.data);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;
Throw:		THROW				{
            if (regularPass()) $$.data = newCopyOfId($1.data);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;
Try:		TRY					{
            if (regularPass()) $$.data = newCopyOfId($1.data);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;
Catch:		CATCH				{
            if (regularPass()) $$.data = newCopyOfId($1.data);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

Name
    :   SimpleName				{
            $$.data = $1.data;
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    assert(javaStat);
                    javaStat->lastParsedName = $1.data;
                } else {
                    PropagateBoundaries($$, $1, $1);
                    javaCheckForPrimaryStart(&$1.data->id.position, &$1.data->id.position);
                    javaCheckForStaticPrefixStart(&$1.data->id.position, &$1.data->id.position);
                }
            };
        }
    |	QualifiedName			{
            $$.data = $1.data;
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    assert(javaStat);
                    javaStat->lastParsedName = $1.data;
                } else {
                    PropagateBoundaries($$, $1, $1);
                    javaCheckForPrimaryStartInNameList($1.data, javaGetNameStartingPosition($1.data));
                    javaCheckForStaticPrefixInNameList($1.data, javaGetNameStartingPosition($1.data));
                }
            };
        }
    ;

SimpleName
    :   IDENTIFIER				{
            $$.data = StackMemoryAlloc(IdList);
            fillIdList($$.data, *$1.data, $1.data->name, TypeDefault, NULL);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

QualifiedName
    :   Name '.' IDENTIFIER		{
            $$.data = StackMemoryAlloc(IdList);
            fillIdList($$.data, *$3.data, $3.data->name, TypeDefault, $1.data);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
        }
    ;

/* following rules are never reduced, they keep completion information */

CompletionTypeName
    :   COMPL_TYPE_NAME0
    |	Name '.' COMPL_TYPE_NAME1
    ;

CompletionFQTypeName
    :   COMPL_PACKAGE_NAME0
    |	Name '.' COMPL_TYPE_NAME1
    ;

CompletionPackageName
    :   COMPL_PACKAGE_NAME0
    |	Name '.' COMPL_PACKAGE_NAME1
    ;

CompletionExpressionName
    :   COMPL_EXPRESSION_NAME0
    |	Name '.' COMPL_EXPRESSION_NAME1
    ;

CompletionConstructorName
    :   COMPL_CONSTRUCTOR_NAME0
    |	Name '.' COMPL_CONSTRUCTOR_NAME1
    ;

/*
CompletionMethodName
    :   COMPL_METHOD_NAME0
    |	Name '.' COMPL_METHOD_NAME1
    ;
*/

LabelDefininigIdentifier
    :   IDENTIFIER          {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    labelReference($1.data,UsageDefined);
                } else {
                    PropagateBoundaries($$, $1, $1);
                }
            };
        }
    |	COMPL_LABEL_NAME		{ assert(0); /* token never used */ }
    ;

LabelUseIdentifier
    :   IDENTIFIER          {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    labelReference($1.data,UsageUsed);
                } else {
                    PropagateBoundaries($$, $1, $1);
                }
            };
        }
    |	COMPL_LABEL_NAME		{ assert(0); /* token never used */ }
    ;

/* ****************************** packages ************************* */

CompilationUnit: {
            if (regularPass()) {
                assert(javaStat);
                *javaStat = s_initJavaStat;
                s_javaThisPackageName = "";      // preset for case if copied somewhere
            };
        } PackageDeclaration_opt {
            if (regularPass()) {
                if ($2.data == NULL) {	/* anonymous package */
                    s_javaThisPackageName = "";
                } else {
                    javaClassifyToPackageName($2.data);
                    s_javaThisPackageName = javaCreateComposedName(NULL,$2.data,'/',
                                                                   NULL,NULL,0);
                }
                javaStat->currentPackage = s_javaThisPackageName;
                if (! SyntaxPassOnly()) {

                    int             packlen;
                    char            *cdir, *fname;
                    JslTypeTab	*jsltypeTab;

                    // it is important to know the package before everything
                    // else, as it must be set on class adding in order to set
                    // isinCurrentPackage field. !!!!!!!!!!!!!!!!
                    // this may be problem for CACHING !!!!
                    if ($2.data == NULL) {	/* anonymous package */
                        int j = 0;
                        javaStat->className = NULL;
                        for (int i=0; currentFile.fileName[i]; i++) {
                            if (currentFile.fileName[i] == FILE_PATH_SEPARATOR)
                                j=i;
                        }
                        cdir = StackMemoryAllocC(j+1, char);
                        strncpy(cdir,currentFile.fileName,j); cdir[j]=0;
                        javaStat->unnamedPackagePath = cdir;
                        javaCheckIfPackageDirectoryIsInClassOrSourcePath(cdir);
                    } else {
                        javaAddPackageDefinition($2.data);
                        javaStat->className = $2.data;
                        int j = 0;
                        for (int i=0; currentFile.fileName[i]; i++) {
                            if (currentFile.fileName[i] == FILE_PATH_SEPARATOR)
                                j=i;
                        }
                        packlen = strlen(s_javaThisPackageName);
                        if (j>packlen && filenameCompare(s_javaThisPackageName,&currentFile.fileName[j-packlen],packlen)==0){
                            cdir = StackMemoryAllocC(j-packlen, char);
                            strncpy(cdir, currentFile.fileName, j-packlen-1); cdir[j-packlen-1]=0;
                            javaStat->namedPackagePath = cdir;
                            javaStat->currentPackage = "";
                            javaCheckIfPackageDirectoryIsInClassOrSourcePath(cdir);
                        } else {
                            if (options.mode != ServerMode) {
                                warningMessage(ERR_ST, "package name does not match directory name");
                            }
                        }
                    }
                    javaParsingInitializations();
                    // make first and second pass through file
                    assert(s_jsl == NULL); // no nesting
                    jsltypeTab = StackMemoryAlloc(JslTypeTab);
                    jslTypeTabInit(jsltypeTab, MAX_JSL_SYMBOLS);
                    javaReadSymbolFromSourceFileInit(olOriginalFileNumber,
                                                     jsltypeTab);

                    fname = getFileItem(olOriginalFileNumber)->name;
                    if (options.mode == ServerMode) {
                        // this must be before reading 's_olOriginalComFile' !!!
                        if (editorFileExists(fname)) {
                            javaReadSymbolsFromSourceFileNoFreeing(fname, fname);
                        }
                    }

                    // this must be last reading of this class before parsing
                    FileItem *fileItem = getFileItem(olOriginalComFileNumber);
                    if (editorFileExists(fileItem->name)) {
                        javaReadSymbolsFromSourceFileNoFreeing(
                            fileItem->name, fname);
                    }

                    javaReadSymbolFromSourceFileEnd();
                    javaAddJslReadedTopLevelClasses(jsltypeTab);
                    assert(s_jsl == NULL);
                }
                // -----------------------------------------------
            } else {
                // -----------------------------------------------
                // jsl pass initialisation
                char            *pname;
                char			ppp[MAX_FILE_NAME_SIZE];

                s_jsl->waitList = NULL;
                if ($2.data != NULL) {
                    javaClassifyToPackageName($2.data);
                }
                javaCreateComposedName(NULL,$2.data,'/',NULL,ppp,MAX_FILE_NAME_SIZE);
                pname = StackMemoryAllocC(strlen(ppp)+1, char);
                strcpy(pname, ppp);
                s_jsl->classStat = newJslClassStat($2.data, NULL, pname, NULL);
                if (inSecondJslPass()) {
                    char        cdir[MAX_FILE_NAME_SIZE];
                    int         i;
                    int			j;
                    /* add this package types */
                    if ($2.data == NULL) {	/* anonymous package */
                        for(i=0,j=0; currentFile.fileName[i]; i++) {
                            if (currentFile.fileName[i] == FILE_PATH_SEPARATOR) j=i;
                        }
                        strncpy(cdir,currentFile.fileName,j);
                        cdir[j]=0;
                        mapDirectoryFiles(cdir, jslAddMapedImportTypeName, ALLOW_EDITOR_FILES,
                                          "", "", NULL, NULL, NULL);
                        // why this is there, it makes problem when moving a class
                        // it stays in fileTab and there is a clash!
                        // [2/8/2003] Maybe I should put it out
                        jslAddAllPackageClassesFromFileTab(NULL);
                    } else {
                        javaMapDirectoryFiles2($2.data,jslAddMapedImportTypeName,
                                                NULL,$2.data,NULL);
                        // why this is there, it makes problem when moving a class
                        // it stays in fileTab and there is a clash!
                        // [2/8/2003] Maybe I should put it out
                        jslAddAllPackageClassesFromFileTab($2.data);
                    }
                    /* add java/lang package types */
                    javaMapDirectoryFiles2(s_javaLangName,
                            jslAddMapedImportTypeName, NULL, s_javaLangName, NULL);
                }
/*&fprintf(dumpOut," [jsl] current package == '%s'\n", pname);&*/
            }
        } ImportDeclarations_opt {
            if (regularPass()) {
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
            SetNullBoundariesFor($$);
        }
    |	ImportDeclarations					/* $$ = $1; */
    ;

ImportDeclarations
    :   SingleTypeImportDeclaration			{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
            if (inSecondJslPass()) {
                JslImportSingleDeclaration($1.data);
            }
        }
    |	TypeImportOnDemandDeclaration			{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
            if (inSecondJslPass()) {
                jslImportOnDemandDeclaration($1.data);
            }
        }
    |	ImportDeclarations SingleTypeImportDeclaration			{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
            if (inSecondJslPass()) {
                JslImportSingleDeclaration($2.data);
            }
        }
    |	ImportDeclarations TypeImportOnDemandDeclaration		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
            if (inSecondJslPass()) {
                jslImportOnDemandDeclaration($2.data);
            }
        }
    ;

/* this is original from JSL
ImportDeclarations
    :   ImportDeclaration
    |	ImportDeclarations ImportDeclaration		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
        }
    ;

ImportDeclaration
    :   SingleTypeImportDeclaration
    |	TypeImportOnDemandDeclaration
    ;
*/

SingleTypeImportDeclaration
    :   Import Name ';'						{
            $$.data = $2.data;
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    Reference *lastUselessRef;
                    Symbol *str;
                    // it was type or packege, but I thing this would be better
                    lastUselessRef = javaClassifyToTypeName($2.data, UsageUsed, &str, USELESS_FQT_REFS_DISALLOWED);
                    // last useless reference is not useless here!
                    if (lastUselessRef!=NULL) lastUselessRef->usage = NO_USAGE;
                    parsedInfo.lastImportLine = $1.data->position.line;
                    if ($2.data->next!=NULL) {
                        javaAddImportConstructionReference(&$2.data->next->id.position, &$1.data->position, UsageDefined);
                    }
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	Import CompletionFQTypeName ';'		{ /* rule never used */ }
    |	Import COMPL_IMPORT_SPECIAL ';'		{ /* rule never used */ }
    ;

TypeImportOnDemandDeclaration
    :   Import Name '.' '*' ';'				{
            $$.data = $2.data;
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    Symbol *str;
                    TypeModifier *expr;
                    Reference *rr, *lastUselessRef;
                    int st __attribute__((unused));
                    st = javaClassifyAmbiguousName($2.data, NULL,&str,&expr,&rr,
                                                   &lastUselessRef, USELESS_FQT_REFS_DISALLOWED,
                                                   CLASS_TO_TYPE,UsageUsed);
                    if (lastUselessRef!=NULL) lastUselessRef->usage = NO_USAGE;
                    parsedInfo.lastImportLine = $1.data->position.line;
                    javaAddImportConstructionReference(&$2.data->id.position, &$1.data->position, UsageDefined);
                } else {
                    PropagateBoundaries($$, $1, $5);
                }
            }
        }
    |	Import CompletionPackageName '.' '*' ';'	{ /* rule never used */ }
    ;

TypeDeclarations_opt:							{
            SetNullBoundariesFor($$);
        }
    |	TypeDeclarations _bef_					{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

TypeDeclarations
    :   _bef_ TypeDeclaration					{
            PropagateBoundariesIfRegularSyntaxPass($$, $2, $2);
        }
    |	TypeDeclarations _bef_ TypeDeclaration					{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
        }
    ;

PackageDeclaration_opt:							{
            $$.data = NULL;
            if (regularPass()) {
                parsedInfo.lastImportLine = 0;
                SetNullBoundariesFor($$);
            }
        }
    |	Package Name ';'						{
            $$.data = $2.data;
            if (regularPass()) {
                parsedInfo.lastImportLine = $1.data->position.line;
                PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
            }
        }
    |	Package error							{
            $$.data = NULL;
            if (regularPass()) {
                parsedInfo.lastImportLine = $1.data->position.line;
                PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
            }
        }
    |	Package CompletionPackageName ';'		{ /* rule never used */ }
    |	PACKAGE COMPL_THIS_PACKAGE_SPECIAL ';'		{ /* rule never used */ }
    ;

TypeDeclaration
    :   ClassDeclaration		{
            if (regularPass()) {
                javaSetClassSourceInformation(s_javaThisPackageName, $1.data);
                PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
            }
        }
    |	InterfaceDeclaration	{
            if (regularPass()) {
                javaSetClassSourceInformation(s_javaThisPackageName, $1.data);
                PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
            }
        }
    |	';'						{}
    |	error					{}
    ;

/* ************************ LALR special ************************** */

Modifiers_opt:					{
            $$.data = AccessDefault;
            SetNullBoundariesFor($$);
        }
    |	Modifiers				{
            $$.data = $1.data;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

Modifiers
    :   Modifier				/*& { $$ = $1; } &*/
    |	Modifiers Modifier		{
            $$.data = $1.data | $2.data;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
        }
    ;

Modifier
    :   PUBLIC			{ $$.data = AccessPublic; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	PROTECTED		{ $$.data = AccessProtected; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	PRIVATE			{ $$.data = AccessPrivate; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	STATIC			{ $$.data = AccessStatic; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	ABSTRACT		{ $$.data = AccessAbstract; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	FINAL			{ $$.data = AccessFinal; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	NATIVE			{ $$.data = AccessNative; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	SYNCHRONIZED	{ $$.data = AccessSynchronized; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	STRICTFP		{ $$.data = 0; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	TRANSIENT		{ $$.data = AccessTransient; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	VOLATILE		{ $$.data = 0; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    ;

/* *************************** Classes *************************** */

InCachingRecovery
    :   CACHING1_TOKEN ClassBody
    ;

/* **************** Class Declaration ****************** */

/* !!!!!!!! here is a problem if there are two in the same unit !!!!
TopLevelClassDeclaration
    :   Modifiers_opt CLASS Identifier
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        $<trail>$ = newClassDefinitionBegin($3.d, $1.d, NULL);
                    }
                }
            }
        Super_opt Interfaces_opt ClassBody		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $7);
        }
    |	Modifiers_opt CLASS error ClassBody		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $4);
        }
    ;
*/

ClassDeclaration
    :   Modifiers_opt CLASS Identifier {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        $<trail>$ = newClassDefinitionBegin($3.data, $1.data, NULL);
                    }
                } else {
                    jslNewClassDefinitionBegin($3.data, $1.data, NULL, CPOS_ST);
                    jslAddDefaultConstructor(s_jsl->classStat->thisClass);
                }
            } Super_opt Interfaces_opt {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        javaAddSuperNestedClassToSymbolTab(javaStat->thisClass);
                    }
                } else {
                    jslAddSuperNestedClassesToJslTypeTab(s_jsl->classStat->thisClass);
                }
            } ClassBody {
                if (regularPass()) {
                    $$.data = $3.data;
                    if (! SyntaxPassOnly()) {
                        newClassDefinitionEnd($<trail>4);
                    } else {
                        PropagateBoundaries($$, $1, $8);
                        if ($$.b.file == NO_FILE_NUMBER) PropagateBoundaries($$, $2, $$);
                        if (positionIsBetween($$.b, cxRefPosition, $$.e)
                            && parsedPositions[SPP_CLASS_DECLARATION_BEGIN_POSITION].file == NO_FILE_NUMBER) {
                            parsedPositions[SPP_CLASS_DECLARATION_BEGIN_POSITION] = $$.b;
                            parsedPositions[SPP_CLASS_DECLARATION_TYPE_BEGIN_POSITION] = $2.b;
                            parsedPositions[SPP_CLASS_DECLARATION_TYPE_END_POSITION] = $2.e;
                            parsedPositions[SPP_CLASS_DECLARATION_END_POSITION] = $$.e;
                        }
                    }
                } else {
                    jslNewClassDefinitionEnd();
                }
            }
    |	Modifiers_opt CLASS Identifier
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        $<trail>$ = newClassDefinitionBegin($3.data, $1.data, NULL);
                    }
                } else {
                    jslNewClassDefinitionBegin($3.data, $1.data, NULL, CPOS_ST);
                }
            }
        error ClassBody
            {
                if (regularPass()) {
                    $$.data = $3.data;
                    if (! SyntaxPassOnly()) {
                        newClassDefinitionEnd($<trail>4);
                    } else {
                        PropagateBoundaries($$, $1, $6);
                        if ($$.b.file == NO_FILE_NUMBER) PropagateBoundaries($$, $2, $6);
                    }
                } else {
                    jslNewClassDefinitionEnd();
                }
            }
    |	Modifiers_opt CLASS COMPL_CLASS_DEF_NAME	{ /* never used */ }
    ;


FunctionInnerClassDeclaration
    :   Modifiers_opt CLASS Identifier {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        $<trail>$ = newClassDefinitionBegin($3.data, $1.data, NULL);
                    }
                } else {
                    jslNewClassDefinitionBegin($3.data, $1.data, NULL, CPOS_FUNCTION_INNER);
                }
            } Super_opt Interfaces_opt {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        javaAddSuperNestedClassToSymbolTab(javaStat->thisClass);
                    }
                } else {
                    jslAddSuperNestedClassesToJslTypeTab(s_jsl->classStat->thisClass);
                }
            } ClassBody {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        newClassDefinitionEnd($<trail>4);
                    } else {
                        PropagateBoundaries($$, $1, $8);
                        if ($$.b.file == NO_FILE_NUMBER) PropagateBoundaries($$, $2, $8);
                        if (positionsAreEqual(cxRefPosition, $3.data->position)) {
                            parsedPositions[SPP_CLASS_DECLARATION_BEGIN_POSITION] = $$.b;
                            parsedPositions[SPP_CLASS_DECLARATION_END_POSITION] = $$.e;
                        }
                    }
                } else {
                    jslNewClassDefinitionEnd();
                }
            }
    |	Modifiers_opt CLASS Identifier
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        $<trail>$ = newClassDefinitionBegin($3.data, $1.data, NULL);
                    }
                } else {
                    jslNewClassDefinitionBegin($3.data, $1.data, NULL, CPOS_FUNCTION_INNER);
                }
            }
        error ClassBody
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        newClassDefinitionEnd($<trail>4);
                    } else {
                        PropagateBoundaries($$, $1, $6);
                        if ($$.b.file == NO_FILE_NUMBER) PropagateBoundaries($$, $2, $6);
                    }
                } else {
                    jslNewClassDefinitionEnd();
                }
            }
    |	Modifiers_opt CLASS COMPL_CLASS_DEF_NAME	{ /* never used */ }
    ;



Super_opt
    :   {
            if (inSecondJslPass()) {
                if (strcmp(s_jsl->classStat->thisClass->linkName,
                            s_javaLangObjectLinkName)!=0) {
                    // add to any except java/lang/Object itself
                    jslAddSuperClassOrInterfaceByName(
                        s_jsl->classStat->thisClass, s_javaLangObjectLinkName);
                }
            }
            SetNullBoundariesFor($$);
        }
    |	EXTENDS ExtendClassOrInterfaceType			{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    assert($2.data && $2.data->type == TypeDefault && $2.data->u.typeModifier);
                    assert($2.data->u.typeModifier->kind == TypeStruct);
                    javaParsedSuperClass($2.data->u.typeModifier->u.t);
                } else {
                    PropagateBoundaries($$, $1, $2);
                }
            }
            if (inSecondJslPass()) {
                assert($2.data && $2.data->type == TypeDefault && $2.data->u.typeModifier);
                assert($2.data->u.typeModifier->kind == TypeStruct);
                jslAddSuperClassOrInterface(s_jsl->classStat->thisClass,
                                            $2.data->u.typeModifier->u.t);
            }
        }
    ;

Interfaces_opt:								{
            SetNullBoundariesFor($$);
        }
    |	IMPLEMENTS InterfaceTypeList		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
        }
    ;

InterfaceTypeList
    :   InterfaceType							{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    assert($1.data && $1.data->type == TypeDefault && $1.data->u.typeModifier);
                    assert($1.data->u.typeModifier->kind == TypeStruct);
                    javaParsedSuperClass($1.data->u.typeModifier->u.t);
                } else {
                    PropagateBoundaries($$, $1, $1);
                }
            }
            if (inSecondJslPass()) {
                assert($1.data && $1.data->type == TypeDefault && $1.data->u.typeModifier);
                assert($1.data->u.typeModifier->kind == TypeStruct);
                jslAddSuperClassOrInterface(s_jsl->classStat->thisClass,
                                            $1.data->u.typeModifier->u.t);
            }
        }
    |	InterfaceTypeList ',' InterfaceType		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    assert($3.data && $3.data->type == TypeDefault && $3.data->u.typeModifier);
                    assert($3.data->u.typeModifier->kind == TypeStruct);
                    javaParsedSuperClass($3.data->u.typeModifier->u.t);
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            }
            if (inSecondJslPass()) {
                assert($3.data && $3.data->type == TypeDefault && $3.data->u.typeModifier);
                assert($3.data->u.typeModifier->kind == TypeStruct);
                jslAddSuperClassOrInterface(s_jsl->classStat->thisClass,
                                            $3.data->u.typeModifier->u.t);
            }
        }
    ;

_bef_:	{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    // TODO, REDO all this stuff around class/method boundaries !!!!!!
                    if (options.mode == ServerMode) {
                        if (parsedClassInfo.parserPassedMarker && !parsedClassInfo.thisMethodMemoriesStored){
                            parsedInfo.cxMemoryIndexAtMethodBegin = parsedClassInfo.cxMemoryIndexAtFunctionBegin;
                            parsedInfo.cxMemoryIndexAtMethodEnd = cxMemory->index;
                                 /*& sprintf(tmpBuff,"setting %s, %d,%d   %d,%d",
                                     olcxOptionsName[options.serverOperation],
                                     parsedClassInfo.parserPassedMarker, parsedClassInfo.thisMethodMemoriesStored,
                                     parsedInfo.cxMemoryIndexAtMethodBegin, parsedInfo.cxMemoryIndexAtMethodEnd),
                                     ppcBottomInformation(tmpBuff); &*/
                            parsedClassInfo.thisMethodMemoriesStored = 1;
                            if (options.serverOperation == OLO_MAYBE_THIS) {
                                changeMethodReferencesUsages(LINK_NAME_MAYBE_THIS_ITEM,
                                                             CategoryLocal, currentFile.characterBuffer.fileNumber,
                                                             javaStat->thisClass);
                            } else if (options.serverOperation == OLO_NOT_FQT_REFS) {
                                changeMethodReferencesUsages(LINK_NAME_NOT_FQT_ITEM,
                                                             CategoryLocal,currentFile.characterBuffer.fileNumber,
                                                             javaStat->thisClass);
                            } else if (options.serverOperation == OLO_USELESS_LONG_NAME) {
                                changeMethodReferencesUsages(LINK_NAME_IMPORTED_QUALIFIED_ITEM,
                                                             CategoryGlobal,currentFile.characterBuffer.fileNumber,
                                                             javaStat->thisClass);
                            }
                            parsedInfo.cxMemoryIndexAtClassBeginning = parsedClassInfo.cxMemoryIndexdiAtClassBegin;
                            parsedInfo.cxMemoryIndexAtClassEnd = cxMemory->index;
                            parsedInfo.classCoordEndLine = currentFile.lineNumber+1;
                            /*& fprintf(dumpOut,"!setting class end line to %d, cb==%d, ce==%d\n",
                              parsedInfo.classCoordEndLine, parsedInfo.cxMemoryIndexAtClassBeginning,
                              parsedInfo.cxMemoryIndexAtClassEnd); &*/
                            if (options.serverOperation == OLO_NOT_FQT_REFS_IN_CLASS) {
                                changeClassReferencesUsages(LINK_NAME_NOT_FQT_ITEM,
                                                            CategoryLocal,currentFile.characterBuffer.fileNumber,
                                                            javaStat->thisClass);
                            } else if (options.serverOperation == OLO_USELESS_LONG_NAME_IN_CLASS) {
                                changeClassReferencesUsages(LINK_NAME_IMPORTED_QUALIFIED_ITEM,
                                                            CategoryGlobal,currentFile.characterBuffer.fileNumber,
                                                            javaStat->thisClass);
                            }
                        }
                    }
                    parsedClassInfo.cxMemoryIndexdiAtClassBegin = cxMemory->index;
                    /*& fprintf(dumpOut,"!setting class begin memory %d\n",
                      parsedClassInfo.cxMemoryIndexdiAtClassBegin); &*/
                    actionsBeforeAfterExternalDefinition();
                }
            }
        }
    ;

ClassBody
    :   _bef_ '{' '}'								{
            PropagateBoundariesIfRegularSyntaxPass($$, $2, $3);
        }
    |	_bef_ '{' ClassBodyDeclarations _bef_ '}'   {
            PropagateBoundariesIfRegularSyntaxPass($$, $2, $5);
        }
    ;

ClassBodyDeclarations
    :   ClassBodyDeclaration                                    {
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |	ClassBodyDeclarations _bef_ ClassBodyDeclaration		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
        }
    ;

ClassBodyDeclaration
    :   ClassMemberDeclaration
    |	ClassInitializer
    |	ConstructorDeclaration
    |	';'
    |	error							{SetNullBoundariesFor($$);}
    ;

ClassMemberDeclaration
    :   ClassDeclaration				{PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	InterfaceDeclaration			{PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	FieldDeclaration				{PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	MethodDeclaration				{PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    ;

/* ****************** Field Declarations ****************** */

AssignmentType
    :   JavaType					{
            $$.data = $1.data;
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    parsedInfo.lastAssignmentStruct = $1.data;
                } else {
                    PropagateBoundaries($$, $1, $1);
                }
            }
    }
    ;

FieldDeclaration
    :   Modifiers_opt AssignmentType VariableDeclarators ';'		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    Symbol *p,*pp,*memb,*clas;
                    int vClass;
                    S_recFindStr    rfs;
                    parsedInfo.lastAssignmentStruct = NULL;
                    clas = javaStat->thisClass;
                    assert(clas != NULL);
                    for(p=$3.data; p!=NULL; p=pp) {
                        pp = p->next;
                        p->next = NULL;
                        if (p->type == TypeError) continue;
                        assert(p->type == TypeDefault);
                        completeDeclarator($2.data, p);
                        vClass = javaStat->classFileNumber;
                        p->access = $1.data;
                        p->storage = StorageField;
                        if (clas->access&AccessInterface) {
                            // set interface default access flags
                            p->access |= (AccessPublic | AccessStatic | AccessFinal);
                        }
                        /*& javaSetFieldLinkName(p); &*/
                        iniFind(clas, &rfs);
                        if (findStrRecordSym(&rfs, p->name, &memb, CLASS_TO_ANY,
                                             ACCESSIBILITY_CHECK_NO,VISIBILITY_CHECK_NO) == RESULT_NOT_FOUND) {
                            assert(clas->u.structSpec);
                            LIST_APPEND(Symbol, clas->u.structSpec->records, p);
                        }
                        addCxReference(p, &p->pos, UsageDefined, vClass, vClass);
                    }
                    $$.data = $3.data;
                    if (options.mode == ServerMode
                        && parsedClassInfo.parserPassedMarker
                        && !parsedClassInfo.thisMethodMemoriesStored){
                        parsedInfo.methodCoordEndLine = currentFile.lineNumber+1;
                    }
                } else {
                    PropagateBoundaries($$, $1, $4);
                    if ($$.b.file == NO_FILE_NUMBER) PropagateBoundaries($$, $2, $4);
                    if (positionIsBetween($$.b, cxRefPosition, $$.e)
                        && parsedPositions[SPP_FIELD_DECLARATION_BEGIN_POSITION].file==NO_FILE_NUMBER) {
                        parsedPositions[SPP_FIELD_DECLARATION_BEGIN_POSITION] = $$.b;
                        parsedPositions[SPP_FIELD_DECLARATION_TYPE_BEGIN_POSITION] = $2.b;
                        parsedPositions[SPP_FIELD_DECLARATION_TYPE_END_POSITION] = $2.e;
                        parsedPositions[SPP_FIELD_DECLARATION_END_POSITION] = $$.e;
                    }
                }
            }
            if (inSecondJslPass()) {
                Symbol *p;
                Symbol *pp;
                Symbol *clas;
                int		vClass;
                clas = s_jsl->classStat->thisClass;
                assert(clas != NULL);
                for(p=$3.data; p!=NULL; p=pp) {
                    pp = p->next;
                    p->next = NULL;
                    if (p->type == TypeError) continue;
                    assert(p->type == TypeDefault);
                    assert(clas->u.structSpec);
                    vClass = clas->u.structSpec->classFileNumber;
                    jslCompleteDeclarator($2.data, p);
                    p->access = $1.data;
                    p->storage = StorageField;
                    if (clas->access&AccessInterface) {
                        // set interface default access flags
                        p->access |= (AccessPublic|AccessStatic|AccessFinal);
                    }
                    log_debug("[jsl] adding field %s to %s", p->name,clas->linkName);
                    LIST_APPEND(Symbol, clas->u.structSpec->records, p);
                    assert(vClass!=NO_FILE_NUMBER);
                    if (p->pos.file!=olOriginalFileNumber && options.serverOperation==OLO_PUSH) {
                        // pre load of saved file akes problem on move field/method, ...
                        addCxReference(p, &p->pos, UsageDefined, vClass, vClass);
                    }
                }
                $$.data = $3.data;
            }
        }
    ;

VariableDeclarators
    :   VariableDeclarator								{
            $$.data = $1.data;
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    assert($$.data->type == TypeDefault || $$.data->type == TypeError);
                } else {
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	VariableDeclarators ',' VariableDeclarator		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    assert($1.data && $3.data);
                    if ($3.data->storage == StorageError) {
                        $$.data = $1.data;
                    } else {
                        $$.data = $3.data;
                        $$.data->next = $1.data;
                    }
                    assert($$.data->type == TypeDefault || $$.data->type == TypeError);
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            }
            if (inSecondJslPass()) {
                assert($1.data && $3.data);
                if ($3.data->storage == StorageError) {
                    $$.data = $1.data;
                } else {
                    $$.data = $3.data;
                    $$.data->next = $1.data;
                }
                assert($$.data->type==TypeDefault || $$.data->type==TypeError);
            }
        }
    ;

VariableDeclarator
    :   VariableDeclaratorId							/*	{ $$ = $1; } */
    |	VariableDeclaratorId '=' VariableInitializer	{
            $$.data = $1.data;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
        }
    |	error											{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data = newSymbolAsCopyOf(&errorSymbol);
                } else {
                    SetNullBoundariesFor($$);
                }
            }
            if (inSecondJslPass()) {
                CF_ALLOC($$.data, Symbol);
                *$$.data = errorSymbol;
            }
        }
    ;

VariableDeclaratorId
    :   Identifier							{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data = newSymbol($1.data->name, $1.data->name, $1.data->position);
                } else {
                    PropagateBoundaries($$, $1, $1);
                }
            }
            if (inSecondJslPass()) {
                char *name;
                CF_ALLOCC(name, strlen($1.data->name)+1, char);
                strcpy(name, $1.data->name);
                CF_ALLOC($$.data, Symbol);
                fillSymbol($$.data, name, name, $1.data->position);
            }
        }
    |	VariableDeclaratorId '[' ']'		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    assert($1.data);
                    $$.data = $1.data;
                    addComposedTypeToSymbol($$.data, TypeArray);
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            }
            if (inSecondJslPass()) {
                assert($1.data);
                $$.data = $1.data;
                JslAddComposedType($$.data, TypeArray);
            }
        }
    |	COMPL_VARIABLE_NAME_HINT {/* rule never used */}
    ;

VariableInitializer
    :   Expression
    |	ArrayInitializer
    ;

/* **************** Method Declarations **************** */

MethodDeclaration
    :   MethodHeader Start_block
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        javaMethodBodyBeginning($1.data);
                    }
                }
            }
        MethodBody End_block
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        javaMethodBodyEnding(&$4.data);
                    } else {
                        PropagateBoundaries($$, $1, $4);
                        if (positionIsBetween($1.b, cxRefPosition, $1.e)) {
                            parsedPositions[SPP_METHOD_DECLARATION_BEGIN_POSITION] = $$.b;
                            parsedPositions[SPP_METHOD_DECLARATION_END_POSITION] = $$.e;
                        }
                    }
                }
            }
    ;

MethodHeader
    :   Modifiers_opt AssignmentType MethodDeclarator Throws_opt	{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    parsedInfo.lastAssignmentStruct = NULL;
                    $$.data = javaMethodHeader($1.data,$2.data,$3.data, StorageMethod);
                } else {
                    PropagateBoundaries($$, $1, $4);
                    if ($$.b.file == NO_FILE_NUMBER) PropagateBoundaries($$, $2, $$);
                    if ($$.e.file == NO_FILE_NUMBER) PropagateBoundaries($$, $$, $3);
                    if (positionIsBetween($$.b, cxRefPosition, $3.e)) {
                        parsedPositions[SPP_METHOD_DECLARATION_TYPE_BEGIN_POSITION] = $2.b;
                        parsedPositions[SPP_METHOD_DECLARATION_TYPE_END_POSITION] = $2.e;
                    }
                }
            }
            if (inSecondJslPass()) {
                $$.data = jslMethodHeader($1.data,$2.data,$3.data,StorageMethod, $4.data);
            }
        }
    |	Modifiers_opt VOID MethodDeclarator Throws_opt	{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data = javaMethodHeader($1.data,&defaultVoidDefinition,$3.data,StorageMethod);
                } else {
                    PropagateBoundaries($$, $1, $4);
                    if ($$.b.file == NO_FILE_NUMBER) PropagateBoundaries($$, $2, $$);
                    if ($$.e.file == NO_FILE_NUMBER) PropagateBoundaries($$, $$, $3);
                    if (positionIsBetween($$.b, cxRefPosition, $3.e)) {
                        parsedPositions[SPP_METHOD_DECLARATION_TYPE_BEGIN_POSITION] = $2.b;
                        parsedPositions[SPP_METHOD_DECLARATION_TYPE_END_POSITION] = $2.e;
                    }
                }
            }
            if (inSecondJslPass()) {
                $$.data = jslMethodHeader($1.data,&defaultVoidDefinition,$3.data,StorageMethod, $4.data);
            }
        }
    |	COMPL_FULL_INHERITED_HEADER		{assert(0);}
    ;

MethodDeclarator
    :   Identifier
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        $<symbol>$ = javaCreateNewMethod($1.data->name, &($1.data->position), MEMORY_XX);
                    }
                }
                if (inSecondJslPass()) {
                    $<symbol>$ = javaCreateNewMethod($1.data->name,&($1.data->position), MEMORY_CF);
                }
            }
        '(' FormalParameterList_opt ')'
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        $$.data = $<symbol>2;
                        assert($$.data && $$.data->u.typeModifier && $$.data->u.typeModifier->kind == TypeFunction);
                        initFunctionTypeModifier(&$$.data->u.typeModifier->u.f , $4.data.symbol);
                    } else {
                        javaHandleDeclaratorParamPositions(&$1.data->position, &$3.data, $4.data.positionList, &$5.data);
                        PropagateBoundaries($$, $1, $5);
                    }
                }
                if (inSecondJslPass()) {
                    $$.data = $<symbol>2;
                    assert($$.data && $$.data->u.typeModifier && $$.data->u.typeModifier->kind == TypeFunction);
                    initFunctionTypeModifier(&$$.data->u.typeModifier->u.f , $4.data.symbol);
                }
            }
    |	MethodDeclarator '[' ']'						{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data = $1.data;
                    addComposedTypeToSymbol($$.data, TypeArray);
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            }
            if (inSecondJslPass()) {
                $$.data = $1.data;
                JslAddComposedType($$.data, TypeArray);
            }
        }
    |	COMPL_METHOD_NAME0					{ assert(0);}
    ;

FormalParameterList_opt:					{
            $$.data.symbol = NULL;
            $$.data.positionList = NULL;
            SetNullBoundariesFor($$);
        }
    |	FormalParameterList					/*& {$$ = $1;} &*/
    ;

FormalParameterList
    :   FormalParameter								{
            if (! SyntaxPassOnly()) {
                $$.data.symbol = $1.data;
            } else {
                $$.data.positionList = NULL;
                appendPositionToList(&$$.data.positionList, &noPosition);
                PropagateBoundaries($$, $1, $1);
            }
        }
    |	FormalParameterList ',' FormalParameter		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data = $1.data;
                    LIST_APPEND(Symbol, $$.data.symbol, $3.data);
                } else {
                    appendPositionToList(&$$.data.positionList, &$2.data);
                    PropagateBoundaries($$, $1, $3);
                }
            }
            if (inSecondJslPass()) {
                $$.data = $1.data;
                LIST_APPEND(Symbol, $$.data.symbol, $3.data);
            }
        }
    ;

FormalParameter
    :   JavaType VariableDeclaratorId			{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data = $2.data;
                    completeDeclarator($1.data, $2.data);
                } else {
                    PropagateBoundaries($$, $1, $2);
                }
            }
            if (inSecondJslPass()) {
                $$.data = $2.data;
                completeDeclarator($1.data, $2.data);
            }
        }
    |	FINAL JavaType VariableDeclaratorId		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data = $3.data;
                    completeDeclarator($2.data, $3.data);
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            }
            if (inSecondJslPass()) {
                $$.data = $3.data;
                completeDeclarator($2.data, $3.data);
            }
        }
    |	error								{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data = newSymbolAsCopyOf(&errorSymbol);
                } else {
                    SetNullBoundariesFor($$);
                }
            }
            if (inSecondJslPass()) {
                CF_ALLOC($$.data, Symbol);
                *$$.data = errorSymbol;
            }
        }
    ;

Throws_opt:								{
            $$.data = NULL;
            SetNullBoundariesFor($$);
        }
    |	THROWS ClassTypeList			{
            $$.data = $2.data;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
        }
    ;

ClassTypeList
    :   ClassType						{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
            if (inSecondJslPass()) {
                assert($1.data && $1.data->type == TypeDefault && $1.data->u.typeModifier);
                assert($1.data->u.typeModifier->kind == TypeStruct);
                CF_ALLOC($$.data, SymbolList);
                /* REPLACED: FILL_symbolList($$.d, $1.d->u.type->u.t, NULL); with compound literal */
                *$$.data = (SymbolList){.element = $1.data->u.typeModifier->u.t, .next = NULL};
            }
        }
    |	ClassTypeList ',' ClassType		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
            if (inSecondJslPass()) {
                assert($3.data && $3.data->type == TypeDefault && $3.data->u.typeModifier);
                assert($3.data->u.typeModifier->kind == TypeStruct);
                CF_ALLOC($$.data, SymbolList);
                /* REPLACED: FILL_symbolList($$.d, $3.d->u.type->u.t, $1.d); with compound literal */
                *$$.data = (SymbolList){.element = $3.data->u.typeModifier->u.t, .next = $1.data};
            }
        }
    ;

MethodBody
    :   Block				/*& { $$ = $1; } &*/
    |	';'					{
            $$.data = $1.data;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

/* ************** Static Initializers ************** */

ClassInitializer
    :   STATIC Block		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
        }
    |	Block				{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

ConstructorDeclaration
    :   Modifiers_opt ConstructorDeclarator Throws_opt
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        Symbol *mh, *args;

                        args = $2.data;
                        /*&
                          if (! ($1.data & AccessStatic)) {
                              args = javaPrependDirectEnclosingInstanceArgument($2.data);
                          }
                          &*/
                        mh=javaMethodHeader($1.data, &errorSymbol, args, StorageConstructor);
                        // TODO! Merge this with 'javaMethodBodyBeginning'!
                        assert(mh->u.typeModifier && mh->u.typeModifier->kind == TypeFunction);
                        beginBlock();  // in order to remove arguments
                        parsedClassInfo.function = mh; /* added for set-target-position checks */
                        /* also needed for pushing label reference */
                        generateInternalLabelReference(-1, UsageDefined);
                        counters.localVar = 0;
                        assert($2.data && $2.data->u.typeModifier);
                        javaAddMethodParametersToSymTable($2.data);
                        mh->u.typeModifier->u.m.signature = strchr(mh->linkName, '(');
                        javaStat->methodModifiers = $1.data;
                    }
                }
                if (inSecondJslPass()) {
                    Symbol *args;
                    args = $2.data;
                    jslMethodHeader($1.data,&defaultVoidDefinition,args,
                                    StorageConstructor, $3.data);
                }
            }
         Start_block ConstructorBody End_block {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    endBlock();
                    PropagateBoundaries($$, $1, $6);
                    if ($$.b.file == NO_FILE_NUMBER)
                        PropagateBoundaries($$, $2, $$);
                }
                parsedClassInfo.function = NULL; /* added for set-target-position checks */
            }
        }
    ;

ConstructorDeclarator
    :   Identifier
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        if (strcmp($1.data->name, javaStat->thisClass->name)==0) {
                            addCxReference(javaStat->thisClass, &$1.data->position,
                                           UsageConstructorDefinition,NO_FILE_NUMBER, NO_FILE_NUMBER);
                            $<symbol>$ = javaCreateNewMethod($1.data->name,//JAVA_CONSTRUCTOR_NAME1,
                                                             &($1.data->position), MEMORY_XX);
                        } else {
                            // a type forgotten for a method?
                            $<symbol>$ = javaCreateNewMethod($1.data->name,&($1.data->position),MEMORY_XX);
                        }
                    }
                }
                if (inSecondJslPass()) {
                    if (strcmp($1.data->name, s_jsl->classStat->thisClass->name)==0) {
                        $<symbol>$ = javaCreateNewMethod(
                                        $1.data->name, //JAVA_CONSTRUCTOR_NAME1,
                                        &($1.data->position),
                                        MEMORY_CF);
                    } else {
                        // a type forgotten for a method?
                        $<symbol>$ = javaCreateNewMethod($1.data->name, &($1.data->position), MEMORY_CF);
                    }
                }
            }
        '(' FormalParameterList_opt ')'
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        $$.data = $<symbol>2;
                        assert($$.data && $$.data->u.typeModifier && $$.data->u.typeModifier->kind == TypeFunction);
                        initFunctionTypeModifier(&$$.data->u.typeModifier->u.f , $4.data.symbol);
                    } else {
                        javaHandleDeclaratorParamPositions(&$1.data->position, &$3.data, $4.data.positionList, &$5.data);
                        PropagateBoundaries($$, $1, $5);
                    }
                }
                if (inSecondJslPass()) {
                    $$.data = $<symbol>2;
                    assert($$.data && $$.data->u.typeModifier && $$.data->u.typeModifier->kind == TypeFunction);
                    initFunctionTypeModifier(&$$.data->u.typeModifier->u.f , $4.data.symbol);
                };
            }
    ;

ConstructorBody
    :   '{' Start_block ExplicitConstructorInvocation BlockStatements End_block '}'	{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $6);
        }
    |	'{' Start_block ExplicitConstructorInvocation End_block '}'					{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $5);
        }
    |	'{' Start_block BlockStatements End_block '}'									{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $5);
        }
    |	'{' '}'																			{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
        }
    ;

/* TO FINISH the constructors signatures */
ExplicitConstructorInvocation
    :   This _erfs_
            {
                if (ComputingPossibleParameterCompletion()) {
                    parsedClassInfo.erfsForParameterCompletion = javaCrErfsForConstructorInvocation(javaStat->thisClass, &$1.data->position);
                }
            } '(' ArgumentList_opt ')'			{
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        javaConstructorInvocation(javaStat->thisClass, &($1.data->position), $5.data.typeModifierList);
                        parsedClassInfo.erfsForParameterCompletion = $2;
                    } else {
                        javaHandleDeclaratorParamPositions(&$1.data->position, &$4.data, $5.data.positionList, &$6.data);
                        PropagateBoundaries($$, $1, $6);
                    }
                }
            }
    |	Super _erfs_
            {
                if (ComputingPossibleParameterCompletion()) {
                    parsedClassInfo.erfsForParameterCompletion = javaCrErfsForConstructorInvocation(javaCurrentSuperClass(), &$1.data->position);
                }
            }   '(' ArgumentList_opt ')'			{
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        Symbol *ss;
                        ss = javaCurrentSuperClass();
                        javaConstructorInvocation(ss, &($1.data->position), $5.data.typeModifierList);
                        parsedClassInfo.erfsForParameterCompletion = $2;
                    } else {
                        javaHandleDeclaratorParamPositions(&$1.data->position, &$4.data, $5.data.positionList, &$6.data);
                        PropagateBoundaries($$, $1, $6);
                    }
                }
            }
    |	Primary  '.' Super _erfs_
            {
                if (ComputingPossibleParameterCompletion()) {
                    parsedClassInfo.erfsForParameterCompletion = javaCrErfsForConstructorInvocation(javaCurrentSuperClass(), &($3.data->position));
                }
            } '(' ArgumentList_opt ')'		{
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        Symbol *ss;
                        ss = javaCurrentSuperClass();
                        javaConstructorInvocation(ss, &($3.data->position), $7.data.typeModifierList);
                        parsedClassInfo.erfsForParameterCompletion = $4;
                    } else {
                        javaHandleDeclaratorParamPositions(&$3.data->position, &$6.data, $7.data.positionList, &$8.data);
                        PropagateBoundaries($$, $1, $8);
                    }
                }
            }
    |	This error					{SetNullBoundariesFor($$);}
    |	Super error					{SetNullBoundariesFor($$);}
    |	Primary error				{SetNullBoundariesFor($$);}
    |	COMPL_SUPER_CONSTRUCTOR1						{assert(0);}
    |	COMPL_THIS_CONSTRUCTOR							{assert(0);}
    |	Primary '.' COMPL_SUPER_CONSTRUCTOR1			{assert(0);}
    ;

/* ******************************* Interfaces ******************** */
/* ************************ Interface Declarations *************** */

InterfaceDeclaration
    :   Modifiers_opt INTERFACE Identifier          {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $<trail>$=newClassDefinitionBegin($3.data,($1.data|AccessInterface),NULL);
                }
            } else {
                jslNewClassDefinitionBegin($3.data, ($1.data|AccessInterface), NULL, CPOS_ST);
            }
        } ExtendsInterfaces_opt {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    javaAddSuperNestedClassToSymbolTab(javaStat->thisClass);
                }
            } else {
                jslAddSuperNestedClassesToJslTypeTab(s_jsl->classStat->thisClass);
            }
        } InterfaceBody {
            if (regularPass()) {
                $$.data = $3.data;
                if (! SyntaxPassOnly()) {
                    newClassDefinitionEnd($<trail>4);
                } else {
                    PropagateBoundaries($$, $1, $7);
                    if ($$.b.file == NO_FILE_NUMBER) PropagateBoundaries($$, $2, $$);
                    if (positionIsBetween($$.b, cxRefPosition, $$.e)
                        && parsedPositions[SPP_CLASS_DECLARATION_BEGIN_POSITION].file == NO_FILE_NUMBER) {
                        parsedPositions[SPP_CLASS_DECLARATION_BEGIN_POSITION] = $$.b;
                        parsedPositions[SPP_CLASS_DECLARATION_TYPE_BEGIN_POSITION] = $2.b;
                        parsedPositions[SPP_CLASS_DECLARATION_TYPE_END_POSITION] = $2.e;
                        parsedPositions[SPP_CLASS_DECLARATION_END_POSITION] = $$.e;
                    }
                }
            } else {
                jslNewClassDefinitionEnd();
            }
        }
    |	Modifiers_opt INTERFACE Identifier
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        $<trail>$=newClassDefinitionBegin($3.data,($1.data|AccessInterface),NULL);
                    }
                } else {
                    jslNewClassDefinitionBegin($3.data, ($1.data|AccessInterface), NULL, CPOS_ST);
                }
            }
        error InterfaceBody
            {
                if (regularPass()) {
                    $$.data = $3.data;
                    if (! SyntaxPassOnly()) {
                        newClassDefinitionEnd($<trail>4);
                    } else {
                        PropagateBoundaries($$, $1, $6);
                        if ($$.b.file == NO_FILE_NUMBER) PropagateBoundaries($$, $2, $$);
                    }
                } else {
                    jslNewClassDefinitionEnd();
                }
            }
    |	Modifiers_opt INTERFACE COMPL_CLASS_DEF_NAME	{ /* never used */ }
    ;

ExtendsInterfaces_opt:					{
            SetNullBoundariesFor($$);
            if (inSecondJslPass()) {
                jslAddSuperClassOrInterfaceByName(s_jsl->classStat->thisClass,
                                                s_javaLangObjectLinkName);
            }
        }
    |	ExtendsInterfaces
    ;

ExtendsInterfaces
    :   EXTENDS ExtendClassOrInterfaceType		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    assert($2.data && $2.data->type == TypeDefault && $2.data->u.typeModifier);
                    assert($2.data->u.typeModifier->kind == TypeStruct);
                    javaParsedSuperClass($2.data->u.typeModifier->u.t);
                } else {
                    PropagateBoundaries($$, $1, $2);
                }
            }
            if (inSecondJslPass()) {
                assert($2.data && $2.data->type == TypeDefault && $2.data->u.typeModifier);
                assert($2.data->u.typeModifier->kind == TypeStruct);
                jslAddSuperClassOrInterface(s_jsl->classStat->thisClass,
                                            $2.data->u.typeModifier->u.t);
            }
        }
    |	ExtendsInterfaces ',' ExtendClassOrInterfaceType        {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    assert($3.data && $3.data->type == TypeDefault && $3.data->u.typeModifier);
                    assert($3.data->u.typeModifier->kind == TypeStruct);
                    javaParsedSuperClass($3.data->u.typeModifier->u.t);
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            }
            if (inSecondJslPass()) {
                assert($3.data && $3.data->type == TypeDefault && $3.data->u.typeModifier);
                assert($3.data->u.typeModifier->kind == TypeStruct);
                jslAddSuperClassOrInterface(s_jsl->classStat->thisClass,
                                            $3.data->u.typeModifier->u.t);
            }
        }
    ;

InterfaceBody
    :   _bef_ '{' '}'											{
            PropagateBoundariesIfRegularSyntaxPass($$, $2, $3);
        }
    |	_bef_ '{' InterfaceMemberDeclarations _bef_ '}'			{
            PropagateBoundariesIfRegularSyntaxPass($$, $2, $5);
        }
    ;

InterfaceMemberDeclarations
    :   InterfaceMemberDeclaration
    |	InterfaceMemberDeclarations _bef_ InterfaceMemberDeclaration	{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
        }
    ;

InterfaceMemberDeclaration
    :   ClassDeclaration				{PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	InterfaceDeclaration			{PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	ConstantDeclaration				{PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	AbstractMethodDeclaration		{PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	';'								{PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	error							{SetNullBoundariesFor($$);}
    ;

ConstantDeclaration
    :   FieldDeclaration				/*& {$$=$1;} &*/
    ;

AbstractMethodDeclaration
    :   MethodHeader Start_block
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        javaMethodBodyBeginning($1.data);
                    }
                }
            }
        ';' End_block
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        javaMethodBodyEnding(&$4.data);
                    } else {
                        PropagateBoundaries($$, $1, $4);
                    }
                }
            }
    ;

/* ***************************** Arrays *************************** */

ArrayInitializer
    :   '{' VariableInitializers ',' '}'		{PropagateBoundariesIfRegularSyntaxPass($$, $1, $4);}
    |	'{' VariableInitializers     '}'		{PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);}
    |	'{' ',' '}'								{PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);}
    |	'{' '}'									{PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);}
    ;

VariableInitializers
    :   VariableInitializer							{
        PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |	VariableInitializers ',' VariableInitializer		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
        }
    ;

/* *********************** Blocks and Statements ***************** */

Block
    :   '{' Start_block BlockStatements End_block '}'		{
            $$.data = $5.data;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $5);
        }
    |	'{' '}'												{
            $$.data = $2.data;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
        }
    ;

BlockStatements
    :   BlockStatement						/*& {$$ = $1;} &*/
    |	BlockStatements BlockStatement		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
        }
    ;

BlockStatement
    :   LocalVariableDeclarationStatement		/*& {$$ = $1;} &*/
    |	FunctionInnerClassDeclaration			/*& {$$ = $1;} &*/
    |	Statement								/*& {$$ = $1;} &*/
    |	error									{SetNullBoundariesFor($$);}
    ;

LocalVariableDeclarationStatement
    :   LocalVariableDeclaration ';'			{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
        }
    ;

LocalVarDeclUntilInit
    :   JavaType VariableDeclaratorId							{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    addNewDeclaration(javaStat->locals, $1.data,$2.data,NULL,StorageAuto);
                    $$.data = $1.data;
                } else {
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    |	FINAL JavaType VariableDeclaratorId						{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    addNewDeclaration(javaStat->locals, $2.data,$3.data,NULL,StorageAuto);
                    $$.data = $2.data;
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	LocalVariableDeclaration ',' VariableDeclaratorId	{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    if ($1.data->type != TypeError) {
                        addNewDeclaration(javaStat->locals, $1.data,$3.data,NULL,StorageAuto);
                    }
                    $$.data = $1.data;
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    ;

LocalVariableDeclaration
    :   LocalVarDeclUntilInit								{
            if (regularPass()) $$.data = $1.data;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |	LocalVarDeclUntilInit {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    parsedInfo.lastAssignmentStruct = $1.data;
                }
            }
        } '=' VariableInitializer		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    parsedInfo.lastAssignmentStruct = NULL;
                    $$.data = $1.data;
                } else {
                    PropagateBoundaries($$, $1, $4);
                }
            }
        }
    /*
    |   error			{
            $$.d = &s_errorSymbol;
            SetNullBoundaries($$);
        }
    */
    ;

Statement
    :   StatementWithoutTrailingSubstatement
    |	LabeledStatement
    |	IfThenStatement
    |	IfThenElseStatement
    |	WhileStatement
    |	ForStatement
    ;

StatementNoShortIf
    :   StatementWithoutTrailingSubstatement
    |	LabeledStatementNoShortIf
    |	IfThenElseStatementNoShortIf
    |	WhileStatementNoShortIf
    |	ForStatementNoShortIf
    ;

StatementWithoutTrailingSubstatement
    :   Block
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

AssertStatement
    :   ASSERT Expression ';'						{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
        }
    |	ASSERT Expression ':' Expression ';'		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $5);
        }
    ;

EmptyStatement
    :   ';'										{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

LabeledStatement
    :   LabelDefininigIdentifier ':' Statement		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
        }
    ;

LabeledStatementNoShortIf
    :   LabelDefininigIdentifier ':' StatementNoShortIf         {
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
        }
    ;

ExpressionStatement
    :   StatementExpression ';'		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
        }
    ;

StatementExpression
    :   Assignment						{PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	PreIncrementExpression			{PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	PreDecrementExpression			{PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	PostIncrementExpression			{PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	PostDecrementExpression			{PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	MethodInvocation				{PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	ClassInstanceCreationExpression	{PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    ;

_ncounter_:  {if (regularPass()) $$.data = nextGeneratedLocalSymbol();}
    ;

_nlabel_:	{if (regularPass()) $$.data = nextGeneratedLabelSymbol();}
    ;

_ngoto_:	{if (regularPass()) $$.data = nextGeneratedGotoSymbol();}
    ;

_nfork_:	{if (regularPass()) $$.data = nextGeneratedForkSymbol();}
    ;


IfThenStatement
    :   IF '(' Expression ')' _nfork_ Statement                     {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    generateInternalLabelReference($5.data, UsageDefined);
                } else {
                    PropagateBoundaries($$, $1, $6);
                }
            }
        }
    ;

IfThenElseStatementPrefix
    :   IF '(' Expression ')' _nfork_ StatementNoShortIf ELSE _ngoto_ {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    generateInternalLabelReference($5.data, UsageDefined);
                    $$.data = $8.data;
                } else {
                    PropagateBoundaries($$, $1, $7);
                }
            }
        }
    ;

IfThenElseStatement
    :   IfThenElseStatementPrefix Statement {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    generateInternalLabelReference($1.data, UsageDefined);
                } else {
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    ;

IfThenElseStatementNoShortIf
    :   IfThenElseStatementPrefix StatementNoShortIf {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    generateInternalLabelReference($1.data, UsageDefined);
                } else {
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    ;

SwitchStatement
    :   SWITCH '(' Expression ')' /*5*/ _ncounter_  {/*6*/
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $<symbol>$ = addContinueBreakLabelSymbol(1000*$5.data,SWITCH_LABEL_NAME);
                }
            }
        } {/*7*/
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $<symbol>$ = addContinueBreakLabelSymbol($5.data, BREAK_LABEL_NAME);
                    generateInternalLabelReference($5.data, UsageFork);
                }
            }
        }   SwitchBlock                     {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    generateSwitchCaseFork(true);
                    deleteContinueBreakSymbol($<symbol>7);
                    deleteContinueBreakSymbol($<symbol>6);
                    generateInternalLabelReference($5.data, UsageDefined);
                } else {
                    PropagateBoundaries($$, $1, $8);
                }
            }
        }
    ;

SwitchBlock
    :   '{' Start_block SwitchBlockStatementGroups SwitchLabels End_block '}'	{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $6);
        }
    |	'{' Start_block SwitchBlockStatementGroups End_block '}'				{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $5);
        }
    |	'{' Start_block SwitchLabels End_block '}'								{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $5);
        }
    |	'{' '}'																	{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
        }
    ;

SwitchBlockStatementGroups
    :   SwitchBlockStatementGroup								/*& {$$=$1;} &*/
    |	SwitchBlockStatementGroups SwitchBlockStatementGroup	{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
        }
    ;

SwitchBlockStatementGroup
    :   SwitchLabels                            {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    generateSwitchCaseFork(false);
                }
            }
        } BlockStatements						{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
        }
    ;

SwitchLabels
    :   SwitchLabel								/*& {$$=$1;} &*/
    |	SwitchLabels SwitchLabel				{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
        }
    |	error									{
            SetNullBoundariesFor($$);
        }
    ;

SwitchLabel
    :   CASE ConstantExpression ':'				{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
        }
    |	DEFAULT ':'								{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
        }
    ;

WhileStatementPrefix
    :   WHILE _nlabel_ '(' Expression ')' /*6*/ _nfork_ {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    if (options.serverOperation == OLO_EXTRACT) {
                        Symbol *cl, *bl;
                        cl = bl = NULL;        // just to avoid warning message
                        cl = addContinueBreakLabelSymbol($2.data, CONTINUE_LABEL_NAME);
                        bl = addContinueBreakLabelSymbol($6.data, BREAK_LABEL_NAME);
                        $$.data = newWhileExtractData($2.data, $6.data, cl, bl);
                    } else {
                        $$.data = NULL;
                    }
                } else {
                    PropagateBoundaries($$, $1, $5);
                }
            }
        }
    ;

WhileStatement
    :   WhileStatementPrefix Statement					{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    if ($1.data != NULL) {
                        deleteContinueBreakSymbol($1.data->i4);
                        deleteContinueBreakSymbol($1.data->i3);
                        generateInternalLabelReference($1.data->i1, UsageUsed);
                        generateInternalLabelReference($1.data->i2, UsageDefined);
                    }
                } else {
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    ;

WhileStatementNoShortIf
    :   WhileStatementPrefix StatementNoShortIf			{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    if ($1.data != NULL) {
                        deleteContinueBreakSymbol($1.data->i4);
                        deleteContinueBreakSymbol($1.data->i3);
                        generateInternalLabelReference($1.data->i1, UsageUsed);
                        generateInternalLabelReference($1.data->i2, UsageDefined);
                    }
                } else {
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    ;

DoStatement
    :   DO _nlabel_ _ncounter_ _ncounter_ { /*5*/
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $<symbol>$ = addContinueBreakLabelSymbol($3.data, CONTINUE_LABEL_NAME);
                }
            }
        } {/*6*/
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $<symbol>$ = addContinueBreakLabelSymbol($4.data, BREAK_LABEL_NAME);
                }
            }
        } Statement WHILE {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    deleteContinueBreakSymbol($<symbol>6);
                    deleteContinueBreakSymbol($<symbol>5);
                    generateInternalLabelReference($3.data, UsageDefined);
                }
            }
        } '(' Expression ')' ';'			{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    generateInternalLabelReference($2.data, UsageFork);
                    generateInternalLabelReference($4.data, UsageDefined);
                } else {
                    PropagateBoundaries($$, $1, $13);
                }
            }
        }
    ;

MaybeExpression:							{
            SetNullBoundariesFor($$);
        }
        | Expression						{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
        ;

ForKeyword
    :   FOR			{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    beginBlock();
                } else {
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    ;

ForStatementPrefix
    :   '(' ForInit_opt ';' /*4*/ _nlabel_
        MaybeExpression ';' /*7*/_ngoto_
        /*8*/ _nlabel_  ForUpdate_opt ')' /*11*/ _nfork_    {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    Symbol *ss __attribute__((unused));
                    generateInternalLabelReference($4.data, UsageUsed);
                    generateInternalLabelReference($7.data, UsageDefined);
                    ss = addContinueBreakLabelSymbol($8.data, CONTINUE_LABEL_NAME);
                    ss = addContinueBreakLabelSymbol($11.data, BREAK_LABEL_NAME);
                    $$.data.i1 = $8.data;
                    $$.data.i2 = $11.data;
                } else {
                    PropagateBoundaries($$, $1, $10);
                }
            }
        }
    ;

ForStatementBody
    :   ForStatementPrefix  Statement		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    deleteContinueBreakLabelSymbol(BREAK_LABEL_NAME);
                    deleteContinueBreakLabelSymbol(CONTINUE_LABEL_NAME);
                    generateInternalLabelReference($1.data.i1, UsageUsed);
                    generateInternalLabelReference($1.data.i2, UsageDefined);
                } else {
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    ;

ForStatementNoShortIfBody
    :   ForStatementPrefix  StatementNoShortIf		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    deleteContinueBreakLabelSymbol(BREAK_LABEL_NAME);
                    deleteContinueBreakLabelSymbol(CONTINUE_LABEL_NAME);
                    generateInternalLabelReference($1.data.i1, UsageUsed);
                    generateInternalLabelReference($1.data.i2, UsageDefined);
                } else {
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    ;

ForStatement
    :   ForKeyword ForStatementBody {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    endBlock();
                } else {
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    |	ForKeyword error {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    endBlock();
                } else {
                    SetNullBoundariesFor($$);
                }
            }
        }
    ;

ForStatementNoShortIf
    :   ForKeyword ForStatementNoShortIfBody {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    endBlock();
                } else {
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    |	ForKeyword error {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    endBlock();
                } else {
                    SetNullBoundariesFor($$);
                }
            }
        }
    ;


ForInit_opt:							{
            SetNullBoundariesFor($$);
        }
    |	StatementExpressionList			{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |	LocalVariableDeclaration		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

ForUpdate_opt:							{
            SetNullBoundariesFor($$);
        }
    |	StatementExpressionList			{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

StatementExpressionList
    :   StatementExpression									{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |	StatementExpressionList ',' StatementExpression		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
        }
    ;

BreakStatement
    :   BREAK LabelUseIdentifier ';'		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
        }
    |	BREAK ';'						{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    genContinueBreakReference(BREAK_LABEL_NAME);
                } else {
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    ;

ContinueStatement
    :   CONTINUE LabelUseIdentifier ';'		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
        }
    |	CONTINUE ';'					{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    genContinueBreakReference(CONTINUE_LABEL_NAME);
                } else {
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    ;

ReturnStatement
    :   RETURN Expression ';'			{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    generateInternalLabelReference(-1, UsageUsed);
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	RETURN ';'						{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    generateInternalLabelReference(-1, UsageUsed);
                } else {
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    ;

ThrowStatement
    :   Throw Expression ';'		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    if (options.serverOperation==OLO_EXTRACT) {
                        addCxReference($2.data.typeModifier->u.t, &$1.data->position, UsageThrown, NO_FILE_NUMBER, NO_FILE_NUMBER);
                    }
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    ;

SynchronizedStatement
    :   SYNCHRONIZED '(' Expression ')' Block				{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $5);
        }
    ;

TryCatches
    :   Catches				/* $$ = $1; */
    |	Finally				/* $$ = $1; */
    |	Catches Finally		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
        }
    ;

TryStatement
    :   Try  _nfork_
            {
                if (options.serverOperation == OLO_EXTRACT) {
                    addTrivialCxReference("TryCatch", TypeTryCatchMarker,StorageDefault,
                                            $1.data->position, UsageTryCatchBegin);
                }
            }
        Block
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        generateInternalLabelReference($2.data, UsageDefined);
                    }
                }
            }
        TryCatches		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $6);
            if (options.serverOperation == OLO_EXTRACT) {
                addTrivialCxReference("TryCatch", TypeTryCatchMarker,StorageDefault,
                                        $1.data->position, UsageTryCatchEnd);
            }
        }

    ;

Catches
    :   CatchClause						/* $$ = $1; */
    |	Catches CatchClause		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
        }
    ;

CatchClause
    :   Catch '(' FormalParameter ')' _nfork_ Start_block
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        if ($3.data->type != TypeError) {
                            addNewSymbolDefinition(javaStat->locals, $3.data, StorageAuto,
                                            UsageDefined);
                            if (options.serverOperation == OLO_EXTRACT) {
                                assert($3.data->type==TypeDefault);
                                addCxReference($3.data->u.typeModifier->u.t, &$1.data->position, UsageCatched, NO_FILE_NUMBER, NO_FILE_NUMBER);
                            }
                        }
                    }
                }
            }
        Block End_block
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        generateInternalLabelReference($5.data, UsageDefined);
                    } else {
                        PropagateBoundaries($$, $1, $8);
                    }
                }
            }
    |	Catch '(' FormalParameter ')' ';'		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    if (options.serverOperation == OLO_EXTRACT) {
                        assert($3.data->type==TypeDefault);
                        addCxReference($3.data->u.typeModifier->u.t, &$1.data->position, UsageCatched, NO_FILE_NUMBER, NO_FILE_NUMBER);
                    }
                } else {
                    PropagateBoundaries($$, $1, $5);
                }
            }
        }
    ;

Finally
    :   FINALLY _nfork_ Block {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    generateInternalLabelReference($2.data, UsageDefined);
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    ;

/* *********************** expressions ************************** */

Primary
    :   PrimaryNoNewArray					{
            if (regularPass()) {
                $$.data = $1.data;
                if (! SyntaxPassOnly()) {
                    s_javaCompletionLastPrimary = s_structRecordCompletionType = $$.data.typeModifier;
                } else {
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	ArrayCreationExpression				{
            if (regularPass()) {
                $$.data = $1.data;
                if (! SyntaxPassOnly()) {
                    s_javaCompletionLastPrimary = s_structRecordCompletionType = $$.data.typeModifier;
                } else {
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    ;

PrimaryNoNewArray
    :   Literal								/* $$ = $1; */
    |	This								{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    assert(javaStat && javaStat->thisType);
//fprintf(dumpOut,"this == %s\n",s_javaStat->thisType->u.t->linkName);
                    $$.data.typeModifier = javaStat->thisType;
                    addThisCxReferences(javaStat->classFileNumber, &$1.data->position);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = &$1.data->position;
                    javaCheckForStaticPrefixStart(&$1.data->position, &$1.data->position);
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	Name '.' THIS						{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    javaQualifiedThis($1.data, $3.data);
                    $$.data.typeModifier = javaClassNameType($1.data);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = javaGetNameStartingPosition($1.data);
                    javaCheckForStaticPrefixStart(&$3.data->position, javaGetNameStartingPosition($1.data));
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	PrimitiveType '.' CLASS				{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = &s_javaClassModifier;
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = $1.data.position;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	Name '.' CLASS						{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    Symbol *str;
                    javaClassifyToTypeName($1.data,UsageUsed, &str, USELESS_FQT_REFS_ALLOWED);
                    $$.data.typeModifier = &s_javaClassModifier;
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = javaGetNameStartingPosition($1.data);
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	ArrayType '.' CLASS					{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = &s_javaClassModifier;
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = $1.data.position;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	VOID '.' CLASS						{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = &s_javaClassModifier;
                    $$.data.reference = NULL;
                } else {
                    SetPrimitiveTypePos($$.data.position, $1.data);
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	'(' Expression ')'					{
            if (regularPass()) {
                $$.data = $2.data;
                if (SyntaxPassOnly()) {
                    $$.data.position = StackMemoryAlloc(Position);
                    *$$.data.position = $1.data;
                    PropagateBoundaries($$, $1, $3);
                    if (positionIsBetween($$.b, cxRefPosition, $$.e)
                        && parsedPositions[SPP_PARENTHESED_EXPRESSION_LPAR_POSITION].file == NO_FILE_NUMBER) {
                        parsedPositions[SPP_PARENTHESED_EXPRESSION_LPAR_POSITION] = $1.b;
                        parsedPositions[SPP_PARENTHESED_EXPRESSION_RPAR_POSITION] = $3.b;
                        parsedPositions[SPP_PARENTHESED_EXPRESSION_BEGIN_POSITION] = $2.b;
                        parsedPositions[SPP_PARENTHESED_EXPRESSION_END_POSITION] = $2.e;
                    }
                }
            }
        }
    |	ClassInstanceCreationExpression		/*& { $$.data = $1.data } &*/
        /* TODO: Here c-xref parsing/analysis stops, anything beyond
           this point does not register */
    |	FieldAccess							/*& { $$.data = $1.data } &*/
    |	MethodInvocation					/*& { $$.data = $1.data } &*/
    |	ArrayAccess							/*& { $$.data = $1.data } &*/
    |	CompletionTypeName '.'		{ assert(0); /* rule never used */ }
    ;

_erfs_:		{
            $$ = parsedClassInfo.erfsForParameterCompletion;
        }
    ;

NestedConstructorInvocation
    :   Primary '.' New Name _erfs_
            {
                if (ComputingPossibleParameterCompletion()) {
                    TypeModifier *mm;
                    parsedClassInfo.erfsForParameterCompletion = NULL;
                    if ($1.data.typeModifier->kind == TypeStruct) {
                        mm = javaNestedNewType($1.data.typeModifier->u.t, $3.data, $4.data);
                        if (mm->kind != TypeError) {
                            parsedClassInfo.erfsForParameterCompletion = javaCrErfsForConstructorInvocation(mm->u.t, &($4.data->id.position));
                        }
                    }
                }
            }
        '(' ArgumentList_opt ')'				{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    parsedClassInfo.erfsForParameterCompletion = $5;
                    if ($1.data.typeModifier->kind == TypeStruct) {
                        $$.data.typeModifier = javaNestedNewType($1.data.typeModifier->u.t, $3.data, $4.data);
                    } else {
                        $$.data.typeModifier = &errorModifier;
                    }
                    javaHandleDeclaratorParamPositions(&$4.data->id.position, &$7.data, $8.data.positionList, &$9.data);
                    assert($$.data.typeModifier);
                    $$.data.idList = $4.data;
                    if ($$.data.typeModifier->kind != TypeError) {
                        javaConstructorInvocation($$.data.typeModifier->u.t, &($4.data->id.position), $8.data.typeModifierList);
                    }
                } else {
                    $$.data.position = $1.data.position;
                    PropagateBoundaries($$, $1, $9);
                }
            }
        }
    |	Name '.' New Name _erfs_
            {
                if (ComputingPossibleParameterCompletion()) {
                    TypeModifier *mm;
                    parsedClassInfo.erfsForParameterCompletion = NULL;
                    mm = javaNewAfterName($1.data, $3.data, $4.data);
                    if (mm->kind != TypeError) {
                        parsedClassInfo.erfsForParameterCompletion = javaCrErfsForConstructorInvocation(mm->u.t, &($4.data->id.position));
                    }
                }
            }
        '(' ArgumentList_opt ')'				{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    parsedClassInfo.erfsForParameterCompletion = $5;
                    $$.data.typeModifier = javaNewAfterName($1.data, $3.data, $4.data);
                    $$.data.idList = $4.data;
                    if ($$.data.typeModifier->kind != TypeError) {
                        javaConstructorInvocation($$.data.typeModifier->u.t, &($4.data->id.position), $8.data.typeModifierList);
                    }
                } else {
                    $$.data.position = javaGetNameStartingPosition($1.data);
                    javaHandleDeclaratorParamPositions(&$4.data->id.position, &$7.data, $8.data.positionList, &$9.data);
                    PropagateBoundaries($$, $1, $9);
                }
            }
        }
    ;

NewName
    :   Name	{
            if (ComputingPossibleParameterCompletion()) {
                Symbol            *ss;
                Symbol			*str;
                TypeModifier		*expr;
                Reference			*rr, *lastUselessRef;
                javaClassifyAmbiguousName($1.data, NULL,&str,&expr,&rr, &lastUselessRef, USELESS_FQT_REFS_ALLOWED,
                                          CLASS_TO_TYPE,UsageUsed);
                $1.data->nameType = TypeStruct;
                ss = javaTypeSymbolUsage($1.data, AccessDefault);
                parsedClassInfo.erfsForParameterCompletion = javaCrErfsForConstructorInvocation(ss, &($1.data->id.position));
            }
            $$ = $1;
        }
    ;

ClassInstanceCreationExpression
    :   New _erfs_ NewName '(' ArgumentList_opt ')'							{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    Symbol *ss, *tt, *ei;
                    Symbol *str;
                    TypeModifier *expr;
                    Reference *rr, *lastUselessRef;

                    parsedClassInfo.erfsForParameterCompletion = $2;
                    lastUselessRef = NULL;
                    javaClassifyAmbiguousName($3.data, NULL,&str,&expr,&rr, &lastUselessRef, USELESS_FQT_REFS_ALLOWED,
                                              CLASS_TO_TYPE,UsageUsed);
                    $3.data->nameType = TypeStruct;
                    ss = javaTypeSymbolUsage($3.data, AccessDefault);
                    if (isANestedClass(ss)) {
                        if (javaIsInnerAndCanGetUnnamedEnclosingInstance(ss, &ei)) {
                            // MARIAN(?): before it was s_javaStat->classFileIndex, but be more precise
                            // in reality you should keep both to discover references
                            // to original class from class nested in method.
                            addThisCxReferences(ei->u.structSpec->classFileNumber, &$1.data->position);
                            // MARIAN(?): I have removed following because it makes problems when
                            // expanding to FQT names, WHY IT WAS HERE ???
                            /*& addSpecialFieldReference(LINK_NAME_NOT_FQT_ITEM,StorageField,
                                         javaStat->classFileNumber, &$1.data->position,
                                         UsageNotFQField); &*/
                        } else {
                            // MARIAN(?): here I should annulate class reference, as it is an error
                            // because can't get enclosing instance, this is sufficient to
                            // pull-up/down to report a problem
                            // BERK, It was completely wrong, because it is completely legal
                            // and annulating of reference makes class renaming wrong!
                            // Well, it is legal only for static nested classes.
                            // But for security reasons, I will keep it in comment,
                            /*& if (! (ss->access&AccessStatic)) {
                                    if (rr!=NULL) rr->usage.kind = s_noUsage;
                                } &*/
                        }
                    }
                    javaConstructorInvocation(ss, &($3.data->id.position), $5.data.typeModifierList);
                    tt = javaTypeNameDefinition($3.data);
                    $$.data.typeModifier = tt->u.typeModifier;
                    $$.data.reference = NULL;
                } else {
                    javaHandleDeclaratorParamPositions(&$3.data->id.position, &$4.data, $5.data.positionList, &$6.data);
                    $$.data.position = &$1.data->position;
                    PropagateBoundaries($$, $1, $6);
                }
            }
        }
    |	New _erfs_ NewName '(' ArgumentList_opt ')'
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        Symbol *ss;
                        parsedClassInfo.erfsForParameterCompletion = $2;
                        javaClassifyToTypeName($3.data,UsageUsed, &ss, USELESS_FQT_REFS_ALLOWED);
                        $<symbol>$ = javaTypeNameDefinition($3.data);
                        ss = javaTypeSymbolUsage($3.data, AccessDefault);
                        javaConstructorInvocation(ss, &($3.data->id.position), $5.data.typeModifierList);
                    } else {
                        javaHandleDeclaratorParamPositions(&$3.data->id.position, &$4.data, $5.data.positionList, &$6.data);
                        // seems that there is no problem like in previous case,
                        // interfaces are never inner.
                    }
                } else {
                    Symbol *str, *cls;
                    jslClassifyAmbiguousTypeName($3.data, &str);
                    cls = jslTypeNameDefinition($3.data);
                    jslNewClassDefinitionBegin(&javaAnonymousClassName,
                                                AccessDefault, cls, CPOS_ST);
                }
            }
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        $<trail>$ = newClassDefinitionBegin(&javaAnonymousClassName,AccessDefault, $<symbol>6);
                    }
                }
            }
        ClassBody			{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    newClassDefinitionEnd($<trail>8);
                    assert($<symbol>7 && $<symbol>7->u.typeModifier);
                    $$.data.typeModifier = $<symbol>7->u.typeModifier;
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = &$1.data->position;
                    PropagateBoundaries($$, $1, $9);
                }
            } else {
                jslNewClassDefinitionEnd();
            }
        }
    |	NestedConstructorInvocation								{
            $$.data.typeModifier = $1.data.typeModifier;
            $$.data.position = $1.data.position;
            $$.data.reference = NULL;
            PropagateBoundaries($$, $1, $1);
        }
    |	NestedConstructorInvocation
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        $$.data.typeModifier = $1.data.typeModifier;
                        $$.data.position = $1.data.position;
                        $$.data.reference = NULL;
                        if ($$.data.typeModifier->kind != TypeError) {
                            $<trail>$ = newClassDefinitionBegin(&javaAnonymousClassName, AccessDefault, $$.data.typeModifier->u.t);
                        } else {
                            $<trail>$ = newAnonClassDefinitionBegin(& $1.data.idList->id);
                        }
                    } else {
                        $$.data.position = $1.data.position;
                    }
                } else {
                    jslNewAnonClassDefinitionBegin(&$1.data.idList->id);
                }
            }
        ClassBody
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        newClassDefinitionEnd($<trail>2);
                    } else {
                        PropagateBoundaries($$, $1, $3);
                    }
                } else {
                    jslNewClassDefinitionEnd();
                }
        }
    |	New _erfs_ CompletionConstructorName '('                {
            assert(0); /* rule never used */
        }
    |	Primary '.' New  COMPL_CONSTRUCTOR_NAME3 '('    {
            assert(0); /* rule never used */
        }
    |	Name '.' New  COMPL_CONSTRUCTOR_NAME2 '('       {
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
            $$.data.typeModifierList = NULL;
            $$.data.positionList = NULL;
            SetNullBoundariesFor($$);
        }
    | ArgumentList				/*& { $$.data = $1.data; } &*/
    ;

ArgumentList
    :   Expression									{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifierList = newTypeModifierList($1.data.typeModifier);
                    if (parsedClassInfo.erfsForParameterCompletion!=NULL) {
                        parsedClassInfo.erfsForParameterCompletion->params = $$.data.typeModifierList;
                    }
                } else {
                    $$.data.positionList = NULL;
                    appendPositionToList(&$$.data.positionList, &noPosition);
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	ArgumentList ',' Expression					{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    S_typeModifierList *p;
                    $$.data = $1.data;
                    p = newTypeModifierList($3.data.typeModifier);
                    LIST_APPEND(S_typeModifierList, $$.data.typeModifierList, p);
                    if (parsedClassInfo.erfsForParameterCompletion!=NULL) parsedClassInfo.erfsForParameterCompletion->params = $$.data.typeModifierList;
                } else {
                    appendPositionToList(&$$.data.positionList, &$2.data);
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    | COMPL_UP_FUN_PROFILE							{assert(0);}
    | ArgumentList ',' COMPL_UP_FUN_PROFILE			{assert(0);}
    ;


ArrayCreationExpression
    :   New _erfs_ PrimitiveType DimExprs Dims_opt			{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    int i;
                    $$.data.typeModifier = newSimpleTypeModifier($3.data.u);
                    for(i=0; i<$4.data; i++)
                        prependTypeModifierWith($$.data.typeModifier, TypeArray);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = &$1.data->position;
                    PropagateBoundaries($$, $1, $5);
                    if ($$.e.file == NO_FILE_NUMBER) PropagateBoundaries($$, $$, $4);
                }
            }
        }
    |	New _erfs_ PrimitiveType Dims ArrayInitializer		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    int i;
                    $$.data.typeModifier = newSimpleTypeModifier($3.data.u);
                    for(i=0; i<$4.data; i++)
                        prependTypeModifierWith($$.data.typeModifier, TypeArray);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = &$1.data->position;
                    PropagateBoundaries($$, $1, $5);
                }
            }
        }
    |	New _erfs_ ClassOrInterfaceType DimExprs Dims_opt			{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    int i;
                    assert($3.data && $3.data->u.typeModifier);
                    $$.data.typeModifier = $3.data->u.typeModifier;
                    for(i=0; i<$4.data; i++)
                        prependTypeModifierWith($$.data.typeModifier, TypeArray);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = &$1.data->position;
                    PropagateBoundaries($$, $1, $5);
                    if ($$.e.file == NO_FILE_NUMBER) PropagateBoundaries($$, $$, $4);
                }
            }
        }
    |	New _erfs_ ClassOrInterfaceType Dims ArrayInitializer		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    int i;
                    assert($3.data && $3.data->u.typeModifier);
                    $$.data.typeModifier = $3.data->u.typeModifier;
                    for(i=0; i<$4.data; i++)
                        prependTypeModifierWith($$.data.typeModifier, TypeArray);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = &$1.data->position;
                    PropagateBoundaries($$, $1, $5);
                }
            }
        }
    ;


DimExprs
    :   DimExpr						{
            if (regularPass()) $$.data = 1;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |	DimExprs DimExpr			{
            if (regularPass()) $$.data = $1.data+1;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
        }
    ;

DimExpr
    :   '[' Expression ']'			{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
        }
    ;

Dims_opt
    :							{
        if (regularPass()) $$.data = 0;
            SetNullBoundariesFor($$);
        }
    |	Dims						/*& { $$ = $1; } &*/
    ;

Dims
    :   '[' ']'						{
            if (regularPass()) $$.data = 1;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
        }
    |	Dims '[' ']'				{
            if (regularPass()) $$.data = $1.data+1;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
        }
    ;

FieldAccess
    :   Primary '.' Identifier					{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    Symbol *rec=NULL;
                    assert($1.data.typeModifier);
                    $$.data.reference = NULL;
                    $$.data.position = $1.data.position;
                    if ($1.data.typeModifier->kind == TypeStruct) {
                        javaLoadClassSymbolsFromFile($1.data.typeModifier->u.t);
                        $$.data.reference = findStructureFieldFromType($1.data.typeModifier, $3.data, &rec, CLASS_TO_EXPR);
                        assert(rec);
                        $$.data.typeModifier = rec->u.typeModifier;
                    } else if (currentLanguage == LANG_JAVA) {
                        $$.data.typeModifier = javaArrayFieldAccess($3.data);
                    } else {
                        $$.data.typeModifier = &errorModifier;
                    }
                    assert($$.data.typeModifier);
                } else {
                    $$.data.position = $1.data.position;
                    javaCheckForPrimaryStart(&$3.data->position, $$.data.position);
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	Super '.' Identifier					{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    Symbol *ss,*rec=NULL;

                    $$.data.reference = NULL;
                    $$.data.position = &$1.data->position;
                    ss = javaCurrentSuperClass();
                    if (ss != &errorSymbol && ss->type!=TypeError) {
                        javaLoadClassSymbolsFromFile(ss);
                        $$.data.reference = findStrRecordFromSymbol(ss, $3.data, &rec,
                                                                 CLASS_TO_EXPR, $1.data);
                        assert(rec);
                        $$.data.typeModifier = rec->u.typeModifier;
                    } else {
                        $$.data.typeModifier = &errorModifier;
                    }
                    assert($$.data.typeModifier);
                } else {
                    $$.data.position = &$1.data->position;
                    javaCheckForPrimaryStart(&$3.data->position, $$.data.position);
                    javaCheckForStaticPrefixStart(&$3.data->position, $$.data.position);
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	Name '.' Super '.' Identifier					{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    Symbol *ss,*rec=NULL;

                    ss = javaQualifiedThis($1.data, $3.data);
                    if (ss != &errorSymbol && ss->type!=TypeError) {
                        javaLoadClassSymbolsFromFile(ss);
                        ss = javaGetSuperClass(ss);
                        $$.data.reference = findStrRecordFromSymbol(ss, $5.data, &rec,
                                                                 CLASS_TO_EXPR, NULL);
                        assert(rec);
                        $$.data.typeModifier = rec->u.typeModifier;
                    } else {
                        $$.data.typeModifier = &errorModifier;
                    }
                    $$.data.reference = NULL;
                    assert($$.data.typeModifier);
                } else {
                    $$.data.position = javaGetNameStartingPosition($1.data);
                    javaCheckForPrimaryStart(&$3.data->position, $$.data.position);
                    javaCheckForPrimaryStart(&$5.data->position, $$.data.position);
                    javaCheckForStaticPrefixStart(&$3.data->position, $$.data.position);
                    javaCheckForStaticPrefixStart(&$5.data->position, $$.data.position);
                    PropagateBoundaries($$, $1, $5);
                }
            }
        }
    |	Primary '.' COMPL_STRUCT_REC_PRIM		{ assert(0); }
    |	Super '.' COMPL_STRUCT_REC_SUPER		{ assert(0); }
    |	Name '.' Super '.' COMPL_QUALIF_SUPER	{ assert(0); }
    ;

MethodInvocation
    :   Name _erfs_ {
            if (ComputingPossibleParameterCompletion()) {
                parsedClassInfo.erfsForParameterCompletion = javaCrErfsForMethodInvocationN($1.data);
            }
        } '(' ArgumentList_opt ')'					{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaMethodInvocationN($1.data,$5.data.typeModifierList);
                    $$.data.reference = NULL;
                    parsedClassInfo.erfsForParameterCompletion = $2;
                } else {
                    $$.data.position = javaGetNameStartingPosition($1.data);
                    javaCheckForPrimaryStartInNameList($1.data, $$.data.position);
                    javaCheckForStaticPrefixInNameList($1.data, $$.data.position);
                    javaHandleDeclaratorParamPositions(&$1.data->id.position, &$4.data, $5.data.positionList, &$6.data);
                    PropagateBoundaries($$, $1, $6);
                }
            }
        }
    |	Primary '.' Identifier _erfs_ {
            if (ComputingPossibleParameterCompletion()) {
                parsedClassInfo.erfsForParameterCompletion = javaCrErfsForMethodInvocationT($1.data.typeModifier, $3.data);
            }
        } '(' ArgumentList_opt ')'	{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaMethodInvocationT($1.data.typeModifier, $3.data, $7.data.typeModifierList);
                    $$.data.reference = NULL;
                    parsedClassInfo.erfsForParameterCompletion = $4;
                } else {
                    $$.data.position = $1.data.position;
                    javaCheckForPrimaryStart(&$3.data->position, $$.data.position);
                    javaHandleDeclaratorParamPositions(&$3.data->position, &$6.data, $7.data.positionList, &$8.data);
                    PropagateBoundaries($$, $1, $8);
                }
            }
        }
    |	Super '.' Identifier _erfs_ {
            if (ComputingPossibleParameterCompletion()) {
                parsedClassInfo.erfsForParameterCompletion = javaCrErfsForMethodInvocationS($1.data, $3.data);
            }
        } '(' ArgumentList_opt ')'	{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaMethodInvocationS($1.data, $3.data, $7.data.typeModifierList);
                    $$.data.reference = NULL;
                    parsedClassInfo.erfsForParameterCompletion = $4;
                } else {
                    $$.data.position = &$1.data->position;
                    javaCheckForPrimaryStart(&$1.data->position, $$.data.position);
                    javaCheckForPrimaryStart(&$3.data->position, $$.data.position);
                    javaHandleDeclaratorParamPositions(&$3.data->position, &$6.data, $7.data.positionList, &$8.data);
                    PropagateBoundaries($$, $1, $8);
                }
            }
        }

/* Served by field access
    |	CompletionExpressionName '('				{ assert(0); }
    |	Primary '.' COMPL_METHOD_PRIMARY			{ assert(0); }
    |	SUPER '.' COMPL_METHOD_SUPER				{ assert(0); }
*/
    ;

ArrayAccess
    :   Name '[' Expression ']'							{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    TypeModifier *tt;
                    tt = javaClassifyToExpressionName($1.data, &($$.data.reference));
                    if (tt->kind==TypeArray) $$.data.typeModifier = tt->next;
                    else $$.data.typeModifier = &errorModifier;
                    assert($$.data.typeModifier);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = javaGetNameStartingPosition($1.data);
                    PropagateBoundaries($$, $1, $4);
                }
            }
        }
    |	PrimaryNoNewArray '[' Expression ']'		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    if ($1.data.typeModifier->kind==TypeArray) $$.data.typeModifier = $1.data.typeModifier->next;
                    else $$.data.typeModifier = &errorModifier;
                    assert($$.data.typeModifier);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = $1.data.position;
                    PropagateBoundaries($$, $1, $4);
                }
            }
        }
    |	CompletionExpressionName '['				{ /* rule never used */ }
    ;

PostfixExpression
    :   Primary
    |	Name											{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaClassifyToExpressionName($1.data, &($$.data.reference));
                } else {
                    $$.data.position = javaGetNameStartingPosition($1.data);
                    javaCheckForPrimaryStartInNameList($1.data, $$.data.position);
                    javaCheckForStaticPrefixInNameList($1.data, $$.data.position);
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	PostIncrementExpression
    |	PostDecrementExpression
    |	CompletionExpressionName                    { /* rule never used */ }
    ;

PostIncrementExpression
    :   PostfixExpression INC_OP		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaCheckNumeric($1.data.typeModifier);
                    resetReferenceUsage($1.data.reference, UsageAddrUsed);
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    ;

PostDecrementExpression
    :   PostfixExpression DEC_OP		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaCheckNumeric($1.data.typeModifier);
                    resetReferenceUsage($1.data.reference, UsageAddrUsed);
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    ;

UnaryExpression
    :   PreIncrementExpression
    |	PreDecrementExpression
    |	'+' UnaryExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaNumericPromotion($2.data.typeModifier);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    |	'-' UnaryExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaNumericPromotion($2.data.typeModifier);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    |	UnaryExpressionNotPlusMinus
    ;

PreIncrementExpression
    :   INC_OP UnaryExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaCheckNumeric($2.data.typeModifier);
                    resetReferenceUsage($2.data.reference, UsageAddrUsed);
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    ;

PreDecrementExpression
    :   DEC_OP UnaryExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaCheckNumeric($2.data.typeModifier);
                    resetReferenceUsage($2.data.reference, UsageAddrUsed);
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    ;

UnaryExpressionNotPlusMinus
    :   PostfixExpression
    |	'~' UnaryExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaNumericPromotion($2.data.typeModifier);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    |	'!' UnaryExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    if ($2.data.typeModifier->kind == TypeBoolean) $$.data.typeModifier = $2.data.typeModifier;
                    else $$.data.typeModifier = &errorModifier;
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    |	CastExpression
    ;

CastExpression
    :   '(' ArrayType ')' UnaryExpression					{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    assert($2.data.symbol && $2.data.symbol->u.typeModifier);
                    $$.data.typeModifier = $2.data.symbol->u.typeModifier;
                    $$.data.reference = NULL;
                    assert($$.data.typeModifier->kind == TypeArray);
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $4);
                    if (positionIsBetween($4.b, cxRefPosition, $4.e)
                        && parsedPositions[SPP_CAST_LPAR_POSITION].file == NO_FILE_NUMBER) {
                        parsedPositions[SPP_CAST_LPAR_POSITION] = $1.b;
                        parsedPositions[SPP_CAST_RPAR_POSITION] = $3.b;
                        parsedPositions[SPP_CAST_TYPE_BEGIN_POSITION] = $2.b;
                        parsedPositions[SPP_CAST_TYPE_END_POSITION] = $2.e;
                        parsedPositions[SPP_CAST_EXPRESSION_BEGIN_POSITION] = $4.b;
                        parsedPositions[SPP_CAST_EXPRESSION_END_POSITION] = $4.e;
                    }
                }
            }
        }
    |	'(' PrimitiveType ')' UnaryExpression				{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = newTypeModifier($2.data.u, NULL, NULL);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $4);
                    if (positionIsBetween($4.b, cxRefPosition, $4.e)
                        && parsedPositions[SPP_CAST_LPAR_POSITION].file == NO_FILE_NUMBER) {
                        parsedPositions[SPP_CAST_LPAR_POSITION] = $1.b;
                        parsedPositions[SPP_CAST_RPAR_POSITION] = $3.b;
                        parsedPositions[SPP_CAST_TYPE_BEGIN_POSITION] = $2.b;
                        parsedPositions[SPP_CAST_TYPE_END_POSITION] = $2.e;
                        parsedPositions[SPP_CAST_EXPRESSION_BEGIN_POSITION] = $4.b;
                        parsedPositions[SPP_CAST_EXPRESSION_END_POSITION] = $4.e;
                    }
                }
            }
        }
    |	'(' Expression ')' UnaryExpressionNotPlusMinus		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = $2.data.typeModifier;
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $4);
                    if (positionIsBetween($4.b, cxRefPosition, $4.e)
                        && parsedPositions[SPP_CAST_LPAR_POSITION].file == NO_FILE_NUMBER) {
                        parsedPositions[SPP_CAST_LPAR_POSITION] = $1.b;
                        parsedPositions[SPP_CAST_RPAR_POSITION] = $3.b;
                        parsedPositions[SPP_CAST_TYPE_BEGIN_POSITION] = $2.b;
                        parsedPositions[SPP_CAST_TYPE_END_POSITION] = $2.e;
                        parsedPositions[SPP_CAST_EXPRESSION_BEGIN_POSITION] = $4.b;
                        parsedPositions[SPP_CAST_EXPRESSION_END_POSITION] = $4.e;
                    }
                }
            }
        }
/*	this is original from the JLS
    |	'(' Name Dims ')' UnaryExpressionNotPlusMinus			{
            if (regularPass()) {
            if (! SyntaxPassOnly()) {
            javaClassifyToTypeName($1.d);
            } else {
                PropagateBoundaries($$, $1, $4);
            }
            }
        }
*/
    ;

MultiplicativeExpression
    :   UnaryExpression
    |	MultiplicativeExpression '*' UnaryExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaBinaryNumericPromotion($1.data.typeModifier,
                                                                   $3.data.typeModifier);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	MultiplicativeExpression '/' UnaryExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaBinaryNumericPromotion($1.data.typeModifier,
                                                                   $3.data.typeModifier);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	MultiplicativeExpression '%' UnaryExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaBinaryNumericPromotion($1.data.typeModifier,
                                                                   $3.data.typeModifier);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    ;

AdditiveExpression
    :   MultiplicativeExpression
    |	AdditiveExpression '+' MultiplicativeExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    int st1, st2;
                    st1 = javaIsStringType($1.data.typeModifier);
                    st2 = javaIsStringType($3.data.typeModifier);
                    if (st1 && st2) {
                        $$.data.typeModifier = $1.data.typeModifier;
                    } else if (st1) {
                        $$.data.typeModifier = $1.data.typeModifier;
                        // TODO add reference to 'toString' on $3.d
                    } else if (st2) {
                        $$.data.typeModifier = $3.data.typeModifier;
                        // TODO add reference to 'toString' on $1.d
                    } else {
                        $$.data.typeModifier = javaBinaryNumericPromotion($1.data.typeModifier,
                                                                       $3.data.typeModifier);
                    }
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	AdditiveExpression '-' MultiplicativeExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaBinaryNumericPromotion($1.data.typeModifier,
                                                                   $3.data.typeModifier);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    ;

ShiftExpression
    :   AdditiveExpression
    |	ShiftExpression LEFT_OP AdditiveExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaNumericPromotion($1.data.typeModifier);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	ShiftExpression RIGHT_OP AdditiveExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaNumericPromotion($1.data.typeModifier);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	ShiftExpression URIGHT_OP AdditiveExpression	{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaNumericPromotion($1.data.typeModifier);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    ;

RelationalExpression
    :   ShiftExpression
    |	RelationalExpression '<' ShiftExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	RelationalExpression '>' ShiftExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	RelationalExpression LE_OP ShiftExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	RelationalExpression GE_OP ShiftExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	RelationalExpression INSTANCEOF ReferenceType		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    ;

EqualityExpression
    :   RelationalExpression
    |	EqualityExpression EQ_OP RelationalExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	EqualityExpression NE_OP RelationalExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    ;

AndExpression
    :   EqualityExpression
    |	AndExpression '&' EqualityExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaBitwiseLogicalPromotion($1.data.typeModifier,
                                                                    $3.data.typeModifier);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    ;

ExclusiveOrExpression
    :   AndExpression
    |	ExclusiveOrExpression '^' AndExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaBitwiseLogicalPromotion($1.data.typeModifier,
                                                                    $3.data.typeModifier);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    ;

InclusiveOrExpression
    :   ExclusiveOrExpression
    |	InclusiveOrExpression '|' ExclusiveOrExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaBitwiseLogicalPromotion($1.data.typeModifier,
                                                                    $3.data.typeModifier);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    ;

ConditionalAndExpression
    :   InclusiveOrExpression
    |	ConditionalAndExpression AND_OP InclusiveOrExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    ;

ConditionalOrExpression
    :   ConditionalAndExpression
    |	ConditionalOrExpression OR_OP ConditionalAndExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    ;

ConditionalExpression
    :   ConditionalOrExpression
    |	ConditionalOrExpression '?' Expression ':' ConditionalExpression	{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = javaConditionalPromotion($3.data.typeModifier,
                                                                 $5.data.typeModifier);
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    PropagateBoundaries($$, $1, $5);
                }
            }
        }
    ;

AssignmentExpression
    :   ConditionalExpression
    |	Assignment
    ;

Assignment
    :   LeftHandSide {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    if ($1.data.typeModifier!=NULL && $1.data.typeModifier->kind == TypeStruct) {
                        parsedInfo.lastAssignmentStruct = $1.data.typeModifier->u.t;
                    }
                }
                $$.data = $1.data;
            }
        } AssignmentOperator AssignmentExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    parsedInfo.lastAssignmentStruct = NULL;
                    if ($1.data.reference != NULL && options.serverOperation == OLO_EXTRACT) {
                        Reference *rr;
                        rr = duplicateReference($1.data.reference);
                        $1.data.reference->usage = NO_USAGE;
                        if ($3.data.u == '=') {
                            resetReferenceUsage(rr, UsageLvalUsed);
                        } else {
                            resetReferenceUsage(rr, UsageAddrUsed);
                        }
                    } else {
                        if ($3.data.u == '=') {
                            resetReferenceUsage($1.data.reference, UsageLvalUsed);
                        } else {
                            resetReferenceUsage($1.data.reference, UsageAddrUsed);
                        }
                        $$.data.typeModifier = $1.data.typeModifier;
                        $$.data.reference = NULL;
                    }
                } else {
                    PropagateBoundaries($$, $1, $4);
                    if (options.mode == ServerMode) {
                        if (positionIsBetween($1.b, cxRefPosition, $1.e)) {
                            parsedPositions[SPP_ASSIGNMENT_OPERATOR_POSITION] = $3.b;
                            parsedPositions[SPP_ASSIGNMENT_END_POSITION] = $4.e;
                        }
                    }
                    $$.data.position = NULL_POS;
                }
            }
        }
    ;

LeftHandSide
    :   Name					{
            if (regularPass()) {
                $$.data.position = javaGetNameStartingPosition($1.data);
                if (! SyntaxPassOnly()) {
                    Reference *rr;
                    $$.data.typeModifier = javaClassifyToExpressionName($1.data, &rr);
                    $$.data.reference = rr;
                } else {
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	FieldAccess
    |	ArrayAccess
    |	CompletionExpressionName                    { /* rule never used */ }
    ;

AssignmentOperator
    :   '='					{
            if (regularPass()) $$.data.u = '=';
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   MUL_ASSIGN          {
            if (regularPass()) $$.data.u = MUL_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   DIV_ASSIGN			{
            if (regularPass()) $$.data.u = DIV_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   MOD_ASSIGN			{
            if (regularPass()) $$.data.u = MOD_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   ADD_ASSIGN			{
            if (regularPass()) $$.data.u = ADD_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   SUB_ASSIGN			{
            if (regularPass()) $$.data.u = SUB_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   LEFT_ASSIGN			{
            if (regularPass()) $$.data.u = LEFT_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   RIGHT_ASSIGN		{
            if (regularPass()) $$.data.u = RIGHT_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   URIGHT_ASSIGN		{
            if (regularPass()) $$.data.u = URIGHT_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   AND_ASSIGN			{
            if (regularPass()) $$.data.u = AND_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   XOR_ASSIGN			{
            if (regularPass()) $$.data.u = XOR_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   OR_ASSIGN			{
            if (regularPass()) $$.data.u = OR_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

Expression
    :   AssignmentExpression
    |	error					{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.data.typeModifier = &errorModifier;
                    $$.data.reference = NULL;
                } else {
                    $$.data.position = NULL_POS;
                    SetNullBoundariesFor($$);
                }
            }
        }
    ;

ConstantExpression
    :   Expression
    ;

/* ****************************************************************** */


Start_block:	{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    beginBlock();
                }
            }
        }
    ;

End_block:		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    endBlock();
                }
            }
        }
    ;


/* ****************************************************************** */
/* ****************************************************************** */
%%

void javaParsingInitializations(void) {
    Symbol *symbol;
    /* &javaMapDirectoryFiles2(s_javaLangName,
       javaAddMapedTypeName, NULL, s_javaLangName, NULL); &*/
    symbol = javaTypeSymbolDefinition(s_javaLangObjectName, AccessDefault, ADD_NO);
    s_javaObjectSymbol = symbol;
    initTypeModifierAsStructUnionOrEnum(&s_javaObjectModifier, TypeStruct, symbol,
                                        NULL, NULL);
    s_javaObjectModifier.u.t = symbol;

    symbol = javaTypeSymbolDefinition(s_javaLangStringName, AccessDefault, ADD_NO);
    s_javaStringSymbol = symbol;
    initTypeModifierAsStructUnionOrEnum(&s_javaStringModifier, TypeStruct, symbol,
                                        NULL, NULL);
    s_javaStringModifier.u.t = symbol;

    symbol = javaTypeSymbolDefinition(s_javaLangClassName, AccessDefault, ADD_NO);
    initTypeModifierAsStructUnionOrEnum(&s_javaClassModifier, TypeStruct, symbol,
                                        NULL, NULL);
    s_javaClassModifier.u.t = symbol;
    s_javaCloneableSymbol = javaTypeSymbolDefinition(s_javaLangCloneableName,
                                                     AccessDefault, ADD_NO);
    s_javaIoSerializableSymbol = javaTypeSymbolDefinition(s_javaIoSerializableName,
                                                          AccessDefault, ADD_NO);

    javaInitArrayObject();
}

static CompletionFunctionsTable specialCompletionsTable[]  = {
    { COMPL_THIS_PACKAGE_SPECIAL,	javaCompleteThisPackageName },
    { COMPL_CLASS_DEF_NAME,			javaCompleteClassDefinitionNameSpecial },
    { COMPL_FULL_INHERITED_HEADER,	javaCompleteFullInheritedMethodHeader },
    { COMPL_CONSTRUCTOR_NAME0,		javaCompleteHintForConstructSingleName },
    { COMPL_VARIABLE_NAME_HINT,		javaHintVariableName },
    { COMPL_UP_FUN_PROFILE,			javaHintCompleteMethodParameters },
    {0,NULL}
};

static CompletionFunctionsTable completionsTable[]  = {
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


static CompletionFunctionsTable hintCompletionsTab[]  = {
    { COMPL_TYPE_NAME0,				javaHintCompleteNonImportedTypes },
// Finally I do not know if this is practical
/*&	{ COMPL_IMPORT_SPECIAL,			javaHintImportFqt }, &*/
    {0,NULL}
};

static bool exists_valid_parser_action_on(int token) {
    int yyn1, yyn2;
    bool shift_action = (yyn1 = yysindex[lastyystate]) && (yyn1 += token) >= 0 &&
        yyn1 <= YYTABLESIZE && yycheck[yyn1] == token;
    bool reduce_action = (yyn2 = yyrindex[lastyystate]) && (yyn2 += token) >= 0 &&
        yyn2 <= YYTABLESIZE && yycheck[yyn2] == token;
    bool valid = shift_action || reduce_action;

    return valid;
}

/* These are similar in the three parsers, except that we have macro
   replacement of YACC variables so that we can have multiple parsers
   linked together. Therefore it is not straight forward to refactor
   out commonalities. */
void makeJavaCompletions(char *string, int len, Position *pos) {
    int token;
    CompletionLine completionLine;

    log_trace("completing \"%s\" in state %d", string, lastyystate);
    strncpy(collectedCompletions.idToProcess, string, MAX_FUN_NAME_SIZE);
    collectedCompletions.idToProcess[MAX_FUN_NAME_SIZE-1] = 0;
    initCompletions(&collectedCompletions, len, *pos);

    /* special wizard completions */
    for (int i=0;(token=specialCompletionsTable[i].token)!=0; i++) {
        if (exists_valid_parser_action_on(token)) {
            log_trace("completing %d==%s in state %d", i, tokenNamesTable[token], lastyystate);
            (*specialCompletionsTable[i].fun)(&collectedCompletions);
            if (collectedCompletions.abortFurtherCompletions)
                return;
        }
    }

    /* If there is a wizard completion, RETURN now */
    if (collectedCompletions.alternativeIndex != 0 && options.serverOperation != OLO_SEARCH)
        return;
    for (int i=0;(token=completionsTable[i].token)!=0; i++) {
        if (exists_valid_parser_action_on(token)) {
            log_trace("completing %d==%s in state %d", i, tokenNamesTable[token], lastyystate);
            (*completionsTable[i].fun)(&collectedCompletions);
            if (collectedCompletions.abortFurtherCompletions)
                return;
        }
    }

    /* basic language tokens */
    for (token=0; token<LAST_TOKEN; token++) {
        if (token==IDENTIFIER)
            continue;
        if (exists_valid_parser_action_on(token)) {
            if (tokenNamesTable[token]!= NULL) {
                if (isalpha(*tokenNamesTable[token]) || *tokenNamesTable[token]=='_') {
                    fillCompletionLine(&completionLine, tokenNamesTable[token], NULL, TypeKeyword,0, 0, NULL,NULL);
                    processName(tokenNamesTable[token], &completionLine, 0, &collectedCompletions);
                } else {
                    /*& fillCompletionLine(&completionLine, tokenNamesTable[token], NULL, TypeToken,0, 0, NULL,NULL); &*/
                }
            }
        }
    }

    /* If the completion window is shown, or there is no completion,
<       add also hints (should be optionally) */
    /*& if (collectedCompletions.prefix[0]!=0  && (collectedCompletions.alternativeIndex != 0) &*/
    /*&	&& options.serverOperation != OLO_SEARCH) return; &*/

    for (int i=0;(token=hintCompletionsTab[i].token)!=0; i++) {
        if (exists_valid_parser_action_on(token)) {
            (*hintCompletionsTab[i].fun)(&collectedCompletions);
            if (collectedCompletions.abortFurtherCompletions)
                return;
        }
    }
}
