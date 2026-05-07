#include "ast.hpp"
#include "Value.hpp"
#include "logging.hpp"
#include "syntax_tree.h"
#include "exitcode.hpp"

#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <stack>

#define _AST_NODE_ERROR_                                                       \
    std::cerr << "Abort due to node cast error."                               \
                 "Contact with TAs to solve your problem."                     \
              << std::endl;                                                    \
    ASSERT(false);
#define _STR_EQ(a, b) (strcmp((a), (b)) == 0)

void AST::run_visitor(ASTVisitor &visitor) { root->accept(visitor); }

AST::AST(syntax_tree *s) {
    if (s == nullptr) {
        std::cerr << "empty input tree!" << std::endl;
        ASSERT(false);
    }
    auto node = transform_node_iter(s->root);
    del_syntax_tree(s);
    root = std::static_pointer_cast<ASTProgram>(node);
}

std::shared_ptr<ASTNode> AST::transform_node_iter(syntax_tree_node *n) {
    // std::cout<<"transform_node_iter: "<<n->name<<std::endl;
    // getchar();
    if (_STR_EQ(n->name, "Program")) {
        // auto node = new ASTProgram();
        auto node = std::make_shared<ASTProgram>();
        // flatten declaration list
        std::stack<syntax_tree_node *> s;
        auto list_ptr = n->children[0];
        while (list_ptr->children_num == 2) {
            s.push(list_ptr->children[1]);
            list_ptr = list_ptr->children[0];
        }
        s.push(list_ptr->children[0]);

        while (!s.empty()) {
            auto child_node = transform_node_iter(s.top());
            if(child_node->is_const_decl()){
                node->defs_and_decls.push_back(
                    std::static_pointer_cast<ASTConstDecl>(child_node));
            }
            else if(child_node->is_var_decl()){
                node->defs_and_decls.push_back(
                    std::static_pointer_cast<ASTVarDecl>(child_node));
            }
            else if(child_node->is_func_def()){
                node->defs_and_decls.push_back(
                    std::static_pointer_cast<ASTFuncDef>(child_node));
            }
            else{
                std::cerr << "Unknown type of declaration" << std::endl;
                ASSERT(false);
            }
            s.pop();
        }
        return node;
    }
    else if (_STR_EQ(n->name, "Decl")) {
        return transform_node_iter(n->children[0]);
    }
    else if(_STR_EQ(n->name, "ConstDecl")) {
        auto node = std::make_shared<ASTConstDecl>();
        SysyType type;
        if (_STR_EQ(n->children[1]->children[0]->name, "int")) {
            type = TYPE_INT;
        }
        else {
           type = TYPE_FLOAT;
        }
        
        auto list_ptr = n->children[2];
        std::stack<syntax_tree_node *> s;
        while(list_ptr->children_num == 3) {
            s.push(list_ptr->children[2]);
            list_ptr = list_ptr->children[0];
        }
        s.push(list_ptr->children[0]);
        while(!s.empty()) {
            auto child_node =
                std::static_pointer_cast<ASTConstDef>(transform_node_iter(s.top()));
            child_node->type = type;
            node->const_defs.push_back(child_node);
            s.pop();
        }
        return node;
    }
    else if (_STR_EQ(n->name, "ConstDef")) {
        auto node = std::make_shared<ASTConstDef>();
        node->id = n->children[0]->name;
        if(n->children[1]->children_num == 0) {
            node->is_array = false;
        }
        else {
            node->is_array = true;
            auto list_ptr = n->children[1];
            std::stack<syntax_tree_node *> s;
            while(list_ptr->children_num == 4) {
                s.push(list_ptr->children[2]);
                list_ptr = list_ptr->children[0];
            }
            while(!s.empty()) {
                auto child_node =
                    std::static_pointer_cast<ASTConstExp>(transform_node_iter(s.top()));
                node->array_size.push_back(child_node);
                s.pop();
            }
        }
        node->init_val =
            std::static_pointer_cast<ASTConstInitVal>(transform_node_iter(n->children[3]));
        return node;
    }
    else if(_STR_EQ(n->name, "ConstInitVal")) {
        auto node = std::make_shared<ASTConstInitVal>();
        if(n->children_num == 1) {
            node->is_single_exp = true;
            node->const_exp =
                std::static_pointer_cast<ASTConstExp>(transform_node_iter(n->children[0]));
        }
        else {
            node->is_single_exp = false;
            auto list_ptr = n->children[1];
            std::stack<syntax_tree_node *> s;
            while(list_ptr->children_num == 3) {
                s.push(list_ptr->children[2]);
                list_ptr = list_ptr->children[0];
            }
            if(list_ptr->children_num == 1){
                s.push(list_ptr->children[0]);
            }
            while(!s.empty()) {
                auto child_node =
                    std::static_pointer_cast<ASTConstInitVal>(transform_node_iter(s.top()));
                node->init_vals.push_back(child_node);
                s.pop();
            }
        }
        return node;
    }
    else if(_STR_EQ(n->name, "VarDecl")) {
        auto node = std::make_shared<ASTVarDecl>();
        SysyType type;
        if (_STR_EQ(n->children[0]->children[0]->name, "int")) {
            type = TYPE_INT;
        }
        else {
           type = TYPE_FLOAT;
        }
        auto list_ptr = n->children[1];
        std::stack<syntax_tree_node *> s;
        while(list_ptr->children_num == 3) {
            s.push(list_ptr->children[2]);
            list_ptr = list_ptr->children[0];
        }
        s.push(list_ptr->children[0]);
        while(!s.empty()){
            auto child_node =
                std::static_pointer_cast<ASTVarDef>(transform_node_iter(s.top()));
            child_node->type = type;
            node->var_defs.push_back(child_node);
            s.pop();
        }
        return node;
    }
    else if (_STR_EQ(n->name, "VarDef")) {
        auto node = std::make_shared<ASTVarDef>();
        node->id = n->children[0]->name;
        if(n->children[1]->children_num == 0) {
            node->is_array = false;
        }
        else {
            node->is_array = true;
            auto list_ptr = n->children[1];
            std::stack<syntax_tree_node *> s;
            while(list_ptr->children_num == 4) {
                s.push(list_ptr->children[2]);
                list_ptr = list_ptr->children[0];
            }
            while(!s.empty()) {
                auto child_node =
                    std::static_pointer_cast<ASTConstExp>(transform_node_iter(s.top()));
                node->array_size.push_back(child_node);
                s.pop();
            }
        }
        if(n->children_num == 4) {
            node->init_val =
                std::static_pointer_cast<ASTInitVal>(transform_node_iter(n->children[3]));
        }
        return node;
    }
    else if(_STR_EQ(n->name, "InitVal")) {
        auto node = std::make_shared<ASTInitVal>();
        if(n->children_num == 1) {
            node->is_single_exp = true;
            node->exp = std::static_pointer_cast<ASTExp>(transform_node_iter(n->children[0]));
        }
        else {
            node->is_single_exp = false;
            auto list_ptr = n->children[1];
            std::stack<syntax_tree_node *> s;
            while(list_ptr->children_num == 3) {
                s.push(list_ptr->children[2]);
                list_ptr = list_ptr->children[0];
            }
            if(list_ptr->children_num == 1){
                s.push(list_ptr->children[0]);
            }
            while(!s.empty()) {
                auto child_node =
                    std::static_pointer_cast<ASTInitVal>(transform_node_iter(s.top()));
                node->init_vals.push_back(child_node);
                s.pop();
            }
        }
        return node;
    }
    else if(_STR_EQ(n->name, "FuncDef")) {
        auto node = std::make_shared<ASTFuncDef>();
        if(_STR_EQ(n->children[0]->children[0]->name, "void")) {
            node->type = TYPE_VOID;
        }
        else if(_STR_EQ(n->children[0]->children[0]->name, "int")) {
            node->type = TYPE_INT;
        }
        else {
            node->type = TYPE_FLOAT;
        }
        node->id = n->children[1]->name;
        if (n->children_num == 6) {
            auto list_ptr = n->children[3];
            std::stack<syntax_tree_node *> s;
            while (list_ptr->children_num == 3) {
                s.push(list_ptr->children[2]);
                list_ptr = list_ptr->children[0];
            }
            s.push(list_ptr->children[0]);
            while (!s.empty()) {
                auto child_node =
                    std::static_pointer_cast<ASTFParam>(transform_node_iter(s.top()));
                node->params.push_back(child_node);
                s.pop();
            }
            node->block =
                std::static_pointer_cast<ASTBlock>(transform_node_iter(n->children[5]));
        }
        else {
            node->block =
                std::static_pointer_cast<ASTBlock>(transform_node_iter(n->children[4]));
        }
        return node;
    }
    else if (_STR_EQ(n->name, "FuncFParam")) {
        auto node = std::make_shared<ASTFParam>();
        if (_STR_EQ(n->children[0]->children[0]->name, "int")) {
            node->type = TYPE_INT;
        }
        else {
            node->type = TYPE_FLOAT;
        }
        node->id = n->children[1]->name;
        if (n->children_num > 2){
            node->is_array = true;
            auto list_ptr = n->children[4];
            std::stack<syntax_tree_node *> s;
            while (list_ptr->children_num > 0) {
                s.push(list_ptr->children[2]);
                list_ptr = list_ptr->children[0];
            }
            while (!s.empty()) {
                auto child_node =
                    std::static_pointer_cast<ASTExp>(transform_node_iter(s.top()));
                node->array_size.push_back(child_node);
                s.pop();
            }
        }

        return node;
    }
    else if (_STR_EQ(n->name, "Block")) {
        auto node = std::make_shared<ASTBlock>();
        auto list_ptr = n->children[1];
        std::stack<syntax_tree_node *> s;
        while (list_ptr->children_num > 0) {
            s.push(list_ptr->children[1]);
            list_ptr = list_ptr->children[0];
        }
        while(!s.empty()) {
            auto child_node = transform_node_iter(s.top());
            if (child_node->is_var_decl()) {
                node->decls_and_stmts.push_back(
                    std::static_pointer_cast<ASTVarDecl>(child_node));
            }
            else if (child_node->is_const_decl()) {
                node->decls_and_stmts.push_back(
                    std::static_pointer_cast<ASTConstDecl>(child_node));
            }
            else if (child_node->is_stmt()) {
                node->decls_and_stmts.push_back(
                    std::static_pointer_cast<ASTStmt>(child_node));
            }
            s.pop();
        }
        return node;
    }
    else if (_STR_EQ(n->name, "BlockItem")) {
        return transform_node_iter(n->children[0]);
    }
    else if (_STR_EQ(n->name, "Stmt")) {
        if(n->children_num >= 2 && _STR_EQ(n->children[1]->name, "=")) {
            auto node = std::make_shared<ASTAssignStmt>();
            node->l_val = std::static_pointer_cast<ASTLVal>(transform_node_iter(n->children[0]));
            node->exp = std::static_pointer_cast<ASTExp>(transform_node_iter(n->children[2]));
            return node;
        }
        else if(_STR_EQ(n->children[0]->name, "Exp")) {
            auto node = std::make_shared<ASTExpStmt>();
            node->exp = std::static_pointer_cast<ASTExp>(transform_node_iter(n->children[0]));
            return node;
        }
        else if(_STR_EQ(n->children[0]->name, "Block")) {
            auto node = std::make_shared<ASTBlockStmt>();
            node->block = std::static_pointer_cast<ASTBlock>(transform_node_iter(n->children[0]));
            return node;
        }
        else if(_STR_EQ(n->children[0]->name, "if")) {
            auto node = std::make_shared<ASTSelectionStmt>();
            node->cond = std::static_pointer_cast<ASTCond>(transform_node_iter(n->children[2]));
            node->if_stmt = std::static_pointer_cast<ASTStmt>(transform_node_iter(n->children[4]));
            if(n->children_num == 7) {
                node->has_else = true;
                node->else_stmt = std::static_pointer_cast<ASTStmt>(transform_node_iter(n->children[6]));
            }
            else {
                node->has_else = false;
            }
            return node;
        }
        else if(_STR_EQ(n->children[0]->name, "while")) {
            auto node = std::make_shared<ASTIterationStmt>();
            node->cond = std::static_pointer_cast<ASTCond>(transform_node_iter(n->children[2]));
            node->stmt = std::static_pointer_cast<ASTStmt>(transform_node_iter(n->children[4]));
            return node;
        }
        else if(_STR_EQ(n->children[0]->name, "return")) {
            auto node = std::make_shared<ASTReturnStmt>();
            if(n->children_num == 3) {
                node->is_empty = false;
                node->exp = std::static_pointer_cast<ASTExp>(transform_node_iter(n->children[1]));
            }
            else {
                node->is_empty = true;
            }
            return node;
        }
        else if(_STR_EQ(n->children[0]->name, "break")) {
            auto node = std::make_shared<ASTBreakStmt>();
            return node;
        }
        else if(_STR_EQ(n->children[0]->name, "continue")) {
            auto node = std::make_shared<ASTContinueStmt>();
            return node;
        }
        else if(_STR_EQ(n->children[0]->name, ";")) {
            auto node = std::make_shared<ASTExpStmt>();
            node->exp = nullptr;
            node->is_empty = true;
            return node;
        }
        else {
            std::cerr << "Unknown type of statement" << std::endl;
            ASSERT(false);
        }
    }
    else if (_STR_EQ(n->name, "Exp")) {
        return transform_node_iter(n->children[0]);
    }
    else if (_STR_EQ(n->name, "Cond")) {
        auto node = std::make_shared<ASTCond>();
        node->exp = std::static_pointer_cast<ASTExp>(transform_node_iter(n->children[0]));
        return node;
    }
    else if (_STR_EQ(n->name, "LVal")) {
        auto node = std::make_shared<ASTLVal>();
        node->id = n->children[0]->name;
        auto list_ptr = n->children[1];
        std::stack<syntax_tree_node *> s;
        while(list_ptr->children_num == 4) {
            s.push(list_ptr->children[2]);
            list_ptr = list_ptr->children[0];
        }
        while(!s.empty()) {
            auto child_node = transform_node_iter(s.top());
            node->array_exp.push_back(std::static_pointer_cast<ASTExp>(child_node));
            s.pop();
        }
        return node;
    }
    else if (_STR_EQ(n->name, "Number")) {
        auto node = std::make_shared<ASTNumber>();
        if(_STR_EQ(n->children[0]->name, "IntConst")) {
            node->type = TYPE_INT;
            node->value = std::stoi(n->children[0]->children[0]->name, nullptr, 0);
        }
        else {
            node->type = TYPE_FLOAT;
            node->value = std::stof(n->children[0]->children[0]->name, nullptr);
        }
        return node;
    }
    else if (_STR_EQ(n->name, "UnaryExp")) {
        auto node = std::make_shared<ASTUnaryExp>();
        if(n->children_num == 1) {
            auto syntax_child = n->children[0]->children[0];
            if(_STR_EQ(syntax_child->name, "Number")) {
                node->number = std::static_pointer_cast<ASTNumber>(transform_node_iter(syntax_child));
            }
            else if(_STR_EQ(syntax_child->name, "LVal")) {
                node->l_val = std::static_pointer_cast<ASTLVal>(transform_node_iter(syntax_child));
            }
            else if(_STR_EQ(n->children[0]->children[1]->name, "Exp")) {
                return transform_node_iter(n->children[0]->children[1]);
            }
        }
        else if(n->children_num == 2) {
            node->has_unary_op = true;
            if(_STR_EQ(n->children[0]->children[0]->name, "+")) {
                node->op = OP_POS;
            }
            else if(_STR_EQ(n->children[0]->children[0]->name, "-")) {
                node->op = OP_NEG;
            }
            else if(_STR_EQ(n->children[0]->children[0]->name, "!")) {
                node->op = OP_NOT;
            }
            node->exp = std::static_pointer_cast<ASTUnaryExp>(transform_node_iter(n->children[1]));
        }
        else if(n->children_num == 4 || n->children_num == 3) {
            node->func_call_id = n->children[0]->name;
            if(n->children_num == 4) {
                auto list_ptr = n->children[2];
                std::stack<syntax_tree_node *> s;
                while(list_ptr->children_num == 3) {
                    s.push(list_ptr->children[2]);
                    list_ptr = list_ptr->children[0];
                }
                s.push(list_ptr->children[0]);
                while(!s.empty()) {
                    auto child_node = transform_node_iter(s.top());
                    node->func_call_args.push_back(std::static_pointer_cast<ASTExp>(child_node));
                    s.pop();
                }
            }
        }
        return node;
    }
    else if (_STR_EQ(n->name, "MulExp")) {
        if(n->children_num == 1) {
            return transform_node_iter(n->children[0]);
        }
        else {
            auto node = std::make_shared<ASTBinaryExp>();
            node->lhs = std::static_pointer_cast<ASTExp>(transform_node_iter(n->children[0]));
            node->rhs = std::static_pointer_cast<ASTExp>(transform_node_iter(n->children[2]));
            if(_STR_EQ(n->children[1]->name, "*")) {
                node->op = OP_MUL;
            }
            else if (_STR_EQ(n->children[1]->name, "/")){
                node->op = OP_DIV;
            }
            else if (_STR_EQ(n->children[1]->name, "%")){
                node->op = OP_MOD;
            }
            return node;
        }
    }
    else if (_STR_EQ(n->name, "AddExp")) {
        if(n->children_num == 1) {
            return transform_node_iter(n->children[0]);
        }
        else {
            auto node = std::make_shared<ASTBinaryExp>();
            node->lhs = std::static_pointer_cast<ASTExp>(transform_node_iter(n->children[0]));
            node->rhs = std::static_pointer_cast<ASTExp>(transform_node_iter(n->children[2]));
            if(_STR_EQ(n->children[1]->name, "+")) {
                node->op = OP_PLUS;
            }
            else if(_STR_EQ(n->children[1]->name, "-")) {
                node->op = OP_MINUS;
            }
            return node;
        }
    }
    else if (_STR_EQ(n->name, "RelExp")) {
        if(n->children_num == 1) {
            return transform_node_iter(n->children[0]);
        }
        else {
            auto node = std::make_shared<ASTBinaryExp>();
            node->lhs = std::static_pointer_cast<ASTExp>(transform_node_iter(n->children[0]));
            node->rhs = std::static_pointer_cast<ASTExp>(transform_node_iter(n->children[2]));
            if(_STR_EQ(n->children[1]->name, "<")) {
                node->op = OP_LT;
            }
            else if(_STR_EQ(n->children[1]->name, "<=")) {
                node->op = OP_LE;
            }
            else if(_STR_EQ(n->children[1]->name, ">")) {
                node->op = OP_GT;
            }
            else if(_STR_EQ(n->children[1]->name, ">=")){
                node->op = OP_GE;
            }
            return node;
        }
    }
    else if (_STR_EQ(n->name, "EqExp")) {
        if(n->children_num == 1) {
            return transform_node_iter(n->children[0]);
        }
        else {
            auto node = std::make_shared<ASTBinaryExp>();
            node->lhs = std::static_pointer_cast<ASTExp>(transform_node_iter(n->children[0]));
            node->rhs = std::static_pointer_cast<ASTExp>(transform_node_iter(n->children[2]));
            if(_STR_EQ(n->children[1]->name, "==")) {
                node->op = OP_EQ;
            }
            else if(_STR_EQ(n->children[1]->name, "!=")) {
                node->op = OP_NEQ;
            }
            return node;
        }
    }
    else if (_STR_EQ(n->name, "LAndExp")) {
        if(n->children_num == 1) {
            return transform_node_iter(n->children[0]);
        }
        else {
            auto node = std::make_shared<ASTBinaryExp>();
            node->lhs = std::static_pointer_cast<ASTExp>(transform_node_iter(n->children[0]));
            node->rhs = std::static_pointer_cast<ASTExp>(transform_node_iter(n->children[2]));
            node->op = OP_AND;
            return node;
        }
    }
    else if (_STR_EQ(n->name, "LOrExp")) {
        if(n->children_num == 1) {
            return transform_node_iter(n->children[0]);
        }
        else {
            auto node = std::make_shared<ASTBinaryExp>();
            node->lhs = std::static_pointer_cast<ASTExp>(transform_node_iter(n->children[0]));
            node->rhs = std::static_pointer_cast<ASTExp>(transform_node_iter(n->children[2]));
            node->op = OP_OR;
            return node;
        }
    }
    else if (_STR_EQ(n->name, "ConstExp")) {
        auto node = std::make_shared<ASTConstExp>();
        node->exp = std::static_pointer_cast<ASTExp>(transform_node_iter(n->children[0]));
        return node;
    }
    else {
        std::cerr << "Unknown node type: " << n->name << std::endl;
        ASSERT(false);
    }
    std::cerr << "Unknown node type: " << n->name << std::endl;
    return nullptr;
}

Value* ASTProgram::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTConstDecl::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTVarDecl::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTFuncDef::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTConstDef::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTVarDef::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTConstInitVal::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTInitVal::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTFParam::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTBlock::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTAssignStmt::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTExpStmt::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTBlockStmt::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTBreakStmt::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTContinueStmt::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTSelectionStmt::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTIterationStmt::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTReturnStmt::accept(ASTVisitor &visitor) {return visitor.visit(*this); }
Value* ASTUnaryExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTBinaryExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTNumber::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTLVal::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTConstExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTCond::accept(ASTVisitor &visitor) { return visitor.visit(*this); }


#define _DEBUG_PRINT_N_(N)                                                     \
    { std::cout << std::string(N, '-'); }

Value* ASTPrinter::visit(ASTProgram &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "program" << std::endl;
    add_depth();
    for (auto def_or_decl : node.defs_and_decls) {
        std::visit([this](auto&& arg){
            arg->accept(*this);
        }, def_or_decl);
    }
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTConstDecl &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "const-decl" << std::endl;
    add_depth();
    for (auto def : node.const_defs) {
        def->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTConstDef &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "const-def: " << node.id;
    if (node.is_array) {
        add_depth();
        for (auto exp : node.array_size) {
            std::cout << "[";
            exp->accept(*this);
            std::cout << "]" << std::endl;
        }
        remove_depth();
    }
    std::cout << std::endl;
    add_depth();
    node.init_val->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTConstInitVal &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "const-init-val";
    if (node.is_single_exp) {
        std::cout << std::endl;
        add_depth();
        node.const_exp->accept(*this);
        remove_depth();
    } else {
        std::cout << "{" << std::endl;
        add_depth();
        for (auto val : node.init_vals) {
            val->accept(*this);
        }
        remove_depth();
        std::cout << "}" << std::endl;
    }
    return nullptr;
}

Value* ASTPrinter::visit(ASTVarDecl &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "var-decl" << std::endl;
    add_depth();
    for (auto def : node.var_defs) {
        def->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTVarDef &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "var-def: " << node.id;
    std::cout << std::endl;
    if (node.is_array) {
        add_depth();
        for (auto exp : node.array_size) {
            std::cout << "[";
            exp->accept(*this);
            std::cout << "]" << std::endl;
        }
        remove_depth();
    }
    if(node.init_val != nullptr){
        add_depth();
        node.init_val->accept(*this);
        remove_depth();
    }
    return nullptr;
}

Value* ASTPrinter::visit(ASTInitVal &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "init-val" << std::endl;
    if(node.is_single_exp){
        add_depth();
        node.exp->accept(*this);
        remove_depth();
    }
    else{
        _DEBUG_PRINT_N_(depth);
        std::cout << "{" << std::endl;
        add_depth();
        for(auto val: node.init_vals){
            val->accept(*this);
        }
        remove_depth();
        _DEBUG_PRINT_N_(depth);
        std::cout << "}" << std::endl;
    }
    return nullptr;
}

Value* ASTPrinter::visit(ASTFuncDef &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "func-def: " ;
    if(node.type == TYPE_INT){
        std::cout << "int ";
    }
    else if(node.type == TYPE_FLOAT){
        std::cout << "float ";
    }
    else if(node.type == TYPE_VOID){
        std::cout << "void ";
    }
    std::cout << node.id << std::endl;
    add_depth();
    for (auto param : node.params) {
        param->accept(*this);
    }
    node.block->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTFParam &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "fparam: " << node.id;
    if (node.is_array) {
        std::cout << "[]" << std::endl;
        add_depth();
        for (auto exp : node.array_size) {
            std::cout << "[";
            exp->accept(*this);
            std::cout << "]" << std::endl;
        }
        remove_depth();
    } else {
        std::cout << std::endl;
    }
    return nullptr;
}

Value* ASTPrinter::visit(ASTBlock &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "block" << std::endl;
    add_depth();
    for (auto decl_or_stmt : node.decls_and_stmts) {
        std::visit([this](auto&& arg){
            arg->accept(*this);
        }, decl_or_stmt);
    }
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTAssignStmt &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "assign-stmt" << std::endl;
    add_depth();
    node.l_val->accept(*this);
    node.exp->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTExpStmt &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "exp-stmt" << std::endl;
    add_depth();
    node.exp->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTBlockStmt &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "block-stmt" << std::endl;
    add_depth();
    node.block->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTSelectionStmt &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "selection-stmt" << std::endl;
    add_depth();
    node.cond->accept(*this);
    node.if_stmt->accept(*this);
    if (node.has_else) {
        node.else_stmt->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTIterationStmt &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "iteration-stmt" << std::endl;
    add_depth();
    node.cond->accept(*this);
    node.stmt->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTBreakStmt &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "break-stmt" << std::endl;
    return nullptr;
}

Value* ASTPrinter::visit(ASTContinueStmt &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "continue-stmt" << std::endl;
    return nullptr;
}
Value* ASTPrinter::visit(ASTReturnStmt &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "return-stmt" << std::endl;
    if (!node.is_empty) {
        add_depth();
        node.exp->accept(*this);
        remove_depth();
    }
    return nullptr;
}

Value* ASTPrinter::visit(ASTUnaryExp &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "unary-exp" << std::endl;
    add_depth();
    if (node.number != nullptr) {
        node.number->accept(*this);
    } else if (node.l_val != nullptr) {
        node.l_val->accept(*this);
    } else if (node.exp != nullptr) {
        if(node.has_unary_op){
            _DEBUG_PRINT_N_(depth);
            if(node.op == OP_POS){
                std::cout << "+" << std::endl;
            }
            else if(node.op == OP_NEG){
                std::cout << "-" << std::endl;
            }
            else if(node.op == OP_NOT){
                std::cout << "!" << std::endl;
            }
            else{
                _AST_NODE_ERROR_
            }
        }
        node.exp->accept(*this);
    } else {
        _DEBUG_PRINT_N_(depth);
        std::cout << "func-call: " << node.func_call_id << std::endl;
        add_depth();
        for (auto arg : node.func_call_args) {
            arg->accept(*this);
        }
        remove_depth();
    }
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTBinaryExp &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "binary-exp: ";
    if (node.op == OP_PLUS) {
        std::cout << "+" << std::endl;
    } else if (node.op == OP_MINUS) {
        std::cout << "-" << std::endl;
    } else if (node.op == OP_MUL) {
        std::cout << "*" << std::endl;
    } else if (node.op == OP_DIV) {
        std::cout << "/" << std::endl;
    } else if (node.op == OP_MOD) {
        std::cout << "%" << std::endl;
    } else if (node.op == OP_LT) {
        std::cout << "<" << std::endl;
    } else if (node.op == OP_LE) {
        std::cout << "<=" << std::endl;
    } else if (node.op == OP_GT) {
        std::cout << ">" << std::endl;
    } else if (node.op == OP_GE) {
        std::cout << ">=" << std::endl;
    } else if (node.op == OP_EQ) {
        std::cout << "==" << std::endl;
    } else if (node.op == OP_NEQ) {
        std::cout << "!=" << std::endl;
    } else if (node.op == OP_AND) {
        std::cout << "&&" << std::endl;
    } else if (node.op == OP_OR) {
        std::cout << "||" << std::endl;
    } else {
        _AST_NODE_ERROR_
    }
    add_depth();
    node.lhs->accept(*this);
    node.rhs->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTNumber &node) {
    _DEBUG_PRINT_N_(depth);
    if (node.type == TYPE_INT) {
        std::cout << "num (int): " << std::get<int>(node.value) << std::endl;
    } else if (node.type == TYPE_FLOAT) {
        std::cout << "num (float): " << std::get<float>(node.value) << std::endl;
    } else {
        _AST_NODE_ERROR_
    }
    return nullptr;
}

Value* ASTPrinter::visit(ASTLVal &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "lval: " << node.id << std::endl;
    for (auto exp : node.array_exp) {
        _DEBUG_PRINT_N_(depth);
        std::cout << "[" << std::endl;
        exp->accept(*this);
        _DEBUG_PRINT_N_(depth);
        std::cout << "]" << std::endl;
    }
    std::cout << std::endl;
    return nullptr;
}

Value* ASTPrinter::visit(ASTConstExp &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "const-exp" << std::endl;
    add_depth();
    node.exp->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTCond &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "cond" << std::endl;
    add_depth();
    node.exp->accept(*this);
    remove_depth();
    return nullptr;
}
