#pragma once

/**
 * @file DAG2End.h
 * @brief 中端到后端DAG转化为后端的头文件
 *
 */

#include <cstdint>
#include <memory>
#include "../backend/DAGBuilder.h"
#include "../backend/Mid2End.h"
#include "../backend/Mid2EndDAG.h"
#include "../backend/Riscv.h"
#include "../backend/RiscvBuilder.h"

namespace mid2EndDAG {
// 中端DAG到后端汇编的构建器
class DAG2EndBuilder {
 protected:
  uint32_t IMM_MAX = (1 << 12) - 1;  ///< 无符号指令立即数最大值
  int IMM_POS_MAX = (1 << 11) - 1;   ///< 有符号指令立即数最大值
  int IMM_NEG_MAX = -(1 << 11);      ///< 有符号指令立即数最小值
  uint32_t IMM_BIT = 12;             ///< 指令立即数最大位数

 protected:
  Module *pModule;                               ///< 所要转换的DAG模块
  std::unique_ptr<riscv::Module> backendModule;  ///< 结果riscv模块
  riscv::RiscvBuilder builder;                   ///< riscv构建器

  std::map<std::string, std::unique_ptr<riscv::SymRegister>> nameRegistersMap;         ///< 名字-虚拟寄存器对应
  std::map<Mid2EndDAG *, riscv::BasicBlock *> DAGBlockMap;                             ///< DAG-BasicBlock对应
  std::map<float, riscv::Global *> floatGlobalMap;                                     ///< float-Global对应
  std::map<riscv::Function *, std::unique_ptr<mid2end::StackTable>> functionStackMap;  ///< 函数-栈表对应
  std::map<Mid2EndPointerNode *, std::unique_ptr<mid2end::LocalArray>> localArrayMap;  ///< DAG pointer节点-局部数组对应

  std::map<Function *, std::map<uint64_t, std::string>> localScalarNameMap;  ///< 标号-标量名字对应
  std::map<Function *, std::map<Mid2EndPointerNode *, std::string>>
      localArrayNameMap;  ///< DAG pointer节点-数组名字对应

  uint64_t floatIndex = 0;                               ///< 浮点常量编号
  uint64_t tmpIndex = 0;                                 ///< 临时变量编号
  std::map<Mid2EndDAGNode *, uint64_t> tmpIndexMap;      ///< DAG节点-临时变量编号对应
  std::map<Mid2EndDAGNode *, std::string> specialNames;  ///< DAG节点-非临时变量名字对应

  // DAG层次的临时区
  std::map<std::string, std::set<std::string>> intReg2regMovMap;               ///< 整型寄存器-寄存器转移表
  std::map<std::string, std::set<std::string>> floatReg2regMovMap;             ///< 浮点寄存器-寄存器转移表
  std::map<uint64_t, std::set<std::string>> int2regMovMap;                     ///< 整型常量-寄存器转移表
  std::map<riscv::FloatSymRegister *, std::set<std::string>> float2regMovMap;  ///< 附带你常量-寄存器转移表
  std::map<int, riscv::IntSymRegister *> DAGIntConstants;                      ///< 存储int常量寄存器的记录表
  std::map<uint64_t, riscv::IntSymRegister *> DAGUint64Constants;  ///< 存储uint64常量寄存器的记录表
  std::map<float, riscv::FloatSymRegister *> DAGFloatConstants;    ///< 存储float常量寄存器的记录表
  std::map<uint64_t, Mid2EndDAGNode *> varInputMap;                ///< 标号-输入节点对应
  std::map<Mid2EndDAGNode *, bool> isVisited;                      ///< 节点访问记录表
  std::map<Mid2EndDAGNode *, bool> isAddToVisited;                 ///< 节点添加记录表
  std::list<Mid2EndDAGNode *> toVisit;                             ///< 待访问队列

 public:
  explicit DAG2EndBuilder() = default;

 protected:
  void interpretePointer(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);        ///< 翻译Pointer节点
  void interpreteConstant(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);       ///< 翻译Constant节点
  void interpreteScalarNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);     ///< 翻译Scalar节点
  void interpreteIntBinaryNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);  ///< 翻译IntBinary节点
  void interpreteUint64BinaryNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);  ///< 翻译Uint64Binary节点
  void interpreteFloatBinaryNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);  ///< 翻译FloatBinary节点
  void interpreteIntUnaryNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);     ///< 翻译IntUnary节点
  void interpreteFloatUnaryNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);  ///< 翻译FloatUnary节点
  void interpreteCallNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);        ///< 翻译Call节点
  void interpreteCondBrNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);      ///< 翻译CondBr节点
  void interpreteBrNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);          ///< 翻译Br节点
  void interpreteReturnNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);      ///< 翻译Return节点
  void interpreteLoadNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);        ///< 翻译Load节点
  void interpreteStoreNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);       ///< 翻译Store节点
  void interpreteLaNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);          ///< 翻译La节点
  void interpreteMemsetNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);      ///< 翻译Memset节点
  void interpreteRiscvCondBrNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);  ///< 翻译RiscvCondBr节点
  void interpreteRiscvF3OpNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);    ///< 翻译RiscvF3Op节点
  void interpreteRiscvLoadNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);    ///< 翻译RiscvLoad节点
  void interpreteRiscvStoreNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);  ///< 翻译RiscvStore节点

 protected:
  auto getOrEmplaceSymRegister(const std::string &name, Function *function, bool isInt, bool is64 = false)
      -> riscv::SymRegister *;                     ///< 获取或创建符号寄存器
  auto getOrEmplaceMemset() -> riscv::Function *;  ///< 获取或创建memset函数
  auto emplaceTmpSymRegister(bool isInt, bool is64 = false) -> riscv::SymRegister *;  ///< 创建临时符号寄存器
  auto getOrEmplaceConstant(int intValue) -> riscv::IntSymRegister *;  ///< 获取或创建存储int常量的虚拟寄存器
  auto getOrEmplaceConstant(uint64_t uint64Value) -> riscv::IntSymRegister *;  ///< 获取或创建存储uint64常量的虚拟寄存器
  auto getOrEmplaceConstant(float floatValue) -> riscv::FloatSymRegister *;  ///< 获取或创建存储float常量的虚拟寄存器
  auto getOperandName(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG) -> std::string;  ///< 获取操作数名字
  auto getOutputNames(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG)
      -> std::list<std::string>;                                            ///< 获取输出变量名字列表
  auto getOperandType(Mid2EndDAGNode *node) -> unsigned int;                ///< 获取操作数类型
  auto getParamPhyReg(bool isInt, unsigned index) -> riscv::PhyRegister *;  ///< 获取传参物理寄存器
  void DivideToMultiply(riscv::IntSymRegister *dividend, riscv::IntSymRegister *result, int divisor);  ///< 除化乘
  void MultiplyToShift(riscv::IntSymRegister *multiplier, riscv::IntSymRegister *result, int64_t imm,
                       bool is64);  ///< 乘化移位

 public:
  void init(Mid2EndDAGBuilder &builder) {
    pModule = builder.getModule();
    localArrayNameMap = builder.getLocalArrayNameMap();
    localScalarNameMap = builder.getLocalScalarNameMap();
  }  ///< 初始化
  auto getStackTable(riscv::Function *function) -> mid2end::StackTable * {
    return functionStackMap.at(function).get();
  }                                                                          ///< 获取栈表
  auto getBuilder() -> riscv::RiscvBuilder & { return builder; }             ///< 获取构建器
  auto getModule() const -> riscv::Module * { return backendModule.get(); }  ///< 获取结果模块

 public:
  void processNode(Mid2EndDAGNode *node, Function *function, Mid2EndDAG *DAG);  ///< 处理节点
  void generateGlobals();                                                       ///< 处理全局变量
  void generateDAG(Mid2EndDAG *DAG);                                            ///< 处理DAG
  void generateFunction(Function *function);                                    ///< 处理函数
  void generateModule();                                                        ///< 处理模块
};
}  // namespace mid2EndDAG