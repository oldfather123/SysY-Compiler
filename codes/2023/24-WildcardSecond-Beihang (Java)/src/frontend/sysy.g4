//test.g4
grammar sysy;
//算术符号
Add: '+';
Sub: '-';
Mul: '*';
Div: '/';
Mod: '%';
//逻辑符号
Not: '!';
And: '&&';
Or: '||';
//比较符号
Less: '<';
Greater: '>';
LessEqual: '<=';
GreaterEqual: '>=';
Equals: '==';
NotEqual: '!=';
//跳过所有空格、\r、\n、\t
WHITE_SPACE: [ \r\n\t] -> skip;
//跳过所有注释
LineComment: '//' ~ [\r\n]* -> skip;

MultipleLineComment: '/*' .*? '*/' -> skip;
String: '"'.*? '"';
//基本类型，函数类型
Int: 'int';
Float: 'float';
//非基本类型
Void:'void';
//修饰符
Const: 'const';

//控制关键字
If: 'if';
Else: 'else';
While: 'while';
Break: 'break';
Continue: 'continue';
Return: 'return';
End: ';';

//标识符终结符
Ident: [_a-zA-Z] [_a-zA-Z0-9]*;
DecimalConst:
    [1-9] [0-9]* | '0';
OctalConst:
    '0' [0-7]+;
HexConst:
    ('0x'|'0X')[0-9a-fA-F]+;

DecimalFloatConst:
    (([0-9]+ '.') | [0-9]* '.' [0-9]+) ([eE] [+-]? [0-9]+)? |
    [0-9]+ [eE] [+-]? [0-9]+;
HexFloatConst:
    ('0x'|'0X') (
        (([0-9a-fA-F]+ '.') | [0-9a-fA-F]* '.' [0-9a-fA-F]+ | [0-9a-fA-F]+)
        ([pP] [+-]? [0-9]+)
    );

compUnit://编译单元 CompUnit → [ CompUnit ] ( Decl | FuncDef )
    (decl | funcDef)+;
decl://声明 Decl → ConstDecl | VarDecl
    constDecl |
    varDecl;
constDecl://常量声明 ConstDecl → 'const' BType ConstDef { ',' ConstDef } ';'
    Const bType constDef (',' constDef)* ';';
bType://基本类型 BType → 'int' | 'float'
    Int | Float;
constDef://常数定义 ConstDef → Ident { '[' ConstExp ']' } '=' ConstInitVal
    Ident ('[' constExp ']')* '=' constInitVal;
constInitVal://常量初值 ConstInitVal → ConstExp  | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
    constExp |
    '{'
        (constInitVal
            (',' constInitVal)*
        )?
    '}';
varDecl://变量声明 VarDecl → BType VarDef { ',' VarDef } ';'
    bType varDef ( ',' varDef)* ';';
varDef://int a[1][2][3]... = {...}
    Ident ('[' constExp ']')* |
    Ident ('[' constExp ']')* '=' initVal;
initVal: //{a, {b,c, ...}, ...}
    exp|
    '{' ( initVal (',' initVal)*)? '}';
funcDef://void fname ( int a[][exp], int b[] ...){ ... }
    funcType Ident '(' (funcFParams)? ')' block;
funcType:
    Void | Int | Float;
funcFParams://int a, int b, ...
    funcFParam (',' funcFParam)*;
funcFParam: //int a[][exp][exp]...
    bType Ident (
        '[' ']' //第一维大小不能指定，与c语言相似
        (
            '[' constExp ']'//这块我在想是不是写错了，exp是什么操作？为什么不是constexp？
        )*
    )?;
block:
   '{' (blockItem)* '}';
blockItem:
    decl | stmt;
stmt:
    lVal '=' exp End|
    (exp)? End |
    block |
    If '(' cond ')' stmt (Else stmt)? |
    While '(' cond ')' stmt |
    Break End |
    Continue End |
    Return (exp)? End;
exp:
    addExp;
cond:
    lOrExp;
lVal:
    Ident ('[' exp ']')*;
primaryExp:
    '(' exp ')' |
    lVal|
    number|
    String;
intConst:
    DecimalConst | OctalConst | HexConst;
floatConst:
    DecimalFloatConst | HexFloatConst;
number:
    intConst | floatConst;
unaryExp:
    primaryExp |
    Ident '(' (funcRParams)?')' |
    unaryOp unaryExp;
unaryOp:
    Add | Sub | Not;
funcRParams:
    exp (',' exp)*;
mulExp:
    unaryExp | mulExp (Mul | Div | Mod) unaryExp;
addExp:
    mulExp | addExp (Add | Sub) mulExp;
relExp:
    addExp | relExp (Less | Greater |LessEqual | GreaterEqual) addExp;
eqExp:
    relExp | eqExp (Equals | NotEqual) relExp;
lAndExp:
    eqExp | lAndExp And eqExp;
lOrExp:
    lAndExp | lOrExp Or lAndExp;
constExp:
    addExp;




