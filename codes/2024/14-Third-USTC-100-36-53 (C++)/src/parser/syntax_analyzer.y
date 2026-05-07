%glr-parser
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "syntax_tree.h"

// external functions from lex
extern int yylex();
extern int yyparse();
extern int yyrestart(FILE *fp);
extern FILE * yyin;

// external variables from lexical_analyzer module
extern int lines;
extern char * yytext;
extern int pos_end;
extern int pos_start;

// Global syntax tree
syntax_tree *gt;

// Error reporting
void yyerror(const char *s);

// Helper functions written for you with love
syntax_tree_node *node(const char *node_name, int children_num, ...);
%}

/*  Complete this definition.
   Hint: See pass_node(), node(), and syntax_tree.h.
         Use forward declaring. */
%union {
    struct _syntax_tree_node *node;
}

/* Your tokens here. */
%token <node> ERROR

%token <node> LP RP LBRACK RBRACK
%token <node> MUL DIV MOD
%token <node> ADD SUB
%token <node> AND OR
%token <node> NOT
%token <node> LT GT LE GE
%token <node> EQ NE
%token <node> ASSIGN
%token <node> COMMA

%left LP RP LBRACK RBRACK
%left MUL DIV MOD
%left ADD SUB
%left AND OR
%left LT GT LE GE
%left EQ NE
%left COMMA

%right ASSIGN
%right NOT

%token <node> LBRACE RBRACE
%token <node> SEMI

%token <node> ELSE IF INT RETURN VOID WHILE FLOAT CONST BREAK CONTINUE

%token <node> IDENT INTCONST FLOATCONST

%type <node> Program
%type <node> CompUnit
%type <node> Decl FuncDef
%type <node> ConstDecl ConstDefList ConstDef ConstExpList ConstInitVal ConstInitValList BType
%type <node> VarDecl VarDefList VarDef InitVal InitValList
%type <node> FuncType FuncFParams FuncFParam ExpList
%type <node> Block BlockItemList BlockItem
%type <node> Stmt LVal Exp Cond PrimaryExp Number UnaryExp UnaryOp FuncRParams
%type <node> MulExp AddExp RelExp EqExp LAndExp LOrExp ConstExp
%type <node> IntConst FloatConst

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%start Program
%%

/* Your rules here. */

/* Example:
program: declaration-list {$$ = node( "program", 1, $1); gt->root = $$;}
       ;
*/

Program :
CompUnit {$$ = node("Program", 1, $1); gt->root = $$;}
;

CompUnit :
CompUnit Decl {$$ = node("CompUnit", 2, $1, $2);}
| CompUnit FuncDef {$$ = node("CompUnit", 2, $1, $2);}
| Decl {$$ = node("CompUnit", 1, $1);}
| FuncDef {$$ = node("CompUnit", 1, $1);}
;

Decl :
ConstDecl {$$ = node("Decl", 1, $1);}
| VarDecl {$$ = node("Decl", 1, $1);}
;

ConstDecl :
CONST BType ConstDefList SEMI {$$ = node("ConstDecl", 4, $1, $2, $3, $4);}
;

ConstDefList :
ConstDef {$$ = node("ConstDefList", 1, $1);}
| ConstDefList COMMA ConstDef {$$ = node("ConstDefList", 3, $1, $2, $3);}
;

BType :
INT {$$ = node("BType", 1, $1);}
| FLOAT {$$ = node("BType", 1, $1);}
;

ConstDef :
IDENT ConstExpList ASSIGN ConstInitVal {$$ = node("ConstDef", 4, $1, $2, $3, $4);}
;

ConstExpList :
/* epsilon */ {$$ = node("ConstExpList", 0);}
| ConstExpList LBRACK ConstExp RBRACK {$$ = node("ConstExpList", 4, $1, $2, $3, $4);}
;

ConstInitVal :
ConstExp {$$ = node("ConstInitVal", 1, $1);}
| LBRACE ConstInitValList RBRACE {$$ = node("ConstInitVal", 3, $1, $2, $3);}
;

ConstInitValList :
/* epsilon */ {$$ = node("ConstInitValList", 0);}
| ConstInitVal {$$ = node("ConstInitValList", 1, $1);}
| ConstInitValList COMMA ConstInitVal {$$ = node("ConstInitValList", 3, $1, $2, $3);}
;

VarDecl :
BType VarDefList SEMI {$$ = node("VarDecl", 3, $1, $2, $3);}
;

VarDefList :
VarDef {$$ = node("VarDefList", 1, $1);}
| VarDefList COMMA VarDef {$$ = node("VarDefList", 3, $1, $2, $3);}
;

VarDef :
IDENT ConstExpList {$$ = node("VarDef", 2, $1, $2);}
| IDENT ConstExpList ASSIGN InitVal {$$ = node("VarDef", 4, $1, $2, $3, $4);}
;

InitVal :
Exp {$$ = node("InitVal", 1, $1);}
| LBRACE InitValList RBRACE {$$ = node("InitVal", 3, $1, $2, $3);}
;

InitValList :
/* epsilon */ {$$ = node("InitValList", 0);}
| InitVal {$$ = node("InitValList", 1, $1);}
| InitValList COMMA InitVal {$$ = node("InitValList", 3, $1, $2, $3);}
;

FuncDef :
FuncType IDENT LP FuncFParams RP Block {$$ = node("FuncDef", 6, $1, $2, $3, $4, $5, $6);}
| FuncType IDENT LP RP Block {$$ = node("FuncDef", 5, $1, $2, $3, $4, $5);}
;

FuncType :
VOID {$$ = node("FuncType", 1, $1);}
| INT {$$ = node("FuncType", 1, $1);}
| FLOAT {$$ = node("FuncType", 1, $1);}
;

FuncFParams :
FuncFParam {$$ = node("FuncFParams", 1, $1);}
| FuncFParams COMMA FuncFParam {$$ = node("FuncFParams", 3, $1, $2, $3);}
;

FuncFParam :
BType IDENT {$$ = node("FuncFParam", 2, $1, $2);}
| BType IDENT LBRACK RBRACK ExpList {$$ = node("FuncFParam", 5, $1, $2, $3, $4, $5);}
;

ExpList :
/* epsilon */ {$$ = node("ExpList", 0);}
| ExpList LBRACK Exp RBRACK {$$ = node("ExpList", 4, $1, $2, $3, $4);}
;

Block :
LBRACE BlockItemList RBRACE {$$ = node("Block", 3, $1, $2, $3);}
;

BlockItemList :
/* epsilon */ {$$ = node("BlockItemList", 0);}
| BlockItemList BlockItem {$$ = node("BlockItemList", 2, $1, $2);}
;

BlockItem :
Decl {$$ = node("BlockItem", 1, $1);}
| Stmt {$$ = node("BlockItem", 1, $1);}
;

Stmt :
LVal ASSIGN Exp SEMI {$$ = node("Stmt", 4, $1, $2, $3, $4);}
| SEMI {$$ = node("Stmt", 1, $1);}
| Exp SEMI {$$ = node("Stmt", 2, $1, $2);}
| Block {$$ = node("Stmt", 1, $1);}
| IF LP Cond RP Stmt %prec LOWER_THAN_ELSE {$$ = node("Stmt", 5, $1, $2, $3, $4, $5);}
| IF LP Cond RP Stmt ELSE Stmt {$$ = node("Stmt", 7, $1, $2, $3, $4, $5, $6, $7);}
| WHILE LP Cond RP Stmt {$$ = node("Stmt", 5, $1, $2, $3, $4, $5);}
| BREAK SEMI {$$ = node("Stmt", 2, $1, $2);}
| CONTINUE SEMI {$$ = node("Stmt", 2, $1, $2);}
| RETURN SEMI {$$ = node("Stmt", 2, $1, $2);}
| RETURN Exp SEMI {$$ = node("Stmt", 3, $1, $2, $3);}
;

Exp :
AddExp {$$ = node("Exp", 1, $1);}
;

Cond :
LOrExp {$$ = node("Cond", 1, $1);}
;

LVal :
IDENT ExpList {$$ = node("LVal", 2, $1, $2);}
;

PrimaryExp :
LP Exp RP {$$ = node("PrimaryExp", 3, $1, $2, $3);}
| LVal {$$ = node("PrimaryExp", 1, $1);}
| Number {$$ = node("PrimaryExp", 1, $1);}
;

Number :
IntConst {$$ = node("Number", 1, $1);}
| FloatConst {$$ = node("Number", 1, $1);}
;

IntConst :
INTCONST {$$ = node("IntConst", 1, $1);}
;

FloatConst :
FLOATCONST {$$ = node("FloatConst", 1, $1);}
;

UnaryExp :
PrimaryExp {$$ = node("UnaryExp", 1, $1);}
| IDENT LP RP {$$ = node("UnaryExp", 3, $1, $2, $3);}
| IDENT LP FuncRParams RP {$$ = node("UnaryExp", 4, $1, $2, $3, $4);}
| UnaryOp UnaryExp {$$ = node("UnaryExp", 2, $1, $2);}
;

UnaryOp :
ADD {$$ = node("UnaryOp", 1, $1);}
| SUB {$$ = node("UnaryOp", 1, $1);}
| NOT {$$ = node("UnaryOp", 1, $1);}
;

FuncRParams :
Exp {$$ = node("FuncRParams", 1, $1);}
| FuncRParams COMMA Exp {$$ = node("FuncRParams", 3, $1, $2, $3);}
;

MulExp :
UnaryExp {$$ = node("MulExp", 1, $1);}
| MulExp MUL UnaryExp {$$ = node("MulExp", 3, $1, $2, $3);}
| MulExp DIV UnaryExp {$$ = node("MulExp", 3, $1, $2, $3);}
| MulExp MOD UnaryExp {$$ = node("MulExp", 3, $1, $2, $3);}
;

AddExp :
MulExp {$$ = node("AddExp", 1, $1);}
| AddExp ADD MulExp {$$ = node("AddExp", 3, $1, $2, $3);}
| AddExp SUB MulExp {$$ = node("AddExp", 3, $1, $2, $3);}
;

RelExp :
AddExp {$$ = node("RelExp", 1, $1);}
| RelExp LT AddExp {$$ = node("RelExp", 3, $1, $2, $3);}
| RelExp GT AddExp {$$ = node("RelExp", 3, $1, $2, $3);}
| RelExp LE AddExp {$$ = node("RelExp", 3, $1, $2, $3);}
| RelExp GE AddExp {$$ = node("RelExp", 3, $1, $2, $3);}
;

EqExp :
RelExp {$$ = node("EqExp", 1, $1);}
| EqExp EQ RelExp {$$ = node("EqExp", 3, $1, $2, $3);}
| EqExp NE RelExp {$$ = node("EqExp", 3, $1, $2, $3);}
;

LAndExp :
EqExp {$$ = node("LAndExp", 1, $1);}
| LAndExp AND EqExp {$$ = node("LAndExp", 3, $1, $2, $3);}
;

LOrExp :
LAndExp {$$ = node("LOrExp", 1, $1);}
| LOrExp OR LAndExp {$$ = node("LOrExp", 3, $1, $2, $3);}
;

ConstExp :
AddExp {$$ = node("ConstExp", 1, $1);}
;

%%

/// The error reporting function.
void yyerror(const char * s)
{
    // TO STUDENTS: This is just an example.
    // You can customize it as you like.
    fprintf(stderr, "error at line %d column %d: %s\n", lines, pos_start, s);
}

/// Parse input from file `input_path`, and prints the parsing results
/// to stdout.  If input_path is NULL, read from stdin.
///
/// This function initializes essential states before running yyparse().
syntax_tree *parse(const char *input_path)
{
    if (input_path != NULL) {
        if (!(yyin = fopen(input_path, "r"))) {
            fprintf(stderr, "[ERR] Open input file %s failed.\n", input_path);
            exit(1);
        }
    } else {
        yyin = stdin;
    }

    lines = pos_start = pos_end = 1;
    gt = new_syntax_tree();
    yyrestart(yyin);
    yyparse();
    return gt;
}

/// A helper function to quickly construct a tree node.
///
/// e.g. $$ = node("program", 1, $1);
syntax_tree_node *node(const char *name, int children_num, ...)
{
    syntax_tree_node *p = new_syntax_tree_node(name);
    syntax_tree_node *child;
    if (children_num == 0) {
        // child = new_syntax_tree_node("epsilon");
        // syntax_tree_add_child(p, child);
    } else {
        va_list ap;
        va_start(ap, children_num);
        for (int i = 0; i < children_num; ++i) {
            child = va_arg(ap, syntax_tree_node *);
            syntax_tree_add_child(p, child);
        }
        va_end(ap);
    }
    return p;
}
