#pragma once

#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include "../backend/Riscv.h"
/**
 * @file InstructionSchedule.h
 *
 * @brief 定义指令调度的头文件
 */
namespace riscv {
/**
 * @brief 指令调度类
 *
 * 1. 指令调度类，实现了两种调度方式
 * 2. 一种是寄存器分配前，按照变量的生命周期最短的目标进行调度，减少寄存器分配时的溢出
 * 3. 一种是寄存器分配后，按照指令的依赖关系进行调度，减少指令的流水线冲突
 *
 */
class InstructionSchedule {
 public:
  InstructionSchedule() = default;
  ~InstructionSchedule() = default;

 private:
  std::map<Instruction::InstKind, int> timeDelayTable;  ///< 指令延迟时间表

  /**
   * @brief 定义指令调度中的节点
   *
   * 对指令进行一层包装，加入父节点，子节点，以及执行时间，方便进行调度
   *
   */
  class DependTreeGraph {
   public:
    DependTreeGraph(Instruction *inst, int delayTime)
        : currentInst(inst), startTime(-1), execDelay(delayTime), parents({}), childs({}) {}

   public:
    bool ifinReady = false;   ///< 是否在ready列表
    bool ifinActive = false;  ///< 是否在active列表

   private:
    Instruction *currentInst;             ///< 当前节点下的指令
    int startTime = -1;                   ///< 指令开始执行
    int execDelay = -1;                   ///< 指令是否执行中
    std::set<DependTreeGraph *> parents;  //< parent是当前指令的依赖条件
    std::set<DependTreeGraph *> childs;   //< childs是依赖于当前指令的指令
    int priority = 0;                     ///< 按照延迟评判的优先级
    int index = -1;                       ///< 在原指令序列中的位置
    int accumuDelay = 0;                  ///< 累积延迟
    bool ifScheduled = false;             ///< 是否已经调度
    bool ifExecing = false;               ///< 是否正在执行

    std::set<DependTreeGraph *> trueChilds;   //< 写后读或者地址存后取的子节点
    std::set<DependTreeGraph *> trueParents;  //< 写后读或者地址存后取的父节点
    int lifeLength = 0;                       ///< 离自己的最近一次定值的距离
   public:
    void addTrueParent(DependTreeGraph *trueParent) {
      this->trueParents.emplace(trueParent);
    }  ///< 添加写后读或者地址存后取的父节点
    void addTrueChild(DependTreeGraph *trueChild) {
      this->trueChilds.emplace(trueChild);
    }  ///< 添加写后读或者地址存后取的子节点
    auto getTrueParents() -> std::set<DependTreeGraph *> { return trueParents; }  ///< 获取写后读或者地址存后取的父节点
    auto getTrueChilds() -> std::set<DependTreeGraph *> { return trueChilds; }  ///< 获取写后读或者地址存后取的子节点
    void setLifeDistance(int distance) { lifeLength = distance; }  ///< 设置离自己的最近一次定值的距离
    auto getLifeDistance() -> int { return lifeLength; }           ///< 获取离自己的最近一次定值的距离

   public:
    void init() {
      parents = {};
      childs = {};
    }
    auto getSelfDelay() const -> int { return execDelay; }                         ///< 获取自己的延迟
    void setDelay(int delay) { this->execDelay = delay; }                          ///< 设置自己的延迟
    void setIndex(int indexSet) { this->index = indexSet; }                        ///< 设置自己的索引
    auto getIndex() const -> int { return index; }                                 ///< 获取自己的索引
    auto getStartTime() const -> int { return startTime; }                         ///< 获取自己的开始时间
    void setStartTime(int start) { this->startTime = start; }                      ///< 设置自己的开始时间
    auto getEndTime() const -> int { return (startTime + execDelay); }             ///< 需要保证初始化了Delay
    void setCurInst(Instruction *instSet) { this->currentInst = instSet; }         ///< 设置自己的指令地址
    void addParent(DependTreeGraph *parentAdd) { parents.emplace(parentAdd); }     ///< 添加父节点
    void addChild(DependTreeGraph *childAdd) { childs.emplace(childAdd); }         ///< 添加子节点
    void removeParent(DependTreeGraph *parentMove) { parents.erase(parentMove); }  ///< 删除父节点
    void removeChild(DependTreeGraph *childMove) { childs.erase(childMove); }      ///< 删除子节点
    auto getParents() const -> std::set<DependTreeGraph *> { return parents; }     ///< 获取父节点
    auto isTop() const -> bool { return parents.empty(); }                         ///< 是否为根节点
    auto isBottom() const -> bool { return childs.empty(); }                       ///< 是否为叶子节点
    void setPriority(int score) { this->priority = score; }                        ///< 设置优先级
    auto getPriority() const -> int { return this->priority; }                     ///< 获取优先级
    auto getChilds() const -> std::set<DependTreeGraph *> { return childs; }       ///< 获取子节点
    auto getCurInst() const -> Instruction * { return this->currentInst; }         ///< 获取当前指令
    void setSchedule() { this->ifScheduled = true; }                               ///< 设置已调度
    auto getStatus() const -> bool { return this->ifScheduled; }                   ///< 获取调度状态
    void setExec() { this->ifExecing = true; }                                     ///< 设置正在执行
    auto getExec() { return this->ifExecing; }                                     ///< 获取执行状态
    void addAccumuDelay(int accumu) { this->accumuDelay += accumu; }               ///< 增加累积延迟
    void setAccumuDelay(int setAccmu) { this->accumuDelay = setAccmu; }            ///< 设置累积延迟
    auto getAccumuDelay() const -> int { return this->accumuDelay; }               ///< 获取累积延迟
  };
  /**
   * @struct CompareNodes
   * @brief 比较节点延迟
   * 作为优先级队列ready的比较函数，方便随时对传入的参数进行比较
   */
  struct CompareNodes {
    bool operator()(const std::shared_ptr<DependTreeGraph> &l1, const std::shared_ptr<DependTreeGraph> &l2) {
      if (l1->getSelfDelay() != l2->getSelfDelay()) {    //< 按照延迟来区分
        return l1->getSelfDelay() < l2->getSelfDelay();  // 注意这里是反向比较
      }
      if (l1->getPriority() != l2->getPriority()) {  //< 延迟高的先执行比较好
        return l1->getPriority() > l2->getPriority();
      }
      return l1->getAccumuDelay() > l2->getAccumuDelay();
    }
  };

  /**
   * @struct CompareNodesLife
   * @brief 比较节点生命周期长度的自定义比较函数
   */
  struct CompareNodesLife {
    bool operator()(const std::shared_ptr<DependTreeGraph> &l1, const std::shared_ptr<DependTreeGraph> &l2) {
      return l1->getLifeDistance() < l2->getLifeDistance();
    }
  };
  std::map<BasicBlock *, std::list<std::pair<Instruction *, std::shared_ptr<DependTreeGraph>>>>
      globalDependGraph{};  ///<   依赖图
  std::priority_queue<std::shared_ptr<DependTreeGraph>, std::vector<std::shared_ptr<DependTreeGraph>>, CompareNodes>
      ready;  ///< 就绪队列

  std::queue<DependTreeGraph *> readyBefore{};  ///<  寄存器前调度的就绪队列
  std::stack<DependTreeGraph *> newactive{};    ///<  寄存器前调度的活跃队列

  std::list<std::shared_ptr<DependTreeGraph>> active{};  ///< 寄存器后调度的活跃队列
  std::list<DependTreeGraph *> finish{};                 ///< 完成队列

  std::list<Instruction *> blockScheduledInst;  ///< 基本块内已调度的指令序列
  std::map<BasicBlock *, std::list<Instruction *>> globalScheduleInstructions;  ///< 全局已调度的指令序列

  std::map<Register *, std::tuple<Register *, Register *, Instruction::InstKind>>
      regAllocaTable{};  ///< 目的寄存器对应的计算映射

 public:
  void buildInitTable(Module *pModule);              ///< 建立延迟时间映射表
  void buildDependGraph(Module *pModule, int mode);  ///< 建立依赖图
  auto buildBlockDependTree(BasicBlock *block) const
      -> std::list<std::pair<Instruction *, std::shared_ptr<DependTreeGraph>>>;  ///< 建立某个基本块的依赖树
  void initRootNodes(BasicBlock *block);                                         ///< 基本块调度前的准备
  void computePriority(BasicBlock *block);                                       ///< 计算优先级,作为调度标准
  void scheduleModule(std::string filename, Module *pModule, int mode,
                      bool ifprint = false);  //< 传入Module，操作Module中的基本块
  void scheduleBlockBefore(
      BasicBlock *block,
      std::list<std::pair<Instruction *, std::shared_ptr<DependTreeGraph>>>);  //< 对一个基本块内的指令进行调度
  void scheduleBlock(
      BasicBlock *block,
      std::list<std::pair<Instruction *, std::shared_ptr<DependTreeGraph>>>);  //< 对一个基本块内的指令进行调度
  void delUselessEdge();               //< 在基本块里,1->2->5,同时,1->5,则删除1->5和5->1的依赖边
  void replaceInsts(Module *pModule);  //< 把原来的指令序列替换掉,沿用riscvprinter的打印函数
  void printNewModule(const std::string &fileName, Module *pModule);  //< 打印新顺序的指令
  void printNewFunc(Function *function);                              //< 按新顺序打印func

 public:
  void computeLife(BasicBlock *block);  //< 计算指令的生命周期，先调度离自己的定值最远的
  auto compareTuple(Register *reg1, Register *reg2) const -> bool;  ///< 比较两个寄存器对应的计算映射是否相等

 public:
  void clear();                                                                     ///<  清空使用的资源
  auto getDelayByInst(Instruction::InstKind kind) const -> int;                     ///< 获取该类型指令的延迟
  auto getNodeByInst(Instruction *inst) const -> std::shared_ptr<DependTreeGraph>;  ///< 通过指令构造节点
  auto getDelaytable() const -> std::map<Instruction::InstKind, int> { return timeDelayTable; }  ///< 获取延迟表
  auto getDependGraph() const
      -> std::map<BasicBlock *, std::list<std::pair<Instruction *, std::shared_ptr<DependTreeGraph>>>> {
    return globalDependGraph;
  }  ///< 获取依赖图
  auto getGloabalScheduledInsts() const -> std::map<BasicBlock *, std::list<Instruction *>> {
    return globalScheduleInstructions;
  }                                                                           ///<  获取全局调度后指令
  auto getInstOpAddr(Instruction *inst) const -> std::pair<Register *, int>;  ///< 获取指令操作数地址加偏移量
  auto ifStackReg(Instruction *inst) -> bool;                                 ///< 判断是否为栈寄存器
  auto ifPhyReg(Instruction *inst) -> bool;                                   ///< 判断是否为物理寄存器
  void updateReady(
      std::list<std::pair<Instruction *, std::shared_ptr<DependTreeGraph>>> blockGraph);  ///< 更新ready列表
  void updateReadyFront(std::list<std::pair<Instruction *, std::shared_ptr<DependTreeGraph>>>
                            blockGraph);  ///< 更新寄存器分配前的ready列表
};
}  // namespace riscv
