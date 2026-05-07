#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
// #include "../midend/Loop.h"
#include "../utils/range.h"
/**
 * @file IR.h
 *
 * @brief 定义IR相关类型与操作的头文件
 */
namespace sysy {

/**
 * \defgroup type Types
 * @brief Sysy的类型系统
 *
 * 1. 基类`Type` 用来表示所有的原始标量类型,
 * 包括 `int`, `float`, `void`, 和表示跳转目标的标签类型。
 * 2. `PointerType` 和 `FunctionType` 派生自`Type` 并且分别表示指针和函数类型。
 *
 * \note `Type`和它的派生类的构造函数声明为'protected'.
 * 用户必须使用Type::getXXXType()获得`Type` 指针。
 * @{
 */

/**
 * @brief 原始标量类型
 *
 * `Type`用来表示所有的原始标量类型,
 * 包括`int`, `float`, `void`, 和表示跳转目标的标签类型。
 */
class Type {
 public:
  /// 定义了原始标量类型种类的枚举类型
  enum Kind {
    kInt,
    kFloat,
    kVoid,
    kLabel,
    kPointer,
    kFunction,
  };

  Kind kind;  ///< 表示具体类型的变量

 protected:
  explicit Type(Kind kind) : kind(kind) {}
  virtual ~Type() = default;

 public:
  static auto getIntType() -> Type *;                    ///< 返回表示Int类型的Type指针
  static auto getFloatType() -> Type *;                  ///< 返回表示Float类型的Type指针
  static auto getVoidType() -> Type *;                   ///< 返回表示Void类型的Type指针
  static auto getLabelType() -> Type *;                  ///< 返回表示Label类型的Type指针
  static auto getPointerType(Type *baseType) -> Type *;  ///< 返回表示指向baseType类型的Pointer类型的Type指针
  static auto getFunctionType(Type *returnType, const std::vector<Type *> &paramTypes = {}) -> Type *;
  ///< 返回表示返回类型为returnType,形参类型列表为paramTypes的函数类型的Type指针

 public:
  auto getKind() const -> Kind { return kind; }                  ///< 返回Type对象代表原始标量类型
  auto isInt() const -> bool { return kind == kInt; }            ///< 判定是否为Int类型
  auto isFloat() const -> bool { return kind == kFloat; }        ///< 判定是否为Float类型
  auto isVoid() const -> bool { return kind == kVoid; }          ///< 判定是否为Void类型
  auto isLabel() const -> bool { return kind == kLabel; }        ///< 判定是否为Label类型
  auto isPointer() const -> bool { return kind == kPointer; }    ///< 判定是否为Pointer类型
  auto isFunction() const -> bool { return kind == kFunction; }  ///< 判定是否为Function类型
  auto getSize() const -> unsigned;                              ///< 返回类型所占的空间大小(字节)
  /// 尝试将一个变量转换为给定的Type及其派生类类型的变量
  template <typename T>
  auto as() const -> std::enable_if_t<std::is_base_of_v<Type, T>, T *> {
    return dynamic_cast<T *>(const_cast<Type *>(this));
  }
};

/**
 * @brief 指针类型
 *
 * 表示指向某个类型的Pointer类型
 */
class PointerType : public Type {
 protected:
  Type *baseType;  ///< 所指向的类型

 protected:
  explicit PointerType(Type *baseType) : Type(kPointer), baseType(baseType) {}

 public:
  static auto get(Type *baseType) -> PointerType *;  ///< 获取指向baseType的Pointer类型

 public:
  auto getBaseType() const -> Type * { return baseType; }  ///< 获取指向的类型
};

/**
 * @brief 函数类型
 *
 * 表示具有特定返回值类型，形参类型列表的Fucntion类型
 */
class FunctionType : public Type {
 private:
  Type *returnType;                ///< 返回值类型
  std::vector<Type *> paramTypes;  ///< 形参类型列表

 protected:
  explicit FunctionType(Type *returnType, std::vector<Type *> paramTypes = {})
      : Type(kFunction), returnType(returnType), paramTypes(std::move(paramTypes)) {}

 public:
  /// 获取返回值类型为returnType， 形参类型列表为paramTypes的Function类型
  static auto get(Type *returnType, const std::vector<Type *> &paramTypes = {}) -> FunctionType *;

 public:
  auto getReturnType() const -> Type * { return returnType; }          ///< 获取返回值类信息
  auto getParamTypes() const { return make_range(paramTypes); }        ///< 获取形参类型列表
  auto getNumParams() const -> unsigned { return paramTypes.size(); }  ///< 获取形参数量
};

/**
 * @}
 */

/**
 * \defgroup ir IR
 *
 * TSysY IR 是一种指令级别的语言. 它被组织为四层树型结构，如下所示：
 *
 * \dot IR Structure
 *   digraph IRStructure{
 *    node [shape="box"]
 *    a [label="Module"]
 *    b [label="GlobalValue"]
 *    c [label="Function"]
 *    d [label="BasicBlock"]
 *    e [label="Instruction"]
 *    a->{b,c}
 *    c->d->e
 *  }
 *
 * \enddot
 *
 * - `Module` 对应顶层"CompUnit"语法结构
 * - `GlobalValue`对应"globalDecl"语法结构
 * - `Function`对应"FuncDef"语法结构
 * - `BasicBlock` 是一连串没有分支指令的指令。一个 `Function`
 *   由一个或多个`BasicBlock`组成
 * - `Instruction` 表示一个原始指令，例如, add 或 sub
 *
 *  SysY IR中基础的数据概念是`Value`。一个 `Value` 像
 * 一个寄存器。它充当`Instruction`的输入输出操作数。每个value
 * 都有一个与之相联系的`Type`，以此说明Value所拥有值的类型。
 *
 * 大多数`Instruction`具有三地址代码的结构, 例如, 最多拥有两个输入操作数和一个输出操作数。
 *
 * SysY IR采用了Static-Single-Assignment (SSA)设计。`Value`作为一个输出操作数
 * 被一些指令所定义, 并被另一些指令当作输入操作数使用。尽管一个Value可以被多个指令使用，其
 * 定义只能发生一次。这导致一个value个定义它的指令存在一一对应关系。换句话说，任何定义一个Value的指令
 * 都可以被看作被定义的指令本身。故在SysY IR中，`Instruction` 也是一个`Value`。查看 `Value` 以获取其继承
 * 关系。
 *
 * @{
 */

class User;
class Value;
class AllocaInst;

/**
 * @brief 使用关系
 *
 * 表示`Value`和它的`User`之间的使用关系。
 */
class Use {
 private:
  /**
   * @brief value在User操作数中的位置
   *
   * value在User操作数中的位置，例如，
   user->getOperands[index] == value*/
  unsigned index;
  User *user;    ///< 使用者
  Value *value;  ///< 被使用的值

 public:
  Use() = default;
  Use(unsigned index, User *user, Value *value) : index(index), user(user), value(value) {}

 public:
  auto getIndex() const -> unsigned { return index; }   ///< 返回value在User操作数中的位置
  auto getUser() const -> User * { return user; }       ///< 返回使用者
  auto getValue() const -> Value * { return value; }    ///< 返回被使用的值
  void setValue(Value *newValue) { value = newValue; }  ///< 将被使用的值设置为newValue
};

/**
 * @brief 所有值的基类
 */
class Value {
 protected:
  Type *type;                            ///< 值的类型
  std::string name;                      ///< 值的名字
  std::list<std::shared_ptr<Use>> uses;  ///< 值的使用关系列表

 protected:
  explicit Value(Type *type, std::string name = "") : type(type), name(std::move(name)) {}
  virtual ~Value() = default;

 public:
  void setName(const std::string &newName) { name = newName; }           ///< 设置名字
  auto getName() const -> const std::string & { return name; }           ///< 获取名字
  auto getType() const -> Type * { return type; }                        ///< 返回值的类型
  auto isInt() const -> bool { return type->isInt(); }                   ///< 判定是否为Int类型
  auto isFloat() const -> bool { return type->isFloat(); }               ///< 判定是否为Float类型
  auto isPointer() const -> bool { return type->isPointer(); }           ///< 判定是否为Pointer类型
  auto getUses() -> std::list<std::shared_ptr<Use>> & { return uses; }   ///< 获取使用关系列表
  void addUse(const std::shared_ptr<Use> &use) { uses.push_back(use); }  ///< 添加使用关系
  void replaceAllUsesWith(Value *value);  ///< 将原来使用该value的使用者全变为使用给定参数value并修改相应use关系
  void removeUse(const std::shared_ptr<Use> &use) { uses.remove(use); }  ///< 删除使用关系use
};

/**
 * @brief 值计数类
 *
 * 其被用于对一列Value *进行计数。其中相邻且相同的Value *按(pValue，num)进行记录。
 * 其中pValue表示指向Value的指针，num表示重复数量。ValueCounter通过记录一系列(pValue，num)来
 * 表示一列Value *，使得存储空间得到节省，方便Memset指令的创建。
 */
class ValueCounter {
 private:
  unsigned __size{};                       ///< 总的Value数量
  std::vector<Value *> __counterValues;    ///< 记录的Value *列表(无重复元素)
  std::vector<unsigned> __counterNumbers;  ///< 记录的Value *重复数量列表

 public:
  ValueCounter() = default;

 public:
  auto size() const -> unsigned { return __size; }  ///< 返回总的Value数量
  auto getValue(unsigned index) const -> Value * {
    if (index >= __size) {
      return nullptr;
    }

    unsigned num = 0;
    for (size_t i = 0; i < __counterNumbers.size(); i++) {
      if (num <= index && index < num + __counterNumbers[i]) {
        return __counterValues[i];
      }
      num += __counterNumbers[i];
    }

    return nullptr;
  }  ///< 根据位置index获取Value *
  auto getValues() const -> const std::vector<Value *> & { return __counterValues; }  ///< 获取互异Value *列表
  auto getNumbers() const -> const std::vector<unsigned> & { return __counterNumbers; }  ///< 获取Value *重复数量列表
  void push_back(Value *value, unsigned num = 1) {
    if (__size != 0 && __counterValues.back() == value) {
      *(__counterNumbers.end() - 1) += num;
    } else {
      __counterValues.push_back(value);
      __counterNumbers.push_back(num);
    }
    __size += num;
  }  ///< 向后插入num个value
  void clear() {
    __size = 0;
    __counterValues.clear();
    __counterNumbers.clear();
  }  ///< 清空ValueCounter
};

/**
 * @brief 编译时就能知道的静态字面值
 *
 * `ConstantValue`并不由指令定义, 也不使用任何Value。它的类型为int/float。
 */
class ConstantValue : public Value {
 protected:
  /// 定义字面量类型的聚合类型
  union {
    int iScalar;
    float fScalar;
  };

 protected:
  explicit ConstantValue(int value, const std::string &name = "") : Value(Type::getIntType(), name), iScalar(value) {}
  explicit ConstantValue(float value, const std::string &name = "")
      : Value(Type::getFloatType(), name), fScalar(value) {}

 public:
  static auto get(int value) -> ConstantValue *;    ///< 获取一个int类型的ConstValue *，其值为value
  static auto get(float value) -> ConstantValue *;  ///< 获取一个float类型的ConstValue *，其值为value

 public:
  auto getInt() const -> int {
    assert(isInt());
    return iScalar;
  }  ///< 返回int类型的值
  auto getFloat() const -> float {
    assert(isFloat());
    return fScalar;
  }  ///< 返回float类型的值
  template <typename T>
  auto getValue() const -> T {
    if (std::is_same<T, int>::value && isInt()) {
      return getInt();
    }
    if (std::is_same<T, float>::value && isFloat()) {
      return getFloat();
    }
    throw std::bad_cast();  // 或者其他适当的异常处理
  }                         ///< 返回值，getInt和getFloat统一化，整数返回整形，浮点返回浮点型
};

class Instruction;
class Function;
class Loop;
/**
 * @brief 基本块
 *
 * 基本块拥有一系列以终止子(如跳转指令)作为结尾的指令。除此之外，
 * 它还记录了前驱与后继基本块，支配树前驱与后继，其所在循环及其循环深度
 */
class BasicBlock : public Value {
  friend class Function;

 public:
  using inst_list = std::list<std::unique_ptr<Instruction>>;
  using iterator = inst_list::iterator;
  using arg_list = std::vector<AllocaInst *>;
  using block_list = std::vector<BasicBlock *>;
  using block_set = std::unordered_set<BasicBlock *>;

 protected:
  Function *parent;              ///< 从属的函数
  inst_list instructions;        ///< 拥有的指令序列
  arg_list arguments;            ///< 分配空间后的形式参数列表
  block_list successors;         ///< 前驱列表
  block_list predecessors;       ///< 后继列表
  BasicBlock *idom = nullptr;    ///< 直接支配结点，即支配树前驱，唯一，默认nullptr
  block_list sdoms;              ///< 支配树后继，可以有多个
  block_set dominants;           ///< 必经结点集合
  block_set dominant_frontiers;  ///< 支配边界
  bool reachable = false;        ///< 用于表示该节点是否可达，默认不可达
  Loop *loopbelong = nullptr;    ///< 用来表示该块属于哪个循环，唯一，默认nullptr
  int loopdepth = 0;             /// < 用来表示其归属循环的深度，默认0

 public:
  explicit BasicBlock(Function *parent, const std::string &name = "")
      : Value(Type::getLabelType(), name), parent(parent) {}

  ~BasicBlock() override {
    for (auto pre : predecessors) {
      pre->removeSuccessor(this);
    }

    for (auto suc : successors) {
      suc->removePredecessor(this);
    }
  }  ///< 基本块的析构函数，同时删除其前驱后继关系

 public:
  auto getNumInstructions() const -> unsigned { return instructions.size(); }  ///< 获取指令数量
  auto getNumArguments() const -> unsigned { return arguments.size(); }        ///< 获取形式参数数量
  auto getNumPredecessors() const -> unsigned { return predecessors.size(); }  ///< 获取前驱数量
  auto getNumSuccessors() const -> unsigned { return successors.size(); }      ///< 获取后继数量
  auto getParent() const -> Function * { return parent; }                      ///< 获取父函数
  void setParent(Function *func) { parent = func; }                            ///< 设置父函数
  auto getInstructions() -> inst_list & { return instructions; }               ///< 获取指令列表
  auto getArguments() -> arg_list & { return arguments; }  ///< 获取分配空间后的形式参数列表
  auto getPredecessors() const -> const block_list & { return predecessors; }  ///< 获取前驱列表
  auto getSuccessors() -> block_list & { return successors; }                  ///< 获取后继列表
  auto getDominants() -> block_set & { return dominants; }
  auto getIdom() -> BasicBlock * { return idom; }
  auto getSdoms() -> block_list & { return sdoms; }
  auto getDFs() -> block_set & { return dominant_frontiers; }
  auto begin() -> iterator { return instructions.begin(); }             ///< 返回指向指令列表开头的迭代器
  auto end() -> iterator { return instructions.end(); }                 ///< 返回指向指令列表末尾的迭代器
  auto terminator() -> iterator { return std::prev(end()); }            ///< 基本块最后的IR
  void insertArgument(AllocaInst *inst) { arguments.push_back(inst); }  ///< 插入分配空间后的形式参数
  void addPredecessor(BasicBlock *block) {
    if (std::find(predecessors.begin(), predecessors.end(), block) == predecessors.end()) {
      predecessors.push_back(block);
    }
  }  ///< 添加前驱
  void addSuccessor(BasicBlock *block) {
    if (std::find(successors.begin(), successors.end(), block) == successors.end()) {
      successors.push_back(block);
    }
  }  ///< 添加后继
  void addPredecessor(const block_list &blocks) {
    for (auto block : blocks) {
      addPredecessor(block);
    }
  }  ///< 添加多个前驱
  void addSuccessor(const block_list &blocks) {
    for (auto block : blocks) {
      addSuccessor(block);
    }
  }  ///< 添加多个后继
  void setIdom(BasicBlock *block) { idom = block; }
  void addSdoms(BasicBlock *block) { sdoms.push_back(block); }
  void clearSdoms() { sdoms.clear(); }
  // 重载1，参数为 BasicBlock*
  auto addDominants(BasicBlock *block) -> void { dominants.emplace(block); }
  // 重载2，参数为 block_set
  auto addDominants(const block_set &blocks) -> void { dominants.insert(blocks.begin(), blocks.end()); }
  auto setDominants(BasicBlock *block) -> void {
    dominants.clear();
    addDominants(block);
  }
  auto setDominants(const block_set &doms) -> void {
    dominants.clear();
    addDominants(doms);
  }
  auto setDFs(const block_set &df) -> void {
    dominant_frontiers.clear();
    for (auto elem : df) {
      dominant_frontiers.emplace(elem);
    }
  }
  void removePredecessor(BasicBlock *block) {
    auto iter = std::find(predecessors.begin(), predecessors.end(), block);
    if (iter != predecessors.end()) {
      predecessors.erase(iter);
    } else {
      assert(false);
    }
  }  ///< 删除前驱
  void removeSuccessor(BasicBlock *block) {
    auto iter = std::find(successors.begin(), successors.end(), block);
    if (iter != successors.end()) {
      successors.erase(iter);
    } else {
      assert(false);
    }
  }  ///< 删除后继
  void replacePredecessor(BasicBlock *oldBlock, BasicBlock *newBlock) {
    for (auto &predecessor : predecessors) {
      if (predecessor == oldBlock) {
        predecessor = newBlock;
        break;
      }
    }
  }  ///< 替换前驱
  // 获取支配树中该块的所有子节点，包括子节点的子节点等，迭代实现
  auto getChildren() -> block_list {
    std::queue<BasicBlock *> q;
    block_list children;
    for (auto sdom : sdoms) {
      q.push(sdom);
      children.push_back(sdom);
    }
    while (!q.empty()) {
      auto block = q.front();
      q.pop();
      for (auto sdom : block->sdoms) {
        q.push(sdom);
        children.push_back(sdom);
      }
    }

    return children;
  }

  auto setreachableTrue() -> void { reachable = true; }  ///< 设置可达

  auto setreachableFalse() -> void { reachable = false; }  ///< 设置不可达

  auto getreachable() -> bool { return reachable; }  ///< 返回可达状态

  static void conectBlocks(BasicBlock *prev, BasicBlock *next) {
    prev->addSuccessor(next);
    next->addPredecessor(prev);
  }  ///< 连接两个块，即设置两个基本块的前驱后继关系
  void setLoop(Loop *loop2set) { loopbelong = loop2set; }              ///< 设置所属循环
  auto getLoop() { return loopbelong; }                                ///< 获得所属循环
  void setLoopDepth(int loopdepth2set) { loopdepth = loopdepth2set; }  ///< 设置循环深度
  auto getLoopDepth() { return loopdepth; }                            ///< 获得其在循环的深度
  void removeInst(iterator pos) { instructions.erase(pos); }           ///< 删除指令
  auto moveInst(iterator sourcePos, iterator targetPos, BasicBlock *block) -> iterator;  ///< 移动指令
};

/**
 * @brief 使用者
 *
 * User是`Value`的派生类型，其使用其他的`Value`作为操作数。*/
class User : public Value {
 protected:
  std::vector<std::shared_ptr<Use>> operands;  ///< 操作数/使用关系

 protected:
  explicit User(Type *type, const std::string &name = "") : Value(type, name) {}

 public:
  auto getNumOperands() const -> unsigned { return operands.size(); }  ///< 获取操作数数量
  auto operand_begin() const { return operands.begin(); }              ///< 返回操作数列表的开头迭代器
  auto operand_end() const { return operands.end(); }                  ///< 返回操作数列表的结尾迭代器
  auto getOperands() const { return make_range(operand_begin(), operand_end()); }           ///< 获取操作数列表
  auto getOperand(unsigned index) const -> Value * { return operands[index]->getValue(); }  ///< 获取位置为index的操作数
  void addOperand(Value *value) {
    operands.emplace_back(std::make_shared<Use>(operands.size(), this, value));
    value->addUse(operands.back());
  }  ///< 增加操作数
  void removeOperand(unsigned index) {
    auto value = getOperand(index);
    value->removeUse(operands[index]);
    operands.erase(operands.begin() + index);
  }  ///< 移除操作数
  template <typename ContainerT>
  void addOperands(const ContainerT &newoperands) {
    for (auto value : newoperands) {
      addOperand(value);
    }
  }                                                   ///< 增加多个操作数
  void replaceOperand(unsigned index, Value *value);  ///< 替换操作数
  void setOperand(unsigned index, Value *value);      ///< 设置操作数
};

class GetSubArrayInst;

/**
 * @brief 左值
 *  具有地址的对象
 */
class LVal {
  friend class GetSubArrayInst;

 protected:
  LVal *fatherLVal{};                              ///< 父左值
  std::list<std::unique_ptr<LVal>> childrenLVals;  ///< 子左值
  GetSubArrayInst *defineInst{};                   /// 定义该左值的GetSubArray指令

 protected:
  LVal() = default;

 public:
  virtual ~LVal() = default;
  virtual auto getLValDims() const -> std::vector<Value *> = 0;  ///< 获取左值的维度
  virtual auto getLValNumDims() const -> unsigned = 0;           ///< 获取左值的维度数量

 public:
  auto getFatherLVal() const -> LVal * { return fatherLVal; }  ///< 获取父左值
  auto getChildrenLVals() const -> const std::list<std::unique_ptr<LVal>> & {
    return childrenLVals;
  }  ///< 获取子左值列表
  auto getAncestorLVal() const -> LVal * {
    auto curLVal = const_cast<LVal *>(this);
    while (curLVal->getFatherLVal() != nullptr) {
      curLVal = curLVal->getFatherLVal();
    }
    return curLVal;
  }                                                                  ///< 获取祖先左值
  void setFatherLVal(LVal *father) { fatherLVal = father; }          ///< 设置父左值
  void setDefineInst(GetSubArrayInst *inst) { defineInst = inst; }   ///< 设置定义指令
  void addChild(LVal *child) { childrenLVals.emplace_back(child); }  ///< 添加子左值
  void removeChild(LVal *child) {
    auto iter = std::find_if(childrenLVals.begin(), childrenLVals.end(),
                             [child](const std::unique_ptr<LVal> &ptr) { return ptr.get() == child; });
    childrenLVals.erase(iter);
  }                                                                       ///< 移除子左值
  auto getDefineInst() const -> GetSubArrayInst * { return defineInst; }  ///< 获取定义指令
};

/**
 * @brief 指令
 *
 * 所有具体指令类型的基类
 */
class Instruction : public User {
  friend class Function;

 public:
  /// 指令标识码
  enum Kind : uint64_t {
    kInvalid = 0x0UL,
    // Binary
    kAdd = 0x1UL << 0,
    kSub = 0x1UL << 1,
    kMul = 0x1UL << 2,
    kDiv = 0x1UL << 3,
    kRem = 0x1UL << 4,
    kICmpEQ = 0x1UL << 5,
    kICmpNE = 0x1UL << 6,
    kICmpLT = 0x1UL << 7,
    kICmpGT = 0x1UL << 8,
    kICmpLE = 0x1UL << 9,
    kICmpGE = 0x1UL << 10,
    kFAdd = 0x1UL << 11,
    kFSub = 0x1UL << 12,
    kFMul = 0x1UL << 13,
    kFDiv = 0x1UL << 14,
    kFCmpEQ = 0x1UL << 15,
    kFCmpNE = 0x1UL << 16,
    kFCmpLT = 0x1UL << 17,
    kFCmpGT = 0x1UL << 18,
    kFCmpLE = 0x1UL << 19,
    kFCmpGE = 0x1UL << 20,
    kAnd = 0x1UL << 21,
    kOr = 0x1UL << 22,
    // Unary
    kNeg = 0x1UL << 23,
    kNot = 0x1UL << 24,
    kFNeg = 0x1UL << 25,
    kFNot = 0x1UL << 26,
    kFtoI = 0x1UL << 27,
    kItoF = 0x1UL << 28,
    // call
    kCall = 0x1UL << 29,
    // terminator
    kCondBr = 0x1UL << 30,
    kBr = 0x1UL << 31,
    kReturn = 0x1UL << 32,
    // mem op
    kAlloca = 0x1UL << 33,
    kLoad = 0x1UL << 34,
    kStore = 0x1UL << 35,
    kLa = 0x1UL << 36,
    kMemset = 0x1UL << 37,
    kGetSubArray = 0x1UL << 38,
    // constant
    // kConstant = 0x1UL << 37,
    // phi
    kPhi = 0x1UL << 39,
    kBitItoF = 0x1UL << 40,
    kBitFtoI = 0x1UL << 41
  };

 protected:
  Kind kind;           ///< 指令标识码
  BasicBlock *parent;  ///< 指令从属的基本快

 protected:
  Instruction(Kind kind, Type *type, BasicBlock *parent = nullptr, const std::string &name = "")
      : User(type, name), kind(kind), parent(parent) {}

 public:
  auto getKind() const -> Kind { return kind; }  ///< 获取指令标识码
  auto getKindString() const -> std::string {
    switch (kind) {
      case kInvalid:
        return "Invalid";
      case kAdd:
        return "Add";
      case kSub:
        return "Sub";
      case kMul:
        return "Mul";
      case kDiv:
        return "Div";
      case kRem:
        return "Rem";
      case kICmpEQ:
        return "ICmpEQ";
      case kICmpNE:
        return "ICmpNE";
      case kICmpLT:
        return "ICmpLT";
      case kICmpGT:
        return "ICmpGT";
      case kICmpLE:
        return "ICmpLE";
      case kICmpGE:
        return "ICmpGE";
      case kFAdd:
        return "FAdd";
      case kFSub:
        return "FSub";
      case kFMul:
        return "FMul";
      case kFDiv:
        return "FDiv";
      case kFCmpEQ:
        return "FCmpEQ";
      case kFCmpNE:
        return "FCmpNE";
      case kFCmpLT:
        return "FCmpLT";
      case kFCmpGT:
        return "FCmpGT";
      case kFCmpLE:
        return "FCmpLE";
      case kFCmpGE:
        return "FCmpGE";
      case kAnd:
        return "And";
      case kOr:
        return "Or";
      case kNeg:
        return "Neg";
      case kNot:
        return "Not";
      case kFNeg:
        return "FNeg";
      case kFNot:
        return "FNot";
      case kFtoI:
        return "FtoI";
      case kItoF:
        return "IToF";
      case kCall:
        return "Call";
      case kCondBr:
        return "CondBr";
      case kBr:
        return "Br";
      case kReturn:
        return "Return";
      case kAlloca:
        return "Alloca";
      case kLoad:
        return "Load";
      case kStore:
        return "Store";
      case kLa:
        return "La";
      case kMemset:
        return "Memset";
      case kPhi:
        return "Phi";
      case kGetSubArray:
        return "GetSubArray";
      default:
        return "Unknown";
    }
  }                                                          ///< 根据指令标识码获取字符串
  auto getParent() const -> BasicBlock * { return parent; }  ///< 获取父基本块
  void setParent(BasicBlock *bb) { parent = bb; }            ///< 设置父基本块

  auto isBinary() const -> bool {
    static constexpr uint64_t BinaryOpMask =
        (kAdd | kSub | kMul | kDiv | kRem) | (kICmpEQ | kICmpNE | kICmpLT | kICmpGT | kICmpLE | kICmpGE) |
        (kFAdd | kFSub | kFMul | kFDiv) | (kFCmpEQ | kFCmpNE | kFCmpLT | kFCmpGT | kFCmpLE | kFCmpGE) | (kAnd | kOr);
    return (kind & BinaryOpMask) != 0U;
  }  ///< 判断指令是否为二元指令
  auto isUnary() const -> bool {
    static constexpr uint64_t UnaryOpMask = kNeg | kNot | kFNeg | kFNot | kFtoI | kItoF;
    return (kind & UnaryOpMask) != 0U;
  }  ///< 判断指令是否为一元指令
  auto isMemory() const -> bool {
    static constexpr uint64_t MemoryOpMask = kAlloca | kLoad | kStore;
    return (kind & MemoryOpMask) != 0U;
  }  ///< 判断指令是否与内存有关
  auto isTerminator() const -> bool {
    static constexpr uint64_t TerminatorOpMask = kCondBr | kBr | kReturn;
    return (kind & TerminatorOpMask) != 0U;
  }  ///< 是否为基本块末尾指令
  auto isCmp() const -> bool {
    static constexpr uint64_t CmpOpMask = (kICmpEQ | kICmpNE | kICmpLT | kICmpGT | kICmpLE | kICmpGE) |
                                          (kFCmpEQ | kFCmpNE | kFCmpLT | kFCmpGT | kFCmpLE | kFCmpGE);
    return (kind & CmpOpMask) != 0U;
  }  ///< 判断指令是否与比较有关
  auto isBranch() const -> bool {
    static constexpr uint64_t BranchOpMask = kBr | kCondBr;
    return (kind & BranchOpMask) != 0U;
  }  ///< 判断指令是否与分支有关
  auto isCommutative() const -> bool {
    static constexpr uint64_t CommutativeOpMask =
        kAdd | kMul | kICmpEQ | kICmpNE | kFAdd | kFMul | kFCmpEQ | kFCmpNE | kAnd | kOr;
    return (kind & CommutativeOpMask) != 0U;
  }                                                                    ///< 判断指令是否是可交换的
  auto isUnconditional() const -> bool { return kind == kBr; }         ///< 是否是无条件跳转指令
  auto isConditional() const -> bool { return kind == kCondBr; }       ///< 是否是有条件跳转指令
  auto isPhi() const -> bool { return kind == kPhi; }                  ///< 是否为Phi指令
  auto isAlloca() const -> bool { return kind == kAlloca; }            ///< 是否是分配指令
  auto isStore() const -> bool { return kind == kStore; }              ///< 是否是存储指令
  auto isLoad() const -> bool { return kind == kLoad; }                ///< 是否是加载指令
  auto isMemset() const -> bool { return kind == kMemset; }            ///< 是否是Memset指令
  auto isLa() const -> bool { return kind == kLa; }                    ///< 是否为La指令
  auto isGetSubArray() const -> bool { return kind == kGetSubArray; }  ///< 是否为GetSubArray指令
  auto isDefine() const -> bool {
    static constexpr uint64_t DefineOpMask = kAlloca | kStore | kPhi;
    return (kind & DefineOpMask) != 0U;
  }                                                          ///< 是否为Define指令
  auto isCall() const -> bool { return kind == kCall; }      ///< 是否为Call指令
  auto isReturn() const -> bool { return kind == kReturn; }  ///< 是否为Return指令
};

class Function;

/**
 * @brief 获取地址指令，其可获取部分数组的地址
 *
 */
class LaInst : public Instruction {
  friend class Function;
  friend class IRBuilder;
  friend class Function;

 protected:
  explicit LaInst(Value *pointer, const std::vector<Value *> &indices = {}, BasicBlock *parent = nullptr,
                  const std::string &name = "")
      : Instruction(Kind::kLa, pointer->getType(), parent, name) {
    assert(pointer);
    addOperand(pointer);
    addOperands(indices);
  }

 public:
  auto getNumIndices() const -> unsigned { return getNumOperands() - 1; }  ///< 获取索引长度
  auto getPointer() const -> Value * { return getOperand(0); }             ///< 获取目标变量的Value指针
  auto getIndices() const { return make_range(std::next(operand_begin()), operand_end()); }  ///< 获取索引列表
  auto getIndex(unsigned index) const -> Value * { return getOperand(index + 1); }  ///< 获取位置为index的索引分量
};

//! Phi Instruction
class PhiInst : public Instruction {
  friend class IRBuilder;
  friend class Function;
  friend class SysySSA;

 protected:
  Value *map_val;  // Phi的旧值保留在这，用以根据value寻找旧的映射关系

  PhiInst(Type *type, Value *lhs, const std::vector<Value *> &rhs, Value *mval, BasicBlock *parent,
          const std::string &name = "")
      : Instruction(Kind::kPhi, type, parent, name) {
    map_val = mval;
    addOperand(lhs);
    addOperands(rhs);
  }

 public:
  // 待添加的方法
  auto getMapVal() -> Value * { return map_val; }
  auto getPointer() const -> Value * { return getOperand(0); }
  auto getValues() { return make_range(std::next(operand_begin()), operand_end()); }
  auto getValue(unsigned index) const -> Value * { return getOperand(index + 1); }
};

/**
 * @brief 调用指令
 */
class CallInst : public Instruction {
  friend class Function;
  friend class IRBuilder;
  friend class Function;

 protected:
  explicit CallInst(Function *callee, const std::vector<Value *> &args = {}, BasicBlock *parent = nullptr,
                    const std::string &name = "");

 public:
  auto getCallee() -> Function *;                                                        ///< 获取被调用函数
  auto getArguments() { return make_range(std::next(operand_begin()), operand_end()); }  ///< 获取参数列表
};

/**
 * @brief 一元指令
 */
class UnaryInst : public Instruction {
  friend class IRBuilder;
  friend class Function;

 protected:
  UnaryInst(Kind kind, Type *type, Value *operand, BasicBlock *parent = nullptr, const std::string &name = "")
      : Instruction(kind, type, parent, name) {
    addOperand(operand);
  }

 public:
  auto getOperand() const -> Value * { return User::getOperand(0); }  ///< 获取操作数
};

/**
 * @brief 二元指令
 */
class BinaryInst : public Instruction {
  friend class IRBuilder;
  friend class Function;

 protected:
  BinaryInst(Kind kind, Type *type, Value *lhs, Value *rhs, BasicBlock *parent, const std::string &name = "")
      : Instruction(kind, type, parent, name) {
    addOperand(lhs);
    addOperand(rhs);
  }

 public:
  auto getLhs() const -> Value * { return getOperand(0); }  ///< 获取左操作数
  auto getRhs() const -> Value * { return getOperand(1); }  ///< 获取右操作数
  template <typename T>
  auto eval(T lhs, T rhs) -> T {
    switch (getKind()) {
      case kAdd:
        return lhs + rhs;
      case kSub:
        return lhs - rhs;
      case kMul:
        return lhs * rhs;
      case kDiv:
        return lhs / rhs;
      case kRem:
        if constexpr (std::is_floating_point<T>::value) {
          throw std::runtime_error("Remainder operation not supported for floating point types.");
        } else {
          return lhs % rhs;
        }
      case kICmpEQ:
        return lhs == rhs;
      case kICmpNE:
        return lhs != rhs;
      case kICmpLT:
        return lhs < rhs;
      case kICmpGT:
        return lhs > rhs;
      case kICmpLE:
        return lhs <= rhs;
      case kICmpGE:
        return lhs >= rhs;
      case kFAdd:
        return lhs + rhs;
      case kFSub:
        return lhs - rhs;
      case kFMul:
        return lhs * rhs;
      case kFDiv:
        return lhs / rhs;
      case kFCmpEQ:
        return lhs == rhs;
      case kFCmpNE:
        return lhs != rhs;
      case kFCmpLT:
        return lhs < rhs;
      case kFCmpGT:
        return lhs > rhs;
      case kFCmpLE:
        return lhs <= rhs;
      case kFCmpGE:
        return lhs >= rhs;
      case kAnd:
        return lhs && rhs;
      case kOr:
        return lhs || rhs;
      default:
        assert(false);
    }
  }  ///< 根据指令类型进行二元计算，eval template模板实现
};

/**
 * @brief 返回指令
 */
class ReturnInst : public Instruction {
  friend class IRBuilder;
  friend class Function;

 protected:
  explicit ReturnInst(Value *value = nullptr, BasicBlock *parent = nullptr, const std::string &name = "")
      : Instruction(kReturn, Type::getVoidType(), parent, name) {
    if (value != nullptr) {
      addOperand(value);
    }
  }

 public:
  auto hasReturnValue() const -> bool { return !operands.empty(); }                              ///< 是否有返回值
  auto getReturnValue() const -> Value * { return hasReturnValue() ? getOperand(0) : nullptr; }  ///< 获取返回值
};

/**
 * @brief 无条件跳转指令
 */
class UncondBrInst : public Instruction {
  friend class IRBuilder;
  friend class Function;

 protected:
  UncondBrInst(BasicBlock *block, const std::vector<Value *> &args, BasicBlock *parent = nullptr)
      : Instruction(kBr, Type::getVoidType(), parent, "") {
    // assert(block->getNumArguments() == args.size());
    // assert(block->getNumArguments() == args.size());
    addOperand(block);
    addOperands(args);
  }

 public:
  auto getBlock() const -> BasicBlock * { return dynamic_cast<BasicBlock *>(getOperand(0)); }  ///< 获取目标基本快
  auto getArguments() const { return make_range(std::next(operands.begin()), operands.end()); }  ///< 获取参数列表
};

/**
 * @brief 条件跳转指令
 */
class CondBrInst : public Instruction {
  friend class IRBuilder;
  friend class Function;

 protected:
  CondBrInst(Value *condition, BasicBlock *thenBlock, BasicBlock *elseBlock, const std::vector<Value *> &thenArgs,
             const std::vector<Value *> &elseArgs, BasicBlock *parent = nullptr)
      : Instruction(kCondBr, Type::getVoidType(), parent, "") {
    // assert(thenBlock->getNumArguments() == thenArgs.size() && elseBlock->getNumArguments() == elseArgs.size());
    addOperand(condition);
    addOperand(thenBlock);
    addOperand(elseBlock);
    addOperands(thenArgs);
    addOperands(elseArgs);
  }

 public:
  auto getCondition() const -> Value * { return getOperand(0); }  ///< 获取跳转条件
  auto getThenBlock() const -> BasicBlock * { return dynamic_cast<BasicBlock *>(getOperand(1)); }  ///< 获取thenBlock *
  auto getElseBlock() const -> BasicBlock * { return dynamic_cast<BasicBlock *>(getOperand(2)); }  ///< 获取elseBlock *
  auto getThenArguments() const {
    auto begin = operands.begin() + 3;
    auto end = begin + getThenBlock()->getNumArguments();
    return make_range(begin, end);
  }  ///< 获取thenBlock的参数列表
  auto getElseArguments() const {
    auto begin = operands.begin() + 3 + getThenBlock()->getNumArguments();
    auto end = operands.end();
    return make_range(begin, end);
  }  ///< 获取elseBlock的参数列表
};

/**
 * @brief 分配指令
 */
class AllocaInst : public Instruction, public LVal {
  friend class Function;
  friend class IRBuilder;
  friend class Function;

 protected:
  explicit AllocaInst(Type *type, const std::vector<Value *> &dims = {}, BasicBlock *parent = nullptr,
                      const std::string &name = "")
      : Instruction(kAlloca, type, parent, name) {
    addOperands(dims);
  }

 public:
  auto getLValDims() const -> std::vector<Value *> override {
    std::vector<Value *> dims;
    for (const auto &dim : getOperands()) {
      dims.emplace_back(dim->getValue());
    }
    return dims;
  }  ///< 获取作为左值的维度数组
  auto getLValNumDims() const -> unsigned override { return getNumOperands(); }
  auto getNumDims() const -> unsigned { return getNumOperands(); }      ///< 获取维度数量
  auto getDims() const { return getOperands(); }                        ///< 获取维度列表
  auto getDim(unsigned index) -> Value * { return getOperand(index); }  ///< 获取位置为index的列表
};

/**
 * @brief 获取子数组指令
 *
 */
class GetSubArrayInst : public Instruction {
  friend class IRBuilder;
  friend class Function;

 public:
  GetSubArrayInst(LVal *fatherArray, LVal *childArray, const std::vector<Value *> &indices,
                  BasicBlock *parent = nullptr, const std::string &name = "")
      : Instruction(Kind::kGetSubArray, Type::getVoidType(), parent, name) {
    auto predicate = [childArray](const std::unique_ptr<LVal> &child) -> bool { return child.get() == childArray; };
    if (std::find_if(fatherArray->childrenLVals.begin(), fatherArray->childrenLVals.end(), predicate) ==
        fatherArray->childrenLVals.end()) {
      fatherArray->childrenLVals.emplace_back(childArray);
    }
    childArray->fatherLVal = fatherArray;
    childArray->defineInst = this;
    auto fatherArrayValue = dynamic_cast<Value *>(fatherArray);
    auto childArrayValue = dynamic_cast<Value *>(childArray);
    assert(fatherArrayValue);
    assert(childArrayValue);
    addOperand(fatherArrayValue);
    addOperand(childArrayValue);
    addOperands(indices);
  }

 public:
  auto getFatherArray() const -> Value * { return getOperand(0); }                              ///< 获取父数组
  auto getChildArray() const -> Value * { return getOperand(1); }                               ///< 获取子数组
  auto getFatherLVal() const -> LVal * { return dynamic_cast<LVal *>(getOperand(0)); }          ///< 获取父左值
  auto getChildLVal() const -> LVal * { return dynamic_cast<LVal *>(getOperand(1)); }           ///< 获取子左值
  auto getIndices() const { return make_range(std::next(operand_begin(), 2), operand_end()); }  ///< 获取索引
  auto getNumIndices() const -> unsigned { return getNumOperands() - 2; }                       ///< 获取索引数量
};

/**
 * @brief 加载指令
 */
class LoadInst : public Instruction {
  friend class IRBuilder;
  friend class Function;

 protected:
  explicit LoadInst(Value *pointer, const std::vector<Value *> &indices = {}, BasicBlock *parent = nullptr,
                    const std::string &name = "")
      : Instruction(kLoad, pointer->getType()->as<PointerType>()->getBaseType(), parent, name) {
    assert(pointer);
    addOperand(pointer);
    addOperands(indices);
  }

 public:
  auto getNumIndices() const -> unsigned { return getNumOperands() - 1; }  ///< 获取索引长度
  auto getPointer() const -> Value * { return getOperand(0); }             ///< 获取目标变量的Value指针
  auto getIndices() const { return make_range(std::next(operand_begin()), operand_end()); }  ///< 获取索引列表
  auto getIndex(unsigned index) const -> Value * { return getOperand(index + 1); }  ///< 获取位置为index的索引分量
  auto getAncestorIndices() const -> std::list<Value *> {
    std::list<Value *> indices;
    for (const auto &index : getIndices()) {
      indices.emplace_back(index->getValue());
    }
    auto curPointer = dynamic_cast<LVal *>(getPointer());
    while (curPointer->getFatherLVal() != nullptr) {
      auto inserter = std::next(indices.begin());
      for (const auto &index : curPointer->getDefineInst()->getIndices()) {
        indices.insert(inserter, index->getValue());
      }
      curPointer = curPointer->getFatherLVal();
    }

    return indices;
  }  ///< 获取相对于祖先数组的索引列表
};

/**
 * @brief 存储指令
 */
class StoreInst : public Instruction {
  friend class IRBuilder;
  friend class Function;

 protected:
  StoreInst(Value *value, Value *pointer, const std::vector<Value *> &indices = {}, BasicBlock *parent = nullptr,
            const std::string &name = "")
      : Instruction(kStore, Type::getVoidType(), parent, name) {
    addOperand(value);
    addOperand(pointer);
    addOperands(indices);
  }

 public:
  auto getNumIndices() const -> unsigned { return getNumOperands() - 2; }  ///< 获取索引长度
  auto getValue() const -> Value * { return getOperand(0); }               ///< 获取要存入的值
  auto getPointer() const -> Value * { return getOperand(1); }             ///< 获取目标变量的Value指针
  auto getIndices() const { return make_range(operand_begin() + 2, operand_end()); }  ///< 获取索引列表
  auto getIndex(unsigned index) const -> Value * { return getOperand(index + 2); }  ///< 获取位置为index的索引分量
  auto getAncestorIndices() const -> std::list<Value *> {
    std::list<Value *> indices;
    for (const auto &index : getIndices()) {
      indices.emplace_back(index->getValue());
    }
    auto curPointer = dynamic_cast<LVal *>(getPointer());
    while (curPointer->getFatherLVal() != nullptr) {
      auto inserter = std::next(indices.begin());
      for (const auto &index : curPointer->getDefineInst()->getIndices()) {
        indices.insert(inserter, index->getValue());
      }
      curPointer = curPointer->getFatherLVal();
    }

    return indices;
  }  ///< 获取相对于祖先数组的索引列表
};

/**
 * @brief Memset指令
 */
class MemsetInst : public Instruction {
  friend class Function;
  friend class IRBuilder;
  friend class Function;

 protected:
  MemsetInst(Value *pointer, Value *begin, Value *size, Value *value, BasicBlock *parent = nullptr,
             const std::string &name = "")
      : Instruction(kMemset, Type::getVoidType(), parent, name) {
    addOperand(pointer);
    addOperand(begin);
    addOperand(size);
    addOperand(value);
  }

 public:
  auto getPointer() const -> Value * { return getOperand(0); }  ///< 获取目标变量的Value指针
  auto getBegin() const -> Value * { return getOperand(1); }    ///< 获取存入的开始地址
  auto getSize() const -> Value * { return getOperand(2); }     ///< 获取存入数量
  auto getValue() const -> Value * { return getOperand(3); }    ///< 获取要存入的值
};

class Module;

// class SCEV {
//  private:
//   std::vector<Value *> scevVal;
//   std::vector<Instruction *> scevIns;
//   std::set<BinaryInst *> instructionsHasBeenCaculated;

//  public:
//   SCEV() = default;

// };
class GlobalValue;

/**
 * @brief  循环结构体，记录着循环的信息，如preheader,header等
 */
class Loop {
  friend class Function;

 public:
  using block_list = std::vector<BasicBlock *>;
  using block_set = std::unordered_set<BasicBlock *>;
  using Loop_list = std::vector<Loop *>;

 protected:
  Function *parent;                      ///< 循环所在的Function
  block_list blocksInLoop;               ///< 循环内的所有block
  BasicBlock *preheaderBlock = nullptr;  ///< 进入循环前的块， preheaderblock
  BasicBlock *headerBlock = nullptr;     ///< 循环头块
  block_list latchBlock;                 ///<循环回边
  block_set exitingBlocks;               ///< 退出循环判断块
  block_set exitBlocks;                  ///< 循环退出后的块
  Loop *parentloop = nullptr;            ///< 父循环，即上层循环
  Loop_list subLoops;                    ///< 子循环，即下层循环
  size_t loopID;                         ///< 循环ID
  unsigned loopDepth;                    ///< 循环深度

  Instruction *indCondVar = nullptr;  ///< 循环判断Instruction
  Instruction::Kind IcmpKind;         ///< 循环判断类型
  Value *indEnd = nullptr;            ///< 循环结束值
  AllocaInst *IndPhi = nullptr;       ///< 循环变量

  ConstantValue *indBegin = nullptr;  ///< 循环开始值
  ConstantValue *indStep = nullptr;   ///< 循环结束值

  std::set<GlobalValue *> GlobalValuechange;  ///< 该循环内改变的全局变量

  int StepType = 0;  ///<  循环变量规则， 1 为加 2 为减 其他为0

  bool parallelable = false;  ///< 是否可并行化

 public:
  explicit Loop(BasicBlock *header, const std::string &name = "") : headerBlock(header) {
    blocksInLoop.push_back(header);
  }
  void setloopID() {
    static unsigned loopCount = 0;
    loopCount = loopCount + 1;
    loopID = loopCount;
  }                                                                           ///<  设置循环ID
  auto getindBegin() { return indBegin; }                                     ///< 获得循环开始值
  auto getindStep() { return indStep; }                                       ///< 获得循环步长
  auto setindBegin(ConstantValue *indBegin2set) { indBegin = indBegin2set; }  ///< 设置循环开始值
  auto setindStep(ConstantValue *indStep2set) { indStep = indStep2set; }      ///< 设置循环步长
  auto setStepType(int StepType2Set) { StepType = StepType2Set; }             ///< 设置循环变量规则
  auto getStepType() { return StepType; }                                     ///< 获得循环变量规则

  auto getLoopID() -> size_t { return loopID; }                        ///<  获得循环ID
  auto getHeader() -> BasicBlock * { return headerBlock; }             ///< 获得循环头结点
  auto getPreheaderBlock() -> BasicBlock * { return preheaderBlock; }  ///<  获得循环前的块，即preheaderblock
  auto getlatchBlock() -> block_list & { return latchBlock; }          ///< 获得循环回边
  auto getexitingBlocks() -> block_set & { return exitingBlocks; }     ///< 获得退出循环判断块
  auto getexitBlocks() -> block_set & { return exitBlocks; }           ///< 获得循环退出后的块
  auto getParentLoop() -> Loop * { return parentloop; }                ///< 获得父循环
  auto setParentLoop(Loop *parentLoop2Set) -> void { parentloop = parentLoop2Set; }  ///< 设置父循环
  auto addBasicBlock(BasicBlock *basicBlock2add) -> void {
    blocksInLoop.push_back(basicBlock2add);
  }                                                                                ///< 对循环内添加块
  auto addSubLoop(Loop *subLoop2add) -> void { subLoops.push_back(subLoop2add); }  ///< 添加子循环
  auto setLoopDepth(unsigned depth2Set) -> void { loopDepth = depth2Set; }         ///< 设置循环深度
  auto getBasicBlocks() -> block_list & { return blocksInLoop; }                   ///< 获得循环内所有块
  auto getSubLoops() -> Loop_list & { return subLoops; }                           ///< 获得子循环
  auto getLoopDepth() -> unsigned { return loopDepth; }                            ///< 获得循环深度
  auto isBasicBlockInLoop(BasicBlock *basicBlocktoCheck) -> bool {
    return std::any_of(blocksInLoop.begin(), blocksInLoop.end(),
                       [basicBlocktoCheck](const BasicBlock *basicblock) { return basicblock == basicBlocktoCheck; });
  }  ///<  判断输入块是否在该循环内
  auto addExitingBlocks(BasicBlock *basicBlock2Add) { exitingBlocks.insert(basicBlock2Add); }  ///< 添加循环退出判断块
  auto addExitBlocks(BasicBlock *basicBlock2Add) { exitBlocks.insert(basicBlock2Add); }  ///< 添加循环退出后的块
  auto addLatchBlock(BasicBlock *basicBlock2Set) { latchBlock.push_back(basicBlock2Set); }  ///< 添加回边
  auto setPredHeader(BasicBlock *predHeader2Set) { preheaderBlock = predHeader2Set; }       ///< 设置preheaderblock
  auto setIndexCondInstr(Instruction *IndexCondInstr2set) {
    indCondVar = IndexCondInstr2set;
  }                                                                      ///< 设置循环判断Instruction
  auto setIcmpKind(Instruction::Kind kind2set) { IcmpKind = kind2set; }  ///< 设置判断类型
  auto getIcmpKind() { return IcmpKind; }                                ///< 获得判断类型
  auto isSimpleLoopInvariant(Value *value) -> bool;  ///< 判断是否为简单循环不变量，若其在loop中，则不是。
  auto setindEnd(Value *indEnd2Set) { indEnd = indEnd2Set; }       ///< 设置循环结束值
  auto setindPhi(AllocaInst *indPhi2Set) { IndPhi = indPhi2Set; }  ///<设置循环变量
  auto getindEnd() -> Value * { return indEnd; }                   ///<获得循环结束值
  auto getindPhi() -> AllocaInst * { return IndPhi; }              ///<获得循环变量
  auto getindCondVar() { return indCondVar; }                      ///<获得循环判断Instruction

  auto addGlobalValuechange(GlobalValue *globalvaluechange2add) {
    GlobalValuechange.insert(globalvaluechange2add);
  }  ///<添加在循环中改变的全局变量
  auto getGlobalValuechange() -> std::set<GlobalValue *> & {
    return GlobalValuechange;
  }  ///<获得在循环中改变的所有全局变量
  auto setParallelableTrue() -> void { parallelable = true; }
  auto setParallelableFalse() -> void { parallelable = false; }
  auto getParallelable() -> bool { return parallelable; }
};
/**
 * @brief 函数
 */
class Function : public Value {
  friend class Module;

 protected:
  Function(Module *parent, Type *type, const std::string &name) : Value(type, name), parent(parent) {
    blocks.emplace_back(new BasicBlock(this));
  }

 public:
  using block_list = std::list<std::unique_ptr<BasicBlock>>;
  using Loop_list = std::list<std::unique_ptr<Loop>>;

  /// 指令标识码
  enum FunctionAttribute : uint64_t {
    PlaceHolder = 0x0UL,
    Pure = 0x1UL << 0,
    SelfRecursive = 0x1UL << 1,
    SideEffect = 0x1UL << 2,
    NoPureCauseMemRead = 0x1UL << 3
  };

 protected:
  Module *parent;                                          ///< 函数的父模块
  block_list blocks;                                       ///< 函数包含的基本块列表
  Loop_list loops;                                         ///< 函数包含的循环列表
  Loop_list topLoops;                                      ///< 函数所包含的顶层循环；
  std::list<std::unique_ptr<AllocaInst>> indirectAllocas;  ///< 函数中mem2reg引入的间接分配的内存

  FunctionAttribute attribute = PlaceHolder;  ///< 函数属性
  std::set<Function *> callees;               ///< 函数调用的函数集合

  std::unordered_map<BasicBlock *, Loop *> basicblock2Loop;
  std::unordered_map<Value *, BasicBlock *> value2AllocBlocks;  ///< value -- alloc block mapping
  std::unordered_map<Value *, std::unordered_map<BasicBlock *, int>>
      value2DefBlocks;  //< value -- define blocks mapping
  std::unordered_map<Value *, std::unordered_map<BasicBlock *, int>> value2UseBlocks;  //< value -- use blocks mapping

 public:
  static auto getcloneIndex() -> unsigned {
    static unsigned cloneIndex = 0;
    cloneIndex += 1;
    return cloneIndex - 1;
  }
  auto clone(const std::string &suffix = "_" + std::to_string(getcloneIndex()) + "@") const
      -> Function *;  ///< 复制函数
  auto getCallees() -> const std::set<Function *> & { return callees; }
  auto addCallee(Function *callee) -> void { callees.insert(callee); }
  auto removeCallee(Function *callee) -> void { callees.erase(callee); }
  auto clearCallees() -> void { callees.clear(); }
  auto getCalleesWithNoExternalAndSelf() -> std::set<Function *>;
  auto getAttribute() const -> FunctionAttribute { return attribute; }  ///< 获取函数属性
  auto setAttribute(FunctionAttribute attr) -> void {
    attribute = static_cast<FunctionAttribute>(attribute | attr);
  }                                                           ///< 设置函数属性
  auto clearAttribute() -> void { attribute = PlaceHolder; }  ///< 清楚所有函数属性，只保留PlaceHolder
  auto getLoopOfBasicBlock(BasicBlock *bb) -> Loop * {
    return basicblock2Loop.count(bb) != 0 ? basicblock2Loop[bb] : nullptr;
  }  ///< 获得块所在循环
  auto getLoopDepthByBlock(BasicBlock *basicblock2Check) {
    if (getLoopOfBasicBlock(basicblock2Check) != nullptr) {
      auto loop = getLoopOfBasicBlock(basicblock2Check);
      return loop->getLoopDepth();
    }
    return static_cast<unsigned>(0);
  }  ///< 通过块，获得其所在循环深度
  auto addBBToLoop(BasicBlock *bb, Loop *LoopToadd) { basicblock2Loop[bb] = LoopToadd; }  ///< 添加块与循环的映射
  auto getBBToLoopRef() -> std::unordered_map<BasicBlock *, Loop *> & {
    return basicblock2Loop;
  }  ///< 获得块-循环映射表
  // auto getNewLoopPtr(BasicBlock *header) -> Loop * { return new Loop(header); }
  auto getReturnType() const -> Type * { return getType()->as<FunctionType>()->getReturnType(); }  ///< 获取返回值类型
  auto getParamTypes() const { return getType()->as<FunctionType>()->getParamTypes(); }  ///< 获取形式参数类型列表
  auto getBasicBlocks() { return make_range(blocks); }                                   ///< 获取基本块列表
  auto getBasicBlocks_NoRange() -> block_list & { return blocks; }
  auto getEntryBlock() -> BasicBlock * { return blocks.front().get(); }  ///< 获取入口块
  auto removeBasicBlock(BasicBlock *blockToRemove) {
    auto is_same_ptr = [blockToRemove](const std::unique_ptr<BasicBlock> &ptr) { return ptr.get() == blockToRemove; };
    blocks.remove_if(is_same_ptr);
    // blocks.erase(std::remove_if(blocks.begin(), blocks.end(), is_same_ptr), blocks.end());
  }  ///< 将该块从function的blocks中删除
  // auto getBasicBlocks_NoRange() -> block_list & { return blocks; }
  auto addBasicBlock(const std::string &name = "") -> BasicBlock * {
    blocks.emplace_back(new BasicBlock(this, name));
    return blocks.back().get();
  }  ///< 添加新的基本块
  auto addBasicBlock(BasicBlock *block) -> BasicBlock * {
    blocks.emplace_back(block);
    return block;
  }  ///< 添加基本块到blocks中
  auto addBasicBlockFront(BasicBlock *block) -> BasicBlock * {
    blocks.emplace_front(block);
    return block;
  }  // 从前端插入新的基本块
  /** value -- alloc blocks mapping */
  auto addValue2AllocBlocks(Value *value, BasicBlock *block) -> void {
    value2AllocBlocks[value] = block;
  }  ///< 添加value -- alloc block mapping
  auto getAllocBlockByValue(Value *value) -> BasicBlock * {
    if (value2AllocBlocks.count(value) > 0) {
      return value2AllocBlocks[value];
    }
    return nullptr;
  }  ///< 通过value获取alloc block
  auto getValue2AllocBlocks() -> std::unordered_map<Value *, BasicBlock *> & {
    return value2AllocBlocks;
  }  ///< 获取所有value -- alloc block mappings
  auto removeValue2AllocBlock(Value *value) -> void {
    value2AllocBlocks.erase(value);
  }  ///< 删除value -- alloc block mapping
  /** value -- define blocks mapping */
  auto addValue2DefBlocks(Value *value, BasicBlock *block) -> void {
    ++value2DefBlocks[value][block];
  }  ///< 添加value -- define block mapping
  // keep in mind that the return is not a reference.
  auto getDefBlocksByValue(Value *value) -> std::unordered_set<BasicBlock *> {
    std::unordered_set<BasicBlock *> blocks;
    if (value2DefBlocks.count(value) > 0) {
      for (const auto &pair : value2DefBlocks[value]) {
        blocks.insert(pair.first);
      }
    }
    return blocks;
  }  ///< 通过value获取define blocks
  auto getValue2DefBlocks() -> std::unordered_map<Value *, std::unordered_map<BasicBlock *, int>> & {
    return value2DefBlocks;
  }  ///< 获取所有value -- define blocks mappings
  auto removeValue2DefBlock(Value *value, BasicBlock *block) -> bool {
    bool changed = false;
    if (--value2DefBlocks[value][block] == 0) {
      value2DefBlocks[value].erase(block);
      if (value2DefBlocks[value].empty()) {
        value2DefBlocks.erase(value);
        changed = true;
      }
    }
    return changed;
  }  ///< 删除value -- define block mapping
  auto getValuesOfDefBlock() -> std::unordered_set<Value *> {
    std::unordered_set<Value *> values;
    for (const auto &pair : value2DefBlocks) {
      values.insert(pair.first);
    }
    return values;
  }  ///< 获取所有定义过的value
  /** value -- use blocks mapping */
  auto addValue2UseBlocks(Value *value, BasicBlock *block) -> void {
    ++value2UseBlocks[value][block];
  }  ///< 添加value -- use block mapping
  // keep in mind that the return is not a reference.
  auto getUseBlocksByValue(Value *value) -> std::unordered_set<BasicBlock *> {
    std::unordered_set<BasicBlock *> blocks;
    if (value2UseBlocks.count(value) > 0) {
      for (const auto &pair : value2UseBlocks[value]) {
        blocks.insert(pair.first);
      }
    }
    return blocks;
  }  ///< 通过value获取use blocks
  auto getValue2UseBlocks() -> std::unordered_map<Value *, std::unordered_map<BasicBlock *, int>> & {
    return value2UseBlocks;
  }  ///< 获取所有value -- use blocks mappings
  auto removeValue2UseBlock(Value *value, BasicBlock *block) -> bool {
    bool changed = false;
    if (--value2UseBlocks[value][block] == 0) {
      value2UseBlocks[value].erase(block);
      if (value2UseBlocks[value].empty()) {
        value2UseBlocks.erase(value);
        changed = true;
      }
    }
    return changed;
  }  ///< 删除value -- use block mapping
  auto addIndirectAlloca(AllocaInst *alloca) { indirectAllocas.emplace_back(alloca); }  ///< 添加间接分配
  auto getIndirectAllocas() -> std::list<std::unique_ptr<AllocaInst>> & {
    return indirectAllocas;
  }  ///< 获取间接分配列表

  /** loop -- begin  */

  void addLoop(Loop *loop) { loops.emplace_back(loop); }        ///< 添加循环（非顶层）
  void addTopLoop(Loop *loop) { topLoops.emplace_back(loop); }  ///< 添加顶层循环
  auto getLoops() -> Loop_list & { return loops; }              ///< 获得循环（非顶层）
  auto getTopLoops() -> Loop_list & { return topLoops; }        ///< 获得顶层循环
  /** loop -- end    */
};

/**
 * @brief 全局变量
 */
class GlobalValue : public User, public LVal {
  friend class Module;

 protected:
  Module *parent;           ///< 父模块
  unsigned numDims;         ///< 维度数量
  ValueCounter initValues;  ///< 初值

 protected:
  /**
   * float/int类型的GlobalValue的默认初值为0/0.0F
   */
  GlobalValue(Module *parent, Type *type, const std::string &name, const std::vector<Value *> &dims = {},
              ValueCounter init = {})
      : User(type, name), parent(parent) {
    assert(type->isPointer());
    addOperands(dims);
    numDims = dims.size();
    if (init.size() == 0) {
      unsigned num = 1;
      for (unsigned i = 0; i < numDims; i++) {
        num *= dynamic_cast<ConstantValue *>(dims[i])->getInt();
      }

      if (dynamic_cast<PointerType *>(type)->getBaseType() == Type::getFloatType()) {
        init.push_back(ConstantValue::get(0.0F), num);
      } else {
        init.push_back(ConstantValue::get(0), num);
      }
    }
    initValues = init;
  }

 public:
  auto getLValNumDims() const -> unsigned override { return numDims; }  ///< 获取作为左值的维度数量
  auto getLValDims() const -> std::vector<Value *> override {
    std::vector<Value *> dims;
    for (const auto &dim : getOperands()) {
      dims.emplace_back(dim->getValue());
    }

    return dims;
  }  ///< 获取作为左值的维度列表

  auto getNumDims() const -> unsigned { return numDims; }                     ///< 获取维度数量
  auto getDim(unsigned index) const -> Value * { return getOperand(index); }  ///< 获取位置为index的维度
  auto getDims() const { return getOperands(); }                              ///< 获取维度列表
  auto getByIndex(unsigned index) const -> Value * {
    return initValues.getValue(index);
  }  ///< 通过一维偏移量index获取初始值
  auto getByIndices(const std::vector<Value *> &indices) const -> Value * {
    int index = 0;
    for (size_t i = 0; i < indices.size(); i++) {
      index = dynamic_cast<ConstantValue *>(getDim(i))->getInt() * index +
              dynamic_cast<ConstantValue *>(indices[i])->getInt();
    }
    return getByIndex(index);
  }  ///< 通过多维索引indices获取初始值
  auto getInitValues() const -> const ValueCounter & { return initValues; }  ///< 获取初始值列表
};

/**
 * @brief 常量
 */
class ConstantVariable : public User, public LVal {
  friend class Module;

 protected:
  Module *parent;           ///< 父模块
  unsigned numDims;         ///< 维度数量
  ValueCounter initValues;  ///< 值

 protected:
  ConstantVariable(Module *parent, Type *type, const std::string &name, const ValueCounter &init,
                   const std::vector<Value *> &dims = {})
      : User(type, name), parent(parent) {
    assert(type->isPointer());
    numDims = dims.size();
    initValues = init;
    addOperands(dims);
  }

 public:
  auto getLValNumDims() const -> unsigned override { return numDims; }  ///< 获取作为左值的维度数量
  auto getLValDims() const -> std::vector<Value *> override {
    std::vector<Value *> dims;
    for (const auto &dim : getOperands()) {
      dims.emplace_back(dim->getValue());
    }

    return dims;
  }  ///< 获取作为左值的维度列表
  auto getByIndex(unsigned index) const -> Value * { return initValues.getValue(index); }  ///< 通过一维位置index获取值
  auto getByIndices(const std::vector<Value *> &indices) const -> Value * {
    int index = 0;
    for (size_t i = 0; i < indices.size(); i++) {
      index = dynamic_cast<ConstantValue *>(getDim(i))->getInt() * index +
              dynamic_cast<ConstantValue *>(indices[i])->getInt();
    }

    return getByIndex(index);
  }                                                        ///< 通过多维索引indices获取初始值
  auto getNumDims() const -> unsigned { return numDims; }  ///< 获取维度数量
  auto getDim(unsigned index) const -> Value * { return getOperand(index); }  ///< 获取位置为index的维度
  auto getDims() const { return getOperands(); }                              ///< 获取维度列表
  auto getInitValues() const { return initValues; }                           ///< 获取初始值
};

/**
 * @brief 符号表节点
 *
 */
using SymbolTableNode = struct SymbolTableNode {
  struct SymbolTableNode *pNode;            ///< 父节点
  std::vector<SymbolTableNode *> children;  ///< 子节点列表
  std::map<std::string, User *> varList;    ///< 变量列表
};

/**
 * @brief 符号表
 *
 * 负责管理全局变量、局部变量和常量。
 */
class SymbolTable {
 private:
  SymbolTableNode *curNode{};                              ///< 当前所在的作用域(符号表节点)
  std::map<std::string, unsigned> variableIndex;           ///< 变量命名索引表
  std::vector<std::unique_ptr<GlobalValue>> globals;       ///< 全局变量列表
  std::vector<std::unique_ptr<ConstantVariable>> consts;   ///< 常量列表
  std::vector<std::unique_ptr<SymbolTableNode>> nodeList;  ///< 符号表节点列表

 public:
  SymbolTable() = default;

  auto getVariable(const std::string &name) const -> User *;  ///< 根据名字name以及当前作用域获取变量
  auto addVariable(const std::string &name, User *variable) -> User *;               ///< 添加变量
  auto getGlobals() -> std::vector<std::unique_ptr<GlobalValue>> &;                  ///< 获取全局变量列表
  auto getConsts() const -> const std::vector<std::unique_ptr<ConstantVariable>> &;  ///< 获取常量列表
  void enterNewScope();                                                              ///< 进入新的作用域
  void leaveScope();                                                                 ///< 离开作用域
  auto isInGlobalScope() const -> bool;                                              ///< 是否位于全局作用域
  void enterGlobalScope();                                                           ///< 进入全局作用域
  auto isCurNodeNull() -> bool { return curNode == nullptr; }
};

/**
 * @brief 模块
 *
 * 代表compUnit。
 */
class Module {
 protected:
  std::map<std::string, std::unique_ptr<Function>> externalFunctions;  ///< 外部函数表
  std::map<std::string, std::unique_ptr<Function>> functions;          ///< 函数表
  SymbolTable variableTable;                                           ///< 符号表

 public:
  Module() = default;

 public:
  auto createFunction(const std::string &name, Type *type) -> Function * {
    auto result = functions.try_emplace(name, new Function(this, type, name));
    if (!result.second) {
      return nullptr;
    }
    return result.first->second.get();
  }  ///< 创建函数
  auto createExternalFunction(const std::string &name, Type *type) -> Function * {
    auto result = externalFunctions.try_emplace(name, new Function(this, type, name));
    if (!result.second) {
      return nullptr;
    }
    return result.first->second.get();
  }  ///< 创建外部函数
  auto createGlobalValue(const std::string &name, Type *type, const std::vector<Value *> &dims = {},
                         const ValueCounter &init = {}) -> GlobalValue * {
    bool isFinished = variableTable.isCurNodeNull();
    if (isFinished) {
      variableTable.enterGlobalScope();
    }
    auto result = variableTable.addVariable(name, new GlobalValue(this, type, name, dims, init));
    if (isFinished) {
      variableTable.leaveScope();
    }
    if (result == nullptr) {
      return nullptr;
    }
    return dynamic_cast<GlobalValue *>(result);
  }  ///< 创建全局变量
  auto createConstVar(const std::string &name, Type *type, const ValueCounter &init,
                      const std::vector<Value *> &dims = {}) -> ConstantVariable * {
    auto result = variableTable.addVariable(name, new ConstantVariable(this, type, name, init, dims));
    if (result == nullptr) {
      return nullptr;
    }
    return dynamic_cast<ConstantVariable *>(result);
  }  ///< 创建常量
  void addVariable(const std::string &name, AllocaInst *variable) {
    variableTable.addVariable(name, variable);
  }  ///< 添加变量
  auto getVariable(const std::string &name) -> User * {
    return variableTable.getVariable(name);
  }  ///< 根据名字name和当前作用域获取变量
  auto getFunction(const std::string &name) const -> Function * {
    auto result = functions.find(name);
    if (result == functions.end()) {
      return nullptr;
    }
    return result->second.get();
  }  ///< 获取函数
  auto getExternalFunction(const std::string &name) const -> Function * {
    auto result = externalFunctions.find(name);
    if (result == functions.end()) {
      return nullptr;
    }
    return result->second.get();
  }  ///< 获取外部函数
  auto getFunctions() -> std::map<std::string, std::unique_ptr<Function>> & { return functions; }  ///< 获取函数列表
  auto getExternalFunctions() const -> const std::map<std::string, std::unique_ptr<Function>> & {
    return externalFunctions;
  }  ///< 获取外部函数列表
  auto getGlobals() -> std::vector<std::unique_ptr<GlobalValue>> & {
    return variableTable.getGlobals();
  }  ///< 获取全局变量列表
  auto getConsts() const -> const std::vector<std::unique_ptr<ConstantVariable>> & {
    return variableTable.getConsts();
  }                                                        ///< 获取常量列表
  void enterNewScope() { variableTable.enterNewScope(); }  ///< 进入新的作用域

  void leaveScope() { variableTable.leaveScope(); }  ///< 离开作用域

  auto isInGlobalArea() const -> bool { return variableTable.isInGlobalScope(); }  ///< 是否位于全局作用域
};

/*!
 * @}
 */
}  // namespace sysy
