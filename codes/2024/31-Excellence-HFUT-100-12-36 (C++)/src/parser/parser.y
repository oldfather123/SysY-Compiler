%{
    #include "SyntaxTree.hpp"
    #include "SyntaxAnalyse.hpp"
    #include <iostream>
    #include <cstring>

    int yylex();
    int yyparse();
    int yyrestart();
    
    extern FILE* yyin;
    extern char* yytext;
    extern int line_number;
    extern int column_end_number;
    extern int column_start_number;

    void yyerror(const char *s) {
        std::cerr << s << std::endl;
        std::cerr << "Error at line " << line_number << ": " << column_end_number << std::endl;
        std::cerr << "Error: " << yytext << std::endl;
        std::abort();
    }

    using namespace ast;
%}

%union {
    char *current_symbol; //we can't use string or any other object with construct function in union. 
    int symbol_size;
    struct ast::compunit_syntax *compunit ;
    struct ast::func_def_syntax *func_def;
    struct ast::expr_syntax *expr;
    struct ast::binop_expr_syntax *binop_expr;
    struct ast::unaryop_expr_syntax *unaryop_expr;
    struct ast::lval_syntax *lval;  
    struct ast::literal_syntax *literal;
    struct ast::stmt_syntax *stmt;
    struct ast::assign_stmt_syntax *assign;
    struct ast::block_syntax *block;
    struct ast::if_stmt_syntax *if_stmt;
    struct ast::return_stmt_syntax *return_stmt;
    struct ast::var_def_stmt_syntax *var_def_stmt;
    struct ast::var_decl_stmt_syntax *var_decl_stmt;
    struct ast::func_f_param_syntax *func_f_param;
    struct ast::var_dimension_syntax *var_dimension;
    struct ast::exp_stmt_syntax *exp_stmt;
    struct ast::while_stmt_syntax *while_stmt;
    struct ast::empty_stmt_syntax *empty_stmt;
    struct ast::break_stmt_syntax *break_stmt;
    struct ast::continue_stmt_syntax *continue_stmt;
    struct ast::init_syntax *init;
    struct ast::func_call_syntax *func_call;
    enum vartype var_type;
}

%token <current_symbol> INT FLOAT VOID IF ELSE WHILE BREAK CONTINUE RETURN Ident CONST
%token <current_symbol> ADD SUB MUL DIV MOD
%token <current_symbol> LPAREN RPAREN LBRACKET RBRACKET LBRACE RBRACE
%token <current_symbol> IntConst FloatConst
%token <current_symbol> LESS GREATER EQUAL NOT
%token <current_symbol> LESS_EQUAL GREATER_EQUAL NOT_EQUAL AND OR
%token <current_symbol> ASSIGN COMMA SEMICOLON
%token <current_symbol> ERROR

%type <compunit> CompUnit
%type <func_def> FuncDef
// %type <var_type> FuncType
%type <var_type> BType
%type <func_f_param> FuncFParam
// %type <var_def_stmt> FuncFParam
%type <block> Block
%type <block> BlockItems
%type <stmt> Stmt
%type <expr> Exp
%type <stmt> Decl
%type <stmt> VarDecl 
%type <stmt> ConstDecl
%type <var_decl_stmt> VarDefGroup
%type <var_decl_stmt> ConstDefGroup
%type <func_def> FuncFParamsGroup
%type <var_def_stmt> VarDef
%type <var_def_stmt> ConstDef
%type <init> InitVal
%type <init> ConstInitVal
%type <expr> AddExp
%type <expr> PrimaryExp
%type <expr> MulExp
%type <expr> UnaryExp
%type <expr> Number
%type <lval> Lval
%type <expr> Cond
%type <expr> LOrExp
%type <expr> LAndExp
%type <expr> EqExp
%type <expr> RelExp
%type <current_symbol> UnaryOp
%type <var_dimension> ExpGroup
%type <expr> ConstExp
%type <var_dimension> ConstExpGroup
%type <init> InitValGroup
%type <init> ConstInitValGroup
%type <func_call> FuncRParamsGroup




%start CompUnit


%%

    CompUnit
    :CompUnit FuncDef { SyntaxAnalyseCompUnit($$,$1,$2);}
    |CompUnit Decl {SyntaxAnalyseCompUnit($$, $1, $2);}
    |FuncDef { SyntaxAnalyseCompUnit($$,nullptr,$1); }
    |Decl {SyntaxAnalyseCompUnit($$, nullptr, $1);}

    FuncDef: BType Ident LPAREN RPAREN Block {
        SyntaxAnalyseFuncDef($$,$1,$2,$5);
        free($3);
        free($4);
    }
    | VOID Ident LPAREN RPAREN Block {
        SyntaxAnalyseFuncDef($$, vartype::VOID, $2, $5);
        free($1);
        free($3);
        free($4);
    }
    | BType Ident LPAREN FuncFParam FuncFParamsGroup RPAREN Block {
        SyntaxAnalyseFuncDef($$, $1, $2, $7);
        SyntaxAnalyseFuncFDecl($$, $4, $5);
        free($3);
        free($6);
    }
    | VOID Ident LPAREN FuncFParam FuncFParamsGroup RPAREN Block {
        SyntaxAnalyseFuncDef($$, vartype::VOID, $2, $7);
        SyntaxAnalyseFuncFDecl($$, $4, $5);
        free($1);
        free($3);
        free($6);
    }

    FuncFParam: BType Ident {
        SyntaxAnalyseFuncFDef($$, $1, $2, nullptr, false);
    }
    | BType Ident LBRACKET RBRACKET ExpGroup {
        SyntaxAnalyseFuncFDef($$, $1, $2, $5, true);
        free($3);
        free($4);
    }             // !!! Sysy上写明数组形参的第一维长度省去，即第一个[]应省略

    ExpGroup: LBRACKET Exp RBRACKET ExpGroup {
        SyntaxAnalyseVarDimension($$, $2, $4);
        free($1);
        free($3);
    }
    | %empty {
        $$ = nullptr;
    }

    FuncFParamsGroup: COMMA FuncFParam FuncFParamsGroup {
        SyntaxAnalyseFuncFDeclGroup($$, $2, $3);
        free($1);
    }
    | %empty {
        $$ = nullptr;
    }

    // FuncType: VOID { SynataxAnalyseFuncType($$,$1);}
    // |INT { SynataxAnalyseFuncType($$,$1);}
    // |FLOAT { SynataxAnalyseFuncType($$,$1);}
    // FuncType: BType {}
    // | VOID {}

    Block: LBRACE BlockItems RBRACE {
        SynataxAnalyseBlock($$,$2);
        free($1);
        free($3);
    }

    BlockItems: BlockItems Stmt { SynataxAnalyseBlockItems($$,$1,$2);}
    | %empty { SynataxAnalyseBlockItems($$,nullptr,nullptr);}
 /*a-难度---------------*/
    | BlockItems Decl{
        SynataxAnalyseBlockItems($$,$1,$2);
    }
 /*--------------------*/

    Stmt: RETURN Exp SEMICOLON {
        SynataxAnalyseStmtReturn($$,$2);
        free($1);
        free($3);
    }
 /*a-难度---------------*/
    | Block{
        SynataxAnalyseStmtBlock($$,$1);
    }
    |RETURN SEMICOLON{
        SynataxAnalyseStmtReturn($$,nullptr);
        free($1);
        free($2);
    }
 /*--------------------*/
 /*a难度---------------*/
    | Lval ASSIGN Exp SEMICOLON{
        SynataxAnalyseStmtAssign($$,$1,$3);
        free($2);
        free($4);
    }
 /*--------------------*/
 /*a+难度---------------*/
    | IF LPAREN Cond RPAREN Stmt {
        SynataxAnalyseStmtIf($$,$3,$5,nullptr);
        free($1);
        free($2);
        free($4);
    }
    | IF LPAREN Cond RPAREN Stmt ELSE Stmt{
        SynataxAnalyseStmtIf($$,$3,$5,$7);
        free($1);
        free($2);
        free($4);
        free($6);
    }
    | Exp SEMICOLON {
        SynataxAnalyseStmtExp($$, $1);
        free($2);
    }
    | SEMICOLON{
        // SynataxAnalyseStmtEmpty($$);
        $$ = new ast::empty_stmt_syntax;
        free($1);
    }
    | WHILE LPAREN Cond RPAREN Stmt {
        SynataxAnalyseStmtWhile($$, $3, $5);
        free($1);
        free($2);
        free($4);
    }
    | BREAK SEMICOLON {
        // SynataxAnalyseStmtBoC(&&, $1);
        $$ = new ast::break_stmt_syntax;
        free($1);
        free($2);
    }
    | CONTINUE SEMICOLON {
        // SynataxAnalyseStmtBoC(&&, $1);
        $$ = new ast::continue_stmt_syntax;
        free($1);
        free($2);
    }
 /*--------------------*/

//     PrimaryExp: IntConst { SynataxAnalysePrimaryExpIntConst($$,$1); }
//     | FloatConst { SynataxAnalysePrimaryExpFloatConst($$,$1); }
//  /*a-难度---------------*/
//     | LPAREN Exp RPAREN{
//         $$=$2;
//     }
//     | Ident{
//         SynataxAnalysePrimaryExpVar($$,$1);
//     }
    PrimaryExp: Number {
        $$ = $1;
    }
    | Lval {
        $$ = $1;
    }
    // | IntConst { SynataxAnalysePrimaryExpIntConst($$,$1); }
    // | FloatConst { SynataxAnalysePrimaryExpFloatConst($$,$1); }
    | LPAREN Exp RPAREN{
        $$=$2;
        free($1);
        free($3);
    }

    Number: IntConst {SynataxAnalysePrimaryExpIntConst($$,$1);}
    | FloatConst {SynataxAnalysePrimaryExpFloatConst($$,$1);}
 /*--------------------*/

 /*a-难度---------------*/
    Decl: VarDecl{
        $$=$1;
    }
    | ConstDecl {
        $$ = $1;
    }

    VarDecl: BType VarDef VarDefGroup SEMICOLON{
        SynataxAnalyseVarDecl($$, $1, $2, $3, false);
        free($4);
    }

    VarDefGroup:  COMMA VarDef VarDefGroup{
        SynataxAnalyseVarDefGroup($$,$2,$3);
        free($1);
    }
    | %empty {
        $$=nullptr;
    }

    VarDef: Ident  {
        SynataxAnalyseVarDef($$,$1, nullptr,nullptr);
    }
    | Ident ASSIGN InitVal{
        SynataxAnalyseVarDef($$,$1, nullptr,$3);
        free($2);
    }
    | Ident ConstExpGroup {
        SynataxAnalyseVarDef($$,$1, $2, nullptr);
    }
    | Ident ConstExpGroup ASSIGN InitVal {
        SynataxAnalyseVarDef($$,$1, $2, $4);
        free($3);
    }

    InitVal: Exp{
        SynataxAnalyseInitVal($$, $1, nullptr, nullptr);
    }
    | LBRACE InitVal InitValGroup RBRACE {
        SynataxAnalyseInitVal($$, nullptr, $2, $3);
        free($1);
        free($4);
    }
    | LBRACE RBRACE {
        SynataxAnalyseInitVal($$, nullptr, nullptr, nullptr);
        free($1);
        free($2);
    }

    InitValGroup: COMMA InitVal InitValGroup {
        SynataxAnalyseInitValGroup($$, $2, $3);
        free($1);
    }
    | %empty {
        $$ = nullptr;
    }

    ConstDecl: CONST BType ConstDef ConstDefGroup SEMICOLON {
        SynataxAnalyseVarDecl($$, $2, $3, $4, true);
        free($1);
        free($5);
    }

    ConstDefGroup: COMMA ConstDef ConstDefGroup {
        SynataxAnalyseVarDefGroup($$, $2, $3);
        free($1);
    }
    | %empty {
        $$ = nullptr;
    }

    ConstDef: Ident ConstExpGroup ASSIGN ConstInitVal {
        SynataxAnalyseVarDef($$, $1, $2, $4);
        free($3);
    }

    ConstInitVal: ConstExp {
        SynataxAnalyseInitVal($$, $1, nullptr, nullptr);
    }
    | LBRACE ConstInitVal ConstInitValGroup RBRACE {
        SynataxAnalyseInitVal($$, nullptr, $2, $3);
        free($1);
        free($4);
    }
    | LBRACE RBRACE {
        SynataxAnalyseInitVal($$, nullptr, nullptr, nullptr);
        free($1);
        free($2);
    }

    ConstInitValGroup: COMMA ConstInitVal ConstInitValGroup {
        SynataxAnalyseInitValGroup($$, $2, $3);
        free($1);
    }
    | %empty {
        $$ = nullptr;
    }

    ConstExpGroup: LBRACKET ConstExp RBRACKET ConstExpGroup {
        SyntaxAnalyseVarDimension($$, $2, $4);
        free($1);
        free($3);
    }
    | %empty {
        $$ = nullptr;
    }

    ConstExp: AddExp {
        $$ = $1;
    }
    
    BType: INT {SynataxAnalyseVarType($$, $1);}
    | FLOAT {SynataxAnalyseVarType($$, $1);}

    AddExp: MulExp{
        $$=$1;
    }
    | AddExp ADD MulExp{
        SynataxAnalyseAddExp($$,$1,$2,$3);
    }
    | AddExp SUB MulExp{
        SynataxAnalyseAddExp($$,$1,$2,$3);
    }

    Exp: AddExp{
        $$=$1;
    }
 /*--------------------*/
 
 /*a难度---------------*/
    MulExp: UnaryExp{
        $$=$1;
    }
    | MulExp MUL UnaryExp {
         SynataxAnalyseMulExp($$,$1,$2,$3);
    }
    | MulExp DIV UnaryExp {
         SynataxAnalyseMulExp($$,$1,$2,$3);
    }
    | MulExp MOD UnaryExp {
        SynataxAnalyseMulExp($$, $1, $2, $3);
    }

    Lval: Ident ExpGroup{
        SynataxAnalyseLval($$,$1, $2);
    }
 /*--------------------*/
 
 /*a+难度---------------*/
   Cond: LOrExp{
    $$=$1;
   }

   LOrExp: LAndExp{
    $$=$1;
   }
   |LOrExp OR LAndExp{
    SynataxAnalyseLOrExp($$,$1,$3);
    free($2);
   }

    LAndExp: EqExp{
        $$=$1;
    }
    | LAndExp AND EqExp {
        SynataxAnalyseLAndExp($$,$1,$3);
        free($2);
    }

    EqExp: RelExp{
        $$=$1;
    }
    |EqExp EQUAL RelExp{
        SynataxAnalyseEqExp($$,$1,$2,$3);
    }
    |EqExp NOT_EQUAL RelExp{
        SynataxAnalyseEqExp($$,$1,$2,$3);
    }

    RelExp: AddExp{
        $$=$1;
    }
    | RelExp LESS AddExp {
        SynataxAnalyseRelExp($$,$1,$2,$3);
    }
    | RelExp GREATER AddExp {
        SynataxAnalyseRelExp($$,$1,$2,$3);
    }
    | RelExp LESS_EQUAL AddExp {
        SynataxAnalyseRelExp($$,$1,$2,$3);
    }
    | RelExp GREATER_EQUAL AddExp {
        SynataxAnalyseRelExp($$,$1,$2,$3);
    }

 /*--------------------*/

 /*a++难度---------------*/
    UnaryExp: PrimaryExp{
        $$=$1;
    }
    | UnaryOp UnaryExp{
        SynataxAnalyseUnaryExp($$,$1,$2);
    }
    | Ident LPAREN Exp FuncRParamsGroup RPAREN {
        SyntaxAnalyseFuncCall($$, $1, $3, $4);
        free($2);
        free($5);
    }
    | Ident LPAREN RPAREN {
        SyntaxAnalyseFuncCall($$, $1, nullptr, nullptr);
        free($2);
        free($3);
    }

    FuncRParamsGroup: COMMA Exp FuncRParamsGroup {
        SyntaxAnalyseFuncCallGroup($$, $2, $3);
        free($1);
    }
    | %empty {
        $$ = nullptr;
    }

    UnaryOp:ADD{
        $$=$1;
        // strcpy($$, $1);
    }
    | SUB{
        $$=$1;
        // strcpy($$, $1);
    }
    | NOT{
        $$=$1;
        // strcpy($$, $1);
    }
 /*--------------------*/

%%

