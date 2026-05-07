#pragma once

#include <sys/types.h>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include "../backend/BackEndActiveAnalysis.h"
#include "../backend/InstructionSchedule.h"
#include "../backend/Peephole.h"
#include "../backend/RegisterAlloc.h"
#include "../backend/Riscv.h"
#include "../backend/RiscvBuilder.h"
#include "../backend/RiscvPrinter.h"
#include "../frontend/IR.h"
#include "../utils/SysyDebugger.h"

/**
 * @file Mid2End.h
 *
 * @brief 定义中端IR转化为后端IR的转化器的头文件
 */

// 数组访问常数合并
namespace mid2end {
/**
 * @brief 定义了局部数组
 *
 */
class LocalArray {
 private:
  std::string name;            ///< 局部数组的名字
  bool isInt;                  ///< 局部数组类型是否为int
  std::vector<unsigned> dims;  ///< 局部数组的维度

 public:
  LocalArray(bool isInt, std::vector<unsigned> dims, std::string name)
      : name(std::move(name)), isInt(isInt), dims(std::move(dims)) {}

 public:
  auto getName() const -> const std::string & { return name; }            ///< 获取全局数组的名字
  auto isIntType() const -> bool { return isInt; }                        ///< 是否为整型
  auto getDims() const -> const std::vector<unsigned> & { return dims; }  ///< 获取维度数组
};

/**
 * @brief 函数栈空间表
 *
 * 其记录了函数参数存放位置，以及函数体内局部变量存放位置
 * 默认指针以外的变量按四字节存，指针按八字节存
 *
 * @warning 只有在所有StackTable的参数和局部变量被记录, 且所有被调用的函数都被记录后，才可计算stackSize
 */
class StackTable {
 public:
  static const std::map<riscv::PhyRegister *, bool> isCalleeReserve;

 private:
  bool isInsertEnd = false;                              ///< 是否完成所有的插入
  uint64_t stackSize = 16;                               ///< 栈空间大小(以字节计算)
  uint64_t outStackSize{};                               ///< 下一个外部栈起始地址(以字节计)
  std::map<riscv::SymRegister *, uint64_t> spillParams;  ///< 溢出形式参数起始位置列表(以字节计)
  std::map<riscv::SymRegister *, uint64_t> spills;       ///< 溢出的寄存器起始位置列表(以字节计)
  std::map<LocalArray *, unsigned> localArrays;          ///< 局部数组的起始位置列表(以字节计)
  std::map<riscv::PhyRegister *, uint64_t> calleeReserveRegisterMap;  ///< 被调用者保存的物理寄存器
  std::set<riscv::PhyRegister *> callerReserveRegisters;              ///< 调用者保存的物理寄存器
  std::set<riscv::PhyRegister *> notSpillParamRegs;                   ///< 不溢出参数使用的物理寄存器
  std::map<riscv::CallInst *, std::set<riscv::PhyRegister *>> activeAtCallInst;  ///< 在调用指令处活跃的物理寄存器
  std::map<riscv::CallInst *, std::map<riscv::PhyRegister *, uint64_t>>
      callerReserveRegisterMap;  ///< 被调用者保存的物理寄存器存储地址(以字节计)
  std::map<riscv::CallInst *, StackTable *> callees;  ///< 被调用函数的栈空间表

 public:
  void addSpillParam(riscv::SymRegister *param);                            ///< 添加溢出的参数符号寄存器
  void addNotSpillParamReg(riscv::PhyRegister *reg);                        ///< 添加未溢出的参数符号寄存器
  void addLocalArray(LocalArray *localArray);                               ///< 添加局部数组
  void addPhyReg(riscv::PhyRegister *reg);                                  ///< 添加使用的物理寄存器
  void addSpill(riscv::SymRegister *spill);                                 ///< 添加溢出的符号寄存器
  void addCalleeStackTable(riscv::CallInst *callInst, StackTable *callee);  ///< 添加call指令及其对应的被调用函数栈表
  void initActiveAtCallInst(riscv::CallInst *callInst) {
    activeAtCallInst.emplace(callInst, std::set<riscv::PhyRegister *>{});
  }  ///< 初始化call指令处的活跃变量信息
  void addActiveAtCallInst(riscv::CallInst *callInst, riscv::PhyRegister *reg);  ///< 添加call指令处的活跃物理寄存器
  void endSpillInsert();                                                         ///< 终止添加

 public:
  StackTable() = default;

 public:
  auto getCallees() const -> const std::map<riscv::CallInst *, StackTable *> & {
    return callees;
  }  ///< 获取call指令及其被调用函数的栈表
  auto getSpillParamAddr(riscv::SymRegister *param) const -> uint64_t;  ///< 获取溢出参数的地址偏移量
  auto getLocalArrayAddr(LocalArray *localArray) const -> uint64_t;     ///< 获取局部数组的地址偏移量
  auto getSpillAddr(riscv::SymRegister *spill) const -> uint64_t;       ///< 获取溢出寄存器的地址偏移量
  auto getSpillParams() const -> const std::map<riscv::SymRegister *, uint64_t> & {
    return spillParams;
  }  ///< 获取溢出参数及其对应的地址偏移量
  auto getNotSpillParamRegs() const -> const std::set<riscv::PhyRegister *> & {
    return notSpillParamRegs;
  }  ///< 获取未溢出参数符号寄存器集合
  auto getCalleeReserveRegMap() const -> const std::map<riscv::PhyRegister *, uint64_t> & {
    return calleeReserveRegisterMap;
  }  ///< 获取被调用者保存的物理寄存器及其对应的保存地址偏移量
  auto getCallerReserveRegMap(riscv::CallInst *callInst) const -> const std::map<riscv::PhyRegister *, uint64_t> & {
    return callerReserveRegisterMap.at(callInst);
  }  ///< 获取调用者保存的物理寄存器及其对应的保存地址偏移量
  auto getCallerReserveRegs() const -> const std::set<riscv::PhyRegister *> & {
    return callerReserveRegisters;
  }                                                                  ///< 获取调用者保存的物理寄存器集合
  auto getOutStackSize() const -> uint64_t { return outStackSize; }  ///< 获取外部栈大小
  auto getStackSize() const -> uint64_t;                             ///< 获取内部栈大小
  auto isSpillParam(riscv::SymRegister *param) const -> bool {
    return spillParams.find(param) != spillParams.end();
  }  ///< 是否为溢出参数
  auto isSpill(riscv::SymRegister *spill) const -> bool {
    return spills.find(spill) != spills.end();
  }  ///< 是否为溢出变量
};

/**
 * @brief 指令选择器基类
 *
 *
 */
class InstSelection {
 protected:
  sysy::Module *midModule;  ///< 待处理的中端IR模块

 public:
  InstSelection() = default;
  virtual ~InstSelection() = default;

 public:
  void init(sysy::Module *inputMidModule) { midModule = inputMidModule; }     ///< 初始化
  virtual void select() = 0;                                                  ///< 指令选择
  virtual auto getModule() const -> riscv::Module * = 0;                      ///< 获取结果模块
  virtual auto getStackTable(riscv::Function *function) -> StackTable * = 0;  ///< 获取栈表
  virtual auto getBuilder() -> riscv::RiscvBuilder & = 0;                     ///< 获取构建器
};

/**
 * @brief 指令生成器类
 *
 */
class CodeGenerater {
 private:
  static std::list<riscv::IntPhyRegister *> intTmpUseRegs;      ///< 整型临时寄存器
  static std::list<riscv::FloatPhyRegister *> floatTmpUseRegs;  ///< 浮点临时寄存器

 protected:
  std::unique_ptr<InstSelection> pSelector;  ///< 指令选择器
  riscv::ActiveVarAnalysis activeAnalysis;   ///< 活跃变量分析器
  riscv::ExtendedLinearScan allocator;       ///< 寄存器分配器
  riscv::InstructionSchedule instschedule;   ///< 指令调度器
  riscv::RiscvCodePrinter riscvPrinter;      ///< riscv 打印器
  riscv::Peephole peephole;                  ///< 窥孔优化器
  sysy::SysyDebugger debug;                  ///< debugger器

 public:
  CodeGenerater(unsigned numInt, unsigned numFloat, InstSelection *generator)
      : pSelector(generator), allocator(riscv::ExtendedLinearScan(numInt, numFloat)) {}

 private:
  auto getPhyReg(unsigned position, riscv::SymRegister *symbol)
      -> riscv::PhyRegister *;  ///< 获取指定位置的符号寄存器对应的物理寄存器

 private:
  void initMove(
      riscv::Function *function, std::map<unsigned, riscv::BasicBlock::iterator> &postionInstIterMap,
      std::map<unsigned, std::map<riscv::PhyRegister *, riscv::PhyRegister *>> &inBlockMoveMap,
      std::map<riscv::BasicBlock *, std::map<riscv::BasicBlock *, std::map<riscv::PhyRegister *, riscv::PhyRegister *>>>
          &betweenBlockMove);  ///< 初始化寄存器转移信息

  void moveRegInBlock(
      std::map<unsigned, riscv::BasicBlock::iterator> &postionInstIterMap,
      std::map<unsigned, std::map<riscv::PhyRegister *, riscv::PhyRegister *>> &inBlockMoveMap);  ///< 块内转移

  void moveRegBetweenBlock(
      riscv::Function *function,
      std::map<riscv::BasicBlock *, std::map<riscv::BasicBlock *, std::map<riscv::PhyRegister *, riscv::PhyRegister *>>>
          &betweenBlockMove);  ///< 块间转移

 protected:
  void initStackTable();                                             ///< 初始化栈表
  void replaceReg();                                                 ///< 替换虚拟寄存器为物理寄存器
  void callParamPass();                                              ///< 函数传参代码插入
  void moveReg();                                                    ///< 寄存器转移代码插入
  void insertSpillReg();                                             ///< 插入溢出代码
  void reserveCalleeReg();                                           ///< 保存被调用者保存寄存器
  void reserveCallerReg();                                           ///< 保存调用者保存寄存器
  void setStackSize();                                               ///< 设置栈大小
  void renameGlobals() { pSelector->getModule()->renameGlobals(); }  ///< 重命名全局变量
  void renameFunctions();                                            ///< 重命名函数
  void simplify();                                                   ///< 化简riscv汇编
  void combBlocks();                                                 ///< 合并基本块
  void sortAndRenameBlocks();                                        ///< 排序并重命名基本块
  void deleteAfterSort();                                            ///< 删除多余的无条件跳转指令

 public:
  void run(sysy::Module *midModule) {
    pSelector->init(midModule);
    pSelector->select();
    combBlocks();
    // 指令调度
    // instschedule.scheduleModule("", pSelector->getModule(), 0, false);
    // instschedule.clear();
    // riscvPrinter.printRiscv("", pSelector->getModule());
    peephole.run(pSelector->getModule());

    activeAnalysis.analyze(pSelector->getModule());
    allocator.AllocateForModule(pSelector->getModule(), activeAnalysis.getActiveTable());
    initStackTable();
    replaceReg();
    moveReg();
    callParamPass();
    insertSpillReg();
    reserveCalleeReg();
    reserveCallerReg();
    setStackSize();
    renameGlobals();
    renameFunctions();
    simplify();
    combBlocks();
    sortAndRenameBlocks();
    deleteAfterSort();
  }                                                                       ///< 寄存器分配
  auto getModule() -> riscv::Module * { return pSelector->getModule(); }  ///< 获取结果模块
};
}  // namespace mid2end
