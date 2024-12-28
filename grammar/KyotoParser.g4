parser grammar KyotoParser;

options {
	tokenVocab = KyotoLexer;
}

program: topLevel* EOF;

topLevel: functionDefinition | fullDeclaration | cdecl;

block: OPEN_BRACE statement* CLOSE_BRACE;

statement:
	expressionStatement
	| declaration
	| fullDeclaration
	| assignment;

expressionStatement: expression SEMICOLON;

expression:
	number
	| unaryOp expression
	| expression multOp expression
	| expression addOp expression
	| expression comparisonOp expression;

unaryOp: MINUS | PLUS;

multOp: ASTERISK | SLASH | PERCENT;

addOp: PLUS | MINUS;

comparisonOp:
	LESS_THAN
	| LESS_THAN_OR_EQUAL
	| GREATER_THAN
	| GREATER_THAN_OR_EQUAL
	| EQUALS;

primaryExpression:
	number
	| CHAR_LITERAL
	| STRING_LITERAL
	| LPAREN expression RPAREN;

number: INTEGER | FLOAT | TRUE | FALSE;

fullDeclaration: type IDENTIFIER EQUAL expression SEMICOLON;

declaration: type IDENTIFIER SEMICOLON;

assignment: IDENTIFIER EQUAL expression SEMICOLON;

functionDefinition:
	FN IDENTIFIER LPAREN parameterList RPAREN type? block;

cdecl:
	CDECL FN IDENTIFIER LPAREN parameterList RPAREN type SEMICOLON;

parameterList:
	parameter (COMMA parameter)* (COMMA ELLIPSIS)?
	| ELLIPSIS
	| /* empty */;

parameter: IDENTIFIER COLON type;

type:
	BOOLEAN
	| CHAR
	| I8
	| I16
	| I32
	| I64
	| U8
	| U16
	| U32
	| U64
	| F32
	| F64
	| STRING
	| VOID;