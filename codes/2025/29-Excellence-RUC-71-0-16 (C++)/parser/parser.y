%{
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>
#include <variant>
#include <optional>
#include "../include/ast.h"

// 声明全局根变量
std::unique_ptr<CompUnit> root;

int yylex();
void yyerror(const char *s);
extern char* yytext;
extern int line_number;
extern int col_number;

// 类型转换辅助函数
template<typename T>
std::unique_ptr<T> make_unique_from_ptr(T* ptr) {
    return std::unique_ptr<T>(ptr);
}
%}

%expect 0  // 期待没有冲突

%union {
    char* str_val;           // 字符串值
    int int_val;             // 整数值
    float float_val;         // 浮点数值
    
    // AST节点指针
    CompUnit* comp_unit;
    Decl* decl;
    ConstDecl* const_decl;
    ConstDef* const_def;
    VarDecl* var_decl;
    VarDef* var_def;
    FuncDef* func_def;
    FuncFParam* func_param;
    Stmt* stmt;
    Block* block;
    Exp* exp;
    LVal* lval;
    InitVal* init_val;
    ConstInitVal* const_init_val;
    StringLiteral* string_literal;
    
    // 列表类型
    std::vector<std::unique_ptr<Decl>>* decl_list;
    std::vector<std::unique_ptr<FuncDef>>* func_list;
    std::vector<std::unique_ptr<ConstDef>>* const_def_list;
    std::vector<std::unique_ptr<VarDef>>* var_def_list;
    std::vector<std::unique_ptr<FuncFParam>>* param_list;
    std::vector<std::unique_ptr<Exp>>* exp_list;
    std::vector<std::unique_ptr<InitVal>>* init_list;
    std::vector<std::unique_ptr<ConstInitVal>>* const_init_list;
    std::vector<std::variant<std::unique_ptr<Stmt>, std::unique_ptr<Decl>>>* block_item_list;
    std::vector<std::variant<std::unique_ptr<Decl>, std::unique_ptr<FuncDef>>>* comp_item_list;
    
    BaseType base_type;      // 基本类型枚举
}

// Token定义
%token <int_val> INT_CONST
%token <float_val> FLOAT_CONST
%token <str_val> IDENT STR_CONST
%token ERROR

// 运算符
%token PLUS MINUS MUL DIV MOD
%token ASSIGN
%token NOT
%token LT GT LEQ GEQ EQ NE
%token AND OR

// 分隔符
%token LPAREN RPAREN
%token LBRACKET RBRACKET  
%token LBRACE RBRACE
%token COMMA SEMI

// 关键字
%token CONST
%token IF ELSE WHILE
%token VOID INT FLOAT
%token RETURN BREAK CONTINUE

// 非终结符类型声明
%type <comp_unit> CompUnit
%type <decl> Decl
%type <const_decl> ConstDecl
%type <const_def> ConstDef
%type <var_decl> VarDecl  
%type <var_def> VarDef
%type <func_def> FuncDef
%type <func_param> FuncFParam
%type <stmt> Stmt IfStmt WhileStmt ExpStmt AssignStmt ReturnStmt BreakStmt ContinueStmt
%type <block> Block
%type <exp> Exp AddExp MulExp UnaryExp PrimaryExp RelExp EqExp LAndExp LOrExp Cond ConstExp FunctionCall Number StringLiteral
%type <lval> LVal
%type <init_val> InitVal
%type <const_init_val> ConstInitVal
%type <base_type> BType

// 列表类型
%type <const_def_list> ConstDefList
%type <var_def_list> VarDefList
%type <param_list> FuncFParams
%type <exp_list> FuncRParams ArrayDims ArrayIndices
%type <init_list> InitValList
%type <const_init_list> ConstInitValList
%type <block_item_list> BlockItemList
%type <comp_item_list> CompItemList

// 运算符优先级和结合性（从低到高）
%left OR
%left AND
%left EQ NE
%left LT GT LEQ GEQ
%left PLUS MINUS
%left MUL DIV MOD
%right UMINUS UPLUS UNOT
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%

// ===--------------------------------------------------------------------=== //
// 完整语法规则
// ===--------------------------------------------------------------------=== //

// CompUnit → CompUnit (Decl | FuncDef) | (Decl | FuncDef)
CompUnit: 
    CompItemList {
        $$ = new CompUnit({line_number});
        $$->items = std::move(*$1);
        delete $1;
        $1 = nullptr;
        root = make_unique_from_ptr($$);
        $$ = nullptr;
    }
    ;

CompItemList:
    Decl {
        $$ = new std::vector<std::variant<std::unique_ptr<Decl>, std::unique_ptr<FuncDef>>>();
        std::variant<std::unique_ptr<Decl>, std::unique_ptr<FuncDef>> item;
        item = make_unique_from_ptr($1);
        $$->push_back(std::move(item));
        $1 = nullptr;
    }
    | FuncDef {
        $$ = new std::vector<std::variant<std::unique_ptr<Decl>, std::unique_ptr<FuncDef>>>();
        std::variant<std::unique_ptr<Decl>, std::unique_ptr<FuncDef>> item;
        item = make_unique_from_ptr($1);
        $$->push_back(std::move(item));
        $1 = nullptr;
    }
    | CompItemList Decl {
        $1->emplace_back(make_unique_from_ptr($2));
        $$ = $1;
        $2 = nullptr;
    }
    | CompItemList FuncDef {
        $1->emplace_back(make_unique_from_ptr($2));
        $$ = $1;
        $2 = nullptr; 
    }
    ;

// Decl → ConstDecl | VarDecl
Decl: 
    ConstDecl { $$ = $1; }
    | VarDecl { $$ = $1; }
    ;

// ConstDecl → 'const' BType ConstDef { ',' ConstDef } ';'
ConstDecl:
    CONST BType ConstDefList SEMI {
        std::vector<std::unique_ptr<ConstDef>> defs;
        for (auto& def : *$3) {
            defs.push_back(std::move(def));
        }
        delete $3;
        $3 = nullptr;
        $$ = new ConstDecl($2, std::move(defs), {line_number});
    }
    ;

ConstDefList:
    ConstDef {
        $$ = new std::vector<std::unique_ptr<ConstDef>>();
        $$->push_back(make_unique_from_ptr($1));
        $1 = nullptr;
    }
    | ConstDefList COMMA ConstDef {
        $1->push_back(make_unique_from_ptr($3));
        $$ = $1;
        $3 = nullptr; 
    }
    ;

// ConstDef → Ident {'[' ConstExp ']'} '=' ConstInitVal
ConstDef:
    IDENT ASSIGN ConstInitVal {
        std::vector<std::unique_ptr<Exp>> dims;
        $$ = new ConstDef(std::string($1), std::move(dims), 
                         make_unique_from_ptr($3), {line_number});
        free($1);
        $3 = nullptr; 
    }
    | IDENT ArrayDims ASSIGN ConstInitVal {
        $$ = new ConstDef(std::string($1), std::move(*$2), 
                         make_unique_from_ptr($4), {line_number});
        free($1);
        delete $2;
        $2 = nullptr;
        $4 = nullptr; 
    }
    ;

// VarDecl → BType VarDef { ',' VarDef } ';'
VarDecl:
    BType VarDefList SEMI {
        std::vector<std::unique_ptr<VarDef>> defs;
        for (auto& def : *$2) {
            defs.push_back(std::move(def));
        }
        delete $2;
        $2 = nullptr;
        $$ = new VarDecl($1, std::move(defs), {line_number});
    }
    ;

VarDefList:
    VarDef {
        $$ = new std::vector<std::unique_ptr<VarDef>>();
        $$->push_back(make_unique_from_ptr($1));
        $1 = nullptr;
    }
    | VarDefList COMMA VarDef {
        $1->push_back(make_unique_from_ptr($3));
        $$ = $1;
        $3 = nullptr;
    }
    ;

// VarDef → Ident {'[' ConstExp ']'} | Ident {'[' ConstExp ']'} '=' InitVal
VarDef:
    IDENT {
        std::vector<std::unique_ptr<Exp>> dims;
        std::optional<std::unique_ptr<InitVal>> init;
        $$ = new VarDef(std::string($1), std::move(dims), std::move(init), {line_number});
        free($1);
    }
    | IDENT ArrayDims {
        std::optional<std::unique_ptr<InitVal>> init;
        $$ = new VarDef(std::string($1), std::move(*$2), std::move(init), {line_number});
        free($1);
        delete $2;
        $2 = nullptr;
    }
    | IDENT ASSIGN InitVal {
        std::vector<std::unique_ptr<Exp>> dims;
        std::optional<std::unique_ptr<InitVal>> init = make_unique_from_ptr($3);
        $$ = new VarDef(std::string($1), std::move(dims), std::move(init), {line_number});
        free($1);
        $3 = nullptr; 
    }
    | IDENT ArrayDims ASSIGN InitVal {
        std::optional<std::unique_ptr<InitVal>> init = make_unique_from_ptr($4);
        $$ = new VarDef(std::string($1), std::move(*$2), std::move(init), {line_number});
        free($1);
        delete $2;
        $2 = nullptr;
        $4 = nullptr; 
    }
    ;

ArrayDims:
    LBRACKET ConstExp RBRACKET {
        $$ = new std::vector<std::unique_ptr<Exp>>();
        $$->push_back(make_unique_from_ptr($2));
        $2 = nullptr;
    }
    | ArrayDims LBRACKET ConstExp RBRACKET {
        $1->push_back(make_unique_from_ptr($3));
        $$ = $1;
        $3 = nullptr; 
    }
    ;

// FuncDef → FuncType Ident '(' [FuncFParams] ')' Block
FuncDef:
    VOID IDENT LPAREN RPAREN Block {
        std::vector<std::unique_ptr<FuncFParam>> params;
        $$ = new FuncDef(BaseType::VOID, std::string($2), std::move(params), 
                        make_unique_from_ptr($5), {line_number});
        free($2);
        $5 = nullptr; 
    }
    | VOID IDENT LPAREN FuncFParams RPAREN Block {
        std::vector<std::unique_ptr<FuncFParam>> params;
        for (auto& param : *$4) {
            params.push_back(std::move(param));
        }
        delete $4;
        $4 = nullptr;
        $$ = new FuncDef(BaseType::VOID, std::string($2), std::move(params), 
                        make_unique_from_ptr($6), {line_number});
        free($2);
        $6 = nullptr; 
    }
    | BType IDENT LPAREN RPAREN Block {
        std::vector<std::unique_ptr<FuncFParam>> params;
        $$ = new FuncDef($1, std::string($2), std::move(params), 
                        make_unique_from_ptr($5), {line_number});
        free($2);
        $5 = nullptr; 
    }
    | BType IDENT LPAREN FuncFParams RPAREN Block {
        std::vector<std::unique_ptr<FuncFParam>> params;
        for (auto& param : *$4) {
            params.push_back(std::move(param));
        }
        delete $4;
        $4 = nullptr;
        $$ = new FuncDef($1, std::string($2), std::move(params), 
                        make_unique_from_ptr($6), {line_number});
        free($2);
        $6 = nullptr; 
    }
    ;

FuncFParams:
    FuncFParam {
        $$ = new std::vector<std::unique_ptr<FuncFParam>>();
        $$->push_back(make_unique_from_ptr($1));
        $1 = nullptr;
    }
    | FuncFParams COMMA FuncFParam {
        $1->push_back(make_unique_from_ptr($3));
        $$ = $1;
        $3 = nullptr;
    }
    ;

// FuncFParam → BType Ident ['[' ']' { '[' Exp ']' }]
FuncFParam:
    BType IDENT {
        std::vector<std::unique_ptr<Exp>> dims;
        $$ = new FuncFParam($1, std::string($2), false, std::move(dims), {line_number});
        free($2);
    }
    | BType IDENT LBRACKET RBRACKET {
        std::vector<std::unique_ptr<Exp>> dims;
        $$ = new FuncFParam($1, std::string($2), true, std::move(dims), {line_number});
        free($2);
    }
    | BType IDENT LBRACKET RBRACKET ArrayDims {
        $$ = new FuncFParam($1, std::string($2), true, std::move(*$5), {line_number});
        free($2);
        delete $5;
        $5 = nullptr;
    }
    ;

// Block → '{' { BlockItem } '}'
Block:
    LBRACE RBRACE {
        $$ = new Block({line_number});
    }
    | LBRACE BlockItemList RBRACE {
        $$ = new Block({line_number});
        $$->items = std::move(*$2);
        delete $2;
        $2 = nullptr;
    }
    ;

BlockItemList:
    Decl {
        $$ = new std::vector<std::variant<std::unique_ptr<Stmt>, std::unique_ptr<Decl>>>();
        std::variant<std::unique_ptr<Stmt>, std::unique_ptr<Decl>> item;
        item = make_unique_from_ptr($1);
        $$->push_back(std::move(item));
        $1 = nullptr; 
    }
    | Stmt {
        $$ = new std::vector<std::variant<std::unique_ptr<Stmt>, std::unique_ptr<Decl>>>();
        std::variant<std::unique_ptr<Stmt>, std::unique_ptr<Decl>> item;
        item = make_unique_from_ptr($1);
        $$->push_back(std::move(item));
        $1 = nullptr; 
    }
    | BlockItemList Decl {
        std::variant<std::unique_ptr<Stmt>, std::unique_ptr<Decl>> item;
        item = make_unique_from_ptr($2);
        $1->push_back(std::move(item));
        $$ = $1;
        $2 = nullptr; 
    }
    | BlockItemList Stmt {
        std::variant<std::unique_ptr<Stmt>, std::unique_ptr<Decl>> item;
        item = make_unique_from_ptr($2);
        $1->push_back(std::move(item));
        $$ = $1;
        $2 = nullptr;
    }
    ;

// Stmt规则
Stmt:
    AssignStmt { $$ = $1; }
    | ExpStmt { $$ = $1; }
    | Block { $$ = $1; }
    | IfStmt { $$ = $1; }
    | WhileStmt { $$ = $1; }
    | BreakStmt { $$ = $1; }
    | ContinueStmt { $$ = $1; }
    | ReturnStmt { $$ = $1; }
    ;

// LVal '=' Exp ';'
AssignStmt:
    LVal ASSIGN Exp SEMI {
        $$ = new AssignStmt(make_unique_from_ptr($1), make_unique_from_ptr($3), {line_number});
        $1 = nullptr; 
        $3 = nullptr; 
    }
    ;

// [Exp] ';'
ExpStmt:
    SEMI {
        std::optional<std::unique_ptr<Exp>> expr;
        $$ = new ExpStmt(std::move(expr), {line_number});
    }
    | Exp SEMI {
        std::optional<std::unique_ptr<Exp>> expr = make_unique_from_ptr($1);
        $$ = new ExpStmt(std::move(expr), {line_number});
        $1 = nullptr; 
    }
    ;

// 'if' '(' Cond ')' Stmt [ 'else' Stmt ]
IfStmt:
    IF LPAREN Cond RPAREN Stmt %prec LOWER_THAN_ELSE {
        std::optional<std::unique_ptr<Stmt>> else_stmt;
        $$ = new IfStmt(make_unique_from_ptr($3), make_unique_from_ptr($5), 
                       std::move(else_stmt), {line_number});
        $3 = nullptr; 
        $5 = nullptr; 
    }
    | IF LPAREN Cond RPAREN Stmt ELSE Stmt {
        std::optional<std::unique_ptr<Stmt>> else_stmt = make_unique_from_ptr($7);
        $$ = new IfStmt(make_unique_from_ptr($3), make_unique_from_ptr($5), 
                       std::move(else_stmt), {line_number});
        $3 = nullptr; 
        $5 = nullptr; 
        $7 = nullptr; 
    }
    ;

// 'while' '(' Cond ')' Stmt
WhileStmt:
    WHILE LPAREN Cond RPAREN Stmt {
        $$ = new WhileStmt(make_unique_from_ptr($3), make_unique_from_ptr($5), {line_number});
        $3 = nullptr;
        $5 = nullptr;  
    }
    ;

// 'break' ';'
BreakStmt:
    BREAK SEMI {
        $$ = new BreakStmt({line_number});
    }
    ;

// 'continue' ';'
ContinueStmt:
    CONTINUE SEMI {
        $$ = new ContinueStmt({line_number});
    }
    ;

// 'return' [Exp] ';'
ReturnStmt:
    RETURN SEMI {
        std::optional<std::unique_ptr<Exp>> expr;
        $$ = new ReturnStmt(std::move(expr), {line_number});
    }
    | RETURN Exp SEMI {
        std::optional<std::unique_ptr<Exp>> expr = make_unique_from_ptr($2);
        $$ = new ReturnStmt(std::move(expr), {line_number});
        $2 = nullptr; 
    }
    ;

// 表达式规则
Exp: 
    AddExp { $$ = $1; }
    ;

// Cond → LOrExp
Cond:
    LOrExp { $$ = $1; }
    ;

// LOrExp → LAndExp | LOrExp '||' LAndExp
LOrExp:
    LAndExp { $$ = $1; }
    | LOrExp OR LAndExp {
        $$ = new BinaryExp(BinaryOp::OR, make_unique_from_ptr($1), 
                          make_unique_from_ptr($3), {line_number});
        $1 = nullptr; 
        $3 = nullptr; 
    }
    ;

// LAndExp → EqExp | LAndExp '&&' EqExp
LAndExp:
    EqExp { $$ = $1; }
    | LAndExp AND EqExp {
        $$ = new BinaryExp(BinaryOp::AND, make_unique_from_ptr($1), 
                          make_unique_from_ptr($3), {line_number});
        $1 = nullptr; 
        $3 = nullptr; 
    }
    ;

// EqExp → RelExp | EqExp ('==' | '!=') RelExp
EqExp:
    RelExp { $$ = $1; }
    | EqExp EQ RelExp {
        $$ = new BinaryExp(BinaryOp::EQ, make_unique_from_ptr($1), 
                          make_unique_from_ptr($3), {line_number});
        $1 = nullptr; 
        $3 = nullptr; 
    }
    | EqExp NE RelExp {
        $$ = new BinaryExp(BinaryOp::NEQ, make_unique_from_ptr($1), 
                          make_unique_from_ptr($3), {line_number});
        $1 = nullptr; 
        $3 = nullptr; 
    }
    ;

// RelExp → AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
RelExp:
    AddExp { $$ = $1; }
    | RelExp LT AddExp {
        $$ = new BinaryExp(BinaryOp::LT, make_unique_from_ptr($1), 
                          make_unique_from_ptr($3), {line_number});
        $1 = nullptr; 
        $3 = nullptr; 
    }
    | RelExp GT AddExp {
        $$ = new BinaryExp(BinaryOp::GT, make_unique_from_ptr($1), 
                          make_unique_from_ptr($3), {line_number});
        $1 = nullptr; 
        $3 = nullptr; 
    }
    | RelExp LEQ AddExp {
        $$ = new BinaryExp(BinaryOp::LTE, make_unique_from_ptr($1), 
                          make_unique_from_ptr($3), {line_number});
        $1 = nullptr; 
        $3 = nullptr; 
    }
    | RelExp GEQ AddExp {
        $$ = new BinaryExp(BinaryOp::GTE, make_unique_from_ptr($1), 
                          make_unique_from_ptr($3), {line_number});
        $1 = nullptr; 
        $3 = nullptr; 
    }
    ;

// AddExp → MulExp | AddExp ('+' | '-') MulExp
AddExp:
    MulExp { $$ = $1; }
    | AddExp PLUS MulExp {
        $$ = new BinaryExp(BinaryOp::ADD, make_unique_from_ptr($1), 
                          make_unique_from_ptr($3), {line_number});
        $1 = nullptr; 
        $3 = nullptr; 
    }
    | AddExp MINUS MulExp {
        $$ = new BinaryExp(BinaryOp::SUB, make_unique_from_ptr($1), 
                          make_unique_from_ptr($3), {line_number});
        $1 = nullptr; 
        $3 = nullptr; 
    }
    ;

// MulExp → UnaryExp | MulExp ('*' | '/' | '%') UnaryExp
MulExp:
    UnaryExp { $$ = $1; }
    | MulExp MUL UnaryExp {
        $$ = new BinaryExp(BinaryOp::MUL, make_unique_from_ptr($1), 
                          make_unique_from_ptr($3), {line_number});
        $1 = nullptr; 
        $3 = nullptr; 
    }
    | MulExp DIV UnaryExp {
        $$ = new BinaryExp(BinaryOp::DIV, make_unique_from_ptr($1), 
                          make_unique_from_ptr($3), {line_number});
        $1 = nullptr; 
        $3 = nullptr; 
    }
    | MulExp MOD UnaryExp {
        $$ = new BinaryExp(BinaryOp::MOD, make_unique_from_ptr($1), 
                          make_unique_from_ptr($3), {line_number});
        $1 = nullptr; 
        $3 = nullptr; 
    }
    ;

// UnaryExp → PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp
UnaryExp:
    PrimaryExp { $$ = $1; }
    | PLUS UnaryExp %prec UPLUS {
        $$ = new UnaryExp(UnaryOp::PLUS, make_unique_from_ptr($2), {line_number});
        $2 = nullptr; 
    }
    | MINUS UnaryExp %prec UMINUS {
        $$ = new UnaryExp(UnaryOp::MINUS, make_unique_from_ptr($2), {line_number});
        $2 = nullptr; 
    }
    | NOT UnaryExp %prec UNOT {
        $$ = new UnaryExp(UnaryOp::NOT, make_unique_from_ptr($2), {line_number});
        $2 = nullptr; 
    }
    ;

// 函数调用 Ident '(' [FuncRParams] ')'
FunctionCall:
    IDENT LPAREN RPAREN {
        std::vector<std::unique_ptr<Exp>> args;
        $$ = new FunctionCall(std::string($1), std::move(args), {line_number});
        free($1);
    }
    | IDENT LPAREN FuncRParams RPAREN {
        std::vector<std::unique_ptr<Exp>> args;
        for (auto& arg : *$3) {
            args.push_back(std::move(arg));
        }
        delete $3; 
        $3 = nullptr;
        $$ = new FunctionCall(std::string($1), std::move(args), {line_number});
        free($1);
    }
    ;

// FuncRParams → Exp { ',' Exp }
FuncRParams:
    Exp {
        $$ = new std::vector<std::unique_ptr<Exp>>();
        $$->push_back(make_unique_from_ptr($1));
        $1 = nullptr;
    }
    | FuncRParams COMMA Exp {
        $1->push_back(make_unique_from_ptr($3));
        $$ = $1;
        $3 = nullptr;
    }
    ;

// PrimaryExp → '(' Exp ')' | LVal | Number | StringLiteral
PrimaryExp:
    LPAREN Exp RPAREN { 
        $$ = $2;
        $2 = nullptr;
    }
    | LVal { $$ = $1; }
    | Number { $$ = $1; }
    | StringLiteral { $$ = $1; }
    | FunctionCall { $$ = $1; }
    ;

// LVal → Ident { '[' Exp ']' }
LVal:
    IDENT {
        std::vector<std::unique_ptr<Exp>> indices;
        $$ = new LVal(std::string($1), std::move(indices), {line_number});
        free($1);
    }
    | IDENT ArrayIndices {
        $$ = new LVal(std::string($1), std::move(*$2), {line_number});
        free($1);
        delete $2;
        $2 = nullptr; 
    }
    ;

ArrayIndices:
    LBRACKET Exp RBRACKET {
        $$ = new std::vector<std::unique_ptr<Exp>>();
        $$->push_back(make_unique_from_ptr($2));
        $2 = nullptr; 
    }
    | ArrayIndices LBRACKET Exp RBRACKET {
        $1->push_back(make_unique_from_ptr($3));
        $$ = $1;
        $3 = nullptr;
    }
    ;

// Number → IntConst | FloatConst
Number:
    INT_CONST {
        $$ = new Number($1, {line_number});
    }
    | FLOAT_CONST {
        $$ = new Number($1, {line_number});
    }
    ;

// StringLiteral → STR_CONST
StringLiteral:
    STR_CONST {
        $$ = new StringLiteral(std::string($1), {line_number});
        free($1);
    }
    ;

// ConstExp → AddExp
ConstExp:
    AddExp { $$ = $1; }
    ;

// InitVal → Exp | '{' [ InitVal { ',' InitVal } ] '}'
InitVal:
    Exp {
        $$ = new InitVal(make_unique_from_ptr($1), {line_number});
        $1 = nullptr;
    }
    | LBRACE RBRACE {
        std::vector<std::unique_ptr<InitVal>> inits;
        $$ = new InitVal(std::move(inits), {line_number});
    }
    | LBRACE InitValList RBRACE {
        std::vector<std::unique_ptr<InitVal>> inits;
        for (auto& init : *$2) {
            inits.push_back(std::move(init));
        }
        delete $2;
        $2 = nullptr;
        $$ = new InitVal(std::move(inits), {line_number});
    }
    ;

InitValList:
    InitVal {
        $$ = new std::vector<std::unique_ptr<InitVal>>();
        $$->push_back(make_unique_from_ptr($1));
        $1 = nullptr; 
    }
    | InitValList COMMA InitVal {
        $1->push_back(make_unique_from_ptr($3));
        $$ = $1;
        $3 = nullptr;
    }
    ;

// ConstInitVal → ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
ConstInitVal:
    ConstExp {
        $$ = new ConstInitVal(make_unique_from_ptr($1), {line_number});
        $1 = nullptr;
    }
    | LBRACE RBRACE {
        std::vector<std::unique_ptr<ConstInitVal>> inits;
        $$ = new ConstInitVal(std::move(inits), {line_number});
    }
    | LBRACE ConstInitValList RBRACE {
        std::vector<std::unique_ptr<ConstInitVal>> inits;
        for (auto& init : *$2) {
            inits.push_back(std::move(init));
        }
        delete $2;
        $2 = nullptr;
        $$ = new ConstInitVal(std::move(inits), {line_number});
    }
    ;

ConstInitValList:
    ConstInitVal {
        $$ = new std::vector<std::unique_ptr<ConstInitVal>>();
        $$->push_back(make_unique_from_ptr($1));
        $1 = nullptr;
    }
    | ConstInitValList COMMA ConstInitVal {
        $1->push_back(make_unique_from_ptr($3));
        $$ = $1;
        $3 = nullptr;
    }
    ;

// 类型规则
BType:
    INT { $$ = BaseType::INT; }
    | FLOAT { $$ = BaseType::FLOAT; }
    ;

%%

void yyerror(const char *s) {
    fprintf(stderr, "Parse error at line %d: %s\n", line_number, s);
}