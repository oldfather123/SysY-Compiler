#include "ast.h"
#include "basic_block.h"
#include "module.h"
#include "symbol_table.h"
#include <queue>
#include <stdexcept>
#include <unordered_map>

class GenerateIR : public ASTVisitor {
private:
  ir::BasicKind _curTypeKind;
  bool _isConst;
  // bool _useConst = false;
  // std::vector<std::unique_ptr<Argument>> _arguments;
  ir::Value *_retVal;
  ir::BasicBlock *_retBlock;
  // bool _isNewFunction = false;
  bool _isRealLVal = false;
  ir::Function *_curFunction = nullptr;
  ir::Value *_curVal = nullptr;
  ir::BasicBlock *_entryBlock = nullptr;
  ir::BasicBlock *_condBlock = nullptr;
  ir::BasicBlock *_trueBlock = nullptr;
  ir::BasicBlock *_falseBlock = nullptr;
  ir::BasicBlock *_breakBlock = nullptr;
  // // continue, break, return stmt should set this to true;
  // bool _hasBranch = false;

  ir::Module *const _module = new ir::Module();
  SymbolTable *const _symbolTable = new SymbolTable();
  ir::BasicBlock *_curBlock;
  std::unordered_map<ir::Argument *, ir::AllocaInst *> _argToAllocaMap;

  void initBuiltInFuncs();

  void initSysCalls();

  void processCond(ir::Value *value);

  void makeInitVal(std::vector<int> &dimensions, std::map<int, AddExp *> &exps,
                   int base, const InitVal *initVal);

  ir::Value *typeConversion(ir::Value *value, ir::BasicKind targetType);

  static ir::BasicKind autoTypePromotion(ir::BasicKind type1,
                                         ir::BasicKind type2);

  void handleScalarDef(Def &ast);

  void handleArrayDef(Def &ast);

  void handleScalarVar(LVal &ast);

  void handleArrayVar(LVal &ast);

  void handleAssignStmt(Stmt &ast);

  void handleIfElseStmt(IfStmt &ast);

  std::unique_ptr<ir::BasicType> handleType(BType &type);

public:
  GenerateIR() {
    _symbolTable->in();
    initBuiltInFuncs();
    initSysCalls();
  }

  void checkTerminator();

  ir::Module *getModule() const { return _module; }

  void visit(CompUnit &ast) override;

  void visit(DeclDef &ast) override;

  void visit(Decl &ast) override;

  void visit(Def &ast) override;

  void visit(InitVal &ast) override;

  void visit(FuncDef &ast) override;

  void visit(FuncFParam &ast) override;

  void visit(Block &ast) override;

  void visit(BlockItem &ast) override;

  void visit(Stmt &ast) override;

  void visit(ReturnStmt &ast) override;

  void visit(IfStmt &ast) override;

  void visit(WhileStmt &ast) override;

  void visit(AddExp &ast) override;

  void visit(MulExp &ast) override;

  void visit(UnaryExp &ast) override;

  void visit(PrimaryExp &ast) override;

  void visit(LVal &ast) override;

  void visit(NumberNode &ast) override;

  void visit(Call &ast) override;

  void visit(RelExp &ast) override;

  void visit(EqExp &ast) override;

  void visit(LAndExp &ast) override;

  void visit(LOrExp &ast) override;
};
