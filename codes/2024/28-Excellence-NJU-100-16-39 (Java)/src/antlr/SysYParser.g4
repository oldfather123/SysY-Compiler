parser grammar SysYParser;
@header {
package antlr;
}

options {
    tokenVocab = SysYLexer;
    }
program
   : compUnit
   ;

compUnit//编译单元: 函数声明+声明
    :
    (funcDef | decl) + EOF
    ;

decl//声明：常量声明+变量声明
    :constDecl
    |varDecl
    ;

constDecl//常量声明
    :CONST bType constDef (COMMA constDef)* SEMICOLON
    ;

bType//基本类型
   : INT
   | FLOAT
    ;

constDef//常数定义 ConstDef → Ident { '[' ConstExp ']' } '=' ConstInitVal
   : IDENT(L_BRACKT constExp R_BRACKT)* ASSIGN constInitVal
    ;

constInitVal//常量初值 ConstInitVal → ConstExp| '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
   : constExp
    |L_BRACE((constInitVal(COMMA constInitVal)*)?)R_BRACE
    ;

varDecl//    变量声明 VarDecl → BType VarDef { ',' VarDef } ';'
    :bType varDef(COMMA varDef)*SEMICOLON
    ;

varDef//    变量定义 VarDef → Ident { '[' ConstExp ']' } | Ident { '[' ConstExp ']' } '=' InitVal
   : IDENT (L_BRACKT constExp R_BRACKT)*(ASSIGN initVal)?
    ;

initVal//    变量初值 InitVal → Exp | '{' [ InitVal { ',' InitVal } ] '}'
   : exp
    |L_BRACE(initVal (COMMA initVal)*) ? R_BRACE
    ;

funcDef//   函数定义 FuncDef → FuncType Ident '(' [FuncFParams] ')' Block
   : funcType IDENT L_PAREN funcFParams? R_PAREN block
    ;

funcType//    函数类型 FuncType → 'void' | 'int'
    :VOID
    |INT
    |FLOAT
    ;

funcFParams//    函数形参表 FuncFParams → FuncFParam { ',' FuncFParam }
    :funcFParam(COMMA funcFParam)*
    ;

funcFParam//    函数形参 FuncFParam → BType Ident ['[' ']' { '[' Exp ']' }]
    :bType IDENT(L_BRACKT R_BRACKT(L_BRACKT exp R_BRACKT)*)?
    ;

block//    语句块 Block → '{' { BlockItem } '}'
   : L_BRACE blockItem* R_BRACE
    ;
blockItem//    语句块项 BlockItem → Decl | Stmt
   : decl
    |stmt
    ;
stmt
//    语句 Stmt → LVal '=' Exp ';' | [Exp] ';' | Block ;
//    | 'if' '( Cond ')' Stmt [ 'else' Stmt ]
//    | 'while' '(' Cond ')' Stmt
//    | 'break' ';' | 'continue' ';'
//    | 'return' [Exp] ';'
   : lVal ASSIGN exp SEMICOLON #stmt_with_assign
    |(exp)? SEMICOLON #stmt_with_exp
    |block #stmt_with_block
    |IF L_PAREN cond R_PAREN stmt (ELSE stmt)? #stmt_with_if
    |WHILE L_PAREN cond R_PAREN stmt #stmt_with_while
    |BREAK SEMICOLON #stmt_with_break
    |CONTINUE SEMICOLON #stmt_with_continue
    |returnStmt #stmt_with_return
    ;

returnStmt
    :RETURN (exp)? SEMICOLON
    ;

exp //表达式
   : L_PAREN exp R_PAREN #exp_with_Paren
   | lVal   #exp_with_lval
   | number #exp_with_num
   | IDENT L_PAREN funcRParams? R_PAREN #exp_with_funcRParams
   | unaryOp exp    #exp_with_unaryOp
   | exp (MUL | DIV | MOD) exp  #exp_with_symbol
   | exp (PLUS | MINUS) exp #exp_with_plus_and_mius
   ;

cond  //条件表达式
   : exp
   | cond (LT | GT | LE | GE) cond
   | cond (EQ | NEQ) cond
   | cond AND cond
   | cond OR cond
   ;

lVal//左值表达式
   : IDENT (L_BRACKT exp R_BRACKT)*
   ;

number//整数
   : INTEGER_CONST
   | FLOAT_CONST
   ;

unaryOp//一元操作符
   : PLUS
   | MINUS
   | NOT
   ;

funcRParams//函数实参表
   : param (COMMA param)*
   ;

param //参数
   : exp
   ;

constExp //常量表达式
   : exp
   ;
