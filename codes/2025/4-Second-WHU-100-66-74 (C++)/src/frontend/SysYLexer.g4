lexer grammar SysYLexer;

CONST : 'const';

INT : 'int';

VOID : 'void';

FLOAT : 'float';

IF : 'if';

ELSE : 'else';

WHILE : 'while';

BREAK : 'break';

CONTINUE : 'continue';

RETURN : 'return';

PLUS : '+';

MINUS : '-';

MUL : '*';

DIV : '/';

MOD : '%';

ASSIGN : '=';

EQ : '==';

NEQ : '!=';

LT : '<';

GT : '>';

LE : '<=';

GE : '>=';

NOT : '!';

AND : '&&';

OR : '||';

L_PAREN : '(';

R_PAREN : ')';

L_BRACE : '{';

R_BRACE : '}';

L_BRACKT : '[';

R_BRACKT : ']';

COMMA : ',';

SEMICOLON : ';';
//标识符
IDENT : ('_'|[a-zA-Z])('_'|[a-zA-Z0-9])*
   ;

//整型常量 依次为十进制数，8进制数和16进制数
INTEGER_CONST : (([1-9][0-9]*)|'0')|('0'[0-7]+)|(('0x'|'0X')([0-9a-fA-F]+))
   ;

//浮点常量
FLOAT_CONST : DECIMAL_FLOAT_CONST | HEXADECIMAL_FLOAT_CONST;
DECIMAL_FLOAT_CONST : (FRACTIONAL_CONST EXPONENT_PART?) | (DIGIT+ EXPONENT_PART);
HEXADECIMAL_FLOAT_CONST : ('0x'|'0X') (HEXADECIMAL_FRACTIONAL_CONST | HEXADECIMAL_DIGIT_SEQUENCE) BINARY_EXPONENT_PART;
FRACTIONAL_CONST : (DIGIT* '.' DIGIT+) | (DIGIT+ '.');
EXPONENT_PART: ('e'|'E') SIGN? DIGIT+;
DIGIT : [0-9];
SIGN : '+'|'-';
HEXADECIMAL_FRACTIONAL_CONST : (HEXADECIMAL_DIGIT_SEQUENCE? '.' HEXADECIMAL_DIGIT_SEQUENCE) | (HEXADECIMAL_DIGIT_SEQUENCE '.');
BINARY_EXPONENT_PART : ('p'|'P') SIGN? DIGIT+;
HEXADECIMAL_DIGIT_SEQUENCE : [0-9a-fA-F]+;

//换行
WS
   : [ \r\n\t]+->skip
   ;

//单行注释
LINE_COMMENT
   : '//' .*? '\n'->skip
   ;

//多行注释
MULTILINE_COMMENT
   : '/*' .*? '*/'->skip
   ;

