/*
../../../byacc-1.9/yacc: 2 shift/reduce conflicts.
*/
%{

#define RECURSIVE

#define java_yylex yylex

#include "java_parser.h"

#include "globals.h"
#include "jslsemact.h"
#include "jsemact.h"
#include "editor.h"
#include "cxref.h"
#include "yylex.h"
#include "stdinc.h"
#include "head.h"
#include "misc.h"
#include "commons.h"
#include "html.h"
#include "complete.h"
#include "proto.h"
#include "protocol.h"
#include "extract.h"
#include "semact.h"
#include "symbol.h"
#include "list.h"
#include "filedescriptor.h"
#include "typemodifier.h"

#include "log.h"
#include "utils.h"

#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define java_yyerror styyerror
#define yyErrorRecovery styyErrorRecovery

#define JslAddComposedType(ddd, ttt) jslAppendComposedType(&ddd->u.type, ttt)

#define JslImportSingleDeclaration(iname) {\
    Symbol *sym;\
    jslClassifyAmbiguousTypeName(iname, &sym);\
    jslTypeSymbolDefinition(iname->id.name, iname->next, ADD_YES, ORDER_PREPEND, true);\
}

/* Import on demand has to solve following situation (not handled by JSL) */
/* In case that there is a class and package differing only in letter case in name */
/* then even if classified to type it should be reclassified dep on the case */

#define JslImportOnDemandDeclaration(iname) {                           \
        Symbol *sym;                                                    \
        int st;                                                         \
        st = jslClassifyAmbiguousTypeName(iname, &sym);                 \
        if (st == TypeStruct) {                                         \
            javaLoadClassSymbolsFromFile(sym);                          \
            jslAddNestedClassesToJslTypeTab(sym, ORDER_APPEND);         \
        } else {                                                        \
            javaMapDirectoryFiles2(iname,jslAddMapedImportTypeName,NULL,iname,NULL); \
        }                                                               \
    }

    /* TODO: This is just silly... Convert to something like newPositionAsCopyOf()  */
#define SetPrimitiveTypePos(res, typ) {         \
        if (1 || SyntaxPassOnly()) {            \
            res = StackMemoryAlloc(Position);   \
            *res = typ->p;                      \
        }                                       \
        else assert(0);                         \
    }

#define PropagateBoundaries(node, startSymbol, endSymbol) {node.b=startSymbol.b; node.e=endSymbol.e;}
#define PropagateBoundariesIfRegularSyntaxPass(node, startSymbol, endSymbol) {         \
        if (regularPass()) {                                            \
            if (SyntaxPassOnly()) {PropagateBoundaries(node, startSymbol, endSymbol);} \
        }                                                               \
    }
#define SetNullBoundariesFor(node) {node.b=s_noPos; node.e=s_noPos;}

#define NULL_POS NULL

#define AddComposedType(ddd, ttt) appendComposedType(&ddd->u.type, ttt)


static bool regularPass(void) { return s_jsl == NULL; }
static bool inSecondJslPass() {
    return !regularPass() && s_jsl->pass==2;
}

#define SyntaxPassOnly() (options.server_operation==OLO_GET_PRIMARY_START || options.server_operation==OLO_GET_PARAM_COORDINATES || options.server_operation==OLO_SYNTAX_PASS_ONLY || s_javaPreScanOnly)

#define ComputingPossibleParameterCompletion() (regularPass() && (! SyntaxPassOnly()) && options.taskRegime==RegimeEditServer && options.server_operation==OLO_COMPLETION)

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
%token SIZEOF RESTRICT _ATOMIC _BOOL _THREADLOCAL _NORETURN
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
%token CCC_OPER_PARENTHESIS CCC_OPER_BRACKETS

/* ************************ MULTI TOKENS START ************************ */
%token MULTI_TOKENS_START

/* commons */
%token IDENTIFIER CONSTANT LONG_CONSTANT
%token FLOAT_CONSTANT DOUBLE_CONSTANT
%token STRING_LITERAL
%token LINE_TOK
%token IDENT_TO_COMPLETE        /* identifier under cursor */

/* c-only */
%token CPP_MAC_ARG IDENT_NO_CPP_EXPAND

/* java-only */
%token CHAR_LITERAL

/* c++-only */

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
                    $$.d.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = &s_noPos;
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	FALSE_LITERAL		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = &s_noPos;
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	CONSTANT			{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = newSimpleTypeModifier(TypeInt);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = &s_noPos;
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	LONG_CONSTANT		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = newSimpleTypeModifier(TypeLong);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = &s_noPos;
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	FLOAT_CONSTANT		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = newSimpleTypeModifier(TypeFloat);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = &s_noPos;
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	DOUBLE_CONSTANT		{
            if (regularPass()) {
                $$.d.typeModifier = newSimpleTypeModifier(TypeDouble);
                $$.d.reference = NULL;
                $$.d.position = &s_noPos;
                if (SyntaxPassOnly()) {PropagateBoundaries($$, $1, $1);}
            }
        }
    |	CHAR_LITERAL		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = newSimpleTypeModifier(TypeChar);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = &s_noPos;
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	STRING_LITERAL		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = &s_javaStringModifier;
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = StackMemoryAlloc(Position);
                    *$$.d.position = $1.d;
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	NULL_LITERAL		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = newSimpleTypeModifier(TypeNull);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = StackMemoryAlloc(Position);
                    *$$.d.position = $1.d->p;
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
                    $$.d = typeSpecifier1($1.d.u);
                    s_cps.lastDeclaratorType = NULL;
                } else {
                    PropagateBoundaries($$, $1, $1);
                }
            };
            if (inSecondJslPass()) {
                $$.d = jslTypeSpecifier1($1.d.u);
            }
        }
    |	ReferenceType	{
            $$.d = $1.d;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

PrimitiveType
    :   NumericType		/* $$ = $1 */
    |	BOOLEAN			{
            $$.d.u  = TypeBoolean;
            if (regularPass()) {
                SetPrimitiveTypePos($$.d.p, $1.d);
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
            $$.d.u  = TypeByte;
            if (regularPass()) SetPrimitiveTypePos($$.d.p, $1.d);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |	SHORT			{
            $$.d.u  = TypeShort;
            if (regularPass()) SetPrimitiveTypePos($$.d.p, $1.d);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |	INT				{
            $$.d.u  = TypeInt;
            if (regularPass()) SetPrimitiveTypePos($$.d.p, $1.d);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |	LONG			{
            $$.d.u  = TypeLong;
            if (regularPass()) SetPrimitiveTypePos($$.d.p, $1.d);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |	CHAR			{
            $$.d.u  = TypeChar;
            if (regularPass()) SetPrimitiveTypePos($$.d.p, $1.d);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

FloatingPointType
    :   FLOAT			{
            $$.d.u  = TypeFloat;
            if (regularPass()) SetPrimitiveTypePos($$.d.p, $1.d);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |	DOUBLE			{
            $$.d.u  = TypeDouble;
            if (regularPass()) SetPrimitiveTypePos($$.d.p, $1.d);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

ReferenceType
    :   ClassOrInterfaceType		/*& { $$.d = $1.d; } &*/
    |	ArrayType					{
            $$.d = $1.d.symbol;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

ClassOrInterfaceType
    :   Name			{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    javaClassifyToTypeName($1.d,UsageUsed, &$$.d, USELESS_FQT_REFS_ALLOWED);
                    $$.d = javaTypeNameDefinition($1.d);
                    assert($$.d->u.type);
                    s_cps.lastDeclaratorType = $$.d->u.type->u.t;
                } else {
                    PropagateBoundaries($$, $1, $1);
                }
            };
            if (inSecondJslPass()) {
                Symbol *str;
                jslClassifyAmbiguousTypeName($1.d, &str);
                $$.d = jslTypeNameDefinition($1.d);
            }
        }
    |	CompletionTypeName	{ /* rule never reduced */ }
    ;

/* following is the same, just to distinguish type after EXTEND keyword */
ExtendClassOrInterfaceType
    :   Name			{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    javaClassifyToTypeName($1.d, USAGE_EXTEND_USAGE, &$$.d, USELESS_FQT_REFS_ALLOWED);
                    $$.d = javaTypeNameDefinition($1.d);
                } else {
                    PropagateBoundaries($$, $1, $1);
                }
            };
            if (inSecondJslPass()) {
                Symbol *str;
                jslClassifyAmbiguousTypeName($1.d, &str);
                $$.d = jslTypeNameDefinition($1.d);
            }
        }
    |	CompletionTypeName	{ /* rule never reduced */ }
    ;

ClassType
    :   ClassOrInterfaceType		/*& { $$.d = $1.d; } &*/
    ;

InterfaceType
    :   ClassOrInterfaceType		/*& { $$.d = $1.d; } &*/
    ;

ArrayType
    :   PrimitiveType '[' ']'		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.symbol = typeSpecifier1($1.d.u);
                    $$.d.symbol->u.type = prependComposedType($$.d.symbol->u.type, TypeArray);
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
                $$.d.position = $1.d.p;
                s_cps.lastDeclaratorType = NULL;
            };
            if (inSecondJslPass()) {
                $$.d.symbol = jslTypeSpecifier1($1.d.u);
                $$.d.symbol->u.type = jslPrependComposedType($$.d.symbol->u.type, TypeArray);
            }
        }
    |	Name '[' ']'				{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    javaClassifyToTypeName($1.d,UsageUsed, &($$.d.symbol), USELESS_FQT_REFS_ALLOWED);
                    $$.d.symbol = javaTypeNameDefinition($1.d);
                    assert($$.d.symbol && $$.d.symbol->u.type);
                    s_cps.lastDeclaratorType = $$.d.symbol->u.type->u.t;
                    $$.d.symbol->u.type = prependComposedType($$.d.symbol->u.type, TypeArray);
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
                $$.d.position = javaGetNameStartingPosition($1.d);
            };
            if (inSecondJslPass()) {
                Symbol *ss;
                jslClassifyAmbiguousTypeName($1.d, &ss);
                $$.d.symbol = jslTypeNameDefinition($1.d);
                $$.d.symbol->u.type = jslPrependComposedType($$.d.symbol->u.type, TypeArray);
            }
        }
    |	ArrayType '[' ']'			{
            if (regularPass()) {
                $$.d = $1.d;
                if (! SyntaxPassOnly()) {
                    $$.d.symbol->u.type = prependComposedType($$.d.symbol->u.type, TypeArray);
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            };
            if (inSecondJslPass()) {
                $$.d = $1.d;
                $$.d.symbol->u.type = jslPrependComposedType($$.d.symbol->u.type, TypeArray);
            }
        }
    |   CompletionTypeName '[' ']'	{ /* rule never used */ }
    ;

/* ****************************** Names **************************** */

Identifier:	IDENTIFIER			{
            if (regularPass()) $$.d = newCopyOfId($1.d);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;
This:		THIS				{
            if (regularPass()) $$.d = newCopyOfId($1.d);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;
Super:		SUPER				{
            if (regularPass()) $$.d = newCopyOfId($1.d);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;
New:		NEW					{
            if (regularPass()) $$.d = newCopyOfId($1.d);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;
Import:		IMPORT				{
            if (regularPass()) $$.d = newCopyOfId($1.d);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;
Package:	PACKAGE				{
            if (regularPass()) $$.d = newCopyOfId($1.d);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;
Throw:		THROW				{
            if (regularPass()) $$.d = newCopyOfId($1.d);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;
Try:		TRY					{
            if (regularPass()) $$.d = newCopyOfId($1.d);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;
Catch:		CATCH				{
            if (regularPass()) $$.d = newCopyOfId($1.d);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

Name
    :   SimpleName				{
            $$.d = $1.d;
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    assert(s_javaStat);
                    s_javaStat->lastParsedName = $1.d;
                } else {
                    PropagateBoundaries($$, $1, $1);
                    javaCheckForPrimaryStart(&$1.d->id.p, &$1.d->id.p);
                    javaCheckForStaticPrefixStart(&$1.d->id.p, &$1.d->id.p);
                }
            };
        }
    |	QualifiedName			{
            $$.d = $1.d;
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    assert(s_javaStat);
                    s_javaStat->lastParsedName = $1.d;
                } else {
                    PropagateBoundaries($$, $1, $1);
                    javaCheckForPrimaryStartInNameList($1.d, javaGetNameStartingPosition($1.d));
                    javaCheckForStaticPrefixInNameList($1.d, javaGetNameStartingPosition($1.d));
                }
            };
        }
    ;

SimpleName
    :   IDENTIFIER				{
            $$.d = StackMemoryAlloc(IdList);
            fillIdList($$.d, *$1.d, $1.d->name, TypeDefault, NULL);
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

QualifiedName
    :   Name '.' IDENTIFIER		{
            $$.d = StackMemoryAlloc(IdList);
            fillIdList($$.d, *$3.d, $3.d->name, TypeDefault, $1.d);
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
                    labelReference($1.d,UsageDefined);
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
                    labelReference($1.d,UsageUsed);
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
                assert(s_javaStat);
                *s_javaStat = s_initJavaStat;
                s_javaThisPackageName = "";      // preset for case if copied somewhere
            };
        } PackageDeclaration_opt {
            if (regularPass()) {
                if ($2.d == NULL) {	/* anonymous package */
                    s_javaThisPackageName = "";
                } else {
                    javaClassifyToPackageName($2.d);
                    s_javaThisPackageName = javaCreateComposedName(NULL,$2.d,'/',
                                                                   NULL,NULL,0);
                }
                s_javaStat->currentPackage = s_javaThisPackageName;
                if (! SyntaxPassOnly()) {

                    int             i,j,packlen;
                    char            *cdir, *fname;
                    JslTypeTab	*jsltypeTab;
                    struct stat     st;
                    // it is important to know the package before everything
                    // else, as it must be set on class adding in order to set
                    // isinCurrentPackage field. !!!!!!!!!!!!!!!!
                    // this may be problem for CACHING !!!!
                    if ($2.d == NULL) {	/* anonymous package */
                        s_javaStat->className = NULL;
                        for(i=0,j=0; currentFile.fileName[i]; i++) {
                            if (currentFile.fileName[i] == FILE_PATH_SEPARATOR) j=i;
                        }
                        cdir = StackMemoryAllocC(j+1, char);
                        strncpy(cdir,currentFile.fileName,j); cdir[j]=0;
                        s_javaStat->unnamedPackagePath = cdir;
                        javaCheckIfPackageDirectoryIsInClassOrSourcePath(cdir);
                    } else {
                        javaAddPackageDefinition($2.d);
                        s_javaStat->className = $2.d;
                        for(i=0,j=0; currentFile.fileName[i]; i++) {
                            if (currentFile.fileName[i] == FILE_PATH_SEPARATOR) j=i;
                        }
                        packlen = strlen(s_javaThisPackageName);
                        if (j>packlen && fnnCmp(s_javaThisPackageName,&currentFile.fileName[j-packlen],packlen)==0){
                            cdir = StackMemoryAllocC(j-packlen, char);
                            strncpy(cdir, currentFile.fileName, j-packlen-1); cdir[j-packlen-1]=0;
                            s_javaStat->namedPackagePath = cdir;
                            s_javaStat->currentPackage = "";
                            javaCheckIfPackageDirectoryIsInClassOrSourcePath(cdir);
                        } else {
                            if (options.taskRegime != RegimeEditServer) {
                                warningMessage(ERR_ST, "package name does not match directory name");
                            }
                        }
                    }
                    javaParsingInitializations();
                    // make first and second pass through file
                    assert(s_jsl == NULL); // no nesting
                    jsltypeTab = StackMemoryAlloc(JslTypeTab);
                    jslTypeTabInit(jsltypeTab, MAX_JSL_SYMBOLS);
                    javaReadSymbolFromSourceFileInit(s_olOriginalFileNumber,
                                                     jsltypeTab);

                    fname = fileTable.tab[s_olOriginalFileNumber]->name;
                    if (options.taskRegime == RegimeEditServer
                        && refactoringOptions.refactoringRegime!=RegimeRefactory) {
                        // this must be before reading 's_olOriginalComFile' !!!
                        if (statb(fname, &st)==0) {
                            javaReadSymbolsFromSourceFileNoFreeing(fname, fname);
                        }
                    }

                    // this must be last reading of this class before parsing
                    if (statb(fileTable.tab[s_olOriginalComFileNumber]->name, &st)==0) {
                        javaReadSymbolsFromSourceFileNoFreeing(
                            fileTable.tab[s_olOriginalComFileNumber]->name, fname);
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
                if ($2.d != NULL) {
                    javaClassifyToPackageName($2.d);
                }
                javaCreateComposedName(NULL,$2.d,'/',NULL,ppp,MAX_FILE_NAME_SIZE);
                pname = StackMemoryAllocC(strlen(ppp)+1, char);
                strcpy(pname, ppp);
                s_jsl->classStat = newJslClassStat($2.d, NULL, pname, NULL);
                if (inSecondJslPass()) {
                    char        cdir[MAX_FILE_NAME_SIZE];
                    int         i;
                    int			j;
                    /* add this package types */
                    if ($2.d == NULL) {	/* anonymous package */
                        for(i=0,j=0; currentFile.fileName[i]; i++) {
                            if (currentFile.fileName[i] == FILE_PATH_SEPARATOR) j=i;
                        }
                        strncpy(cdir,currentFile.fileName,j);
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
                JslImportSingleDeclaration($1.d);
            }
        }
    |	TypeImportOnDemandDeclaration			{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
            if (inSecondJslPass()) {
                JslImportOnDemandDeclaration($1.d);
            }
        }
    |	ImportDeclarations SingleTypeImportDeclaration			{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
            if (inSecondJslPass()) {
                JslImportSingleDeclaration($2.d);
            }
        }
    |	ImportDeclarations TypeImportOnDemandDeclaration		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
            if (inSecondJslPass()) {
                JslImportOnDemandDeclaration($2.d);
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
            $$.d = $2.d;
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    Reference *lastUselessRef;
                    Symbol *str;
                    // it was type or packege, but I thing this would be better
                    lastUselessRef = javaClassifyToTypeName($2.d, UsageUsed, &str, USELESS_FQT_REFS_DISALLOWED);
                    // last useless reference is not useless here!
                    if (lastUselessRef!=NULL) lastUselessRef->usage = s_noUsage;
                    s_cps.lastImportLine = $1.d->p.line;
                    if ($2.d->next!=NULL) {
                        javaAddImportConstructionReference(&$2.d->next->id.p, &$1.d->p, UsageDefined);
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
            $$.d = $2.d;
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    Symbol *str;
                    TypeModifier *expr;
                    Reference *rr, *lastUselessRef;
                    int st __attribute__((unused));
                    st = javaClassifyAmbiguousName($2.d, NULL,&str,&expr,&rr,
                                                   &lastUselessRef, USELESS_FQT_REFS_DISALLOWED,
                                                   CLASS_TO_TYPE,UsageUsed);
                    if (lastUselessRef!=NULL) lastUselessRef->usage = s_noUsage;
                    s_cps.lastImportLine = $1.d->p.line;
                    javaAddImportConstructionReference(&$2.d->id.p, &$1.d->p, UsageDefined);
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
            $$.d = NULL;
            if (regularPass()) {
                s_cps.lastImportLine = 0;
                SetNullBoundariesFor($$);
            }
        }
    |	Package Name ';'						{
            $$.d = $2.d;
            if (regularPass()) {
                s_cps.lastImportLine = $1.d->p.line;
                PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
            }
        }
    |	Package error							{
            $$.d = NULL;
            if (regularPass()) {
                s_cps.lastImportLine = $1.d->p.line;
                PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
            }
        }
    |	Package CompletionPackageName ';'		{ /* rule never used */ }
    |	PACKAGE COMPL_THIS_PACKAGE_SPECIAL ';'		{ /* rule never used */ }
    ;

TypeDeclaration
    :   ClassDeclaration		{
            if (regularPass()) {
                javaSetClassSourceInformation(s_javaThisPackageName, $1.d);
                PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
            }
        }
    |	InterfaceDeclaration	{
            if (regularPass()) {
                javaSetClassSourceInformation(s_javaThisPackageName, $1.d);
                PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
            }
        }
    |	';'						{}
    |	error					{}
    ;

/* ************************ LALR special ************************** */

Modifiers_opt:					{
            $$.d = AccessDefault;
            SetNullBoundariesFor($$);
        }
    |	Modifiers				{
            $$.d = $1.d;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

Modifiers
    :   Modifier				/*& { $$ = $1; } &*/
    |	Modifiers Modifier		{
            $$.d = $1.d | $2.d;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
        }
    ;

Modifier
    :   PUBLIC			{ $$.d = AccessPublic; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	PROTECTED		{ $$.d = AccessProtected; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	PRIVATE			{ $$.d = AccessPrivate; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	STATIC			{ $$.d = AccessStatic; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	ABSTRACT		{ $$.d = AccessAbstract; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	FINAL			{ $$.d = AccessFinal; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	NATIVE			{ $$.d = AccessNative; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	SYNCHRONIZED	{ $$.d = AccessSynchronized; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	STRICTFP		{ $$.d = 0; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	TRANSIENT		{ $$.d = AccessTransient; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
    |	VOLATILE		{ $$.d = 0; PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);}
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
                        $<trail>$ = newClassDefinitionBegin($3.d, $1.d, NULL);
                    }
                } else {
                    jslNewClassDefinitionBegin($3.d, $1.d, NULL, CPOS_ST);
                    jslAddDefaultConstructor(s_jsl->classStat->thisClass);
                }
            } Super_opt Interfaces_opt {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        javaAddSuperNestedClassToSymbolTab(s_javaStat->thisClass);
                    }
                } else {
                    jslAddSuperNestedClassesToJslTypeTab(s_jsl->classStat->thisClass);
                }
            } ClassBody {
                if (regularPass()) {
                    $$.d = $3.d;
                    if (! SyntaxPassOnly()) {
                        newClassDefinitionEnd($<trail>4);
                    } else {
                        PropagateBoundaries($$, $1, $8);
                        if ($$.b.file == noFileIndex) PropagateBoundaries($$, $2, $$);
                        if (positionIsInTheSameFileAndBetween($$.b, s_cxRefPos, $$.e)
                            && s_spp[SPP_CLASS_DECLARATION_BEGIN_POSITION].file == noFileIndex) {
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
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        $<trail>$ = newClassDefinitionBegin($3.d, $1.d, NULL);
                    }
                } else {
                    jslNewClassDefinitionBegin($3.d, $1.d, NULL, CPOS_ST);
                }
            }
        error ClassBody
            {
                if (regularPass()) {
                    $$.d = $3.d;
                    if (! SyntaxPassOnly()) {
                        newClassDefinitionEnd($<trail>4);
                    } else {
                        PropagateBoundaries($$, $1, $6);
                        if ($$.b.file == noFileIndex) PropagateBoundaries($$, $2, $6);
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
                        $<trail>$ = newClassDefinitionBegin($3.d, $1.d, NULL);
                    }
                } else {
                    jslNewClassDefinitionBegin($3.d, $1.d, NULL, CPOS_FUNCTION_INNER);
                }
            } Super_opt Interfaces_opt {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        javaAddSuperNestedClassToSymbolTab(s_javaStat->thisClass);
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
                        if ($$.b.file == noFileIndex) PropagateBoundaries($$, $2, $8);
                        if (positionsAreEqual(s_cxRefPos, $3.d->p)) {
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
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        $<trail>$ = newClassDefinitionBegin($3.d, $1.d, NULL);
                    }
                } else {
                    jslNewClassDefinitionBegin($3.d, $1.d, NULL, CPOS_FUNCTION_INNER);
                }
            }
        error ClassBody
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        newClassDefinitionEnd($<trail>4);
                    } else {
                        PropagateBoundaries($$, $1, $6);
                        if ($$.b.file == noFileIndex) PropagateBoundaries($$, $2, $6);
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
                    assert($2.d && $2.d->bits.symbolType == TypeDefault && $2.d->u.type);
                    assert($2.d->u.type->kind == TypeStruct);
                    javaParsedSuperClass($2.d->u.type->u.t);
                } else {
                    PropagateBoundaries($$, $1, $2);
                }
            }
            if (inSecondJslPass()) {
                assert($2.d && $2.d->bits.symbolType == TypeDefault && $2.d->u.type);
                assert($2.d->u.type->kind == TypeStruct);
                jslAddSuperClassOrInterface(s_jsl->classStat->thisClass,
                                            $2.d->u.type->u.t);
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
                    assert($1.d && $1.d->bits.symbolType == TypeDefault && $1.d->u.type);
                    assert($1.d->u.type->kind == TypeStruct);
                    javaParsedSuperClass($1.d->u.type->u.t);
                } else {
                    PropagateBoundaries($$, $1, $1);
                }
            }
            if (inSecondJslPass()) {
                assert($1.d && $1.d->bits.symbolType == TypeDefault && $1.d->u.type);
                assert($1.d->u.type->kind == TypeStruct);
                jslAddSuperClassOrInterface(s_jsl->classStat->thisClass,
                                            $1.d->u.type->u.t);
            }
        }
    |	InterfaceTypeList ',' InterfaceType		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    assert($3.d && $3.d->bits.symbolType == TypeDefault && $3.d->u.type);
                    assert($3.d->u.type->kind == TypeStruct);
                    javaParsedSuperClass($3.d->u.type->u.t);
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            }
            if (inSecondJslPass()) {
                assert($3.d && $3.d->bits.symbolType == TypeDefault && $3.d->u.type);
                assert($3.d->u.type->kind == TypeStruct);
                jslAddSuperClassOrInterface(s_jsl->classStat->thisClass,
                                            $3.d->u.type->u.t);
            }
        }
    ;

_bef_:	{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    // TODO, REDO all this stuff around class/method boundaries !!!!!!
                    if (options.taskRegime == RegimeEditServer) {
                        if (s_cp.parserPassedMarker && !s_cp.thisMethodMemoriesStored){
                            s_cps.cxMemoryIndexAtMethodBegin = s_cp.cxMemoryIndexAtFunctionBegin;
                            s_cps.cxMemoryIndexAtMethodEnd = cxMemory->index;
                            /*& sprintf(tmpBuff,"setting %s, %d,%d   %d,%d",
                                    olcxOptionsName[options.server_operation],
                                    s_cp.parserPassedMarker, s_cp.thisMethodMemoriesStored,
                                    s_cps.cxMemoryIndexAtMethodBegin, s_cps.cxMemoryIndexAtMethodEnd),
                                ppcGenRecord(PPC_BOTTOM_INFORMATION, tmpBuff); &*/
                            s_cp.thisMethodMemoriesStored = 1;
                            if (options.server_operation == OLO_MAYBE_THIS) {
                                changeMethodReferencesUsages(LINK_NAME_MAYBE_THIS_ITEM,
                                                             CategoryLocal, currentFile.lexBuffer.buffer.fileNumber,
                                                             s_javaStat->thisClass);
                            } else if (options.server_operation == OLO_NOT_FQT_REFS) {
                                changeMethodReferencesUsages(LINK_NAME_NOT_FQT_ITEM,
                                                             CategoryLocal,currentFile.lexBuffer.buffer.fileNumber,
                                                             s_javaStat->thisClass);
                            } else if (options.server_operation == OLO_USELESS_LONG_NAME) {
                                changeMethodReferencesUsages(LINK_NAME_IMPORTED_QUALIFIED_ITEM,
                                                             CategoryGlobal,currentFile.lexBuffer.buffer.fileNumber,
                                                             s_javaStat->thisClass);
                            }
                            s_cps.cxMemoryIndexAtClassBeginning = s_cp.cxMemoryIndexdiAtClassBegin;
                            s_cps.cxMemoryIndexAtClassEnd = cxMemory->index;
                            s_cps.classCoordEndLine = currentFile.lineNumber+1;
                            /*& fprintf(dumpOut,"!setting class end line to %d, cb==%d, ce==%d\n",
                              s_cps.classCoordEndLine, s_cps.cxMemoryIndexAtClassBeginning,
                              s_cps.cxMemoryIndexAtClassEnd); &*/
                            if (options.server_operation == OLO_NOT_FQT_REFS_IN_CLASS) {
                                changeClassReferencesUsages(LINK_NAME_NOT_FQT_ITEM,
                                                            CategoryLocal,currentFile.lexBuffer.buffer.fileNumber,
                                                            s_javaStat->thisClass);
                            } else if (options.server_operation == OLO_USELESS_LONG_NAME_IN_CLASS) {
                                changeClassReferencesUsages(LINK_NAME_IMPORTED_QUALIFIED_ITEM,
                                                            CategoryGlobal,currentFile.lexBuffer.buffer.fileNumber,
                                                            s_javaStat->thisClass);
                            }
                        }
                    }
                    s_cp.cxMemoryIndexdiAtClassBegin = cxMemory->index;
                    /*& fprintf(dumpOut,"!setting class begin memory %d\n",
                      s_cp.cxMemoryIndexdiAtClassBegin); &*/
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
            $$.d = $1.d;
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    s_cps.lastAssignementStruct = $1.d;
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
                    s_cps.lastAssignementStruct = NULL;
                    clas = s_javaStat->thisClass;
                    assert(clas != NULL);
                    for(p=$3.d; p!=NULL; p=pp) {
                        pp = p->next;
                        p->next = NULL;
                        if (p->bits.symbolType == TypeError) continue;
                        assert(p->bits.symbolType == TypeDefault);
                        completeDeclarator($2.d, p);
                        vClass = s_javaStat->classFileIndex;
                        p->bits.access = $1.d;
                        p->bits.storage = StorageField;
                        if (clas->bits.access&AccessInterface) {
                            // set interface default access flags
                            p->bits.access |= (AccessPublic | AccessStatic | AccessFinal);
                        }
                        /*& javaSetFieldLinkName(p); &*/
                        iniFind(clas, &rfs);
                        if (findStrRecordSym(&rfs, p->name, &memb, CLASS_TO_ANY,
                                             ACCESSIBILITY_CHECK_NO,VISIBILITY_CHECK_NO) == RETURN_NOT_FOUND) {
                            assert(clas->u.s);
                            LIST_APPEND(Symbol, clas->u.s->records, p);
                        }
                        addCxReference(p, &p->pos, UsageDefined, vClass, vClass);
                        htmlAddJavaDocReference(p, &p->pos, vClass, vClass);
                    }
                    $$.d = $3.d;
                    if (options.taskRegime == RegimeEditServer
                        && s_cp.parserPassedMarker
                        && !s_cp.thisMethodMemoriesStored){
                        s_cps.methodCoordEndLine = currentFile.lineNumber+1;
                    }
                } else {
                    PropagateBoundaries($$, $1, $4);
                    if ($$.b.file == noFileIndex) PropagateBoundaries($$, $2, $4);
                    if (positionIsInTheSameFileAndBetween($$.b, s_cxRefPos, $$.e)
                        && s_spp[SPP_FIELD_DECLARATION_BEGIN_POSITION].file==noFileIndex) {
                        s_spp[SPP_FIELD_DECLARATION_BEGIN_POSITION] = $$.b;
                        s_spp[SPP_FIELD_DECLARATION_TYPE_BEGIN_POSITION] = $2.b;
                        s_spp[SPP_FIELD_DECLARATION_TYPE_END_POSITION] = $2.e;
                        s_spp[SPP_FIELD_DECLARATION_END_POSITION] = $$.e;
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
                for(p=$3.d; p!=NULL; p=pp) {
                    pp = p->next;
                    p->next = NULL;
                    if (p->bits.symbolType == TypeError) continue;
                    assert(p->bits.symbolType == TypeDefault);
                    assert(clas->u.s);
                    vClass = clas->u.s->classFile;
                    jslCompleteDeclarator($2.d, p);
                    p->bits.access = $1.d;
                    p->bits.storage = StorageField;
                    if (clas->bits.access&AccessInterface) {
                        // set interface default access flags
                        p->bits.access |= (AccessPublic|AccessStatic|AccessFinal);
                    }
                    log_debug("[jsl] adding field %s to %s\n",
                              p->name,clas->linkName);
                    LIST_APPEND(Symbol, clas->u.s->records, p);
                    assert(vClass!=noFileIndex);
                    if (p->pos.file!=s_olOriginalFileNumber && options.server_operation==OLO_PUSH) {
                        // pre load of saved file akes problem on move field/method, ...
                        addCxReference(p, &p->pos, UsageDefined, vClass, vClass);
                    }
                }
                $$.d = $3.d;
            }
        }
    ;

VariableDeclarators
    :   VariableDeclarator								{
            $$.d = $1.d;
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    assert($$.d->bits.symbolType == TypeDefault || $$.d->bits.symbolType == TypeError);
                } else {
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	VariableDeclarators ',' VariableDeclarator		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    assert($1.d && $3.d);
                    if ($3.d->bits.storage == StorageError) {
                        $$.d = $1.d;
                    } else {
                        $$.d = $3.d;
                        $$.d->next = $1.d;
                    }
                    assert($$.d->bits.symbolType == TypeDefault || $$.d->bits.symbolType == TypeError);
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            }
            if (inSecondJslPass()) {
                assert($1.d && $3.d);
                if ($3.d->bits.storage == StorageError) {
                    $$.d = $1.d;
                } else {
                    $$.d = $3.d;
                    $$.d->next = $1.d;
                }
                assert($$.d->bits.symbolType==TypeDefault || $$.d->bits.symbolType==TypeError);
            }
        }
    ;

VariableDeclarator
    :   VariableDeclaratorId							/*	{ $$ = $1; } */
    |	VariableDeclaratorId '=' VariableInitializer	{
            $$.d = $1.d;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
        }
    |	error											{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d = newSymbolAsCopyOf(&s_errorSymbol);
                } else {
                    SetNullBoundariesFor($$);
                }
            }
            if (inSecondJslPass()) {
                CF_ALLOC($$.d, Symbol);
                *$$.d = s_errorSymbol;
            }
        }
    ;

VariableDeclaratorId
    :   Identifier							{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d = newSymbol($1.d->name, $1.d->name, $1.d->p);
                } else {
                    PropagateBoundaries($$, $1, $1);
                }
            }
            if (inSecondJslPass()) {
                char *name;
                CF_ALLOCC(name, strlen($1.d->name)+1, char);
                strcpy(name, $1.d->name);
                CF_ALLOC($$.d, Symbol);
                fillSymbol($$.d, name, name, $1.d->p);
            }
        }
    |	VariableDeclaratorId '[' ']'		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    assert($1.d);
                    $$.d = $1.d;
                    AddComposedType($$.d, TypeArray);
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            }
            if (inSecondJslPass()) {
                assert($1.d);
                $$.d = $1.d;
                JslAddComposedType($$.d, TypeArray);
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
                        javaMethodBodyBeginning($1.d);
                    }
                }
            }
        MethodBody Stop_block
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        javaMethodBodyEnding(&$4.d);
                    } else {
                        PropagateBoundaries($$, $1, $4);
                        if (positionIsInTheSameFileAndBetween($1.b, s_cxRefPos, $1.e)) {
                            s_spp[SPP_METHOD_DECLARATION_BEGIN_POSITION] = $$.b;
                            s_spp[SPP_METHOD_DECLARATION_END_POSITION] = $$.e;
                        }
                    }
                }
            }
    ;

MethodHeader
    :   Modifiers_opt AssignmentType MethodDeclarator Throws_opt	{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    s_cps.lastAssignementStruct = NULL;
                    $$.d = javaMethodHeader($1.d,$2.d,$3.d, StorageMethod);
                } else {
                    PropagateBoundaries($$, $1, $4);
                    if ($$.b.file == noFileIndex) PropagateBoundaries($$, $2, $$);
                    if ($$.e.file == noFileIndex) PropagateBoundaries($$, $$, $3);
                    if (positionIsInTheSameFileAndBetween($$.b, s_cxRefPos, $3.e)) {
                        s_spp[SPP_METHOD_DECLARATION_TYPE_BEGIN_POSITION] = $2.b;
                        s_spp[SPP_METHOD_DECLARATION_TYPE_END_POSITION] = $2.e;
                    }
                }
            }
            if (inSecondJslPass()) {
                $$.d = jslMethodHeader($1.d,$2.d,$3.d,StorageMethod, $4.d);
            }
        }
    |	Modifiers_opt VOID MethodDeclarator Throws_opt	{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d = javaMethodHeader($1.d,&s_defaultVoidDefinition,$3.d,StorageMethod);
                } else {
                    PropagateBoundaries($$, $1, $4);
                    if ($$.b.file == noFileIndex) PropagateBoundaries($$, $2, $$);
                    if ($$.e.file == noFileIndex) PropagateBoundaries($$, $$, $3);
                    if (positionIsInTheSameFileAndBetween($$.b, s_cxRefPos, $3.e)) {
                        s_spp[SPP_METHOD_DECLARATION_TYPE_BEGIN_POSITION] = $2.b;
                        s_spp[SPP_METHOD_DECLARATION_TYPE_END_POSITION] = $2.e;
                    }
                }
            }
            if (inSecondJslPass()) {
                $$.d = jslMethodHeader($1.d,&s_defaultVoidDefinition,$3.d,StorageMethod, $4.d);
            }
        }
    |	COMPL_FULL_INHERITED_HEADER		{assert(0);}
    ;

MethodDeclarator
    :   Identifier
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        $<symbol>$ = javaCreateNewMethod($1.d->name, &($1.d->p), MEMORY_XX);
                    }
                }
                if (inSecondJslPass()) {
                    $<symbol>$ = javaCreateNewMethod($1.d->name,&($1.d->p), MEMORY_CF);
                }
            }
        '(' FormalParameterList_opt ')'
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        $$.d = $<symbol>2;
                        assert($$.d && $$.d->u.type && $$.d->u.type->kind == TypeFunction);
                        initFunctionTypeModifier(&$$.d->u.type->u.f , $4.d.s);
                    } else {
                        javaHandleDeclaratorParamPositions(&$1.d->p, &$3.d, $4.d.p, &$5.d);
                        PropagateBoundaries($$, $1, $5);
                    }
                }
                if (inSecondJslPass()) {
                    $$.d = $<symbol>2;
                    assert($$.d && $$.d->u.type && $$.d->u.type->kind == TypeFunction);
                    initFunctionTypeModifier(&$$.d->u.type->u.f , $4.d.s);
                }
            }
    |	MethodDeclarator '[' ']'						{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d = $1.d;
                    AddComposedType($$.d, TypeArray);
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            }
            if (inSecondJslPass()) {
                $$.d = $1.d;
                JslAddComposedType($$.d, TypeArray);
            }
        }
    |	COMPL_METHOD_NAME0					{ assert(0);}
    ;

FormalParameterList_opt:					{
            $$.d.s = NULL;
            $$.d.p = NULL;
            SetNullBoundariesFor($$);
        }
    |	FormalParameterList					/*& {$$ = $1;} &*/
    ;

FormalParameterList
    :   FormalParameter								{
            if (! SyntaxPassOnly()) {
                $$.d.s = $1.d;
            } else {
                $$.d.p = NULL;
                appendPositionToList(&$$.d.p, &s_noPos);
                PropagateBoundaries($$, $1, $1);
            }
        }
    |	FormalParameterList ',' FormalParameter		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d = $1.d;
                    LIST_APPEND(Symbol, $$.d.s, $3.d);
                } else {
                    appendPositionToList(&$$.d.p, &$2.d);
                    PropagateBoundaries($$, $1, $3);
                }
            }
            if (inSecondJslPass()) {
                $$.d = $1.d;
                LIST_APPEND(Symbol, $$.d.s, $3.d);
            }
        }
    ;

FormalParameter
    :   JavaType VariableDeclaratorId			{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d = $2.d;
                    completeDeclarator($1.d, $2.d);
                } else {
                    PropagateBoundaries($$, $1, $2);
                }
            }
            if (inSecondJslPass()) {
                $$.d = $2.d;
                completeDeclarator($1.d, $2.d);
            }
        }
    |	FINAL JavaType VariableDeclaratorId		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d = $3.d;
                    completeDeclarator($2.d, $3.d);
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            }
            if (inSecondJslPass()) {
                $$.d = $3.d;
                completeDeclarator($2.d, $3.d);
            }
        }
    |	error								{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d = newSymbolAsCopyOf(&s_errorSymbol);
                } else {
                    SetNullBoundariesFor($$);
                }
            }
            if (inSecondJslPass()) {
                CF_ALLOC($$.d, Symbol);
                *$$.d = s_errorSymbol;
            }
        }
    ;

Throws_opt:								{
            $$.d = NULL;
            SetNullBoundariesFor($$);
        }
    |	THROWS ClassTypeList			{
            $$.d = $2.d;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
        }
    ;

ClassTypeList
    :   ClassType						{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
            if (inSecondJslPass()) {
                assert($1.d && $1.d->bits.symbolType == TypeDefault && $1.d->u.type);
                assert($1.d->u.type->kind == TypeStruct);
                CF_ALLOC($$.d, SymbolList);
                /* REPLACED: FILL_symbolList($$.d, $1.d->u.type->u.t, NULL); with compound literal */
                *$$.d = (SymbolList){.d = $1.d->u.type->u.t, .next = NULL};
            }
        }
    |	ClassTypeList ',' ClassType		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
            if (inSecondJslPass()) {
                assert($3.d && $3.d->bits.symbolType == TypeDefault && $3.d->u.type);
                assert($3.d->u.type->kind == TypeStruct);
                CF_ALLOC($$.d, SymbolList);
                /* REPLACED: FILL_symbolList($$.d, $3.d->u.type->u.t, $1.d); with compound literal */
                *$$.d = (SymbolList){.d = $3.d->u.type->u.t, .next = $1.d};
            }
        }
    ;

MethodBody
    :   Block				/*& { $$ = $1; } &*/
    |	';'					{
            $$.d = $1.d;
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

                        args = $2.d;
                        /*&
                          if (! ($1.d & AccessStatic)) {
                              args = javaPrependDirectEnclosingInstanceArgument($2.d);
                          }
                          &*/
                        mh=javaMethodHeader($1.d, &s_errorSymbol, args, StorageConstructor);
                        // TODO! Merge this with 'javaMethodBodyBeginning'!
                        assert(mh->u.type && mh->u.type->kind == TypeFunction);
                        stackMemoryBlockStart();  // in order to remove arguments
                        s_cp.function = mh; /* added for set-target-position checks */
                        /* also needed for pushing label reference */
                        generateInternalLabelReference(-1, UsageDefined);
                        counters.localVar = 0;
                        assert($2.d && $2.d->u.type);
                        javaAddMethodParametersToSymTable($2.d);
                        mh->u.type->u.m.signature = strchr(mh->linkName, '(');
                        s_javaStat->methodModifiers = $1.d;
                    }
                }
                if (inSecondJslPass()) {
                    Symbol *args;
                    args = $2.d;
                    jslMethodHeader($1.d,&s_defaultVoidDefinition,args,
                                    StorageConstructor, $3.d);
                }
            }
         Start_block ConstructorBody Stop_block {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    stackMemoryBlockFree();
                    if (options.taskRegime == RegimeHtmlGenerate) {
                        htmlAddFunctionSeparatorReference();
                    } else {
                        PropagateBoundaries($$, $1, $6);
                        if ($$.b.file == noFileIndex) PropagateBoundaries($$, $2, $$);
                    }
                }
                s_cp.function = NULL; /* added for set-target-position checks */
            }
        }
    ;

ConstructorDeclarator
    :   Identifier
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        if (strcmp($1.d->name, s_javaStat->thisClass->name)==0) {
                            addCxReference(s_javaStat->thisClass, &$1.d->p,
                                           UsageConstructorDefinition,noFileIndex, noFileIndex);
                            $<symbol>$ = javaCreateNewMethod($1.d->name,//JAVA_CONSTRUCTOR_NAME1,
                                                             &($1.d->p), MEMORY_XX);
                        } else {
                            // a type forgotten for a method?
                            $<symbol>$ = javaCreateNewMethod($1.d->name,&($1.d->p),MEMORY_XX);
                        }
                    }
                }
                if (inSecondJslPass()) {
                    if (strcmp($1.d->name, s_jsl->classStat->thisClass->name)==0) {
                        $<symbol>$ = javaCreateNewMethod(
                                        $1.d->name, //JAVA_CONSTRUCTOR_NAME1,
                                        &($1.d->p),
                                        MEMORY_CF);
                    } else {
                        // a type forgotten for a method?
                        $<symbol>$ = javaCreateNewMethod($1.d->name, &($1.d->p), MEMORY_CF);
                    }
                }
            }
        '(' FormalParameterList_opt ')'
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        $$.d = $<symbol>2;
                        assert($$.d && $$.d->u.type && $$.d->u.type->kind == TypeFunction);
                        initFunctionTypeModifier(&$$.d->u.type->u.f , $4.d.s);
                    } else {
                        javaHandleDeclaratorParamPositions(&$1.d->p, &$3.d, $4.d.p, &$5.d);
                        PropagateBoundaries($$, $1, $5);
                    }
                }
                if (inSecondJslPass()) {
                    $$.d = $<symbol>2;
                    assert($$.d && $$.d->u.type && $$.d->u.type->kind == TypeFunction);
                    initFunctionTypeModifier(&$$.d->u.type->u.f , $4.d.s);
                };
            }
    ;

ConstructorBody
    :   '{' Start_block ExplicitConstructorInvocation BlockStatements Stop_block '}'	{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $6);
        }
    |	'{' Start_block ExplicitConstructorInvocation Stop_block '}'					{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $5);
        }
    |	'{' Start_block BlockStatements Stop_block '}'									{
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
                    s_cp.erfsForParamsComplet = javaCrErfsForConstructorInvocation(s_javaStat->thisClass, &$1.d->p);
                }
            } '(' ArgumentList_opt ')'			{
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        javaConstructorInvocation(s_javaStat->thisClass, &($1.d->p), $5.d.t);
                        s_cp.erfsForParamsComplet = $2;
                    } else {
                        javaHandleDeclaratorParamPositions(&$1.d->p, &$4.d, $5.d.p, &$6.d);
                        PropagateBoundaries($$, $1, $6);
                    }
                }
            }
    |	Super _erfs_
            {
                if (ComputingPossibleParameterCompletion()) {
                    s_cp.erfsForParamsComplet = javaCrErfsForConstructorInvocation(javaCurrentSuperClass(), &$1.d->p);
                }
            }   '(' ArgumentList_opt ')'			{
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        Symbol *ss;
                        ss = javaCurrentSuperClass();
                        javaConstructorInvocation(ss, &($1.d->p), $5.d.t);
                        s_cp.erfsForParamsComplet = $2;
                    } else {
                        javaHandleDeclaratorParamPositions(&$1.d->p, &$4.d, $5.d.p, &$6.d);
                        PropagateBoundaries($$, $1, $6);
                    }
                }
            }
    |	Primary  '.' Super _erfs_
            {
                if (ComputingPossibleParameterCompletion()) {
                    s_cp.erfsForParamsComplet = javaCrErfsForConstructorInvocation(javaCurrentSuperClass(), &($3.d->p));
                }
            } '(' ArgumentList_opt ')'		{
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        Symbol *ss;
                        ss = javaCurrentSuperClass();
                        javaConstructorInvocation(ss, &($3.d->p), $7.d.t);
                        s_cp.erfsForParamsComplet = $4;
                    } else {
                        javaHandleDeclaratorParamPositions(&$3.d->p, &$6.d, $7.d.p, &$8.d);
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
                    $<trail>$=newClassDefinitionBegin($3.d,($1.d|AccessInterface),NULL);
                }
            } else {
                jslNewClassDefinitionBegin($3.d, ($1.d|AccessInterface), NULL, CPOS_ST);
            }
        } ExtendsInterfaces_opt {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    javaAddSuperNestedClassToSymbolTab(s_javaStat->thisClass);
                }
            } else {
                jslAddSuperNestedClassesToJslTypeTab(s_jsl->classStat->thisClass);
            }
        } InterfaceBody {
            if (regularPass()) {
                $$.d = $3.d;
                if (! SyntaxPassOnly()) {
                    newClassDefinitionEnd($<trail>4);
                } else {
                    PropagateBoundaries($$, $1, $7);
                    if ($$.b.file == noFileIndex) PropagateBoundaries($$, $2, $$);
                    if (positionIsInTheSameFileAndBetween($$.b, s_cxRefPos, $$.e)
                        && s_spp[SPP_CLASS_DECLARATION_BEGIN_POSITION].file == noFileIndex) {
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
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        $<trail>$=newClassDefinitionBegin($3.d,($1.d|AccessInterface),NULL);
                    }
                } else {
                    jslNewClassDefinitionBegin($3.d, ($1.d|AccessInterface), NULL, CPOS_ST);
                }
            }
        error InterfaceBody
            {
                if (regularPass()) {
                    $$.d = $3.d;
                    if (! SyntaxPassOnly()) {
                        newClassDefinitionEnd($<trail>4);
                    } else {
                        PropagateBoundaries($$, $1, $6);
                        if ($$.b.file == noFileIndex) PropagateBoundaries($$, $2, $$);
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
                    assert($2.d && $2.d->bits.symbolType == TypeDefault && $2.d->u.type);
                    assert($2.d->u.type->kind == TypeStruct);
                    javaParsedSuperClass($2.d->u.type->u.t);
                } else {
                    PropagateBoundaries($$, $1, $2);
                }
            }
            if (inSecondJslPass()) {
                assert($2.d && $2.d->bits.symbolType == TypeDefault && $2.d->u.type);
                assert($2.d->u.type->kind == TypeStruct);
                jslAddSuperClassOrInterface(s_jsl->classStat->thisClass,
                                            $2.d->u.type->u.t);
            }
        }
    |	ExtendsInterfaces ',' ExtendClassOrInterfaceType        {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    assert($3.d && $3.d->bits.symbolType == TypeDefault && $3.d->u.type);
                    assert($3.d->u.type->kind == TypeStruct);
                    javaParsedSuperClass($3.d->u.type->u.t);
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            }
            if (inSecondJslPass()) {
                assert($3.d && $3.d->bits.symbolType == TypeDefault && $3.d->u.type);
                assert($3.d->u.type->kind == TypeStruct);
                jslAddSuperClassOrInterface(s_jsl->classStat->thisClass,
                                            $3.d->u.type->u.t);
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
                        javaMethodBodyBeginning($1.d);
                    }
                }
            }
        ';' Stop_block
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        javaMethodBodyEnding(&$4.d);
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
    :   '{' Start_block BlockStatements Stop_block '}'		{
            $$.d = $5.d;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $5);
        }
    |	'{' '}'												{
            $$.d = $2.d;
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
                    addNewDeclaration($1.d,$2.d,NULL,StorageAuto,s_javaStat->locals);
                    $$.d = $1.d;
                } else {
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    |	FINAL JavaType VariableDeclaratorId						{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    addNewDeclaration($2.d,$3.d,NULL,StorageAuto,s_javaStat->locals);
                    $$.d = $2.d;
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	LocalVariableDeclaration ',' VariableDeclaratorId	{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    if ($1.d->bits.symbolType != TypeError) {
                        addNewDeclaration($1.d,$3.d,NULL,StorageAuto,s_javaStat->locals);
                    }
                    $$.d = $1.d;
                } else {
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    ;

LocalVariableDeclaration
    :   LocalVarDeclUntilInit								{
            if (regularPass()) $$.d = $1.d;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |	LocalVarDeclUntilInit {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    s_cps.lastAssignementStruct = $1.d;
                }
            }
        } '=' VariableInitializer		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    s_cps.lastAssignementStruct = NULL;
                    $$.d = $1.d;
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

_ncounter_:  {if (regularPass()) $$.d = nextGeneratedLocalSymbol();}
    ;

_nlabel_:	{if (regularPass()) $$.d = nextGeneratedLabelSymbol();}
    ;

_ngoto_:	{if (regularPass()) $$.d = nextGeneratedGotoSymbol();}
    ;

_nfork_:	{if (regularPass()) $$.d = nextGeneratedForkSymbol();}
    ;


IfThenStatement
    :   IF '(' Expression ')' _nfork_ Statement                     {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    generateInternalLabelReference($5.d, UsageDefined);
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
                    generateInternalLabelReference($5.d, UsageDefined);
                    $$.d = $8.d;
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
                    generateInternalLabelReference($1.d, UsageDefined);
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
                    generateInternalLabelReference($1.d, UsageDefined);
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
                    $<symbol>$ = addContinueBreakLabelSymbol(1000*$5.d,SWITCH_LABEL_NAME);
                }
            }
        } {/*7*/
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $<symbol>$ = addContinueBreakLabelSymbol($5.d, BREAK_LABEL_NAME);
                    generateInternalLabelReference($5.d, UsageFork);
                }
            }
        }   SwitchBlock                     {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    generateSwitchCaseFork(true);
                    deleteContinueBreakSymbol($<symbol>7);
                    deleteContinueBreakSymbol($<symbol>6);
                    generateInternalLabelReference($5.d, UsageDefined);
                } else {
                    PropagateBoundaries($$, $1, $8);
                }
            }
        }
    ;

SwitchBlock
    :   '{' Start_block SwitchBlockStatementGroups SwitchLabels Stop_block '}'	{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $6);
        }
    |	'{' Start_block SwitchBlockStatementGroups Stop_block '}'				{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $5);
        }
    |	'{' Start_block SwitchLabels Stop_block '}'								{
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
                    if (options.server_operation == OLO_EXTRACT) {
                        Symbol *cl, *bl;
                        cl = bl = NULL;        // just to avoid warning message
                        cl = addContinueBreakLabelSymbol($2.d, CONTINUE_LABEL_NAME);
                        bl = addContinueBreakLabelSymbol($6.d, BREAK_LABEL_NAME);
                        $$.d = newWhileExtractData($2.d, $6.d, cl, bl);
                    } else {
                        $$.d = NULL;
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
                    if ($1.d != NULL) {
                        deleteContinueBreakSymbol($1.d->i4);
                        deleteContinueBreakSymbol($1.d->i3);
                        generateInternalLabelReference($1.d->i1, UsageUsed);
                        generateInternalLabelReference($1.d->i2, UsageDefined);
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
                    if ($1.d != NULL) {
                        deleteContinueBreakSymbol($1.d->i4);
                        deleteContinueBreakSymbol($1.d->i3);
                        generateInternalLabelReference($1.d->i1, UsageUsed);
                        generateInternalLabelReference($1.d->i2, UsageDefined);
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
                    $<symbol>$ = addContinueBreakLabelSymbol($3.d, CONTINUE_LABEL_NAME);
                }
            }
        } {/*6*/
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $<symbol>$ = addContinueBreakLabelSymbol($4.d, BREAK_LABEL_NAME);
                }
            }
        } Statement WHILE {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    deleteContinueBreakSymbol($<symbol>6);
                    deleteContinueBreakSymbol($<symbol>5);
                    generateInternalLabelReference($3.d, UsageDefined);
                }
            }
        } '(' Expression ')' ';'			{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    generateInternalLabelReference($2.d, UsageFork);
                    generateInternalLabelReference($4.d, UsageDefined);
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
                    stackMemoryBlockStart();
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
                    generateInternalLabelReference($4.d, UsageUsed);
                    generateInternalLabelReference($7.d, UsageDefined);
                    ss = addContinueBreakLabelSymbol($8.d, CONTINUE_LABEL_NAME);
                    ss = addContinueBreakLabelSymbol($11.d, BREAK_LABEL_NAME);
                    $$.d.i1 = $8.d;
                    $$.d.i2 = $11.d;
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
                    generateInternalLabelReference($1.d.i1, UsageUsed);
                    generateInternalLabelReference($1.d.i2, UsageDefined);
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
                    generateInternalLabelReference($1.d.i1, UsageUsed);
                    generateInternalLabelReference($1.d.i2, UsageDefined);
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
                    stackMemoryBlockFree();
                } else {
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    |	ForKeyword error {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    stackMemoryBlockFree();
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
                    stackMemoryBlockFree();
                } else {
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    |	ForKeyword error {
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    stackMemoryBlockFree();
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
                    if (options.server_operation==OLO_EXTRACT) {
                        addCxReference($2.d.typeModifier->u.t, &$1.d->p, UsageThrown, noFileIndex, noFileIndex);
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
                if (options.server_operation == OLO_EXTRACT) {
                    addTrivialCxReference("TryCatch", TypeTryCatchMarker,StorageDefault,
                                            &$1.d->p, UsageTryCatchBegin);
                }
            }
        Block
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        generateInternalLabelReference($2.d, UsageDefined);
                    }
                }
            }
        TryCatches		{
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $6);
            if (options.server_operation == OLO_EXTRACT) {
                addTrivialCxReference("TryCatch", TypeTryCatchMarker,StorageDefault,
                                        &$1.d->p, UsageTryCatchEnd);
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
                        if ($3.d->bits.symbolType != TypeError) {
                            addNewSymbolDef($3.d, StorageAuto, s_javaStat->locals,
                                            UsageDefined);
                            if (options.server_operation == OLO_EXTRACT) {
                                assert($3.d->bits.symbolType==TypeDefault);
                                addCxReference($3.d->u.type->u.t, &$1.d->p, UsageCatched, noFileIndex, noFileIndex);
                            }
                        }
                    }
                }
            }
        Block Stop_block
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        generateInternalLabelReference($5.d, UsageDefined);
                    } else {
                        PropagateBoundaries($$, $1, $8);
                    }
                }
            }
    |	Catch '(' FormalParameter ')' ';'		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    if (options.server_operation == OLO_EXTRACT) {
                        assert($3.d->bits.symbolType==TypeDefault);
                        addCxReference($3.d->u.type->u.t, &$1.d->p, UsageCatched, noFileIndex, noFileIndex);
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
                    generateInternalLabelReference($2.d, UsageDefined);
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
                $$.d = $1.d;
                if (! SyntaxPassOnly()) {
                    s_javaCompletionLastPrimary = s_structRecordCompletionType = $$.d.typeModifier;
                } else {
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	ArrayCreationExpression				{
            if (regularPass()) {
                $$.d = $1.d;
                if (! SyntaxPassOnly()) {
                    s_javaCompletionLastPrimary = s_structRecordCompletionType = $$.d.typeModifier;
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
                    assert(s_javaStat && s_javaStat->thisType);
//fprintf(dumpOut,"this == %s\n",s_javaStat->thisType->u.t->linkName);
                    $$.d.typeModifier = s_javaStat->thisType;
                    addThisCxReferences(s_javaStat->classFileIndex, &$1.d->p);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = &$1.d->p;
                    javaCheckForStaticPrefixStart(&$1.d->p, &$1.d->p);
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	Name '.' THIS						{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    javaQualifiedThis($1.d, $3.d);
                    $$.d.typeModifier = javaClassNameType($1.d);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = javaGetNameStartingPosition($1.d);
                    javaCheckForStaticPrefixStart(&$3.d->p, javaGetNameStartingPosition($1.d));
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	PrimitiveType '.' CLASS				{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = &s_javaClassModifier;
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = $1.d.p;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	Name '.' CLASS						{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    Symbol *str;
                    javaClassifyToTypeName($1.d,UsageUsed, &str, USELESS_FQT_REFS_ALLOWED);
                    $$.d.typeModifier = &s_javaClassModifier;
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = javaGetNameStartingPosition($1.d);
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	ArrayType '.' CLASS					{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = &s_javaClassModifier;
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = $1.d.position;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	VOID '.' CLASS						{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = &s_javaClassModifier;
                    $$.d.reference = NULL;
                } else {
                    SetPrimitiveTypePos($$.d.position, $1.d);
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	'(' Expression ')'					{
            if (regularPass()) {
                $$.d = $2.d;
                if (SyntaxPassOnly()) {
                    $$.d.position = StackMemoryAlloc(Position);
                    *$$.d.position = $1.d;
                    PropagateBoundaries($$, $1, $3);
                    if (positionIsInTheSameFileAndBetween($$.b, s_cxRefPos, $$.e)
                        && s_spp[SPP_PARENTHESED_EXPRESSION_LPAR_POSITION].file == noFileIndex) {
                        s_spp[SPP_PARENTHESED_EXPRESSION_LPAR_POSITION] = $1.b;
                        s_spp[SPP_PARENTHESED_EXPRESSION_RPAR_POSITION] = $3.b;
                        s_spp[SPP_PARENTHESED_EXPRESSION_BEGIN_POSITION] = $2.b;
                        s_spp[SPP_PARENTHESED_EXPRESSION_END_POSITION] = $2.e;
                    }
                }
            }
        }
    |	ClassInstanceCreationExpression		/*& { $$.d = $1.d } &*/
        /* TODO: Here c-xref parsing/analysis stops, anything beyond
           this point does not register */
    |	FieldAccess							/*& { $$.d = $1.d } &*/
    |	MethodInvocation					/*& { $$.d = $1.d } &*/
    |	ArrayAccess							/*& { $$.d = $1.d } &*/
    |	CompletionTypeName '.'		{ assert(0); /* rule never used */ }
    ;

_erfs_:		{
            $$ = s_cp.erfsForParamsComplet;
        }
    ;

NestedConstructorInvocation
    :   Primary '.' New Name _erfs_
            {
                if (ComputingPossibleParameterCompletion()) {
                    TypeModifier *mm;
                    s_cp.erfsForParamsComplet = NULL;
                    if ($1.d.typeModifier->kind == TypeStruct) {
                        mm = javaNestedNewType($1.d.typeModifier->u.t, $3.d, $4.d);
                        if (mm->kind != TypeError) {
                            s_cp.erfsForParamsComplet = javaCrErfsForConstructorInvocation(mm->u.t, &($4.d->id.p));
                        }
                    }
                }
            }
        '(' ArgumentList_opt ')'				{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    s_cp.erfsForParamsComplet = $5;
                    if ($1.d.typeModifier->kind == TypeStruct) {
                        $$.d.typeModifier = javaNestedNewType($1.d.typeModifier->u.t, $3.d, $4.d);
                    } else {
                        $$.d.typeModifier = &s_errorModifier;
                    }
                    javaHandleDeclaratorParamPositions(&$4.d->id.p, &$7.d, $8.d.p, &$9.d);
                    assert($$.d.typeModifier);
                    $$.d.idList = $4.d;
                    if ($$.d.typeModifier->kind != TypeError) {
                        javaConstructorInvocation($$.d.typeModifier->u.t, &($4.d->id.p), $8.d.t);
                    }
                } else {
                    $$.d.pp = $1.d.position;
                    PropagateBoundaries($$, $1, $9);
                }
            }
        }
    |	Name '.' New Name _erfs_
            {
                if (ComputingPossibleParameterCompletion()) {
                    TypeModifier *mm;
                    s_cp.erfsForParamsComplet = NULL;
                    mm = javaNewAfterName($1.d, $3.d, $4.d);
                    if (mm->kind != TypeError) {
                        s_cp.erfsForParamsComplet = javaCrErfsForConstructorInvocation(mm->u.t, &($4.d->id.p));
                    }
                }
            }
        '(' ArgumentList_opt ')'				{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    s_cp.erfsForParamsComplet = $5;
                    $$.d.typeModifier = javaNewAfterName($1.d, $3.d, $4.d);
                    $$.d.idList = $4.d;
                    if ($$.d.typeModifier->kind != TypeError) {
                        javaConstructorInvocation($$.d.typeModifier->u.t, &($4.d->id.p), $8.d.t);
                    }
                } else {
                    $$.d.pp = javaGetNameStartingPosition($1.d);
                    javaHandleDeclaratorParamPositions(&$4.d->id.p, &$7.d, $8.d.p, &$9.d);
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
                javaClassifyAmbiguousName($1.d, NULL,&str,&expr,&rr, &lastUselessRef, USELESS_FQT_REFS_ALLOWED,
                                          CLASS_TO_TYPE,UsageUsed);
                $1.d->nameType = TypeStruct;
                ss = javaTypeSymbolUsage($1.d, AccessDefault);
                s_cp.erfsForParamsComplet = javaCrErfsForConstructorInvocation(ss, &($1.d->id.p));
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

                    s_cp.erfsForParamsComplet = $2;
                    lastUselessRef = NULL;
                    javaClassifyAmbiguousName($3.d, NULL,&str,&expr,&rr, &lastUselessRef, USELESS_FQT_REFS_ALLOWED,
                                              CLASS_TO_TYPE,UsageUsed);
                    $3.d->nameType = TypeStruct;
                    ss = javaTypeSymbolUsage($3.d, AccessDefault);
                    if (isANestedClass(ss)) {
                        if (javaIsInnerAndCanGetUnnamedEnclosingInstance(ss, &ei)) {
                            // before it was s_javaStat->classFileInd, but be more precise
                            // in reality you should keep both to discover references
                            // to original class from class nested in method.
                            addThisCxReferences(ei->u.s->classFile, &$1.d->p);
                            // I have removed following because it makes problems when
                            // expanding to FQT names, WHY IT WAS HERE ???
                            /*& addSpecialFieldReference(LINK_NAME_NOT_FQT_ITEM,StorageField,
                                         s_javaStat->classFileInd, &$1.d->p,
                                         UsageNotFQField); &*/
                        } else {
                            // here I should annulate class reference, as it is an error
                            // because can't get enclosing instance, this is sufficient to
                            // pull-up/down to report a problem
                            // BERK, It was completely wrong, because it is completely legal
                            // and annulating of reference makes class renaming wrong!
                            // Well, it is legal only for static nested classes.
                            // But for security reasons, I will keep it in comment,
                            /*& if (! (ss->bits.access&AccessStatic)) {
                                    if (rr!=NULL) rr->usg.base = s_noUsage;
                                } &*/
                        }
                    }
                    javaConstructorInvocation(ss, &($3.d->id.p), $5.d.t);
                    tt = javaTypeNameDefinition($3.d);
                    $$.d.typeModifier = tt->u.type;
                    $$.d.reference = NULL;
                } else {
                    javaHandleDeclaratorParamPositions(&$3.d->id.p, &$4.d, $5.d.p, &$6.d);
                    $$.d.position = &$1.d->p;
                    PropagateBoundaries($$, $1, $6);
                }
            }
        }
    |	New _erfs_ NewName '(' ArgumentList_opt ')'
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        Symbol *ss;
                        s_cp.erfsForParamsComplet = $2;
                        javaClassifyToTypeName($3.d,UsageUsed, &ss, USELESS_FQT_REFS_ALLOWED);
                        $<symbol>$ = javaTypeNameDefinition($3.d);
                        ss = javaTypeSymbolUsage($3.d, AccessDefault);
                        javaConstructorInvocation(ss, &($3.d->id.p), $5.d.t);
                    } else {
                        javaHandleDeclaratorParamPositions(&$3.d->id.p, &$4.d, $5.d.p, &$6.d);
                        // seems that there is no problem like in previous case,
                        // interfaces are never inner.
                    }
                } else {
                    Symbol *str, *cls;
                    jslClassifyAmbiguousTypeName($3.d, &str);
                    cls = jslTypeNameDefinition($3.d);
                    jslNewClassDefinitionBegin(&s_javaAnonymousClassName,
                                                AccessDefault, cls, CPOS_ST);
                }
            }
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        $<trail>$ = newClassDefinitionBegin(&s_javaAnonymousClassName,AccessDefault, $<symbol>6);
                    }
                }
            }
        ClassBody			{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    newClassDefinitionEnd($<trail>8);
                    assert($<symbol>7 && $<symbol>7->u.type);
                    $$.d.typeModifier = $<symbol>7->u.type;
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = &$1.d->p;
                    PropagateBoundaries($$, $1, $9);
                }
            } else {
                jslNewClassDefinitionEnd();
            }
        }
    |	NestedConstructorInvocation								{
            $$.d.typeModifier = $1.d.typeModifier;
            $$.d.position = $1.d.pp;
            $$.d.reference = NULL;
            PropagateBoundaries($$, $1, $1);
        }
    |	NestedConstructorInvocation
            {
                if (regularPass()) {
                    if (! SyntaxPassOnly()) {
                        $$.d.typeModifier = $1.d.typeModifier;
                        $$.d.position = $1.d.pp;
                        $$.d.reference = NULL;
                        if ($$.d.typeModifier->kind != TypeError) {
                            $<trail>$ = newClassDefinitionBegin(&s_javaAnonymousClassName, AccessDefault, $$.d.typeModifier->u.t);
                        } else {
                            $<trail>$ = newAnonClassDefinitionBegin(& $1.d.idList->id);
                        }
                    } else {
                        $$.d.position = $1.d.pp;
                    }
                } else {
                    jslNewAnonClassDefinitionBegin(&$1.d.idList->id);
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
            $$.d.t = NULL;
            $$.d.p = NULL;
            SetNullBoundariesFor($$);
        }
    | ArgumentList				/*& { $$.d = $1.d; } &*/
    ;

ArgumentList
    :   Expression									{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.t = newTypeModifierList($1.d.typeModifier);
                    if (s_cp.erfsForParamsComplet!=NULL) {
                        s_cp.erfsForParamsComplet->params = $$.d.t;
                    }
                } else {
                    $$.d.p = NULL;
                    appendPositionToList(&$$.d.p, &s_noPos);
                    PropagateBoundaries($$, $1, $1);
                }
            }
        }
    |	ArgumentList ',' Expression					{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    S_typeModifierList *p;
                    $$.d = $1.d;
                    p = newTypeModifierList($3.d.typeModifier);
                    LIST_APPEND(S_typeModifierList, $$.d.t, p);
                    if (s_cp.erfsForParamsComplet!=NULL) s_cp.erfsForParamsComplet->params = $$.d.t;
                } else {
                    appendPositionToList(&$$.d.p, &$2.d);
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
                    $$.d.typeModifier = newSimpleTypeModifier($3.d.u);
                    for(i=0; i<$4.d; i++)
                        prependTypeModifierWith($$.d.typeModifier, TypeArray);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = &$1.d->p;
                    PropagateBoundaries($$, $1, $5);
                    if ($$.e.file == noFileIndex) PropagateBoundaries($$, $$, $4);
                }
            }
        }
    |	New _erfs_ PrimitiveType Dims ArrayInitializer		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    int i;
                    $$.d.typeModifier = newSimpleTypeModifier($3.d.u);
                    for(i=0; i<$4.d; i++)
                        prependTypeModifierWith($$.d.typeModifier, TypeArray);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = &$1.d->p;
                    PropagateBoundaries($$, $1, $5);
                }
            }
        }
    |	New _erfs_ ClassOrInterfaceType DimExprs Dims_opt			{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    int i;
                    assert($3.d && $3.d->u.type);
                    $$.d.typeModifier = $3.d->u.type;
                    for(i=0; i<$4.d; i++)
                        prependTypeModifierWith($$.d.typeModifier, TypeArray);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = &$1.d->p;
                    PropagateBoundaries($$, $1, $5);
                    if ($$.e.file == noFileIndex) PropagateBoundaries($$, $$, $4);
                }
            }
        }
    |	New _erfs_ ClassOrInterfaceType Dims ArrayInitializer		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    int i;
                    assert($3.d && $3.d->u.type);
                    $$.d.typeModifier = $3.d->u.type;
                    for(i=0; i<$4.d; i++)
                        prependTypeModifierWith($$.d.typeModifier, TypeArray);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = &$1.d->p;
                    PropagateBoundaries($$, $1, $5);
                }
            }
        }
    ;


DimExprs
    :   DimExpr						{
            if (regularPass()) $$.d = 1;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |	DimExprs DimExpr			{
            if (regularPass()) $$.d = $1.d+1;
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
        if (regularPass()) $$.d = 0;
            SetNullBoundariesFor($$);
        }
    |	Dims						/*& { $$ = $1; } &*/
    ;

Dims
    :   '[' ']'						{
            if (regularPass()) $$.d = 1;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $2);
        }
    |	Dims '[' ']'				{
            if (regularPass()) $$.d = $1.d+1;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $3);
        }
    ;

FieldAccess
    :   Primary '.' Identifier					{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    Symbol *rec=NULL;
                    assert($1.d.typeModifier);
                    $$.d.reference = NULL;
                    $$.d.position = $1.d.position;
                    if ($1.d.typeModifier->kind == TypeStruct) {
                        javaLoadClassSymbolsFromFile($1.d.typeModifier->u.t);
                        $$.d.reference = findStructureFieldFromType($1.d.typeModifier, $3.d, &rec, CLASS_TO_EXPR);
                        assert(rec);
                        $$.d.typeModifier = rec->u.type;
                    } else if (s_language == LANG_JAVA) {
                        $$.d.typeModifier = javaArrayFieldAccess($3.d);
                    } else {
                        $$.d.typeModifier = &s_errorModifier;
                    }
                    assert($$.d.typeModifier);
                } else {
                    $$.d.position = $1.d.position;
                    javaCheckForPrimaryStart(&$3.d->p, $$.d.position);
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	Super '.' Identifier					{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    Symbol *ss,*rec=NULL;

                    $$.d.reference = NULL;
                    $$.d.position = &$1.d->p;
                    ss = javaCurrentSuperClass();
                    if (ss != &s_errorSymbol && ss->bits.symbolType!=TypeError) {
                        javaLoadClassSymbolsFromFile(ss);
                        $$.d.reference = findStrRecordFromSymbol(ss, $3.d, &rec,
                                                                 CLASS_TO_EXPR, $1.d);
                        assert(rec);
                        $$.d.typeModifier = rec->u.type;
                    } else {
                        $$.d.typeModifier = &s_errorModifier;
                    }
                    assert($$.d.typeModifier);
                } else {
                    $$.d.position = &$1.d->p;
                    javaCheckForPrimaryStart(&$3.d->p, $$.d.position);
                    javaCheckForStaticPrefixStart(&$3.d->p, $$.d.position);
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	Name '.' Super '.' Identifier					{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    Symbol *ss,*rec=NULL;

                    ss = javaQualifiedThis($1.d, $3.d);
                    if (ss != &s_errorSymbol && ss->bits.symbolType!=TypeError) {
                        javaLoadClassSymbolsFromFile(ss);
                        ss = javaGetSuperClass(ss);
                        $$.d.reference = findStrRecordFromSymbol(ss, $5.d, &rec,
                                                                 CLASS_TO_EXPR, NULL);
                        assert(rec);
                        $$.d.typeModifier = rec->u.type;
                    } else {
                        $$.d.typeModifier = &s_errorModifier;
                    }
                    $$.d.reference = NULL;
                    assert($$.d.typeModifier);
                } else {
                    $$.d.position = javaGetNameStartingPosition($1.d);
                    javaCheckForPrimaryStart(&$3.d->p, $$.d.position);
                    javaCheckForPrimaryStart(&$5.d->p, $$.d.position);
                    javaCheckForStaticPrefixStart(&$3.d->p, $$.d.position);
                    javaCheckForStaticPrefixStart(&$5.d->p, $$.d.position);
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
                s_cp.erfsForParamsComplet = javaCrErfsForMethodInvocationN($1.d);
            }
        } '(' ArgumentList_opt ')'					{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = javaMethodInvocationN($1.d,$5.d.t);
                    $$.d.reference = NULL;
                    s_cp.erfsForParamsComplet = $2;
                } else {
                    $$.d.position = javaGetNameStartingPosition($1.d);
                    javaCheckForPrimaryStartInNameList($1.d, $$.d.position);
                    javaCheckForStaticPrefixInNameList($1.d, $$.d.position);
                    javaHandleDeclaratorParamPositions(&$1.d->id.p, &$4.d, $5.d.p, &$6.d);
                    PropagateBoundaries($$, $1, $6);
                }
            }
        }
    |	Primary '.' Identifier _erfs_ {
            if (ComputingPossibleParameterCompletion()) {
                s_cp.erfsForParamsComplet = javaCrErfsForMethodInvocationT($1.d.typeModifier, $3.d);
            }
        } '(' ArgumentList_opt ')'	{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = javaMethodInvocationT($1.d.typeModifier, $3.d, $7.d.t);
                    $$.d.reference = NULL;
                    s_cp.erfsForParamsComplet = $4;
                } else {
                    $$.d.position = $1.d.position;
                    javaCheckForPrimaryStart(&$3.d->p, $$.d.position);
                    javaHandleDeclaratorParamPositions(&$3.d->p, &$6.d, $7.d.p, &$8.d);
                    PropagateBoundaries($$, $1, $8);
                }
            }
        }
    |	Super '.' Identifier _erfs_ {
            if (ComputingPossibleParameterCompletion()) {
                s_cp.erfsForParamsComplet = javaCrErfsForMethodInvocationS($1.d, $3.d);
            }
        } '(' ArgumentList_opt ')'	{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = javaMethodInvocationS($1.d, $3.d, $7.d.t);
                    $$.d.reference = NULL;
                    s_cp.erfsForParamsComplet = $4;
                } else {
                    $$.d.position = &$1.d->p;
                    javaCheckForPrimaryStart(&$1.d->p, $$.d.position);
                    javaCheckForPrimaryStart(&$3.d->p, $$.d.position);
                    javaHandleDeclaratorParamPositions(&$3.d->p, &$6.d, $7.d.p, &$8.d);
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
                    tt = javaClassifyToExpressionName($1.d, &($$.d.reference));
                    if (tt->kind==TypeArray) $$.d.typeModifier = tt->next;
                    else $$.d.typeModifier = &s_errorModifier;
                    assert($$.d.typeModifier);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = javaGetNameStartingPosition($1.d);
                    PropagateBoundaries($$, $1, $4);
                }
            }
        }
    |	PrimaryNoNewArray '[' Expression ']'		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    if ($1.d.typeModifier->kind==TypeArray) $$.d.typeModifier = $1.d.typeModifier->next;
                    else $$.d.typeModifier = &s_errorModifier;
                    assert($$.d.typeModifier);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = $1.d.position;
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
                    $$.d.typeModifier = javaClassifyToExpressionName($1.d, &($$.d.reference));
                } else {
                    $$.d.position = javaGetNameStartingPosition($1.d);
                    javaCheckForPrimaryStartInNameList($1.d, $$.d.position);
                    javaCheckForStaticPrefixInNameList($1.d, $$.d.position);
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
                    $$.d.typeModifier = javaCheckNumeric($1.d.typeModifier);
                    RESET_REFERENCE_USAGE($1.d.reference, UsageAddrUsed);
                } else {
                    $$.d.position = NULL_POS;
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    ;

PostDecrementExpression
    :   PostfixExpression DEC_OP		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = javaCheckNumeric($1.d.typeModifier);
                    RESET_REFERENCE_USAGE($1.d.reference, UsageAddrUsed);
                } else {
                    $$.d.position = NULL_POS;
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
                    $$.d.typeModifier = javaNumericPromotion($2.d.typeModifier);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    |	'-' UnaryExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = javaNumericPromotion($2.d.typeModifier);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
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
                    $$.d.typeModifier = javaCheckNumeric($2.d.typeModifier);
                    RESET_REFERENCE_USAGE($2.d.reference, UsageAddrUsed);
                } else {
                    $$.d.position = NULL_POS;
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    ;

PreDecrementExpression
    :   DEC_OP UnaryExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = javaCheckNumeric($2.d.typeModifier);
                    RESET_REFERENCE_USAGE($2.d.reference, UsageAddrUsed);
                } else {
                    $$.d.position = NULL_POS;
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
                    $$.d.typeModifier = javaNumericPromotion($2.d.typeModifier);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
                    PropagateBoundaries($$, $1, $2);
                }
            }
        }
    |	'!' UnaryExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    if ($2.d.typeModifier->kind == TypeBoolean) $$.d.typeModifier = $2.d.typeModifier;
                    else $$.d.typeModifier = &s_errorModifier;
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
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
                    assert($2.d.symbol && $2.d.symbol->u.type);
                    $$.d.typeModifier = $2.d.symbol->u.type;
                    $$.d.reference = NULL;
                    assert($$.d.typeModifier->kind == TypeArray);
                } else {
                    $$.d.position = NULL_POS;
                    PropagateBoundaries($$, $1, $4);
                    if (positionIsInTheSameFileAndBetween($4.b, s_cxRefPos, $4.e)
                        && s_spp[SPP_CAST_LPAR_POSITION].file == noFileIndex) {
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
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = newTypeModifier($2.d.u, NULL, NULL);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
                    PropagateBoundaries($$, $1, $4);
                    if (positionIsInTheSameFileAndBetween($4.b, s_cxRefPos, $4.e)
                        && s_spp[SPP_CAST_LPAR_POSITION].file == noFileIndex) {
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
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = $2.d.typeModifier;
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
                    PropagateBoundaries($$, $1, $4);
                    if (positionIsInTheSameFileAndBetween($4.b, s_cxRefPos, $4.e)
                        && s_spp[SPP_CAST_LPAR_POSITION].file == noFileIndex) {
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
                    $$.d.typeModifier = javaBinaryNumericPromotion($1.d.typeModifier,
                                                                   $3.d.typeModifier);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	MultiplicativeExpression '/' UnaryExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = javaBinaryNumericPromotion($1.d.typeModifier,
                                                                   $3.d.typeModifier);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	MultiplicativeExpression '%' UnaryExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = javaBinaryNumericPromotion($1.d.typeModifier,
                                                                   $3.d.typeModifier);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
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
                    st1 = javaIsStringType($1.d.typeModifier);
                    st2 = javaIsStringType($3.d.typeModifier);
                    if (st1 && st2) {
                        $$.d.typeModifier = $1.d.typeModifier;
                    } else if (st1) {
                        $$.d.typeModifier = $1.d.typeModifier;
                        // TODO add reference to 'toString' on $3.d
                    } else if (st2) {
                        $$.d.typeModifier = $3.d.typeModifier;
                        // TODO add reference to 'toString' on $1.d
                    } else {
                        $$.d.typeModifier = javaBinaryNumericPromotion($1.d.typeModifier,
                                                                       $3.d.typeModifier);
                    }
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	AdditiveExpression '-' MultiplicativeExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = javaBinaryNumericPromotion($1.d.typeModifier,
                                                                   $3.d.typeModifier);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
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
                    $$.d.typeModifier = javaNumericPromotion($1.d.typeModifier);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	ShiftExpression RIGHT_OP AdditiveExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = javaNumericPromotion($1.d.typeModifier);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	ShiftExpression URIGHT_OP AdditiveExpression	{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = javaNumericPromotion($1.d.typeModifier);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
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
                    $$.d.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	RelationalExpression '>' ShiftExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	RelationalExpression LE_OP ShiftExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	RelationalExpression GE_OP ShiftExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	RelationalExpression INSTANCEOF ReferenceType		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
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
                    $$.d.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
                    PropagateBoundaries($$, $1, $3);
                }
            }
        }
    |	EqualityExpression NE_OP RelationalExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
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
                    $$.d.typeModifier = javaBitwiseLogicalPromotion($1.d.typeModifier,
                                                                    $3.d.typeModifier);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
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
                    $$.d.typeModifier = javaBitwiseLogicalPromotion($1.d.typeModifier,
                                                                    $3.d.typeModifier);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
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
                    $$.d.typeModifier = javaBitwiseLogicalPromotion($1.d.typeModifier,
                                                                    $3.d.typeModifier);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
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
                    $$.d.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
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
                    $$.d.typeModifier = newSimpleTypeModifier(TypeBoolean);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
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
                    $$.d.typeModifier = javaConditionalPromotion($3.d.typeModifier,
                                                                 $5.d.typeModifier);
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
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
                    if ($1.d.typeModifier!=NULL && $1.d.typeModifier->kind == TypeStruct) {
                        s_cps.lastAssignementStruct = $1.d.typeModifier->u.t;
                    }
                }
                $$.d = $1.d;
            }
        } AssignmentOperator AssignmentExpression		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    s_cps.lastAssignementStruct = NULL;
                    if ($1.d.reference != NULL && options.server_operation == OLO_EXTRACT) {
                        Reference *rr;
                        rr = duplicateReference($1.d.reference);
                        $1.d.reference->usage = s_noUsage;
                        if ($3.d.u == '=') {
                            RESET_REFERENCE_USAGE(rr, UsageLvalUsed);
                        } else {
                            RESET_REFERENCE_USAGE(rr, UsageAddrUsed);
                        }
                    } else {
                        if ($3.d.u == '=') {
                            RESET_REFERENCE_USAGE($1.d.reference, UsageLvalUsed);
                        } else {
                            RESET_REFERENCE_USAGE($1.d.reference, UsageAddrUsed);
                        }
                        $$.d.typeModifier = $1.d.typeModifier;
                        $$.d.reference = NULL;
                        /*
                          fprintf(dumpOut,": java Type Dump\n"); fflush(dumpOut);
                          javaTypeDump($1.d.t);
                          fprintf(dumpOut,"\n = \n"); fflush(dumpOut);
                          javaTypeDump($4.d.t);
                          fprintf(dumpOut,"\ndump end\n"); fflush(dumpOut);
                        */
                    }
                } else {
                    PropagateBoundaries($$, $1, $4);
                    if (options.taskRegime == RegimeEditServer) {
                        if (positionIsInTheSameFileAndBetween($1.b, s_cxRefPos, $1.e)) {
                            s_spp[SPP_ASSIGNMENT_OPERATOR_POSITION] = $3.b;
                            s_spp[SPP_ASSIGNMENT_END_POSITION] = $4.e;
                        }
                    }
                    $$.d.position = NULL_POS;
                }
            }
        }
    ;

LeftHandSide
    :   Name					{
            if (regularPass()) {
                $$.d.position = javaGetNameStartingPosition($1.d);
                if (! SyntaxPassOnly()) {
                    Reference *rr;
                    $$.d.typeModifier = javaClassifyToExpressionName($1.d, &rr);
                    $$.d.reference = rr;
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
            if (regularPass()) $$.d.u = '=';
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   MUL_ASSIGN          {
            if (regularPass()) $$.d.u = MUL_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   DIV_ASSIGN			{
            if (regularPass()) $$.d.u = DIV_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   MOD_ASSIGN			{
            if (regularPass()) $$.d.u = MOD_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   ADD_ASSIGN			{
            if (regularPass()) $$.d.u = ADD_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   SUB_ASSIGN			{
            if (regularPass()) $$.d.u = SUB_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   LEFT_ASSIGN			{
            if (regularPass()) $$.d.u = LEFT_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   RIGHT_ASSIGN		{
            if (regularPass()) $$.d.u = RIGHT_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   URIGHT_ASSIGN		{
            if (regularPass()) $$.d.u = URIGHT_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   AND_ASSIGN			{
            if (regularPass()) $$.d.u = AND_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   XOR_ASSIGN			{
            if (regularPass()) $$.d.u = XOR_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    |   OR_ASSIGN			{
            if (regularPass()) $$.d.u = OR_ASSIGN;
            PropagateBoundariesIfRegularSyntaxPass($$, $1, $1);
        }
    ;

Expression
    :   AssignmentExpression
    |	error					{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    $$.d.typeModifier = &s_errorModifier;
                    $$.d.reference = NULL;
                } else {
                    $$.d.position = NULL_POS;
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
                    stackMemoryBlockStart();
                }
            }
        }
    ;

Stop_block:		{
            if (regularPass()) {
                if (! SyntaxPassOnly()) {
                    stackMemoryBlockFree();
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

static CompletionFunctionsTable spCompletionsTab[]  = {
    { COMPL_THIS_PACKAGE_SPECIAL,	javaCompleteThisPackageName },
    { COMPL_CLASS_DEF_NAME,			javaCompleteClassDefinitionNameSpecial },
    { COMPL_FULL_INHERITED_HEADER,	javaCompleteFullInheritedMethodHeader },
    { COMPL_CONSTRUCTOR_NAME0,		javaCompleteHintForConstructSingleName },
    { COMPL_VARIABLE_NAME_HINT,		javaHintVariableName },
    { COMPL_UP_FUN_PROFILE,			javaHintCompleteMethodParameters },
    {0,NULL}
};

static CompletionFunctionsTable completionsTab[]  = {
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
void makeJavaCompletions(char *s, int len, Position *pos) {
    int token, i;
    CompletionLine compLine;

    log_trace("completing \"%s\" in state %d", s, lastyystate);
    strncpy(s_completions.idToProcess, s, MAX_FUN_NAME_SIZE);
    s_completions.idToProcess[MAX_FUN_NAME_SIZE-1] = 0;
    initCompletions(&s_completions, len, *pos);

    /* special wizard completions */
    for (i=0;(token=spCompletionsTab[i].token)!=0; i++) {
        if (exists_valid_parser_action_on(token)) {
            log_trace("completing %d==%s in state %d", i, s_tokenName[token], lastyystate);
            (*spCompletionsTab[i].fun)(&s_completions);
            if (s_completions.abortFurtherCompletions)
                return;
        }
    }

    /* If there is a wizard completion, RETURN now */
    if (s_completions.alternativeIndex != 0 && options.server_operation != OLO_SEARCH) return;
    for (i=0;(token=completionsTab[i].token)!=0; i++) {
        if (exists_valid_parser_action_on(token)) {
            log_trace("completing %d==%s in state %d", i, s_tokenName[token], lastyystate);
            (*completionsTab[i].fun)(&s_completions);
            if (s_completions.abortFurtherCompletions)
                return;
        }
    }

    /* basic language tokens */
    for (token=0; token<LAST_TOKEN; token++) {
        if (token==IDENTIFIER) continue;
        if (exists_valid_parser_action_on(token)) {
            if (s_tokenName[token]!= NULL) {
                if (isalpha(*s_tokenName[token]) || *s_tokenName[token]=='_') {
                    fillCompletionLine(&compLine, s_tokenName[token], NULL, TypeKeyword,0, 0, NULL,NULL);
                    processName(s_tokenName[token], &compLine, 0, &s_completions);
                } else {
                    /*& fillCompletionLine(&compLine, s_tokenName[token], NULL, TypeToken,0, 0, NULL,NULL); &*/
                }
            }
        }
    }

    /* If the completion window is shown, or there is no completion,
       add also hints (should be optionally) */
    /*& if (s_completions.comPrefix[0]!=0  && (s_completions.alternativeIndex != 0) &*/
    /*&	&& options.cxrefs != OLO_SEARCH) return; &*/

    for (i=0;(token=hintCompletionsTab[i].token)!=0; i++) {
        if (exists_valid_parser_action_on(token)) {
            (*hintCompletionsTab[i].fun)(&s_completions);
            if (s_completions.abortFurtherCompletions)
                return;
        }
    }
}
