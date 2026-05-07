/*
what semant ayanlysis in aaac handles in detail is listed in following:
1. fixup the initlist, i.e. {1,2,{}} -> {{1,2},{}} for int a[3][2]
2. analyze const property, propagate const property, and caculate const value
3. analyze type, fixup all expr's type, and assert whether two exprs' type is matched
4. fixup varexpr's reference to vardecl
*/
#include "AST/decl.h"
#include "AST/expr.h"
#include "AST/stmt.h"
#include "AST/type.h"
#include "AST/utils.h"
#include "ASTContext.h"
#include "common.h"
#include "errmsg.h"
#include <cstddef>
#include <iostream>
#include <memory>
#include <vector>
namespace aaac {
namespace frontend {
#define CHECK_ERR(context) \
do {\
if(context->hasErr()) return;\
} while(0)\

/*
-----------------------------------
            declaration
-----------------------------------
*/
void TUDecl::PopulateLibFunction() {
    Assert(HasPopulatedLibFunc == false, "only can be called once");
    /* construct runtime function */
    FuncDecl *LibFunction = nullptr;
    {
        // int getint();
        LibFunction = new FuncDecl(0,"int","getint",new FParmList,new CompondStmt(0));
        LibFunction->setFwdDecl();
        decls.push_back(LibFunction);
    }
    {
        // int getch();
        LibFunction = new FuncDecl(0,"int","getch",new FParmList,new CompondStmt(0));
        LibFunction->setFwdDecl();
        decls.push_back(LibFunction);
    }
    {
        // int getarray(int a[]);
        ArrayDecl *fparm = new ArrayDecl(0,"int","a",nullptr);
        fparm->appendSubscript(new EmptyExpr(0));
        LibFunction = new FuncDecl(0,"int","getarray",new FParmList({fparm}),new CompondStmt(0));
        LibFunction->setFwdDecl();
        decls.push_back(LibFunction);
    }
    {
        // float getfloat();
        LibFunction = new FuncDecl(0,"float","getfloat",new FParmList,new CompondStmt(0));
        LibFunction->setFwdDecl();
        decls.push_back(LibFunction);
    }
    {
        // int getfarray(float a[]);
        ArrayDecl *fparm = new ArrayDecl(0,"float","a",nullptr);
        fparm->appendSubscript(new EmptyExpr(0));
        LibFunction = new FuncDecl(0,"int","getfarray",new FParmList({fparm}),new CompondStmt(0));
        LibFunction->setFwdDecl();
        decls.push_back(LibFunction);
    }
    {
        // void putint(int a)
        VarDecl *fparm = new VarDecl(0,"int","a",nullptr);
        LibFunction = new FuncDecl(0,"void","putint",new FParmList{fparm},new CompondStmt(0));
        LibFunction->setFwdDecl();
        decls.push_back(LibFunction);
    }
    {
        // void putch(int a)
        VarDecl *fparm = new VarDecl(0,"int","a",nullptr);
        LibFunction = new FuncDecl(0,"void","putch",new FParmList{fparm},new CompondStmt(0));
        LibFunction->setFwdDecl();
        decls.push_back(LibFunction);
    }
    {
        // void putarray(int n,int a[])
        VarDecl *fparm1 = new VarDecl(0,"int","n",nullptr);
        ArrayDecl *fparm2 = new ArrayDecl(0,"int","a",nullptr);
        fparm2->appendSubscript(new EmptyExpr(0));
        LibFunction = new FuncDecl(0,"void","putarray",new FParmList{fparm1,fparm2},new CompondStmt(0));
        LibFunction->setFwdDecl();
        decls.push_back(LibFunction);
    }
    {
        // void putfloat(float a)
        VarDecl *fparm = new VarDecl(0,"float","a",nullptr);
        LibFunction = new FuncDecl(0,"void","putfloat",new FParmList{fparm},new CompondStmt(0));
        LibFunction->setFwdDecl();
        decls.push_back(LibFunction);
    }
    {
        // void putfarray(int n, float a[])
        VarDecl *fparm1 = new VarDecl(0,"int","n",nullptr);
        ArrayDecl *fparm2 = new ArrayDecl(0,"float","a",nullptr);
        fparm2->appendSubscript(new EmptyExpr(0));
        LibFunction = new FuncDecl(0,"void","putfarray",new FParmList{fparm1,fparm2},new CompondStmt(0));
        LibFunction->setFwdDecl();
        decls.push_back(LibFunction);
    }
    {
        // void putf(char a[], ...)
        // TODO
    }
    {
        // void starttime()
        LibFunction = new FuncDecl(0,"void","starttime",new FParmList,new CompondStmt(0));
        LibFunction->setFwdDecl();
        decls.push_back(LibFunction);
    }
    {
        // void stoptime()
        LibFunction = new FuncDecl(0,"void","stoptime",new FParmList,new CompondStmt(0));
        LibFunction->setFwdDecl();
        decls.push_back(LibFunction);
    }
    {
        // void _sysy_starttime(int lineno)
        VarDecl *fparm1 = new VarDecl(0,"int","lineno",nullptr);
        LibFunction = new FuncDecl(0,"void","_sysy_starttime",new FParmList{fparm1},new CompondStmt(0));
        LibFunction->setFwdDecl();
        decls.push_back(LibFunction);
    }
    {
        // void _sysy_stoptime(int lineno)
        VarDecl *fparm1 = new VarDecl(0,"int","lineno",nullptr);
        LibFunction = new FuncDecl(0,"void","_sysy_stoptime",new FParmList{fparm1},new CompondStmt(0));
        LibFunction->setFwdDecl();
        decls.push_back(LibFunction);
    }
    // declaration of builtin function
    {
        // int __builtin_memset(int ptr[], int value, int num);
        ArrayDecl *fparm1 = new ArrayDecl(0,"int","ptr",nullptr);
        fparm1->appendSubscript(new EmptyExpr(0));
        VarDecl *fparm2 = new VarDecl(0,"int","value",nullptr);
        VarDecl *fparm3 = new VarDecl(0,"int","num",nullptr);
        LibFunction = new FuncDecl(0, "void", "__builtin_memset",new FParmList{fparm1,fparm2,fparm3},new CompondStmt(0));
        LibFunction->setFwdDecl();
        decls.push_back(LibFunction);
    }
    HasPopulatedLibFunc = true;
}

void TUDecl::SemAnalyze(ASTContext *Context) {
    auto &venv = Context->venv;
    auto &fenv = Context->fenv;
    venv.BeginScope();
    fenv.BeginScope();
    Context->setInGlobal();
    for(auto decl : decls) {
        decl->SemAnalyze(Context);
    }
    Context->setInGlobal(false);
    venv.EndScope();
    fenv.EndScope();
}

void VarDecl::SemAnalyze(ASTContext *Context) {
    auto TypeFactory = TypeFactory::getTypeFactory();
    auto &venv = Context->venv;
    this->setGlobal(Context->isInGlobal());
    // get var type
    if(type == nullptr) {
        type = TypeFactory->getBuiltinType(BType);
    }
    if(isConst() && init == nullptr) {
        Context->errmsg->InsertErrAt(getPos(), 
        "const variable declaration should have const init expr");
        return;
    }
    // handle init expression
    if(init) {
        Assert(!isFormalParm(), "formal parm can't be inited");
        init->SemAnalyze(Context);
        if(!init->getType()->isBuiltinType()) {
            Context->errmsg->InsertErrAt(getPos(), 
            "type of init expr of simple variable should be builtin type");
            return;
        }
        // init expr of const var must be const
        if(isConst() && !init->isConst()) {
            Context->errmsg->InsertErrAt(getPos(), 
                        "const variable declaration should have const init expr");
            return;
        }
        if(isGlobal() && !init->isConst()) {
            Context->errmsg->InsertErrAt(getPos(), 
                        "global variable declaration should have const init expr");
            return;
        }
        // fixup implicit cast node
        if(*type == *TypeFactory::FloatTy && *init->getType() == *TypeFactory::IntTy) {
            init = new ImplicitCast(init,CastType::IntToFloat);
        } else if(*type == *TypeFactory::IntTy && *init->getType() == *TypeFactory::FloatTy) {
            init = new ImplicitCast(init,CastType::FloatToInt);
        }
        // substitute origin init expr for the calculated result if it is const var decl
        // TODO: following can be substituted by copyLiteral
        if(isConst() || isGlobal()) {
            Expr* old_init  = init;
            Expr* const_expr = old_init->getConstExpr();
            if(isIntLiteral(const_expr)) {
                init = new IntLiteral(old_init->getPos(),getValueInt(const_expr));
            } else if(isFloatLiteral(const_expr)) {
                init = new FloatLiteral(old_init->getPos(),getValueFloat(const_expr));
            } else {
                Assert(false, "unreachable");
            }
            delete old_init;
            Assert(isLiteral(init),"current init must be a const literal");
        }
    }
    venv.insert(getIdent(), this);
    // std::cout << "venv insert " << getIdent() << " " << this << std::endl;
}
void ArrayDecl::SemAnalyze(ASTContext *Context) {
    // all array var is escape
    setEscape(); 
    // construct type of this declaration
    auto TFactory = TypeFactory::getTypeFactory();
    type = TFactory->getBuiltinType(BType);
    /*  对于int [2][3][4]的Array type, type的嵌套在逻辑上等价于
        typedef typea = int [4]
        typedef typeb = typea [3]
        typedef typec = typeb [2]
        typec = int [2][3][4]
    */
    for(auto it = subscripts.rbegin(); it != subscripts.rend(); it++) {
        if(isFormalParm() && it == subscripts.rend() - 1) {
            type = TFactory->getArrayType(type, 0);
            continue;
        }
        Expr* &expr = *it;
        // std::cout << this->getAsString() << " " << expr->getAsString() << std::endl;
        expr->SemAnalyze(Context);
        if(!isIntLiteral(expr->getConstExpr())) {
            Context->errmsg->InsertErrAt(expr->getPos(), 
            "array subscript in declaration must be const int");
            return;
        }
        int sub = getValueInt(expr->getConstExpr());
        type = TFactory->getArrayType(type, sub);
        expr = new IntLiteral(expr->getPos(),sub);
    }
    this->setGlobal(Context->isInGlobal());
    // printf("array : %s -> %s isconst %d\n",getIdent().c_str(),type->getAsString().c_str(),isConst());
    if(isFormalParm()) {
        Context->venv.insert(getIdent(), this);    
        return;
    }
    // start handle init expr
    if(isConst() && init == nullptr) {
        Context->errmsg->InsertErrAt(getPos(), 
        "const variable declaration should have const init expr");
        return;
    }
    // fixup init list for non-const array declaration
    if(init == nullptr) {
        // init = new InitList(getPos());
        Context->venv.insert(getIdent(), this);
        return;
    }

    if(typeid(*init) != typeid(InitList)) {
        Context->errmsg->InsertErrAt(getPos(), "init expr of array must be init list");
        return;
    }

    // set arraytype of init_list
    init->setType(type);

    init->SemAnalyze(Context);
    if(isConst() || init->isConst()) {
        if(!init->isConst()) {
            Context->errmsg->InsertErrAt(getPos(), 
            "const array declaration should have const init expr");
            return;
        }
        InitList *new_init = static_cast<InitList*>(init)->copy_constlist();
        delete init;
        init = new_init;
    }
    static_cast<InitList*>(init)->setReference(this);

    Context->venv.insert(getIdent(), this);
}

void FuncDecl::SemAnalyze(ASTContext *Context) {
    auto &venv = Context->venv;
    auto &fenv = Context->fenv;
    auto TFactory = TypeFactory::getTypeFactory();
    venv.BeginScope();
    Context->setInGlobal(false);
    std::shared_ptr<Type> ret_type = TFactory->getBuiltinType(retType);
    std::vector<std::shared_ptr<Type>> formals_type;
    for(FParmVarDecl *fparm: *fparms) {
        fparm->setFormalParm();
        fparm->SemAnalyze(Context);
        formals_type.push_back(fparm->getType());
    }
    type = TFactory->getFuncType(formals_type, ret_type);
    fenv.insert(getIdent(), this);
    Context->InFunc = this;
    body->SemAnalyze(Context);
    body->appendStmt(new ReturnStmt(0,(retType == "void"?nullptr:new IntLiteral(0,0))));
    Context->InFunc = nullptr;
    // printf("func analyzed : %s\n", type->getAsString().c_str());
    venv.EndScope();
    Context->setInGlobal();
}

/*
-----------------------------------
            expression
-----------------------------------
*/
void UnaryOperator::SemAnalyze(ASTContext *Context) {
    if(op == Negate) {
        expr->setExType(ExType::Conditioned);
    }
    expr->SemAnalyze(Context);
    setType(expr->getType());
    if(expr->isConst() && op != Negate) {
        int co = ((op == Positive)?1:-1);
        setConst();
        auto literal = copyLiteral(expr->getConstExpr());
        if(expr->getType() == TypeFactory::IntTy) {
            auto intliteral = static_cast<IntLiteral*>(literal);
            intliteral->setValue(co * intliteral->getValue());
            setConstExpr(intliteral);
        } else if(expr->getType() == TypeFactory::FloatTy) {
            auto floatliteral = static_cast<FloatLiteral*>(literal);
            floatliteral->setValue(co * floatliteral->getValue());
            setConstExpr(floatliteral);
        }
    }
}

void BinaryOperator::CondSemAnalyze(ASTContext *Context) {
    bool lhs_int = (lhs->getType() == TypeFactory::IntTy);
    bool rhs_int = (rhs->getType() == TypeFactory::IntTy);
    if(!lhs_int || !rhs_int) {
        if(lhs_int) {
            auto extype = lhs->getExType();
            lhs->setExType(Expressioned);
            lhs = new ImplicitCast(lhs,CastType::IntToFloat);
            lhs->setExType(extype);
        }
        if(rhs_int) {
            auto extype = rhs->getExType();
            rhs->setExType(Expressioned);
            rhs = new ImplicitCast(rhs,CastType::IntToFloat);
            rhs->setExType(extype);
        }
    }
    setType(TypeFactory::IntTy);
}

void BinaryOperator::ArithSemAnalyze(ASTContext *Context) {
    auto TFactory = TypeFactory::getTypeFactory();
    if(lhs->isConst() && rhs->isConst()) {
        setConst();
    }
    bool lhs_int = (lhs->getType() == TypeFactory::IntTy);
    bool rhs_int = (rhs->getType() == TypeFactory::IntTy);
    if(!lhs_int || !rhs_int) {
        if(op == MODULE_OP) {
            Context->errmsg->InsertErrAt(getPos(), "module operator requirs Int type");
            return;
        }
        setType(TFactory->getBuiltinType("float"));
        if(lhs_int) {
            lhs = new ImplicitCast(lhs,CastType::IntToFloat);
        }
        if(rhs_int) {
            rhs = new ImplicitCast(rhs,CastType::IntToFloat);
        }
    } else {
        setType(TypeFactory::IntTy);
    }
    // if two operands are const, calculate the result
    if(isConst()) {
        if(type == TypeFactory::IntTy) {
            int lhs_op = getValueInt(lhs->getConstExpr());
            int rhs_op = getValueInt(rhs->getConstExpr());
            int res = calculateArith(lhs_op, rhs_op, op);
            setConstExpr(new IntLiteral(getPos(),res));
        } else if(type == TypeFactory::FloatTy) {
            float lhs_op = getValueFloat(lhs->getConstExpr());
            float rhs_op = getValueFloat(rhs->getConstExpr());
            int res = calculateArith(lhs_op, rhs_op, op);
            setConstExpr(new FloatLiteral(getPos(),res));
        }
    }
}

// TODO: relational operators
void BinaryOperator::SemAnalyze(ASTContext *Context) {
    if(PLUS_OP <= op && op <= GE_OP) {
        lhs->setExType(Expressioned);
        rhs->setExType(Expressioned);
    } else if(AND_OP <= op && op <= OR_OP) {
        lhs->setExType(Conditioned);
        rhs->setExType(Conditioned);
    } else {
        Assert(false, "unreachable");
    }
    rhs->SemAnalyze(Context);
    lhs->SemAnalyze(Context);
    if(!lhs->getType()->isBuiltinType() || !rhs->getType()->isBuiltinType()) {
        Context->errmsg->InsertErrAt(getPos(), 
            "operands of a BinaryOperator should be a builtin type");
        return;
    }
    if(isArithOp(op)) {
        ArithSemAnalyze(Context);
    } else if(isCondOp(op)) {
        CondSemAnalyze(Context);
    }
}

void IntLiteral::SemAnalyze(ASTContext *Context) {
    // setConst();
    // setConstExpr(new IntLiteral(getPos(),getValue()));
}

void FloatLiteral::SemAnalyze(ASTContext *Context) {
    // setConst();
    // setConstExpr(new FloatLiteral(getPos(),getValue()));
}

void ImplicitCast::SemAnalyze(ASTContext *Context) {
    return;
}

void InitList::SemAnalyze(ASTContext *Context) {
    std::vector<std::shared_ptr<ArrayType>> Types;
    auto cur_type = std::dynamic_pointer_cast<ArrayType>(type);
    Types.push_back(cur_type);
    while (true) {
        auto base = std::dynamic_pointer_cast<ArrayType>(Types.back()->getBaseType());
        if (base == nullptr) {
            break;
        }
        Types.push_back(base);
    }
    std::vector<InitList *> new_lists(Types.size(), nullptr);
    int idx = 0;
    new_lists[idx] = new InitList(getPos());
    new_lists[idx]->setType(Types[idx]);
    new_lists[idx]->setConst();
    auto func = [&] () -> void {
        for (size_t i = 0; i < init_list.size(); i++) {
            Expr *expr = init_list[i];
            if (typeid(*expr) == typeid(InitList)) {
                assert(Types[idx + 1]->isArrayType());
                expr->setType(Types[idx + 1]);
                expr->SemAnalyze(Context);
                new_lists[idx]->append(expr);
                if (!expr->isConst()) {
                    new_lists[idx]->setConst(false);
                }
                if (new_lists[idx]->init_list.size() == Types[idx]->getArrayLen()) {
                    if (idx == 0) {
                        return;
                    }
                    new_lists[idx - 1]->append(new_lists[idx]);
                    if (!new_lists[idx]->isConst()) {
                        new_lists[idx - 1]->setConst(false);
                    }
                    new_lists[idx] = nullptr;
                    idx -= 1;
                }
                continue;
            }
            expr->SemAnalyze(Context);
            if (expr->getType()->isBuiltinType()) {
                if (expr->getType() != cur_type->getElementType()) {
                    if (cur_type->getElementType() == TypeFactory::IntTy) {
                        Context->errmsg->InsertErrAt(expr->getPos(), "element should be float not int");
                        break;
                    }
                    expr = new ImplicitCast(expr, CastType::IntToFloat);
                }
                while (idx + 1 < new_lists.size() && new_lists[idx + 1] == nullptr) {
                    idx += 1;
                    new_lists[idx] = new InitList(getPos());
                    new_lists[idx]->setType(Types[idx]);
                    new_lists[idx]->setConst();
                }
                new_lists[idx]->append(expr);
                if (!expr->isConst()) {
                    new_lists[idx]->setConst(false);
                }
                if (new_lists[idx]->init_list.size() == Types[idx]->getArrayLen()) {
                    if (idx == 0) {
                        return;
                    }
                    new_lists[idx - 1]->append(new_lists[idx]);
                    if (!new_lists[idx]->isConst()) {
                        new_lists[idx - 1]->setConst(false);
                    }
                    new_lists[idx] = nullptr;
                    idx -= 1;
                }
            }
        }
        while (idx) {
            new_lists[idx - 1]->append(new_lists[idx]);
            if (!new_lists[idx]->isConst()) {
                new_lists[idx - 1]->setConst(false);
            }
            new_lists[idx] = nullptr;
            idx -= 1;
        }
    };
    func();
    setConst(new_lists[0]->isConst());
    this->clear();
    std::swap(init_list,new_lists[0]->init_list);
    for (auto ptr: new_lists) {
        delete ptr;
    }
}

void CallExpr::SemAnalyze(ASTContext *Context) {
    auto &fenv = Context->fenv;
    if (func_name == "starttime") {
        func_name = "_sysy_starttime";
        rparms->push_back(new IntLiteral(getPos(),Context->getLineOf(getPos())));
    } else if (func_name == "stoptime") {
        func_name = "_sysy_stoptime";
        rparms->push_back(new IntLiteral(getPos(),Context->getLineOf(getPos())));
    }
    auto funcdecl = fenv.find(func_name);
    if(funcdecl == nullptr) {
        Context->errmsg->InsertErrAt(getPos(), 
        "function %s hasn't been declared",func_name.c_str());
        return;
    }
    auto functiontype = funcdecl->getFunctionType();
    auto it = functiontype->getFormals().begin();
    setType(functiontype);
    for(auto &rparm: *rparms) {
        rparm->setExType(Expressioned);
        rparm->SemAnalyze(Context);
        auto Ftype = *it;
        if(typeid(*rparm) == typeid(Var) && Ftype->isArrayType() && rparm->getType()->isArrayType()) {
            std::cout << rparm->getAsString() << " maybe array" << std::endl;
        }
        if (!rparm->getType()->isArrayType() && Ftype != rparm->getType()) {
            if (Ftype == TypeFactory::FloatTy) {
                rparm = new ImplicitCast(rparm,CastType::IntToFloat);
            } else {
                rparm = new ImplicitCast(rparm,CastType::FloatToInt);
            }
        }
        it++;
    }
    this->funcdecl = funcdecl;
}

void Var::SemAnalyze(ASTContext *Context) {
    auto &venv = Context->venv;
    reference = venv.find(ident);
    // std::cout << "venv find " << ident << " " << reference << std::endl;
    if(reference == nullptr) {
        Context->errmsg->InsertErrAt(getPos(), 
        "var %s hasn't been declared",ident.c_str());
        return;
    }
    setType(reference->type);
    if(reference->isConst()) {
        setConst();
        if(type->isBuiltinType()) {
            setConstExpr(copyLiteral(reference->init));
        } else if(type->isArrayType()) {
            setConstExpr(static_cast<InitList*>(reference->init)->copy_constlist());
        }
    }
}
void SubscriptVar::SemAnalyze(ASTContext *Context) {
/*
    以int a[2][3][4]; a[1][2][3]为例，subvar的结构是
                      int - subvar
                            /    \
               int[4] - subvar    3
                        /    \
        int[3][4] - subvar    2
                    /    \
   int [2][3][4] - a      1
*/
    var->SemAnalyze(Context);
    if(!var->getType()->isArrayType()) {
        Context->errmsg->InsertErrAt(getPos(), 
        "Subscripted value is not an array");
        return;
    }
    subscript->SemAnalyze(Context);
    if(subscript->getType() != TypeFactory::IntTy) {
        Context->errmsg->InsertErrAt(getPos(), 
        "Array subscript is not an integer");
        return;
    }
    auto arr_type = std::static_pointer_cast<ArrayType>(var->getType());
    setType(arr_type->getBaseType());
    if(var->isConst() && subscript->isConst()) {
        setConst();
        int sub = getValueInt(subscript->getConstExpr());
        InitList *list = static_cast<InitList*>(var->getConstExpr());
        if(type->isArrayType()) {
            // TODO: we assume the subscript is within range. if it's out of range, 
            // the result of copy_constlist is nullptr, we should check this
            auto ptr = list->copy_constlist(sub);
            if(ptr == nullptr) {
                Context->errmsg->InsertErrAt(getPos(), "subscript out of range");
                return;
            }
            setConstExpr(ptr);
        } else if(type->isBuiltinType()) {
            auto ptr = list->get_element(sub);
            if(ptr == nullptr) {
                Context->errmsg->InsertErrAt(getPos(), "subscript out of range");
                return;
            }
            setConstExpr(ptr);
        }
    }
}

void EmptyExpr::SemAnalyze(ASTContext *Context) {
    setExType(Voided);
}

/*
-----------------------------------
            statement
-----------------------------------
*/
void AssignStmt::SemAnalyze(ASTContext *Context) {
    var->SemAnalyze(Context);
    if(!var->getType()->isBuiltinType()) {
        Context->errmsg->InsertErrAt(getPos(), "lvalue should be int or float");
        return;
    }
    if(var->isConst()) {
        Context->errmsg->InsertErrAt(getPos(), "lvalue cannot be const");
        return;
    }
    expr->SemAnalyze(Context);
    // fixup implicit cast node
    if(var->getType() == TypeFactory::FloatTy && expr->getType() == TypeFactory::IntTy) {
        expr = new ImplicitCast(expr,CastType::IntToFloat);
    } else if(var->getType() == TypeFactory::IntTy && expr->getType() == TypeFactory::FloatTy) {
        expr = new ImplicitCast(expr,CastType::FloatToInt);
    }
}

void DeclStmt::SemAnalyze(ASTContext *Context) {
    decl->SemAnalyze(Context);
}

void CompondStmt::SemAnalyze(ASTContext *Context) {
    auto &venv = Context->venv;
    venv.BeginScope();
    for(Stmt *stmt: stmts) {
        stmt->SemAnalyze(Context);
    }
    venv.EndScope();
}

void IfStmt::SemAnalyze(ASTContext *Context) {
    cond->setExType(Expr::Conditioned);
    cond->SemAnalyze(Context);
    then->SemAnalyze(Context);
    if(elsee) {
        elsee->SemAnalyze(Context);
    }
}

void WhileStmt::SemAnalyze(ASTContext *Context) {
    cond->setExType(Expr::Conditioned);
    cond->SemAnalyze(Context);
    Context->while_depth++;
    body->SemAnalyze(Context);
    Context->while_depth--;
}

void ReturnStmt::SemAnalyze(ASTContext *Context) {
    auto funcdecl = Context->InFunc;
    auto functype = funcdecl->getFunctionType();
    if(ret) {
        ret->SemAnalyze(Context);
        // std::cout << "return stmt: " << ret->getAsString() << "\n";
        if(ret->getType() != functype->getRetType()) {
            if(ret->getType() == TypeFactory::VoidTy ||
            functype->getRetType() == TypeFactory::VoidTy) {
                Context->errmsg->InsertErrAt(getPos(), "return type unmatch");
                return;    
            }
            if(functype->getRetType() == TypeFactory::IntTy) {
                ret = new ImplicitCast(ret,CastType::FloatToInt);
            }
            if(functype->getRetType() == TypeFactory::FloatTy) {
                ret = new ImplicitCast(ret,CastType::IntToFloat);
            }
        }
        return;
    }
    if(TypeFactory::VoidTy != functype->getRetType()) {
        Context->errmsg->InsertErrAt(getPos(), "return type unmatch");
    }
}

void BreakStmt::SemAnalyze(ASTContext *Context) {
    if(Context->while_depth == 0) {
        Context->errmsg->InsertErrAt(getPos(), "break should be inside while stmt");
    }
}

void ContinueStmt::SemAnalyze(ASTContext *Context) {
    if(Context->while_depth == 0) {
        Context->errmsg->InsertErrAt(getPos(), "break should be inside while stmt");
    }
}

void EmptyStmt::SemAnalyze(ASTContext *Context) {

}

} // namespace aaac
} // namespace frontend
