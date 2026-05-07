grammar SysYParser;

program:compUnit;
/*编译单元*/
compUnit: ( decl | funcDef)* EOF;
/*声明*/
decl: constDecl | varDecl;
/*常量声明*/
constDecl: 'const' bType constDef ( ',' constDef )* ';';
/*基本类型*/
bType: 'int' | 'float';
/*常数定义*/
constDef
    : Identifier  '=' constInitVal                        #constDefSingle
    | initLVal '=' constInitVal   #constDefArray
    ;
/*常量初值*/
constInitVal
    : constExp                                            #constInitValSingle
    | '{'( constInitVal ( ',' constInitVal )* )? '}'      #constInitValArray
    ;
/*变量声明*/
varDecl: bType varDef ( ',' varDef )* ';';
/*变量定义*/
varDef
    : Identifier                                          #varDefSingle
    | initLVal                    #varDefArray
    | Identifier '=' initVal                              #varDefSingleInitVal
    | initLVal '=' initVal        #varDefArrayInitVal
    ;
initLVal:Identifier ('['constExp']')+;
/*变量初值*/
initVal
    : exp                                                 #initValSingle
    | '{' ( initVal ( ',' initVal )* )? '}'               #initValArray
    ;
/*函数定义*/
funcDef: funcType Identifier '(' funcFParams? ')' block;
/*函数类型*/
funcType: 'void' | 'int' | 'float';
/*函数形参表*/
funcFParams: funcFParam ( ',' funcFParam )*;
/*函数形参*/
funcFParam
    : bType Identifier                                    #funcFParamSingle
    | bType Identifier '['  ']' ( '[' exp ']' )*          #funcFParamArray
    ;

/*语句块*/
block: '{' ( blockItem )* '}';
/*语句块项*/
blockItem: decl | stmt;
/*语句*/
stmt
    : lVal '=' exp ';'                                    #stmtAssign
    | exp? ';'                                            #stmtExp
    | block                                               #stmtBlock
    | 'if' '(' cond ')' stmt ( 'else' stmt )?             #stmtCond
    | 'while' '(' cond ')' stmt                           #stmtWhile
    | 'break' ';'                                         #stmtBreak
    | 'continue' ';'                                      #stmtContinue
    | 'return' exp? ';'                                   #stmtReturn
    ;
/*表达式*/
exp: addExp; /*注：sysY 表达式是 int float型表达式*/
/*条件表达式*/
cond: lOrExp;
/*左值表达式*/
lVal: Identifier                                          #lValSingle
    | Identifier ('[' exp ']')+                           #lValArray
    ;
/*基本表达式*/
primaryExp
    : '(' exp ')'                                         #primaryExpParen
    | lVal                                                #primaryExpLVal
    | number                                              #primaryExpNumber
    ;
/*数值*/
number
    : intConst
    | floatConst
    ;
/*一元表达式*/
unaryExp
    : primaryExp                                          #unaryExpPrimaryExp
    | Identifier '(' funcRParams? ')'                     #unaryExpFuncR
    | unaryOp unaryExp                                    #unaryExpUnary
    ;
/*单目运算符*/
unaryOp: '+' | '-' | '!'; /*注 '!' 仅出现在条件表达式中*/

/*函数实参表*/
funcRParams: funcRParam (',' funcRParam)*;
funcRParam: exp | StringLiteral ;


/*乘除模表达式*/
mulExp: left=unaryExp (op+=('*' | '/' | '%') right+=unaryExp )* ;
/*加减表达式*/
addExp: left=mulExp (op+=('+' | '-') right+=mulExp)* ;
/*关系表达式*/
relExp: left=addExp (op+=('<' | '>' | '<=' | '>=') right+=addExp)*;
/*相等性表达式*/
eqExp: left=relExp (op+=('==' | '!=') right+=relExp)*;
/*逻辑与表达式*/
lAndExp: left=eqExp (op+='&&' right+=eqExp)* ;
/*逻辑或表达式*/
lOrExp: left=lAndExp (op+='||' right+=lAndExp)*;
/*常量表达式*/
constExp: addExp ; /*注： 使用的 Ident 必须是常量*/

/*标识符*/
Identifier: IdentifierNondigit (IdentifierNondigit|Digit)*;
fragment IdentifierNondigit: [A-Z_a-z];
fragment Digit:[0-9];

/*整型常量*/
intConst: DecimalConstant                                 #intDecConst
    | OctalConstant                                       #intOctConst
    | HexadecimalConstant                                 #intHexConst
    ;
DecimalConstant: NonzeroDigit | NonzeroDigit Digit+;
OctalConstant: '0'| '0' OctalDigit+;
HexadecimalConstant: HexadecimalPrefix HexadecimalDigit+;
fragment HexadecimalPrefix: '0x' | '0X';
fragment NonzeroDigit: [1-9];
fragment OctalDigit: [0-7];
fragment HexadecimalDigit: [0-9a-fA-F];


/*浮点型数*/
floatConst: FloatingConstant;
FloatingConstant
    : DecimalFloatingConstant
    | HexadecimalFloatingConstant;
fragment DecimalFloatingConstant
    : FractionalConstant ExponentPart?
    | DigitSequence ExponentPart
    ;
fragment HexadecimalFloatingConstant
    : HexadecimalPrefix HexadecimalFractionalConstant BinaryExponentPart
    | HexadecimalPrefix HexadecimalDigitSequence BinaryExponentPart
    ;
fragment FractionalConstant
    : DigitSequence? '.' DigitSequence
    | DigitSequence '.'
    ;
fragment ExponentPart
    : 'e' Sign? DigitSequence
    | 'E' Sign? DigitSequence
    ;
fragment Sign : '+' | '-';
fragment DigitSequence : Digit+;
fragment HexadecimalFractionalConstant
    : HexadecimalDigitSequence? '.' HexadecimalDigitSequence
    | HexadecimalDigitSequence '.'
    ;
fragment BinaryExponentPart
    : 'p' Sign? DigitSequence
    | 'P' Sign? DigitSequence
    ;
fragment HexadecimalDigitSequence: HexadecimalDigit+;

/*用于特殊输入的字符串*/
StringLiteral: '"' SChar*  '"';
fragment SChar
    : ~["\\\r\n]
    | EscapeSequence
    ;
fragment EscapeSequence: '\\' ["\\];

Whitespace: [ \t]+ -> skip;

Newline: ('\r' '\n'?|'\n') -> skip;

/*注释*/
BlockComment: '/*' .*? '*/'-> skip;

LineComment: '//' ~[\r\n]* -> skip;
