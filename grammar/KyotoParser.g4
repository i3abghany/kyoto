parser grammar KyotoParser;

options {
	tokenVocab = KyotoLexer;
}

program: topLevel* EOF;

topLevel:
	functionDefinition
	| fullDeclaration
	| cdecl
	| classDefinition;

block: OPEN_BRACE statement* CLOSE_BRACE;

classDefinition:
	CLASS IDENTIFIER (COLON IDENTIFIER)? OPEN_BRACE classComponents CLOSE_BRACE;

classComponents: classComponent*;

classComponent:
	declaration
	| constructorDefinition
	| functionDefinition;

constructorDefinition:
	CONSTRUCTOR LPAREN parameterList RPAREN block;

statement:
	block
	| expressionStatement
	| declaration
	| fullDeclaration
	| ifStatement
	| forStatement
	| returnStatement
	| freeStatement;

expressionStatement: expression SEMICOLON;

expression:
	number														# numberExpression
	| CHAR_LITERAL												# charExpression
	| STRING_LITERAL											# stringExpression
	| IDENTIFIER												# identifierExpression
	| LPAREN expression RPAREN									# parenthesizedExpression
	| NEW type LPAREN expressionList RPAREN						# newExpression
	| NEW type OPEN_BRACKET expression CLOSE_BRACKET			# newArrayExpression
	| LPAREN type RPAREN expression								# castExpression
	| SIZEOF LPAREN (expression | type) RPAREN					# sizeofExpression
	| MATCH expression OPEN_BRACE matchCase+ CLOSE_BRACE		# matchExpression
	| type OPEN_BRACE expressionList CLOSE_BRACE				# arrayExpression
	| ASTERISK expression										# dereferenceExpression
	| expression OPEN_BRACKET expression CLOSE_BRACKET			# arrayIndexExpression
	| expression DOT IDENTIFIER									# memberAccessExpression
	| expression DOT IDENTIFIER LPAREN expressionList RPAREN	# methodCallExpression
	| AMPERSAND expression										# addressOfExpression
	| PLUS_PLUS expression										# prefixIncrementExpression
	| MINUS_MINUS expression									# prefixDecrementExpression
	| MINUS expression											# negationExpression
	| PLUS expression											# positiveExpression
	| IDENTIFIER LPAREN expressionList RPAREN					# functionCallExpression
	| expression ASTERISK expression							# multiplicationExpression
	| expression SLASH expression								# divisionExpression
	| expression PERCENT expression								# modulusExpression
	| expression PLUS expression								# additionExpression
	| expression MINUS expression								# subtractionExpression
	| expression EQUALS expression								# equalsExpression
	| expression NOT_EQUALS expression							# notEqualsExpression
	| expression LESS_THAN expression							# lessThanExpression
	| expression LESS_THAN_OR_EQUAL expression					# lessThanOrEqualExpression
	| expression GREATER_THAN expression						# greaterThanExpression
	| expression GREATER_THAN_OR_EQUAL expression				# greaterThanOrEqualExpression
	| expression LOGICAL_AND expression							# logicalAndExpression
	| expression LOGICAL_OR expression							# logicalOrExpression
	| <assoc = right> expression EQUAL expression				# assignmentExpression;

number: INTEGER | FLOAT | TRUE | FALSE;

matchCase: (expression | DEFAULT) ARROW expression COMMA;

expressionList: expression (COMMA expression)* | /* empty */;

fullDeclaration:
	VAR IDENTIFIER COLON type EQUAL expression SEMICOLON	# regularDeclaration
	| VAR IDENTIFIER EQUAL expression SEMICOLON				# typelessDeclaration;

declaration: VAR IDENTIFIER COLON type SEMICOLON;

returnStatement: RETURN expression? SEMICOLON;

freeStatement: FREE expression SEMICOLON;

ifStatement:
	IF LPAREN expression RPAREN block elseIfElseStatement;

elseIfElseStatement:
	ELSE IF LPAREN expression RPAREN block elseIfElseStatement	# elseIfStatement
	| optionalElseStatement										# elseStatement;

optionalElseStatement: ELSE block | /* empty */;

forStatement:
	FOR LPAREN forInit forCondition forUpdate RPAREN block;

forInit: fullDeclaration | expressionStatement | SEMICOLON;

forCondition: expressionStatement | SEMICOLON;

forUpdate: expression | /* empty */;

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
	BOOLEAN										# boolType
	| CHAR										# charType
	| I8										# i8Type
	| I16										# i16Type
	| I32										# i32Type
	| I64										# i64Type
	| F32										# f32Type
	| F64										# f64Type
	| STRING									# strType
	| VOID										# voidType
	| IDENTIFIER								# classType
	| type OPEN_BRACKET CLOSE_BRACKET			# arrayType
	| type ASTERISK+							# pointerType;