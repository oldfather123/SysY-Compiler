#include "AST/expr.h"
#include "AST/stmt.h"
#include "AST/decl.h"
#include "AST/const_value.h"
#include "AST/type.h"
#include "AST/utils.h"
#include "ASTContext.h"
#include "common.h"
#include "frontend.h"
#include "global.h"
#include "sym.h"
#include <deque>
#include <memory>
#include <stack>
#include <vector>

namespace aaac {
namespace frontend {

/*
-----------------------------------
            declaration
-----------------------------------
*/
void TUDecl::Translate(ASTContext *Context) {
    Context->setInGlobal();
    for(auto decl : this->decls) {
        decl->Translate(Context);
    }
}

void VarDecl::Translate(ASTContext *Context) {
    // auto var = Context->NewVar(this->type);
    auto var = Context->NewNamedVar(this->type,this->getIdent());
    Context->insertVarDecl(this, var);
    if(Context->isInGlobal()) {
        std::shared_ptr<InitializeNode> node;
        if(this->getType() == TypeFactory::IntTy) {
            node = InitializeNode::create(var,std::vector<int>(),(init? getValueInt(init): 0));
        } else if(this->getType() == TypeFactory::FloatTy) {
            node = InitializeNode::create(var,std::vector<int>(),(init? getValueFloat(init): float(0.0)));
        }
        Context->emitNode(node);
    } else {
        if(init) {
            init->Translate(Context);
            auto ret = Context->getResult();
            std::shared_ptr<CopyAssignNode> node;
            node = CopyAssignNode::create(var,OperandList({ret}));
            Context->emitNode(node);
        }
    }
}

void ArrayDecl::Translate(ASTContext *Context) {
    auto var = Context->NewNamedVar(this->type, this->getIdent());
    Context->insertVarDecl(this, var);
    if (!Context->isInGlobal()) {
        // 所有的array都是escape的
        Context->CurFunctionNode->addEscape(this, var);
    }
    if(init) {
        init->Translate(Context);
    }
}

void FuncDecl::Translate(ASTContext *Context) {
    auto backend_func = std::make_shared<backend::Function>();
    backend_func->name = this->getIdent();
    auto function = sym::Function::create(type,this->getIdent(),false,backend_func);
    auto node = FunctionNode::create(function);
    Context->setCurFunction(node);
    auto it = fparms->begin();
    for(auto var: function->getParameters()) {
        Context->insertVarDecl(*it++, var);
    }
    Context->insertFuncDecl(this, function);
    if(isFwdDecl()) {
        if (getIdent().find("__builtin_") == 0) {
            Context->insertBuiltinFuncDecl(getIdent(), this);
        }
        return;
    }
    Context->emitNode(node);
    Context->setInGlobal(false);
    body->Translate(Context);
    Context->setInGlobal();
    Context->clearCurFunction();
}

/*
-----------------------------------
            expression
-----------------------------------
*/
void UnaryOperator::TranslateLogic(ASTContext *Context) {
    expr->Translate(Context);
    // TODO: 先这么干了，直觉上貌似是可以的
    Context->exchangePatchList();
    if(getExType() == Expressioned) {
        Context->emitAsExpr();
    }
}

void UnaryOperator::TranslateArith(ASTContext *Context) {
    expr->Translate(Context);
    auto var = Context->getResult();
    if(op == Negative) {
        auto node = NegateAssignNode::create(var,OperandList(1,var));
        Context->emitNode(node);
    }
    if(getExType() == Conditioned) {
        Context->emitAsCond(var);
    } else {
        Context->setResult(var);
    }
}

void UnaryOperator::Translate(ASTContext *Context) {
    if(op == UnaryOp::Negate) {
        TranslateLogic(Context);
    } else {
        TranslateArith(Context);
    }
}

void BinaryOperator::TranslateLogic(ASTContext *Context) {
    Assert(op == AND_OP || op == OR_OP, "invalid operator");
    lhs->Translate(Context);
    auto [True, False] = Context->getPatchListPair();
    auto label = LabelNode::generateTemp();
    Context->emitNode(label);
    if(op == AND_OP) {
        Context->patchList(True,label);
    } else {
        Context->patchList(False, label);
    }
    rhs->Translate(Context);
    if(op == AND_OP) {
        Context->appendFalseList(False);
    } else {
        Context->appendTrueList(True);
    }
}

void BinaryOperator::TranslateCompr(ASTContext *Context) {
    OperandList operands;
    lhs->Translate(Context);
    operands.push_back(Context->getResult());
    rhs->Translate(Context);
    operands.push_back(Context->getResult());
    std::shared_ptr<ConditionNode> node;
    switch (op) {
        case EQ_OP: node = EqConditionNode::create(operands); break;
        case NEQ_OP: node = NeqConditionNode::create(operands); break;
        case LT_OP: node = LtConditionNode::create(operands); break;
        case LE_OP: node = LeqConditionNode::create(operands); break;
        case GT_OP: node = GtConditionNode::create(operands); break;
        case GE_OP: node = GeqConditionNode::create(operands); break;
        default:
            Assert(false, "unreachable");
    }
    Context->emitNode(node);
    Context->appendTrueList(node);
    Context->appendFalseList(node);
    if(getExType() == Expressioned) {
        Context->emitAsExpr();
    }
}

void BinaryOperator::TranslateArith(ASTContext *Context) {
    OperandList operands;
    lhs->Translate(Context);
    operands.push_back(Context->getResult());
    rhs->Translate(Context);
    operands.push_back(Context->getResult());
    std::shared_ptr<AssignNode> node;
    auto var = Context->NewVar(getType());
    switch (op) {
        case PLUS_OP: node = AddAssignNode::create(var,operands); break;
        case MINUS_OP: node = SubAssignNode::create(var,operands); break;
        case TIMES_OP: node = MulAssignNode::create(var,operands); break;
        case DIVIDE_OP: node = DivAssignNode::create(var,operands); break;
        case MODULE_OP: node = ModAssignNode::create(var,operands); break;
        default:
            Assert(false, "unreachable");
    }
    Context->emitNode(node);
    if(getExType() == Conditioned) {
        Context->emitAsCond(var);
    } else {
        Context->setResult(var);
    }
}

void BinaryOperator::Translate(ASTContext *Context) {
    if(PLUS_OP <= op && op <= MODULE_OP) {
        TranslateArith(Context);
    } else if(EQ_OP <= op && op <= GE_OP) {
        TranslateCompr(Context);
    } else {
        TranslateLogic(Context);
    }
}

void IntLiteral::Translate(ASTContext *Context) {
    auto var = Context->NewVar(TypeFactory::IntTy);
    auto node = CopyAssignNode::create(var,OperandList(1,value));
    Context->emitNode(node);
    if(this->getExType() == Conditioned) {
        Context->emitAsCond(var);
    } else {
        Context->setResult(var);
    }
}

void FloatLiteral::Translate(ASTContext *Context) {
    auto var = Context->NewVar(TypeFactory::FloatTy);
    auto node = CopyAssignNode::create(var,OperandList(1,float(value)));
    Context->emitNode(node);
    if(this->getExType() == Conditioned) {
        Context->emitAsCond(var);
    } else {
        Context->setResult(var);
    }
}

void ImplicitCast::Translate(ASTContext *Context) {
    expr->Translate(Context);
    auto var = Context->getResult();
    assert(var != nullptr);
    std::shared_ptr<sym::Var> res;
    std::shared_ptr<AssignNode> node;
    if(castTy == CastType::IntToFloat) {
        res = Context->NewVar(TypeFactory::FloatTy);
        node = FCstAssignNode::create(res,OperandList({var}));
    } else {
        res = Context->NewVar(TypeFactory::IntTy);
        node = ICstAssignNode::create(res,OperandList({var}));
    }
    Context->emitNode(node);
    if(this->getExType() == Conditioned) {
        Context->emitAsCond(res);
    } else {
        Context->setResult(res);
    }
}

void InitList::TranslateGlobal(ASTContext *Context) {
    OperandList operands;
    std::vector<int> st;
    std::vector<InitList *> lists;
    st.push_back(0), lists.push_back(this), operands.push_back(0);
    while(!st.empty()) {
        int index = st.back();
        InitList *list = lists.back();
        if(index >= list->getLen()) {
            st.pop_back();
            operands.pop_back();
            lists.pop_back();
            if(st.empty()) break;
            index = st.back() + 1;
            st.back() = index;
            operands.back() = index;
            continue;
        }
        Expr *expr = list->init_list[index];
        if(typeid(*expr) == typeid(InitList)) {
            st.push_back(0);
            operands.push_back(0);
            lists.push_back(static_cast<InitList*>(expr));
            continue;
        } else {
            auto arr = Context->lookupVarDecl(reference);
            std::variant<int,float> value;
            Assert(expr->isConst(), "init value in global must be const");
            if(expr->getType() == TypeFactory::IntTy) {
                value = getValueInt(expr);
            } else {
                value = getValueFloat(expr);
            }
            auto node = InitializeNode::create(arr,st,value);
            Context->emitNode(node);
        }
        index++;
        st.back() = index;
        operands.back() = index;
    }
}

void InitList::TranslateLocal(ASTContext *Context) {
    OperandList operands;
    std::vector<int> st;
    std::vector<InitList *> lists;

    // for large array
    if (type->getTypeSize() > 16 || true) {
        // 1. call __builtin_memset
        auto memsetType = Context->lookupBuildinFuncDecl("__builtin_memset");
        assert(memsetType != nullptr);
        std::shared_ptr<InsnNode> node = nullptr;
        auto arr = Context->lookupVarDecl(reference);
        auto arg0 = Context->NewVar(getType());
        node = LoadNode::create(arg0,arr,OperandList{});
        Context->emitNode(node);
        auto arg1 = Context->NewVar(TypeFactory::IntTy);
        node = CopyAssignNode::create(arg1, OperandList{Operand(0)});
        Context->emitNode(node);
        auto arg2 = Context->NewVar(TypeFactory::IntTy);
        node = CopyAssignNode::create(arg2, OperandList{Operand(type->getTypeSize())});
        Context->emitNode(node);
        node = CallNode::create(memsetType.get(), nullptr, OperandList{Operand(arg0),Operand(arg1), Operand(arg2)});
        Context->emitNode(node);
        // 2. init rest elements
        st.push_back(0), lists.push_back(this), operands.push_back(0);
        while(!st.empty()) {
            int index = st.back();
            InitList *list = lists.back();
            if(index >= list->getLen()) {
                st.pop_back();
                operands.pop_back();
                lists.pop_back();
                if(st.empty()) break;
                index = st.back() + 1;
                st.back() = index;
                operands.back() = index;
                continue;
            }
            Expr *expr = list->init_list[index];
            if(typeid(*expr) == typeid(InitList)) {
                st.push_back(0);
                operands.push_back(0);
                lists.push_back(static_cast<InitList*>(expr));
                continue;
            } else {
                auto arr = Context->lookupVarDecl(reference);
                expr->Translate(Context);
                auto node = StoreNode::create(arr,operands,Context->getResult());
                Context->emitNode(node);
            }
            index++;
            st.back() = index;
            operands.back() = index;
        }
        return;
    }

    // fill zero one-by-one, for small array
    auto dfs = [&](auto self, InitList *list, std::shared_ptr<ArrayType> list_type, bool peseudo) -> void {
        int index = 0;
        // auto list_type = std::dynamic_pointer_cast<ArrayType>(list->getType());
        while (index < list_type->getArrayLen()) {
            operands.push_back(Operand(index));
            if (!peseudo && index < list->getLen()) {
                Expr *expr = list->init_list[index];
                if (typeid(*expr) == typeid(InitList)) {
                    self(self, static_cast<InitList *>(expr), std::dynamic_pointer_cast<ArrayType>(list_type->getBaseType()), false | peseudo);
                } else {
                    auto arr = Context->lookupVarDecl(reference);
                    if (peseudo) {
                        // 前面的下标已经超出范围了
                        auto node = StoreNode::create(arr,operands,Operand(0));
                        Context->emitNode(node);
                    } else {
                        auto arr = Context->lookupVarDecl(reference);
                        expr->Translate(Context);
                        auto node = StoreNode::create(arr,operands,Context->getResult());
                        Context->emitNode(node);
                    }
                }
            } else {
                if (list_type->getBaseType()->isArrayType()) {
                    // auto *sub = static_cast<InitList*>(list->init_list[0]);
                    self(self, nullptr, std::dynamic_pointer_cast<ArrayType>(list_type->getBaseType()), true);
                } else {
                    auto arr = Context->lookupVarDecl(reference);
                    auto node = StoreNode::create(arr,operands,Operand(0));
                    Context->emitNode(node);
                }
            }
            operands.pop_back();
            index++;
        }
    };
    dfs(dfs,this, std::dynamic_pointer_cast<ArrayType>(this->getType()),false);
}


void InitList::Translate(ASTContext *Context) {
    Assert(reference != nullptr, "must be the outmost InitList");
    if(Context->isInGlobal()) {
        TranslateGlobal(Context);
    } else {
        TranslateLocal(Context);
    }
}

void CallExpr::Translate(ASTContext *Context) {
    auto functiontype = (funcdecl->getFunctionType());
    auto function = Context->lookupFuncDecl(funcdecl);
    std::shared_ptr<sym::Var> ret = nullptr;
    if(functiontype->getRetType() != TypeFactory::VoidTy) {
        ret = Context->NewVar(functiontype->getRetType());
    }
    OperandList args;
    for(auto rparm: *rparms) {
        rparm->Translate(Context);
        if (typeid(*rparm) == typeid(Var) && rparm->getType()->isArrayType()) {
            auto var = Context->NewVar(rparm->getType());
            auto node = LoadNode::create(var, Context->getResult(), OperandList{});
            Context->emitNode(node);
            args.push_back(var);
            continue;
        }
        args.push_back(Context->getResult());
    }
    auto node = CallNode::create(function.get(),ret,args);
    Context->emitNode(node);
    if(this->getExType() == Conditioned) {
        Context->emitAsCond(ret);
    } else {
        Context->setResult(ret);
    }
}

void Var::Translate(ASTContext *Context) {
    std::shared_ptr<sym::Var> var = Context->lookupVarDecl(reference);
    if(reference->isGlobal() && !getType()->isArrayType()) {
        auto global = var;
        var = Context->NewVar(global->getType());
        Context->emitNode(LoadNode::create(var,global,OperandList()));
    }
    if(this->getExType() == Conditioned) {
        Context->emitAsCond(var);
    } else {
        Context->setResult(var);
    }
}

void Var::TranslateAssign(ASTContext *Context, Operand var) {
    Assert(reference->isGlobal(),"must be global variable");
    std::shared_ptr<sym::Var> assigned = Context->lookupVarDecl(reference);
    auto node = StoreNode::create(assigned,OperandList{},var);
    Context->emitNode(node);
}

void SubscriptVar::Translate(ASTContext *Context) {
    // Assert(getType()->isBuiltinType(), "must be IntTy or FloatTy, i.e. outmost layer");
    std::shared_ptr<sym::Var> arr;
    OperandList indexes;
    std::deque<SubscriptVar*> st;
    st.push_back(this);
    while(true) {
        SubscriptVar *subvar = st.front();
        if(typeid(*subvar->var) == typeid(SubscriptVar)) {
            st.push_front(static_cast<SubscriptVar*>(subvar->var));
            continue;
        }
        subvar->var->Translate(Context);
        arr = Context->getResult();
        break;
    }
    for(auto sub: st) {
        sub->subscript->Translate(Context);
        indexes.push_back(Context->getResult());
    }
    auto var = Context->NewVar(getType());
    auto node = LoadNode::create(var,arr,indexes);
    Context->emitNode(node);
    if(this->getExType() == Conditioned) {
        Context->emitAsCond(var);
    } else {
        Context->setResult(var);
    }
}

void SubscriptVar::TranslateAssign(ASTContext *Context, Operand var) {
    // Assert(getType()->isBuiltinType(), "must be IntTy or FloatTy, i.e. outmost layer");
    std::shared_ptr<sym::Var> arr;
    OperandList indexes;
    std::deque<SubscriptVar*> st;
    st.push_back(this);
    while(true) {
        SubscriptVar *subvar = st.front();
        if(typeid(*subvar->var) == typeid(SubscriptVar)) {
            st.push_front(static_cast<SubscriptVar*>(subvar->var));
            continue;
        }
        subvar->var->Translate(Context);
        arr = Context->getResult();
        break;
    }
    for(auto sub: st) {
        sub->subscript->Translate(Context);
        indexes.push_back(Context->getResult());
    }
    auto node = StoreNode::create(arr,indexes,var);
    Context->emitNode(node);
}

void EmptyExpr::Translate(ASTContext *Context) {
    return;
}

/*
-----------------------------------
            statement
-----------------------------------
*/
void AssignStmt::Translate(ASTContext *Context) {
    std::shared_ptr<sym::Var> dest, src;
    std::shared_ptr<CopyAssignNode> node;
    
    expr->Translate(Context);
    src = Context->getResult();
    if(typeid(*var) != typeid(SubscriptVar) && !var->isGlobal()) {
        var->Translate(Context);
        dest = Context->getResult();
        node = CopyAssignNode::create(dest,OperandList({src}));
        Context->emitNode(node);
    } else {
        auto subvar = static_cast<SubscriptVar*>(var);
        subvar->TranslateAssign(Context, src);
    }
}

void DeclStmt::Translate(ASTContext *Context) {
    decl->Translate(Context);
}

void CompondStmt::Translate(ASTContext *Context) {
    for(auto stmt: stmts) {
        stmt->Translate(Context);
    }
}

void IfStmt::Translate(ASTContext *Context) {
    if(elsee) {
        TranslateWithElse(Context);
    } else {
        TranslateNoElse(Context);
    }
}

void IfStmt::TranslateWithElse(ASTContext *Context) {
/*
    cond -> true: L_then, false: L_else
    L_then: then
    goto L_end
    L_else: else
    L_end:
*/
    auto L_then = LabelNode::generateTemp();
    auto L_else = LabelNode::generateTemp();
    auto L_end = LabelNode::generateTemp();
    cond->Translate(Context);
    Context->patchTrue(L_then);
    Context->patchFalse(L_else);
    Context->emitNode(L_then);
    then->Translate(Context);
    Context->emitNode(JumpNode::create(L_end));
    Context->emitNode(L_else);
    elsee->Translate(Context);
    Context->emitNode(L_end);
}

void IfStmt::TranslateNoElse(ASTContext *Context) {
/*
    cond -> true: L_then, false: L_end
    L_then: then
    L_end;
*/
    auto L_then = LabelNode::generateTemp();
    auto L_end = LabelNode::generateTemp();
    cond->Translate(Context);
    Context->patchTrue(L_then);
    Context->patchFalse(L_end);
    Context->emitNode(L_then);
    then->Translate(Context);
    Context->emitNode(L_end);
}

void WhileStmt::Translate(ASTContext *Context) {
/*
    L_cont:
    cond -> true: L_truee, false: L_brk
    L_truee:
    body
    goto L_cont
    L_brk:
*/
    auto cont = LabelNode::generateTemp();
    auto brk = LabelNode::generateTemp();
    auto truee = LabelNode::generateTemp();
    Context->whiles_push({cont,brk});
    Context->emitNode(cont);
    cond->Translate(Context);
    Context->emitNode(truee);
    Context->patchTrue(truee);
    Context->patchFalse(brk);
    body->Translate(Context);
    Context->emitNode(JumpNode::create(cont));
    Context->emitNode(brk);
    Context->whiles_pop();
}

void ReturnStmt::Translate(ASTContext *Context) {
    if(!ret) {
        Context->emitNode(ReturnNode::create());
        return;
    }
    ret->Translate(Context);
    auto node = ReturnNode::create(Context->getResult());
    Context->emitNode(node);
}

void BreakStmt::Translate(ASTContext *Context) {
    auto [cont,brk] = Context->whiles_top();
    auto node = JumpNode::create(brk);
    Context->emitNode(node);
}

void ContinueStmt::Translate(ASTContext *Context) {
    auto [cont,brk] = Context->whiles_top();
    auto node = JumpNode::create(cont);
    Context->emitNode(node);
}

void EmptyStmt::Translate(ASTContext *Context) {
    Context->emitNode(NopNode::create());
}

} // namespace frontend
} // namespace aaac