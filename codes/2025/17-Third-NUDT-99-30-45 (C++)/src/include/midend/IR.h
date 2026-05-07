#pragma once

#include "range.h"
#include <cassert>
#include <cstdint>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <type_traits>
#include <vector>
#include <variant>
#include <unordered_map>
#include <cmath>
#include <set>
#include <unordered_set>
#include <queue>
#include <algorithm>

namespace sysy {

// Global cleanup function to release all statically allocated IR objects
void cleanupIRPools();

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
    kArray,
  };

  Kind kind;  ///< 表示具体类型的变量

 protected:
  explicit Type(Kind kind) : kind(kind) {}
  virtual ~Type() = default;

 public:
  static Type* getIntType();                    ///< 返回表示Int类型的Type指针
  static Type* getFloatType();                  ///< 返回表示Float类型的Type指针
  static Type* getVoidType();                   ///< 返回表示Void类型的Type指针
  static Type* getLabelType();                  ///< 返回表示Label类型的Type指针
  static Type* getPointerType(Type *baseType);  ///< 返回表示指向baseType类型的Pointer类型的Type指针
  static Type* getFunctionType(Type *returnType, const std::vector<Type *> &paramTypes = {});
  ///< 返回表示返回类型为returnType,形参类型列表为paramTypes的函数类型的Type指针
  static Type* getArrayType(Type *elementType, unsigned numElements);

 public:
  Kind getKind() const { return kind; }                  ///< 返回Type对象代表原始标量类型
  bool isInt() const { return kind == kInt; }            ///< 判定是否为Int类型
  bool isFloat() const { return kind == kFloat; }        ///< 判定是否为Float类型
  bool isVoid() const { return kind == kVoid; }          ///< 判定是否为Void类型
  bool isLabel() const { return kind == kLabel; }        ///< 判定是否为Label类型
  bool isPointer() const { return kind == kPointer; }    ///< 判定是否为Pointer类型
  bool isFunction() const { return kind == kFunction; }  ///< 判定是否为Function类型
  bool isArray() const { return kind == Kind::kArray; }
  unsigned getSize() const;                              ///< 返回类型所占的空间大小(字节)
  /// 尝试将一个变量转换为给定的Type及其派生类类型的变量
  template <typename T>
  auto as() const -> std::enable_if_t<std::is_base_of_v<Type, T>, T *> {
    return dynamic_cast<T *>(const_cast<Type *>(this));
  }
  virtual void print(std::ostream& os) const;
};

class PointerType : public Type {
 protected:
  Type *baseType;  ///< 所指向的类型

 protected:
  explicit PointerType(Type *baseType) : Type(kPointer), baseType(baseType) {}

 public:
  static PointerType* get(Type *baseType);  ///< 获取指向baseType的Pointer类型
  
  // Cleanup method to release all cached pointer types (call at program exit)
  static void cleanup();

 public:
  Type* getBaseType() const { return baseType; }  ///< 获取指向的类型
};

class FunctionType : public Type {
 private:
  Type *returnType;                ///< 返回值类型
  std::vector<Type *> paramTypes;  ///< 形参类型列表

 protected:
  explicit FunctionType(Type *returnType, std::vector<Type *> paramTypes = {})
      : Type(kFunction), returnType(returnType), paramTypes(std::move(paramTypes)) {}

 public:
  /// 获取返回值类型为returnType， 形参类型列表为paramTypes的Function类型
  static FunctionType* get(Type *returnType, const std::vector<Type *> &paramTypes = {});
  
  // Cleanup method to release all cached function types (call at program exit)
  static void cleanup();

 public:
  Type* getReturnType() const { return returnType; }          ///< 获取返回值类信息
  auto getParamTypes() const { return make_range(paramTypes); }        ///< 获取形参类型列表
  unsigned getNumParams() const { return paramTypes.size(); }  ///< 获取形参数量
};

class ArrayType : public Type {
 public:
  // elements：数组的元素类型 (例如，int[3] 的 elementType 是 int)
  // numElements：该维度的大小 (例如，int[3] 的 numElements 是 3)
  static ArrayType *get(Type *elementType, unsigned numElements);
  
  // Cleanup method to release all cached array types (call at program exit)
  static void cleanup();

  Type *getElementType() const { return elementType; }
  unsigned getNumElements() const { return numElements; }

 protected:
  ArrayType(Type *elementType, unsigned numElements)
      : Type(Kind::kArray), elementType(elementType), numElements(numElements) {}
  Type *elementType;
  unsigned numElements; // 当前维度的大小
};

/*!
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

//! `Use` 表示`Value`和它的`User`之间的使用关系。

class Use {
 private:
  /**
   * value在User操作数中的位置，例如，
   * user->getOperands[index] == value
   */
  unsigned index;
  User *user;    ///< 使用者
  Value *value;  ///< 被使用的值

 public:
  Use() = default;
  Use(unsigned index, User *user, Value *value) : index(index), user(user), value(value) {}

 public:
  unsigned getIndex() const { return index; }   ///< 返回value在User操作数中的位置
  void setIndex(int newIndex) { index = newIndex; }  ///< 设置value在User操作数中的位置
  User* getUser() const { return user; }       ///< 返回使用者
  Value* getValue() const { return value; }    ///< 返回被使用的值
  void setValue(Value *newValue) { value = newValue; }  ///< 将被使用的值设置为newValue
  void print(std::ostream& os) const;
};

//! The base class of all value types

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
  const std::string& getName() const { return name; }           ///< 获取名字
  Type* getType() const { return type; }                        ///< 返回值的类型
  bool isInt() const { return type->isInt(); }                   ///< 判定是否为Int类型
  bool isFloat() const { return type->isFloat(); }               ///< 判定是否为Float类型
  bool isPointer() const { return type->isPointer(); }           ///< 判定是否为Pointer类型
  std::list<std::shared_ptr<Use>>& getUses() { return uses; }   ///< 获取使用关系列表
  void addUse(const std::shared_ptr<Use> &use) { uses.push_back(use); }  ///< 添加使用关系
  void replaceAllUsesWith(Value *value);  ///< 将原来使用该value的使用者全变为使用给定参数value并修改相应use关系
  void removeUse(const std::shared_ptr<Use> &use) { 
    assert(use != nullptr && "Use cannot be null");
    assert(use->getValue() == this && "Use being removed does NOT point to this Value!");
    auto it = std::find(uses.begin(), uses.end(), use);
    assert(it != uses.end() && "Use not found in Value's uses");
    uses.remove(use); 
  }  ///< 删除使用关系use
  void removeAllUses();
  virtual void print(std::ostream& os) const = 0;  ///< 输出值信息到输出流
};

/**
 * ValueCounter 需要理解为一个Value *的计数器。
 * 它的主要目的是为了节省存储空间和方便Memset指令的创建。
 * ValueCounter记录了一列Value *的互异元素和每个元素的重复数量。
 * 例如，假设有一列Value *为{v1, v1, v2, v3, v3, v3, v4}，
 * 那么ValueCounter将记录为：
 * - __counterValues: {v1, v2, v3, v4}
 * - __counterNumbers: {2, 1, 3, 1}
 * - __size: 7  
 * 使得存储空间得到节省，方便Memset指令的创建。
 */
class ValueCounter {
 private:
  unsigned __size{};                       ///< 总的Value数量
  std::vector<Value *> __counterValues;    ///< 记录的Value *列表(无重复元素)
  std::vector<unsigned> __counterNumbers;  ///< 记录的Value *重复数量列表

 public:
  ValueCounter() = default;

 public:
  unsigned size() const { return __size; }  ///< 返回总的Value数量
  Value* getValue(unsigned index) const {
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
  const std::vector<Value *>& getValues() const { return __counterValues; }  ///< 获取互异Value *列表
  const std::vector<unsigned>& getNumbers() const { return __counterNumbers; }  ///< 获取Value *重复数量列表
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


// --- Refactored ConstantValue and related classes start here ---

using ConstantValVariant = std::variant<int, float>;

// Helper for hashing std::variant
struct VariantHash {
  template <class T>
  std::size_t operator()(const T& val) const {
    return std::hash<T>{}(val);
  }
  std::size_t operator()(const ConstantValVariant& v) const {
    return std::visit(*this, v);
  }
};

struct ConstantValueKey {
  Type* type;
  ConstantValVariant val;

  bool operator==(const ConstantValueKey& other) const {
    // Assuming Type objects are canonicalized, or add Type::isSame()
    // If Type::isSame() is not available and Type objects are not canonicalized,
    // this comparison might not be robust enough for structural equivalence of types.
    return type == other.type && val == other.val;
  }
};

struct ConstantValueHash {
  std::size_t operator()(const ConstantValueKey& key) const {
    std::size_t typeHash = std::hash<Type*>{}(key.type);
    std::size_t valHash = VariantHash{}(key.val);
    // A simple way to combine hashes
    return typeHash ^ (valHash << 1);
  }
};

struct ConstantValueEqual {
  bool operator()(const ConstantValueKey& lhs, const ConstantValueKey& rhs) const {
    // Assuming Type objects are canonicalized (e.g., Type::getIntType() always returns same pointer)
    // If not, and Type::isSame() is intended, it should be added to Type class.
    return lhs.type == rhs.type && lhs.val == rhs.val;
  }
};

/*!
 * Static constants known at compile time.
 *
 * `ConstantValue`s are not defined by instructions, and do not use any other
 * `Value`s. It's type is either `int` or `float`.
 * `ConstantValue`并不由指令定义, 也不使用任何Value。它的类型为int/float。
 */

template<class T> struct always_false : std::false_type {};
template<class T> constexpr bool always_false_v = always_false<T>::value;

class ConstantValue : public Value {
protected:
  static std::unordered_map<ConstantValueKey, ConstantValue*, ConstantValueHash, ConstantValueEqual> mConstantPool;

public:
  explicit ConstantValue(Type* type, const std::string& name = "") : Value(type, name) {}
  virtual ~ConstantValue() = default;

  virtual size_t hash() const = 0;
  virtual ConstantValVariant getVal() const = 0;

  // Static factory method to get a canonical ConstantValue from the pool
  static ConstantValue* get(Type* type, ConstantValVariant val);
  
  // Cleanup method to release all cached constants (call at program exit)
  static void cleanup();

  // Helper methods to access constant values with appropriate casting
  int getInt() const {
    auto val = getVal();
    if (std::holds_alternative<int>(val)) {
      return std::get<int>(val);
    } else if (std::holds_alternative<float>(val)) {
      return static_cast<int>(std::get<float>(val));
    }
    // Handle other possible types if needed
    return 0; // Default fallback
  }

  float getFloat() const {
    auto val = getVal();
    if (std::holds_alternative<float>(val)) {
      return std::get<float>(val);
    } else if (std::holds_alternative<int>(val)) {
      return static_cast<float>(std::get<int>(val));
    }
    // Handle other possible types if needed
    return 0.0f; // Default fallback
  }

  template<typename T>
  T getVal() const {
    if constexpr (std::is_same_v<T, int>) {
      return getInt();
    } else if constexpr (std::is_same_v<T, float>) {
      return getFloat();
    } else {
      // This ensures a compilation error if an unsupported type is used
      static_assert(always_false_v<T>, "Unsupported type for ConstantValue::getValue()");
    }
  }

  virtual bool isZero() const = 0;
  virtual bool isOne() const = 0;
  void print(std::ostream& os) const = 0;
};

class ConstantInteger : public ConstantValue {
  int constVal;
public:
  explicit ConstantInteger(Type* type, int val, const std::string& name = "")
      : ConstantValue(type, name), constVal(val) {}

  size_t hash() const override {
      std::size_t typeHash = std::hash<Type*>{}(getType());
      std::size_t valHash = std::hash<int>{}(constVal);
      return typeHash ^ (valHash << 1);
  }
  int getInt() const { return constVal; }
  ConstantValVariant getVal() const override { return constVal; }

  static ConstantInteger* get(Type* type, int val);
  static ConstantInteger* get(int val) { return get(Type::getIntType(), val); }

  ConstantInteger* getNeg() const {
    assert(getType()->isInt() && "Cannot negate non-integer constant");
    return ConstantInteger::get(-constVal);
  }

  bool isZero() const override { return constVal == 0; }
  bool isOne() const override { return constVal == 1; }
  void print(std::ostream& os) const;
};

class ConstantFloating : public ConstantValue {
  float constFVal;
public:
  explicit ConstantFloating(Type* type, float val, const std::string& name = "")
      : ConstantValue(type, name), constFVal(val) {}

  size_t hash() const override {
    std::size_t typeHash = std::hash<Type*>{}(getType());
    std::size_t valHash = std::hash<float>{}(constFVal);
    return typeHash ^ (valHash << 1);
  }
  float getFloat() const { return constFVal; }
  ConstantValVariant getVal() const override { return constFVal; }

  static ConstantFloating* get(Type* type, float val);
  static ConstantFloating* get(float val) { return get(Type::getFloatType(), val); }

  ConstantFloating* getNeg() const {
    assert(getType()->isFloat() && "Cannot negate non-float constant");
    return ConstantFloating::get(-constFVal);
  }

  bool isZero() const override { return constFVal == 0.0f; }
  bool isOne() const override { return constFVal == 1.0f; }
  void print(std::ostream& os) const;
};

class UndefinedValue : public ConstantValue {
private:
  static std::unordered_map<Type*, UndefinedValue*> UndefValues;

protected:
  explicit UndefinedValue(Type* type, const std::string& name = "")
      : ConstantValue(type, name) {
    assert(!type->isVoid() && "Cannot create UndefinedValue of void type!");
  }

public:
  static UndefinedValue* get(Type* type);
  
  // Cleanup method to release all cached undefined values (call at program exit)
  static void cleanup();

  size_t hash() const override {
    return std::hash<Type*>{}(getType());
  }

  ConstantValVariant getVal() const override {
    if (getType()->isInt()) {
      return 0; // Return 0 for undefined integer
    } else if (getType()->isFloat()) {
      return 0.0f; // Return 0.0f for undefined float
    }
    assert(false && "UndefinedValue has unexpected type for getValue()");
    return 0; // Should not be reached
  }

  bool isZero() const override { return false; }
  bool isOne() const override { return false; }
  void print(std::ostream& os) const;
};

// --- End of refactored ConstantValue and related classes ---

class Instruction;
class Function;
class BasicBlock;

/*!
 * The container for `Instruction` sequence.
 *
 * `BasicBlock` maintains a list of `Instruction`s, with the last one being
 * a terminator (branch or return). Besides, `BasicBlock` stores its arguments
 * and records its predecessor and successor `BasicBlock`s.
 */
class BasicBlock : public Value {
  friend class Function;

public:

  using inst_list = std::list<std::unique_ptr<Instruction>>;
  using iterator = inst_list::iterator;
  using block_list = std::vector<BasicBlock *>;
  using block_set = std::unordered_set<BasicBlock *>;

protected:

  Function *parent;              ///< 从属的函数
  inst_list instructions;        ///< 拥有的指令序列
  block_list successors;         ///< 前驱列表
  block_list predecessors;       ///< 后继列表
  bool reachable = false;

public:
  explicit BasicBlock(Function *parent, const std::string &name = "")
      : Value(Type::getLabelType(), name), parent(parent) {}
  ~BasicBlock() override {
    // for (auto pre : predecessors) {
    //   pre->removeSuccessor(this);
    // }
    // for (auto suc : successors) {
    //   suc->removePredecessor(this);
    // }
    // 这些关系应该在 BasicBlock 被从 Function 中移除时，
    // 由负责 CFG 优化的 Pass (例如 SCCP 的 RemoveDeadBlock) 显式地清理。
    // 析构函数只负责清理 BasicBlock 自身拥有的资源（例如，指令列表）。
  }
  
public:

  unsigned getNumInstructions() const { return instructions.size(); }
  unsigned getNumPredecessors() const { return predecessors.size(); }
  unsigned getNumSuccessors() const { return successors.size(); }
  Function* getParent() const { return parent; }
  void setParent(Function *func) { parent = func; }
  inst_list& getInstructions() { return instructions; }
  auto getInstructions_Range() const { return make_range(instructions); }
  block_list& getPredecessors() { return predecessors; }
  void clearPredecessors() { predecessors.clear(); }
  block_list& getSuccessors() { return successors; }
  void clearSuccessors() { successors.clear(); }
  iterator begin() { return instructions.begin(); }
  iterator end() { return instructions.end(); }
  iterator terminator() { return std::prev(end()); }
  iterator findInstIterator(Instruction *inst) {
    return std::find_if(instructions.begin(), instructions.end(),
                        [inst](const std::unique_ptr<Instruction> &i) { return i.get() == inst; });
  }  ///< 查找指定指令的迭代器
  bool hasSuccessor(BasicBlock *block) const {
    return std::find(successors.begin(), successors.end(), block) != successors.end();
  }  ///< 判断是否有后继块
  bool hasPredecessor(BasicBlock *block) const {
    return std::find(predecessors.begin(), predecessors.end(), block) != predecessors.end();
  }  ///< 判断是否有前驱块
  void addPredecessor(BasicBlock *block) {
    if (std::find(predecessors.begin(), predecessors.end(), block) == predecessors.end()) {
      predecessors.push_back(block);
    }
  }
  void addSuccessor(BasicBlock *block) {
    if (std::find(successors.begin(), successors.end(), block) == successors.end()) {
      successors.push_back(block);
    }
  }
  void addPredecessor(const block_list &blocks) {
    for (auto block : blocks) {
      addPredecessor(block);
    }
  }
  void addSuccessor(const block_list &blocks) {
    for (auto block : blocks) {
      addSuccessor(block);
    }
  }
  void removePredecessor(BasicBlock *block) {
    auto iter = std::find(predecessors.begin(), predecessors.end(), block);
    if (iter != predecessors.end()) {
      predecessors.erase(iter);
    } else {
      // 如果没有找到前驱块，可能是因为它已经被移除或不存在
      // 这可能是一个错误情况，或者是因为在CFG优化过程中已经处理
      // assert(false && "Predecessor block not found in BasicBlock");
    }
  }
  void removeSuccessor(BasicBlock *block) {
    auto iter = std::find(successors.begin(), successors.end(), block);
    if (iter != successors.end()) {
      successors.erase(iter);
    } else {
      // 如果没有找到后继块，可能是因为它已经被移除或不存在
      // 这可能是一个错误情况，或者是因为在CFG优化过程中已经处理
      // assert(false && "Successor block not found in BasicBlock");
    }
  }
  void replacePredecessor(BasicBlock *oldBlock, BasicBlock *newBlock) {
    for (auto &predecessor : predecessors) {
      if (predecessor == oldBlock) {
        predecessor = newBlock;
        break;
      }
    }
  }
  void setreachableTrue() { reachable = true; }  ///< 设置可达
  void setreachableFalse() { reachable = false; }  ///< 设置不可达
  bool getreachable() { return reachable; }  ///< 返回可达状态
  static void conectBlocks(BasicBlock *prev, BasicBlock *next) {
    prev->addSuccessor(next);
    next->addPredecessor(prev);
  }
  iterator removeInst(iterator pos) { return instructions.erase(pos); }
  void removeInst(Instruction *inst) {
    auto pos = std::find_if(instructions.begin(), instructions.end(),
                            [inst](const std::unique_ptr<Instruction> &i) { return i.get() == inst; });
    if (pos != instructions.end()) {
      instructions.erase(pos);
    } else {
      assert(false && "Instruction not found in BasicBlock");
    }
  }  ///< 移除指定位置的指令
  iterator moveInst(iterator sourcePos, iterator targetPos, BasicBlock *block);
  
  /// 清理基本块中的所有使用关系
  void cleanup();
  
  void print(std::ostream& os) const; 
};

//! User is the abstract base type of `Value` types which use other `Value` as
//! operands. Currently, there are two kinds of `User`s, `Instruction` and
//! `GlobalValue`.
//  User是`Value`的派生类型，其使用其他的`Value`作为操作数


class User : public Value {
 protected:
  std::vector<std::shared_ptr<Use>> operands;  ///< 操作数/使用关系

 protected:
  explicit User(Type *type, const std::string &name = "") : Value(type, name) {}

 public:
  unsigned getNumOperands() const { return operands.size(); }  ///< 获取操作数数量
  auto operand_begin() const { return operands.begin(); }              ///< 返回操作数列表的开头迭代器
  auto operand_end() const { return operands.end(); }                  ///< 返回操作数列表的结尾迭代器
  auto getOperands() const { return make_range(operand_begin(), operand_end()); }           ///< 获取操作数列表
  Value* getOperand(unsigned index) const { return operands[index]->getValue(); }  ///< 获取位置为index的操作数
  void addOperand(Value *value) {
    operands.emplace_back(std::make_shared<Use>(operands.size(), this, value));
    value->addUse(operands.back());
  }  ///< 增加操作数
  void removeOperand(unsigned index);
  template <typename ContainerT>
  void addOperands(const ContainerT &newoperands) {
    for (auto value : newoperands) {
      addOperand(value);
    }
  }                                                   ///< 增加多个操作数
  void replaceOperand(unsigned index, Value *value);  ///< 替换操作数
  void setOperand(unsigned index, Value *value);      ///< 设置操作数
  
  /// 清理用户的所有操作数使用关系
  void cleanup();
};

/*!
 * Base of all concrete instruction types.
 */
class Instruction : public User {
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
    // kXor = 0x1UL << 46,
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
    kUnreachable = 0x1UL << 33,
    // mem op
    kAlloca = 0x1UL << 34,
    kLoad = 0x1UL << 35,
    kStore = 0x1UL << 36,
    kGetElementPtr = 0x1UL << 37,
    kMemset = 0x1UL << 38,
    // phi
    kPhi = 0x1UL << 39,
    kBitItoF = 0x1UL << 40,
    kBitFtoI = 0x1UL << 41,
    kSrl = 0x1UL << 42, // 逻辑右移
    kSll = 0x1UL << 43, // 逻辑左移
    kSra = 0x1UL << 44, // 算术右移
    kMulh = 0x1UL << 45
  };

protected:
  Kind kind; 
  BasicBlock *parent;

protected:
  Instruction(Kind kind, Type *type, BasicBlock *parent = nullptr, const std::string &name = "")
      : User(type, name), kind(kind), parent(parent) {}

public:

public:
  Kind getKind() const { return kind; }
  std::string getKindString() const{
    switch (kind) {
      case kInvalid:
        return "invalid";
      case kAdd:
        return "add";
      case kSub:
        return "sub";
      case kMul:
        return "mul";
      case kDiv:
        return "sdiv";
      case kRem:
        return "srem";
      case kICmpEQ:
        return "icmp eq";
      case kICmpNE:
        return "icmp ne";
      case kICmpLT:
        return "icmp slt";
      case kICmpGT:
        return "icmp sgt";
      case kICmpLE:
        return "icmp sle";
      case kICmpGE:
        return "icmp sge";
      case kFAdd:
        return "fadd";
      case kFSub:
        return "fsub";
      case kFMul:
        return "fmul";
      case kFDiv:
        return "fdiv";
      case kFCmpEQ:
        return "fcmp oeq";
      case kFCmpNE:
        return "fcmp one";
      case kFCmpLT:
        return "fcmp olt";
      case kFCmpGT:
        return "fcmp ogt";
      case kFCmpLE:
        return "fcmp ole";
      case kFCmpGE:
        return "fcmp oge";
      case kAnd:
        return "and";
      case kOr:
        return "or";
      case kNeg:
        return "neg";
      case kNot:
        return "not";
      case kFNeg:
        return "FNeg";
      case kFNot:
        return "FNot";
      case kFtoI:
        return "FtoI";
      case kItoF:
        return "iToF";
      case kCall:
        return "call";
      case kCondBr:
        return "condBr";
      case kBr:
        return "br";
      case kReturn:
        return "return";
      case kUnreachable:
        return "unreachable";
      case kAlloca:
        return "alloca";
      case kLoad:
        return "load";
      case kStore:
        return "store";
      case kGetElementPtr:
        return "getElementPtr";
      case kMemset:
        return "memset";
      case kPhi:
        return "phi";
      case kBitItoF:
        return "BitItoF";
      case kBitFtoI:
        return "BitFtoI";
      case kSrl:
        return "lshr";
      case kSll:
        return "shl";
      case kSra:
        return "ashr";
      case kMulh:
        return "mulh";
      default:
        return "Unknown";
    }
  }                                                          ///< 根据指令标识码获取字符串
  
  BasicBlock* getParent() const { return parent; }
  Function* getFunction() const { return parent->getParent(); }
  void setParent(BasicBlock *bb) { parent = bb; }

  bool isBinary() const {
    static constexpr uint64_t BinaryOpMask =
        (kAdd | kSub | kMul | kDiv | kRem | kAnd | kOr | kSra | kSrl | kSll | kMulh) |
        (kICmpEQ | kICmpNE | kICmpLT | kICmpGT | kICmpLE | kICmpGE);
    return kind & BinaryOpMask;
  }
  bool isFPBinary() const {
    static constexpr uint64_t FPBinaryOpMask =
        (kFAdd | kFSub | kFMul | kFDiv) |
        (kFCmpEQ | kFCmpNE | kFCmpLT | kFCmpGT | kFCmpLE | kFCmpGE);
    return kind & FPBinaryOpMask;
  }
  bool isUnary() const {
    static constexpr uint64_t UnaryOpMask = 
        kNeg | kNot | kFNeg | kFNot | kFtoI | kItoF | kBitFtoI | kBitItoF;
    return kind & UnaryOpMask;
  }
  bool isMemory() const {
    static constexpr uint64_t MemoryOpMask = 
        kAlloca | kLoad | kStore;
    return kind & MemoryOpMask;
  }
  bool isTerminator() const {
    static constexpr uint64_t TerminatorOpMask = kCondBr | kBr | kReturn | kUnreachable;
    return kind & TerminatorOpMask;
  }
  bool isCmp() const {
    static constexpr uint64_t CmpOpMask =
        (kICmpEQ | kICmpNE | kICmpLT | kICmpGT | kICmpLE | kICmpGE) |
        (kFCmpEQ | kFCmpNE | kFCmpLT | kFCmpGT | kFCmpLE | kFCmpGE);
    return kind & CmpOpMask;
  }
  bool isBranch() const {
    static constexpr uint64_t BranchOpMask = kBr | kCondBr;
    return kind & BranchOpMask;
  }
  bool isCommutative() const {
    static constexpr uint64_t CommutativeOpMask =
        kAdd | kMul | kICmpEQ | kICmpNE | kFAdd | kFMul | kFCmpEQ | kFCmpNE | kAnd | kOr;
    return kind & CommutativeOpMask;
  }
  bool isUnconditional() const { return kind == kBr; }
  bool isConditional() const { return kind == kCondBr; }
  bool isCondBr() const { return kind == kCondBr; }
  bool isPhi() const { return kind == kPhi; }
  bool isAlloca() const { return kind == kAlloca; }
  bool isLoad() const { return kind == kLoad; }
  bool isStore() const { return kind == kStore; }
  bool isGetElementPtr() const { return kind == kGetElementPtr; }
  bool isMemset() const { return kind == kMemset; }
  bool isCall() const { return kind == kCall; }
  bool isReturn() const { return kind == kReturn; }
  bool isUnreachable() const { return kind == kUnreachable; }
  bool isDefine() const {
    static constexpr uint64_t DefineOpMask = kAlloca | kStore | kPhi;
    return (kind & DefineOpMask) != 0U;
  } 
  
  virtual ~Instruction() = default;
  
  virtual void print(std::ostream& os) const = 0;
}; // class Instruction

class Function;
//! Function call.

class PhiInst : public Instruction {
  friend class IRBuilder;
  friend class Function;

 protected:

  std::unordered_map<BasicBlock *, Value *> blk2val;  ///< 存储每个基本块对应的值
  unsigned vsize;  ///< 存储值的数量

  PhiInst(Type *type, 
          const std::vector<Value *> &rhs = {}, 
          const std::vector<BasicBlock*> &Blocks = {},
          BasicBlock *parent = nullptr,
          const std::string &name = "")
      : Instruction(Kind::kPhi, type, parent, name), vsize(rhs.size()) {
    assert(rhs.size() == Blocks.size() && "PhiInst: rhs and Blocks must have the same size");
    for(size_t i = 0; i < vsize; ++i) {
      addOperand(rhs[i]);
      addOperand(Blocks[i]);
      blk2val[Blocks[i]] = rhs[i];
    }
  }

 public:
  unsigned getNumIncomingValues() const { return vsize; }  ///< 获取传入值的数量
  Value *getIncomingValue(unsigned Idx) const { return getOperand(Idx * 2); }  ///< 获取指定位置的传入值
  BasicBlock *getIncomingBlock(unsigned Idx) const {return dynamic_cast<BasicBlock *>(getOperand(Idx * 2 + 1)); }  ///< 获取指定位置的传入基本块

  Value* getValfromBlk(BasicBlock* block);
  BasicBlock* getBlkfromVal(Value* value);
  auto getIncomingValues() const {
    std::vector<std::pair<BasicBlock*, Value*>> result;
    for (const auto& [block, value] : blk2val) {
      result.emplace_back(block, value);
    }
    return result;
  }
  void addIncoming(Value *value, BasicBlock *block) {
    assert(value && block && "PhiInst: value and block cannot be null");
    addOperand(value);
    addOperand(block);
    blk2val[block] = value;
    vsize++;
  }  ///< 添加传入值和对应的基本块
  void removeIncoming(unsigned Idx) {
    assert(Idx < vsize && "PhiInst: Index out of bounds");
    auto blk = getIncomingBlock(Idx);
    removeOperand(Idx * 2 + 1);  // Remove block
    removeOperand(Idx * 2);  // Remove value
    blk2val.erase(blk);
    vsize--;
  }  ///< 移除指定位置的传入值和对应的基本块
  // 移除指定的传入值或基本块
  void removeIncomingValue(Value *value);
  void removeIncomingBlock(BasicBlock *block);
  // 设置指定位置的传入值或基本块
  void setIncomingValue(unsigned Idx, Value *value);
  void setIncomingBlock(unsigned Idx, BasicBlock *block);
  // 替换指定位置的传入值或基本块（原理是删除再添加）保留旧块或者旧值
  void replaceIncomingValue(Value *oldValue, Value *newValue);
  void replaceIncomingBlock(BasicBlock *oldBlock, BasicBlock *newBlock);
  // 替换指定位置的传入值或基本块（原理是删除再添加）
  void replaceIncomingValue(Value *oldValue, Value *newValue, BasicBlock *newBlock);
  void replaceIncomingBlock(BasicBlock *oldBlock, BasicBlock *newBlock, Value *newValue);
  void refreshMap() {
    blk2val.clear();
    vsize = getNumOperands() / 2;
    for (unsigned i = 0; i < vsize; ++i) {
      blk2val[getIncomingBlock(i)] = getIncomingValue(i);
    }
  }  ///< 刷新块到值的映射关系
  auto getValues() { return make_range(std::next(operand_begin()), operand_end()); }
  void print(std::ostream& os) const override;
};


class CallInst : public Instruction {
  friend class Function;
  friend class IRBuilder;

protected:
  CallInst(Function *callee, const std::vector<Value *> &args, BasicBlock *parent = nullptr, const std::string &name = "");

public:
  Function *getCallee() const;
  auto getArguments() const {
    return make_range(std::next(operand_begin()), operand_end());
  }
  void print(std::ostream& os) const override;
}; // class CallInst

//! Unary instruction, includes '!', '-' and type conversion.
class UnaryInst : public Instruction {
  friend class Function;
  friend class IRBuilder;

protected:
  UnaryInst(Kind kind, Type *type, Value *operand, BasicBlock *parent = nullptr,
            const std::string &name = "")
      : Instruction(kind, type, parent, name) {
    addOperand(operand);
  }


public:
  Value* getOperand() const { return User::getOperand(0); }
  void print(std::ostream& os) const override;
}; // class UnaryInst

//! Binary instruction, e.g., arithmatic, relation, logic, etc.
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
  Value* getLhs() const { return getOperand(0); }
  Value* getRhs() const { return getOperand(1); }
  template <typename T>
  T eval(T lhs, T rhs) {
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
  static BinaryInst* create(Kind kind, Type *type, Value *lhs, Value *rhs, BasicBlock *parent, const std::string &name = "") {
        // 后端处理数组访存操作时需要创建计算地址的指令，需要在外部构造 BinaryInst 对象
        return new BinaryInst(kind, type, lhs, rhs, parent, name);
  }
  void print(std::ostream& os) const override;
}; // class BinaryInst

//! The return statement
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
  bool hasReturnValue() const { return not operands.empty(); }
  Value* getReturnValue() const {
    return hasReturnValue() ? getOperand(0) : nullptr;
  }
  void print(std::ostream& os) const override;
};

//! Unconditional branch
class UncondBrInst : public Instruction {
  friend class IRBuilder;
  friend class Function;

protected:
  UncondBrInst(BasicBlock *block,
               BasicBlock *parent = nullptr)
      : Instruction(kBr, Type::getVoidType(), parent, "") {
    addOperand(block);
  }

public:
  BasicBlock* getBlock() const { return dynamic_cast<BasicBlock *>(getOperand(0)); }
  auto getArguments() const {
    return make_range(std::next(operand_begin()), operand_end());
  }
  std::vector<BasicBlock *> getSuccessors() const {
    std::vector<BasicBlock *> succs;
    // 假设无条件分支的目标块是它的第一个操作数
    if (getNumOperands() > 0) {
      if (auto target_bb = dynamic_cast<BasicBlock *>(getOperand(0))) {
        succs.push_back(target_bb);
      }
    }
    return succs;
  }
  void print(std::ostream& os) const override;
}; // class UncondBrInst

//! Conditional branch
// 这里的args是指向条件分支的两个分支的参数列表但是现在弃用了
// 通过mem2reg优化后，数据流分析将不会由arguments来传递
class CondBrInst : public Instruction {
  friend class IRBuilder;
  friend class Function;
  
protected:
  CondBrInst(Value *condition, BasicBlock *thenBlock, BasicBlock *elseBlock, 
              BasicBlock *parent = nullptr)
      : Instruction(kCondBr, Type::getVoidType(), parent, "") {
    addOperand(condition);
    addOperand(thenBlock);
    addOperand(elseBlock);
  }
public:
  Value* getCondition() const { return getOperand(0); }
  BasicBlock* getThenBlock() const {
    return dynamic_cast<BasicBlock *>(getOperand(1));
  }
  BasicBlock* getElseBlock() const {
    return dynamic_cast<BasicBlock *>(getOperand(2));
  }
  std::vector<BasicBlock *> getSuccessors() const {
    std::vector<BasicBlock *> succs;
    // 假设条件分支的真实块是第二个操作数，假块是第三个操作数
    // 操作数通常是：[0] 条件值, [1] TrueTargetBlock, [2] FalseTargetBlock
    if (getNumOperands() > 2) {
      if (auto true_bb = getThenBlock()) {
        succs.push_back(true_bb);
      }
      if (auto false_bb = getElseBlock()) {
        succs.push_back(false_bb);
      }
    }
    return succs;
  }
  void print(std::ostream& os) const override;
}; // class CondBrInst

class UnreachableInst : public Instruction {
public:
  // 构造函数：设置指令类型为 kUnreachable
  explicit UnreachableInst(const std::string& name, BasicBlock *parent = nullptr)
      : Instruction(kUnreachable, Type::getVoidType(), parent, "") {}
  void print(std::ostream& os) const { os << "unreachable"; }
};

//! Allocate memory for stack variables, used for non-global variable declartion
class AllocaInst : public Instruction {
  friend class IRBuilder;
  friend class Function;
protected:
  AllocaInst(Type *type,
             BasicBlock *parent = nullptr, const std::string &name = "")
      : Instruction(kAlloca, type, parent, name) {
  }

public:
  //! 获取分配的类型
  Type* getAllocatedType() const {
    return getType()->as<PointerType>()->getBaseType();
  }  ///< 获取分配的类型
  void print(std::ostream& os) const override;
}; // class AllocaInst


class GetElementPtrInst : public Instruction {
  friend class IRBuilder;

protected:
  // GEP的构造函数：
  // resultType: GEP计算出的地址的类型 (通常是指向目标元素类型的指针)
  // basePointer: 基指针 (第一个操作数)
  // indices: 索引列表 (后续操作数)
  GetElementPtrInst(Type *resultType,
                    Value *basePointer,
                    const std::vector<Value *> &indices = {},
                    BasicBlock *parent = nullptr, const std::string &name = "")
      : Instruction(Kind::kGetElementPtr, resultType, parent, name) {
    assert(basePointer && "GEP base pointer cannot be null!");
    // TODO : 安全检查
    assert(basePointer->getType()->isPointer() );
    addOperand(basePointer); // 第一个操作数是基指针
    addOperands(indices);    // 随后的操作数是索引
  }
public:
  Value* getBasePointer() const { return getOperand(0); }
  unsigned getNumIndices() const { return getNumOperands() - 1; }
  auto getIndices() const { return make_range(std::next(operand_begin()), operand_end());}
  Value* getIndex(unsigned index) const {
    assert(index < getNumIndices() && "Index out of bounds for GEP!");
    return getOperand(index + 1);
  }

  // 静态工厂方法，用于创建GEP指令 (如果需要外部直接创建而非通过IRBuilder)
  static GetElementPtrInst* create(Type *resultType, Value *basePointer,
                                    const std::vector<Value *> &indices = {},
                                    BasicBlock *parent = nullptr, const std::string &name = "") {
    return new GetElementPtrInst(resultType, basePointer, indices, parent, name);
  }
  void print(std::ostream& os) const override;
};

//! Load a value from memory address specified by a pointer value
class LoadInst : public Instruction {
  friend class IRBuilder;
  friend class Function;

protected:
  LoadInst(Value *pointer,
           BasicBlock *parent = nullptr, const std::string &name = "")
      : Instruction(kLoad, pointer->getType()->as<PointerType>()->getBaseType(),
                    parent, name) {
    addOperand(pointer);
  }

public:
  Value* getPointer() const { return getOperand(0); }
  void print(std::ostream& os) const override;
}; // class LoadInst

//! Store a value to memory address specified by a pointer value
class StoreInst : public Instruction {
  friend class IRBuilder;
  friend class Function;

protected:
  StoreInst(Value *value, Value *pointer,
            BasicBlock *parent = nullptr, const std::string &name = "")
      : Instruction(kStore, Type::getVoidType(), parent, name) {
    addOperand(value);
    addOperand(pointer);
  }

public:
  Value* getValue() const { return getOperand(0); }
  Value* getPointer() const { return getOperand(1); }
  void print(std::ostream& os) const override;
}; // class StoreInst

//! Memset instruction
class MemsetInst : public Instruction {
  friend class IRBuilder;
  friend class Function;

protected:
  //! Create a memset instruction.
  //! \param pointer The pointer to the memory location to be set.
  //! \param begin The starting address of the memory region to be set.
  //! \param size The size of the memory region to be set.
  //! \param value The value to set the memory region to.
  //! \param parent The parent basic block of this instruction.
  //! \param name The name of this instruction.
  MemsetInst(Value *pointer, Value *begin, Value *size, Value *value,
             BasicBlock *parent = nullptr, const std::string &name = "")
      : Instruction(kMemset, Type::getVoidType(), parent, name) {
    addOperand(pointer);
    addOperand(begin);
    addOperand(size);
    addOperand(value);
  }

public:
  Value* getPointer() const { return getOperand(0); }
  Value* getBegin() const { return getOperand(1); }
  Value* getSize() const { return getOperand(2); }
  Value* getValue() const { return getOperand(3); }
  void print(std::ostream& os) const override;
};

class GlobalValue;


class Argument : public Value {
protected:
  Function *func;
  int index;

public:
  Argument(Type *type, Function *func, int index, const std::string &name = "")
      : Value(type, name), func(func), index(index) {}

public:
  Function* getParent() const { return func; }
  int getIndex() const { return index; }
  
  /// 清理参数的使用关系
  void cleanup();
  
  void print(std::ostream& os) const;
};


class Module;
//! Function definitionclass 
class Function : public Value {
  friend class Module;
protected:
  Function(Module *parent, Type *type, const std::string &name) : Value(type, name), parent(parent) {
    blocks.emplace_back(new BasicBlock(this, "entry_" + name));  ///< 创建一个入口基本块
  }

public:
  using block_list = std::list<std::unique_ptr<BasicBlock>>;
  using arg_list = std::vector<Argument *>;
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
  arg_list arguments;                                     ///< 函数参数列表
  FunctionAttribute attribute = PlaceHolder;  ///< 函数属性
  std::set<Function *> callees;               ///< 函数调用的函数集合
  public:
  static unsigned getcloneIndex() {
    static unsigned cloneIndex = 0;
    cloneIndex += 1;
    return cloneIndex - 1;
  }
  Function* clone(const std::string &suffix = "_" + std::to_string(getcloneIndex()) + "@") const;
  const std::set<Function *>& getCallees() { return callees; }
  void addCallee(Function *callee) { callees.insert(callee); }
  void removeCallee(Function *callee) { callees.erase(callee); }
  void clearCallees() { callees.clear(); }
  std::set<Function *> getCalleesWithNoExternalAndSelf();
  FunctionAttribute getAttribute() const { return attribute; }
  void setAttribute(FunctionAttribute attr) {
    attribute = static_cast<FunctionAttribute>(attribute | attr);
  }
  void clearAttribute() { attribute = PlaceHolder; }
  Type* getReturnType() const { return getType()->as<FunctionType>()->getReturnType(); }
  auto getParamTypes() const { return getType()->as<FunctionType>()->getParamTypes(); }
  auto getBasicBlocks() { return make_range(blocks); }
  block_list& getBasicBlocks_NoRange() { return blocks; }
  BasicBlock* getEntryBlock() { return blocks.front().get(); }
  void insertArgument(Argument *arg) { arguments.push_back(arg); }
  arg_list& getArguments() { return arguments; }
  unsigned getNumArguments() const { return arguments.size(); }
  Argument* getArgument(unsigned index) const {
    assert(index < arguments.size() && "Argument index out of bounds");
    return arguments[index];
  }  ///< 获取位置为index的参数
  auto getArgumentsRange() const {
    return make_range(arguments.begin(), arguments.end());
  }  ///< 获取参数列表的范围
  void removeBasicBlock(BasicBlock *blockToRemove) {
    auto is_same_ptr = [blockToRemove](const std::unique_ptr<BasicBlock> &ptr) { return ptr.get() == blockToRemove; };
    blocks.remove_if(is_same_ptr);
  }
  BasicBlock* addBasicBlock(const std::string &name, BasicBlock *before) {
    // 在指定的基本块之前添加一个新的基本块
    auto it = std::find_if(blocks.begin(), blocks.end(),
                           [before](const std::unique_ptr<BasicBlock> &ptr) { return ptr.get() == before; });
    if (it != blocks.end()) {
      auto newblk = blocks.emplace(it, std::make_unique<BasicBlock>(this, name));
      return newblk->get();  // 返回新添加的基本块指针
    }
    assert(false && "BasicBlock to insert before not found!"); 
    return nullptr;  // 如果没有找到指定的基本块，则返回nullptr
  }  ///< 添加一个新的基本块到某个基本块之前
  BasicBlock* addBasicBlock(const std::string &name = "") {
    blocks.emplace_back(std::make_unique<BasicBlock>(this, name));
    return blocks.back().get();
  }
  BasicBlock* addBasicBlock(BasicBlock *block) {
    blocks.emplace_back(block);
    return block;
  }
  BasicBlock* addBasicBlockFront(BasicBlock *block) {
    blocks.emplace_front(block);
    return block;
  }
  
  /// 清理函数中的所有使用关系
  void cleanup();
  
  void print(std::ostream& os) const;
};

//! Global value declared at file scope
class GlobalValue : public Value {
  friend class Module;

protected:
  Module *parent;           ///< 父模块
  unsigned numDims;         ///< 维度数量
  ValueCounter initValues;  ///< 初值

protected:
  GlobalValue(Module *parent, Type *type, const std::string &name,
              ValueCounter init = {})
      : Value(type, name), parent(parent) {
    assert(type->isPointer());
    // 维度信息已经被记录到Type中，dim只是为了方便初始化
    numDims = 0;
    if (init.size() == 0) {
      unsigned num = 1;
      auto arrayType = type->as<ArrayType>();
      while (arrayType) {
        numDims++;
        num *= arrayType->getNumElements();
        arrayType = arrayType->getElementType()->as<ArrayType>();
      }
      if (dynamic_cast<PointerType *>(type)->getBaseType() == Type::getFloatType()) {
        init.push_back(ConstantFloating::get(0.0F), num); // Use new constant factory
      } else {
        init.push_back(ConstantInteger::get(0), num); // Use new constant factory
      }
    }
    initValues = init;
  }

public:
  unsigned getNumIndices() const {
    return numDims;
  } ///< 获取维度数量
  unsigned getIndex(unsigned index) const {
    assert(index < getNumIndices() && "Index out of bounds for GlobalValue!");
    Type *GlobalValueType = getType()->as<PointerType>()->getBaseType();
    for (unsigned i = 0; i < index; i++) {
      GlobalValueType = GlobalValueType->as<ArrayType>()->getElementType();
    }
    return GlobalValueType->as<ArrayType>()->getNumElements();
  } ///< 获取维度大小(从第0个开始)
  Value* getByIndex(unsigned index) const  {
    return initValues.getValue(index);
  }  ///< 通过一维偏移量index获取初始值
  Value* getByIndices(const std::vector<Value *> &indices) const  {
    int index = 0;
    Type *GlobalValueType = getType()->as<PointerType>()->getBaseType();
    for (size_t i = 0; i < indices.size(); i++) {
      // Ensure dims[i] and indices[i] are ConstantInteger and retrieve their values correctly
      // GlobalValueType->as<ArrayType>()->getNumElements();
      auto dim_val = GlobalValueType->as<ArrayType>()->getNumElements();
      auto idx_val = dynamic_cast<ConstantInteger*>(indices[i]);
      assert(dim_val && idx_val && "Dims and indices must be constant integers");
      index = dim_val * index + idx_val->getInt();
      GlobalValueType = GlobalValueType->as<ArrayType>()->getElementType();
    }
    return getByIndex(index);
  }  ///< 通过多维索引indices获取初始值
  const ValueCounter& getInitValues() const { return initValues; } 
  void print(std::ostream& os) const;
}; // class GlobalValue


class ConstantVariable : public Value {
  friend class Module;

 protected:
  Module *parent;           ///< 父模块
  unsigned numDims;         ///< 维度数量
  ValueCounter initValues;  ///< 值

 protected:
  ConstantVariable(Module *parent, Type *type, const std::string &name, const ValueCounter &init)
      : Value(type, name), parent(parent) {
    assert(type->isPointer());
    // numDims = dims.size();
    numDims = 0;
    if(type->as<PointerType>()->getBaseType()->isArray()) {
      auto arrayType = type->as<ArrayType>();
      while (arrayType) {
        numDims++;
        arrayType = arrayType->getElementType()->as<ArrayType>();
      }
    }
    initValues = init;
  }

 public:
  unsigned getNumIndices() const {
    return numDims;
  } ///< 获取索引数量
  unsigned getIndex(unsigned index) const {
    assert(index < getNumIndices() && "Index out of bounds for ConstantVariable!");
    Type *ConstantVariableType = getType()->as<PointerType>()->getBaseType();
    for (unsigned i = 0; i < index; i++) {
      ConstantVariableType = ConstantVariableType->as<ArrayType>()->getElementType();
    }
    return ConstantVariableType->as<ArrayType>()->getNumElements();
  } ///< 获取索引个数(从第0个开始)
  Value* getByIndex(unsigned index) const { return initValues.getValue(index); }  ///< 通过一维位置index获取值
  Value* getByIndices(const std::vector<Value *> &indices) const {
    int index = 0;
    // 计算偏移量
    Type *ConstantVariableType = getType()->as<PointerType>()->getBaseType();
    for (size_t i = 0; i < indices.size(); i++) {
      // Ensure dims[i] and indices[i] are ConstantInteger and retrieve their values correctly
      // ConstantVariableType->as<ArrayType>()->getNumElements();
      auto dim_val = ConstantVariableType->as<ArrayType>()->getNumElements();
      auto idx_val = dynamic_cast<ConstantInteger*>(indices[i]);
      assert(dim_val && idx_val && "Dims and indices must be constant integers");
      index = dim_val * index + idx_val->getInt();
      ConstantVariableType = ConstantVariableType->as<ArrayType>()->getElementType();
    }

    return getByIndex(index);
  }                                                        ///< 通过多维索引indices获取初始值
  const ValueCounter& getInitValues() const { return initValues; }                           ///< 获取初始值
  void print(std::ostream& os) const;
  void print_init(std::ostream& os) const;
};

using SymbolTableNode = struct SymbolTableNode {
  SymbolTableNode *pNode;            ///< 父节点
  std::vector<SymbolTableNode *> children;  ///< 子节点列表
  std::map<std::string, Value *> varList;    ///< 变量列表
};


class SymbolTable {
 private:
  SymbolTableNode *curNode{};                              ///< 当前所在的作用域(符号表节点)
  std::map<std::string, unsigned> variableIndex;           ///< 变量命名索引表
  std::vector<std::unique_ptr<GlobalValue>> globals;       ///< 全局变量列表
  std::vector<std::unique_ptr<ConstantVariable>> globalconsts;   ///< 全局常量列表
  std::vector<std::unique_ptr<SymbolTableNode>> nodeList;  ///< 符号表节点列表

 public:
  SymbolTable() = default;

  Value* getVariable(const std::string &name) const;  ///< 根据名字name以及当前作用域获取变量
  Value* addVariable(const std::string &name, Value *variable);               ///< 添加变量
  void registerParameterName(const std::string &name);                        ///< 注册函数参数名字，避免alloca重名
  void addVariableDirectly(const std::string &name, Value *variable);        ///< 直接添加变量到当前作用域，不重命名
  std::vector<std::unique_ptr<GlobalValue>>& getGlobals();                  ///< 获取全局变量列表
  const std::vector<std::unique_ptr<ConstantVariable>>& getConsts() const;  ///< 获取全局常量列表
  void enterNewScope();                                                              ///< 进入新的作用域
  void leaveScope();                                                                 ///< 离开作用域
  bool isInGlobalScope() const;                                              ///< 是否位于全局作用域
  void enterGlobalScope();                                                           ///< 进入全局作用域
  bool isCurNodeNull() { return curNode == nullptr; }
  
  /// 清理符号表中的所有内容
  void cleanup();
};

//! IR unit for representing a SysY compile unit
class Module {
 protected:
  std::map<std::string, std::unique_ptr<Function>> externalFunctions;  ///< 外部函数表
  std::map<std::string, std::unique_ptr<Function>> functions;          ///< 函数表
  SymbolTable variableTable;                                           ///< 符号表

 public:
  Module() = default;

 public:
  Function* createFunction(const std::string &name, Type *type) {
    auto result = functions.try_emplace(name, new Function(this, type, name));
    if (!result.second) {
      return nullptr;
    }
    return result.first->second.get();
  }  ///< 创建函数
  Function* createExternalFunction(const std::string &name, Type *type) {
    auto result = externalFunctions.try_emplace(name, new Function(this, type, name));
    if (!result.second) {
      return nullptr;
    }
    return result.first->second.get();
  }  ///< 创建外部函数
  ///< 变量创建伴随着符号表的更新
  GlobalValue* createGlobalValue(const std::string &name, Type *type, const ValueCounter &init = {}) {
    bool isFinished = variableTable.isCurNodeNull();
    if (isFinished) {
      variableTable.enterGlobalScope();
    }
    auto result = variableTable.addVariable(name, new GlobalValue(this, type, name, init));
    if (isFinished) {
      variableTable.leaveScope();
    }
    if (result == nullptr) {
      return nullptr;
    }
    return dynamic_cast<GlobalValue *>(result);
  }  ///< 创建全局变量
  ConstantVariable* createConstVar(const std::string &name, Type *type, const ValueCounter &init) {
    auto result = variableTable.addVariable(name, new ConstantVariable(this, type, name, init));
    if (result == nullptr) {
      return nullptr;
    }
    return dynamic_cast<ConstantVariable *>(result);
  }  ///< 创建常量
  void addVariable(const std::string &name, AllocaInst *variable) {
    variableTable.addVariable(name, variable);
  }  ///< 添加变量
  void addVariableDirectly(const std::string &name, AllocaInst *variable) {
    variableTable.addVariableDirectly(name, variable);
  }  ///< 直接添加变量到当前作用域，不重命名
  void registerParameterName(const std::string &name) {
    variableTable.registerParameterName(name);
  }  ///< 注册函数参数名字，避免alloca重名
  Value* getVariable(const std::string &name) {
    return variableTable.getVariable(name);
  }  ///< 根据名字name和当前作用域获取变量
  Function* getFunction(const std::string &name) const {
    auto result = functions.find(name);
    if (result == functions.end()) {
      return nullptr;
    }
    return result->second.get();
  }  ///< 获取函数
  Function* getExternalFunction(const std::string &name) const {
    auto result = externalFunctions.find(name);
    if (result == externalFunctions.end()) {
      return nullptr;
    }
    return result->second.get();
  }  ///< 获取外部函数
  std::map<std::string, std::unique_ptr<Function>>& getFunctions() { return functions; }  ///< 获取函数列表
  const std::map<std::string, std::unique_ptr<Function>>& getExternalFunctions() const {
    return externalFunctions;
  }  ///< 获取外部函数列表
  std::vector<std::unique_ptr<GlobalValue>>& getGlobals() {
    return variableTable.getGlobals();
  }  ///< 获取全局变量列表
  const std::vector<std::unique_ptr<ConstantVariable>>& getConsts() const {
    return variableTable.getConsts();
  }                                                        ///< 获取常量列表
  void enterNewScope() { variableTable.enterNewScope(); }  ///< 进入新的作用域

  void leaveScope() { variableTable.leaveScope(); }  ///< 离开作用域

  bool isInGlobalArea() const { return variableTable.isInGlobalScope(); }  ///< 是否位于全局作用域

  /// 清理模块中的所有对象，包括函数、基本块、指令等
  void cleanup();

  void print(std::ostream& os) const;
};

/*!
 * @}
 */

} // namespace sysy
