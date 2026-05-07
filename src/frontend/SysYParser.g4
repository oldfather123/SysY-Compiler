parser grammar SysYParser;

options {
    tokenVocab = SysYLexer;
}

compUnit
    : (decl | funcDef)+ EOF
    ;

decl
    : constDecl
    | varDecl
    ;

constDecl
    : CONST bType constDef (COMMA constDef)* SEMICOLON
    ;

bType
    : INT
    | FLOAT
    ;

constDef
    : IDENT (LBRACK constExp RBRACK)* ASSIGN constInitVal
    ;

constInitVal
    : constExp
    | LBRACE (constInitVal (COMMA constInitVal)*)? RBRACE
    ;

varDecl
    : bType varDef (COMMA varDef)* SEMICOLON
    ;

varDef
    : IDENT (LBRACK constExp RBRACK)* (ASSIGN initVal)?
    ;

initVal
    : exp
    | LBRACE (initVal (COMMA initVal)*)? RBRACE
    ;

funcDef
    : funcType IDENT LPAREN funcFParams? RPAREN block
    ;

funcType
    : VOID
    | INT
    | FLOAT
    ;

funcFParams
    : funcFParam (COMMA funcFParam)*
    ;

funcFParam
    : bType IDENT (LBRACK RBRACK (LBRACK exp RBRACK)*)?
    ;

block
    : LBRACE blockItem* RBRACE
    ;

blockItem
    : decl
    | stmt
    ;

stmt
    : lVal ASSIGN exp SEMICOLON
    | exp? SEMICOLON
    | block
    | IF LPAREN cond RPAREN stmt (ELSE stmt)?
    | WHILE LPAREN cond RPAREN stmt
    | BREAK SEMICOLON
    | CONTINUE SEMICOLON
    | RETURN exp? SEMICOLON
    ;

exp
    : addExp
    ;

cond
    : lOrExp
    ;

lVal
    : IDENT (LBRACK exp RBRACK)*
    ;

primaryExp
    : LPAREN exp RPAREN
    | lVal
    | number
    ;

number
    : INT_CONST
    | FLOAT_CONST
    ;

unaryExp
    : primaryExp
    | IDENT LPAREN funcRParams? RPAREN
    | unaryOp unaryExp
    ;

unaryOp
    : PLUS
    | MINUS
    | NOT
    ;

funcRParams
    : funcRParam (COMMA funcRParam)*
    ;

funcRParam
    : exp
    | STRING_CONST
    ;

mulExp
    : unaryExp
    | mulExp (MUL | DIV | MOD) unaryExp
    ;

addExp
    : mulExp
    | addExp (PLUS | MINUS) mulExp
    ;

relExp
    : addExp
    | relExp (LT | GT | LE | GE) addExp
    ;

eqExp
    : relExp
    | eqExp (EQ | NEQ) relExp
    ;

lAndExp
    : eqExp
    | lAndExp AND eqExp
    ;

lOrExp
    : lAndExp
    | lOrExp OR lAndExp
    ;

constExp
    : addExp
    ;
