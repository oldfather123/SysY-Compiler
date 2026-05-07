// s->z->m
//修了吞东西的bug

#include "../../include/common/ast.hpp"
#include "../../include/common/syntax_tree.h"
#include <cstring>
#include <iostream>
#include <stack>
//#include "exit.hpp"

#define _AST_NODE_ERROR_                                   \
    std::cerr << "Abort due to node cast error."           \
                 "Contact with TAs to solve your problem." \
              << std::endl;                                \
    std::abort();
// 字符串比较宏
#define _STR_EQ(a, b) (strcmp((a), (b)) == 0)

void AST::run_visitor(ASTVisitor &visitor) { root->accept(visitor); }

AST::AST(syntax_tree *s)
{
    if (s == nullptr)
    {
        std::cerr << "empty input tree!" << std::endl;
        std::abort();
    }
    auto node = transform_node_iter(s->root);
    del_syntax_tree(s);
    root = std::static_pointer_cast<ASTProgram>(node);
}

std::shared_ptr<ASTNode> AST::transform_node_iter(syntax_tree_node *n)
{
    // 1. 处理 “program”
    if (_STR_EQ(n->name, "program"))
    {
        auto node = std::make_shared<ASTProgram>();

        // for (int i = 0; i < n->children_num; ++i) {
        //     auto child = n->children[i];
        //     if (_STR_EQ(child->name, "CompMod")) {
        //         std::stack<syntax_tree_node *> s;
        //         auto list_ptr = child;
        //         while (list_ptr->children_num == 2) {
        //             s.push(list_ptr->children[1]);
        //             list_ptr = list_ptr->children[0];
        //         }
        //         s.push(list_ptr->children[0]);
        //         while (!s.empty()) {
        //             auto ast_node = std::static_pointer_cast<ASTUnit>(
        //                 transform_node_iter(s.top()));
        //             node->compunits.push_back(ast_node);
        //             s.pop();
        //         }
        //     }
        //     else if (_STR_EQ(child->name, "MainMod")) {
        //         auto main_def_node = std::static_pointer_cast<ASTUnit>(
        //             transform_node_iter(child->children[0]));
        //         node->compunits.push_back(main_def_node);
        //     }
        // }
        std::stack<syntax_tree_node *> s;
        auto list_ptr = n->children[0];
        while (list_ptr->children_num == 2) {
            s.push(list_ptr->children[1]);
            list_ptr = list_ptr->children[0];
        }
        s.push(list_ptr->children[0]);
        while (!s.empty()) {
            auto ast_node = std::static_pointer_cast<ASTUnit>(
                transform_node_iter(s.top()));
            node->compunits.push_back(ast_node);
            s.pop();
        }
        return node;
    }
    // 2. 处理 “Decl”
    else if (_STR_EQ(n->name, "Decl"))
    {
        auto node = std::make_shared<ASTDecl>();
        auto child = n->children[0]; // ConstDecl / VarDecl / FuncDecl

        if (_STR_EQ(child->name, "ConstDecl"))
        {
            SysyType type;
            // ConstDecl -> CONST BType ConstDefList SEMICOLON
            if (_STR_EQ(child->children[1]->children[0]->name, "int")) {
                node->type = TYPE_INT;
                type = TYPE_INT;
            } else {
                node->type = TYPE_FLOAT;
                type = TYPE_FLOAT;
            }
            node->decl_kind = Const;

            std::stack<syntax_tree_node *> s;
            auto list_ptr = child->children[2]; // ConstDefList
            while (list_ptr->children_num == 3) {
                s.push(list_ptr->children[2]);
                list_ptr = list_ptr->children[0];
            }
            s.push(list_ptr->children[0]);

            while (!s.empty()) {
                auto child_node = std::static_pointer_cast<ASTConstDef>(
                    transform_node_iter(s.top()));
                child_node->type = type;
                node->cdef_lists.push_back(child_node);
                s.pop();
            }
        }
        else if (_STR_EQ(child->name, "VarDecl"))
        {
            SysyType type;
            // VarDecl -> BType VarDeclList SEMICOLON
            if (_STR_EQ(child->children[0]->children[0]->name, "int")) {
                node->type = TYPE_INT;
                type = TYPE_INT;
            } else {
                node->type = TYPE_FLOAT;
                type = TYPE_FLOAT;
            }
            node->decl_kind = Var;

            std::stack<syntax_tree_node *> s;
            auto list_ptr = child->children[1]; // VarDeclList
            while (list_ptr->children_num == 3) {
                s.push(list_ptr->children[2]);
                list_ptr = list_ptr->children[0];
            }
            s.push(list_ptr->children[0]);

            while (!s.empty()) {
                auto child_node = std::static_pointer_cast<ASTVarDef>(
                    transform_node_iter(s.top()));
                child_node->type = type;
                node->vdef_lists.push_back(child_node);
                s.pop();
            }
        }
        else if (_STR_EQ(child->name, "FuncDecl"))
        {
            // FuncDecl -> (BType|VOID) IDENT LPAREN (FuncParams)? RPAREN SEMICOLON
            node->decl_kind = DeclKind::Func;
            if (_STR_EQ(child->children[0]->name, "void") ||
                _STR_EQ(child->children[0]->name, "VOID")) {
                node->type = TYPE_VOID;
            } else {
                std::string btype_str = child->children[0]->children[0]->name;
                node->type = _STR_EQ(btype_str.c_str(), "int")
                                 ? TYPE_INT
                                 : TYPE_FLOAT;
            }
            node->func_name = child->children[1]->name;

            if (n->children_num == 6) {
                std::stack<syntax_tree_node *> s;
                auto list_ptr = n->children[3]; // FuncParams
                while (list_ptr->children_num == 3) {
                    s.push(list_ptr->children[2]);
                    list_ptr = list_ptr->children[0];
                }
                s.push(list_ptr->children[0]);
                while (!s.empty()) {
                    auto param_node = std::static_pointer_cast<ASTParam>(
                        transform_node_iter(s.top()));
                    node->params.push_back(param_node);
                    s.pop();
                }
            } else {
                node->params.clear();
            }
        }

        return node;
    }
    // 3. 处理 “ConstDef”
    else if (_STR_EQ(n->name, "ConstDef"))
    {
        auto node = std::make_shared<ASTConstDef>();
        node->id = n->children[0]->name;
        node->is_const = true;

        if (n->children[1]->children_num == 0) {
            node->is_array = false;
        } else {
            node->is_array = true;
            std::stack<syntax_tree_node *> s_exp;
            auto list_ptr = n->children[1]; // ConstList
            while (list_ptr->children_num == 4) {
                s_exp.push(list_ptr->children[2]);
                list_ptr = list_ptr->children[0];
            }
            while (!s_exp.empty()) {
                auto exp_node = std::static_pointer_cast<ASTExp>(
                    transform_node_iter(s_exp.top()));
                node->exp_lists.push_back(exp_node);
                node->length++;
                s_exp.pop();
            }
        }

        auto initval_node = std::static_pointer_cast<ASTInitVal>(
            transform_node_iter(n->children[3])); // ConstInit
        node->initval_list = initval_node;
        return node;
    }
    // 4. 处理 “VarDef”
    else if (_STR_EQ(n->name, "VarDef"))
    {
        auto node = std::make_shared<ASTVarDef>();
        node->id = n->children[0]->name;
        node->is_const = false;

        if (n->children[1]->children_num == 0) {
            node->is_array = false;
        } else {
            node->is_array = true;
            std::stack<syntax_tree_node *> s;
            auto list_ptr = n->children[1]; // ConstList
            while (list_ptr->children_num == 4) {
                s.push(list_ptr->children[2]);
                list_ptr = list_ptr->children[0];
            }
            while (!s.empty()) {
                auto exp_node = std::static_pointer_cast<ASTExp>(
                    transform_node_iter(s.top()));
                node->exp_lists.push_back(exp_node);
                node->length++;
                s.pop();
            }
        }

        if (n->children_num == 4) {
            auto initval_node = std::static_pointer_cast<ASTInitVal>(
                transform_node_iter(n->children[3])); // InitVal
            node->initval_list = initval_node;
        }
        return node;
    }
    // 5. 处理 “ConstInitVal”
    else if (_STR_EQ(n->name, "ConstInit"))
    {
        auto node = std::make_shared<ASTInitVal>();
        node->is_const = true;

        if (n->children_num == 1) {
            // ConstInitVal -> ConstExp
            auto exp_node = std::static_pointer_cast<ASTExp>(
                transform_node_iter(n->children[0]));
            node->value = exp_node;
            node->is_singlevalue = true;
        }
        else if (n->children_num == 3) {
            // ConstInitVal -> { ConstInitValList }
            node->is_singlevalue = false;
            std::stack<syntax_tree_node *> s;
            auto list_ptr = n->children[1]; // ConstInitValList
            while (list_ptr->children_num == 3) {
                s.push(list_ptr->children[2]);
                list_ptr = list_ptr->children[0];
            }
            s.push(list_ptr->children[0]);
            while (!s.empty()) {
                auto initval_node = std::static_pointer_cast<ASTInitVal>(
                    transform_node_iter(s.top()));
                node->initval_list.push_back(initval_node);
                s.pop();
            }
        }
        else {
            // ConstInitVal -> { }
            node->value = nullptr;
        }
        return node;
    }
    // 6. 处理 “InitVal”
    else if (_STR_EQ(n->name, "InitVal"))
    {
        auto node = std::make_shared<ASTInitVal>();
        node->is_const = false;

        if (n->children_num == 1) {
            // InitVal -> Expr
            auto exp_node = std::static_pointer_cast<ASTExp>(
                transform_node_iter(n->children[0]));
            node->value = exp_node;
            node->is_singlevalue = true;
        }
        else if (n->children_num == 3) {
            // InitVal -> { InitValList }
            node->is_singlevalue = false;
            std::stack<syntax_tree_node *> s;
            auto list_ptr = n->children[1]; // InitValList
            while (list_ptr->children_num == 3) {
                s.push(list_ptr->children[2]);
                list_ptr = list_ptr->children[0];
            }
            s.push(list_ptr->children[0]);
            while (!s.empty()) {
                auto initval_node = std::static_pointer_cast<ASTInitVal>(
                    transform_node_iter(s.top()));
                node->initval_list.push_back(initval_node);
                s.pop();
            }
        }
        else {
            node->value = nullptr;
        }
        return node;
    }
    // 7. 处理 “Expr” / “ConstExp”
    else if (_STR_EQ(n->name, "Expr"))
    {
        auto node = std::make_shared<ASTExp>();
        node->is_const = false;
        auto addexp_node = std::static_pointer_cast<ASTAddExp>(
            transform_node_iter(n->children[0])); // Add_Expr
        node->add_exp = addexp_node;
        return node;
    }
    else if (_STR_EQ(n->name, "ConstExp"))
    {
        auto node = std::make_shared<ASTExp>();
        node->is_const = true;
        auto addexp_node = std::static_pointer_cast<ASTAddExp>(
            transform_node_iter(n->children[0])); // Add_Expr
        node->add_exp = addexp_node;
        return node;
    }
    // 8. 处理 “FuncDef”
    else if (_STR_EQ(n->name, "FuncDef"))
    {
        auto node = std::make_shared<ASTFuncDef>();
        node->id = n->children[1]->name; // IDENT

        if (_STR_EQ(n->children[0]->name, "void")) {
            node->type = TYPE_VOID;
        } else {
            if (_STR_EQ(n->children[0]->children[0]->name, "int"))
                node->type = TYPE_INT;
            else
                node->type = TYPE_FLOAT;
        }

        if (n->children_num == 6) {
            std::stack<syntax_tree_node *> s;
            auto list_ptr = n->children[3]; // FuncParams
            if (!_STR_EQ(list_ptr->children[0]->name, "VOID")) {
                while (list_ptr->children_num == 3) {
                    s.push(list_ptr->children[2]);
                    list_ptr = list_ptr->children[0];
                }
                s.push(list_ptr->children[0]);
                while (!s.empty()) {
                    auto child_node = std::static_pointer_cast<ASTParam>(
                        transform_node_iter(s.top()));
                    node->params.push_back(child_node);
                    s.pop();
                }
            }
            auto block_node = std::static_pointer_cast<ASTBlock>(
                transform_node_iter(n->children[5])); // Body
            node->block = block_node;
        }
        else {
            auto block_node = std::static_pointer_cast<ASTBlock>(
                transform_node_iter(n->children[4])); // Body
            node->block = block_node;
        }
        return node;
    }
    // 9. 处理 “MainDef”
    // else if (_STR_EQ(n->name, "MainDef"))
    // {
    //     auto node = std::make_shared<ASTMainDef>();
    //     node->id = n->children[1]->name; // MAIN
    //     node->type = TYPE_INT;

    //     if (n->children_num == 6) {
    //         auto block_node = std::static_pointer_cast<ASTBlock>(
    //             transform_node_iter(n->children[5])); // Body
    //         node->block = block_node;
    //     }
    //     else {
    //         auto block_node = std::static_pointer_cast<ASTBlock>(
    //             transform_node_iter(n->children[4])); // Body
    //         node->block = block_node;
    //     }
    //     return node;
    // }
    // 10. 处理 “FuncParam”
    else if (_STR_EQ(n->name, "FuncParam"))
    {
        auto node = std::make_shared<ASTParam>();
        if (_STR_EQ(n->children[0]->children[0]->name, "int"))
            node->type = TYPE_INT;
        else
            node->type = TYPE_FLOAT;

        node->id = n->children[1]->name; // IDENT

        if (n->children_num == 2) {
            node->isarray = false;
        } else {
            node->isarray = true;
            std::stack<syntax_tree_node *> s;
            auto list_ptr = n->children[4]; // ExpList
            while (list_ptr->children_num == 4) {
                s.push(list_ptr->children[2]);
                list_ptr = list_ptr->children[0];
            }
            while (!s.empty()) {
                auto child_node = std::static_pointer_cast<ASTExp>(
                    transform_node_iter(s.top()));
                node->array_lists.push_back(child_node);
                s.pop();
            }
        }
        return node;
    }
    // 11. 处理 “Body” (也就是 Block)
    else if (_STR_EQ(n->name, "Body"))
    {
        auto node = std::make_shared<ASTBlock>();
        std::stack<syntax_tree_node *> s;
        auto list_ptr = n->children[1]; // BodyList
        while (list_ptr->children_num == 2) {
            s.push(list_ptr->children[1]);
            list_ptr = list_ptr->children[0];
        }
        while (!s.empty()) {
            if (_STR_EQ(s.top()->children[0]->name, "Decl")) {
                auto child_node = std::static_pointer_cast<ASTDecl>(
                    transform_node_iter(s.top()->children[0]));
                node->decl_lists.push_back(child_node);
                node->list_type.push_back(0);
                s.pop();
            }
            else {
                std::shared_ptr<ASTStmt> child_node;

                // 11.2.1 赋值 / 复合赋值
                // if (_STR_EQ(s.top()->children[0]->children[0]->name, "LVal")) {
                //     auto assign_op_node = s.top()->children[0]->children[1];
                //     std::string op_name = assign_op_node->name;
                //     if (op_name == "ASSIGN") {
                //         auto node = std::make_shared<ASTAssignStmt>();
                //         auto var_node = std::static_pointer_cast<ASTVar>(
                //             transform_node_iter(s.top()->children[0]->children[0]));
                //         node->var = var_node;
                //         auto exp_node = std::static_pointer_cast<ASTExp>(
                //             transform_node_iter(s.top()->children[0]->children[2]));
                //         node->expression = exp_node;
                //         child_node = std::static_pointer_cast<ASTStmt>(node);
                //     }
                //     else {
                //         auto node = std::make_shared<ASTCompoundAssignStmt>();
                //         auto var_node = std::static_pointer_cast<ASTVar>(
                //             transform_node_iter(s.top()->children[0]->children[0]));
                //         node->var = var_node;
                //         auto exp_node = std::static_pointer_cast<ASTExp>(
                //             transform_node_iter(s.top()->children[0]->children[2]));
                //         node->expression = exp_node;
                //         if (op_name == "ADASS") node->op = "+=";
                //         else if (op_name == "SUASS") node->op = "-=";
                //         else if (op_name == "MUASS") node->op = "*=";
                //         else if (op_name == "DIASS") node->op = "/=";
                //         else node->op = op_name;
                //         child_node = std::static_pointer_cast<ASTStmt>(node);
                //     }
                if (_STR_EQ(s.top()->children[0]->children[0]->name, "LVal"))
                {
                    auto assign_op_node = s.top()->children[0]->children[1]; // 赋值操作符节点
                    std::string op_name = assign_op_node->name;              // ASSIGN, ADASS, SUASS, MUASS, DIASS等
                    if (op_name == "ASSIGN" || op_name == "=")
                    {
                        // 普通赋值 LVal = Expr;
                        auto node = std::make_shared<ASTAssignStmt>();
                        auto var_node = std::static_pointer_cast<ASTVar>(transform_node_iter(s.top()->children[0]->children[0]));
                        node->var = var_node;
                        auto exp_node = std::static_pointer_cast<ASTExp>(transform_node_iter(s.top()->children[0]->children[2]));
                        node->expression = exp_node;
                        child_node = std::static_pointer_cast<ASTStmt>(node);
                    }
                    else
                    {
                        // 复合赋值 e.g. LVal += Expr;
                        auto node = std::make_shared<ASTCompoundAssignStmt>();
                        auto var_node = std::static_pointer_cast<ASTVar>(transform_node_iter(s.top()->children[0]->children[0]));
                        node->var = var_node;
                        auto exp_node = std::static_pointer_cast<ASTExp>(transform_node_iter(s.top()->children[0]->children[2]));
                        node->expression = exp_node;

                        // 记录具体复合赋值类型，比如 "+=", "-=" 等
                        if (op_name == "ADASS" || op_name == "+=")
                            node->op = "+=";
                        else if (op_name == "SUASS" || op_name == "-=")
                            node->op = "-=";
                        else if (op_name == "MUASS" || op_name == "*=")
                            node->op = "*=";
                        else if (op_name == "DIASS" || op_name == "/=")
                            node->op = "/=";
                        else
                            node->op = op_name; // 防御写法
                    }
                }
                // 11.2.2 “Expr;”
                else if (_STR_EQ(s.top()->children[0]->children[0]->name, "Expr")) {
                    child_node = std::static_pointer_cast<ASTStmt>(
                        transform_node_iter(s.top()->children[0]->children[0]));
                }
                // 11.2.3 嵌套 Block
                else if (_STR_EQ(s.top()->children[0]->children[0]->name, "Body")) {
                    child_node = std::static_pointer_cast<ASTStmt>(
                        transform_node_iter(s.top()->children[0]->children[0]));
                }
                // 11.2.4 if 语句：IFItem
                else if (_STR_EQ(s.top()->children[0]->children[0]->name, "IFItem")) {
                    auto node = std::make_shared<ASTSelectionStmt>();
                    auto cond_node = std::static_pointer_cast<ASTOrExp>(
                        transform_node_iter(
                            s.top()->children[0]->children[0]->children[2]->children[0]
                        )
                    );
                    node->cond = cond_node;
                    auto if_stmt_node = std::static_pointer_cast<ASTStmt>(
                        transform_node_iter(
                            s.top()->children[0]->children[0]->children[4]
                        )
                    );
                    node->if_stmt = if_stmt_node;
                    if (s.top()->children[0]->children[0]->children_num == 7) {
                        auto else_stmt_node = std::static_pointer_cast<ASTStmt>(
                            transform_node_iter(
                                s.top()->children[0]->children[0]->children[6]
                            )
                        );
                        node->else_stmt = else_stmt_node;
                    }
                    child_node = std::static_pointer_cast<ASTStmt>(node);
                }
                // 11.2.5 while 语句：WhileItem
                else if (_STR_EQ(s.top()->children[0]->children[0]->name, "WhileItem")) {
                    auto node = std::make_shared<ASTIterationStmt>();
                    auto cond_node = std::static_pointer_cast<ASTOrExp>(
                        transform_node_iter(
                            s.top()->children[0]->children[0]->children[2]->children[0]
                        )
                    );
                    node->cond = cond_node;
                    auto stmt_node = std::static_pointer_cast<ASTStmt>(
                        transform_node_iter(
                            s.top()->children[0]->children[0]->children[4]
                        )
                    );
                    node->stmt = stmt_node;
                    child_node = std::static_pointer_cast<ASTStmt>(node);
                }
                // 11.2.6 jump_stmt: return / break / continue
                else if (_STR_EQ(s.top()->children[0]->children[0]->name, "jump_stmt")) {
                    auto jump_node = s.top()->children[0]->children[0];
                    if (_STR_EQ(jump_node->children[0]->name, "return")) {
                        auto node = std::make_shared<ASTReturnStmt>();
                        if (jump_node->children_num == 3) {
                            auto exp = std::static_pointer_cast<ASTExp>(
                                transform_node_iter(jump_node->children[1]));
                            node->expression = exp;
                        }
                        child_node = std::static_pointer_cast<ASTStmt>(node);
                    }
                    else if (_STR_EQ(jump_node->children[0]->name, "break")) {
                        auto node = std::make_shared<ASTBreak>();
                        node->is_break = true;
                        child_node = std::static_pointer_cast<ASTStmt>(node);
                    }
                    else if (_STR_EQ(jump_node->children[0]->name, "continue")) {
                        auto node = std::make_shared<ASTContinue>();
                        node->is_continue = true;
                        child_node = std::static_pointer_cast<ASTStmt>(node);
                    }
                    else {
                        child_node = nullptr;
                    }
                }
                // 11.2.7 其他情况，直接跳过
                else {
                    child_node = nullptr;
                }

                if (child_node != nullptr) {
                    node->stmt_lists.push_back(child_node);
                    node->list_type.push_back(1);
                }
                s.pop();
            }
        }
        return node;
    }
    // 12. 处理 “stmt”（顶层）
    else if (_STR_EQ(n->name, "stmt"))
    {
        // a) 空语句 “;”
        if (_STR_EQ(n->children[0]->name, ";"))
            return nullptr;

        // b) 赋值/复合赋值：Stmt -> LVal ASSIGN Expr ; 或 LVal ADASS Expr ;
        if (_STR_EQ(n->children[0]->name, "LVal")) {
            std::string op_name = n->children[1]->name;
            if (op_name == "ASSIGN" || op_name == "=") {
                auto node = std::make_shared<ASTAssignStmt>();
                auto var_node = std::static_pointer_cast<ASTVar>(
                    transform_node_iter(n->children[0]));
                node->var = var_node;
                auto exp_node = std::static_pointer_cast<ASTExp>(
                    transform_node_iter(n->children[2]));
                node->expression = exp_node;
                return node;
            }
            else {
                auto node = std::make_shared<ASTCompoundAssignStmt>();
                auto var_node = std::static_pointer_cast<ASTVar>(
                    transform_node_iter(n->children[0]));
                node->var = var_node;
                auto exp_node = std::static_pointer_cast<ASTExp>(
                    transform_node_iter(n->children[2]));
                node->expression = exp_node;
                if (op_name == "ADASS") node->op = "+=";
                else if (op_name == "SUASS") node->op = "-=";
                else if (op_name == "MUASS") node->op = "*=";
                else if (op_name == "DIASS") node->op = "/=";
                else node->op = op_name;
                return node;
            }
        }
        // c) 表达式语句：Stmt -> Expr ;
        else if (_STR_EQ(n->children[0]->name, "Expr")) {
            return transform_node_iter(n->children[0]);
        }
        // d) 嵌套 Block：Stmt -> Body
        else if (_STR_EQ(n->children[0]->name, "Body")) {
            return transform_node_iter(n->children[0]);
        }
        // e) if 语句：Stmt -> IFItem
        else if (_STR_EQ(n->children[0]->name, "IFItem"))
        {
            auto node = std::make_shared<ASTSelectionStmt>();
            auto ifnode = n->children[0];
            // IFItem -> IF LPAREN Cond RPAREN stmt (ELSE stmt)?
            auto cond_node = std::static_pointer_cast<ASTOrExp>(
                transform_node_iter(ifnode->children[2]->children[0])
            );
            node->cond = cond_node;
            auto then_stmt = std::static_pointer_cast<ASTStmt>(
                transform_node_iter(ifnode->children[4])
            );
            node->if_stmt = then_stmt;
            if (ifnode->children_num == 7) {
                auto else_stmt = std::static_pointer_cast<ASTStmt>(
                    transform_node_iter(ifnode->children[6])
                );
                node->else_stmt = else_stmt;
            }
            return node;
        }
        // f) while 语句：Stmt -> WhileItem
        else if (_STR_EQ(n->children[0]->name, "WhileItem"))
        {
            auto node = std::make_shared<ASTIterationStmt>();
            auto whilenode = n->children[0];
            // WhileItem -> WHILE LPAREN Cond RPAREN stmt
            auto cond_node = std::static_pointer_cast<ASTOrExp>(
                transform_node_iter(whilenode->children[2]->children[0])
            );
            node->cond = cond_node;
            auto stmt_node = std::static_pointer_cast<ASTStmt>(
                transform_node_iter(whilenode->children[4])
            );
            node->stmt = stmt_node;
            return node;
        }
        // g) 
        else if (_STR_EQ(n->children[0]->name, "jump_stmt"))
        {
            auto jump_node = n->children[0];
            if (_STR_EQ(jump_node->children[0]->name, "return")) {
                auto node = std::make_shared<ASTReturnStmt>();
                if (jump_node->children_num == 3) {
                    auto exp = std::static_pointer_cast<ASTExp>(
                        transform_node_iter(jump_node->children[1]));
                    node->expression = exp;
                }
                return node;
            }
            else if (_STR_EQ(jump_node->children[0]->name, "break")) {
                auto node = std::make_shared<ASTBreak>();
                node->is_break = true;
                return node;
            }
            else if (_STR_EQ(jump_node->children[0]->name, "continue")) {
                auto node = std::make_shared<ASTContinue>();
                node->is_continue = true;
                return node;
            }
            else {
                return nullptr;
            }
        }
        // i) 其他情况，跳过
        else
        {
            return nullptr;
        }
    }
    // 13. 处理 “LVal”
    else if (_STR_EQ(n->name, "LVal"))
    {
        auto node = std::make_shared<ASTVar>();
        node->id = n->children[0]->name;
        std::stack<syntax_tree_node *> s;
        auto list_ptr = n->children[1]; // ExpList
        while (list_ptr->children_num == 4) {
            s.push(list_ptr->children[2]);
            list_ptr = list_ptr->children[0];
        }
        while (!s.empty()) {
            auto child_node = std::static_pointer_cast<ASTExp>(
                transform_node_iter(s.top()));
            node->length++;
            node->array_lists.push_back(child_node);
            s.pop();
        }
        return node;
    }
    // 14. 处理 “Func_Expr”（函数调用）
    else if (_STR_EQ(n->name, "Func_Expr"))
    {
        auto node = std::make_shared<ASTFuncExp>();
        node->id = n->children[0]->name; // IDENT
        if (n->children_num == 4) {
            std::stack<syntax_tree_node *> s_args;
            auto list_ptr = n->children[2]; // FuncRParams
            while (list_ptr->children_num == 3) {
                s_args.push(list_ptr->children[2]);
                list_ptr = list_ptr->children[0];
            }
            s_args.push(list_ptr->children[0]);
            while (!s_args.empty()) {
                auto child_node = std::static_pointer_cast<ASTExp>(
                    transform_node_iter(s_args.top()));
                node->args.push_back(child_node);
                s_args.pop();
            }
        }
        return node;
    }
    // 15. 处理 “OR_Expr”
    else if (_STR_EQ(n->name, "OR_Expr"))
    {
        auto node = std::make_shared<ASTOrExp>();
        if (n->children_num == 3) {
            auto lor_exp_node = std::static_pointer_cast<ASTOrExp>(
                transform_node_iter(n->children[0])
            );
            node->lor_exp = lor_exp_node;
            node->or_op = OP_OR;
            auto and_exp_node = std::static_pointer_cast<ASTAndExp>(
                transform_node_iter(n->children[2])
            );
            node->land_exp = and_exp_node;
        } else {
            auto and_exp_node = std::static_pointer_cast<ASTAndExp>(
                transform_node_iter(n->children[0])
            );
            node->land_exp = and_exp_node;
        }
        return node;
    }
    // 16. 处理 “And_expr”
    else if (_STR_EQ(n->name, "And_expr"))
    {
        auto node = std::make_shared<ASTAndExp>();
        if (n->children_num == 3) {
            auto land_exp_node = std::static_pointer_cast<ASTAndExp>(
                transform_node_iter(n->children[0])
            );
            node->land_exp = land_exp_node;
            node->and_op = OP_AND;
            auto eq_exp_node = std::static_pointer_cast<ASTEqExp>(
                transform_node_iter(n->children[2])
            );
            node->eq_exp = eq_exp_node;
        } else {
            auto eq_exp_node = std::static_pointer_cast<ASTEqExp>(
                transform_node_iter(n->children[0])
            );
            node->eq_exp = eq_exp_node;
        }
        return node;
    }
    // 17. 处理 “Eq_Expr”
    else if (_STR_EQ(n->name, "Eq_Expr"))
    {
        auto node = std::make_shared<ASTEqExp>();
        if (n->children_num == 3) {
            auto eq_exp_node = std::static_pointer_cast<ASTEqExp>(
                transform_node_iter(n->children[0])
            );
            node->eq_exp = eq_exp_node;
            if (_STR_EQ(n->children[1]->name, "==")) {
                node->op = OP_EQ;
            } else {
                node->op = OP_NEQ;
            }
            auto rel_exp_node = std::static_pointer_cast<ASTRelExp>(
                transform_node_iter(n->children[2])
            );
            node->rel_exp = rel_exp_node;
        } else {
            auto rel_exp_node = std::static_pointer_cast<ASTRelExp>(
                transform_node_iter(n->children[0])
            );
            node->rel_exp = rel_exp_node;
        }
        return node;
    }
    // 18. 处理 “Rel_Expr”
    else if (_STR_EQ(n->name, "Rel_Expr"))
    {
        auto node = std::make_shared<ASTRelExp>();
        if (n->children_num == 3) {
            auto rel_exp_node = std::static_pointer_cast<ASTRelExp>(
                transform_node_iter(n->children[0])
            );
            node->rel_exp = rel_exp_node;
            if (_STR_EQ(n->children[1]->name, "<")) {
                node->op = OP_LT;
            }
            else if (_STR_EQ(n->children[1]->name, "<=")) {
                node->op = OP_LE;
            }
            else if (_STR_EQ(n->children[1]->name, ">")) {
                node->op = OP_GT;
            }
            else {
                node->op = OP_GE;
            }
            auto add_exp_node = std::static_pointer_cast<ASTAddExp>(
                transform_node_iter(n->children[2])
            );
            node->add_exp = add_exp_node;
        }
        else {
            auto add_exp_node = std::static_pointer_cast<ASTAddExp>(
                transform_node_iter(n->children[0])
            );
            node->add_exp = add_exp_node;
        }
        return node;
    }
    // 19. 处理 “Add_Expr”
    else if (_STR_EQ(n->name, "Add_Expr"))
    {
        auto node = std::make_shared<ASTAddExp>();
        auto outer = node;
        while (true) {
            if (n->children_num == 3) {
                if (_STR_EQ(n->children[1]->name, "+")) {
                    node->op = OP_ADD;
                } else {
                    node->op = OP_SUB;
                }
                auto mul_exp_node = std::static_pointer_cast<ASTMulExp>(
                    transform_node_iter(n->children[2])
                );
                node->mul_exp = mul_exp_node;
                auto new_node = std::make_shared<ASTAddExp>();
                node->add_exp = new_node;
                node = new_node;
                n = n->children[0];
            } else {
                auto mul_exp_node = std::static_pointer_cast<ASTMulExp>(
                    transform_node_iter(n->children[0])
                );
                node->mul_exp = mul_exp_node;
                node->add_exp = nullptr;
                break;
            }
        }
        return outer;
    }
    // 20. 处理 “MDM_Expr”
    else if (_STR_EQ(n->name, "MDM_Expr"))
    {
        auto node = std::make_shared<ASTMulExp>();
        if (n->children_num == 3) {
            auto mul_exp_node = std::static_pointer_cast<ASTMulExp>(
                transform_node_iter(n->children[0])
            );
            node->mul_exp = mul_exp_node;
            if (_STR_EQ(n->children[1]->name, "*")) {
                node->op = OP_MUL;
            }
            else if (_STR_EQ(n->children[1]->name, "/")) {
                node->op = OP_DIV;
            }
            else {
                node->op = OP_MOD;
            }
            auto unary_exp_node = std::static_pointer_cast<ASTUnaryExp>(
                transform_node_iter(n->children[2])
            );
            node->unary_exp = unary_exp_node;
        }
        else {
            auto unary_exp_node = std::static_pointer_cast<ASTUnaryExp>(
                transform_node_iter(n->children[0])
            );
            node->unary_exp = unary_exp_node;
        }
        return node;
    }
    // 21. 处理 “Unary_Expr”
    else if (_STR_EQ(n->name, "Unary_Expr"))
    {
        auto node = std::make_shared<ASTUnaryExp>();
        // 只有一个 Primary_Expr
        if (n->children_num == 1 &&
            _STR_EQ(n->children[0]->name, "Primary_Expr")) {
            auto primary_node = std::static_pointer_cast<ASTPrimaryExp>(
                transform_node_iter(n->children[0])
            );
            node->primary_exp = primary_node;
        }
        // 一元运算符
        else if (n->children_num == 2) {
            if (_STR_EQ(n->children[0]->name, "+")) {
                node->unary_op = OP_PLUS;
            }
            else if (_STR_EQ(n->children[0]->name, "-")) {
                node->unary_op = OP_MINUS;
            }
            else {
                node->unary_op = OP_NOT;
            }
            auto unary_exp_node = std::static_pointer_cast<ASTUnaryExp>(
                transform_node_iter(n->children[1])
            );
            node->unary_exp = unary_exp_node;
        }
        // 函数调用：Func_Expr
        else {
            auto call_node = std::make_shared<ASTFuncExp>();
            call_node->id = n->children[0]->children[0]->name; // IDENT
            if (n->children[0]->children_num == 4) {
                std::stack<syntax_tree_node *> s;
                auto list_ptr = n->children[0]->children[2]; // FuncRParams
                while (list_ptr->children_num == 3) {
                    s.push(list_ptr->children[2]);
                    list_ptr = list_ptr->children[0];
                }
                s.push(list_ptr->children[0]);
                while (!s.empty()) {
                    auto child_node = std::static_pointer_cast<ASTExp>(
                        transform_node_iter(s.top())
                    );
                    call_node->args.push_back(child_node);
                    s.pop();
                }
            }
            node->func_exp = call_node;
        }
        return node;
    }
    // 22. 处理 “Primary_Expr”
    else if (_STR_EQ(n->name, "Primary_Expr"))
    {
        if (n->children_num == 1) {
            return transform_node_iter(n->children[0]);
        }
        else {
            return transform_node_iter(n->children[1]);
        }
    }
    // 23. 处理 “Number”
    else if (_STR_EQ(n->name, "Number"))
    {
        auto node = std::make_shared<ASTNum>();
        if (_STR_EQ(n->children[0]->name, "Integer")) {
            node->type = TYPE_INT;
            std::string value = n->children[0]->children[0]->name; // INTCONST
            if (value.size() > 1 && (value[1] == 'x' || value[1] == 'X')) {
                node->i_val = std::stoi(value, nullptr, 16);
            }
            else if (value[0] == '0' && value.size() > 1) {
                node->i_val = std::stoi(value, nullptr, 8);
            }
            else {
                node->i_val = std::stoi(n->children[0]->children[0]->name);
            }
        }
        else if (_STR_EQ(n->children[0]->name, "Floatnum")) {
            node->type = TYPE_FLOAT;
            std::string value = n->children[0]->children[0]->name; // FLOATCONST
            if (value.size() > 1 && (value[1] == 'x' || value[1] == 'X')) {
                char *endPtr;
                node->f_val = static_cast<float>(std::strtod(value.c_str(), &endPtr));
            }
            else {
                node->f_val = std::stof(value, nullptr);
            }
        }
        else {
            std::cerr << n->children[0]->name << std::endl;
            std::cerr << "Wrong Type" << std::endl;
            std::abort();
        }
        return node;
    }
    // 24. 兜底：都不匹配时返回 nullptr
    else
    {
        return nullptr;
    }
}


Value *ASTProgram::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTFuncDef::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
// Value *ASTMainDef::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTDecl::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTConstDef::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTVarDef::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTParam::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTBlock::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTAssignStmt::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTCompoundAssignStmt::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTSelectionStmt::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTIterationStmt::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTBreak::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTContinue::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTReturnStmt::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTVar::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTNum::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTUnaryExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTFuncExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTAddExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTMulExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTRelExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTEqExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTAndExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTOrExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value *ASTInitVal::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
// 打印一棵AST
#define _DEBUT_PRINT_N_(N)                \
    {                                     \
        std::cout << std::string(N, '-'); \
    }

Value *ASTPrinter::visit(ASTProgram &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << "program" << std::endl;
    add_depth();
    for (auto comp : node.compunits)
    {
        comp->accept(*this);
    }
    remove_depth();
    return nullptr;
}
Value *ASTPrinter::visit(ASTNum &node)
{
    _DEBUT_PRINT_N_(depth);
    if (node.type == TYPE_INT)
    {
        std::cout << "num(int): " << node.i_val << std::endl;
    }
    else if (node.type == TYPE_FLOAT)
    {
        std::cout << "num(float): " << node.f_val << std::endl;
    }
    else
    {
        _AST_NODE_ERROR_
    }
    return nullptr;
}
Value *ASTPrinter::visit(ASTDecl &node)
{
    _DEBUT_PRINT_N_(depth);
    if (node.decl_kind == Const)
    {
        std::cout << "ConstDecl: " << static_cast<int>(node.type) << std::endl;
        add_depth();
        for (auto &def : node.cdef_lists)
        {
            def->accept(*this);
        }
        remove_depth();
    }
    else if (node.decl_kind == Var)
    {
        std::cout << "VarDecl: " << static_cast<int>(node.type) << std::endl;
        add_depth();
        for (auto &def : node.vdef_lists)
        {
            def->accept(*this);
        }
        remove_depth();
    }
    else // FuncDecl
    {
        std::cout << "FuncDecl: " << node.func_name << std::endl;

        if (!node.params.empty())
        {
            add_depth();
            std::cout << "Params:" << std::endl;
            add_depth();
            for (auto &param : node.params)
            {
                param->accept(*this);
            }
            remove_depth();
            remove_depth();
        }
        // 这里假设函数体是def_lists的第一个元素，或者你有专门字段保存函数体
        if (!node.cdef_lists.empty())
        {
            add_depth();
            std::cout << "Function Body:" << std::endl;
            add_depth();
            for (auto &def : node.cdef_lists)
            {
                def->accept(*this);
            }
            remove_depth();
            remove_depth();
        }
        if (!node.vdef_lists.empty())
        {
            add_depth();
            std::cout << "Function Body:" << std::endl;
            add_depth();
            for (auto &def : node.vdef_lists)
            {
                def->accept(*this);
            }
            remove_depth();
            remove_depth();
        }
    }
    return nullptr;
}
Value *ASTPrinter::visit(ASTFuncDef &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << "FuncDef: " << node.id << std::endl;
    add_depth();
    for (auto param : node.params)
    {
        param->accept(*this);
    }
    node.block->accept(*this);
    remove_depth();

    return nullptr;
}
// Value *ASTPrinter::visit(ASTMainDef &node)
// {
//     _DEBUT_PRINT_N_(depth);
//     std::cout << "MainDef: " << node.id << std::endl;
//     add_depth();
//     node.block->accept(*this);
//     remove_depth();

//     return nullptr;
// }
Value *ASTPrinter::visit(ASTConstDef &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << node.id << std::endl;
    if (node.length > 0)
    {
        for (auto exp : node.exp_lists)
        {
            add_depth();
            _DEBUT_PRINT_N_(depth);
            std::cout << "[]" << std::endl;
            exp->accept(*this);
            remove_depth();
        }
    }
    if (node.initval_list != nullptr)
    {
        add_depth();
        _DEBUT_PRINT_N_(depth);
        std::cout << "=" << std::endl;
        node.initval_list->accept(*this);
        remove_depth();
    }
    return nullptr;
}
Value *ASTPrinter::visit(ASTVarDef &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << node.id << std::endl;
    if (node.length > 0)
    {
        for (auto exp : node.exp_lists)
        {
            add_depth();
            _DEBUT_PRINT_N_(depth);
            std::cout << "[]" << std::endl;
            exp->accept(*this);
            remove_depth();
        }
    }
    if (node.initval_list != nullptr)
    {
        add_depth();
        _DEBUT_PRINT_N_(depth);
        std::cout << "=" << std::endl;
        node.initval_list->accept(*this);
        remove_depth();
    }
    return nullptr;
}
Value *ASTPrinter::visit(ASTParam &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << "param: " << std::endl;
    if (node.type == TYPE_INT)
    {
        std::cout << "int " << node.id;
    }
    else
    {
        std::cout << "float " << node.id;
    }
    if (node.isarray)
        std::cout << "[]" << std::endl;
    else
        std::cout << std::endl;
    if (!node.array_lists.empty())
    {
        for (auto array : node.array_lists)
        {
            add_depth();
            _DEBUT_PRINT_N_(depth);
            std::cout << "[]" << std::endl;
            array->accept(*this);
            remove_depth();
        }
    }
    return nullptr;
}
Value *ASTPrinter::visit(ASTBlock &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << "Block" << std::endl;
    add_depth();
    for (auto decl : node.decl_lists)
    {
        decl->accept(*this);
    }
    for (auto stmt : node.stmt_lists)
    {
        stmt->accept(*this);
    }
    remove_depth();
    return nullptr;
}
Value *ASTPrinter::visit(ASTAssignStmt &node)
{
    // LOG(DEBUG) << "assign";
    _DEBUT_PRINT_N_(depth);
    std::cout << "AssignStmt: " << std::endl;
    add_depth();
    node.var->accept(*this);
    node.expression->accept(*this);
    remove_depth();
    return nullptr;
}
Value *ASTPrinter::visit(ASTCompoundAssignStmt &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << "CompoundAssignStmt: " << node.op << std::endl;

    if (node.var)
    {
        add_depth();
        node.var->accept(*this);
        remove_depth();
    }
    if (node.expression)
    {
        add_depth();
        node.expression->accept(*this);
        remove_depth();
    }
    return nullptr;
}
Value *ASTPrinter::visit(ASTSelectionStmt &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << "SelectionStmt: " << std::endl;
    add_depth();
    node.cond->accept(*this);
    if (node.if_stmt != nullptr)
        node.if_stmt->accept(*this);
    if (node.else_stmt != nullptr)
        node.else_stmt->accept(*this);
    remove_depth();
    return nullptr;
}
Value *ASTPrinter::visit(ASTIterationStmt &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << "IterationStmt: " << std::endl;
    add_depth();
    node.cond->accept(*this);
    if (node.stmt != nullptr)
        node.stmt->accept(*this);
    remove_depth();
    return nullptr;
}
Value *ASTPrinter::visit(ASTReturnStmt &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << "ReturnStmt: " << std::endl;
    if (node.expression == nullptr)
    {
        std::cout << ": void" << std::endl;
    }
    else
    {
        add_depth();
        node.expression->accept(*this);
        remove_depth();
    }
    return nullptr;
}
Value *ASTPrinter::visit(ASTBreak &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << "Break" << std::endl;
    return nullptr;
}
Value *ASTPrinter::visit(ASTContinue &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << "Continue" << std::endl;
    return nullptr;
}
Value *ASTPrinter::visit(ASTExp &node)
{
    // LOG(DEBUG) << "exp";
    _DEBUT_PRINT_N_(depth);
    std::cout << "expression" << std::endl;
    add_depth();
    if (node.add_exp != nullptr)
        node.add_exp->accept(*this);
    remove_depth();
    return nullptr;
}
Value *ASTPrinter::visit(ASTVar &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << "var: " << node.id << std::endl;
    if (node.length > 0)
    {
        for (auto array : node.array_lists)
        {
            add_depth();
            _DEBUT_PRINT_N_(depth);
            std::cout << "[]" << std::endl;
            array->accept(*this);
            remove_depth();
        }
    }
    return nullptr;
}
Value *ASTPrinter::visit(ASTUnaryExp &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << "UnaryExp: " << std::endl;
    if (node.unary_exp != nullptr)
    {
        _DEBUT_PRINT_N_(depth);
        if (node.unary_op == OP_PLUS)
            std::cout << "+";
        else if (node.unary_op == OP_MINUS)
            std::cout << "-";
        else
            std::cout << "!";
        add_depth();
        node.unary_exp->accept(*this);
        remove_depth();
    }
    else if (node.primary_exp != nullptr)
    {
        add_depth();
        node.primary_exp->accept(*this);
        remove_depth();
    }
    else if (node.func_exp != nullptr)
    {
        add_depth();
        node.func_exp->accept(*this);
        remove_depth();
    }
    else
    {
        _AST_NODE_ERROR_
    }
    return nullptr;
}
Value *ASTPrinter::visit(ASTFuncExp &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << "FuncExp :" << node.id << "()" << std::endl;
    add_depth();
    for (auto arg : node.args)
    {
        arg->accept(*this);
    }
    remove_depth();
    return nullptr;
}
Value *ASTPrinter::visit(ASTAddExp &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << "AddExp: ";
    if (node.add_exp == nullptr)
        std::cout << std::endl;
    else
    {
        std::cout << ": ";
        if (node.op == OP_ADD)
            std::cout << "+";
        else if (node.op == OP_SUB)
            std::cout << "-";
        else
            std::abort();
        std::cout << std::endl;
    }
    add_depth();
    if (node.add_exp != nullptr)
        node.add_exp->accept(*this);
    node.mul_exp->accept(*this);
    remove_depth();
    return nullptr;
}
Value *ASTPrinter::visit(ASTMulExp &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << "MulExp: ";
    if (node.mul_exp == nullptr)
        std::cout << std::endl;
    else
    {
        std::cout << ": ";
        if (node.op == OP_MUL)
            std::cout << "*";
        else if (node.op == OP_DIV)
            std::cout << "/";
        else if (node.op == OP_MOD)
            std::cout << "%";
        else
            std::abort();
        std::cout << std::endl;
    }
    add_depth();
    if (node.mul_exp != nullptr)
        node.mul_exp->accept(*this);
    node.unary_exp->accept(*this);
    remove_depth();
    return nullptr;
}
Value *ASTPrinter::visit(ASTRelExp &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << "RelExp: ";
    if (node.rel_exp == nullptr)
        std::cout << std::endl;
    else
    {
        std::cout << ": ";
        if (node.op == OP_LT)
            std::cout << "<";
        else if (node.op == OP_LE)
            std::cout << "<=";
        else if (node.op == OP_GT)
            std::cout << ">";
        else if (node.op == OP_GE)
            std::cout << ">=";
        else
            std::abort();
        std::cout << std::endl;
    }
    add_depth();
    if (node.rel_exp != nullptr)
        node.rel_exp->accept(*this);
    node.add_exp->accept(*this);
    remove_depth();
    return nullptr;
}
Value *ASTPrinter::visit(ASTEqExp &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << "EqExp: ";
    if (node.eq_exp == nullptr)
        std::cout << std::endl;
    else
    {
        std::cout << ": ";
        if (node.op == OP_EQ)
            std::cout << "==";
        else if (node.op == OP_NEQ)
            std::cout << "!=";
        else
            std::abort();
        std::cout << std::endl;
    }
    add_depth();
    if (node.eq_exp != nullptr)
        node.eq_exp->accept(*this);
    node.rel_exp->accept(*this);
    remove_depth();
    return nullptr;
}
Value *ASTPrinter::visit(ASTAndExp &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << "LAndExp: ";
    if (node.land_exp == nullptr)
        std::cout << std::endl;
    else
    {
        std::cout << ": ";
        if (node.and_op == OP_AND)
            std::cout << "&&";
        else
            std::abort();
        std::cout << std::endl;
    }
    add_depth();
    if (node.land_exp != nullptr)
        node.land_exp->accept(*this);
    node.eq_exp->accept(*this);
    remove_depth();
    return nullptr;
}
Value *ASTPrinter::visit(ASTOrExp &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << "LOrExp: ";
    if (node.lor_exp == nullptr)
        std::cout << std::endl;
    else
    {
        std::cout << ": ";
        if (node.or_op == OP_OR)
            std::cout << "||";
        else
            std::abort();
        std::cout << std::endl;
    }
    add_depth();
    if (node.lor_exp != nullptr)
        node.lor_exp->accept(*this);
    node.land_exp->accept(*this);
    remove_depth();
    return nullptr;
}
Value *ASTPrinter::visit(ASTInitVal &node)
{
    _DEBUT_PRINT_N_(depth);
    std::cout << "InitVal: " << std::endl;
    if (!node.initval_list.empty())
    {
        add_depth();
        _DEBUT_PRINT_N_(depth);
        std::cout << "{" << std::endl;
        for (auto initval : node.initval_list)
        {
            add_depth();
            initval->accept(*this);
            remove_depth();
        }
        _DEBUT_PRINT_N_(depth);
        std::cout << "}" << std::endl;
        remove_depth();
    }
    else
    {
        add_depth();
        if (node.value != nullptr)
            node.value->accept(*this);
        else
        {
            _DEBUT_PRINT_N_(depth);
            std::cout << "{}" << std::endl;
        }
        remove_depth();
    }
    return nullptr;
}


    