#pragma once

#include "define.h"
#include <memory>
#include <string>
#include <vector>

using namespace std;

class BaseNode;

class CompUnit;
class DeclDef;
class Decl;
class DefList;
class Def;
class ArrayList;
class InitValList;
class InitVal;
class FuncDef;
class FuncFParamList;
class FuncFParam;
class Block;
class BlockItemList;
class BlockItem;
class Stmt;
class ReturnStmt;
class IfStmt;
class WhileStmt;
class LVal;
class PrimaryExp;
class NumberNode;
class UnaryExp;
class Call;
class FuncCParamList;
class MulExp;
class AddExp;
class RelExp;
class EqExp;
class LAndExp;
class LOrExp;

class ASTVisitor;

class BaseNode {
public:
  virtual void accept(ASTVisitor &visitor) = 0;

  BaseNode() = default;

  virtual ~BaseNode() = default;
};

class CompUnit : public BaseNode {
public:
  vector<unique_ptr<DeclDef>> declDefList;
  void accept(ASTVisitor &visitor) override;
};

class DeclDef : public BaseNode {
public:
  unique_ptr<Decl> decl = nullptr;
  unique_ptr<FuncDef> funcDef = nullptr;
  void accept(ASTVisitor &visitor) override;
};

class Decl final : public BaseNode {
public:
  BType bType = BType::VOID;
  bool isConst = false;
  vector<unique_ptr<Def>> defList;
  void accept(ASTVisitor &visitor) override;
};

class DefList {
public:
  vector<unique_ptr<Def>> list;
};

class Def : public BaseNode {
public:
  unique_ptr<string> id;
  vector<unique_ptr<AddExp>> arrays;
  unique_ptr<InitVal> initVal;
  void accept(ASTVisitor &visitor) override;
};

class ArrayList {
public:
  vector<unique_ptr<AddExp>> list;
};

class InitVal : public BaseNode {
public:
  unique_ptr<AddExp> exp;
  vector<unique_ptr<InitVal>> initValList;
  void accept(ASTVisitor &visitor) override;
};

class InitValList {
public:
  vector<unique_ptr<InitVal>> list;
};

class FuncDef : public BaseNode {
public:
  BType returnType = BType::VOID;
  unique_ptr<string> id;
  vector<unique_ptr<FuncFParam>> funcFParamList;
  unique_ptr<Block> block = nullptr;
  void accept(ASTVisitor &visitor) override;
};

class FuncFParamList {
public:
  vector<unique_ptr<FuncFParam>> list;
};

class FuncFParam : public BaseNode {
public:
  BType bType;
  unique_ptr<string> id;
  bool isArray = false;
  vector<unique_ptr<AddExp>> arrays;
  void accept(ASTVisitor &visitor) override;
};

class Block : public BaseNode {
public:
  vector<unique_ptr<BlockItem>> blockItemList;
  void accept(ASTVisitor &visitor) override;
};

class BlockItemList {
public:
  vector<unique_ptr<BlockItem>> list;
};

class BlockItem : public BaseNode {
public:
  unique_ptr<Decl> decl = nullptr;
  unique_ptr<Stmt> stmt = nullptr;
  void accept(ASTVisitor &visitor) override;
};

class Stmt : public BaseNode {
public:
  StmtType sType;
  unique_ptr<LVal> lVal = nullptr;
  unique_ptr<AddExp> exp = nullptr;
  unique_ptr<ReturnStmt> returnStmt = nullptr;
  unique_ptr<IfStmt> ifStmt = nullptr;
  unique_ptr<WhileStmt> whileStmtAST = nullptr;
  unique_ptr<Block> block = nullptr;
  void accept(ASTVisitor &visitor) override;
};

class ReturnStmt : public BaseNode {
public:
  unique_ptr<AddExp> exp = nullptr;
  void accept(ASTVisitor &visitor) override;
};

class IfStmt : public BaseNode {
public:
  unique_ptr<LOrExp> cond;
  unique_ptr<Stmt> ifStmt, elseStmt;
  void accept(ASTVisitor &visitor) override;
};

class WhileStmt : public BaseNode {
public:
  unique_ptr<LOrExp> cond;
  unique_ptr<Stmt> stmt;
  void accept(ASTVisitor &visitor) override;
};

class UnaryExp : public BaseNode {
public:
  unique_ptr<PrimaryExp> primaryExp;
  unique_ptr<Call> call;
  unique_ptr<UnaryExp> unaryExp;
  UnaryOp op;
  void accept(ASTVisitor &visitor) override;
};

class AddExp : public BaseNode {
public:
  unique_ptr<AddExp> addExp;
  unique_ptr<MulExp> mulExp;
  AddOp op;
  void accept(ASTVisitor &visitor) override;
};

class MulExp : public BaseNode {
public:
  unique_ptr<UnaryExp> unaryExp;
  unique_ptr<MulExp> mulExp;
  MulOp op;
  void accept(ASTVisitor &visitor) override;
};

class NumberNode : public BaseNode {
public:
  bool isInt;
  union {
    int intval;
    float floatval;
  };

  void accept(ASTVisitor &visitor) override;
};

class PrimaryExp : public BaseNode {
public:
  unique_ptr<AddExp> exp;
  unique_ptr<LVal> lval;
  unique_ptr<NumberNode> number;
  void accept(ASTVisitor &visitor) override;
};

class LVal : public BaseNode {
public:
  unique_ptr<string> id;
  vector<unique_ptr<AddExp>> arrays;
  void accept(ASTVisitor &visitor) override;
};

class Call : public BaseNode {
public:
  unique_ptr<string> id;
  vector<unique_ptr<AddExp>> funcCParamList;
  void accept(ASTVisitor &visitor) override;
};

class FuncCParamList {
public:
  vector<unique_ptr<AddExp>> list;
};

class RelExp : public BaseNode {
public:
  unique_ptr<AddExp> addExp;
  unique_ptr<RelExp> relExp;
  RelOp op;
  void accept(ASTVisitor &visitor) override;
};

class EqExp : public BaseNode {
public:
  unique_ptr<RelExp> relExp;
  unique_ptr<EqExp> eqExp;
  EqOp op;
  void accept(ASTVisitor &visitor) override;
};

class LAndExp : public BaseNode {
public:
  unique_ptr<EqExp> eqExp;
  unique_ptr<LAndExp> lAndExp;
  void accept(ASTVisitor &visitor) override;
};

class LOrExp : public BaseNode {
public:
  unique_ptr<LOrExp> lOrExp;
  unique_ptr<LAndExp> lAndExp;
  void accept(ASTVisitor &visitor) override;
};

class ASTVisitor {
public:
  virtual void visit(CompUnit &ast) = 0;

  virtual void visit(DeclDef &ast) = 0;

  virtual void visit(Decl &ast) = 0;

  virtual void visit(Def &ast) = 0;

  virtual void visit(InitVal &ast) = 0;

  virtual void visit(FuncDef &ast) = 0;

  virtual void visit(FuncFParam &ast) = 0;

  virtual void visit(Block &ast) = 0;

  virtual void visit(BlockItem &ast) = 0;

  virtual void visit(Stmt &ast) = 0;

  virtual void visit(ReturnStmt &ast) = 0;

  virtual void visit(IfStmt &ast) = 0;

  virtual void visit(WhileStmt &ast) = 0;

  virtual void visit(AddExp &ast) = 0;

  virtual void visit(MulExp &ast) = 0;

  virtual void visit(UnaryExp &ast) = 0;

  virtual void visit(PrimaryExp &ast) = 0;

  virtual void visit(LVal &ast) = 0;

  virtual void visit(NumberNode &ast) = 0;

  virtual void visit(Call &ast) = 0;

  virtual void visit(RelExp &ast) = 0;

  virtual void visit(EqExp &ast) = 0;

  virtual void visit(LAndExp &ast) = 0;

  virtual void visit(LOrExp &ast) = 0;
};
