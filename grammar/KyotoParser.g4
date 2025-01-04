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
	| ifStatement
	| returnStatement
	;

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
	| expression LESS_THAN expression #lessThanExpression
	| expression LESS_THAN_OR_EQUAL expression #lessThanOrEqualExpression
	| expression GREATER_THAN expression #greaterThanExpression
	| expression GREATER_THAN_OR_EQUAL expression #greaterThanOrEqualExpression

	| expression EQUALS expression #equalsExpression
	| expression NOT_EQUALS expression #notEqualsExpression
	| expression LESS_THAN expression #lessThanExpression
	| expression LESS_THAN_OR_EQUAL expression #lessThanOrEqualExpression
	| expression GREATER_THAN expression #greaterThanExpression
	| expression GREATER_THAN_OR_EQUAL expression #greaterThanOrEqualExpression

	| expression LOGICAL_AND expression #logicalAndExpression
	| expression LOGICAL_OR expression #logicalOrExpression

	| expression logicalOp expression #logicalExpression
	| <assoc=right> expression EQUAL expression #assignmentExpression
	;

logicalOp:
	LOGICAL_AND
	| LOGICAL_OR;

number: INTEGER | FLOAT | TRUE | FALSE;

fullDeclaration: VAR IDENTIFIER COLON type EQUAL expression SEMICOLON;

declaration: VAR IDENTIFIER COLON type SEMICOLON;

returnStatement: RETURN expression SEMICOLON;

ifStatement: IF LPAREN expression RPAREN block (ELSE block)?
			| IF LPAREN expression RPAREN block (ELSE ifStatement)?;

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
	| F32
	| F64
	| STRING
	| VOID;