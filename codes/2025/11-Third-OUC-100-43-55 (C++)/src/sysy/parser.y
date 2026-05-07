%define parse.error verbose
%locations

%{
    #include <cstring>
    #include <memory>
    #include <stdarg.h>

    #include "ast.h"
    #include "define.h"

    using namespace std;
    unique_ptr<CompUnit> root;

    extern int yylineno;
    extern int yylex();
    extern void yyerror(const char *s);
    extern void initFileName(char *name);
    char filename[100];
%}


%union {
    CompUnit* compUnit;
    DeclDef* declDef;
    Decl* decl;
    DefList* defList;
    Def* def;
    ArrayList* arrays;
    InitValList* initValList;
    InitVal* initVal;
    FuncDef* funcDef;
    FuncFParamList* funcFParamList;
    FuncFParam* funcFParam;
    Block* block;
    BlockItemList* blockItemList;
    BlockItem* blockItem;
    Stmt* stmt;
    ReturnStmt* returnStmt;
    IfStmt* ifStmt;
    WhileStmt* whileStmtAST;
    LVal* lVal;
    PrimaryExp* primaryExp;
    NumberNode* number;
    UnaryExp* unaryExp;
    Call* call;
    FuncCParamList* funcCParamList;
    MulExp* mulExp;
    AddExp* addExp;
    RelExp* relExp;
    EqExp* eqExp;
    LAndExp* lAndExp;
    LOrExp* lOrExp;

    BType ty;
    UnaryOp op;
    string* token;
    int int_val;
    float float_val;
};

%type <compUnit> CompUnit;
%type <declDef> DeclDef;
%type <decl> Decl;
%type <defList> DefList;
%type <def> Def;
%type <arrays> Arrays;
%type <initValList> InitValList;
%type <initVal> InitVal;
%type <funcDef> FuncDef;
%type <funcFParamList> FuncFParamList
%type <funcFParam> FuncFParam;
%type <block> Block;
%type <blockItemList> BlockItemList;
%type <blockItem> BlockItem;
%type <stmt> Stmt;
%type <returnStmt> ReturnStmt;
%type <ifStmt> IfStmt;
%type <whileStmtAST> IterationStmt;
%type <lVal> LVal;
%type <primaryExp> PrimaryExp;
%type <number> Number;
%type <unaryExp> UnaryExp;
%type <call> Call;
%type <funcCParamList> FuncCParamList;
%type <mulExp> MulExp;
%type <addExp> Exp AddExp;
%type <relExp> RelExp;
%type <eqExp> EqExp;
%type <lAndExp> LAndExp;
%type <lOrExp> Cond LOrExp;

%type <ty> BType VoidType
%type <op> UnaryOp

// %token 定义终结符的语义值类型
%token <int_val> INT           // 指定INT字面量的语义值是type_int，有词法分析得到的数值
%token <float_val> FLOAT       // 指定FLOAT字面量的语义值是type_float，有词法分析得到的数值
%token <token> ID              // 指定ID
%token GTE LTE GT LT EQ NEQ    // 关系运算
%token INTTYPE FLOATTYPE VOID  // 数据类型
%token CONST RETURN IF ELSE WHILE BREAK CONTINUE
%token LP RP LB RB LC RC COMMA SEMICOLON
%token NOT ASSIGN MINUS ADD MUL DIV MOD AND OR
// Unused tokens
/* %token POS NEG */

%left ASSIGN
%left OR AND
%left EQ NEQ
%left GTE LTE GT LT
%left ADD MINUS
%left MOD MUL DIV
%right NOT
// Unused tokens
/* %right POS NEG */

%nonassoc LOWER_THEN_ELSE
%nonassoc ELSE

%start Program

%%
Program:
    CompUnit {
        root = unique_ptr<CompUnit>($1);
    };

// 编译单元
CompUnit:
    CompUnit DeclDef {
        $$ = $1;
        $$->declDefList.push_back(unique_ptr<DeclDef>($2));
    }|
    DeclDef {
        $$ = new CompUnit();
        $$->declDefList.push_back(unique_ptr<DeclDef>($1));
    };

//声明或者函数定义
DeclDef:
    Decl {
        $$ = new DeclDef();
        $$->decl = unique_ptr<Decl>($1);
    }|
    FuncDef {
        $$ = new DeclDef();
        $$->funcDef = unique_ptr<FuncDef>($1);
    };

// 变量或常量声明
Decl:
    CONST BType DefList SEMICOLON {
        $$ = new Decl();
        $$->isConst = true;
        $$->bType = $2;
        $$->defList.swap($3->list);
    }|
    BType DefList SEMICOLON {
        $$ = new Decl();
        $$->isConst = false;
        $$->bType = $1;
        $$->defList.swap($2->list);
    };

// 基本类型
BType:
    INTTYPE {
        $$ = BType::INT;
    }|
    FLOATTYPE {
        $$ = BType::FLOAT;
    };

// 空类型
VoidType:
    VOID {
        $$ = BType::VOID;
    };

// 定义列表
DefList:
    Def {
        $$ = new DefList();
        $$->list.push_back(unique_ptr<Def>($1));
    }|
    DefList COMMA Def {
        $$ = $1;
        $$->list.push_back(unique_ptr<Def>($3));
    };

// 定义
Def:
    ID Arrays ASSIGN InitVal {
        $$ = new Def();
        $$->id = unique_ptr<string>($1);
        $$->arrays.swap($2->list);
        $$->initVal = unique_ptr<InitVal>($4);
    }|
    ID ASSIGN InitVal {
        $$ = new Def();
        $$->id = unique_ptr<string>($1);
        $$->initVal = unique_ptr<InitVal>($3);
    }|
    ID Arrays {
        $$ = new Def();
        $$->id = unique_ptr<string>($1);
        $$->arrays.swap($2->list);
    }|
    ID {
        $$ = new Def();
        $$->id = unique_ptr<string>($1);
    };

// 数组
Arrays:
    LB Exp RB {
        $$ = new ArrayList();
        $$->list.push_back(unique_ptr<AddExp>($2));
    }|
    Arrays LB Exp RB {
        $$ = $1;
        $$->list.push_back(unique_ptr<AddExp>($3));
    };

// 变量或常量初值
InitVal:
    Exp {
        $$ = new InitVal();
        $$->exp = unique_ptr<AddExp>($1);
    }|
    LC RC {
        $$ = new InitVal();
    }|
    LC InitValList RC {
        $$ = new InitVal();
        $$->initValList.swap($2->list);
    };

// 变量列表
InitValList:
    InitValList COMMA InitVal {
        $$ = $1;
        $$->list.push_back(unique_ptr<InitVal>($3));
    }|
    InitVal {
        $$ = new InitValList();
        $$->list.push_back(unique_ptr<InitVal>($1));
    };

// 函数定义
FuncDef:
    BType ID LP FuncFParamList RP Block {
        $$ = new FuncDef();
        $$->returnType = $1;
        $$->id = unique_ptr<string>($2);
        $$->funcFParamList.swap($4->list);
        $$->block = unique_ptr<Block>($6);
    }|
    BType ID LP RP Block {
        $$ = new FuncDef();
        $$->returnType = $1;
        $$->id = unique_ptr<string>($2);
        $$->block = unique_ptr<Block>($5);
    }|
    VoidType ID LP FuncFParamList RP Block {
        $$ = new FuncDef();
        $$->returnType = $1;
        $$->id = unique_ptr<string>($2);
        $$->funcFParamList.swap($4->list);
        $$->block = unique_ptr<Block>($6);
    }|
    VoidType ID LP RP Block {
        $$ = new FuncDef();
        $$->returnType = $1;
        $$->id = unique_ptr<string>($2);
        $$->block = unique_ptr<Block>($5);
    };

// 函数形参列表
FuncFParamList:
    FuncFParam {
        $$ = new FuncFParamList();
        $$->list.push_back(unique_ptr<FuncFParam>($1));
    }|
    FuncFParamList COMMA FuncFParam {
        $$ = $1;
        $$->list.push_back(unique_ptr<FuncFParam>($3));
    };

// 函数形参
FuncFParam:
    BType ID {
        $$ = new FuncFParam();
        $$->bType = $1;
        $$->id = unique_ptr<string>($2);
        $$->isArray = false;
    }|
    BType ID LB RB {
        $$ = new FuncFParam();
        $$->bType = $1;
        $$->id = unique_ptr<string>($2);
        $$->isArray = true;
    }|
    BType ID LB RB Arrays {
        $$ = new FuncFParam();
        $$->bType = $1;
        $$->id = unique_ptr<string>($2);
        $$->isArray = true;
        $$->arrays.swap($5->list);
    };

// 语句块
Block:
    LC RC {
        $$ = new Block();
    }|
    LC BlockItemList RC {
        $$ = new Block();
        $$->blockItemList.swap($2->list);
    };

// 语句块项列表
BlockItemList:
    BlockItem {
        $$ = new BlockItemList();
        $$->list.push_back(unique_ptr<BlockItem>($1));
    }|
    BlockItemList BlockItem {
        $$ = $1;
        $$->list.push_back(unique_ptr<BlockItem>($2));
    };

// 语句块项
BlockItem:
    Decl {
        $$ = new BlockItem();
        $$->decl = unique_ptr<Decl>($1);
    }|
    Stmt {
        $$ = new BlockItem();
        $$->stmt = unique_ptr<Stmt>($1);
    };

Stmt:
    SEMICOLON {
        $$ = new Stmt();
        $$->sType = StmtType::SEMI;
    }|
    LVal ASSIGN Exp SEMICOLON {
        $$ = new Stmt();
        $$->sType = StmtType::ASGN;
        $$->lVal = unique_ptr<LVal>($1);
        $$->exp = unique_ptr<AddExp>($3);
    }|
    Exp SEMICOLON {
        $$ = new Stmt();
        $$->sType = StmtType::EXP;
        $$->exp = unique_ptr<AddExp>($1);
    }|
    CONTINUE SEMICOLON {
        $$ = new Stmt();
        $$->sType = StmtType::CONTINUE;
    }|
    BREAK SEMICOLON {
        $$ = new Stmt();
        $$->sType = StmtType::BREAK;
    }|
    Block {
        $$ = new Stmt();
        $$->sType = StmtType::BLK;
        $$->block = unique_ptr<Block>($1);
    }|
    ReturnStmt {
        $$ = new Stmt();
        $$->sType = StmtType::RET;
        $$->returnStmt = unique_ptr<ReturnStmt>($1);
    }|
    IfStmt {
        $$ = new Stmt();
        $$->sType = StmtType::IF;
        $$->ifStmt = unique_ptr<IfStmt>($1);
    }|
    IterationStmt {
        $$ = new Stmt();
        $$->sType = StmtType::WHILE;
        $$->whileStmtAST = unique_ptr<WhileStmt>($1);
    };

//选择语句
IfStmt:
    IF LP Cond RP Stmt %prec LOWER_THEN_ELSE {
        $$ = new IfStmt();
        $$->cond = unique_ptr<LOrExp>($3);
        $$->ifStmt = unique_ptr<Stmt>($5);
    }|
    IF LP Cond RP Stmt ELSE Stmt {
        $$ = new IfStmt();
        $$->cond = unique_ptr<LOrExp>($3);
        $$->ifStmt = unique_ptr<Stmt>($5);
        $$->elseStmt = unique_ptr<Stmt>($7);
    };

//循环语句
IterationStmt:
    WHILE LP Cond RP Stmt {
        $$ = new WhileStmt();
        $$->cond = unique_ptr<LOrExp>($3);
        $$->stmt = unique_ptr<Stmt>($5);
    };

//返回语句
ReturnStmt:
    RETURN Exp SEMICOLON {
        $$ = new ReturnStmt();
        $$->exp = unique_ptr<AddExp>($2);
    }|
    RETURN SEMICOLON {
        $$ = new ReturnStmt();
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
        $$ = new LVal();
        $$->id = unique_ptr<string>($1);
    }|
    ID Arrays {
        $$ = new LVal();
        $$->id = unique_ptr<string>($1);
        $$->arrays.swap($2->list);
    };

// 基本表达式
PrimaryExp:
    LP Exp RP {
        $$ = new PrimaryExp();
        $$->exp = unique_ptr<AddExp>($2);
    }|
    LVal {
        $$ = new PrimaryExp();
        $$->lval = unique_ptr<LVal>($1);
    }|
    Number {
        $$ = new PrimaryExp();
        $$->number = unique_ptr<NumberNode>($1);
    };

// 数值
Number:
    INT {
        $$ = new NumberNode();
        $$->isInt = true;
        $$->intval = $1;
    }|
    FLOAT {
        $$ = new NumberNode();
        $$->isInt = false;
        $$->floatval = $1;
    };

// 一元表达式
UnaryExp:
    PrimaryExp {
        $$ = new UnaryExp();
        $$->primaryExp = unique_ptr<PrimaryExp>($1);
    }|
    Call {
        $$ = new UnaryExp();
        $$->call = unique_ptr<Call>($1);
    }|
    UnaryOp UnaryExp {
        $$ = new UnaryExp();
        $$->op = $1;
        $$->unaryExp = unique_ptr<UnaryExp>($2);
    };

//函数调用
Call:
    ID LP RP {
        $$ = new Call();
        $$->id = unique_ptr<string>($1);
    }|
    ID LP FuncCParamList RP {
        $$ = new Call();
        $$->id = unique_ptr<string>($1);
        $$->funcCParamList.swap($3->list);
    };

// 单目运算符,这里可能与优先级相关，不删除该非终结符
UnaryOp:
    ADD {
        $$ = UnaryOp::ADD;
    }|
    MINUS {
        $$ = UnaryOp::MINUS;
    }|
    NOT {
        $$ = UnaryOp::NOT;
    };

// 函数实参表
FuncCParamList:
    Exp {
        $$ = new FuncCParamList();
        $$->list.push_back(unique_ptr<AddExp>($1));
    }|
    FuncCParamList COMMA Exp {
        $$ = (FuncCParamList*) $1;
        $$->list.push_back(unique_ptr<AddExp>($3));
    };

//乘除模表达式
MulExp:
    UnaryExp {
        $$ = new MulExp();
        $$->unaryExp = unique_ptr<UnaryExp>($1);
    }|
    MulExp MUL UnaryExp {
        $$ = new MulExp();
        $$->mulExp = unique_ptr<MulExp>($1);
        $$->op = MulOp::MUL;
        $$->unaryExp = unique_ptr<UnaryExp>($3);
    }|
    MulExp DIV UnaryExp {
        $$ = new MulExp();
        $$->mulExp = unique_ptr<MulExp>($1);
        $$->op = MulOp::DIV;
        $$->unaryExp = unique_ptr<UnaryExp>($3);
    }|
    MulExp MOD UnaryExp {
        $$ = new MulExp();
        $$->mulExp = unique_ptr<MulExp>($1);
        $$->op = MulOp::MOD;
        $$->unaryExp = unique_ptr<UnaryExp>($3);
    };

// 加减表达式
AddExp:
    MulExp {
        $$ = new AddExp();
        $$->mulExp = unique_ptr<MulExp>($1);
    }|
    AddExp ADD MulExp {
        $$ = new AddExp();
        $$->addExp = unique_ptr<AddExp>($1);
        $$->op = AddOp::ADD;
        $$->mulExp = unique_ptr<MulExp>($3);
    }|
    AddExp MINUS MulExp {
        $$ = new AddExp();
        $$->addExp = unique_ptr<AddExp>($1);
        $$->op = AddOp::MINUS;
        $$->mulExp = unique_ptr<MulExp>($3);
    };

// 关系表达式
RelExp:
    AddExp {
        $$ = new RelExp();
        $$->addExp = unique_ptr<AddExp>($1);
    }|
    RelExp GTE AddExp {
        $$ = new RelExp();
        $$->relExp = unique_ptr<RelExp>($1);
        $$->op = RelOp::GTE;
        $$->addExp = unique_ptr<AddExp>($3);
    }|
    RelExp LTE AddExp {
        $$ = new RelExp();
        $$->relExp = unique_ptr<RelExp>($1);
        $$->op = RelOp::LTE;
        $$->addExp = unique_ptr<AddExp>($3);
    }|
    RelExp GT AddExp {
        $$ = new RelExp();
        $$->relExp = unique_ptr<RelExp>($1);
        $$->op = RelOp::GT;
        $$->addExp = unique_ptr<AddExp>($3);
    }|
    RelExp LT AddExp {
        $$ = new RelExp();
        $$->relExp = unique_ptr<RelExp>($1);
        $$->op = RelOp::LT;
        $$->addExp = unique_ptr<AddExp>($3);
    };

// 相等性表达式
EqExp:
    RelExp {
        $$ = new EqExp();
        $$->relExp = unique_ptr<RelExp>($1);
    }|
    EqExp EQ RelExp {
        $$ = new EqExp();
        $$->eqExp = unique_ptr<EqExp>($1);
        $$->op = EqOp::EQ;
        $$->relExp = unique_ptr<RelExp>($3);
    }|
    EqExp NEQ RelExp {
        $$ = new EqExp();
        $$->eqExp = unique_ptr<EqExp>($1);
        $$->op = EqOp::NEQ;
        $$->relExp = unique_ptr<RelExp>($3);
    };

// 逻辑与表达式
LAndExp:
    EqExp {
        $$ = new LAndExp();
        $$->eqExp = unique_ptr<EqExp>($1);
    }|
    LAndExp AND EqExp {
        $$ = new LAndExp();
        $$->lAndExp = unique_ptr<LAndExp>($1);
        $$->eqExp = unique_ptr<EqExp>($3);
    };

// 逻辑或表达式
LOrExp:
    LAndExp {
        $$ = new LOrExp();
        $$->lAndExp = unique_ptr<LAndExp>($1);
    }|
    LOrExp OR LAndExp {
        $$ = new LOrExp();
        $$->lOrExp = unique_ptr<LOrExp>($1);
        $$->lAndExp = unique_ptr<LAndExp>($3);
    };
%%

void initFileName(char *name) {
    strcpy(filename, name);
}

void yyerror(const char* fmt) {
    printf("%s:%d ", filename, yylloc.first_line);
    printf("%s\n", fmt);
}
