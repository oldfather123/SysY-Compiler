#include "ast_optimizer.h"
#include <cassert>
#include <iostream>

bool needNewScope = 1;

ASTNode* ASTOptimizer::optimize(ASTNode* root) {
    if (!root) return nullptr;
    
    // 先分析变量使用情况
    // analyzeVarUsage(root);
    
    // 根据节点类型选择优化策略
    switch (root->type) {
        case NODE_COMP_UNIT:
            return optimizeCompUnit(root);
        case NODE_FUNC_DEF:
            return optimizeFuncDef(root);
        case NODE_BLOCK:
            return optimizeBlock(root);
        case NODE_VAR_DECL:
        case NODE_ARRAY_DECL:
            return optimizeVarDecl(root);
        case NODE_IF:
            return optimizeIf(root);
        case NODE_WHILE:
            return optimizeWhile(root);
        case NODE_BIN_OP:
            return optimizeExpr(root);
        default:
            // 递归优化子节点
            ASTNode* p = root;
            while (p) {
                if (p->next) {
                    p->next = optimize(p->next);
                }
                p = p->next;
            }
            return root;
    }
}

void ASTOptimizer::analyzeVarUsage(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case NODE_COMP_UNIT:
            // 编译单元使用全局作用域
            scopeMap[node] = currentScope;
            // analyzeVarUsage(node->comp.children);
            for (ASTNode *child = node->comp.children; child; child = child->next)
                analyzeVarUsage(child);
            break;
            
        case NODE_FUNC_DEF: {
            // 为函数创建新作用域
            auto oldScope = currentScope;
            enterScope(node);
            
            // 处理参数
            ASTNode* param = node->func_def.params;
            while (param) {
                if (param->type == NODE_VAR_DECL) {
                    auto& info = currentScope->variables[param->decl.name];
                    info.isDefined = true;
                    info.isUsed = true;  // 参数默认为已使用
                    // info.isUsed = false;
                    info.defNode = param;
                    info.defLine = param->line_no;
                    info.isGlobal = false;
                }
                param = param->next;
            }
            
            // analyzeVarUsage(node->func_def.params);
            needNewScope = 0;
            analyzeVarUsage(node->func_def.body);
            needNewScope = 1;
            
            currentScope = oldScope;
            break;
        }
        
        case NODE_BLOCK: {
            auto oldScope = currentScope;
            bool newScopeCreated = 0;
            if (needNewScope) enterScope(node), newScopeCreated = 1;
            
            auto curr = node->block.items;
            while (curr) {
                // if (curr->type == NODE_FUNC_CALL) std::cout << "func call node\n"; 
                analyzeVarUsage(curr);
                curr = curr->next;
            }
            if (newScopeCreated) exitScope();
            currentScope = oldScope;
            break;
        }
        case NODE_IF:
            analyzeVarUsage(node->if_stmt.cond);
            if (node->if_stmt.then->type == NODE_BLOCK) needNewScope = 1;
            analyzeVarUsage(node->if_stmt.then);
            if (node->if_stmt.els) {
                if (node->if_stmt.els->type == NODE_BLOCK) needNewScope = 1;
                analyzeVarUsage(node->if_stmt.els);
            }
            break;
        case NODE_WHILE:
            analyzeVarUsage(node->while_stmt.cond);
            if (node->while_stmt.body->type == NODE_BLOCK) needNewScope = 1;
            analyzeVarUsage(node->while_stmt.body);
            break;
        case NODE_RETURN:
            analyzeVarUsage(node->return_stmt.expr);
            break;
        case NODE_EXPR_STMT:
            analyzeVarUsage(node->return_stmt.expr);
            break;
        case NODE_ASSIGN:
            analyzeVarUsage(node->assign.lval);
            analyzeVarUsage(node->assign.rval);
            break;
        case NODE_BIN_OP:
            analyzeVarUsage(node->bin_op.left);
            analyzeVarUsage(node->bin_op.right);
            break;
        case NODE_UNARY_OP:
            analyzeVarUsage(node->unary_op.expr);
            break;
        case NODE_FUNC_CALL: {
            // std::cout << node->func_call.name << '\n';
            // analyzeVarUsage(node->func_call.args);
            for (ASTNode *arg = node->func_call.args; arg; arg = arg->next) 
                analyzeVarUsage(arg);
            break;
        }
        case NODE_ARRAY_INDEX: {
            // analyzeVarUsage(node->array_index.expr);
            ASTNode *p = node;
            while (p) {
                analyzeVarUsage(p->array_index.expr);
                p = p->next;
            }
            break;
        }
        case NODE_INIT_LIST: {
            auto curr = node->init_list.first;
            while (curr) {
                analyzeVarUsage(curr);
               curr = curr->next;
            }
            break;
        }
        case NODE_CONST_INIT_LIST: {
            auto curr = node->const_init_list.first;
            while (curr) {
                analyzeVarUsage(curr);
                curr = curr->next;
            }
            break;
        }
        case NODE_INIT_VAL: 
            analyzeVarUsage(node->const_init.expr);
            break;
        case NODE_CONST_INIT:
            analyzeVarUsage(node->init_val.expr);
            break;
        case NODE_VAR_DECL:
        case NODE_ARRAY_DECL:
        case NODE_CONST_DECL:
        case NODE_CONST_ARRAY_DECL: {
            auto name = node->decl.name;
            auto& info = currentScope->variables[name];
            info.isDefined = true;
            info.defNode = node;
            info.defLine = node->line_no;
            info.isInitialized = (node->decl.init != nullptr);
            info.isGlobal = (scopeMap.find(node) == scopeMap.end());
            if (node->decl.dims) analyzeVarUsage(node->decl.dims);
            if (node->decl.init) {
                analyzeVarUsage(node->decl.init);
            }
            break;
        }
        case NODE_LVAL: {
            auto name = node->lval.name;
            auto scope = currentScope;
            // if (!strcmp(node->lval.name, "j")) std::cout << node->lval.name << '\n';
            while (scope) {
                auto it = scope->variables.find(name);
                if (it != scope->variables.end()) {
                    it->second.isUsed = true;
                    // if (!strcmp(node->lval.name, "j")) std::cout << "ok\n";
                    break;
                }
                scope = scope->parent;
            }
            analyzeVarUsage(node->lval.indices);
            break;
        }
        case NODE_ARRAY_DIMS: {
            while (node) {
                analyzeVarUsage(node->array_dims.size);
                node = node->array_dims.next_dim;
            }
        }
    }
    
    // 递归处理后继节点
    // if (node->next) {
    //     analyzeVarUsage(node->next);
    // }
}

bool containsGetArray(ASTNode *node) {
    if (!node) return false;
    
    switch (node->type) {
        case NODE_FUNC_CALL:
            if (!strcmp(node->func_call.name, "getarray") || 
                !strcmp(node->func_call.name, "getfarray")) {
                return true;
            }
            return containsGetArray(node->func_call.args);
            
        case NODE_BIN_OP:
            return containsGetArray(node->bin_op.left) || 
                   containsGetArray(node->bin_op.right);
            
        case NODE_UNARY_OP:
            return containsGetArray(node->unary_op.expr);
            
        case NODE_INIT_VAL:
            return containsGetArray(node->init_val.expr);
            
        case NODE_INIT_LIST:
            return containsGetArray(node->init_list.first);
            
        case NODE_CONST_INIT:
            return containsGetArray(node->const_init.expr);
            
        case NODE_CONST_INIT_LIST:
            return containsGetArray(node->const_init_list.first);
            
        default:
            if (node->next) {
                return containsGetArray(node->next);
            }
            return false;
    }
}

ASTNode* ASTOptimizer::optimizeVarDecl(ASTNode* node) {
    if (!node) return nullptr;
    
    auto name = node->decl.name;
    auto info = currentScope->findVarInCurrentScope(name);
    
    // 检查变量是否未被使用
    if (info && !info->isUsed) {
        // 如果变量有初始化且初始化中包含getarray调用，则不能删除
        if (node->decl.init && containsGetArray(node->decl.init)) {
            return node;
        }
        return nullptr;  // 否则可以删除
    }
    
    // 优化初始化表达式
    if (node->decl.init) {
        node->decl.init = optimizeExpr(node->decl.init);
    }
    
    return node;
}

ASTNode* ASTOptimizer::optimizeExpr(ASTNode* node) {
    if (!node) return nullptr;
    
    if (node->type == NODE_BIN_OP) {
        // 递归优化左右子表达式
        node->bin_op.left = optimizeExpr(node->bin_op.left);
        node->bin_op.right = optimizeExpr(node->bin_op.right);
        
        // 如果是常量表达式，进行折叠
        if (isConstExpr(node)) {
            int value = evalConstExpr(node);
            return createConstNode(value, node->line_no);
        }
    }
    
    return node;
}

bool ASTOptimizer::isConstExpr(ASTNode* node) {
    if (!node) return false;
    
    if (node->type == NODE_CONST_EXPR) {
        return true;
    }
    
    if (node->type == NODE_BIN_OP) {
        return isConstExpr(node->bin_op.left) && 
               isConstExpr(node->bin_op.right);
    }
    
    return false;
}

double ASTOptimizer::evalConstExpr(ASTNode* node) {
    if (!node) return 0.0;
    
    if (node->type == NODE_CONST_EXPR)
        return (node->data_type == TYPE_INT) ? (double)node->const_expr.int_val : node->const_expr.float_val;
    
    if (node->type == NODE_BIN_OP) {
        double left = evalConstExpr(node->bin_op.left);
        double right = evalConstExpr(node->bin_op.right);
        
        switch (node->bin_op.op) {
            case OP_ADD: return left + right;
            case OP_SUB: return left - right;
            case OP_MUL: return left * right;
            case OP_DIV: return right ? left / right : 0.0;
            case OP_MOD: return right ? (double)((long)left % (long)right) : 0.0;
            case OP_AND: return (double)(left && right);
            case OP_BAND: return (double)((long)left & (long)right);
            case OP_BOR: return (double)((long)left | (long)right);
            case OP_BXOR: return (double)((long)left ^ (long)right);
            case OP_EQ: return (double)(left == right);
            case OP_GE: return (double)(left >= right);
            case OP_GT: return (double)(left > right);
            case OP_LE: return (double)(left <= right);
            case OP_LT: return (double)(left < right);
            case OP_NE: return (double)(left != right);
            case OP_OR: return (double)(left || right);
            case OP_SHL: return (double)((long)left << (long)right);
            case OP_SHR: return (double)((long)left >> (long)right);
            default: return 0;
        }
    }
    if (node->type == NODE_INIT_VAL)
        return evalConstExpr(node->init_val.expr);
    if (node->type == NODE_CONST_INIT)
        return evalConstExpr(node->const_init.expr);
    if (node->type == NODE_UNARY_OP) {
        double val = evalConstExpr(node->unary_op.expr);
        switch (node->unary_op.op) {
        case OP_NEG:
            return -val;
        case OP_POS:
            return val;
        case OP_BNOT:
            return (double)(~(long)val);
        case OP_NOT:
            return !val;
        default:
            return 0.0;
        }
    }
    // 处理在语义分析中插入的类型转换节点
    if (node->type == NODE_TYPE_CAST) {
        double val = evalConstExpr(node->unary_op.expr);
        if (node->data_type == TYPE_INT)
            return (double)((int)val); // 模拟向int的转换
        if (node->data_type == TYPE_FLOAT)
            return val; // 已经是double，无需操作
    }
    
    return 0;
}

ASTNode* ASTOptimizer::createConstNode(int value, int line_no) {
    return new_const_expr(value, line_no);
}

ASTNode* ASTOptimizer::optimizeBlock(ASTNode* node) {
    if (!node || node->type != NODE_BLOCK) return node;
    
    ASTNode* curr = node->block.items;
    ASTNode* head = nullptr;
    ASTNode* tail = nullptr;
    bool newScopeCreated = 0;
    if (needNewScope) enterScope(node), newScopeCreated = 1;

    // 优化块中的每个语句
    while (curr) {
        ASTNode* next = curr->next;
        curr->next = nullptr;
        
        ASTNode* optimized = optimize(curr);
        if (optimized) {
            if (!head) {
                head = optimized;
                tail = optimized;
            } else {
                tail->next = optimized;
                tail = optimized;
            }
        }
        curr = next;
    }
    
    node->block.items = head;
    if (newScopeCreated) exitScope();
    return node;
}

ASTNode* ASTOptimizer::optimizeCompUnit(ASTNode* node) {
    if (!node || node->type != NODE_COMP_UNIT) return node;
    
    // 优化编译单元中的所有声明和定义
    ASTNode* curr = node->comp.children;
    ASTNode* head = nullptr;
    ASTNode* tail = nullptr;
    
    while (curr) {
        ASTNode* next = curr->next;
        curr->next = nullptr;
        
        ASTNode* optimized = optimize(curr);
        if (optimized) {
            if (!head) {
                head = optimized;
                tail = optimized;
            } else {
                tail->next = optimized;
                tail = optimized;
            }
        }
        curr = next;
    }
    
    node->comp.children = head;
    return node;
}

ASTNode* ASTOptimizer::optimizeIf(ASTNode* node) {
    if (!node || node->type != NODE_IF) return node;
    
    // 优化条件表达式
    node->if_stmt.cond = optimizeExpr(node->if_stmt.cond);
    
    // 如果条件是常量表达式，可以进行分支优化
    if (isConstExpr(node->if_stmt.cond)) {
        int condValue = evalConstExpr(node->if_stmt.cond);
        if (condValue) {
            // 条件永真，只保留then分支
            if (node->if_stmt.then->type == NODE_BLOCK) needNewScope = 1;
            return optimize(node->if_stmt.then);
        } else if (node->if_stmt.els) {
            // 条件永假，且有else分支，只保留else分支
            if (node->if_stmt.els->type == NODE_BLOCK) needNewScope = 1;
            return optimize(node->if_stmt.els);
        } else {
            // 条件永假，且无else分支，删除整个if语句
            return nullptr;
        }
    }

    // 优化then分支
    if (node->if_stmt.then->type == NODE_BLOCK) needNewScope = 1;
    node->if_stmt.then = optimize(node->if_stmt.then);
    
    // 优化else分支（如果存在）
    if (node->if_stmt.els) {
        if (node->if_stmt.els->type == NODE_BLOCK) needNewScope = 1;
        node->if_stmt.els = optimize(node->if_stmt.els);
    }
    
    return node;
}

ASTNode* ASTOptimizer::optimizeWhile(ASTNode* node) {
    if (!node || node->type != NODE_WHILE) return node;
    
    // 优化循环条件
    node->while_stmt.cond = optimizeExpr(node->while_stmt.cond);
    
    // 如果条件是常量表达式
    if (isConstExpr(node->while_stmt.cond)) {
        int condValue = evalConstExpr(node->while_stmt.cond);
        if (!condValue) {
            // 条件永假，删除整个循环
            return nullptr;
        }
        // 注意：条件永真的循环不能删除，因为可能包含break语句
    }
    
    // 优化循环体
    if (node->while_stmt.body->type == NODE_BLOCK) needNewScope = 1;
    node->while_stmt.body = optimize(node->while_stmt.body);
    
    return node;
}

ASTNode* ASTOptimizer::optimizeFuncDef(ASTNode* node) {
    if (!node || node->type != NODE_FUNC_DEF) return node;
    
    // 进入新的函数作用域
    enterScope(node);
    
    // 优化函数参数
    if (node->func_def.params) {
        ASTNode* curr = node->func_def.params;
        ASTNode* head = nullptr;
        ASTNode* tail = nullptr;
        
        while (curr) {
            ASTNode* next = curr->next;
            curr->next = nullptr;
            
            ASTNode *optimized = optimize(curr);
            if (optimized) {
                if (!head) {
                    head = curr;
                    tail = curr;
                } else {
                    tail->next = curr;
                    tail = curr;
                }
            }
            curr = next;
        }
        node->func_def.params = head;
    }
    
    // 优化函数体
    if (node->func_def.body) {
        // 函数体是一个块，让 optimizeBlock 来处理作用域
        needNewScope = 0;
        node->func_def.body = optimize(node->func_def.body);
        needNewScope = 1;
    }
    
    // 退出函数作用域
    exitScope();
    
    return node;
}