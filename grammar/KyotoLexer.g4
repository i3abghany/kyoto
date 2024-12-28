lexer grammar KyotoLexer;

FLOAT: DIGIT+ DOT DIGIT+;
INTEGER: DIGIT+;
DIGIT: [0-9];
TRUE: 'true';
FALSE: 'false';

SEMICOLON: ';';

LESS_THAN: '<';
LESS_THAN_OR_EQUAL: '<=';
GREATER_THAN: '>';
GREATER_THAN_OR_EQUAL: '>=';
EQUALS: '==';

PLUS: '+';
MINUS: '-';
ASTERISK: '*';
SLASH: '/';
PERCENT: '%';
EQUAL: '=';

STRING_LITERAL: '"' (~["\\\r\n] | '\\' .)* '"';
CHAR_LITERAL: '\'' (~['\\\r\n] | '\\' .) '\'';

LPAREN: '(';
RPAREN: ')';
OPEN_BRACE: '{';
CLOSE_BRACE: '}';
OPEN_BRACKET: '[';
CLOSE_BRACKET: ']';

COMMA: ',';
COLON: ':';
DOT: '.';

LINE_COMMENT: '//' ~[\r\n]*;
BLOCK_COMMENT: '/*' .*? '*/';

WS: [ \t\r\n]+ -> skip;

BOOLEAN: 'bool';
CHAR: 'char';
I8: 'i8';
I16: 'i16';
I32: 'i32';
I64: 'i64';
U8: 'u8';
U16: 'u16';
U32: 'u32';
U64: 'u64';
F32: 'f32';
F64: 'f64';
STRING: 'string';
VOID: 'void';

FN: 'fn';
CDECL: 'cdecl';
ELLIPSIS: '...';

// This has to be defined after all the keywords because it will match all of
// them if defined first.
IDENTIFIER: LETTER (LETTER | [0-9])*;
fragment LETTER: [a-zA-Z\u0080-\u{10FFFF}];