%{
#include <fstream>
#include "../include/ast.h"
#include "../include/symbol.h"
#include "../include/type.h"

int yylex();
void yyerror(char *s, ...);
int error_num = 0;
Program ASTroot;

extern int line;
// extern std::ofstream fout;
%}
// Token-Type token(value)
%union{
    char* error_msg;
    Symbol* symbol_token;
    int int_token;         double float_token; // 对于SysY的浮点常量，我们需要先以double类型计算，再在语法树节点创建的时候转为float
    Program program;
    CompUnit comp_unit;    std::vector<CompUnit>* comp_list;
    ExprBase expression;   std::vector<ExprBase>* expression_list;
    DeclBase decl;
    Def def;               std::vector<Def>* def_list;
    FuncDef func_def;
    FuncFParam formal;   std::vector<FuncFParam>* formal_list;
    Stmt stmt;
    Block block;
    BlockItem block_item;   std::vector<BlockItem>* block_item_list;
    InitValBase initval;  std::vector<InitValBase>* initval_list;
}
// declare the terminals (leaf-node of AST has token)
// Token(value) Token_id
%token <symbol_token> IDENT
%token <float_token> FLOAT_CONST
%token <int_token> INT_CONST
%token LEQ GEQ EQ NE // <=   >=   ==   != 
// %token LT GT // <    >
%token AND OR NOT // &&  || !
// %token PLUS MINUS MUL DIV MOD // +    -    *    /    %
%token CONST IF ELSE WHILE NONE_TYPE INT FLOAT
%token RETURN BREAK CONTINUE
%token ERROR

//give the type of nonterminals (nonleaf-node of AST doesn't have token)
%type <program> Program
%type <comp_unit> CompUnit 
%type <comp_list> Comp_list
%type <expression> Exp LOrExp AddExp MulExp RelExp EqExp LAndExp UnaryExp PrimaryExp
%type <expression> ConstExp Lval FuncRParams Cond
%type <expression> IntConst FloatConst 
%type <expression> ArrayDim 
%type <expression_list> ArrayDim_list Exp_list;
%type <decl> Decl VarDecl ConstDecl
%type <def> ConstDef VarDef
%type <def_list> ConstDef_list VarDef_list 
%type <func_def> FuncDef
%type <formal> FuncFParam 
%type <formal_list> FuncFParams
%type <stmt> Stmt 
%type <block> Block
%type <block_item> BlockItem
%type <block_item_list> BlockItem_list
%type <initval> ConstInitVal VarInitVal  
%type <initval_list> VarInitVal_list ConstInitVal_list

// THEN和ELSE用于处理if和else的移进-规约冲突：else优先级高，移进
%precedence THEN
%precedence ELSE
%% 

/*********************  Program ***********************/
Program 
:Comp_list
{
    @$ = @1;      
    ASTroot = new __Program($1);
    ASTroot->SetLineNumber(line);
};
/********************* Comp_Unit******************* */
CompUnit
:Decl {$$ = new CompUnit_Decl($1); $$->SetLineNumber(line);}
|FuncDef {$$ = new CompUnit_FuncDef($1); $$->SetLineNumber(line);};

Comp_list
:CompUnit {$$ = new std::vector<CompUnit>; ($$)->push_back($1);}
|Comp_list CompUnit {($1)->push_back($2); $$ = $1;};

/*****************  Expression & Cond ***********************/
//******************** Exp
Exp_list
:Exp{$$ = new std::vector<ExprBase>;($$)->push_back($1);}
|Exp_list ',' Exp{($1)->push_back($3);$$ = $1;};

Exp
:AddExp{ $$ = $1; $$->SetLineNumber(line);};

ConstExp
:AddExp{ $$ = $1;$$->SetLineNumber(line);};

MulExp
:UnaryExp{ $$ = $1; $$->SetLineNumber(line);} 
|MulExp '*' UnaryExp{$$ = new MulExp($1,OpType::Mul,$3); $$->SetLineNumber(line);}
|MulExp '/' UnaryExp{$$ = new MulExp($1,OpType::Div,$3); $$->SetLineNumber(line);}
|MulExp '%' UnaryExp{$$ = new MulExp($1,OpType::Mod,$3); $$->SetLineNumber(line);};

AddExp
:MulExp{ $$ = $1; $$->SetLineNumber(line);} 
|AddExp '+' MulExp{$$ = new AddExp($1,OpType::Add,$3); $$->SetLineNumber(line);}
|AddExp '-' MulExp{$$ = new AddExp($1,OpType::Sub,$3); $$->SetLineNumber(line);};

//******************** Cond
RelExp 
:AddExp{$$ = $1;$$->SetLineNumber(line);} 
|RelExp '<' AddExp{$$ = new RelExp($1,OpType::Lt,$3); $$->SetLineNumber(line);}
|RelExp '>' AddExp{$$ = new RelExp($1,OpType::Gt,$3); $$->SetLineNumber(line);}
|RelExp LEQ AddExp{$$ = new RelExp($1,OpType::Le,$3); $$->SetLineNumber(line);}
|RelExp GEQ AddExp{$$ = new RelExp($1,OpType::Ge,$3); $$->SetLineNumber(line);};

EqExp
:RelExp{$$ = $1;$$->SetLineNumber(line);} 
|EqExp EQ RelExp{$$ = new EqExp($1,OpType::Eq,$3); $$->SetLineNumber(line);}
|EqExp NE RelExp{$$ = new EqExp($1,OpType::Neq,$3); $$->SetLineNumber(line);};

LAndExp
:EqExp{$$ = $1;$$->SetLineNumber(line);} 
|LAndExp AND EqExp{$$ = new LAndExp($1,OpType::And,$3); $$->SetLineNumber(line);}; 

LOrExp
:LAndExp{$$ = $1;$$->SetLineNumber(line);}
|LOrExp OR LAndExp{$$ = new LOrExp($1,OpType::Or,$3); $$->SetLineNumber(line);};

Cond
:LOrExp{ $$ = $1; $$->SetLineNumber(line);};

//********************** PrimaryExp ******************************

//基本表达式
PrimaryExp
:IntConst{ 
    $$ = $1; 
    $$->SetLineNumber(line);}
|FloatConst{
    $$ = $1; 
    $$->SetLineNumber(line);}
|Lval{
    $$ = $1; 
    $$->SetLineNumber(line);}
|'(' Exp ')'{
    $$ = new PrimaryExp($2);
    $$->SetLineNumber(line);
};

IntConst
:INT_CONST{$$ = new IntConst($1);$$->SetLineNumber(line);};

FloatConst
:FLOAT_CONST{$$ = new FloatConst($1);$$->SetLineNumber(line);};

Lval
:IDENT{//普通变量
    $$ = new Lval($1,nullptr);
    $$->SetLineNumber(line);
}
|IDENT ArrayDim_list{ //数组
    $$ = new Lval($1,$2);
    $$->SetLineNumber(line);};

//*********************** UnaryExp *****************************
UnaryExp
:PrimaryExp{ $$ = $1; }
|IDENT '(' FuncRParams ')'{
    $$ = new FuncCall($1,$3);
    $$->SetLineNumber(line);}
|IDENT '(' ')'{
    // 将sylib.h中的宏定义starttime()替换为_sysy_starttime(line)
    if($1->getName() == "starttime"){
        auto params = new std::vector<ExprBase>;
        params->push_back(new IntConst(line));
        ExprBase temp = new FuncRParams(params);
        $$ = new FuncCall(new Symbol("_sysy_starttime"),temp);
        $$->SetLineNumber(line);
    }
    else if($1->getName() == "stoptime"){
        auto params = new std::vector<ExprBase>;
        params->push_back(new IntConst(line));
        ExprBase temp = new FuncRParams(params);
        $$ = new FuncCall(new Symbol("_sysy_stoptime"),temp);
        $$->SetLineNumber(line);
    }
    else{
        $$ = new FuncCall($1,nullptr);
        $$->SetLineNumber(line);
    }
}//正负号，负数的识别**
|'+' UnaryExp{ $$ = new UnaryExp(OpType::Add,$2); $$->SetLineNumber(line);}
|'-' UnaryExp{ $$ = new UnaryExp(OpType::Sub,$2); $$->SetLineNumber(line);}
|'!' UnaryExp{ $$ = new UnaryExp(OpType::Not,$2); $$->SetLineNumber(line);};


/************************ Func **********************/
//实参
FuncRParams
:Exp_list{ $$ = new FuncRParams($1);$$->SetLineNumber(line);};

//形参
FuncFParams
:FuncFParam{ $$ = new std::vector<FuncFParam>;($$)->push_back($1);}
|FuncFParams ',' FuncFParam{ ($1)->push_back($3);$$ = $1;};

FuncFParam
:INT IDENT{
    BuiltinType* type = new BuiltinType(BuiltinType::Int);
    $$ = new __FuncFParam(type,$2,nullptr);
    $$->SetLineNumber(line);
}
|FLOAT IDENT{
    BuiltinType* type = new BuiltinType(BuiltinType::Float);
    $$ = new __FuncFParam(type,$2,nullptr);
    $$->SetLineNumber(line);
}//数组
|INT IDENT '['  ']' {
    std::vector<ExprBase>* temp = new std::vector<ExprBase>;
    temp->push_back(nullptr);
    BuiltinType* type = new BuiltinType(BuiltinType::Int);
    $$ = new __FuncFParam(type,$2,temp);
    $$->SetLineNumber(line);
}
|FLOAT IDENT '['  ']' {
    std::vector<ExprBase>* temp = new std::vector<ExprBase>;
    temp->push_back(nullptr);
    BuiltinType* type = new BuiltinType(BuiltinType::Float);
    $$ = new __FuncFParam(type,$2,temp);
    $$->SetLineNumber(line);
}
|INT IDENT '[' ']' ArrayDim_list{
    $5->insert($5->begin(),nullptr);
    BuiltinType* type = new BuiltinType(BuiltinType::Int);
    $$ = new __FuncFParam(type,$2,$5);
    $$->SetLineNumber(line);
}
|FLOAT IDENT '[' ']' ArrayDim_list{
    $5->insert($5->begin(),nullptr);
    BuiltinType* type = new BuiltinType(BuiltinType::Float);
    $$ = new __FuncFParam(type,$2,$5);
    $$->SetLineNumber(line);
};

//函数定义
FuncDef
:INT IDENT '(' FuncFParams ')' Block
{   
    BuiltinType* type = new BuiltinType(BuiltinType::Int);
    $$ = new __FuncDef(type,$2,$4,$6);
    $$->SetLineNumber(line);
}
|INT IDENT '(' ')' Block
{   
    BuiltinType* type = new BuiltinType(BuiltinType::Int);
    $$ = new __FuncDef(type,$2,new std::vector<FuncFParam>(),$5); 
    $$->SetLineNumber(line);
}//返回FLOAT
|FLOAT IDENT '(' FuncFParams ')' Block
{   
    BuiltinType* type = new BuiltinType(BuiltinType::Float);
    $$ = new __FuncDef(type,$2,$4,$6);
    $$->SetLineNumber(line);
}
|FLOAT IDENT '(' ')' Block
{
    BuiltinType* type = new BuiltinType(BuiltinType::Float);
    $$ = new __FuncDef(type,$2,new std::vector<FuncFParam>(),$5); 
    $$->SetLineNumber(line);
}//返回VOID
|NONE_TYPE IDENT '(' FuncFParams ')' Block
{   
    BuiltinType* type = new BuiltinType(BuiltinType::Void);
    $$ = new __FuncDef(type,$2,$4,$6);
    $$->SetLineNumber(line);
}
|NONE_TYPE IDENT '(' ')' Block
{   
    BuiltinType* type = new BuiltinType(BuiltinType::Void);
    $$ = new __FuncDef(type,$2,new std::vector<FuncFParam>(),$5); 
    $$->SetLineNumber(line);
}

/************************ Stmt **********************/
Stmt
:Lval '=' Exp ';'{//赋值
    $$ = new AssignStmt($1,$3);$$->SetLineNumber(line);}//表达式
|Exp ';'{
    $$ = new ExprStmt($1); $$->SetLineNumber(line);}//空表达式
|';'{
    $$ = new ExprStmt(NULL);$$->SetLineNumber(line);}//语句块
|Block{
    $$ = new BlockStmt($1);$$->SetLineNumber(line);}//if 
|IF '(' Cond ')' Stmt %prec THEN{
    $$ = new IfStmt($3,$5,NULL);$$->SetLineNumber(line);}//if else 
|IF '(' Cond ')' Stmt ELSE Stmt{
    $$ = new IfStmt($3,$5,$7);$$->SetLineNumber(line);}//while循环
|WHILE '(' Cond ')' Stmt{
    $$ = new WhileStmt($3,$5);$$->SetLineNumber(line);}//break
|BREAK ';'{
    $$ = new BreakStmt();$$->SetLineNumber(line);}//continue
|CONTINUE ';'{
    $$ = new ContinueStmt();$$->SetLineNumber(line);}//有返回值的return
|RETURN Exp ';'{
    $$ = new RetStmt($2);$$->SetLineNumber(line);}//无返回值的return
|RETURN ';'{
    $$ = new RetStmt(NULL);$$->SetLineNumber(line);};

/************************ InitVal *************************/

ConstInitVal_list
:ConstInitVal{ 
    $$ = new std::vector<InitValBase>;($$)->push_back($1);}
|ConstInitVal_list ',' ConstInitVal{
    ($1)->push_back($3); $$ = $1;};

ConstInitVal
:ConstExp{ //单值初始化
    $$ = new ConstInitVal($1); $$->SetLineNumber(line);}
|'{' ConstInitVal_list '}'{ //初始化列表
    $$ = new ConstInitValList($2);$$->SetLineNumber(line);}
|'{' '}'{//初始化为空
    $$ = new ConstInitValList(new std::vector<InitValBase>()); $$->SetLineNumber(line);};

VarInitVal_list
:VarInitVal{
    $$ = new std::vector<InitValBase>;($$)->push_back($1);}
|VarInitVal_list ',' VarInitVal{
    ($1)->push_back($3);$$ = $1;};

VarInitVal
:Exp{ 
    $$ = new VarInitVal($1); $$->SetLineNumber(line);}
|'{' VarInitVal_list '}'{ 
    $$ = new VarInitValList($2);$$->SetLineNumber(line);}
|'{' '}'{
    $$ = new VarInitValList(new std::vector<InitValBase>()); $$->SetLineNumber(line);};


/************************ Def  *************************/

VarDef
:IDENT '=' VarInitVal{  
    $$ = new VarDef($1,nullptr,$3);$$->SetLineNumber(line);}
|IDENT{  
    $$ = new VarDef_no_init($1,nullptr); $$->SetLineNumber(line);}
//数组
|IDENT ArrayDim_list {
    $$ = new VarDef_no_init($1, $2); $$->SetLineNumber(line);}
|IDENT ArrayDim_list '=' VarInitVal {
    $$ = new VarDef($1, $2, $4); $$->SetLineNumber(line);};   

ConstDef
:IDENT '=' ConstInitVal { //CONST声明时必须初始化 
    $$ = new ConstDef($1,nullptr,$3); $$->SetLineNumber(line);}
|IDENT ArrayDim_list '=' ConstInitVal {
    $$ = new ConstDef($1, $2, $4); $$->SetLineNumber(line); };

//数组维度表达式（支持多维,维度必须为常量）
ArrayDim
: '[' ConstExp ']' {
    $$=$2;$$->SetLineNumber(line);};

ArrayDim_list
:ArrayDim{
    $$ = new std::vector<ExprBase>;$$->push_back($1); }
|ArrayDim_list ArrayDim{
    $$ = $1; $$->push_back($2);};

/************************ Decl *************************/

Decl
:VarDecl{
    $$ = $1; $$->SetLineNumber(line);}
|ConstDecl{
    $$ = $1; $$->SetLineNumber(line);};

//Var
VarDecl
:INT VarDef_list ';'{
    BuiltinType* type = new BuiltinType(BuiltinType::Int);
    $$ = new VarDecl(type,$2); $$->SetLineNumber(line);}
| FLOAT VarDef_list ';'{
    BuiltinType* type = new BuiltinType(BuiltinType::Float);
    $$ = new VarDecl(type,$2);$$->SetLineNumber(line);};

VarDef_list
: VarDef {
    $$ = new std::vector<Def>;($$)->push_back($1);}
| VarDef_list ',' VarDef{
    ($1)->push_back($3);$$ = $1;};

//Const
ConstDecl
:CONST INT ConstDef_list ';'{
    BuiltinType* type = new BuiltinType(BuiltinType::Int);
    $$ = new ConstDecl(type,$3); $$->SetLineNumber(line);}
|CONST FLOAT ConstDef_list ';'{
    BuiltinType* type = new BuiltinType(BuiltinType::Float);
    $$ = new ConstDecl(type,$3);$$->SetLineNumber(line);};
 
ConstDef_list
:ConstDef {
    $$ = new std::vector<Def>; ($$)->push_back($1);}
| ConstDef_list ',' ConstDef{
    ($1)->push_back($3);$$ = $1;};

/************************ Block *************************/

Block
:'{' BlockItem_list '}'{
    $$ = new __Block($2);$$->SetLineNumber(line);}
|'{' '}'{
    $$ = new __Block(new std::vector<BlockItem>);$$->SetLineNumber(line);};

//语句列表
BlockItem_list
:BlockItem{
    $$ = new std::vector<BlockItem>;($$)->push_back($1);}
|BlockItem_list  BlockItem{
    ($1)->push_back($2);$$ = $1;};

//语句:Decl声明式；Stmt语句
BlockItem
:Decl{
    $$ = new BlockItem_Decl($1);$$->SetLineNumber(line);}
|Stmt{
    $$ = new BlockItem_Stmt($1);$$->SetLineNumber(line);};


%%
void yyerror(char* s, ...)
{
    ++error_num;
    std::cout << "parser error in line " << line << std::endl;
}