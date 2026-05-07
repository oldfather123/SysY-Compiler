#ifndef SYSY_TREE_H
#define SYSY_TREE_H

// AST definition
#include "symbol.h"
#include "type.h"
// #include "visitor.h" I think visitor patten is no use
#include <iostream>
#include <vector>
#include <variant>

class ASTNode {
public:
    int line; 
    NodeAttribute attribute; 
    // virtual ~ASTNode() = default;
    void SetLineNumber(int line_number){line=line_number;}
	virtual void codeIR() = 0;
    virtual void TypeCheck() = 0; 
    virtual void printAST(std::ostream &s, int pad) = 0;
};

class __Block;
typedef __Block *Block;

class __Stmt : public ASTNode {};
typedef __Stmt *Stmt;

class __Def : public ASTNode {
    public:
        int scope = -1;    // 在语义分析阶段填入正确的作用域
        //Symbol* name;
        virtual Symbol* GetSymbol()=0;//新增
};
typedef __Def *Def;
    
class __DeclBase : public ASTNode {
    public:
        virtual Type* GetTypedecl()=0;//新增
        virtual std::vector<Def>* GetDefs()=0;//新增
};
typedef __DeclBase *DeclBase;

/*****************  Expression  ***********************/
/* basic class of expression */
class __ExprBase : public ASTNode {
public:
    int scope = -1;
	virtual void codeIR() {}
    virtual void TypeCheck() {}
    virtual void printAST(std::ostream &s, int pad) {}
};
typedef __ExprBase *ExprBase;

// Exp → AddExp
class Exp : public __ExprBase {
public:
	ExprBase addExp;
	Exp(ExprBase addExp) : addExp(addExp) {};
	
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};
typedef __ExprBase* ExprBase;
// ConstExp → AddExp
class ConstExp : public __ExprBase {
    public:
        ExprBase addExp;
        ConstExp(ExprBase addExp) : addExp(addExp) {}
    
        void codeIR();
        void TypeCheck();
        void printAST(std::ostream &s, int pad);
};
/*
// AddExp → MulExp | AddExp ('+' | '−') MulExp 
class Add2MulExp : public __ExprBase {
public:
	ExprBase mulExp;

	// select Production 1.
    Add2MulExp(ExprBase mulExp) : mulExp(mulExp) {}
	
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};
class BinaryAddExp : public __ExprBase {
public:
	ExprBase addExp, mulExp;
	OpType op;

	// select Production 2.
    BinaryAddExp(ExprBase addExp, OpType op, ExprBase mulExp) : addExp(addExp), op(op), mulExp(mulExp) {}
	
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

// MulExp → UnaryExp | MulExp ('*' | '/' | '%') UnaryExp 
class Mul2UnaryExp : public __ExprBase {
public:
    ExprBase unaryExp;

    // select Production 1.
    Mul2UnaryExp(ExprBase unaryExp) : unaryExp(unaryExp) {}

    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

class BinaryMulExp : public __ExprBase {
public:
    ExprBase mulExp, unaryExp;
    OpType op;

    // select Production 2.
    BinaryMulExp(ExprBase mulExp, OpType op, ExprBase unaryExp) : mulExp(mulExp), op(op), unaryExp(unaryExp) {}

    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

// RelExp → AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
class Rel2AddExp : public __ExprBase {
public:
    ExprBase addExp;

    // select Production 1.
    Rel2AddExp(ExprBase addExp) : addExp(addExp) {}

    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

class BinaryRelExp : public __ExprBase {
public:
    ExprBase relExp, addExp;
    OpType op;

    // select Production 2.
    BinaryRelExp(ExprBase relExp, OpType op, ExprBase addExp) : relExp(relExp), op(op), addExp(addExp) {}

    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

// EqExp → RelExp | EqExp ('==' | '!=') RelExp
class Eq2RelExp: public __ExprBase {
public:
    ExprBase relExp;

    // select Production 1.
    Eq2RelExp(ExprBase relExp) : relExp(relExp) {}

    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

class BinaryEqExp: public __ExprBase {
public:
    ExprBase eqExp, relExp;
    OpType op;

    // select Production 2.
    BinaryEqExp(ExprBase eqExp, OpType op, ExprBase relExp) : eqExp(eqExp), op(op), relExp(relExp) {}

    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

// LAndExp → EqExp | LAndExp '&&' EqExp
class LAnd2EqExp : public __ExprBase {
public:
    ExprBase eqExp;

    // select Production 1.
    LAnd2EqExp(ExprBase eqExp) : eqExp(eqExp) {}

    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

class BinaryLAndExp : public __ExprBase {
public:
    ExprBase lAndExp, eqExp;
    OpType op;

    // select Production 2.
    BinaryLAndExp(ExprBase lAndExp, OpType op, ExprBase eqExp)
        : lAndExp(lAndExp), op(op), eqExp(eqExp) {}

    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

// LOrExp → LAndExp | LOrExp '||' LAndExp
class LOr2LAndExp : public __ExprBase {
public:
    ExprBase lAndExp;

    // select Production 1.
    LOr2LAndExp(ExprBase lAndExp) :lAndExp(lAndExp) {}

    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

class BinaryLOrExp : public __ExprBase {
public:
    ExprBase lOrExp, lAndExp;
    OpType op;

    // select Production 2.
    BinaryLOrExp(ExprBase lOrExp, OpType op, ExprBase lAndExp) 
        : lOrExp(lOrExp), op(op), lAndExp(lAndExp) {}

    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

*/

// AddExp → MulExp | AddExp ('+' | '−') MulExp 
class AddExp : public __ExprBase {
public:
	ExprBase addExp, mulExp;
	OpType op;

    AddExp(ExprBase addExp, OpType op, ExprBase mulExp) : addExp(addExp), op(op), mulExp(mulExp) {}
	
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

// MulExp → UnaryExp | MulExp ('*' | '/' | '%') UnaryExp 
class MulExp : public __ExprBase {
public:
    ExprBase mulExp, unaryExp;
    OpType op;

    MulExp(ExprBase mulExp, OpType op, ExprBase unaryExp) : mulExp(mulExp), op(op), unaryExp(unaryExp) {}

    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

// RelExp → AddExp | RelExp ('<' | '>' | '<=' | '>=') AddExp
class RelExp : public __ExprBase {
public:
    ExprBase relExp, addExp;
    OpType op;

    RelExp(ExprBase relExp, OpType op, ExprBase addExp) : relExp(relExp), op(op), addExp(addExp) {}

    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

// EqExp → RelExp | EqExp ('==' | '!=') RelExp
class EqExp: public __ExprBase {
public:
    ExprBase eqExp, relExp;
    OpType op;

    EqExp(ExprBase eqExp, OpType op, ExprBase relExp) : eqExp(eqExp), op(op), relExp(relExp) {}

    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

// LAndExp → EqExp | LAndExp '&&' EqExp
class LAndExp : public __ExprBase {
public:
    ExprBase lAndExp, eqExp;
    OpType op;

    LAndExp(ExprBase lAndExp, OpType op, ExprBase eqExp)
        : lAndExp(lAndExp), op(op), eqExp(eqExp) {}

    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

// LOrExp → LAndExp | LOrExp '||' LAndExp
class LOrExp : public __ExprBase {
public:
    ExprBase lOrExp, lAndExp;
    OpType op;

    LOrExp(ExprBase lOrExp, OpType op, ExprBase lAndExp) 
        : lOrExp(lOrExp), op(op), lAndExp(lAndExp) {}

    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};


/*****************  Lval  ***********************/

// LVal → Ident {'[' Exp ']'}
class Lval : public __ExprBase {
public:
    Symbol* name;
    std::vector<ExprBase> *dims;  // may be empty
    bool is_left=true;//新增，左值或者右值,Lval的typecheck为false,在assign_stmt之后被赋值为true
    int scope = -1; //作用域

    Lval(Symbol* n, std::vector<ExprBase> *d) : name(n), dims(d) {}
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

// FuncRParams → Exp { ',' Exp }
class FuncRParams : public __ExprBase {
public:
    std::vector<ExprBase> *rParams{};
    FuncRParams(std::vector<ExprBase> *p) : rParams(p) {}

    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

// UnaryExp → Ident '(' [FuncRParams] ')'
class FuncCall : public __ExprBase {
public:
    Symbol* name;
    ExprBase funcRParams;
    
	FuncCall(Symbol* n, ExprBase f) : name(n), funcRParams(f) {}

    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

// UnaryExp →  UnaryOp UnaryExp 
// UnaryOp  → '+' | '−' | '!' 
class UnaryExp : public __ExprBase {
public:
	ExprBase unaryExp;
	OpType unaryOp;

    UnaryExp(OpType unaryOp, ExprBase unaryExp) : unaryExp(unaryExp), unaryOp(unaryOp) {}

    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

/******************** Int/Float Const **********************/
class IntConst : public __ExprBase {
public:
    int val;
    IntConst(int v) : val(v) {}
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

class FloatConst : public __ExprBase {
public:
    float val;
    FloatConst(float v) : val(v) {}
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

// PrimaryExp → '(' Exp ')' | LVal | Number
class PrimaryExp : public __ExprBase {
public:
	ExprBase exp;
    PrimaryExp(ExprBase exp) : exp(exp) {}
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

/************************* Stmt **************************/

// Stmt → LVal '=' Exp ';' 
class AssignStmt : public __Stmt {
public:
    ExprBase lval;
    ExprBase exp;

    AssignStmt(ExprBase l, ExprBase e) : lval(l), exp(e) {}
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

// Stmt → [Exp] ';' 
class ExprStmt : public __Stmt {
public:
    ExprBase exp;  // may be empty

	bool getExp() { return exp; }
    ExprStmt(ExprBase e) : exp(e) {}
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

// Stmt → Block
class BlockStmt : public __Stmt {
public:
    Block b;
    BlockStmt(Block b) : b(b) {}
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

// Stmt → 'if' '( Cond ')' Stmt  ['else' Stmt] 
class IfStmt : public __Stmt {
public:
    ExprBase Cond;
    Stmt ifStmt;
    Stmt elseStmt;   // may be empty

	Stmt getElseStmt() { return elseStmt; }
    IfStmt(ExprBase c, Stmt i, Stmt t) : Cond(c), ifStmt(i), elseStmt(t) {}
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

// Stmt → 'while' '(' Cond ')' Stmt
class WhileStmt : public __Stmt {
public:
    ExprBase Cond;
    Stmt loopBody;

    WhileStmt(ExprBase c, Stmt b) : Cond(c), loopBody(b) {}
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

// Stmt → 'continue' ';'
class ContinueStmt : public __Stmt {
public:
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);

    ContinueStmt() {}
};

// Stmt → 'break' ';'
class BreakStmt : public __Stmt {
public:
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);

    BreakStmt() {}
};

// Stmt → 'return' [Exp] 
class RetStmt : public __Stmt {
public:
    ExprBase retExp; // may be empty
    RetStmt(ExprBase r) : retExp(r) {}

	ExprBase getExp() { return retExp; }
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};


/************************ InitVal *************************/
class __InitValBase : public ASTNode {
    public:
    virtual ExprBase getExp() = 0;
};
typedef __InitValBase *InitValBase;

// ConstInitVal → ConstExp  
//				 | '{' [ ConstInitVal { ',' ConstInitVal } ] '}' 
class ConstInitValList : public __InitValBase {
public:
    std::vector<InitValBase> *initval;
    ConstInitValList(std::vector<InitValBase> *i) : initval(i) {}
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
	ExprBase getExp() {return nullptr;}
};

class ConstInitVal : public __InitValBase {
public:
    ExprBase exp;
    ConstInitVal(ExprBase e) : exp(e) {}
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
	ExprBase getExp() {return exp;}
};

// VarInitVal → Exp | '{' [ InitVal { ',' InitVal } ] '}' 
class VarInitValList : public __InitValBase {
public:
    std::vector<InitValBase> *initval;
    VarInitValList(std::vector<InitValBase> *i) : initval(i) {}
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
	ExprBase getExp() {return nullptr;}
};

class VarInitVal: public __InitValBase { // Multiple Inheritance
public:
    ExprBase exp;
    VarInitVal(ExprBase e) : exp(e) {}
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
	ExprBase getExp() {return exp;}
};

/************************ Def  *************************/

class VarDef_no_init : public __Def {
public:
    Symbol* name;
    std::vector<ExprBase> *dims;
    // 如果dims为nullptr, 表示该变量不含数组下标
    VarDef_no_init(Symbol* n, std::vector<ExprBase> *d) : name(n), dims(d) {}

    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
    Symbol* GetSymbol(){return name;}
};

class VarDef : public __Def {
public:
    Symbol* name;
    std::vector<ExprBase> *dims;
    InitValBase init;
    VarDef(Symbol* n, std::vector<ExprBase> *d, InitValBase i) : name(n), dims(d), init(i) {}

    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
    Symbol* GetSymbol(){return name;}
};

class ConstDef : public __Def {
public:
    Symbol* name;
    std::vector<ExprBase> *dims;
    // 如果dims为nullptr, 表示该变量不含数组下标
    InitValBase init;
    ConstDef(Symbol* n, std::vector<ExprBase> *d, InitValBase i) : name(n), dims(d), init(i) {}

    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
    Symbol* GetSymbol(){return name;}
};

/************************ Decl *************************/

// var definition
class VarDecl : public __DeclBase {
public:
    Type* type_decl;
    std::vector<Def> *var_def_list{};
    // construction
    VarDecl(Type* t, std::vector<Def> *v) : type_decl(t), var_def_list(v) {}
    std::vector<Def>* GetDefs(){return var_def_list;}
    Type* GetTypedecl(){return type_decl;}
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

// const var definition
class ConstDecl : public __DeclBase {
public:
    Type* type_decl;
    std::vector<Def> *var_def_list{};
    // construction
    ConstDecl(Type* t, std::vector<Def> *v) : type_decl(t), var_def_list(v) {}
    std::vector<Def>* GetDefs(){return var_def_list;}
    Type* GetTypedecl(){return type_decl;}
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

/************************ Block *************************/
class __BlockItem : public ASTNode {
public:
};
typedef __BlockItem *BlockItem;

class BlockItem_Decl : public __BlockItem {
public:
    DeclBase decl;
    BlockItem_Decl(DeclBase d) : decl(d) {}
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

class BlockItem_Stmt : public __BlockItem {
public:
    Stmt stmt;
    BlockItem_Stmt(Stmt s) : stmt(s) {}
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};

// block
class __Block : public ASTNode {
public:
    std::vector<BlockItem> *item_list{};
    // construction
    __Block() {}
    __Block(std::vector<BlockItem> *i) : item_list(i) {}
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};
typedef __Block *Block;

/************************** Func *************************/
// FuncParam -> Type IDENT
// FuncParam -> Type IDENT [] {[Exp]}
class __FuncFParam : public ASTNode {
public:
    Type* type_decl;
    std::vector<ExprBase> *dims;
    // 如果dims为nullptr, 表示该变量不含数组下标
    Symbol* name;
    int scope = -1;    // 在语义分析阶段填入正确的作用域

    __FuncFParam(Type* t, Symbol* n, std::vector<ExprBase> *d) {
        type_decl = t;
        name = n;
        dims = d;
    }
	// 函数声明时省略形参 int foo(int [][3]); 
    __FuncFParam(Type* t, std::vector<ExprBase> *d) {
        type_decl = t;
        dims = d;
    }
	// int foo(int, int);
    __FuncFParam(Type* t) : type_decl(t), dims(nullptr) {}
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};
typedef __FuncFParam *FuncFParam;

// return_type name '(' [formals] ')' block
class __FuncDef : public ASTNode {
public:
    Type* return_type;
    Symbol* name;
    std::vector<FuncFParam> *formals;
    Block block;
    __FuncDef(Type* t, Symbol* functionName, std::vector<FuncFParam> *f, Block b) {
        formals = f;
        name = functionName;
        return_type = t;
        block = b;
    }
    void codeIR();
    void TypeCheck();
    void printAST(std::ostream &s, int pad);
};
typedef __FuncDef *FuncDef;


/*****************  CompUnit  ***********************/
class __CompUnit : public ASTNode {
    public:
};
typedef __CompUnit *CompUnit;
class CompUnit_Decl : public __CompUnit {
    public:
        DeclBase decl;
        CompUnit_Decl(DeclBase d) : decl(d) {}
        void codeIR();
        void TypeCheck();
        void printAST(std::ostream &s, int pad);
};
    
class CompUnit_FuncDef : public __CompUnit {
    public:
        FuncDef func_def;
        CompUnit_FuncDef(FuncDef f) : func_def(f) {}
        void codeIR();
        void TypeCheck();
        void printAST(std::ostream &s, int pad);
};

/*****************  Program ***********************/

class __Program : public ASTNode {
    public:
        std::vector<CompUnit> *comp_list;
    
        __Program(std::vector<CompUnit> *c) { comp_list = c; }
        void codeIR();
        void TypeCheck();
        void printAST(std::ostream &s, int pad);
};
typedef __Program *Program;


#endif