lexer grammar SysYLexer;

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

LPAREN: '(';
RPAREN: ')';
LBRACE: '{';
RBRACE: '}';
LBRACK: '[';
RBRACK: ']';
COMMA: ',';
SEMICOLON: ';';

FLOAT_CONST
    : DecimalFloatConst
    | HexadecimalFloatConst
    ;

INT_CONST
    : DecimalConst
    | OctalConst
    | HexadecimalConst
    ;

STRING_CONST
    : '"' (EscapeSequence | ~["\\\r\n])* '"'
    ;

IDENT
    : IdentifierNondigit (IdentifierNondigit | Digit)*
    ;

LINE_COMMENT
    : '//' ~[\r\n]* -> skip
    ;

BLOCK_COMMENT
    : '/*' .*? '*/' -> skip
    ;

WS
    : [ \t\r\n]+ -> skip
    ;

fragment DecimalConst
    : NonzeroDigit Digit*
    ;

fragment OctalConst
    : '0' OctalDigit*
    ;

fragment HexadecimalConst
    : HexadecimalPrefix HexadecimalDigit+
    ;

fragment DecimalFloatConst
    : FractionalConst ExponentPart?
    | DigitSequence ExponentPart
    ;

fragment HexadecimalFloatConst
    : HexadecimalPrefix (HexadecimalFractionalConst | HexadecimalDigitSequence) BinaryExponentPart
    ;

fragment FractionalConst
    : DigitSequence? '.' DigitSequence
    | DigitSequence '.'
    ;

fragment ExponentPart
    : [eE] Sign? DigitSequence
    ;

fragment HexadecimalFractionalConst
    : HexadecimalDigitSequence? '.' HexadecimalDigitSequence
    | HexadecimalDigitSequence '.'
    ;

fragment BinaryExponentPart
    : [pP] Sign? DigitSequence
    ;

fragment EscapeSequence
    : '\\' (['"?\\abfnrtv] | OctalDigit OctalDigit? OctalDigit? | 'x' HexadecimalDigit+)
    ;

fragment HexadecimalPrefix
    : '0x'
    | '0X'
    ;

fragment Sign
    : '+'
    | '-'
    ;

fragment IdentifierNondigit
    : [A-Za-z_]
    ;

fragment DigitSequence
    : Digit+
    ;

fragment HexadecimalDigitSequence
    : HexadecimalDigit+
    ;

fragment NonzeroDigit
    : [1-9]
    ;

fragment Digit
    : [0-9]
    ;

fragment OctalDigit
    : [0-7]
    ;

fragment HexadecimalDigit
    : [0-9a-fA-F]
    ;
