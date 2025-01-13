parser grammar KyotoParser;

options {
	tokenVocab = KyotoLexer;
}

program: topLevel* EOF;

topLevel: functionDefinition | fullDeclaration | cdecl | classDefinition;

block: OPEN_BRACE statement* CLOSE_BRACE;

classDefinition: CLASS IDENTIFIER OPEN_BRACE classComponents CLOSE_BRACE;

classComponents: classComponent*;

classComponent:
    declaration
    | fullDeclaration
    | functionDefinition
    ;

statement:
	block
	| expressionStatement
	| declaration
	| fullDeclaration
	| ifStatement
	| forStatement
	| returnStatement;

expressionStatement: expression SEMICOLON;

expression:
	number											# numberExpression
	| CHAR_LITERAL									# charExpression
	| STRING_LITERAL								# stringExpression
	| IDENTIFIER									# identifierExpression
	| LPAREN expression RPAREN						# parenthesizedExpression
	| ASTERISK expression							# dereferenceExpression
	| AMPERSAND expression							# addressOfExpression
	| PLUS_PLUS expression							# prefixIncrementExpression
	| MINUS_MINUS expression						# prefixDecrementExpression
	| MINUS expression								# negationExpression
	| PLUS expression								# positiveExpression
	| IDENTIFIER LPAREN argumentList RPAREN			# functionCallExpression
	| expression ASTERISK expression				# multiplicationExpression
	| expression SLASH expression					# divisionExpression
	| expression PERCENT expression					# modulusExpression
	| expression PLUS expression					# additionExpression
	| expression MINUS expression					# subtractionExpression
	| expression EQUALS expression					# equalsExpression
	| expression NOT_EQUALS expression				# notEqualsExpression
	| expression LESS_THAN expression				# lessThanExpression
	| expression LESS_THAN_OR_EQUAL expression		# lessThanOrEqualExpression
	| expression GREATER_THAN expression			# greaterThanExpression
	| expression GREATER_THAN_OR_EQUAL expression	# greaterThanOrEqualExpression
	| expression LOGICAL_AND expression				# logicalAndExpression
	| expression LOGICAL_OR expression				# logicalOrExpression
	| expression logicalOp expression				# logicalExpression
	| <assoc = right> expression EQUAL expression	# assignmentExpression;

argumentList: expression (COMMA expression)* | /* empty */;

logicalOp: LOGICAL_AND | LOGICAL_OR;

number: INTEGER | FLOAT | TRUE | FALSE;

fullDeclaration:
	VAR IDENTIFIER COLON type EQUAL expression SEMICOLON	# regularDeclaration
	| VAR IDENTIFIER EQUAL expression SEMICOLON				# typelessDeclaration;

declaration: VAR IDENTIFIER COLON type SEMICOLON;

returnStatement: RETURN expression? SEMICOLON;

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

forUpdate: expression |;

functionDefinition:
	FN IDENTIFIER LPAREN parameterList RPAREN type? block;

cdecl:
	CDECL FN IDENTIFIER LPAREN parameterList RPAREN type SEMICOLON;

parameterList:
	parameter (COMMA parameter)* (COMMA ELLIPSIS)?
	| ELLIPSIS
	| /* empty */;

parameter: IDENTIFIER COLON type;

pointerSuffix: ASTERISK;

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
	| VOID
	| type pointerSuffix+;