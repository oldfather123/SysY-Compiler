parser grammar SysYParser;

options {
    tokenVocab = SysYLexer;
}

compUnit
    :(funcDef | decl) compUnit? ;

decl
    :constDecl
    |varDecl;

funcName
    :IDENT;

constDecl
    :CONST bType constDef(COMMA constDef)*SEMICOLON;

bType
    :INT
    |FLOAT;

constDef
    :IDENT (L_BRACKT constExp R_BRACKT)*ASSIGN constInitVal;

constInitVal
    :constExp
    |(L_BRACE (constInitVal(COMMA constInitVal)*)? R_BRACE);

varDecl
    :bType varDef (COMMA varDef)* SEMICOLON;

varDef
    :(IDENT (L_BRACKT constExp R_BRACKT)*)
    |(IDENT (L_BRACKT constExp R_BRACKT)* ASSIGN initVal);

initVal
    :exp
    |(L_BRACE (initVal(COMMA initVal)*)? R_BRACE);

funcDef
    :funcType funcName L_PAREN funcFParams? R_PAREN block;

funcType
    :VOID
    |INT
    |FLOAT;

funcFParams
    :funcFParam(COMMA funcFParam)*;

funcFParam
    :bType IDENT (L_BRACKT R_BRACKT (L_BRACKT exp R_BRACKT)*)?;

block
    :L_BRACE blockItem* R_BRACE;

blockItem
    :decl
    |stmt;

stmt
    :block
    |lVal ASSIGN exp SEMICOLON
    |exp? SEMICOLON
    |ifStruct
    |whileStruct
    |BREAK SEMICOLON
    |CONTINUE SEMICOLON
    |RETURN exp? SEMICOLON;

ifStruct
    :(IF L_PAREN cond R_PAREN stmt (ELSE stmt)?);

whileStruct
    :WHILE L_PAREN cond R_PAREN stmt;
exp
   : L_PAREN exp R_PAREN
   | lVal
   | number
   | funcName L_PAREN funcRParams? R_PAREN
   | unaryOp exp
   | exp (MUL | DIV | MOD) exp
   | exp (PLUS | MINUS) exp
   ;

cond
   : exp
   | cond (LT  | GT | LE | GE) cond
   | cond (EQ | NEQ) cond
   | cond AND cond
   | cond OR cond
   ;

lVal
   : IDENT (L_BRACKT exp R_BRACKT)*
   ;

number
   : INTEGER_CONST
   | FLOAT_CONST
   ;

unaryOp
   : PLUS
   | MINUS
   | NOT
   ;

funcRParams
   : param (COMMA param)*
   ;

param
   : exp
   ;

constExp
   : exp
   ;