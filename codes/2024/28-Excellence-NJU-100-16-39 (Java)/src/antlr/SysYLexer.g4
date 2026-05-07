lexer grammar SysYLexer;

@header {
package antlr;
}

CONST : 'const';

INT : 'int';

FLOAT : 'float';

VOID : 'void';

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

IDENT : //以下划线或字母开头，仅包含下划线、英文字母大小写、阿拉伯数字
    (LETTER | '_') (LETTER | DIGIT | '_')*
   ;

fragment    //十进制整数常量
DECIMAL_CONST : '0' | [1-9] [0-9]*;

fragment    //八进制整数常量
OCTAL_CONST : '0' [0-7]+;

fragment    //十六进制整数常量
HEX_CONST :
           ('0x' | '0X') (DIGIT | [a-fA-F])+ ;

INTEGER_CONST : //十进制整数常量、八进制整数常量、十六进制整数常量
                (DECIMAL_CONST |
                OCTAL_CONST |
                HEX_CONST)
                ;

FLOAT_CONST    //十进制浮点数常量 |十六进制浮点数常量
           :    (DecimalFloatingConstant |
                HexadecimalFloatingConstant)
                ;

DecimalFloatingConstant
    :   FractionalConstant ExponentPart?
    |   DigitSequence ExponentPart
    ;

HexadecimalFloatingConstant
    :   HexadecimalPrefix HexadecimalFractionalConstant BinaryExponentPart
    |   HexadecimalPrefix HexadecimalDigitSequence BinaryExponentPart
    ;

fragment
FractionalConstant
    :   DigitSequence? '.' DigitSequence
    |   DigitSequence '.'
    ;

fragment
ExponentPart
    :   'e' Sign? DigitSequence
    |   'E' Sign? DigitSequence
    ;

fragment
Sign
    :   '+'
    |   '-'
    ;

fragment DigitSequence: DIGIT+;

fragment
HexadecimalDigit
    :   [0-9a-fA-F]
    ;

fragment
HexadecimalPrefix
    :   '0x'
    |   '0X'
    ;

fragment
HexadecimalFractionalConstant
    :   HexadecimalDigitSequence? '.' HexadecimalDigitSequence
    |   HexadecimalDigitSequence '.'
    ;

fragment
HexadecimalDigitSequence
    :   HexadecimalDigit+
    ;

fragment
BinaryExponentPart
    :   'P' Sign? DigitSequence
    |   'p' Sign? DigitSequence
    ;

WS :
    [ \r\n\t]+ -> skip
   ;

LINE_COMMENT:
    '//' .*? '\n' -> skip
   ;

MULTILINE_COMMENT:
   '/*' .*? '*/' -> skip
   ;
fragment DIGIT :  [0-9] ;
fragment LETTER : [a-zA-Z];