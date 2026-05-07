%define parse.error verbose
%locations

%{
#include <memory>
#include <cstring>
#include <stdarg.h>
#include "ast.hpp"

using namespace std;

unique_ptr<CompUnitAST> root; /* the top level root node of our final AST */

extern int yylineno;
extern int yylex();
extern void yyerror(const char *s);
extern void initFileName(char *name);
char filename[100];
%}

%union {
    CompUnitAST* comp_unit;
    DeclDefAST* decl_def;
    DeclAST* decl;
    DefListAST* def_list;
    DefAST* def;
    ArraysAST* arrays;
    InitValListAST* init_val_list;
    InitValAST* init_val;
    FuncDefAST* func_def;
    FuncParamListAST* func_param_list;
    FuncParamAST* func_param;
    BlockAST* block;
    BlockItemListAST* block_item_list;
    BlockItemAST* block_it;
    StmtAST* stmt;
    ReturnStmtAST* ret_stmt;
    CondStmtAST* cond_stmt;
    LoopStmtAST* loop_stmt;
    LValAST* l_val;
    PrimaryExpAST* pri_exp;
    NumberAST* number;
    UnaryExpAST* unary_exp;
    CallAST* call;
    FuncArgListAST* func_arg_list;
    MulExpAST* mul_exp;
    AddExpAST* add_exp;
    RelExpAST* rel_exp;
    EqExpAST* eq_exp;
    LAndExpAST* l_and_exp;
    LOrExpAST* l_or_exp;

    TYPE type;
    UOP op;
    string* token;
    int int_val;
    float float_val;
};

%type <comp_unit> CompUnit;
%type <decl_def> DeclDef;
%type <decl> Decl;
%type <def_list> DefList;
%type <def> Def;
%type <arrays> Arrays;
%type <init_val_list> InitValList;
%type <init_val> InitVal;
%type <func_def> FuncDef;
%type <func_param_list> FuncParamList
%type <func_param> FuncParam;
%type <block> Block;
%type <block_item_list> BlockItemList;
%type <block_it> BlockItem;
%type <stmt> Stmt;
%type <ret_stmt> ReturnStmt;
%type <cond_stmt> ConditionStmt;
%type <loop_stmt> LoopStmt;
%type <l_val> LVal;
%type <pri_exp> PrimaryExp;
%type <number> Number;
%type <unary_exp> UnaryExp;
%type <call> Call;
%type <func_arg_list> FuncArgList;
%type <mul_exp> MulExp;
%type <add_exp> Exp AddExp;
%type <rel_exp> RelExp;
%type <eq_exp> EqExp;
%type <l_and_exp> LAndExp;
%type <l_or_exp> Cond LOrExp;
%type <type> BType VoidType
%type <op> UnaryOp

// 定义终结符的语义值
%token <int_val> INT                                // int 字面量，直接得到值
%token <float_val> FLOAT                            // float 字面量，直接得到值
%token INTTYPE FLOATTYPE VOID                       // 数据类型
%token CONST RETURN IF ELSE WHILE BREAK CONTINUE    // 关键字
%token <token> ID                                   // 指定ID
%token GTE LTE GT LT EQ NEQ                         // 关系运算
%token LP RP LB RB LC RC COMMA SEMICOLON            // 符号
%token NOT ASSIGN MINUS ADD MUL DIV MOD AND OR      // 算术运算符

// 指定符号的结合性
%left ASSIGN
%left OR AND
%left EQ NEQ
%left GTE LTE GT LT
%left ADD MINUS
%left MOD MUL DIV
%right NOT

// else 匹配规则：与最近的 if 匹配
%nonassoc LOWER_THEN_ELSE
%nonassoc ELSE

// 文法开始符号
%start Program

%%
Program:
    CompUnit {
        root = unique_ptr<CompUnitAST>($1);
    };

// 编译单元
CompUnit:
    CompUnit DeclDef {
        $$ = $1;
        $$->decl_def_list.push_back(unique_ptr<DeclDefAST>($2));
    }|
    DeclDef {
        $$ = new CompUnitAST();
        $$->decl_def_list.push_back(unique_ptr<DeclDefAST>($1));
    };

//声明或者函数定义
DeclDef:
    Decl {
        $$ = new DeclDefAST();
        $$->decl = unique_ptr<DeclAST>($1);
    }|
    FuncDef {
        $$ = new DeclDefAST();
        $$->func_def = unique_ptr<FuncDefAST>($1);
    };

// 变量或常量声明
Decl:
    CONST BType DefList SEMICOLON {
        $$ = new DeclAST();
        $$->is_const = true;
        $$->type = $2;
        $$->def_list.swap($3->list);
    }|
    BType DefList SEMICOLON {
        $$ = new DeclAST();
        $$->is_const = false;
        $$->type = $1;
        $$->def_list.swap($2->list);
    };

// 基本类型
BType:
    INTTYPE {
        $$ = TYPE_INT;
    }|
    FLOATTYPE {
        $$ = TYPE_FLOAT;
    };

// 空类型
VoidType:
    VOID {
        $$ = TYPE_VOID;
    };

// 定义列表
DefList:
    Def {
        $$ = new DefListAST();
        $$->list.push_back(unique_ptr<DefAST>($1));
    }|
    DefList COMMA Def {
        $$ = $1;
        $$->list.push_back(unique_ptr<DefAST>($3));
    };

// 定义
Def:
    ID Arrays ASSIGN InitVal {
        $$ = new DefAST();
        $$->id = unique_ptr<string>($1);
        $$->arrays.swap($2->list);
        $$->init_val = unique_ptr<InitValAST>($4);
    }|
    ID ASSIGN InitVal {
        $$ = new DefAST();
        $$->id = unique_ptr<string>($1);
        $$->init_val = unique_ptr<InitValAST>($3);
    }|
    ID Arrays {
        $$ = new DefAST();
        $$->id = unique_ptr<string>($1);
        $$->arrays.swap($2->list);
    }|
    ID {
        $$ = new DefAST();
        $$->id = unique_ptr<string>($1);
    };

// 数组
Arrays:
    LB Exp RB {
        $$ = new ArraysAST();
        $$->list.push_back(unique_ptr<AddExpAST>($2));
    }|
    Arrays LB Exp RB {
        $$ = $1;
        $$->list.push_back(unique_ptr<AddExpAST>($3));
    };

// 变量或常量初值
InitVal:
    Exp {
        $$ = new InitValAST();
        $$->exp = unique_ptr<AddExpAST>($1);
    }|
    LC RC {
        $$ = new InitValAST();
    }|
    LC InitValList RC {
        $$ = new InitValAST();
        $$->init_val_list.swap($2->list);
    };

// 变量列表
InitValList:
    InitValList COMMA InitVal {
        $$ = $1;
        $$->list.push_back(unique_ptr<InitValAST>($3));
    }|
    InitVal {
        $$ = new InitValListAST();
        $$->list.push_back(unique_ptr<InitValAST>($1));
    };

// 函数定义
FuncDef:
    BType ID LP FuncParamList RP Block {
        $$ = new FuncDefAST();
        $$->func_type = $1;
        $$->id = unique_ptr<string>($2);
        $$->func_param_list.swap($4->list);
        $$->block = unique_ptr<BlockAST>($6);
    }|
    BType ID LP RP Block {
        $$ = new FuncDefAST();
        $$->func_type = $1;
        $$->id = unique_ptr<string>($2);
        $$->block = unique_ptr<BlockAST>($5);
    }|
    VoidType ID LP FuncParamList RP Block {
        $$ = new FuncDefAST();
        $$->func_type = $1;
        $$->id = unique_ptr<string>($2);
        $$->func_param_list.swap($4->list);
        $$->block = unique_ptr<BlockAST>($6);
    }|
    VoidType ID LP RP Block {
        $$ = new FuncDefAST();
        $$->func_type = $1;
        $$->id = unique_ptr<string>($2);
        $$->block = unique_ptr<BlockAST>($5);
    };

// 函数形参列表
FuncParamList:
    FuncParam {
        $$ = new FuncParamListAST();
        $$->list.push_back(unique_ptr<FuncParamAST>($1));
    }|
    FuncParamList COMMA FuncParam {
        $$ = $1;
        $$->list.push_back(unique_ptr<FuncParamAST>($3));
    };

// 函数形参
FuncParam:
    BType ID {
        $$ = new FuncParamAST();
        $$->type = $1;
        $$->id = unique_ptr<string>($2);
        $$->is_array = false;
    }|
    BType ID LB RB {
        $$ = new FuncParamAST();
        $$->type = $1;
        $$->id = unique_ptr<string>($2);
        $$->is_array = true;
    }|
    BType ID LB RB Arrays {
        $$ = new FuncParamAST();
        $$->type = $1;
        $$->id = unique_ptr<string>($2);
        $$->is_array = true;
        $$->arrays.swap($5->list);
    };

// 语句块
Block:
    LC RC {
        $$ = new BlockAST();
    }|
    LC BlockItemList RC {
        $$ = new BlockAST();
        $$->block_item_list.swap($2->list);
    };

// 语句块项列表
BlockItemList:
    BlockItem {
        $$ = new BlockItemListAST();
        $$->list.push_back(unique_ptr<BlockItemAST>($1));
    }|
    BlockItemList BlockItem {
        $$ = $1;
        $$->list.push_back(unique_ptr<BlockItemAST>($2));
    };

// 语句块项
BlockItem:
    Decl {
        $$ = new BlockItemAST();
        $$->decl = unique_ptr<DeclAST>($1);
    }|
    Stmt {
        $$ = new BlockItemAST();
        $$->stmt = unique_ptr<StmtAST>($1);
    };

// 语句，根据type判断是何种类型的Stmt
Stmt:
    SEMICOLON {
        $$ = new StmtAST();
        $$->type = SEMI;
    }|
    LVal ASSIGN Exp SEMICOLON {
        $$ = new StmtAST();
        $$->type = ASS;
        $$->l_val = unique_ptr<LValAST>($1);
        $$->exp = unique_ptr<AddExpAST>($3);
    }|
    Exp SEMICOLON {
        $$ = new StmtAST();
        $$->type = EXP;
        $$->exp = unique_ptr<AddExpAST>($1);
    }|
    CONTINUE SEMICOLON {
        $$ = new StmtAST();
        $$->type = CONT;
    }|
    BREAK SEMICOLON {
        $$ = new StmtAST();
        $$->type = BRK;
    }|
    Block {
        $$ = new StmtAST();
        $$->type = BLK;
        $$->block = unique_ptr<BlockAST>($1);
    }|
    ReturnStmt {
        $$ = new StmtAST();
        $$->type = RET;
        $$->ret_stmt = unique_ptr<ReturnStmtAST>($1);
    }|
    ConditionStmt {
        $$ = new StmtAST();
        $$->type = COND;
        $$->cond_stmt = unique_ptr<CondStmtAST>($1);
    }|
    LoopStmt {
        $$ = new StmtAST();
        $$->type = LOOP;
        $$->loop_stmt = unique_ptr<LoopStmtAST>($1);
    };

//选择语句
ConditionStmt:
    IF LP Cond RP Stmt %prec LOWER_THEN_ELSE {
        $$ = new CondStmtAST();
        $$->cond = unique_ptr<LOrExpAST>($3);
        $$->if_stmt = unique_ptr<StmtAST>($5);
    }|
    IF LP Cond RP Stmt ELSE Stmt {
        $$ = new CondStmtAST();
        $$->cond = unique_ptr<LOrExpAST>($3);
        $$->if_stmt = unique_ptr<StmtAST>($5);
        $$->else_stmt = unique_ptr<StmtAST>($7);
    };

//循环语句
LoopStmt:
    WHILE LP Cond RP Stmt {
        $$ = new LoopStmtAST();
        $$->cond = unique_ptr<LOrExpAST>($3);
        $$->stmt = unique_ptr<StmtAST>($5);
    };

//返回语句
ReturnStmt:
    RETURN Exp SEMICOLON {
        $$ = new ReturnStmtAST();
        $$->exp = unique_ptr<AddExpAST>($2);
    }|
    RETURN SEMICOLON {
        $$ = new ReturnStmtAST();
    };

// 表达式
Exp:
    AddExp {
        $$ = $1;
    };

// 条件表达式
Cond:
    LOrExp {
        $$ = $1;
    };

// 左值表达式
LVal:
    ID {
        $$ = new LValAST();
        $$->id = unique_ptr<string>($1);
    }|
    ID Arrays {
        $$ = new LValAST();
        $$->id = unique_ptr<string>($1);
        $$->arrays.swap($2->list);
    };

// 基本表达式
PrimaryExp:
    LP Exp RP {
        $$ = new PrimaryExpAST();
        $$->exp = unique_ptr<AddExpAST>($2);
    }|
    LVal {
        $$ = new PrimaryExpAST();
        $$->l_val = unique_ptr<LValAST>($1);
    }|
    Number {
        $$ = new PrimaryExpAST();
        $$->number = unique_ptr<NumberAST>($1);
    };

// 数值
Number:
    INT {
        $$ = new NumberAST();
        $$->is_int = true;
        $$->int_val = $1;
    }|
    FLOAT {
        $$ = new NumberAST();
        $$->is_int = false;
        $$->float_val = $1;
    };

// 一元表达式
UnaryExp:
    PrimaryExp {
        $$ = new UnaryExpAST();
        $$->pri_exp = unique_ptr<PrimaryExpAST>($1);
    }|
    Call {
        $$ = new UnaryExpAST();
        $$->call = unique_ptr<CallAST>($1);
    }|
    UnaryOp UnaryExp {
        $$ = new UnaryExpAST();
        $$->op = $1;
        $$->unary_exp = unique_ptr<UnaryExpAST>($2);
    };

//函数调用
Call:
    ID LP RP {
        $$ = new CallAST();
        $$->id = unique_ptr<string>($1);
    }|
    ID LP FuncArgList RP {
        $$ = new CallAST();
        $$->id = unique_ptr<string>($1);
        $$->func_arg_list.swap($3->list);
    };

// 单目运算符,这里可能与优先级相关，不删除该非终结符
UnaryOp:
    ADD {
        $$ = UOP_ADD;
    }|
    MINUS {
        $$ = UOP_MINUS;
    }|
    NOT {
        $$ = UOP_NOT;
    };

// 函数实参表
FuncArgList:
    Exp {
        $$ = new FuncArgListAST();
        $$->list.push_back(unique_ptr<AddExpAST>($1));
    }|
    FuncArgList COMMA Exp {
        $$ = (FuncArgListAST*) $1;
        $$->list.push_back(unique_ptr<AddExpAST>($3));
    };

//乘除模表达式
MulExp:
    UnaryExp {
        $$ = new MulExpAST();
        $$->unary_exp = unique_ptr<UnaryExpAST>($1);
    }|
    MulExp MUL UnaryExp {
        $$ = new MulExpAST();
        $$->mul_exp = unique_ptr<MulExpAST>($1);
        $$->op = MOP_MUL;
        $$->unary_exp = unique_ptr<UnaryExpAST>($3);
    }|
    MulExp DIV UnaryExp {
        $$ = new MulExpAST();
        $$->mul_exp = unique_ptr<MulExpAST>($1);
        $$->op = MOP_DIV;
        $$->unary_exp = unique_ptr<UnaryExpAST>($3);
    }|
    MulExp MOD UnaryExp {
        $$ = new MulExpAST();
        $$->mul_exp = unique_ptr<MulExpAST>($1);
        $$->op = MOP_MOD;
        $$->unary_exp = unique_ptr<UnaryExpAST>($3);
    };

// 加减表达式
AddExp:
    MulExp {
        $$ = new AddExpAST();
        $$->mul_exp = unique_ptr<MulExpAST>($1);
    }|
    AddExp ADD MulExp {
        $$ = new AddExpAST();
        $$->add_exp = unique_ptr<AddExpAST>($1);
        $$->op = AOP_ADD;
        $$->mul_exp = unique_ptr<MulExpAST>($3);
    }|
    AddExp MINUS MulExp {
        $$ = new AddExpAST();
        $$->add_exp = unique_ptr<AddExpAST>($1);
        $$->op = AOP_MINUS;
        $$->mul_exp = unique_ptr<MulExpAST>($3);
    };

// 关系表达式
RelExp:
    AddExp {
        $$ = new RelExpAST();
        $$->add_exp = unique_ptr<AddExpAST>($1);
    }|
    RelExp GTE AddExp {
        $$ = new RelExpAST();
        $$->rel_exp = unique_ptr<RelExpAST>($1);
        $$->op = ROP_GTE;
        $$->add_exp = unique_ptr<AddExpAST>($3);
    }|
    RelExp LTE AddExp {
        $$ = new RelExpAST();
        $$->rel_exp = unique_ptr<RelExpAST>($1);
        $$->op = ROP_LTE;
        $$->add_exp = unique_ptr<AddExpAST>($3);
    }|
    RelExp GT AddExp {
        $$ = new RelExpAST();
        $$->rel_exp = unique_ptr<RelExpAST>($1);
        $$->op = ROP_GT;
        $$->add_exp = unique_ptr<AddExpAST>($3);
    }|
    RelExp LT AddExp {
        $$ = new RelExpAST();
        $$->rel_exp = unique_ptr<RelExpAST>($1);
        $$->op = ROP_LT;
        $$->add_exp = unique_ptr<AddExpAST>($3);
    };

// 相等性表达式
EqExp:
    RelExp {
        $$ = new EqExpAST();
        $$->rel_exp = unique_ptr<RelExpAST>($1);
    }|
    EqExp EQ RelExp {
        $$ = new EqExpAST();
        $$->eq_exp = unique_ptr<EqExpAST>($1);
        $$->op = EOP_EQ;
        $$->rel_exp = unique_ptr<RelExpAST>($3);
    }|
    EqExp NEQ RelExp {
        $$ = new EqExpAST();
        $$->eq_exp = unique_ptr<EqExpAST>($1);
        $$->op = EOP_NEQ;
        $$->rel_exp = unique_ptr<RelExpAST>($3);
    };

// 逻辑与表达式
LAndExp:
    EqExp {
        $$ = new LAndExpAST();
        $$->eq_exp = unique_ptr<EqExpAST>($1);
    }|
    LAndExp AND EqExp {
        $$ = new LAndExpAST();
        $$->l_and_exp = unique_ptr<LAndExpAST>($1);
        $$->eq_exp = unique_ptr<EqExpAST>($3);
    };

// 逻辑或表达式
LOrExp:
    LAndExp {
        $$ = new LOrExpAST();
        $$->l_and_exp = unique_ptr<LAndExpAST>($1);
    }|
    LOrExp OR LAndExp {
        $$ = new LOrExpAST();
        $$->l_or_exp = unique_ptr<LOrExpAST>($1);
        $$->l_and_exp = unique_ptr<LAndExpAST>($3);
    };
%%

void initFileName(char *name) {
    strcpy(filename, name);
}

void yyerror(const char* fmt) {
    printf("%s:%d ", filename, yylloc.first_line);
    printf("%s\n", fmt);
}
