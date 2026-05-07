#ifndef AAAC_ASTCONTEXT_H
#define AAAC_ASTCONTEXT_H

#include "ADT/scoped_table.h"
#include "AST/decl.h"
#include "AST/expr.h"
#include "myparser.h"
#include "errmsg.h"
#include "frontend.h"
#include "sym.h"
#include <memory>
#include <string>
#include <stack>
#include <utility>

namespace aaac {
namespace frontend {
using namespace ::frontend::err;

using VEnv = ADT::ScopedTable<std::string, VarDecl>;
using FEnv = ADT::ScopedTable<std::string, FuncDecl>;

class ASTContext {
friend class TUDecl;
friend class VarDecl;
friend class ArrayDecl;
friend class FuncDecl;
friend class UnaryOperator;
friend class BinaryOperator;
friend class IntLiteral;
friend class FloatLiteral;
friend class ImplicitCast;
friend class InitList;
friend class CallExpr;
friend class Var;
friend class SubscriptVar;
friend class CompondStmt;
friend class AssignStmt;
friend class DeclStmt;
friend class CompondStmt;
friend class IfStmt;
friend class ReturnStmt;
friend class WhileStmt;
friend class BreakStmt;
friend class ContinueStmt;
friend class EmptyStmt;
public:
ASTContext(std::string fname, std::ostream &out = std::cout):
    parser(std::make_unique<Parser>(fname,out)),module(Module::create()) {}
ASTContext(std::istream &in, std::ostream &out = std::cout):
    parser(std::make_unique<Parser>(in,out)),module(Module::create()) {}

void parse() {
    parser->parse();
    TU = std::unique_ptr<TUDecl>(parser->getTUDecl());
    errmsg = parser->transferErrMsg();
    // std::cout << "-------------------------------------------\n";
    // TU->PrintTo(std::cout, 0);
    // std::cout << "-------------------------------------------\n";
    TU->SemAnalyze(this);
    std::cout << "-------------------------------------------\n";
    TU->PrintTo(std::cout, 0);
    std::cout << "-------------------------------------------\n";
}

void translate() {
    TU->Translate(this);
    // std::cout << "-------------------------------------------\n";
    // std::cout << "AFTER TRANSLATE\n";
    // module->dump(std::cout);
    // std::cout << "-------------------------------------------\n";
}

std::unique_ptr<ErrMsg> transferErrMsg() {return std::move(errmsg);}
std::shared_ptr<Module> transferModule() {return std::move(module);}

bool hasErr() { return errmsg->hasErr(); };

private:
    int getLineOf(int pos) { return errmsg->getLineOf(pos); }

    using label_t = std::shared_ptr<LabelNode>;
    using ConditionNode_t = std::shared_ptr<ConditionNode>;
    using PatchList = std::list<std::pair<ConditionNode_t, bool>>;
    
    void setInGlobal(bool isGlobal = true) { this->isGlobal = isGlobal; }
    bool isInGlobal() { return this->isGlobal; }

    void setCurFunction(std::shared_ptr<FunctionNode> function) {
        CurFunctionNode = function;
    }
    void clearCurFunction() { CurFunctionNode = nullptr; }

    int global_count = 1000;
    std::shared_ptr<sym::Var> 
    NewNamedVar(std::shared_ptr<frontend::Type> type, std::string name) {
        if(isInGlobal()) {
            return module->createGlobalVar(type,"G_" + name);
        }
        return CurFunctionNode->createVariable(type, "L_" + name);
    }
    std::shared_ptr<sym::Var>
    NewVar(std::shared_ptr<frontend::Type> type) {
        if(isInGlobal()) {
            return module->createGlobalVar(type, "g_" + std::to_string(global_count++));
        }
        return CurFunctionNode->generateTempVariable(type);
    }
    
    bool insertVarDecl(VarDecl *decl, std::shared_ptr<sym::Var> var) {
        if(VarMap.count(decl)) return false;
        VarMap[decl] = var;
        return true;
    }
    std::shared_ptr<sym::Var> lookupVarDecl(VarDecl *decl) {
        if(!VarMap.count(decl)) return nullptr;
        return VarMap[decl];
    }
    bool insertFuncDecl(FuncDecl *decl, std::shared_ptr<sym::Function> func) {
        if(FuncMap.count(decl)) return false;
        FuncMap[decl] = func;
        return true;
    }
    std::shared_ptr<sym::Function> lookupFuncDecl(FuncDecl *decl) {
        if(!FuncMap.count(decl)) return nullptr;
        return FuncMap[decl];
    }
    void insertBuiltinFuncDecl(std::string name, FuncDecl *decl) {
        BuiltinFuncMap[name] = decl;
    }
    std::shared_ptr<sym::Function> lookupBuildinFuncDecl(std::string name) {
        return FuncMap[BuiltinFuncMap[name]];
    }

    std::pair<label_t,label_t> whiles_top() {
        return whiles.top();
    }
    void whiles_pop() {
        whiles.pop();
    }
    void whiles_push(std::pair<label_t,label_t> pair) {
        whiles.push(pair);
    }
    
    void emitNode(std::shared_ptr<BindNode> node) {
        Assert(isGlobal, "only can emit BindNode in Global");
        module->addBinding(node);
    }
    void emitNode(std::shared_ptr<InsnNode> node) {
        CurFunctionNode->addNode(node);
    }
    void emitNode(InsnNodeList nodelist) {
        for(auto node: nodelist) {
            CurFunctionNode->addNode(node);
        }
    }
    void emitAsCond(std::shared_ptr<sym::Var> var) {
        std::shared_ptr<NeqConditionNode> node;
        if(var->getType() == TypeFactory::IntTy) {
            node = NeqConditionNode::create(OperandList{var,0});
        } else if (var->getType() == TypeFactory::FloatTy) {
            node = NeqConditionNode::create(OperandList{var,float(0.0)});
        }
        emitNode(node);
        appendFalseList(node);
        appendTrueList(node);
    }
    void emitAsExpr() {
    /*
        cond -> true: L_True, false: L_False
        L_True: ret = 1
        goto L_End
        L_False: ret = 0
        L_End
    */
        auto L_True = LabelNode::generateTemp();
        auto L_False = LabelNode::generateTemp();
        auto L_End = LabelNode::generateTemp();
        auto ret = NewVar(TypeFactory::IntTy);
        patchTrue(L_True);
        patchFalse(L_False);
        emitNode(L_True);
        emitNode(CopyAssignNode::create(ret,OperandList(1,1)));
        emitNode(JumpNode::create(L_End));
        emitNode(L_False);
        emitNode(CopyAssignNode::create(ret,OperandList(1,0)));
        emitNode(L_End);
        setResult(ret);
    }

    void setResult(std::shared_ptr<sym::Var> var) { result = var; }
    std::shared_ptr<sym::Var> getResult() { return result; }

    std::pair<PatchList,PatchList> getPatchListPair() {
        auto ret = std::make_pair(TrueList, FalseList);
        TrueList.clear(), FalseList.clear();
        return ret;
    }
    void exchangePatchList() { std::swap(TrueList, FalseList); }
    void appendTrueList(std::shared_ptr<ConditionNode> node) {
        TrueList.push_back(std::make_pair(node,true));
    }
    void appendTrueList(PatchList list) {
        TrueList.insert(TrueList.end(), list.begin(),list.end());
    }
    void appendFalseList(std::shared_ptr<ConditionNode> node) {
        FalseList.push_back(std::make_pair(node,false));
    }
    void appendFalseList(PatchList list) {
        FalseList.insert(FalseList.end(), list.begin(),list.end());
    }
    void patchList(PatchList &list, std::shared_ptr<LabelNode> label) {
        for(auto [node, pred]: list) {
            if(pred) {
                node->patchTrueLabel(label);
            } else {
                node->patchFalseLabel(label);
            }
        }
        list.clear();
    }
    void patchTrue(std::shared_ptr<LabelNode> label) {
        patchList(TrueList,label);
    }
    void patchFalse(std::shared_ptr<LabelNode> label) {
        patchList(FalseList,label);
    }


    
private:
    std::unique_ptr<Parser> parser;
    std::unique_ptr<TUDecl> TU;
    std::unique_ptr<ErrMsg> errmsg;
    std::shared_ptr<Module> module;
    
    // args for both semantic analysis and tranlate
    bool isGlobal;

    // args for semantic analysis
    VEnv venv;
    FEnv fenv;
    FuncDecl *InFunc;
    int while_depth = 0;

    // args for translate
    std::shared_ptr<FunctionNode> CurFunctionNode;
    std::map<VarDecl*,std::shared_ptr<sym::Var>> VarMap;
    std::map<FuncDecl*,std::shared_ptr<sym::Function>> FuncMap;
    std::map<std::string, FuncDecl*> BuiltinFuncMap;
    std::stack<std::pair<label_t,label_t>> whiles; // (Label_cont,Label_end)
    std::shared_ptr<sym::Var> result;
    PatchList TrueList, FalseList;
};



} // namespace frontend
} // namespace aaac
#endif