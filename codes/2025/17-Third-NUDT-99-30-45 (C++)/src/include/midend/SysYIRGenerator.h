#pragma once
#include "IR.h"
#include "IRBuilder.h"
#include "SysYBaseVisitor.h"
#include "SysYParser.h"
#include <memory>
#include <unordered_map>
#include <forward_list>

namespace sysy {


// @brief 用于存储数组值的树结构
// 多位数组本质上是一维数组的嵌套可以用树来表示。
class ArrayValueTree {
private:
  Value *value = nullptr; /// 该节点存储的value
  std::vector<std::unique_ptr<ArrayValueTree>> children; /// 子节点列表

public:
  ArrayValueTree() = default;

public:
  auto getValue() const -> Value * { return value; }
  auto getChildren() const
      -> const std::vector<std::unique_ptr<ArrayValueTree>> & {
    return children;
  }

  void setValue(Value *newValue) { value = newValue; }
  void addChild(ArrayValueTree *newChild) { children.emplace_back(newChild); }
  void addChildren(const std::vector<ArrayValueTree *> &newChildren) {
    for (const auto &child : newChildren) {
      children.emplace_back(child);
    }
  }
};


class Utils {
public:
  // transform a tree of ArrayValueTree to a ValueCounter
  static void tree2Array(Type *type, ArrayValueTree *root,
                         const std::vector<Value *> &dims, unsigned numDims,
                         ValueCounter &result, IRBuilder *builder);                          
  static void
  createExternalFunction(const std::vector<Type *> &paramTypes,
                         const std::vector<std::string> &paramNames,
                         const std::vector<std::vector<Value *>> &paramDims,
                         Type *returnType, const std::string &funcName,
                         Module *pModule, IRBuilder *pBuilder);

  static void initExternalFunction(Module *pModule, IRBuilder *pBuilder);
  static void modify_timefuncname(Module *pModule);
};

class SysYIRGenerator : public SysYBaseVisitor {

private:
  std::unique_ptr<Module> module;
  IRBuilder builder;

  using ValueOrOperator = std::variant<Value*, int>;
  std::vector<ValueOrOperator> BinaryExpStack;  ///< 用于存储二元表达式的中缀表达式
  std::vector<int> BinaryExpLenStack;  ///< 用于存储该层次的二元表达式的长度
  // 下面是用于后缀表达式的计算的数据结构
  std::vector<ValueOrOperator> BinaryRPNStack;  ///< 用于存储二元表达式的后缀表达式
  std::vector<int> BinaryOpStack;  ///< 用于存储二元表达式中缀表达式转换到后缀表达式的操作符栈 
  std::vector<Value *> BinaryValueStack;  ///< 用于存储后缀表达式计算的操作数栈
  
  // 约定操作符：
  // 1: 'ADD', 2: 'SUB', 3: 'MUL', 4: 'DIV', 5: '%', 6: 'PLUS', 7: 'NEG', 8: 'NOT', 9: 'LPAREN', 10: 'RPAREN'
  // 这里的操作符是为了方便后缀表达式的计算而设计
  // 其中，'ADD', 'SUB', 'MUL', 'DIV', '%'
  // 分别对应加法、减法、乘法、除法和取模
  // 'PLUS' 和 'NEG' 分别对应一元加法和一元减法
  // 'NOT' 对应逻辑非
  // 'LPAREN' 和 'RPAREN' 分别对应左括号和右括号
  enum BinaryOp {
    ADD = 1, SUB = 2, MUL = 3, DIV = 4, MOD = 5, PLUS = 6, NEG = 7, NOT = 8, LPAREN = 9, RPAREN = 10,
  };
  int getOperatorPrecedence(int op) {
    switch (op) {
    case MUL: case DIV: case MOD:   return 2;
    case ADD: case SUB: return 1;
    case PLUS: case NEG: case NOT:   return 3;
    case LPAREN: case RPAREN: return 0; // Parentheses have lowest precedence for stack logic
    default: return -1; // Unknown operator
    }
  };

  struct ExpKey {
    BinaryOp op;  ///< 操作符
    Value *left;  ///< 左操作数
    Value *right; ///< 右操作数
    ExpKey(BinaryOp op, Value *left, Value *right) : op(op), left(left), right(right) {}

    bool operator<(const ExpKey &other) const {
      if (op != other.op)
        return op < other.op; ///< 比较操作符
      if (left != other.left)
        return left < other.left; ///< 比较左操作
      return right < other.right; ///< 比较右操作数
    } ///< 重载小于运算符用于比较ExpKey
  };

  struct UnExpKey {
    BinaryOp op;    ///< 一元操作符
    Value *operand; ///< 操作数
    UnExpKey(BinaryOp op, Value *operand) : op(op), operand(operand) {}

    bool operator<(const UnExpKey &other) const {
      if (op != other.op)
        return op < other.op;         ///< 比较操作符
      return operand < other.operand; ///< 比较操作数
    } ///< 重载小于运算符用于比较UnExpKey
  };

  struct GEPKey {
    Value *basePointer;
    std::vector<Value *> indices;

    // 为 std::map 定义比较运算符，使得 GEPKey 可以作为键
    bool operator<(const GEPKey &other) const {
      if (basePointer != other.basePointer) {
        return basePointer < other.basePointer;
      }
      // 逐个比较索引，确保顺序一致
      if (indices.size() != other.indices.size()) {
        return indices.size() < other.indices.size();
      }
      for (size_t i = 0; i < indices.size(); ++i) {
        if (indices[i] != other.indices[i]) {
          return indices[i] < other.indices[i];
        }
      }
      return false; // 如果 basePointer 和所有索引都相同，则认为相等
    }
  };
  std::map<GEPKey, Value*> availableGEPs; ///< 用于存储 GEP 的缓存
  std::map<ExpKey, Value*> availableBinaryExpressions;
  std::map<UnExpKey, Value*> availableUnaryExpressions;
  std::map<Value*, Value*> availableLoads;

public:
  SysYIRGenerator() = default;

public:
  Module *get() const { return module.get(); }
  IRBuilder *getBuilder(){ return &builder; }
public:

  std::any visitCompUnit(SysYParser::CompUnitContext *ctx) override;

  std::any visitGlobalConstDecl(SysYParser::GlobalConstDeclContext *ctx) override;
  std::any visitGlobalVarDecl(SysYParser::GlobalVarDeclContext *ctx) override;

  // std::any visitDecl(SysYParser::DeclContext *ctx) override;
  std::any visitConstDecl(SysYParser::ConstDeclContext *ctx) override;
  std::any visitVarDecl(SysYParser::VarDeclContext *ctx) override;
  
  std::any visitBType(SysYParser::BTypeContext *ctx) override;
  
  // std::any visitConstDef(SysYParser::ConstDefContext *ctx) override;
  // std::any visitVarDef(SysYParser::VarDefContext *ctx) override;

  std::any visitScalarInitValue(SysYParser::ScalarInitValueContext *ctx) override;
  std::any visitArrayInitValue(SysYParser::ArrayInitValueContext *ctx) override;

  std::any visitConstScalarInitValue(SysYParser::ConstScalarInitValueContext *ctx) override;
  std::any visitConstArrayInitValue(SysYParser::ConstArrayInitValueContext *ctx) override;

  // std::any visitConstInitVal(SysYParser::ConstInitValContext *ctx) override;
  std::any visitFuncType(SysYParser::FuncTypeContext* ctx) override;
  std::any visitFuncDef(SysYParser::FuncDefContext* ctx) override;
  // std::any visitInitVal(SysYParser::InitValContext *ctx) override;
  // std::any visitFuncFParam(SysYParser::FuncFParamContext *ctx) override;
  // std::any visitFuncFParams(SysYParser::FuncFParamsContext *ctx) override;

  std::any visitBlockStmt(SysYParser::BlockStmtContext* ctx) override;
  // std::any visitStmt(SysYParser::StmtContext *ctx) override;
  std::any visitAssignStmt(SysYParser::AssignStmtContext *ctx) override;
  std::any visitExpStmt(SysYParser::ExpStmtContext *ctx) override;
  // std::any visitBlkStmt(SysYParser::BlkStmtContext *ctx) override;
  std::any visitIfStmt(SysYParser::IfStmtContext *ctx) override;
  std::any visitWhileStmt(SysYParser::WhileStmtContext *ctx) override;
  std::any visitBreakStmt(SysYParser::BreakStmtContext *ctx) override;
  std::any visitContinueStmt(SysYParser::ContinueStmtContext *ctx) override;
  std::any visitReturnStmt(SysYParser::ReturnStmtContext *ctx) override;
  
  // std::any visitExp(SysYParser::ExpContext *ctx) override;
  // std::any visitCond(SysYParser::CondContext *ctx) override;

  std::any visitLValue(SysYParser::LValueContext *ctx) override;

  std::any visitPrimaryExp(SysYParser::PrimaryExpContext *ctx) override;
  
  // std::any visitParenExp(SysYParser::ParenExpContext *ctx) override;
  std::any visitNumber(SysYParser::NumberContext *ctx) override;
  // std::any visitString(SysYParser::StringContext *ctx) override;
  
  std::any visitCall(SysYParser::CallContext *ctx) override;
  
  std::any visitUnaryExp(SysYParser::UnaryExpContext *ctx) override;
  // std::any visitUnaryOp(SysYParser::UnaryOpContext *ctx) override;
  
  // std::any visitUnExp(SysYParser::UnExpContext *ctx) override;

  std::any visitFuncRParams(SysYParser::FuncRParamsContext *ctx) override;
  std::any visitMulExp(SysYParser::MulExpContext *ctx) override;
  std::any visitAddExp(SysYParser::AddExpContext *ctx) override;
  std::any visitRelExp(SysYParser::RelExpContext *ctx) override;
  std::any visitEqExp(SysYParser::EqExpContext *ctx) override;
  std::any visitLAndExp(SysYParser::LAndExpContext *ctx) override;
  std::any visitLOrExp(SysYParser::LOrExpContext *ctx) override;
  
  std::any visitConstExp(SysYParser::ConstExpContext *ctx) override;

  bool isRightAssociative(int op);
  Value* promoteType(Value* value, Type* targetType);
  Value* computeExp(SysYParser::ExpContext *ctx, Type* targetType = nullptr);
  Value* computeAddExp(SysYParser::AddExpContext *ctx, Type* targetType = nullptr);
  void compute();

  // 参数是发生 store 操作的目标地址/变量的 Value*
  void invalidateExpressionsOnStore(Value* storedAddress);

  // 清除因函数调用而失效的表达式缓存（保守策略）
  void invalidateExpressionsOnCall();

  // 在进入新的基本块时清空所有表达式缓存
  void enterNewBasicBlock();
public:
  // 获取GEP指令的地址
  Value* getGEPAddressInst(Value* basePointer, const std::vector<Value*>& indices);
  // 构建数组类型
  Type* buildArrayType(Type* baseType, const std::vector<Value*>& dims);

  unsigned countArrayDimensions(Type* type);


}; // class SysYIRGenerator

} // namespace sysy