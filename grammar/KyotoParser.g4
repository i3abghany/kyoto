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
	| returnStatement
	| assignment;

expressionStatement: expression SEMICOLON;

expression:
	number #numberExpression
	| CHAR_LITERAL #charExpression
	| STRING_LITERAL #stringExpression
	| IDENTIFIER #identifierExpression
	| LPAREN expression RPAREN #parenthesizedExpression
	| MINUS expression #negationExpression
	| PLUS expression #positiveExpression
	| expression ASTERISK expression #multiplicationExpression
	| expression SLASH expression #divisionExpression
	| expression PERCENT expression #modulusExpression
	| expression PLUS expression #additionExpression
	| expression MINUS expression #subtractionExpression
	| expression comparisonOp expression #comparisonExpression
	;

unaryOp: MINUS | PLUS;

comparisonOp:
	LESS_THAN
	| LESS_THAN_OR_EQUAL
	| GREATER_THAN
	| GREATER_THAN_OR_EQUAL
	| EQUALS;
number: INTEGER | FLOAT | TRUE | FALSE;

fullDeclaration: VAR IDENTIFIER COLON type EQUAL expression SEMICOLON;

declaration: VAR IDENTIFIER COLON type SEMICOLON;

assignment: IDENTIFIER EQUAL expression SEMICOLON;

returnStatement: RETURN expression SEMICOLON;

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