lexer grammar SysYLexer;

options {
	language = Cpp;
}

CONST: 'const';
INT: 'int';
FLOAT: 'float';
VOID: 'void';
IF: 'if';
ELSE: 'else';
WHILE: 'while';
BREAK: 'break';
CONTINUE: 'continue';
RETURN: 'return';
PLUS: '+';
MINUS: '-';
MUL: '*';
DIV: '/';
MOD: '%';
ASSIGN: '=';
EQ: '==';
NEQ: '!=';
LT: '<';
GT: '>';
LE: '<=';
GE: '>=';
NOT: '!';
AND: '&&';
OR: '||';
L_PAREN: '(';
R_PAREN: ')';
L_BRACE: '{';
R_BRACE: '}';
L_BRACKT: '[';
R_BRACKT: ']';
COMMA: ',';
SEMICOLON: ';';

IDENT: [_a-zA-Z][_a-zA-Z0-9]*; //以下划线或字母开头，仅包含下划线、英文字母大小写、阿拉伯数字

FLOAT_CONST:
	// 十进制
	FRACTION_CONST (EXPONENT_CONST)?
	| [0-9]+ EXPONENT_CONST
	// 十六进制
	| '0' [xX] (Hexadecimal_digit)? '.' Hexadecimal_digit Binary_exponent
	| '0' [xX] Hexadecimal_digit '.' Binary_exponent
	| '0' [xX] Hexadecimal_digit Binary_exponent;

INTEGER_CONST:
	('0' | [1-9] DIGIT*)
	| '0' [0-7]*
	| '0' [xX][0-9a-fA-F]*; //数字常量，包含十进制数，0开头的八进制数，0x或0X开头的十六进制数

fragment FRACTION_CONST: DIGIT* '.' DIGIT+ | DIGIT+ '.';

fragment EXPONENT_CONST: [eE] [+-]? DIGIT+;

fragment Hexadecimal_digit: [0-9a-fA-F]+;

fragment Binary_exponent: [pP] [+-] DIGIT+;

WS: [ \r\n\t]+ -> skip;

LINE_COMMENT: '//' .*? '\n' -> skip;

MULTILINE_COMMENT: '/*' .*? '*/' -> skip;

fragment DIGIT: '0' ..'9';