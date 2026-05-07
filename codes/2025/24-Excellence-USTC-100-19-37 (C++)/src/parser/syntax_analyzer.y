%{
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "syntax_tree.h"


// external functions from lex
extern int yylex();
extern int yyparse();
extern int yyrestart(FILE * input_file);
extern FILE * yyin;

// external variables from lexical_analyzer module
extern int lines;
extern char * yytext;
extern int pos_end;
extern int pos_start;

// Global syntax tree
syntax_tree * gt;

// Error reporting
void yyerror(const char * s);

// Helper functions written for you with love
syntax_tree_node * node(const char * node_name, int children_num, ...);
%}

/* Complete this definition.
   Hint: See pass_node(), node(), and syntax_tree.h.
         Use forward declaring. */
%union {
    struct _syntax_tree_node * node;
	char * name;
}

/* Your tokens here. */
%token <node> Error
%token <node> Add
%token <node> Sub
%token <node> Mul
%token <node> Div
%token <node> Mod
%token <node> LT
%token <node> LTE
%token <node> GT
%token <node> GTE
%token <node> Eq
%token <node> NEq
%token <node> Not
%token <node> And
%token <node> Or
%token <node> Assign
%token <node> Semicolon
%token <node> Comma
%token <node> LParenthese
%token <node> RParenthese
%token <node> LBracket
%token <node> RBracket
%token <node> LBrace
%token <node> RBrace
%token <node> Const
%token <node> Int
%token <node> Float
%token <node> If
%token <node> Else
%token <node> Return
%token <node> Void
%token <node> While
%token <node> Break
%token <node> Continue
%token <node> Ident
%token <node> DecIntConst
%token <node> OctIntConst
%token <node> HexIntConst
%token <node> DecFloatConst
%token <node> HexFloatConst

%left Comma
%left Or
%left And
%left Eq NEq
%left LT LTE GT GTE
%left Add Sub
%left Mul Div Mod

%right Assign
%right Not

%nonassoc Epsilon
%nonassoc Else

%type <node> Program CompUnit
%type <node> Decl ConstDecl BType ConstDef ConstInitVal VarDecl VarDef InitVal
%type <node> FuncDef FuncFParams FuncFParam
%type <node> Block BlockItem Stmt
%type <node> Exp Cond LVal PrimaryExp DecIntNum OctIntNum HexIntNum DecFloatNum HexFloatNum UnaryExp UnaryOp
%type <node> FuncRParams MulExp AddExp RelExp EqExp LAndExp LOrExp ConstExp

// List系列
%type <node> ConstDefList ConstExpList ConstInitValList VarDefList InitValList ExpList BlockItemList

%start Program

%%

Program         :   CompUnit { $$=node("Program",1,$1); gt->root=$$; }
                ;
CompUnit        :   CompUnit Decl { $$=node("CompUnit",2,$1,$2); }
                |   CompUnit FuncDef { $$=node("CompUnit",2,$1,$2); }
                |   Decl { $$=node("CompUnit",1,$1); }
                |   FuncDef { $$=node("CompUnit",1,$1); }
                ;
BType           :   Int { $$=node("Btype",1,$1); }
                |   Float { $$=node("Btype",1,$1); }
                ;
Decl            :   ConstDecl { $$=node("Decl",1,$1); }
                |   VarDecl { $$=node("Decl",1,$1); }
                ;
ConstDecl       :   Const BType ConstDefList Semicolon { $$=node("ConstDecl",4,$1,$2,$3,$4); }
                ;
ConstDefList    :   ConstDef {$$=node("ConstDefList",1,$1);}
                |   ConstDefList Comma ConstDef { $$=node("ConstDefList",3,$1,$2,$3); }
                ;
ConstDef        :   Ident ConstExpList Assign ConstInitVal { $$=node("ConstDef",4,$1,$2,$3,$4); }
                ;
ConstExpList    :   ConstExpList LBracket ConstExp RBracket { $$=node("ConstExpList",4,$1,$2,$3,$4); }
                |   { $$=node("ConstExpList",0); }
                ;
ConstInitVal    :   ConstExp { $$=node("ConstInitVal",1,$1); }
                |   LBrace RBrace { $$=node("ConstInitVal",2,$1,$2); }
                |   LBrace ConstInitValList RBrace { $$=node("ConstInitVal",3,$1,$2,$3); }
                ;
ConstInitValList:   ConstInitVal { $$=node("ConstInitValList",1,$1); }
                |   ConstInitValList Comma ConstInitVal { $$=node("ConstInitValList",3,$1,$2,$3); }
                ;
VarDecl         :   BType VarDefList Semicolon { $$=node("VarDecl",3,$1,$2,$3); }
                ;
VarDefList      :   VarDef { $$=node("VarDefList",1,$1); }
                |   VarDefList Comma VarDef { $$=node("VarDefList",3,$1,$2,$3); }
                ;
VarDef          :   Ident ConstExpList { $$=node("VarDef",2,$1,$2); }
                |   Ident ConstExpList Assign InitVal { $$=node("VarDef",4,$1,$2,$3,$4); }
                ;
InitVal         :   Exp { $$=node("InitVal",1,$1); }
                |   LBrace RBrace { $$=node("InitVal",2,$1,$2); }
                |   LBrace InitValList RBrace { $$=node("InitVal",3,$1,$2,$3); }
                ;
InitValList     :   InitVal { $$=node("InitValList",1,$1); }
                |   InitValList Comma InitVal { $$=node("InitValList",3,$1,$2,$3); }
                ;
FuncDef         :   BType Ident LParenthese RParenthese Block { $$=node("FuncDef",5,$1,$2,$3,$4,$5); }
                |   Void Ident LParenthese RParenthese Block { $$=node("FuncDef",5,$1,$2,$3,$4,$5); }
                |   BType Ident LParenthese FuncFParams RParenthese Block { $$=node("FuncDef",6,$1,$2,$3,$4,$5,$6); }
                |   Void Ident LParenthese FuncFParams RParenthese Block { $$=node("FuncDef",6,$1,$2,$3,$4,$5,$6); }
                ;

FuncFParams     :   FuncFParam { $$=node("FuncFParams",1,$1); }
                |   FuncFParams Comma FuncFParam { $$=node("FuncFParams",3,$1,$2,$3); }
                ;
FuncFParam      :   BType Ident { $$=node("FuncFParam",2,$1,$2); }
                |   BType Ident LBracket RBracket ExpList { $$=node("FuncFParam",5,$1,$2,$3,$4,$5); }
                ;
ExpList         :   ExpList LBracket Exp RBracket { $$=node("ExpList",4,$1,$2,$3,$4); }
                |   { $$=node("ExpList",0); }
                ;
Block           :   LBrace BlockItemList RBrace { $$=node("Block",3,$1,$2,$3); }
                ;
BlockItemList   :   BlockItemList BlockItem { $$=node("BlockItemList",2,$1,$2); }
                |   { $$=node("BlockItemList",0); }
                ;
BlockItem       :   Decl { $$=node("BlockItem",1,$1); }
                |   Stmt { $$=node("BlockItem",1,$1); }
                ;
Stmt            :   LVal Assign Exp Semicolon { $$=node("Stmt",4,$1,$2,$3,$4); }
                |   Exp Semicolon { $$=node("Stmt",2,$1,$2); }
                |   Semicolon { $$=node("Stmt",1,$1); }
                |   Block { $$=node("Stmt",1,$1); }
                |   If LParenthese Cond RParenthese Stmt %prec Epsilon { $$=node("Stmt",5,$1,$2,$3,$4,$5); }
                |   If LParenthese Cond RParenthese Stmt Else Stmt { $$=node("Stmt",7,$1,$2,$3,$4,$5,$6,$7); }
                |   While LParenthese Cond RParenthese Stmt { $$=node("Stmt",5,$1,$2,$3,$4,$5); }
                |   Break Semicolon { $$=node("Stmt",2,$1,$2); }
                |   Continue Semicolon { $$=node("Stmt",2,$1,$2); }
                |   Return Semicolon { $$=node("Stmt",2,$1,$2); }
                |   Return Exp Semicolon { $$=node("Stmt",3,$1,$2,$3); }
                ;
Exp             :   AddExp { $$=node("Exp",1,$1); }
                ;
Cond            :   LOrExp { $$=node("Cond",1,$1); }
                ;
LVal            :   Ident ExpList { $$=node("LVal",2,$1,$2); }
                ;
PrimaryExp      :   LParenthese Exp RParenthese { $$=node("PrimaryExp",3,$1,$2,$3); }
                |   LVal { $$=node("PrimaryExp",1,$1); }
                |   DecIntNum { $$=node("PrimaryExp",1,$1); }
                |   OctIntNum { $$=node("PrimaryExp",1,$1); }
                |   HexIntNum { $$=node("PrimaryExp",1,$1); }
                |   DecFloatNum { $$=node("PrimaryExp",1,$1); }
                |   HexFloatNum { $$=node("PrimaryExp",1,$1); }
                ;
DecIntNum       :   DecIntConst { $$=node("DecIntNum",1,$1); }
                ;
OctIntNum       :   OctIntConst { $$=node("OctIntNum",1,$1); }
                ;
HexIntNum       :   HexIntConst { $$=node("HexIntNum",1,$1); }
                ;
DecFloatNum     :   DecFloatConst { $$=node("DecFloatNum",1,$1); }
                ;
HexFloatNum     :   HexFloatConst { $$=node("HexFloatNum",1,$1); }
                ;
UnaryExp        :   PrimaryExp { $$=node("UnaryExp",1,$1); }
                |   Ident LParenthese RParenthese { $$=node("UnaryExp",3,$1,$2,$3); }
                |   Ident LParenthese FuncRParams RParenthese { $$=node("UnaryExp",4,$1,$2,$3,$4); }
                |   UnaryOp UnaryExp { $$=node("UnaryExp",2,$1,$2); }
                ;
UnaryOp         :   Add { $$=node("UnaryOp",1,$1); }
                |   Sub { $$=node("UnaryOp",1,$1); }
                |   Not { $$=node("UnaryOp",1,$1); }
                ;
FuncRParams     :   Exp { $$=node("FuncRParams",1,$1); }
                |   FuncRParams Comma Exp { $$=node("FuncRParams",3,$1,$2,$3); }
                ;
MulExp          :   UnaryExp { $$=node("MulExp",1,$1); }
                |   MulExp Mul UnaryExp { $$=node("MulExp",3,$1,$2,$3); }
                |   MulExp Div UnaryExp { $$=node("MulExp",3,$1,$2,$3); }
                |   MulExp Mod UnaryExp { $$=node("MulExp",3,$1,$2,$3); }
                ;
AddExp          :   MulExp { $$=node("AddExp",1,$1); }
                |   AddExp Add MulExp { $$=node("AddExp",3,$1,$2,$3); }
                |   AddExp Sub MulExp { $$=node("AddExp",3,$1,$2,$3); }
                ;
RelExp          :   AddExp { $$=node("RelExp",1,$1); }
                |   RelExp LT AddExp { $$=node("RelExp",3,$1,$2,$3); }
                |   RelExp LTE AddExp { $$=node("RelExp",3,$1,$2,$3); }
                |   RelExp GT AddExp { $$=node("RelExp",3,$1,$2,$3); }
                |   RelExp GTE AddExp { $$=node("RelExp",3,$1,$2,$3); }
                ;
EqExp           :   RelExp { $$=node("EqExp",1,$1); }
                |   EqExp Eq RelExp { $$=node("EqExp",3,$1,$2,$3); }
                |   EqExp NEq RelExp { $$=node("EqExp",3,$1,$2,$3); }
                ;
LAndExp         :   EqExp { $$=node("LAndExp",1,$1); }
                |   LAndExp And EqExp { $$=node("LAndExp",3,$1,$2,$3); }
                ;
LOrExp          :   LAndExp { $$=node("LOrExp",1,$1); }
                |   LOrExp Or LAndExp { $$=node("LOrExp",3,$1,$2,$3); }
                ;
ConstExp        :   AddExp { $$=node("ConstExp",1,$1); }
                ;

%%

// The error reporting function.
void yyerror(const char * s) {
    fprintf(stderr, "error at line %d column %d: %s\n", lines, pos_start, s);
}

// Parse input from file `input_path`, and prints the parsing results
// to stdout.  If input_path is NULL, read from stdin.
//
// This function initializes essential states before running yyparse().
syntax_tree * parse(const char * input_path) {
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

// A helper function to quickly construct a tree node.
//
// e.g. $$ = node("Program", 1, $1);
syntax_tree_node * node(const char *name, int children_num, ...) {
    syntax_tree_node * p = new_syntax_tree_node(name);
    syntax_tree_node * child;
    // 这里表示 epsilon 结点是通过 children_num == 0 来判断的
    if (children_num == 0) {
        child = new_syntax_tree_node("epsilon");
        syntax_tree_add_child(p, child);
    } else {
        va_list ap;
        va_start(ap, children_num);
        for (int i = 0; i < children_num; i++) {
            child = va_arg(ap, syntax_tree_node *);
            syntax_tree_add_child(p, child);
        }
        va_end(ap);
    }
    return p;
}
