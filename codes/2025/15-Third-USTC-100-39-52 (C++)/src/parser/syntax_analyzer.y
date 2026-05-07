//定义部分的文字块
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "../../include/common/syntax_tree.h"
#include "syntax_analyzer.h"

// external functions from lex
extern int yylex();
extern int yyparse();
extern int yyrestart(FILE *input_file);
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

%union {
    struct _syntax_tree_node* node;
}

/* TODO: Your tokens here. */
//终结符
%token <node> ERROR
// 终结符 + - * / % (单目运算符 + - 包含在其中)
%token <node> ADD SUB MUL DIV MOD ADASS SUASS MUASS DIASS
// 终结符 逻辑运算符 < <= > >= == !=
%token <node> LT LET GT GET EQ NEQ
// 终结符 括号 ( ) [ ] { }
%token <node> LPARENTHESIS RPARENTHESIS LBRACKET RBRACKET LBRACE RBRACE
// 终结符 = , ; !  
%token <node> ASSIGN COMMA SEMICOLON NOT
// 终结符 && || 
%token <node> AND OR
// int float void const
%token <node> INT FLOAT VOID CONST MAIN
// if else while break continue return
%token <node> IF ELSE WHILE BREAK CONTINUE RETURN FOR
// 标识符 Ident
%token <node> IDENT
// 整数常量和浮点数常量
%token <node> INTCONST FLOATCONST

// 优先级
%right ADASS SUASS MUASS DIASS
%left COMMA
%left ASSIGN
%left EQ NEQ LT LET GT GET
%left ADD SUB
%left MUL DIV MOD
%left NOT
%nonassoc LOWER_THAN_ASIGN

// 非终结符 //MainMod
%type <node> Program
%type <node> CompMod
// 声明 及 常量声明
%type <node> Decl ConstDecl
%type <node> ConstDefList
// 基本类型
%type <node> BType
// 常数定义 常量初值
%type <node> ConstDef
%type <node> ConstList 
%type <node> ConstInit
%type <node> ConstInitList
// 变量声明，变量定义，变量初值
%type <node> VarDecl VarDeclList VarDef InitVal InitValList
//函数定义，函数类型，函数形参表, 函数形参 函数声明MainDef
%type <node> FuncDef FuncParams FuncParam FuncDecl 
%type <node> ExpList 
// 语句块, 语句块项，语句及特殊句式
%type <node> Body BodyItem stmt
%type <node> BodyList WhileItem IFItem jump_stmt
%type <node> Forstmt ForInit ForCond ForIncr
//表达式 数值
%type <node> Expr Number
//函数实参表
%type <node> FuncRParams
// 各种表达式
%type <node> Cond LVal Primary_Expr Unary_Expr MDM_Expr Add_Expr Rel_Expr Eq_Expr And_expr OR_Expr ConstExp Func_Expr
//值
%type <node> Integer Floatnum
// 开始符
%start Program

%%

// 定义开始
Program : CompMod {$$ = node("program", 1, $1); gt->root = $$;}
;
// 编译模块,包括声明和函数
CompMod 
: CompMod Decl {$$ = node("CompMod", 2, $1, $2);}
| CompMod FuncDef  {$$ = node("CompMod", 2, $1, $2);}
| Decl {$$ = node("CompMod", 1, $1);}
| FuncDef {$$ = node("CompMod", 1, $1);}
;
//基本类型
BType
: INT {$$ = node("BType", 1, $1);}
| FLOAT {$$ = node("BType", 1, $1);}
;
// 声明：常量、变量、函数
Decl
: ConstDecl {$$ = node("Decl", 1, $1);}
| VarDecl   {$$ = node("Decl", 1, $1);}
| FuncDecl  {$$ = node("Decl", 1, $1);}
;
//函数声明
FuncDecl
: BType IDENT LPARENTHESIS FuncParams RPARENTHESIS SEMICOLON {$$ = node("FuncDecl", 6, $1, $2, $3, $4, $5, $6);}
| VOID IDENT LPARENTHESIS FuncParams RPARENTHESIS SEMICOLON {$$ = node("FuncDecl", 6, $1, $2, $3, $4, $5, $6);}
| VOID IDENT LPARENTHESIS RPARENTHESIS SEMICOLON {$$ = node("FuncDecl", 5, $1, $2, $3, $4, $5);}
| BType IDENT LPARENTHESIS RPARENTHESIS SEMICOLON {$$ = node("FuncDecl", 5, $1, $2, $3, $4, $5);}
;
// 常量声明
ConstDecl
: CONST BType ConstDefList SEMICOLON {$$ = node("ConstDecl", 4, $1, $2, $3, $4);}
;
ConstDefList 
: ConstDef {$$ = node("ConstDefList", 1, $1);}
| ConstDefList COMMA ConstDef {$$ = node("ConstDefList", 3, $1, $2, $3);}
;
// 常数定义,必定会赋值
ConstDef
: IDENT ConstList ASSIGN ConstInit {$$ = node("ConstDef", 4, $1, $2, $3, $4);}
;
// 是否为数组格式
ConstList
:  {$$ = node("ConstList", 0);}
| ConstList LBRACKET ConstExp RBRACKET {$$ = node("ConstList", 4, $1, $2, $3, $4);}
;
// 常量初值，考虑数组等
ConstInit
: ConstExp  {$$ = node("ConstInit", 1, $1);}
| LBRACE ConstInitList RBRACE {$$ = node("ConstInit", 3, $1, $2, $3);}
| LBRACE RBRACE {$$ = node("ConstInit", 2, $1, $2);}
;
ConstInitList
: ConstInit {$$ = node("ConstInitList", 1, $1);}
| ConstInitList COMMA ConstInit {$$ = node("ConstInitList", 3, $1, $2, $3);}
;
// 常量表达式
ConstExp 
: Add_Expr {$$ = node("ConstExp", 1, $1);}
;
// 变量声明
VarDecl 
: BType VarDeclList SEMICOLON {$$ = node("VarDecl", 3, $1, $2, $3);}
;
VarDeclList
:  VarDef { $$ = node("VarDefList", 1, $1); }
| VarDeclList COMMA VarDef {$$ = node("VarDeclList", 3, $1, $2, $3);}
;
// 变量定义
VarDef
: IDENT ConstList {$$ = node("VarDef", 2, $1, $2);}
| IDENT ConstList ASSIGN InitVal {$$ = node("VarDef", 4, $1, $2, $3, $4);}
;
// 变量初值
InitVal
: Expr {$$ = node("InitVal", 1, $1);}
| LBRACE InitValList RBRACE {$$ = node("InitVal", 3, $1, $2, $3);}
| LBRACE RBRACE {$$ = node("InitVal", 2, $1, $2);}
;
InitValList
: InitVal {$$ = node("InitValList", 1, $1);}
| InitValList COMMA InitVal {$$ = node("InitValList", 3, $1, $2, $3);}
;
// 函数定义
FuncDef
: BType IDENT LPARENTHESIS RPARENTHESIS Body {$$ = node("FuncDef", 5, $1, $2, $3, $4, $5);}
| VOID IDENT LPARENTHESIS RPARENTHESIS Body {$$ = node("FuncDef", 5, $1, $2, $3, $4, $5);}
| BType IDENT LPARENTHESIS FuncParams RPARENTHESIS Body {$$ = node("FuncDef", 6, $1, $2, $3, $4, $5, $6);}
| VOID IDENT LPARENTHESIS FuncParams RPARENTHESIS Body {$$ = node("FuncDef", 6, $1, $2, $3, $4, $5, $6);}
;
// 函数形参表，考虑主函数形参有时显示为VOID的情况
FuncParams 
: FuncParam {$$ = node("FuncParams", 1, $1);}
| FuncParams COMMA FuncParam {$$ = node("FuncParams", 3, $1, $2, $3);}
| VOID {$$ = node("FuncParams", 1, $1);}
;
// 函数形参
FuncParam 
: BType IDENT {$$ = node("FuncParam", 2, $1, $2);}
| BType IDENT LBRACKET RBRACKET ExpList {$$ = node("FuncParam", 5, $1, $2, $3, $4, $5);}
;
ExpList 
: {$$ = node("ExpList", 0);}
| ExpList LBRACKET Expr RBRACKET {$$ = node("ExpList", 4, $1, $2, $3, $4);}
;
// 语句块
Body
: LBRACE BodyList RBRACE {$$ = node("Body", 3, $1, $2, $3);}
;
BodyList
: {$$ = node("BodyList", 0);}
| BodyList BodyItem {$$ = node("BodyList", 2, $1, $2);}
;
// 块项
BodyItem
: Decl {$$ = node("BodyItem", 1, $1);}
| stmt {$$ = node("BodyItem", 1, $1);}
;
// 基本语句
stmt
: LVal ASSIGN Expr SEMICOLON {$$ = node("stmt", 4, $1, $2, $3, $4);}
| LVal ADASS Expr SEMICOLON {$$ = node("stmt", 4, $1, $2, $3, $4);}
| LVal SUASS Expr SEMICOLON {$$ = node("stmt", 4, $1, $2, $3, $4);}
| LVal MUASS Expr SEMICOLON {$$ = node("stmt", 4, $1, $2, $3, $4);}
| LVal DIASS Expr SEMICOLON {$$ = node("stmt", 4, $1, $2, $3, $4);}
| Expr SEMICOLON {$$ = node("stmt", 2, $1, $2);}
| SEMICOLON {$$ = node("stmt", 1, $1);}
| Body {$$ = node("stmt", 1, $1);}
| IFItem {$$ = node("stmt", 1, $1);}
| WhileItem {$$ = node("stmt", 1, $1);}
| jump_stmt {$$ = node("stmt", 1, $1);}
| Forstmt {$$ = node("stmt", 1, $1);}
;
// for循环模块 好像不用实现？
Forstmt
: FOR LPARENTHESIS ForInit SEMICOLON ForCond SEMICOLON ForIncr RPARENTHESIS stmt { $$ = node("Forstmt", 5, $3, $5, $7, $9); }
;
ForInit
: Expr { $$ = node("ForInit", 1, $1); }
| { $$ = node("ForInit", 0); }
;
ForCond
: Cond { $$ = node("ForCond", 1, $1); }
| { $$ = node("ForCond", 0); }
;
ForIncr
: Expr { $$ = node("ForIncr", 1, $1); }
| { $$ = node("ForIncr", 0); }
;
//if 模块
IFItem
: IF LPARENTHESIS Cond RPARENTHESIS stmt {$$ = node("IFItem", 5, $1, $2, $3, $4, $5);}
| IF LPARENTHESIS Cond RPARENTHESIS stmt ELSE stmt {$$ = node("IFItem", 7, $1, $2, $3, $4, $5, $6, $7);}
;
//while 模块
WhileItem
: WHILE LPARENTHESIS Cond RPARENTHESIS stmt {$$ = node("WhileItem", 5, $1, $2, $3, $4, $5);}
;
// 跳转语句
jump_stmt
: RETURN Expr SEMICOLON { $$ = node("jump_stmt", 3, $1, $2, $3); }
| RETURN SEMICOLON { $$ = node("jump_stmt", 2, $1, $2); }
| BREAK SEMICOLON { $$ = node("jump_stmt", 2, $1, $2); }
| CONTINUE SEMICOLON { $$ = node("jump_stmt", 2, $1, $2); }
;
// 表达式(起点，匹配所有表达式)
Expr 
: Add_Expr {$$ = node("Expr", 1, $1);}
;
// 条件表达式
Cond
: OR_Expr {$$ = node("Cond", 1, $1);}
;
// 左值表达式
LVal 
: IDENT ExpList {$$ = node("LVal", 2, $1, $2);}
;
// 基本表达式
Primary_Expr
: LPARENTHESIS Expr RPARENTHESIS {$$ = node("Primary_Expr", 3, $1, $2, $3);}
| LVal {$$ = node("Primary_Expr", 1, $1);}
| Number {$$ = node("Primary_Expr", 1, $1);}
;
// 数值
Number 
: Integer {$$ = node("Number", 1, $1);}
| Floatnum {$$ = node("Number", 1, $1);}
;
//一元表达式
Unary_Expr
: Primary_Expr {$$ = node("Unary_Expr", 1, $1);}
| Func_Expr     {$$ = node("Unary_Expr", 1, $1);}
| ADD Unary_Expr {$$ = node("Unary_Expr", 2, $1, $2);}
| SUB Unary_Expr {$$ = node("Unary_Expr", 2, $1, $2);}
| NOT Unary_Expr {$$ = node("Unary_Expr", 2, $1, $2);}
;
//函数表达式即函数调用
Func_Expr
: IDENT LPARENTHESIS FuncRParams RPARENTHESIS {$$ = node("Func_Expr", 4, $1, $2, $3, $4);}
| IDENT LPARENTHESIS RPARENTHESIS {$$ = node("Func_Expr", 3, $1, $2, $3);}
;
// 函数实参表
FuncRParams 
: Expr {$$ = node("FuncRParams", 1, $1);}
| FuncRParams COMMA Expr {$$ = node("FuncRParams", 3, $1, $2, $3);}
;
// 乘除模表达式
MDM_Expr 
: Unary_Expr {$$ = node("MDM_Expr", 1, $1);}
| MDM_Expr MUL Unary_Expr {$$ = node("MDM_Expr", 3, $1, $2, $3);}
| MDM_Expr DIV Unary_Expr {$$ = node("MDM_Expr", 3, $1, $2, $3);}
| MDM_Expr MOD Unary_Expr {$$ = node("MDM_Expr", 3, $1, $2, $3);}
;
// 加减表达式
Add_Expr
: MDM_Expr {$$ = node("Add_Expr", 1, $1);}
| Add_Expr ADD MDM_Expr {$$ = node("Add_Expr", 3, $1, $2, $3);}
| Add_Expr SUB MDM_Expr {$$ = node("Add_Expr", 3, $1, $2, $3);}
;
// 大小关系表达式
Rel_Expr 
: Add_Expr {$$ = node("Rel_Expr", 1, $1);}
| Rel_Expr LT Add_Expr {$$ = node("Rel_Expr", 3, $1, $2, $3);}
| Rel_Expr LET Add_Expr {$$ = node("Rel_Expr", 3, $1, $2, $3);}
| Rel_Expr GT Add_Expr {$$ = node("Rel_Expr", 3, $1, $2, $3);}
| Rel_Expr GET Add_Expr {$$ = node("Rel_Expr", 3, $1, $2, $3);}
;
// 逻辑等、非表达式
Eq_Expr 
: Rel_Expr {$$ = node("Eq_Expr", 1, $1);}
| Eq_Expr EQ Rel_Expr {$$ = node("Eq_Expr", 3, $1, $2, $3);}
| Eq_Expr NEQ Rel_Expr {$$ = node("Eq_Expr", 3, $1, $2, $3);}
;
// 逻辑与表达式
And_expr 
: Eq_Expr {$$ = node("And_expr", 1, $1);}
| And_expr AND Eq_Expr {$$ = node("And_expr", 3, $1, $2, $3);}
;
// 逻辑或表达式
OR_Expr 
: And_expr {$$ = node("OR_Expr", 1, $1);}
| OR_Expr OR And_expr {$$ = node("OR_Expr", 3, $1, $2, $3);}
;
//整型数据
Integer
: INTCONST {$$ = node("Integer", 1, $1);}
;
//浮点型数据
Floatnum
: FLOATCONST {$$ = node("Floatnum", 1, $1);}
;
%%

/// The error reporting function.
void yyerror(const char * s)
{   
    // TO sTUDENTs: This is just an example.
    // You can customize it as you like.
    fprintf(stderr, "%d:%d: error:%s\n", lines, pos_start, s);
}

/// Parse input from file `input_path`, and prints the parsing results
/// to stdout.  If input_path is NULL, read from stdin.
///
/// This function initializes esential states before running yyparse().
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
        child = new_syntax_tree_node("epsilon");
        syntax_tree_add_child(p, child);
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