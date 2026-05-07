#include "ast.hpp"

#include <cstring>
#include <iostream>
#include <stack>

#define _AST_NODE_ERROR_                                                       \
  std::cerr << "Abort due to node cast error."                                 \
               "Contact with TAs to solve your problem."                       \
            << std::endl;                                                      \
  std::abort();

#define _STR_EQ(a, b) (strcmp((a), (b)) == 0)

void AST::run_visitor(ASTVisitor &visitor) { root->accept(visitor); }

AST::AST(syntax_tree *s) {
  if (s == nullptr) {
    std::cerr << "empty input tree!" << std::endl;
    std::abort();
  }
  if (s->root == nullptr) {
    std::cerr << "empty root node!" << std::endl;
    std::abort();
  }
  auto node = transform_node_iter(s->root);
  if (node == nullptr) {
    std::cerr << "failed to transform syntax tree!" << std::endl;
    std::abort();
  }
  del_syntax_tree(s);
  root = std::shared_ptr<ASTProgram>(static_cast<ASTProgram *>(node));
}

ASTNode *AST::transform_node_iter(syntax_tree_node *n) {
  // 检查空指针
  if (n == nullptr) {
    std::cerr << "Error: null syntax tree node!" << std::endl;
    return nullptr;
  }
  
  if (n->name == nullptr) {
    std::cerr << "Error: syntax tree node has null name!" << std::endl;
    return nullptr;
  }
  
  // 调试输出 - 显示正在处理的节点
  // std::cerr << "Processing node: " << n->name << " with " << n->children_num << " children" << std::endl;
  
  // 首先处理 epsilon 节点 - 放在最前面以确保被处理
  if (_STR_EQ(n->name, "epsilon")) {
    // epsilon 节点表示空，直接返回 nullptr
    return nullptr;
  }
  
  // Program
  if (_STR_EQ(n->name, "program")) {
    auto node = new ASTProgram();
    
    // 检查是否有子节点
    if (n->children_num == 0 || !n->children[0]) {
      std::cerr << "Error: program node has no children!" << std::endl;
      return node;
    }
    
    // 展开CompUnit列表
    std::stack<ASTCompUnit *> s;
    auto p = n->children[0];
    while (p && _STR_EQ(p->name, "CompUnit") && p->children_num == 2) {
      // 使用dynamic_cast替代static_cast，并添加错误检查
      auto compUnit = dynamic_cast<ASTCompUnit *>(transform_node_iter(p->children[1]));
      if (!compUnit) {
        std::cerr << "Failed to cast to ASTCompUnit" << std::endl;
        std::abort();
      }
      s.push(compUnit);
      p = p->children[0];
    }
    auto compUnit = dynamic_cast<ASTCompUnit *>(transform_node_iter(p));
    if (!compUnit) {
      std::cerr << "Failed to cast to ASTCompUnit" << std::endl;
      std::abort();
    }
    s.push(compUnit);
    
    while (!s.empty()) {
      node->compUnits.push_back(std::shared_ptr<ASTCompUnit>(s.top()));
      s.pop();
    }
    return node;
  }
  // CompUnit
  else if (_STR_EQ(n->name, "CompUnit")) {
    if (n->children_num == 1) {
      return transform_node_iter(n->children[0]);
    } else {
      // 递归情况已在Program中处理
      std::cerr << "Unexpected CompUnit structure" << std::endl;
      return nullptr;
    }
  }
  // Decl
  else if (_STR_EQ(n->name, "Decl")) {
    return transform_node_iter(n->children[0]);
  }
  // ConstDecl
  else if (_STR_EQ(n->name, "ConstDecl")) {
    auto node = new ASTConstDecl();
    // CONST BType ConstDefList SEMICOLON
    if (n->children_num >= 4 && n->children[1] && n->children[1]->children_num > 0 && 
        n->children[1]->children[0] && n->children[1]->children[0]->name) {
      if (_STR_EQ(n->children[1]->children[0]->name, "int")) {
        node->bType = TYPE_INT;
      } else {
        node->bType = TYPE_FLOAT;
      }
    } else {
      node->bType = TYPE_INT; // 默认类型
    }
    
    // 展开ConstDefList
    auto defList = n->children[2];
    std::stack<ASTConstDef *> s;
    while (defList && _STR_EQ(defList->name, "ConstDefList") && defList->children_num == 3) {
      auto constDef = static_cast<ASTConstDef *>(transform_node_iter(defList->children[2]));
      if (constDef) {
        s.push(constDef);
      }
      defList = defList->children[0];
    }
    if (defList && defList->children[0]) {
      auto constDef = static_cast<ASTConstDef *>(transform_node_iter(defList->children[0]));
      if (constDef) {
        s.push(constDef);
      }
    }
    
    while (!s.empty()) {
      node->constDefs.push_back(std::shared_ptr<ASTConstDef>(s.top()));
      s.pop();
    }
    return node;  // 由于使用了虚继承，可以直接返回
  }
  // BType
  else if (_STR_EQ(n->name, "BType")) {
    // 不需要单独处理，在父节点中直接访问
    return nullptr;
  }
  // ConstDef
  else if (_STR_EQ(n->name, "ConstDef")) {
    auto node = new ASTConstDef();
    // IDENT ConstExpList ASSIGN ConstInitVal
    if (n->children_num >= 4 && n->children[0] && n->children[0]->name) {
      node->ident = n->children[0]->name;
    }
    
    // 处理数组维度
    if (n->children_num >= 2 && n->children[1]) {
      auto expList = n->children[1];
      if (expList && expList->children_num > 0 && expList->children[0] && !_STR_EQ(expList->children[0]->name, "epsilon")) {
        while (expList && expList->children_num == 4) {
          auto constExp = static_cast<ASTConstExp *>(transform_node_iter(expList->children[2]));
          if (constExp) {
            node->dims.push_back(std::shared_ptr<ASTConstExp>(constExp));
          }
          expList = expList->children[0];
        }
      }
    }
    
    if (n->children_num >= 4 && n->children[3]) {
      node->constInitVal = std::shared_ptr<ASTConstInitVal>(
        static_cast<ASTConstInitVal *>(transform_node_iter(n->children[3])));
    }
    return node;
  }
  // ConstInitVal
  else if (_STR_EQ(n->name, "ConstInitVal")) {
    auto node = new ASTConstInitVal();
    if (n->children_num == 1) {
      // ConstExp
      node->constExp = std::shared_ptr<ASTConstExp>(
        static_cast<ASTConstExp *>(transform_node_iter(n->children[0])));
    } else if (n->children_num == 2) {
      // LBRACE RBRACE
      // 空数组初始化
    } else {
      // LBRACE ConstInitValList RBRACE
      auto valList = n->children[1];
      std::stack<ASTConstInitVal *> s;
      while (valList && _STR_EQ(valList->name, "ConstInitValList") && valList->children_num == 3) {
        auto val = static_cast<ASTConstInitVal *>(transform_node_iter(valList->children[2]));
        if (val) {
          s.push(val);
        }
        valList = valList->children[0];
      }
      if (valList && valList->children[0]) {
        auto val = static_cast<ASTConstInitVal *>(transform_node_iter(valList->children[0]));
        if (val) {
          s.push(val);
        }
      }
      
      while (!s.empty()) {
        node->constInitVals.push_back(std::shared_ptr<ASTConstInitVal>(s.top()));
        s.pop();
      }
    }
    return node;
  }
  // VarDecl
  else if (_STR_EQ(n->name, "VarDecl")) {
    auto node = new ASTVarDecl();
    // BType VarDef VarDeclList SEMICOLON
    if (n->children_num >= 4 && n->children[0] && n->children[0]->children_num > 0 && 
        n->children[0]->children[0] && n->children[0]->children[0]->name) {
      if (_STR_EQ(n->children[0]->children[0]->name, "int")) {
        node->bType = TYPE_INT;
      } else {
        node->bType = TYPE_FLOAT;
      }
    } else {
      node->bType = TYPE_INT; // 默认类型
    }
    
    // 添加第一个VarDef
    if (n->children_num >= 2 && n->children[1]) {
      auto varDef = static_cast<ASTVarDef *>(transform_node_iter(n->children[1]));
      if (varDef) {
        node->varDefs.push_back(std::shared_ptr<ASTVarDef>(varDef));
      }
    }
    
    // 处理VarDeclList
    auto declList = n->children[2];
    if (declList && declList->children_num > 0 && declList->children[0] && !_STR_EQ(declList->children[0]->name, "epsilon")) {
      std::stack<ASTVarDef *> s;
      while (declList && _STR_EQ(declList->name, "VarDeclList") && declList->children_num == 3) {
        auto varDef = static_cast<ASTVarDef *>(transform_node_iter(declList->children[2]));
        if (varDef) {
          s.push(varDef);
        }
        declList = declList->children[0];
      }
      
      while (!s.empty()) {
        node->varDefs.push_back(std::shared_ptr<ASTVarDef>(s.top()));
        s.pop();
      }
    }
    return node;  // 由于使用了虚继承，可以直接返回
  }
  // VarDef
  else if (_STR_EQ(n->name, "VarDef")) {
    auto node = new ASTVarDef();
    if (n->children_num >= 1 && n->children[0] && n->children[0]->name) {
      node->ident = n->children[0]->name;
    }
    
    // 处理数组维度
    if (n->children_num >= 2 && n->children[1]) {
      auto expList = n->children[1];
      if (expList && expList->children_num > 0 && expList->children[0] && !_STR_EQ(expList->children[0]->name, "epsilon")) {
        while (expList && expList->children_num == 4) {
          auto constExp = static_cast<ASTConstExp *>(transform_node_iter(expList->children[2]));
          if (constExp) {
            node->dims.push_back(std::shared_ptr<ASTConstExp>(constExp));
          }
          expList = expList->children[0];
        }
      }
    }
    
    // 处理初始值
    if (n->children_num == 4 && n->children[3]) {
      node->initVal = std::shared_ptr<ASTInitVal>(
        static_cast<ASTInitVal *>(transform_node_iter(n->children[3])));
    }
    return node;
  }
  // InitVal
  else if (_STR_EQ(n->name, "InitVal")) {
    auto node = new ASTInitVal();
    if (n->children_num == 1) {
      // Exp
      node->exp = std::shared_ptr<ASTExp>(
        static_cast<ASTExp *>(transform_node_iter(n->children[0])));
    } else if (n->children_num == 2) {
      // LBRACE RBRACE
      // 空数组初始化
    } else {
      // LBRACE InitValList RBRACE
      auto valList = n->children[1];
      std::stack<ASTInitVal *> s;
      while (valList && _STR_EQ(valList->name, "InitValList") && valList->children_num == 3) {
        auto val = static_cast<ASTInitVal *>(transform_node_iter(valList->children[2]));
        if (val) {
          s.push(val);
        }
        valList = valList->children[0];
      }
      if (valList && valList->children[0]) {
        auto val = static_cast<ASTInitVal *>(transform_node_iter(valList->children[0]));
        if (val) {
          s.push(val);
        }
      }
      
      while (!s.empty()) {
        node->initVals.push_back(std::shared_ptr<ASTInitVal>(s.top()));
        s.pop();
      }
    }
    return node;
  }
  // FuncDef
  else if (_STR_EQ(n->name, "FuncDef")) {
    auto node = new ASTFuncDef();
    
    // 检查基本的子节点数
    if (n->children_num < 5) {
      std::cerr << "Error: FuncDef has insufficient children!" << std::endl;
      return node;
    }
    
    // 确定函数返回类型
    if (n->children[0] && n->children[0]->name && _STR_EQ(n->children[0]->name, "void")) {
      node->funcType = TYPE_VOID;
      if (n->children[1] && n->children[1]->name) {
        node->ident = n->children[1]->name;
      }
      
      // 处理参数
      if (n->children_num == 5) {
        // 无参数
      } else if (n->children_num == 6 && n->children[3]) {
        // 有参数
        auto paramList = n->children[3];
        std::stack<ASTFuncFParam *> s;
        while (paramList && _STR_EQ(paramList->name, "FuncFParams") && paramList->children_num == 3) {
          auto param = static_cast<ASTFuncFParam *>(transform_node_iter(paramList->children[2]));
          if (param) {
            s.push(param);
          }
          paramList = paramList->children[0];
        }
        if (paramList && paramList->children[0]) {
          auto param = static_cast<ASTFuncFParam *>(transform_node_iter(paramList->children[0]));
          if (param) {
            s.push(param);
          }
        }
        
        while (!s.empty()) {
          node->params.push_back(std::shared_ptr<ASTFuncFParam>(s.top()));
          s.pop();
        }
      }
      
      // Block
      if (n->children[n->children_num - 1]) {
        node->block = std::shared_ptr<ASTBlock>(
          static_cast<ASTBlock *>(transform_node_iter(n->children[n->children_num - 1])));
      }
    } else if (n->children[0] && n->children[0]->children_num > 0 && 
               n->children[0]->children[0] && n->children[0]->children[0]->name) {
      // BType (int or float)
      if (_STR_EQ(n->children[0]->children[0]->name, "int")) {
        node->funcType = TYPE_INT;
      } else {
        node->funcType = TYPE_FLOAT;
      }
      
      if (n->children[1] && n->children[1]->name) {
        node->ident = n->children[1]->name;
      }
      
      // 处理参数
      if (n->children_num == 5) {
        // 无参数
      } else if (n->children_num == 6 && n->children[3]) {
        // 有参数
        auto paramList = n->children[3];
        std::stack<ASTFuncFParam *> s;
        while (paramList && _STR_EQ(paramList->name, "FuncFParams") && paramList->children_num == 3) {
          auto param = static_cast<ASTFuncFParam *>(transform_node_iter(paramList->children[2]));
          if (param) {
            s.push(param);
          }
          paramList = paramList->children[0];
        }
        if (paramList && paramList->children[0]) {
          auto param = static_cast<ASTFuncFParam *>(transform_node_iter(paramList->children[0]));
          if (param) {
            s.push(param);
          }
        }
        
        while (!s.empty()) {
          node->params.push_back(std::shared_ptr<ASTFuncFParam>(s.top()));
          s.pop();
        }
      }
      
      // Block
      if (n->children[n->children_num - 1]) {
        node->block = std::shared_ptr<ASTBlock>(
          static_cast<ASTBlock *>(transform_node_iter(n->children[n->children_num - 1])));
      }
    } else {
      node->funcType = TYPE_INT; // 默认类型
    }
    return node;
  }
  // FuncFParam
  else if (_STR_EQ(n->name, "FuncFParam")) {
    auto node = new ASTFuncFParam();
    // BType IDENT
    if (n->children_num >= 2 && n->children[0] && n->children[0]->children_num > 0 &&
        n->children[0]->children[0] && n->children[0]->children[0]->name) {
      if (_STR_EQ(n->children[0]->children[0]->name, "int")) {
        node->bType = TYPE_INT;
      } else {
        node->bType = TYPE_FLOAT;
      }
    } else {
      node->bType = TYPE_INT; // 默认类型
    }
    
    if (n->children[1] && n->children[1]->name) {
      node->ident = n->children[1]->name;
    }
    
    // 检查是否是数组参数
    if (n->children_num > 2) {
      node->isArray = true;
      // 处理数组维度
      if (n->children_num >= 5 && n->children[4]) {
        auto expList = n->children[4];
        if (expList && expList->children_num > 0 && expList->children[0] && !_STR_EQ(expList->children[0]->name, "epsilon")) {
          while (expList && expList->children_num == 4) {
            auto exp = static_cast<ASTExp *>(transform_node_iter(expList->children[2]));
            if (exp) {
              node->dims.push_back(std::shared_ptr<ASTExp>(exp));
            }
            expList = expList->children[0];
          }
        }
      }
    } else {
      node->isArray = false;
    }
    return node;
  }
  // Block
  else if (_STR_EQ(n->name, "Block")) {
    auto node = new ASTBlock();
    // LBRACE BlockList RBRACE
    if (n->children_num >= 3) {
      auto blockList = n->children[1];
      if (blockList && _STR_EQ(blockList->name, "BlockList")) {
        // 检查是否是空的 BlockList（children_num == 0 或只有 epsilon）
        if (blockList->children_num == 0) {
          // 空块，不做任何操作
        } else if (blockList->children_num == 1 && blockList->children[0] && 
                   _STR_EQ(blockList->children[0]->name, "epsilon")) {
          // epsilon 节点，空块
        } else if (blockList->children_num == 1) {
          // 单个语句的情况
          auto item = transform_node_iter(blockList->children[0]);
          if (item) {
            if (auto blockItem = dynamic_cast<ASTBlockItem *>(item)) {
              node->blockItems.push_back(std::shared_ptr<ASTBlockItem>(blockItem));
            }
          }
        } else if (blockList->children_num == 2) {
          // 多个语句，展开 BlockList
          std::stack<syntax_tree_node *> s;
          while (blockList && _STR_EQ(blockList->name, "BlockList") && blockList->children_num == 2) {
            if (blockList->children[1]) {
              s.push(blockList->children[1]);
            }
            blockList = blockList->children[0];
          }
          
          // 处理最后一个 BlockList 节点（可能是单个语句）
          if (blockList && blockList->children_num == 1 && blockList->children[0]) {
            auto item = transform_node_iter(blockList->children[0]);
            if (item) {
              if (auto blockItem = dynamic_cast<ASTBlockItem *>(item)) {
                node->blockItems.push_back(std::shared_ptr<ASTBlockItem>(blockItem));
              }
            }
          }
          
          while (!s.empty()) {
            auto item = transform_node_iter(s.top());
            if (item) {
              if (auto blockItem = dynamic_cast<ASTBlockItem *>(item)) {
                node->blockItems.push_back(std::shared_ptr<ASTBlockItem>(blockItem));
              }
            }
            s.pop();
          }
        }
      }
    }
    return node;
  }
  // BlockItem
  else if (_STR_EQ(n->name, "BlockItem")) {
    return transform_node_iter(n->children[0]);
  }
 // Stmt
// Stmt
else if (_STR_EQ(n->name, "Stmt")) {
    // 检查有效的子节点数
    if (n->children_num == 0) {
        return nullptr;
    }
    
    // 确保第一个子节点存在
    if (!n->children[0] || !n->children[0]->name) {
        return nullptr;
    }
    
    // 添加调试输出
    // std::cerr << "[AST DEBUG] Stmt with " << n->children_num << " children, first child: " 
    //           << n->children[0]->name << std::endl;
    // if (n->children_num >= 2 && n->children[1]) {
    //     std::cerr << "[AST DEBUG] Second child: " << n->children[1]->name << std::endl;
    // }
    
    // 检查第一个子节点来确定语句类型
    if (_STR_EQ(n->children[0]->name, "break")) {
        // BREAK SEMICOLON
        return new ASTBreakStmt();
    } else if (_STR_EQ(n->children[0]->name, "continue")) {
        // CONTINUE SEMICOLON
        return new ASTContinueStmt();
    } else if (_STR_EQ(n->children[0]->name, "return")) {
        // RETURN [Exp] SEMICOLON
        auto node = new ASTReturnStmt();
        if (n->children_num == 3 && n->children[1] && !_STR_EQ(n->children[1]->name, ";")) {
            node->exp = std::shared_ptr<ASTExp>(
              static_cast<ASTExp *>(transform_node_iter(n->children[1])));
        }
        return node;
    } else if (_STR_EQ(n->children[0]->name, "if")) {
        // IF LPARENTHESIS Cond RPARENTHESIS Stmt [ELSE Stmt]
        auto node = new ASTIfStmt();
        node->cond = std::shared_ptr<ASTCond>(
            static_cast<ASTCond *>(transform_node_iter(n->children[2])));
        
        // 使用dynamic_cast并检查结果
        auto thenStmt = dynamic_cast<ASTStmt *>(transform_node_iter(n->children[4]));
        if (!thenStmt) {
            std::cerr << "Failed to cast to ASTStmt (then)" << std::endl;
            std::abort();
        }
        node->thenStmt = std::shared_ptr<ASTStmt>(thenStmt);
        
        if (n->children_num == 7) {
            auto elseStmt = dynamic_cast<ASTStmt *>(transform_node_iter(n->children[6]));
            if (!elseStmt) {
                std::cerr << "Failed to cast to ASTStmt (else)" << std::endl;
                std::abort();
            }
            node->elseStmt = std::shared_ptr<ASTStmt>(elseStmt);
        }
        return node;
    } else if (_STR_EQ(n->children[0]->name, "while")) {
        // WHILE LPARENTHESIS Cond RPARENTHESIS Stmt
        auto node = new ASTWhileStmt();
        node->cond = std::shared_ptr<ASTCond>(
            static_cast<ASTCond *>(transform_node_iter(n->children[2])));
        
        // 使用dynamic_cast并检查结果
        auto stmt = dynamic_cast<ASTStmt *>(transform_node_iter(n->children[4]));
        if (!stmt) {
            std::cerr << "Failed to cast to ASTStmt (while)" << std::endl;
            std::abort();
        }
        node->stmt = std::shared_ptr<ASTStmt>(stmt);
        return node;
    } else if (_STR_EQ(n->children[0]->name, "Block")) {
        // Block
        auto node = new ASTBlockStmt();
        node->block = std::shared_ptr<ASTBlock>(
            static_cast<ASTBlock *>(transform_node_iter(n->children[0])));
        return node;
    } else if (_STR_EQ(n->children[0]->name, ";")) {
        // SEMICOLON (空语句)
        auto node = new ASTExpStmt();
        return node;
    } else if (n->children_num >= 2 && n->children[1] && _STR_EQ(n->children[1]->name, "=")) {
        // LVal ASSIGN Exp SEMICOLON
        auto node = new ASTAssignStmt();
        node->lVal = std::shared_ptr<ASTLVal>(
            static_cast<ASTLVal *>(transform_node_iter(n->children[0])));
        node->exp = std::shared_ptr<ASTExp>(
            static_cast<ASTExp *>(transform_node_iter(n->children[2])));
        return node;
    } else if (n->children_num == 2 && n->children[1] && _STR_EQ(n->children[1]->name, ";")) {
        // Exp SEMICOLON
        auto node = new ASTExpStmt();
        node->exp = std::shared_ptr<ASTExp>(
            static_cast<ASTExp *>(transform_node_iter(n->children[0])));
        return node;
    }
    
    std::cerr << "[ast]: unhandled Stmt type with first child: " << n->children[0]->name << std::endl;
    return nullptr;
}
  // Exp
  else if (_STR_EQ(n->name, "Exp")) {
    auto node = new ASTExp();
    node->addExp = std::shared_ptr<ASTAddExp>(
      static_cast<ASTAddExp *>(transform_node_iter(n->children[0])));
    return node;
  }
  // Cond
  else if (_STR_EQ(n->name, "Cond")) {
    auto node = new ASTCond();
    node->lOrExp = std::shared_ptr<ASTLOrExp>(
      static_cast<ASTLOrExp *>(transform_node_iter(n->children[0])));
    return node;
  }
  // LVal
  else if (_STR_EQ(n->name, "LVal")) {
    auto node = new ASTLVal();
    if (n->children_num >= 1 && n->children[0] && n->children[0]->name) {
      node->ident = n->children[0]->name;
    }
    
    // 处理数组下标
    if (n->children_num >= 2 && n->children[1]) {
      auto expList = n->children[1];
      if (expList && expList->children_num > 0 && expList->children[0] && !_STR_EQ(expList->children[0]->name, "epsilon")) {
        while (expList && expList->children_num == 4) {
          auto exp = static_cast<ASTExp *>(transform_node_iter(expList->children[2]));
          if (exp) {
            node->indices.push_back(std::shared_ptr<ASTExp>(exp));
          }
          expList = expList->children[0];
        }
      }
    }
    return node;
  }
  // PrimaryExp
  else if (_STR_EQ(n->name, "PrimaryExp")) {
    if (n->children_num == 3) {
      // LPARENTHESIS Exp RPARENTHESIS
      auto node = new ASTParenExp();
      node->exp = std::shared_ptr<ASTExp>(
        static_cast<ASTExp *>(transform_node_iter(n->children[1])));
      return node;
    } else if (_STR_EQ(n->children[0]->name, "LVal")) {
      // LVal
      auto node = new ASTLValExp();
      node->lVal = std::shared_ptr<ASTLVal>(
        static_cast<ASTLVal *>(transform_node_iter(n->children[0])));
      return node;
    } else {
      // Number
      return transform_node_iter(n->children[0]);
    }
  }
  // Number
  else if (_STR_EQ(n->name, "Number")) {
    return transform_node_iter(n->children[0]);
  }
  // Integer
 // 修改 ast.cpp 中的 Integer 处理
else if (_STR_EQ(n->name, "Integer")) {
    auto node = new ASTNumber();
    node->isInt = true;
    if (n->children_num >= 1 && n->children[0] && n->children[0]->name) {
        try {
            std::string num_str = n->children[0]->name;
            // 检查是否是十六进制
            if (num_str.length() > 2 && num_str[0] == '0' && 
                (num_str[1] == 'x' || num_str[1] == 'X')) {
                node->intVal = std::stoi(num_str, nullptr, 16);
            }
            // 检查是否是八进制
            else if (num_str.length() > 1 && num_str[0] == '0' && 
                     num_str[1] >= '0' && num_str[1] <= '7') {
                node->intVal = std::stoi(num_str, nullptr, 8);
            }
            // 否则是十进制
            else {
                node->intVal = std::stoi(num_str, nullptr, 10);
            }
        } catch (...) {
            node->intVal = 0; // 默认值
        }
    } else {
        node->intVal = 0; // 默认值
    }
    return node;
}
  // Floatnum
  else if (_STR_EQ(n->name, "Floatnum")) {
    auto node = new ASTNumber();
    node->isInt = false;
    if (n->children_num >= 1 && n->children[0] && n->children[0]->name) {
      try {
        node->floatVal = std::stof(n->children[0]->name);
      } catch (...) {
        node->floatVal = 0.0f; // 默认值
      }
    } else {
      node->floatVal = 0.0f; // 默认值
    }
    return node;
  }
  // UnaryExp
  else if (_STR_EQ(n->name, "UnaryExp")) {
    if (n->children_num == 1) {
      // PrimaryExp
      auto node = new ASTPrimaryUnaryExp();
      node->primaryExp = std::shared_ptr<ASTPrimaryExp>(
        static_cast<ASTPrimaryExp *>(transform_node_iter(n->children[0])));
      return node;
    } else if (n->children_num >= 3 && _STR_EQ(n->children[1]->name, "(")) {
      // IDENT LPARENTHESIS [FuncRParams] RPARENTHESIS
      auto node = new ASTCallExp();
      node->ident = n->children[0]->name;
      if (n->children_num == 4) {
        node->funcRParams = std::shared_ptr<ASTFuncRParams>(
          static_cast<ASTFuncRParams *>(transform_node_iter(n->children[2])));
      }
      return node;
    } else {
      // UnaryOp UnaryExp
      auto node = new ASTUnaryOpExp();
      if (n->children[0] && n->children[0]->children_num > 0 && 
          n->children[0]->children[0] && n->children[0]->children[0]->name) {
        auto op = n->children[0]->children[0]->name;
        if (_STR_EQ(op, "+")) {
          node->unaryOp = UOP_PLUS;
        } else if (_STR_EQ(op, "-")) {
          node->unaryOp = UOP_MINUS;
        } else {
          node->unaryOp = UOP_NOT;
        }
      } else {
        node->unaryOp = UOP_PLUS; // 默认操作符
      }
      
      if (n->children_num >= 2 && n->children[1]) {
        node->unaryExp = std::shared_ptr<ASTUnaryExp>(
          static_cast<ASTUnaryExp *>(transform_node_iter(n->children[1])));
      }
      return node;
    }
  }
  // FuncRParams
  else if (_STR_EQ(n->name, "FuncRParams")) {
    auto node = new ASTFuncRParams();
    std::stack<ASTExp *> s;
    auto params = n;
    while (params && _STR_EQ(params->name, "FuncRParams") && params->children_num == 3) {
      auto exp = static_cast<ASTExp *>(transform_node_iter(params->children[2]));
      if (exp) {
        s.push(exp);
      }
      params = params->children[0];
    }
    if (params && params->children[0]) {
      auto exp = static_cast<ASTExp *>(transform_node_iter(params->children[0]));
      if (exp) {
        s.push(exp);
      }
    }
    
    while (!s.empty()) {
      node->exps.push_back(std::shared_ptr<ASTExp>(s.top()));
      s.pop();
    }
    return node;
  }
  // MulExp
  else if (_STR_EQ(n->name, "MulExp")) {
    auto node = new ASTMulExp();
    if (n->children_num == 1) {
      // UnaryExp
      node->unaryExp = std::shared_ptr<ASTUnaryExp>(
        static_cast<ASTUnaryExp *>(transform_node_iter(n->children[0])));
    } else {
      // MulExp MulOp UnaryExp
      node->mulExp = std::shared_ptr<ASTMulExp>(
        static_cast<ASTMulExp *>(transform_node_iter(n->children[0])));
      
      if (n->children_num >= 2 && n->children[1] && n->children[1]->name) {
        auto op = n->children[1]->name;
        if (_STR_EQ(op, "*")) {
          node->mulOp = OP_MUL;
        } else if (_STR_EQ(op, "/")) {
          node->mulOp = OP_DIV;
        } else {
          node->mulOp = OP_MOD;
        }
      } else {
        node->mulOp = OP_MUL; // 默认操作符
      }
      
      if (n->children_num >= 3 && n->children[2]) {
        node->unaryExp = std::shared_ptr<ASTUnaryExp>(
          static_cast<ASTUnaryExp *>(transform_node_iter(n->children[2])));
      }
    }
    return node;
  }
  // AddExp
  else if (_STR_EQ(n->name, "AddExp")) {
    auto node = new ASTAddExp();
    if (n->children_num == 1) {
      // MulExp
      node->mulExp = std::shared_ptr<ASTMulExp>(
        static_cast<ASTMulExp *>(transform_node_iter(n->children[0])));
    } else {
      // AddExp AddOp MulExp
      node->addExp = std::shared_ptr<ASTAddExp>(
        static_cast<ASTAddExp *>(transform_node_iter(n->children[0])));
      
      if (n->children_num >= 2 && n->children[1] && n->children[1]->name) {
        auto op = n->children[1]->name;
        if (_STR_EQ(op, "+")) {
          node->addOp = OP_PLUS;
        } else {
          node->addOp = OP_MINUS;
        }
      } else {
        node->addOp = OP_PLUS; // 默认操作符
      }
      
      if (n->children_num >= 3 && n->children[2]) {
        node->mulExp = std::shared_ptr<ASTMulExp>(
          static_cast<ASTMulExp *>(transform_node_iter(n->children[2])));
      }
    }
    return node;
  }
  // RelExp
  else if (_STR_EQ(n->name, "RelExp")) {
    auto node = new ASTRelExp();
    if (n->children_num == 1) {
      // AddExp
      node->addExp = std::shared_ptr<ASTAddExp>(
        static_cast<ASTAddExp *>(transform_node_iter(n->children[0])));
    } else {
      // RelExp RelOp AddExp
      node->relExp = std::shared_ptr<ASTRelExp>(
        static_cast<ASTRelExp *>(transform_node_iter(n->children[0])));
      
      if (n->children_num >= 2 && n->children[1] && n->children[1]->name) {
        auto op = n->children[1]->name;
        if (_STR_EQ(op, "<")) {
          node->relOp = OP_LT;
        } else if (_STR_EQ(op, "<=")) {
          node->relOp = OP_LE;
        } else if (_STR_EQ(op, ">")) {
          node->relOp = OP_GT;
        } else {
          node->relOp = OP_GE;
        }
      } else {
        node->relOp = OP_LT; // 默认操作符
      }
      
      if (n->children_num >= 3 && n->children[2]) {
        node->addExp = std::shared_ptr<ASTAddExp>(
          static_cast<ASTAddExp *>(transform_node_iter(n->children[2])));
      }
    }
    return node;
  }
  // EqExp
  else if (_STR_EQ(n->name, "EqExp")) {
    auto node = new ASTEqExp();
    if (n->children_num == 1) {
      // RelExp
      node->relExp = std::shared_ptr<ASTRelExp>(
        static_cast<ASTRelExp *>(transform_node_iter(n->children[0])));
    } else {
      // EqExp EqOp RelExp
      node->eqExp = std::shared_ptr<ASTEqExp>(
        static_cast<ASTEqExp *>(transform_node_iter(n->children[0])));
      
      if (n->children_num >= 2 && n->children[1] && n->children[1]->name) {
        auto op = n->children[1]->name;
        if (_STR_EQ(op, "==")) {
          node->eqOp = OP_EQ;
        } else {
          node->eqOp = OP_NE;
        }
      } else {
        node->eqOp = OP_EQ; // 默认操作符
      }
      
      if (n->children_num >= 3 && n->children[2]) {
        node->relExp = std::shared_ptr<ASTRelExp>(
          static_cast<ASTRelExp *>(transform_node_iter(n->children[2])));
      }
    }
    return node;
  }
  // LAndExp
  else if (_STR_EQ(n->name, "LAndExp")) {
    auto node = new ASTLAndExp();
    if (n->children_num == 1) {
      // EqExp
      node->eqExp = std::shared_ptr<ASTEqExp>(
        static_cast<ASTEqExp *>(transform_node_iter(n->children[0])));
    } else {
      // LAndExp AND EqExp
      node->lAndExp = std::shared_ptr<ASTLAndExp>(
        static_cast<ASTLAndExp *>(transform_node_iter(n->children[0])));
      node->eqExp = std::shared_ptr<ASTEqExp>(
        static_cast<ASTEqExp *>(transform_node_iter(n->children[2])));
    }
    return node;
  }
  // LOrExp
  else if (_STR_EQ(n->name, "LOrExp")) {
    auto node = new ASTLOrExp();
    if (n->children_num == 1) {
      // LAndExp
      node->lAndExp = std::shared_ptr<ASTLAndExp>(
        static_cast<ASTLAndExp *>(transform_node_iter(n->children[0])));
    } else {
      // LOrExp OR LAndExp
      node->lOrExp = std::shared_ptr<ASTLOrExp>(
        static_cast<ASTLOrExp *>(transform_node_iter(n->children[0])));
      node->lAndExp = std::shared_ptr<ASTLAndExp>(
        static_cast<ASTLAndExp *>(transform_node_iter(n->children[2])));
    }
    return node;
  }
  // ConstExp
  else if (_STR_EQ(n->name, "ConstExp")) {
    auto node = new ASTConstExp();
    node->addExp = std::shared_ptr<ASTAddExp>(
      static_cast<ASTAddExp *>(transform_node_iter(n->children[0])));
    return node;
  }
  // 处理各种列表节点
  else if (_STR_EQ(n->name, "ConstExpList") || 
           _STR_EQ(n->name, "ExpList") || 
           _STR_EQ(n->name, "VarDeclList") ||
           _STR_EQ(n->name, "ConstDefList") ||
           _STR_EQ(n->name, "ConstInitValList") ||
           _STR_EQ(n->name, "InitValList") ||
           _STR_EQ(n->name, "BlockList") ||
           _STR_EQ(n->name, "FuncFParams") ||
           _STR_EQ(n->name, "FuncFParamList")) {
    // 这些列表节点在父节点中处理，不需要单独处理
    return nullptr;
  }
  // 处理运算符节点
  else if (_STR_EQ(n->name, "UnaryOp") ||
           _STR_EQ(n->name, "MulOp") ||
           _STR_EQ(n->name, "AddOp") ||
           _STR_EQ(n->name, "RelOp") ||
           _STR_EQ(n->name, "EqOp")) {
    // 运算符节点在父节点中直接访问，不需要单独处理
    return nullptr;
  }
  // 处理终结符
  else if (_STR_EQ(n->name, "void") || _STR_EQ(n->name, "int") || _STR_EQ(n->name, "float") ||
           _STR_EQ(n->name, "const") || _STR_EQ(n->name, "if") || _STR_EQ(n->name, "else") ||
           _STR_EQ(n->name, "while") || _STR_EQ(n->name, "break") || _STR_EQ(n->name, "continue") ||
           _STR_EQ(n->name, "return") || _STR_EQ(n->name, "(") || _STR_EQ(n->name, ")") ||
           _STR_EQ(n->name, "[") || _STR_EQ(n->name, "]") || _STR_EQ(n->name, "{") ||
           _STR_EQ(n->name, "}") || _STR_EQ(n->name, ";") || _STR_EQ(n->name, ",") ||
           _STR_EQ(n->name, "=") || _STR_EQ(n->name, "==") || _STR_EQ(n->name, "!=") ||
           _STR_EQ(n->name, "<=") || _STR_EQ(n->name, ">=") || _STR_EQ(n->name, "<") ||
           _STR_EQ(n->name, ">") || _STR_EQ(n->name, "+") || _STR_EQ(n->name, "-") ||
           _STR_EQ(n->name, "*") || _STR_EQ(n->name, "/") || _STR_EQ(n->name, "%") ||
           _STR_EQ(n->name, "!") || _STR_EQ(n->name, "&&") || _STR_EQ(n->name, "||")) {
    // 终结符在父节点中处理
    return nullptr;
  }
  // 处理标识符和字面量
  else if (n->children_num == 0) {
    // 叶子节点，可能是标识符或字面量
    // 这些应该在父节点中直接使用 n->name
    std::cerr << "[ast]: warning: leaf node '" << n->name << "' reached transform_node_iter" << std::endl;
    return nullptr;
  }
  else {
    // 打印更详细的错误信息
    std::cerr << "[ast]: unhandled node type: '" << n->name << "'" << std::endl;
    std::cerr << "  children_num: " << n->children_num << std::endl;
    if (n->children_num > 0) {
      std::cerr << "  first child: " << (n->children[0] ? (n->children[0]->name ? n->children[0]->name : "<null name>") : "<null>") << std::endl;
    }
    std::abort();
  }
  
  return nullptr;  // 添加默认返回值
}

// Accept方法实现
Value* ASTProgram::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTConstDecl::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTConstDef::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTConstInitVal::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTVarDecl::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTVarDef::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTInitVal::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTFuncDef::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTFuncFParam::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTBlock::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTAssignStmt::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTExpStmt::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTBlockStmt::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTIfStmt::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTWhileStmt::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTBreakStmt::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTContinueStmt::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTReturnStmt::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTCond::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTLVal::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTParenExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTLValExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTNumber::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTPrimaryUnaryExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTCallExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTUnaryOpExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTFuncRParams::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTMulExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTAddExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTRelExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTEqExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTLAndExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTLOrExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }
Value* ASTConstExp::accept(ASTVisitor &visitor) { return visitor.visit(*this); }

// ASTPrinter实现
#define _DEBUG_PRINT_N_(N) { std::cout << std::string(N, '-'); }

Value* ASTPrinter::visit(ASTProgram &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "Program" << std::endl;
    add_depth();
    for (auto &unit : node.compUnits) {
        unit->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTConstDecl &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "ConstDecl" << std::endl;
    add_depth();
    for (auto &def : node.constDefs) {
        def->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTConstDef &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "ConstDef: " << node.ident << std::endl;
    add_depth();
    for (auto &dim : node.dims) {
        dim->accept(*this);
    }
    node.constInitVal->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTConstInitVal &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "ConstInitVal" << std::endl;
    add_depth();
    if (node.constExp) {
        node.constExp->accept(*this);
    } else {
        for (auto &val : node.constInitVals) {
            val->accept(*this);
        }
    }
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTVarDecl &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "VarDecl" << std::endl;
    add_depth();
    for (auto &def : node.varDefs) {
        def->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTVarDef &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "VarDef: " << node.ident << std::endl;
    add_depth();
    for (auto &dim : node.dims) {
        dim->accept(*this);
    }
    if (node.initVal) {
        node.initVal->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTInitVal &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "InitVal" << std::endl;
    add_depth();
    if (node.exp) {
        node.exp->accept(*this);
    } else {
        for (auto &val : node.initVals) {
            val->accept(*this);
        }
    }
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTFuncDef &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "FuncDef: " << node.ident << std::endl;
    add_depth();
    for (auto &param : node.params) {
        param->accept(*this);
    }
    node.block->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTFuncFParam &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "FuncFParam: " << node.ident;
    if (node.isArray) {
        std::cout << "[]";
    }
    std::cout << std::endl;
    add_depth();
    for (auto &dim : node.dims) {
        dim->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTBlock &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "Block" << std::endl;
    add_depth();
    for (auto &item : node.blockItems) {
        item->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTAssignStmt &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "AssignStmt" << std::endl;
    add_depth();
    node.lVal->accept(*this);
    node.exp->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTExpStmt &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "ExpStmt" << std::endl;
    add_depth();
    if (node.exp) {
        node.exp->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTBlockStmt &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "BlockStmt" << std::endl;
    add_depth();
    node.block->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTIfStmt &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "IfStmt" << std::endl;
    add_depth();
    node.cond->accept(*this);
    node.thenStmt->accept(*this);
    if (node.elseStmt) {
        node.elseStmt->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTWhileStmt &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "WhileStmt" << std::endl;
    add_depth();
    node.cond->accept(*this);
    node.stmt->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTBreakStmt &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "BreakStmt" << std::endl;
    return nullptr;
}

Value* ASTPrinter::visit(ASTContinueStmt &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "ContinueStmt" << std::endl;
    return nullptr;
}

Value* ASTPrinter::visit(ASTReturnStmt &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "ReturnStmt" << std::endl;
    add_depth();
    if (node.exp) {
        node.exp->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTExp &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "Exp" << std::endl;
    add_depth();
    node.addExp->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTCond &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "Cond" << std::endl;
    add_depth();
    node.lOrExp->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTLVal &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "LVal: " << node.ident << std::endl;
    add_depth();
    for (auto &index : node.indices) {
        index->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTParenExp &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "ParenExp" << std::endl;
    add_depth();
    node.exp->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTLValExp &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "LValExp" << std::endl;
    add_depth();
    node.lVal->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTNumber &node) {
    _DEBUG_PRINT_N_(depth);
    if (node.isInt) {
        std::cout << "Number (int): " << node.intVal << std::endl;
    } else {
        std::cout << "Number (float): " << node.floatVal << std::endl;
    }
    return nullptr;
}

Value* ASTPrinter::visit(ASTPrimaryUnaryExp &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "PrimaryUnaryExp" << std::endl;
    add_depth();
    node.primaryExp->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTCallExp &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "CallExp: " << node.ident << "()" << std::endl;
    add_depth();
    if (node.funcRParams) {
        node.funcRParams->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTUnaryOpExp &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "UnaryOpExp: ";
    switch (node.unaryOp) {
        case UOP_PLUS: std::cout << "+"; break;
        case UOP_MINUS: std::cout << "-"; break;
        case UOP_NOT: std::cout << "!"; break;
    }
    std::cout << std::endl;
    add_depth();
    node.unaryExp->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTFuncRParams &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "FuncRParams" << std::endl;
    add_depth();
    for (auto &exp : node.exps) {
        exp->accept(*this);
    }
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTMulExp &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "MulExp";
    if (node.mulExp) {
        std::cout << " ";
        switch (node.mulOp) {
            case OP_MUL: std::cout << "*"; break;
            case OP_DIV: std::cout << "/"; break;
            case OP_MOD: std::cout << "%"; break;
        }
    }
    std::cout << std::endl;
    add_depth();
    if (node.mulExp) {
        node.mulExp->accept(*this);
    }
    node.unaryExp->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTAddExp &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "AddExp";
    if (node.addExp) {
        std::cout << " ";
        switch (node.addOp) {
            case OP_PLUS: std::cout << "+"; break;
            case OP_MINUS: std::cout << "-"; break;
        }
    }
    std::cout << std::endl;
    add_depth();
    if (node.addExp) {
        node.addExp->accept(*this);
    }
    node.mulExp->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTRelExp &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "RelExp";
    if (node.relExp) {
        std::cout << " ";
        switch (node.relOp) {
            case OP_LT: std::cout << "<"; break;
            case OP_LE: std::cout << "<="; break;
            case OP_GT: std::cout << ">"; break;
            case OP_GE: std::cout << ">="; break;
            case OP_EQ: std::cout << "=="; break;
            case OP_NE: std::cout << "!="; break;
        }
    }
    std::cout << std::endl;
    add_depth();
    if (node.relExp) {
        node.relExp->accept(*this);
    }
    node.addExp->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTEqExp &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "EqExp";
    if (node.eqExp) {
        std::cout << " ";
        switch (node.eqOp) {
            case OP_EQ: std::cout << "=="; break;
            case OP_NE: std::cout << "!="; break;
            case OP_LT: case OP_LE: case OP_GT: case OP_GE:
                // 添加了完整的枚举值处理
                std::cout << "??"; break;
        }
    }
    std::cout << std::endl;
    add_depth();
    if (node.eqExp) {
        node.eqExp->accept(*this);
    }
    node.relExp->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTLAndExp &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "LAndExp";
    if (node.lAndExp) {
        std::cout << " &&";
    }
    std::cout << std::endl;
    add_depth();
    if (node.lAndExp) {
        node.lAndExp->accept(*this);
    }
    node.eqExp->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTLOrExp &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "LOrExp";
    if (node.lOrExp) {
        std::cout << " ||";
    }
    std::cout << std::endl;
    add_depth();
    if (node.lOrExp) {
        node.lOrExp->accept(*this);
    }
    node.lAndExp->accept(*this);
    remove_depth();
    return nullptr;
}

Value* ASTPrinter::visit(ASTConstExp &node) {
    _DEBUG_PRINT_N_(depth);
    std::cout << "ConstExp" << std::endl;
    add_depth();
    node.addExp->accept(*this);
    remove_depth();
    return nullptr;
}