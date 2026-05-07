#include <cstring>
#include <iostream>
#include <memory>
#include <stack>
#include <string>

#include "ast.hpp"
#include "syntax_tree.h"
extern void del_syntax_tree(syntax_tree *tree);

//#define DEBUG
#ifdef DEBUG
    #define _DEBUG_(string)                                               \
        std::cout << "[AST] DEBUG: " << string << std::endl << std::flush;
#else
    #define _DEBUG_(string)
#endif

#define _AST_ERROR_(error)                                                      \
    std::cerr << "[AST] ERROR: " << error << std::endl;                         \
    std::abort();

#define _STR_EQ_(a, b) (strcmp((a), (b)) == 0)


void AST::run_visitor(ASTVisitor &visitor) { root->accept(visitor); }

AST::AST(syntax_tree * s) {
    if (s == nullptr) {
        std::cerr << "empty input tree!" << std::endl;
        std::abort();
    }
    auto node = transform_node_iter(s->root);
    del_syntax_tree(s);
    root = std::shared_ptr<ASTProgram>(static_cast<ASTProgram *>(node));
}

ASTNode *AST::transform_node_iter(syntax_tree_node * n) {
    if (_STR_EQ_(n->name, "Program")) {
        // Program  :   CompUnit { $$=node("Program",1,$1); gt->root=$$; }
        // CompUnit :   CompUnit Decl { $$=node("CompUnit",2,$1,$2); }
        //          |   CompUnit FuncDef { $$=node("CompUnit",2,$1,$2); }
        //          |   Decl { $$=node("CompUnit",1,$1); }
        //          |   FuncDef { $$=node("CompUnit",1,$1); }
        _DEBUG_("Program");
        auto node = new ASTProgram();
        // flatten CompUnits
        std::stack<syntax_tree_node *> s;
        auto ptr = n->children[0];
        while (ptr->children_num == 2) {
            s.push(ptr->children[1]);
            ptr = ptr->children[0];
        }
        s.push(ptr->children[0]);
        // create ASTCompUnit for each CompUnit
        while (!s.empty()) {
            auto comp_unit = static_cast<ASTCompUnit *>(transform_node_iter(s.top()));
            node->comp_units.push_back(std::shared_ptr<ASTCompUnit>(comp_unit));
            s.pop();
        }
        _DEBUG_("AST Generated");
        return node;
    } else if (_STR_EQ_(n->name, "Decl")) {
        // Decl :   ConstDecl { $$=node("Decl",1,$1); }
        //      |   VarDecl { $$=node("Decl",1,$1); }
        _DEBUG_("Decl");
        return transform_node_iter(n->children[0]);
    } else if (_STR_EQ_(n->name, "ConstDecl")) {
        // ConstDecl    :   Const BType ConstDefList Semicolon { $$=node("ConstDecl",4,$1,$2,$3,$4); }
        // BType        :   Int { $$=node("Btype",1,$1); }
        //              |   Float { $$=node("Btype",1,$1); }
        // ConstDefList :   ConstDef {$$=node("ConstDefList",1,$1);}
        //              |   ConstDefList Comma ConstDef { $$=node("ConstDefList",3,$1,$2,$3); }
        _DEBUG_("ConstDecl");
        auto node = new ASTDecl();
        node->is_const = true;
        if (_STR_EQ_(n->children[1]->children[0]->name, "int")) {
            node->type = TYPE_INT;
        } else if (_STR_EQ_(n->children[1]->children[0]->name, "float")) {
            node->type = TYPE_FLOAT;
        } else {
            _AST_ERROR_("unknown type in ConstDecl");
        }
        // flatten ConstDeflist
        std::stack<syntax_tree_node *> s;
        auto ptr = n->children[2];
        while (ptr->children_num == 3) {
            s.push(ptr->children[2]);
            ptr = ptr->children[0];
        }
        s.push(ptr->children[0]);
        // create ASTDef for each ConstDef
        while (!s.empty()) {
            auto def = static_cast<ASTDef *>(transform_node_iter(s.top()));
            def->is_const = true;
            def->type = node->type;
            node->defs.push_back(std::shared_ptr<ASTDef>(def));
            s.pop();
        }
        return node;
    } else if (_STR_EQ_(n->name, "VarDecl")) {
        // VarDecl      :   BType VarDefList Semicolon { $$=node("VarDecl",3,$1,$2,$3); }
        // BType        :   Int { $$=node("Btype",1,$1); }
        //              |   Float { $$=node("Btype",1,$1); }
        // VarDefList   :   VarDef { $$=node("VarDefList",1,$1); }
        //              |   VarDefList Comma VarDef { $$=node("VarDefList",3,$1,$2,$3); }
        _DEBUG_("VarDecl");
        auto node = new ASTDecl();
        node->is_const = false;
        if (_STR_EQ_(n->children[0]->children[0]->name, "int")) {
            node->type = TYPE_INT;
        } else if (_STR_EQ_(n->children[0]->children[0]->name, "float")) {
            node->type = TYPE_FLOAT;
        } else {
            _AST_ERROR_("unknown type in VarDecl");
        }
        // flatten VarDefList
        std::stack<syntax_tree_node *> s;
        auto ptr = n->children[1];
        while (ptr->children_num == 3) {
            s.push(ptr->children[2]);
            ptr = ptr->children[0];
        }
        s.push(ptr->children[0]);
        // create ASTDef for each VarDef
        while (!s.empty()) {
            auto def = static_cast<ASTDef *>(transform_node_iter(s.top()));
            def->type = node->type;
            node->defs.push_back(std::shared_ptr<ASTDef>(def));
            s.pop();
        }
        return node;
    } else if (_STR_EQ_(n->name, "FuncDef")) {
        // FuncDef      :   BType Ident LParenthese RParenthese Block { $$=node("FuncDef",5,$1,$2,$3,$4,$5); }
        //              |   Void Ident LParenthese RParenthese Block { $$=node("FuncDef",5,$1,$2,$3,$4,$5); }
        //              |   BType Ident LParenthese FuncFParams RParenthese Block { $$=node("FuncDef",6,$1,$2,$3,$4,$5,$6); }
        //              |   Void Ident LParenthese FuncFParams RParenthese Block { $$=node("FuncDef",6,$1,$2,$3,$4,$5,$6); }
        // BType        :   Int { $$=node("Btype",1,$1); }
        //              |   Float { $$=node("Btype",1,$1); }
        // FuncFParams  :   FuncFParam { $$=node("FuncFParams",1,$1); }
        //              |   FuncFParams Comma FuncFParam { $$=node("FuncFParams",3,$1,$2,$3); }
        _DEBUG_("FuncDef");
        auto node = new ASTFuncDef();
        node->id = n->children[1]->name;
        if (_STR_EQ_(n->children[0]->name, "void")) {
            node->type = TYPE_VOID;
        } else if (_STR_EQ_(n->children[0]->children[0]->name, "int")) {
            node->type = TYPE_INT;
        } else if (_STR_EQ_(n->children[0]->children[0]->name, "float")) {
            node->type = TYPE_FLOAT;
        } else {
            _AST_ERROR_("unknown type in FuncDef");
        }
        syntax_tree_node * ptr = nullptr;
        std::stack<syntax_tree_node *> s;
        if (n->children_num == 6) { // params exist
            // flatten FuncFParams
            ptr = n->children[3];
            while (ptr->children_num == 3) {
                s.push(ptr->children[2]);
                ptr = ptr->children[0];
            }
            s.push(ptr->children[0]);
            // create ASTFuncParam for each FuncFParam
            while (!s.empty()) {
                auto param = static_cast<ASTFuncParam *>(transform_node_iter(s.top()));
                node->params.push_back(std::shared_ptr<ASTFuncParam>(param));
                s.pop();
            }
            ptr = n->children[5]->children[1];
        } else if (n->children_num == 5) { // params not exist
            ptr = n->children[4]->children[1];
        } else {
            _AST_ERROR_("FuncDef has unexpected number of children");
        }
        // flatten BlockItems
        while (ptr->children_num == 2) {
            s.push(ptr->children[1]);
            ptr = ptr->children[0];
        }
        // create ASTBlockItem for each BlockItem
        while (!s.empty()) {
            auto block_item = static_cast<ASTBlockItem *>(transform_node_iter(s.top()));
            if (block_item != nullptr) {
                node->block_items.push_back(std::shared_ptr<ASTBlockItem>(block_item));
            }
            s.pop();
        }
        return node;
    } else if (_STR_EQ_(n->name, "FuncFParam")) {
        // FuncFParam   :   BType Ident { $$=node("FuncFParam",2,$1,$2); }
        //              |   BType Ident LBracket RBracket ExpList { $$=node("FuncFParam",5,$1,$2,$3,$4,$5); }
        // ExpList      :   ExpList LBracket Exp RBracket { $$=node("VarDef",4,$1,$2,$3,$4); }
        //              |   { $$=node("ExpList",0); }
        _DEBUG_("FuncFParam");
        auto node = new ASTFuncParam();
        if (_STR_EQ_(n->children[0]->children[0]->name, "int")) {
            node->type = TYPE_INT;
        } else if (_STR_EQ_(n->children[0]->children[0]->name, "float")) {
            node->type = TYPE_FLOAT;
        } else {
            _AST_ERROR_("unknown type in FuncFParam");
        }
        node->id = n->children[1]->name;
        if (n->children_num > 2) {
            node->is_array = true;
            // flatten ExpList
            std::stack<syntax_tree_node *> s;
            auto ptr = n->children[4];
            while (ptr->children_num == 4) {
                s.push(ptr->children[2]);
                ptr = ptr->children[0];
            }
            // create ASTAddExp for each Exp in ExpList
            while (!s.empty()) {
                auto dim = static_cast<ASTExp *>(transform_node_iter(s.top()));
                node->dims.push_back(std::shared_ptr<ASTExp>(dim));
                s.pop();
            }
        }
        return node;
    } else if (_STR_EQ_(n->name, "ConstDef")) {
        // ConstDef     :   Ident ConstExpList Assign ConstInitVal { $$=node("ConstDef",4,$1,$2,$3,$4); }
        // ConstExpList :   ConstExpList LBracket ConstExp RBracket { $$=node("ConstExpList",4,$1,$2,$3,$4); }
        //              |   { $$=node("ConstExpList",0); }
        auto node = new ASTDef();
        node->is_const = true;
        node->id = n->children[0]->name;
        // flatten ConstExpList
        std::stack<syntax_tree_node *> s;
        auto ptr = n->children[1];
        while (ptr->children_num == 4) {
            s.push(ptr->children[2]);
            ptr = ptr->children[0];
        }
        // create ASTAddExp for each ConstExp in ConstExpList
        while (!s.empty()) {
            auto dim = static_cast<ASTExp *>(transform_node_iter(s.top()));
            node->dims.push_back(std::shared_ptr<ASTExp>(dim));
            s.pop();
        }
        auto init_val = static_cast<ASTInitVal *>(transform_node_iter(n->children[3]));
        node->init_val = std::shared_ptr<ASTInitVal>(init_val);
        return node;
    } else if (_STR_EQ_(n->name, "ConstInitVal")) {
        // ConstInitVal    :   ConstExp { $$=node("ConstInitVal",1,$1); }
        //                 |   LBrace RBrace { $$=node("ConstInitVal",2,$1,$2); }
        //                 |   LBrace ConstInitValList RBrace { $$=node("ConstInitVal",3,$1,$2,$3); }
        //                 ;
        // ConstInitValList:   ConstInitVal { $$=node("ConstInitValList",1,$1); }
        //                 |   ConstInitValList Comma ConstInitVal { $$=node("ConstInitValList",3,$1,$2,$3); }
        auto node = new ASTInitVal();
        node->is_const = true;
        if (n->children_num == 1) { // single ConstExp
            auto exp = static_cast<ASTExp *>(transform_node_iter(n->children[0]));
            node->exp = std::shared_ptr<ASTExp>(exp);
        } else if (n->children_num == 2) { // empty ConstInitValList
            node->is_default = true;
        } else if (n->children_num == 3) { // ConstInitValList
            // flatten ConstInitValList
            std::stack<syntax_tree_node *> s;
            auto ptr = n->children[1];
            while (ptr->children_num == 3) {
                s.push(ptr->children[2]);
                ptr = ptr->children[0];
            }
            s.push(ptr->children[0]);
            // create ASTInitVal for each ConstInitVal in ConstInitValList
            while (!s.empty()) {
                auto init_val = static_cast<ASTInitVal *>(transform_node_iter(s.top()));
                node->level = std::max(node->level, init_val->level+1);
                node->init_vals.push_back(std::shared_ptr<ASTInitVal>(init_val));
                s.pop();
            }
        } else {
            _AST_ERROR_("ConstInitVal has unexpected number of children");
        }
        return node;
    } else if (_STR_EQ_(n->name, "VarDef")) {
        // VarDef       :   Ident ConstExpList { $$=node("VarDef",2,$1,$2); }
        //              |   Ident ConstExpList Assign InitVal { $$=node("VarDef",4,$1,$2,$3,$4); }
        // ConstExpList :   ConstExpList LBracket ConstExp RBracket { $$=node("ConstExpList",4,$1,$2,$3,$4); }
        //              |   { $$=node("ConstExpList",0); }
        auto node = new ASTDef();
        node->is_const = false;
        node->id = n->children[0]->name;
        // flatten ConstExpList
        std::stack<syntax_tree_node *> s;
        auto ptr = n->children[1];
        while (ptr->children_num == 4) {
            s.push(ptr->children[2]);
            ptr = ptr->children[0];
        }
        while (!s.empty()) {
            auto dim = static_cast<ASTAddExp *>(transform_node_iter(s.top()));
            node->dims.push_back(std::shared_ptr<ASTAddExp>(dim));
            s.pop();
        }
        if (n->children_num == 2) { // no init value
            node->init_val = nullptr;
        } else {
            auto init_val = static_cast<ASTInitVal *>(transform_node_iter(n->children[3]));
            node->init_val = std::shared_ptr<ASTInitVal>(init_val);
        }
        return node;
    } else if (_STR_EQ_(n->name, "InitVal")) {
        // InitVal         :   Exp { $$=node("InitVal",1,$1); }
        //                 |   LBrace RBrace { $$=node("InitVal",2,$1,$2); }
        //                 |   LBrace InitValList RBrace { $$=node("InitVal",3,$1,$2,$3); }
        //                 ;
        // InitValList     :   InitVal { $$=node("InitValList",1,$1); }
        //                 |   InitValList Comma InitVal { $$=node("InitValList",3,$1,$2,$3); }
        _DEBUG_("InitVal");
        auto node = new ASTInitVal();
        if (n->children_num == 1) {
            auto exp = static_cast<ASTExp *>(transform_node_iter(n->children[0]));
            node->exp = std::shared_ptr<ASTExp>(exp);
        } else if (n->children_num == 2) { // empty InitValList
            node->is_default = true;
        } else if (n->children_num == 3) {
            // flatten ConstInitValList
            std::stack<syntax_tree_node *> s;
            auto ptr = n->children[1];
            while (ptr->children_num == 3) {
                s.push(ptr->children[2]);
                ptr = ptr->children[0];
            }
            s.push(ptr->children[0]);
            while (!s.empty()) {
                auto init_val = static_cast<ASTInitVal *>(transform_node_iter(s.top()));
                node->level = std::max(node->level, init_val->level+1);
                node->init_vals.push_back(std::shared_ptr<ASTInitVal>(init_val));
                s.pop();
            }
        } else {
            _AST_ERROR_("InitVal has unexpected number of children");
        }
        return node;
    } else if (_STR_EQ_(n->name, "Stmt")) {
        // Stmt :   LVal Assign Exp Semicolon { $$=node("Stmt",4,$1,$2,$3,$4); }
        //      |   Exp Semicolon { $$=node("Stmt",2,$1,$2); }
        //      |   Semicolon { $$=node("Stmt",1,$1); }
        //      |   Block { $$=node("Stmt",1,$1); }
        //      |   If LParenthese Cond RParenthese Stmt %prec Epsilon { $$=node("Stmt",5,$1,$2,$3,$4,$5); }
        //      |   If LParenthese Cond RParenthese Stmt Else Stmt { $$=node("Stmt",7,$1,$2,$3,$4,$5,$6,$7); }
        //      |   While LParenthese Cond RParenthese Stmt { $$=node("Stmt",5,$1,$2,$3,$4,$5); }
        //      |   Break Semicolon { $$=node("Stmt",2,$1,$2); }
        //      |   Continue Semicolon { $$=node("Stmt",2,$1,$2); }
        //      |   Return Semicolon { $$=node("Stmt",2,$1,$2); }
        //      |   Return Exp Semicolon { $$=node("Stmt",3,$1,$2,$3); }
        if (n->children_num == 1) { // ASTBlockStmt or empty Stmt
            if (_STR_EQ_(n->children[0]->name, "Block")) {
                return transform_node_iter(n->children[0]);
            } else if (_STR_EQ_(n->children[0]->name, ";")) {
                _DEBUG_("EmptyStmt");
                return nullptr;
            } else {
                _AST_ERROR_("Stmt has unexpected child");
            }
        } else if (n->children_num == 4) { // ASTAssignStmt
            _DEBUG_("AssignStmt");
            auto node = new ASTAssignStmt();
            auto lval = static_cast<ASTLVal *>(transform_node_iter(n->children[0]));
            lval->is_lval = true;
            node->lval = std::shared_ptr<ASTLVal>(lval);

            auto exp = static_cast<ASTExp *>(transform_node_iter(n->children[2]));
            node->exp = std::shared_ptr<ASTExp>(exp);
            return node;
        } else if (_STR_EQ_(n->children[0]->name, "break")) { // ASTBreakStmt
            _DEBUG_("BreakStmt");
            auto node = new ASTBreakStmt();
            return node;
        } else if (_STR_EQ_(n->children[0]->name, "continue")) { // ASTContinueStmt
            _DEBUG_("ContinueStmt");
            auto node = new ASTContinueStmt();
            return node;
        } else if (_STR_EQ_(n->children[0]->name, "while")) { // ASTIterationStmt
            _DEBUG_("IterationStmt");
            auto node = new ASTIterationStmt();
            auto cond = static_cast<ASTCond *>(transform_node_iter(n->children[2]));
            node->cond = std::shared_ptr<ASTCond>(cond);

            auto stmt = static_cast<ASTStmt *>(transform_node_iter(n->children[4]));
            node->stmt = std::shared_ptr<ASTStmt>(stmt);

            return node;
        } 
        else if(_STR_EQ_(n->children[0]->name, "Exp")) {
            auto node = new ASTExpStmt();
            _DEBUG_("ExpStmt");
            auto exp = static_cast<ASTExp *>(transform_node_iter(n->children[0]));
            node->exp = std::shared_ptr<ASTExp>(exp);
            return node; // ASTExpStmt
        }
        else if (_STR_EQ_(n->children[0]->name, "if")) { // ASTSelectionStmt
            _DEBUG_("SelectionStmt");
            auto node = new ASTSelectionStmt();
            auto cond = static_cast<ASTCond *>(transform_node_iter(n->children[2]));
            node->cond = std::shared_ptr<ASTCond>(cond);

            auto if_stmt = static_cast<ASTStmt *>(transform_node_iter(n->children[4]));
            if (if_stmt != nullptr) {
                node->if_stmt = std::shared_ptr<ASTStmt>(if_stmt);
            } else {
                node->if_stmt = nullptr;
            }
            if (n->children_num == 7) {
                auto else_stmt = static_cast<ASTStmt *>(transform_node_iter(n->children[6]));
                node->else_stmt = std::shared_ptr<ASTStmt>(else_stmt);
            } else {
                node->else_stmt = nullptr;
            }

            return node;
        } else if (_STR_EQ_(n->children[0]->name, "return")) { // ASTReturnStmt
            _DEBUG_("ReturnStmt");
            auto node = new ASTReturnStmt();
            if (n->children_num == 3) {
                auto exp = static_cast<ASTExp *>(transform_node_iter(n->children[1]));
                node->exp = std::shared_ptr<ASTExp>(exp);
            } else if (n->children_num == 2) {
                node->exp = nullptr; // empty return
            } else {
                _AST_ERROR_("ReturnStmt has unexpected number of children");
            }
            return node;
        }
    } else if (_STR_EQ_(n->name, "Block")) {
        _DEBUG_("BlockStmt");
        auto node = new ASTBlockStmt();
        // flatten BlockItems
        std::stack<syntax_tree_node *> s;
        auto ptr = n->children[1];
        while (ptr->children_num == 2) {
            s.push(ptr->children[1]);
            ptr = ptr->children[0];
        }
        // create ASTBlockItem for each BlockItem
        while (!s.empty()) {
            auto block_item = static_cast<ASTBlockItem *>(transform_node_iter(s.top()));
            s.pop();
            if (block_item == nullptr) {
                continue;
            }
            auto block_item_shared = std::shared_ptr<ASTBlockItem>(block_item);
            node->block_items.push_back(block_item_shared);
        }
        return node;
    } else if (_STR_EQ_(n->name, "BlockItem")) {
        auto node = new ASTBlockItem();
        if (_STR_EQ_(n->children[0]->name, "Decl")) {
            node->is_decl = true;
            auto decl = static_cast<ASTDecl *>(transform_node_iter(n->children[0]));
            node->item = std::shared_ptr<ASTDecl>(decl);
        } else if (_STR_EQ_(n->children[0]->name, "Stmt")) {
            node->is_decl = false;
            auto stmt = static_cast<ASTStmt *>(transform_node_iter(n->children[0]));
            if (stmt == nullptr) {
                delete node;
                return nullptr;
            }
            node->item = std::shared_ptr<ASTStmt>(stmt);
        }
        return node;
    } else if (_STR_EQ_(n->name, "Exp") || _STR_EQ_(n->name, "ConstExp")) {
        // Exp -> AddExp
        // ConstExp -> AddExp
        _DEBUG_("Exp/ConstExp");
        return transform_node_iter(n->children[0]);
    } else if (_STR_EQ_(n->name, "Cond")) {
        // Cond -> LOrExp
        _DEBUG_("Cond");
        return transform_node_iter(n->children[0]);
    } else if (_STR_EQ_(n->name, "LOrExp")) {
        // LOrExp -> LAndExp || LAndExp || LAndExp ……
        _DEBUG_("LOrExp");
        auto node = new ASTLOrExp();
        std::stack<syntax_tree_node *> s;
        auto ptr = n;
        while (ptr->children_num == 3) {
            s.push(ptr->children[2]);
            ptr = ptr->children[0];
        }
        s.push(ptr->children[0]);
        while (!s.empty()) {
            // 顺序貌似反了，没反
            auto and_exp = static_cast<ASTLAndExp *>(transform_node_iter(s.top()));
            node->and_exps.push_back(std::shared_ptr<ASTLAndExp>(and_exp));
            s.pop();
        } 
        return node;
    } else if (_STR_EQ_(n->name, "LAndExp")) {
        _DEBUG_("LAndExp");
        auto node = new ASTLAndExp();
        std::stack<syntax_tree_node *> s;
        auto ptr = n;
        while (ptr->children_num == 3) {
            s.push(ptr->children[2]);
            ptr = ptr->children[0];
        }
        s.push(ptr->children[0]);
        while (!s.empty()) {
            auto eq_exp = static_cast<ASTEqExp *>(transform_node_iter(s.top()));
            node->eq_exps.push_back(std::shared_ptr<ASTEqExp>(eq_exp));
            s.pop();
        }
        return node;
    } else if (_STR_EQ_(n->name, "EqExp")) {
        _DEBUG_("EqExp");
        auto node = new ASTEqExp();
        std::stack<std::pair<EqOp, syntax_tree_node *>> s;
        auto ptr = n;
        while (ptr->children_num == 3) {
            auto op_name = ptr->children[1]->name;
            if (_STR_EQ_(op_name, "==")) {
                s.push({OP_EQ, ptr->children[2]});
            }
            else if (_STR_EQ_(op_name, "!=")) {
                s.push({OP_NEQ, ptr->children[2]});
            }
            ptr = ptr->children[0];
        }
        auto rel_exp = static_cast<ASTRelExp *>(transform_node_iter(ptr->children[0]));
        node->left = std::shared_ptr<ASTRelExp>(rel_exp);
        while (!s.empty()) {
            auto pair = s.top();
            auto right = static_cast<ASTRelExp *>(transform_node_iter(pair.second));
            node->right.push_back({pair.first, std::shared_ptr<ASTRelExp>(right)});
            s.pop();
        }
        return node;
    } else if (_STR_EQ_(n->name, "RelExp")) {
        _DEBUG_("RelExp");
        auto node = new ASTRelExp();
        std::stack<std::pair<RelOp, syntax_tree_node *>> s;
        auto ptr = n;
        while (ptr->children_num == 3) {
            auto op_name = ptr->children[1]->name;
            if (_STR_EQ_(op_name, "<="))
                s.push({OP_LE,ptr->children[2]});
            else if (_STR_EQ_(op_name, "<"))
                s.push({OP_LT,ptr->children[2]});
            else if (_STR_EQ_(op_name, ">"))
                s.push({OP_GT,ptr->children[2]});
            else if (_STR_EQ_(op_name, ">="))
                s.push({OP_GE,ptr->children[2]});
            ptr = ptr->children[0];
        }
        auto left = static_cast<ASTAddExp *>(transform_node_iter(ptr->children[0]));
        node->left = std::shared_ptr<ASTAddExp>(left);
        while (!s.empty()) {
            auto pair = s.top();
            auto right = static_cast<ASTAddExp *>(transform_node_iter(pair.second));
            node->right.push_back({pair.first, std::shared_ptr<ASTAddExp>(right)});
            s.pop();
        }
        return node;
    } else if (_STR_EQ_(n->name, "AddExp")) {
        _DEBUG_("AddExp");
        auto node = new ASTAddExp();
        std::stack<std::pair<AddOp, syntax_tree_node *>> s;
        auto ptr = n;
        while (ptr->children_num == 3) {
            auto op_name = ptr->children[1]->name;
            if (_STR_EQ_(op_name, "+"))
                s.push({OP_PLUS, ptr->children[2]});
            else if (_STR_EQ_(op_name, "-"))
                s.push({OP_MINUS, ptr->children[2]});
            ptr = ptr->children[0];
        }
        auto left = static_cast<ASTMulExp *>(transform_node_iter(ptr->children[0]));
        node->left = std::shared_ptr<ASTMulExp>(left);
        while (!s.empty()) {
            auto pair = s.top();
            auto right = static_cast<ASTMulExp *>(transform_node_iter(pair.second));
            node->right.push_back({pair.first, std::shared_ptr<ASTMulExp>(right)});
            s.pop();
        }
        return node;
    } else if (_STR_EQ_(n->name, "MulExp")) {
        _DEBUG_("MulExp");
        auto node = new ASTMulExp();
        std::stack<std::pair<MulOp, syntax_tree_node *>> s;
        auto ptr = n;
        while (ptr->children_num == 3) {
            auto op_name = ptr->children[1]->name;
            if (_STR_EQ_(op_name, "*"))
                s.push({OP_MUL, ptr->children[2]});
            else if (_STR_EQ_(op_name, "/"))
                s.push({OP_DIV, ptr->children[2]});
            else if (_STR_EQ_(op_name, "%"))
                s.push({OP_MOD, ptr->children[2]});
            else {
                _AST_ERROR_(op_name);
            }
            ptr = ptr->children[0];
        }
        auto left = static_cast<ASTUnaryExp *>(transform_node_iter(ptr->children[0]));
        node->left = std::shared_ptr<ASTUnaryExp>(left);
        while (!s.empty()) {
            auto pair = s.top();
            auto right = static_cast<ASTUnaryExp *>(transform_node_iter(pair.second));
            node->right.push_back({pair.first, std::shared_ptr<ASTUnaryExp>(right)});
            s.pop();
        }
        return node;
    } else if (_STR_EQ_(n->name, "UnaryExp")) {
        _DEBUG_("UnaryExp");
        auto node = new ASTUnaryExp();
        auto ptr = n;
        // 处理 UnaryOp 的情况
        while (ptr->children_num == 2) {
            // UnaryOp UnaryExp
            auto op_name = ptr->children[0]->children[0]->name;
            if (_STR_EQ_(op_name, "!")) {
                node->ops.push_back(OP_NOT);
            } else if (_STR_EQ_(op_name, "-")){
                node->ops.push_back(OP_NEG);
            } else if (_STR_EQ_(op_name, "+")){
                node->ops.push_back(OP_POS);
            } else {
                _AST_ERROR_(op_name);
            }
            ptr = ptr->children[1];
        }
        if (ptr->children_num == 1) {
            // PrimaryExp
            auto primary_exp = static_cast<ASTPrimaryExp *>(transform_node_iter(ptr->children[0]));
            node->primary_exp = std::shared_ptr<ASTPrimaryExp>(primary_exp);
        } else if (ptr->children_num == 3) {
            // Ident '(' ')'
            auto call = new ASTCall();
            call->id = ptr->children[0]->name;
            node->primary_exp = std::shared_ptr<ASTPrimaryExp>(call);
        } else if (ptr->children_num == 4) {
            // LVal '(' FuncRParams ')'
            auto call = new ASTCall();
            call->id = ptr->children[0]->name;
            // FuncRParams
            auto ptr_ptr = ptr->children[2];
            std::stack<syntax_tree_node *> s;
            while (ptr_ptr->children_num == 3) {
                s.push(ptr_ptr->children[2]);
                ptr_ptr = ptr_ptr->children[0];
            }
            s.push(ptr_ptr->children[0]);
            while (!s.empty()) {
                auto exp_node = static_cast<ASTExp *>(transform_node_iter(s.top()));
                call->params.push_back(std::shared_ptr<ASTExp>(exp_node));
                s.pop();
            }
            node->primary_exp = std::shared_ptr<ASTPrimaryExp>(call);
        } else {
            _AST_ERROR_("");
        }
        return node;
     } else if (_STR_EQ_(n->name, "PrimaryExp")) {
        // 共有四种情况
        // LParenthese Exp RParenthese { $$=node("PrimaryExp",3,$1,$2,$3); }
        // LVal { $$=node("PrimaryExp",1,$1); }
        // IntNum { $$=node("PrimaryExp",1,$1); }
        // FloatNum { $$=node("PrimaryExp",1,$1); }
        _DEBUG_("PrimaryExp");
        if (n->children_num == 1 && _STR_EQ_(n->children[0]->name, "LVal")) {
            return transform_node_iter(n->children[0]);
        } else if (n->children_num == 1 && _STR_EQ_(n->children[0]->name, "DecIntNum")) {
            // DecIntNum
            _DEBUG_("DecIntNum");
            auto node = new ASTNum();
            node->type = TYPE_INT;
            node->i_val = std::stoi(n->children[0]->children[0]->name, nullptr, 10);
            return node;
        } else if (n->children_num == 1 && _STR_EQ_(n->children[0]->name, "OctIntNum")) {
             // OctIntNum
            _DEBUG_("OctIntNum");
            auto node = new ASTNum();
            node->type = TYPE_INT;
            node->i_val = std::stoi(n->children[0]->children[0]->name, nullptr, 8);
            return node;
        } else if (n->children_num == 1 && _STR_EQ_(n->children[0]->name, "HexIntNum")) {
            // HexIntNum
            _DEBUG_("HexIntNum");
            auto node = new ASTNum();
            node->type = TYPE_INT;
            node->i_val = std::stoi(n->children[0]->children[0]->name, nullptr, 16);
            return node;
        } else if (n->children_num == 1 && (_STR_EQ_(n->children[0]->name, "DecFloatNum") || _STR_EQ_(n->children[0]->name, "HexFloatNum"))) {
            _DEBUG_("DecFloatNum/HexFloatNum");
            // DecFloatNum/HexFloatNum
            auto node = new ASTNum();
            node->type = TYPE_FLOAT;
            node->f_val = std::stof(n->children[0]->children[0]->name);
            return node;
        } else if (n->children_num == 3) {
            // (Exp)
            auto node = new ASTParenthesizedExp();
            auto exp_node = static_cast<ASTExp *>(transform_node_iter(n->children[1]));
            node->exp = std::shared_ptr<ASTExp>(exp_node);
            return node;
        } else {
            _AST_ERROR_("");
        }
    } else if (_STR_EQ_(n->name, "LVal")) {
        // LVal -> Ident
        _DEBUG_("LVal");
        auto node = new ASTLVal();
        node->id = n->children[0]->name;
        // 可能是数组名
        auto ptr = n->children[1];
        std::stack<syntax_tree_node *> s;
        while (ptr->children_num == 4) {
            s.push(ptr->children[2]);
            ptr = ptr->children[0];
        }
        while (!s.empty()) {
            auto index = static_cast<ASTExp *>(transform_node_iter(s.top()));
            node->indices.push_back(std::shared_ptr<ASTExp>(index));
            s.pop();
        }
        return node;
    }
    else {
        _AST_ERROR_("unknown ASTNode type: " + std::string(n->name));
    }
    return nullptr;
}

Value * ASTProgram::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTAddExp::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTAssignStmt::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTBlockItem::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTBlockStmt::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTBreakStmt::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTCall::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTContinueStmt::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTDecl::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTDef::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTEqExp::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTFuncDef::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTFuncParam::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTInitVal::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTIterationStmt::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTLAndExp::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTLOrExp::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTLVal::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTMulExp::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTNum::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTParenthesizedExp::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTRelExp::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTReturnStmt::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTSelectionStmt::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTUnaryExp::accept(ASTVisitor & visitor) { return visitor.visit(*this); }
Value * ASTExpStmt::accept(ASTVisitor & visitor) { return visitor.visit(*this); }

#define _DEBUG_N_(N)                                                     \
    std::cout << std::string(N, '-');

Value * ASTPrinter::visit(ASTProgram & node) {
    _DEBUG_N_(depth);
    std::cout << "Program" << std::endl;
    add_depth();
    for (auto comp_unit : node.comp_units) {
        comp_unit->accept(*this);
    }
    remove_depth();
    return nullptr;
}
Value * ASTPrinter::visit(ASTAddExp & node) {
    _DEBUG_N_(depth);
    std::cout << "AddExp" << std::endl;
    add_depth();
    node.left->accept(*this);
    for (auto item : node.right) {
        _DEBUG_N_(depth);
        std::cout << (item.first == OP_PLUS ? "+" : "-") << std::endl;
        item.second->accept(*this);
    }
    remove_depth();
    return nullptr;
}
Value * ASTPrinter::visit(ASTAssignStmt & node) {
    _DEBUG_N_(depth);
    std::cout << "AssignStmt" << std::endl;
    add_depth();
    node.lval->accept(*this);
    node.exp->accept(*this);
    remove_depth();
    return nullptr;
}
Value * ASTPrinter::visit(ASTBlockItem & node) {
    _DEBUG_N_(depth);
    std::cout << "BlockItem" << std::endl;
    add_depth();
    if (node.is_decl) {
        auto decl = std::get<std::shared_ptr<ASTDecl>>(node.item);
        decl->accept(*this);
    } else {
        auto stmt = std::get<std::shared_ptr<ASTStmt>>(node.item);
        stmt->accept(*this);
    }
    remove_depth();
    return nullptr;
}
Value * ASTPrinter::visit(ASTBlockStmt & node) {
    _DEBUG_N_(depth);
    std::cout << "BlockStmt" << std::endl;
    add_depth();
    for (auto block_item : node.block_items) {
        block_item->accept(*this);
    }
    remove_depth();
    return nullptr;
}
Value * ASTPrinter::visit(ASTBreakStmt & node) {
    _DEBUG_N_(depth);
    std::cout << "BreakStmt : break" << std::endl;
    return nullptr;
}
Value * ASTPrinter::visit(ASTCall & node) {
    _DEBUG_N_(depth);
    std::cout << "Call: " << node.id << std::endl;
    add_depth();
    if (node.params.size() != 0) {
        std::cout << "Function parameters:" << std::endl;
        for (const auto &param : node.params) {
            param->accept(*this);
        }
    } else {
        std::cout << "No parameters" << std::endl;
    }
    remove_depth();
    return nullptr;
}
Value * ASTPrinter::visit(ASTContinueStmt & node) {
    _DEBUG_N_(depth);
    std::cout << "ContinueStmt : continue" << std::endl;
    return nullptr;
}
Value * ASTPrinter::visit(ASTDecl & node) {
    _DEBUG_N_(depth);
    std::string type = (node.type == TYPE_INT) ? "int" : "float";
    if (node.is_const) {
        std::cout << "ConstDecl: " << type << std::endl;
    } else {
        std::cout << "VarDecl: " << type << std::endl;
    }
    add_depth();
    for (auto def : node.defs) {
        def->accept(*this);
    }
    remove_depth();
    return nullptr;

}
Value * ASTPrinter::visit(ASTDef & node) {
    _DEBUG_N_(depth);
    std::cout << (node.is_const ? "ConstDef: " : "Def: ") << node.id << std::endl;
    add_depth();
    if (node.dims.size() != 0) {
        _DEBUG_N_(depth);
        std::cout << "Dims: " << std::endl;
        for (auto dim : node.dims) {
            dim->accept(*this);
        }
    }
    remove_depth();
    if (node.init_val) {
        _DEBUG_N_(depth);
        std::cout << "InitVal: " << std::endl;
        add_depth();
        node.init_val->accept(*this);
        remove_depth();
    }
    return nullptr;
}
Value * ASTPrinter::visit(ASTEqExp & node) {
    _DEBUG_N_(depth);
    std::cout << "EqExp" << std::endl;
    add_depth();
    node.left->accept(*this);
    for (const auto &item : node.right) {
        std::cout << item.first;
        item.second->accept(*this);
    }
    remove_depth();
    return nullptr;
}
Value * ASTPrinter::visit(ASTFuncDef & node) {
    _DEBUG_N_(depth);
    std::cout << "FuncDef: " << std::endl;
    add_depth();
    for (auto param : node.params) {
        param->accept(*this);
    }
    for (auto block_item : node.block_items) {
        block_item->accept(*this);
    }
    remove_depth();
    return nullptr;
}
Value * ASTPrinter::visit(ASTFuncParam & node) {
    _DEBUG_N_(depth);
    std::string type = (node.type == TYPE_INT) ? "int" : "float";
    std::cout << "FuncParam: " << type << " " << node.id << std::endl;
    if (node.is_array) {
        add_depth();
        _DEBUG_N_(depth);
        std::cout << "[]" << std::endl;
        for (auto dim : node.dims) {
            _DEBUG_N_(depth);
            std::cout << "[" << std::endl;
            dim->accept(*this);
            _DEBUG_N_(depth);
            std::cout << "]" << std::endl;
        }
        remove_depth();
    }
    return nullptr;
}
Value * ASTPrinter::visit(ASTInitVal & node) {
    if (node.is_default) {
        _DEBUG_N_(depth);
        std::cout << "Default InitVal" << std::endl;
    } else if (node.exp) {
        _DEBUG_N_(depth);
        std::cout << "InitVal with Exp" << std::endl;
        add_depth();
        node.exp->accept(*this);
        remove_depth();
    } else {
        _DEBUG_N_(depth);
        std::cout << "{" << std::endl;
        add_depth();
        for (auto init_val : node.init_vals) {
            init_val->accept(*this);
        }
        remove_depth();
        _DEBUG_N_(depth);
        std::cout << "}" << std::endl;
    }
    return nullptr;
}
Value * ASTPrinter::visit(ASTIterationStmt & node) {
    _DEBUG_N_(depth);
    std::cout << "IterationStmt" << std::endl;
    add_depth();
    node.cond->accept(*this);
    node.stmt->accept(*this);
    remove_depth();
    return nullptr;
}
Value * ASTPrinter::visit(ASTLAndExp & node) {
    _DEBUG_N_(depth);
    std::cout << "LAndExp" << std::endl;
    add_depth();
    for (const auto &item : node.eq_exps) {
        item->accept(*this);
    }
    remove_depth();
    return nullptr;
}
Value * ASTPrinter::visit(ASTLOrExp & node) {
    _DEBUG_N_(depth);
    std::cout << "LOrExp" << std::endl;
    add_depth();
    for (const auto &item : node.and_exps) {
        item->accept(*this);
    }
    remove_depth();
    return nullptr;
}
Value * ASTPrinter::visit(ASTLVal & node) {
    _DEBUG_N_(depth);
    std::cout << "LVal: " << node.id << std::endl;
    add_depth();
    if (node.indices.size() != 0) {
        for (const auto &exp : node.indices) {
            exp->accept(*this);
        }
    }
    remove_depth();
    return nullptr;
}
Value * ASTPrinter::visit(ASTMulExp & node) {
    _DEBUG_N_(depth);
    std::cout << "MulExp" << std::endl;
    add_depth();
    node.left->accept(*this);
    for (const auto &item : node.right) {
        std::cout << item.first;
        item.second->accept(*this);
    }
    remove_depth();
    return nullptr;
}
Value * ASTPrinter::visit(ASTNum & node) {
    _DEBUG_N_(depth);
    std::cout << "Num: " << (node.type == TYPE_INT ? node.i_val : node.f_val) << std::endl;
    return nullptr; // No further processing needed for numbers
}
Value * ASTPrinter::visit(ASTParenthesizedExp & node) {
    _DEBUG_N_(depth);
    std::cout << "ParenthesizedExp" << std::endl;
    add_depth();
    node.exp->accept(*this);
    remove_depth();
    return nullptr;
}
Value * ASTPrinter::visit(ASTRelExp & node) {
    _DEBUG_N_(depth);
    std::cout << "RelExp" << std::endl;
    add_depth();
    node.left->accept(*this);
    for (const auto &item : node.right) {
        std::cout << item.first;
        item.second->accept(*this);
    }
    remove_depth();
    return nullptr;
}
Value * ASTPrinter::visit(ASTReturnStmt & node) {
    _DEBUG_N_(depth);
    std::cout << "ReturnStmt" << std::endl;
    add_depth();
    if (node.exp != nullptr) {
        node.exp->accept(*this);
    } else {
        _DEBUG_N_(depth);
        std::cout << "void" << std::endl;
    }
    remove_depth();
    return nullptr;
}
Value * ASTPrinter::visit(ASTSelectionStmt & node) {
    _DEBUG_N_(depth);
    std::cout << "SelectionStmt" << std::endl;
    add_depth();
    node.cond->accept(*this);
    if (node.if_stmt != nullptr) {
        node.if_stmt->accept(*this);
    }
    if(node.else_stmt != nullptr) {
        node.else_stmt->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value * ASTPrinter::visit(ASTUnaryExp & node) {
    _DEBUG_N_(depth);
    std::cout << "UnaryExp" << std::endl;
    add_depth();
    _DEBUG_N_(depth);
    for (auto op : node.ops) {
        std::cout << (op == OP_NOT ? "!" : (op == OP_NEG ? "-" : "+")) ;
    }
    std::cout << std::endl;
    node.primary_exp->accept(*this);
    remove_depth();
    return nullptr;
}

Value * ASTPrinter::visit(ASTExpStmt & node) {
    _DEBUG_N_(depth);
    std::cout << "ExpStmt" << std::endl;
    add_depth();
    node.exp->accept(*this);
    remove_depth();
    return nullptr;
}