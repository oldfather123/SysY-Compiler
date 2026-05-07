grammar SysY;
// 语法
compUnit : (decl|funcDef)+; // compUnit : [CompUnit](Decl|FuncDef);
decl : constDecl | varDecl;
constDecl : 'const' bType constDef (','constDef)* ';';
bType : 'int' | 'float';
constDef : IDENT ('['constExp']')* '=' constInitVal;
constInitVal : constExp
                | '{' (constInitVal (',' constInitVal)*)? '}';
varDecl : bType varDef (',' varDef)* ';';
varDef : IDENT ('[' constExp ']')*
        | IDENT ('[' constExp ']')* '=' initVal;
initVal : exp | '{' (initVal (',' initVal)* )? '}';
funcDef : funcType IDENT '(' (funcFParams)? ')' block;
funcType : 'void' | 'int' | 'float';
funcFParams : funcFParam (',' funcFParam)*;
funcFParam : bType IDENT ('['']'('['exp']')*)?;
block : '{' (blockItem)* '}';
blockItem : decl | stmt;
stmt : lVal '=' exp ';' | (exp)?';' | block
        | 'if' '(' cond ')' stmt ('else' stmt)?
        | 'while' '(' cond ')' stmt
        | 'break' ';' | 'continue' ';'
        | 'return' (exp)? ';';
exp : addExp;
cond : lOrExp;
lVal : IDENT ('[' exp ']')*;
primaryExp : '(' exp ')' | lVal | number;
number : INTCONST | FLOATCONST;
unaryExp : primaryExp | IDENT '(' (funcRParams)? ')'
            | unaryOp unaryExp;
unaryOp : '+' | '-' | '!';
funcRParams : exp (','exp)*
                | STRING (','exp)*;
 mulExp : unaryExp | mulExp ('*' | '/' | '%') unaryExp;
//mulExp : (unaryExp ('*' | '/' | '%'))* unaryExp;
// addExp : mulExp | addExp ('+' | '-') mulExp;
addExp : (mulExp ('+' | '-'))* mulExp;
 relExp : addExp | relExp ('<' | '>' | '<=' | '>=') addExp;
//relExp : (addExp ('<' | '>' | '<=' | '>='))* addExp;
 eqExp : relExp | eqExp ('==' | '!=') relExp;
//eqExp : (relExp ('==' | '!='))* relExp;
 lAndExp : eqExp | lAndExp '&&' eqExp;
//lAndExp : (eqExp '&&')* eqExp;
 lOrExp : lAndExp | lOrExp '||' lAndExp;
//lOrExp : (lAndExp '||')* lAndExp;
constExp : addExp;

// 词法
IDENT : [_a-zA-Z]([_a-zA-Z] | [_a-zA-Z0-9])*;
INTCONST : DECIMAL | OCTAL | HEXADECIMAL;
fragment DECIMAL : [1-9][0-9]*;
fragment OCTAL : '0'[0-7]*;
fragment HexadecimalPrefix : '0x' | '0X';
fragment HEXADECIMAL : (HexadecimalPrefix) HEX_DIGIT_SEQ;
FLOATCONST : DECIMAL_F | HEXADECIMAL_F;
fragment DECIMAL_F : (FRAC EXPO_PART?) | DIGIT_SEQ EXPO_PART; // 忽略suffix
fragment FRAC : DIGIT_SEQ? '.' DIGIT_SEQ | DIGIT_SEQ '.';
fragment EXPO_PART : ('e' | 'E') ('+' | '-')? DIGIT_SEQ;
fragment DIGIT_SEQ : [0-9]+;
fragment HEXADECIMAL_F : (HexadecimalPrefix) (HEX_FRAC BIN_EXPO_PART | HEX_DIGIT_SEQ BIN_EXPO_PART);
fragment HEX_FRAC : HEX_DIGIT_SEQ? '.' HEX_DIGIT_SEQ | HEX_DIGIT_SEQ '.';
fragment HEX_DIGIT_SEQ : [0-9a-fA-F]+;
fragment BIN_EXPO_PART : ('p' | 'P') ('+' | '-')? DIGIT_SEQ;
STRING : '"'.*?'"';

LINE_COMMENT : '//' .*? '\r'? '\n' -> skip;
COMMENT : '/*' .*? '*/' -> skip;
WS : [ \t\r\n]+ -> skip;
