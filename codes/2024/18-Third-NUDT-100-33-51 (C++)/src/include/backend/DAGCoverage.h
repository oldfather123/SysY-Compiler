#pragma once

/**
 * @file DAGCoverage.h
 * @brief 基于DAG覆盖的指令选择器的头文件
 *
 */

#include "../backend/DAG2End.h"
#include "../backend/DAGBuilder.h"
#include "../backend/DAGPrinter.h"
#include "../backend/Mid2End.h"
#include "../frontend/IR.h"
namespace mid2EndDAG {
/**
 * @brief DAG覆盖指令选择
 *
 */
class DAGCoverage : public mid2end::InstSelection {
 protected:
  Mid2EndDAGBuilder DAGbuilder;     ///< DAG构建器
  Mid2EndDAGSimplifier simplifier;  ///< DAG化简器
  DAG2EndBuilder codeBuilder;       ///< DAG到汇编的转换器

 public:
  DAGCoverage() = default;

 public:
  void select() override {
    DAGbuilder.initBasicElements(midModule);
    DAGbuilder.buildDAG(midModule);

    // Mid2EndDAGPrinter printer(DAGbuilder);
    // printer.printModule();

    simplifier.init(DAGbuilder.getModule());
    simplifier.run();

    // Mid2EndDAGPrinter printer(DAGbuilder);
    // printer.printModule();

    codeBuilder.init(DAGbuilder);
    codeBuilder.generateModule();
  }  ///< 指令选择
  auto getStackTable(riscv::Function *function) -> mid2end::StackTable * override {
    return codeBuilder.getStackTable(function);
  }                                                                                         ///< 获取栈表
  auto getBuilder() -> riscv::RiscvBuilder & override { return codeBuilder.getBuilder(); }  ///< 获取构建器
  auto getModule() const -> riscv::Module * override { return codeBuilder.getModule(); }    ///< 获取riscv模块
};
}  // namespace mid2EndDAG