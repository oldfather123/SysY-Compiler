grammar SysY;

/*----------------------------------------------------------------*/
/* Lexer rules                                                    */
/*----------------------------------------------------------------*/

INT: 'int';
FLOAT: 'float';
VOID: 'void';

fragment IdentifierNondigit: [a-zA-Z_];
fragment IdentifierDigit: [0-9];

Ident: IdentifierNondigit (IdentifierDigit | IdentifierNondigit)*;

fragment Digit:[0-9];
fragment NoneZeroDigit: [1-9];
fragment OctalDigit: [0-7];
fragment HexadecimalDigit: [0-9a-fA-F];

fragment HexadecimalPrefix: '0x' | '0X';

IntConst: DecimalConst | OctalConst | HexadecimalConst;
DecimalConst: NoneZeroDigit Digit*;
OctalConst: '0' | '0' OctalDigit+;
HexadecimalConst: HexadecimalPrefix HexadecimalDigit+;

fragment Sign: '+' | '-';
fragment FractionalConst: DigitSequence? '.' DigitSequence
        | DigitSequence '.';
fragment ExponentPart: 'e' Sign? DigitSequence
        | 'E' Sign? DigitSequence;
fragment DigitSequence: Digit+;
fragment HexadecimalFractionalConst: HexadecimalDigitSequence? '.' HexadecimalDigitSequence
        | HexadecimalDigitSequence '.';
fragment BinaryExponentPart: 'P' Sign? DigitSequence
        | 'p' Sign? DigitSequence;
fragment HexadecimalDigitSequence: HexadecimalDigit+;

FloatConst: DecimalFloatConst | HexadecimalFloatConst;
DecimalFloatConst: FractionalConst ExponentPart?
        | DigitSequence ExponentPart;
HexadecimalFloatConst: HexadecimalPrefix HexadecimalFractionalConst BinaryExponentPart
        | HexadecimalPrefix HexadecimalDigitSequence BinaryExponentPart;

String: '"' (ESC | .)*? '"';

fragment ESC: '\\"' | '\\\\';

WS: [ \t\r\n] -> skip;
LINE_COMMENT: '//' .*? '\r'? '\n' -> skip;
COMMENT: '/*' .*? '*/' -> skip;


/*----------------------------------------------------------------*/
/* Syntax rules                                                    */
/*----------------------------------------------------------------*/

compUnit: (globalDecl | funcDef)+;
globalDecl: constDecl                           # globalConstDecl
        | varDecl                               # globalVarDecl;
decl: constDecl | varDecl;
constDecl: 'const' bType constDef (',' constDef)* ';';
bType: INT | FLOAT;
constDef: Ident ('[' constExp ']')* '=' constInitVal;
constInitVal: constExp                                          # constScalarInitValue
        | '{' (constInitVal (',' constInitVal)*)? '}'           # constArrayInitValue;
varDecl: bType varDef (',' varDef)* ';';
varDef: Ident ('[' constExp ']')* ('=' initVal)?;
initVal: exp                                                    # scalarInitValue
        |'{'(initVal (',' initVal)*)?'}'                        # arrayInitValue;
funcDef: funcType Ident '(' funcFParams? ')' block;
funcType: VOID | INT | FLOAT;
funcFParams: funcFParam (',' funcFParam)*;
funcFParamIndices: '['']'('[' exp ']')*;
funcFParam: bType Ident funcFParamIndices?;
block: '{' blockItem* '}';
blockItem: decl | stmt;
stmt: lVal '=' exp';'                           # assignStmt
        | exp? ';'                              # expStmt
        | block                                 # blockStmt
        | 'if' '(' cond ')' stmt ('else' stmt)? # ifStmt
        | 'while' '(' cond ')' stmt             # whileStmt
        | 'break' ';'                           # breakStmt
        | 'continue' ';'                        # continueStmt
        | 'return' exp? ';'                     # returnStmt;
exp: addExp;
cond: lOrExp;
lVal: Ident ('[' exp ']')*;
primaryExp: '(' exp ')'                                          
        | lVal                                                                  
        | number;                                    
number: IntConst | FloatConst;
callExp: Ident '(' funcRParams? ')';
unaryExp: primaryExp                            
        | callExp
        | unaryOp unaryExp;                     
unaryOp: '+' | '-' | '!';
mulOp: '*' | '/' | '%';
addOp: '+' | '-' ;
relOp: '<' | '>' | '<=' | '>=';
eqOp: '==' | '!=';
funcRParams: exp (',' exp)*;
mulExp: unaryExp (mulOp unaryExp)*;
addExp: mulExp (addOp mulExp)*;
relExp: addExp (relOp addExp)*;
eqExp: relExp (eqOp relExp)*;
lAndExp: eqExp ('&&' eqExp)*;
lOrExp: lAndExp ('||' lAndExp)*;
constExp: addExp;

