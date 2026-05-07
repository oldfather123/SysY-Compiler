%filenames Parser
%scanner scanner.h
/* TODO : %baseclass-preinclude ast.h */
%baseclass-preinclude bison_hdr.h
// %namespace aaac
/* TODO : %union and type of token */
%union {
    int ival;
    float dval;
    std::string *sval;
    Expr *expr;
    Stmt *stmt;
    Var *var;
    Decl *decl;
    VarDecl *vdecl;
    TUDecl *TU;
    RparmList *rparm_list;
    DeclList *dlist;
    InitList *init_list;
    SubList *sub_list;
    CompondStmt * compond_stmt;
    FParmList *fparm_list;
    FuncDecl *fdecl;
}

%token <sval> IDENT
%token <ival> INT
%token <dval> FLOAT

%token
    COMMA SEMICOLON 
    LPAREN RPAREN LBRACK RBRACK LBRACE RBRACE
    IF ELSE WHILE BREAK CONTINUE RETURN CONST
    INT_TY FLOAT_TY VOID_Ty

%right ASSIGN
%left OR
%left AND
%right NEG
%nonassoc EQ NEQ LT LE GT GE
%left PLUS MINUS
%left TIMES DIVIDE MODULE
%right UMINUS

/* TODO : %type  */
%type <TU> program
%type <fdecl> funcdecl
%type <vdecl> vardef fparm 
%type <expr> init exp primaryexp addexp mulexp unaryexp rparm cond lorexp landexp eqexp relexp
%type <var> lvalue
%type <rparm_list> rparms
%type <dlist> vardefs dec_line
%type <init_list> init_list
%type <sub_list> def_subs fparm_subs
%type <compond_stmt> block items
%type <sval> btype
%type <stmt> stmt
%type <fparm_list> fparms
%start program

%%

program: 
  program dec_line {
    $2->patchTo($1);
    $$ = $1;
}
| dec_line {
     $$ = TU = new TUDecl(d_scanner.getPos());
     $1->patchTo($$);
}
;

btype:
  INT_TY { $$ = new std::string("int"); }
| FLOAT_TY { $$ = new std::string("float"); }
| VOID_Ty { $$ = new std::string("void"); }
;

dec_line:
  CONST btype vardefs SEMICOLON {
    //std::cout << "dec_line rule1\n";
    $3->patch(*$2,true);
    delete $2;
    $$ = $3;
}
| btype vardefs SEMICOLON {
    $2->patch(*$1);
    $$ = $2;
}
| funcdecl {
    auto ptr = new DeclList();
    ptr->append($1);
    $$ = ptr;
}
;

vardefs:
  vardefs COMMA vardef {
    // std::cout << "following vardef" << std::endl;
    $$ = $1->append($3);
}
| vardef {
    // std::cout << "first vardef" << std::endl;
    $$ = new DeclList();
    $$->append($1);
}
;

vardef:
  IDENT ASSIGN init {
    //std::cout << *$1 << std::endl;
    $$ = new VarDecl(d_scanner.getPos(),"",*$1,$3);
}
| IDENT {
    $$ = new VarDecl(d_scanner.getPos(),"",*$1,nullptr);
}
| IDENT def_subs ASSIGN init {
    auto ptr = new ArrayDecl(d_scanner.getPos(),"",*$1,$4);
    $2->patchTo(ptr);
    delete $2;
    $$ = ptr;
}
| IDENT def_subs {
    //std::cout << "tag1 " << $1 << std::endl;
    auto ptr = new ArrayDecl(d_scanner.getPos(),"",*$1,nullptr);
    //std::cout << "here1" << std::endl;
    $2->patchTo(ptr);
    //std::cout << "here2" << std::endl;
    delete $2;
    $$ = ptr;
}
;

def_subs:
  def_subs LBRACK exp RBRACK {
    //$3->PrintTo(std::cout,0); std::cout << std::endl;
    $$ = $1->append($3);
}
| LBRACK exp RBRACK {
    $$ = new SubList();
    $$->append($2);
}
;

init:
  exp { 
    $$ = $1; 
    //std::cout << "init rule1\n";  
} 
| LBRACE init_list RBRACE {
    $$ = $2; 
    //std::cout << "init rule2\n"; 
}
| LBRACE RBRACE {
    $$ = new InitList(d_scanner.getPos());
}
;

init_list:
  init_list COMMA init {
    $$ = $1->append($3);
}
| init {
    //std::cout << "init list\n";
    $$ = new InitList(d_scanner.getPos());
    $$->append($1);
}
;

funcdecl:
  btype IDENT LPAREN fparms RPAREN block {
    $$ = new FuncDecl(d_scanner.getPos(), *$1, *$2, $4, $6);
}
| btype IDENT LPAREN RPAREN block {
    $$ = new FuncDecl(d_scanner.getPos(), *$1, *$2, new FParmList(), $5);
}
;

fparms:
  fparms COMMA fparm {
    $1->push_back($3);
    $$ = $1;
}
| fparm {
    $$ = new FParmList();
    $$->push_back($1);
}
;

fparm:
  btype IDENT {
    $$ = new VarDecl(d_scanner.getPos(),*$1, *$2, nullptr);
    delete $1;
}
| btype IDENT LBRACK RBRACK fparm_subs {
    $5->prepend(new EmptyExpr(d_scanner.getPos()));
    auto ptr = new ArrayDecl(d_scanner.getPos(),*$1, *$2, nullptr);
    $5->patchTo(ptr);
    $$ = ptr;
    delete $1;
}
| btype IDENT LBRACK RBRACK {
    auto ptr = new ArrayDecl(d_scanner.getPos(), *$1, *$2, nullptr);
    ptr->appendSubscript(new EmptyExpr(d_scanner.getPos()));
    $$ = ptr;
    delete $1;
}
;

fparm_subs:
  fparm_subs LBRACK exp RBRACK {
    $$ = $1->append($3);
}
| LBRACK exp RBRACK {
    $$ = new SubList();
    $$->append($2);
}
;

block:
  LBRACE RBRACE {
    $$ = new CompondStmt(d_scanner.getPos());
}
| LBRACE items RBRACE {
    $$ = $2;
}
;

items:
  items stmt {
    $1->appendStmt($2);
    $$ = $1;
}
| items dec_line {
    $2->patchTo($1);
    $$ = $1;
}
| stmt {
    $$ = new CompondStmt(d_scanner.getPos());
    $$->appendStmt($1);
}
| dec_line {
    $$ = new CompondStmt(d_scanner.getPos());
    $1->patchTo($$);
}
;

stmt:
  lvalue ASSIGN exp SEMICOLON {
    $$ = new AssignStmt(d_scanner.getPos(), $1, $3);
}
| exp SEMICOLON {
    $$ = $1;
}
| block {
    $$ = $1;
}
| IF LPAREN cond RPAREN stmt {
    $$ = new IfStmt(d_scanner.getPos(),$3,$5,nullptr);
}
| IF LPAREN cond RPAREN stmt ELSE stmt {
    $$ = new IfStmt(d_scanner.getPos(),$3,$5,$7);
}
| WHILE LPAREN cond RPAREN stmt {
    $$ = new WhileStmt(d_scanner.getPos(),$3,$5);
}
| BREAK SEMICOLON {
    $$ = new BreakStmt(d_scanner.getPos());
}
| CONTINUE SEMICOLON {
    $$ = new ContinueStmt(d_scanner.getPos());
}
| RETURN SEMICOLON {
    $$ = new ReturnStmt(d_scanner.getPos(),nullptr);
}
| RETURN exp SEMICOLON {
    $$ = new ReturnStmt(d_scanner.getPos(),$2);
}
| SEMICOLON {
    $$ = new EmptyStmt(d_scanner.getPos());
}
;

cond:
  lorexp {
    $$ = $1;
}
;

lorexp:
  landexp {
    $$ = $1;
}
| lorexp OR landexp {
    $$ = new BinaryOperator(d_scanner.getPos(), OR_OP, $1, $3);
}
;

landexp:
  eqexp {
    $$ = $1;
}
| landexp AND eqexp {
    $$ = new BinaryOperator(d_scanner.getPos(), AND_OP, $1, $3);
}
;

eqexp:
  relexp {
    $$ = $1;
}
| eqexp EQ relexp {
    $$ = new BinaryOperator(d_scanner.getPos(), EQ_OP, $1, $3);
}
|
eqexp NEQ relexp {
    $$ = new BinaryOperator(d_scanner.getPos(), NEQ_OP, $1, $3);
}
;

relexp:
  addexp {
    $$ = $1;
}
| relexp LT addexp {
    $$ = new BinaryOperator(d_scanner.getPos(), LT_OP, $1, $3);
}
| relexp LE addexp {
    $$ = new BinaryOperator(d_scanner.getPos(), LE_OP, $1, $3);
}
| relexp GT addexp {
    $$ = new BinaryOperator(d_scanner.getPos(), GT_OP, $1, $3);
}
| relexp GE addexp {
    $$ = new BinaryOperator(d_scanner.getPos(), GE_OP, $1, $3);
}
;

exp:
  addexp { 
    $$ = $1; 
    //std::cout << "exp : "; $1->PrintTo(std::cout,0); std::cout << std::endl; 
}
;

addexp:
  mulexp {
    $$ = $1;
}
| addexp PLUS mulexp {
    $$ = new BinaryOperator(d_scanner.getPos(), BinaryOp::PLUS_OP, $1, $3);
}
| addexp MINUS mulexp {
    $$ = new BinaryOperator(d_scanner.getPos(), BinaryOp::MINUS_OP, $1, $3);
}
;

mulexp:
  unaryexp {
    $$ = $1;
}
| mulexp TIMES unaryexp {
    $$ = new BinaryOperator(d_scanner.getPos(), BinaryOp::TIMES_OP, $1, $3);
}
| mulexp DIVIDE unaryexp {
    $$ = new BinaryOperator(d_scanner.getPos(), BinaryOp::DIVIDE_OP, $1, $3);
}
| mulexp MODULE unaryexp {
    $$ = new BinaryOperator(d_scanner.getPos(), BinaryOp::MODULE_OP, $1, $3);
}
;

unaryexp:
  primaryexp {
    $$ = $1;
}
| IDENT LPAREN rparms RPAREN {
// TODO: callexpr
    $$ = new CallExpr(d_scanner.getPos(), *$1, $3);
}
| IDENT LPAREN RPAREN {
    $$ = new CallExpr(d_scanner.getPos(), *$1, new RparmList());
}
| PLUS unaryexp {
    $$ = new UnaryOperator(d_scanner.getPos(), UnaryOp::Positive, $2);
}
| MINUS unaryexp {
    $$ = new UnaryOperator(d_scanner.getPos(), UnaryOp::Negative, $2);
}
| NEG unaryexp {
    $$ = new UnaryOperator(d_scanner.getPos(), UnaryOp::Negate, $2);
}
;

rparms:
  rparms COMMA rparm {
    $1->push_back($3);
    $$ = $1;
}
| rparm {
    $$ = new RparmList({$1});
}
;

rparm: exp { $$ = $1; };

primaryexp:
  LPAREN exp RPAREN {
    $$ = $2;
}
| lvalue {
    $$ = $1;
}
| INT {
    $$ = new IntLiteral(d_scanner.getPos(), $1);
}
| FLOAT {
    $$ = new FloatLiteral(d_scanner.getPos(), $1);
}
;

lvalue:
  IDENT {
    //std::cout << "lvalue : " << *$1 << std::endl;
    $$ = new Var(d_scanner.getPos(),*$1);
}
| lvalue LBRACK exp RBRACK {
    $$ = new SubscriptVar(d_scanner.getPos(), $1, $3);
}
;

