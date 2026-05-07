#pragma once

#include "../../SysYBaseVisitor.h"
#include "../../SysYParser.h"
#include "../frontend/IR.h"
#include "../frontend/IRBuilder.h"
#include <memory>
#include <string>
#include <vector>

/**
 * @file SysYIRGenerator.h
 * @brief
 * 该源文件包含对ANTRL生成的AST树每个节点的访问函数的重载声明，以及IR生成类IRGenerator的声明
 *
 * IR生成类IRGenerator继承自ANTRL提供的SysYBaseVisitor，本质上是ANTRL生成的AST树的遍历器。其通过
 * 控制每个节点访问时的动作生成IR。
 */

/** @fn auto SysYIRGenerator::visitDecl(SysYParser::DeclContext *ctx) ->
 * std::any override
 *  @brief AST树Decl节点的访问函数
 *
 *  调用子节点访问函数并返回其返回值
 *  @param [in] ctx 指向Decl节点的指针
 *  @return 由子节点决定
 */

/** @fn auto SysYIRGenerator::visitVarDef(SysYParser::VarDefContext *ctx) ->
 * std::any override
 *  @brief AST树VarDef节点的访问函数
 *
 *  调用子节点访问函数并返回其返回值
 *  @param [in] ctx 指向VarDef节点的指针
 *  @return 由子节点决定
 */

/** @fn auto SysYIRGenerator::visitConstDef(SysYParser::ConstDefContext *ctx) ->
 * std::any override
 *  @brief AST树ConstDef节点的访问函数
 *
 *  调用子节点访问函数并返回其返回值
 *  @param [in] ctx 指向ConstDef节点的指针
 *  @return 由子节点决定
 */

/** @fn auto SysYIRGenerator::visitBlockItem(SysYParser::BlockItemContext *ctx)
 * -> std::any override
 *  @brief AST树BlockItem节点的访问函数
 *
 *  调用子节点访问函数并返回其返回值
 *  @param [in] ctx 指向BlockItem节点的指针
 *  @return 由子节点决定
 */

/** @fn auto SysYIRGenerator::visitExpStmt(SysYParser::ExpStmtContext *ctx) ->
 * std::any override
 *  @brief AST树ExpStmt节点的访问函数
 *
 *  调用子节点访问函数并返回其返回值
 *  @param [in] ctx 指向ExpStmt节点的指针
 *  @return 由子节点决定
 */

/** @fn auto SysYIRGenerator::visitBlockStmt(SysYParser::BlockStmtContext *ctx)
 * -> std::any override
 *  @brief AST树BlockStmt节点的访问函数
 *
 *  调用子节点访问函数并返回其返回值
 *  @param [in] ctx 指向BlockStmt节点的指针
 *  @return 由子节点决定
 */

/** @fn auto SysYIRGenerator::visitExp(SysYParser::ExpContext *ctx) -> std::any
 * override
 *  @brief AST树Exp节点的访问函数
 *
 *  调用子节点访问函数并返回其返回值
 *  @param [in] ctx 指向Exp节点的指针
 *  @return 由子节点决定
 */

/** @fn auto SysYIRGenerator::visitCond(SysYParser::CondContext *ctx) ->
 * std::any override
 *  @brief AST树Cond节点的访问函数
 *
 *  调用子节点访问函数并返回其返回值
 *  @param [in] ctx 指向Cond节点的指针
 *  @return 由子节点决定
 */

/** @fn auto SysYIRGenerator::visitConstExp(SysYParser::ConstExpContext *ctx) ->
 * std::any override
 *  @brief AST树ConstExp节点的访问函数
 *
 *  调用子节点访问函数并返回其返回值
 *  @param [in] ctx 指向ConstExp节点的指针
 *  @return 由子节点决定
 */

namespace sysy {
/**
 * @brief 初值树节点
 *
 * 用来表示初值，其父子关系反映了数组初值中括号的嵌套关系。其中外层为父节点，内层为子节点
 */
class ValueTreeNode {
private:
  Value *value = nullptr; /// 该节点存储的值
  std::vector<std::unique_ptr<ValueTreeNode>> children; /// 子节点列表

public:
  ValueTreeNode() = default;

public:
  /** @brief 获取节点存储的值*/
  auto getValue() const -> Value * { return value; }
  /** @brief 获取子节点列表*/
  auto getChildren() const
      -> const std::vector<std::unique_ptr<ValueTreeNode>> & {
    return children;
  }

  /** @brief 设置节点的值*/
  void setValue(Value *newValue) { value = newValue; }
  /** @brief 添加子节点*/
  void addChild(ValueTreeNode *newChild) { children.emplace_back(newChild); }
  /** @brief 添加多个子节点*/
  void addChildren(const std::vector<ValueTreeNode *> &newChildren) {
    for (const auto &child : newChildren) {
      children.emplace_back(child);
    }
  }
};

/**
 * @ingroup utility
 * @brief 工具类
 *
 * 存放了有用的工具函数，供编译器的实现使用
 */
class Helper {
public:
  static void tree2Array(Type *type, ValueTreeNode *root,
                         const std::vector<Value *> &dims, unsigned numDims,
                         ValueCounter &result, IRBuilder *builder);
  static void
  createExternalFunction(const std::vector<Type *> &paramTypes,
                         const std::vector<std::string> &paramNames,
                         const std::vector<std::vector<Value *>> &paramDims,
                         Type *returnType, const std::string &funcName,
                         Module *pModule, IRBuilder *pBuilder);

  static void initExternalFunction(Module *pModule, IRBuilder *pBuilder);
};

/**
 * @brief IR生成类
 *
 * 使用时，调用成员函数getModule()，生成中间IR并存入数据成员module。
 */
class SysYIRGenerator : public SysYBaseVisitor {
private:
  std::unique_ptr<Module> module; ///< 模块，保存符号表以及IR等信息
  IRBuilder builder; ///< 构建器，拥有IR构建相关的函数接口

public:
  SysYIRGenerator() = default;

public:
  void clear() { module.reset(); } ///< 清空原有的前端信息
  /** @brief 返回模块*/
  auto getModule() const -> Module * { return module.get(); }
  /** @brief 返回构建器*/
  auto getBuilder() -> IRBuilder * { return &builder; }

public:
  auto visitCompUnit(SysYParser::CompUnitContext *ctx) -> std::any override;

  auto visitGlobalConstDecl(SysYParser::GlobalConstDeclContext *ctx)
      -> std::any override;

  auto visitGlobalVarDecl(SysYParser::GlobalVarDeclContext *ctx)
      -> std::any override;

  auto visitDecl(SysYParser::DeclContext *ctx) -> std::any override {
    return visitChildren(ctx);
  };

  auto visitVarDecl(SysYParser::VarDeclContext *ctx) -> std::any override;

  auto visitConstDecl(SysYParser::ConstDeclContext *ctx) -> std::any override;

  auto visitBType(SysYParser::BTypeContext *ctx) -> std::any override;

  auto visitVarDef(SysYParser::VarDefContext *ctx) -> std::any override {
    return visitChildren(ctx);
  }

  auto visitConstDef(SysYParser::ConstDefContext *ctx) -> std::any override {
    return visitChildren(ctx);
  }

  auto visitScalarInitValue(SysYParser::ScalarInitValueContext *ctx)
      -> std::any override;

  auto visitArrayInitValue(SysYParser::ArrayInitValueContext *ctx)
      -> std::any override;

  auto visitConstScalarInitValue(SysYParser::ConstScalarInitValueContext *ctx)
      -> std::any override;

  auto visitConstArrayInitValue(SysYParser::ConstArrayInitValueContext *ctx)
      -> std::any override;

  auto visitFuncDef(SysYParser::FuncDefContext *ctx) -> std::any override;

  auto visitFuncType(SysYParser::FuncTypeContext *ctx) -> std::any override;

  auto visitBlock(SysYParser::BlockContext *ctx) -> std::any override;

  auto visitBlockItem(SysYParser::BlockItemContext *ctx) -> std::any override {
    return visitChildren(ctx);
  }

  auto visitAssignStmt(SysYParser::AssignStmtContext *ctx) -> std::any override;

  auto visitExpStmt(SysYParser::ExpStmtContext *ctx) -> std::any override {
    return visitChildren(ctx);
  }

  auto visitBlockStmt(SysYParser::BlockStmtContext *ctx) -> std::any override {
    return visitChildren(ctx);
  }

  auto visitIfStmt(SysYParser::IfStmtContext *ctx) -> std::any override;

  auto visitWhileStmt(SysYParser::WhileStmtContext *ctx) -> std::any override;

  auto visitBreakStmt(SysYParser::BreakStmtContext *ctx) -> std::any override;

  auto visitContinueStmt(SysYParser::ContinueStmtContext *ctx)
      -> std::any override;

  auto visitReturnStmt(SysYParser::ReturnStmtContext *ctx) -> std::any override;

  auto visitExp(SysYParser::ExpContext *ctx) -> std::any override {
    return visitChildren(ctx);
  }

  auto visitCond(SysYParser::CondContext *ctx) -> std::any override {
    return visitChildren(ctx);
  }

  auto visitLVal(SysYParser::LValContext *ctx) -> std::any override;

  auto visitPrimaryExp(SysYParser::PrimaryExpContext *ctx) -> std::any override;

  auto visitNumber(SysYParser::NumberContext *ctx) -> std::any override;

  auto visitCallExp(SysYParser::CallExpContext *ctx) -> std::any override;

  auto visitUnaryExp(SysYParser::UnaryExpContext *ctx) -> std::any override;

  auto visitFuncRParams(SysYParser::FuncRParamsContext *ctx)
      -> std::any override;

  auto visitMulExp(SysYParser::MulExpContext *ctx) -> std::any override;

  auto visitAddExp(SysYParser::AddExpContext *ctx) -> std::any override;

  auto visitRelExp(SysYParser::RelExpContext *ctx) -> std::any override;

  auto visitEqExp(SysYParser::EqExpContext *ctx) -> std::any override;

  auto visitLAndExp(SysYParser::LAndExpContext *ctx) -> std::any override;

  auto visitLOrExp(SysYParser::LOrExpContext *ctx) -> std::any override;

  auto visitConstExp(SysYParser::ConstExpContext *ctx) -> std::any override {
    return visitChildren(ctx);
  }
}; // class SysYIRGenerator
} // namespace sysy
