#ifndef AST_OPTIMIZER_H
#define AST_OPTIMIZER_H

#include "ast.h"
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

class ASTOptimizer {
private:
    // 作用域类，用于管理变量的可见性
    class Scope {
    public:
        struct VarInfo {
            bool isDefined;     
            bool isUsed;        
            bool isInitialized;
            ASTNode* defNode;   
            int defLine;        
            bool isGlobal;      
        };
        
        std::map<std::string, VarInfo> variables;
        std::shared_ptr<Scope> parent;
        
        Scope(std::shared_ptr<Scope> p = nullptr) : parent(p) {}
        
        // 在当前作用域中查找变量
        VarInfo* findVarInCurrentScope(const std::string& name) {
            auto it = variables.find(name);
            return it != variables.end() ? &it->second : nullptr;
        }
        
        // 在当前作用域及所有父作用域中查找变量
        VarInfo* findVar(const std::string& name) {
            // 先在当前作用域查找
            VarInfo* info = findVarInCurrentScope(name);
            if (info) return info;
            
            // 如果当前作用域没找到且有父作用域，则在父作用域中查找
            if (parent) {
                return parent->findVar(name);
            }
            
            return nullptr;
        }
    };
    
    // 使用节点地址作为作用域的唯一标识
    std::unordered_map<ASTNode*, std::shared_ptr<Scope>> scopeMap;
    std::shared_ptr<Scope> currentScope;
    
    // 作用域管理
    void enterScope(ASTNode* node) {
        auto it = scopeMap.find(node);
        if (it != scopeMap.end()) {
            // 如果作用域已存在，直接使用
            currentScope = it->second;
        } else {
            // 创建新作用域并保存
            auto newScope = std::make_shared<Scope>(currentScope);
            scopeMap[node] = newScope;
            currentScope = newScope;
        }
    }
    
    void exitScope() {
        if (currentScope) {
            currentScope = currentScope->parent;
        }
    }
    
    // 优化相关函数
    ASTNode* optimizeCompUnit(ASTNode* node);
    ASTNode* optimizeFuncDef(ASTNode* node);
    ASTNode* optimizeBlock(ASTNode* node);
    ASTNode* optimizeExpr(ASTNode* node);
    ASTNode* optimizeVarDecl(ASTNode* node);
    ASTNode* optimizeIf(ASTNode* node);
    ASTNode* optimizeWhile(ASTNode* node);
    
    bool isConstExpr(ASTNode* node);
    double evalConstExpr(ASTNode* node);
    bool canRemoveNode(ASTNode* node);
    ASTNode* createConstNode(int value, int line_no);

public:
    ASTOptimizer() {
        // 创建全局作用域
        currentScope = std::make_shared<Scope>();
    }
    // 分析变量使用情况（递归遍历AST）
    void analyzeVarUsage(ASTNode* node);
    // 根据分析结果进行优化
    ASTNode* optimize(ASTNode* node);
};

#endif