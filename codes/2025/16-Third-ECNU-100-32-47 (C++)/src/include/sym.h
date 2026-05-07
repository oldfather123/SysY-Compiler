/**
 * @file sym.h
 * 该头文件定义了编译过程中符号信息存储的相关数据结构
 */
#ifndef AAAC_SYM_H
#define AAAC_SYM_H

#include "AST/type.h"
#include <memory>
#include <ostream>
#pragma once

#include "./utils/property_manager.h"
#include "common.h"
#include "config.h"

#include <list>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_set>
#include <vector>

namespace aaac {
// Forward declarations
namespace frontend {
class Type;
}
namespace backend {
class Insn;

using UseChainNode = std::shared_ptr<Insn>;
using UseChain = std::list<UseChainNode>;
} // namespace backend

namespace sym {
/**
 * @brief 基础类型
 *
 * @todo 需要根据语言语法设计类型
 */
enum class BaseType : uint8_t { UNIMPLEMENTED = 0, VOID, INT, FLOAT };

/**
 * @brief 标签条目信息
 *
 * 用于记录代码标签的位置信息
 */
struct LabelEntry {
  std::string name;
  int offset = -1;
};

/**
 * @brief 基本块的引用节点
 *
 */
struct RefBlock {
  std::weak_ptr<backend::BasicBlock> block = {};
  RefBlock *next = nullptr;
};
DEF_LIST(RefBlock)

/**
 * @brief 重命名的上下文
 *
 * 用于SSA形式中的变量重命名过程
 */
struct RenameContext {
  int counter = 0;
  int stack[64]{};
  int stack_idx = 0;
};

/**
 * @brief 活跃变量分析信息
 *
 * 记录变量的活跃范围和使用链信息
 */
struct LiveInfo : utils::BaseProperty {
  int liveness_range = 0;
  int loop_depth = 0;
  std::unique_ptr<backend::UseChain> use_chain = nullptr;
  std::shared_ptr<backend::Insn> last_assignment = nullptr;

  void setLiveness(int endpoint) {
    if (liveness_range >= endpoint)
      return;
    liveness_range = endpoint;
  }
};

/**
 * @brief 寄存器分配信息
 *
 * 记录变量在寄存器分配阶段的信息
 */
struct RegAllocInfo : utils::BaseProperty {
  int consumed_registers = 0; ///< 消耗的寄存器
  int stack_offset = -1;      ///< 正数，栈偏移量（-1表示未分配）
  int fp_offset = 0;          ///< 负数，以 s0 为基，生成代码时计算
  bool precoloured = false;
};

/**
 * @brief 常量传播信息
 *
 * 记录变量的常量传播优化信息
 */
struct ConstPropInfo : utils::BaseProperty {
  bool is_compile_time_const = false;
  int initialization_value = 0;
};

struct RefInfo : utils::BaseProperty {
  RenameContext rename_ctx{};
  RefBlockList ref_blocks{};
};

/**
 * @brief 作用域属性
 *
 * 记录变量在作用域中的数组信息
 */
struct ScopeInfo : utils::BaseProperty {
  std::shared_ptr<sym::Var> base_var = nullptr;
  int array_size = 0;
  int subscript = -1;
  std::shared_ptr<sym::Var> scripts[64]{};
  std::shared_ptr<sym::Var> getStackTopSubscriptVar() const {
    ///< @todo 尚未实现
    return nullptr;
  }
};
/**
 * @brief 基础信息
 *
 * 记录变量的基础信息，它的存在是因为我懒得写一大堆初始化函数，
 * 索性全部交给 PropertyManager 自生自灭
 */
struct BasicInfo : utils::BaseProperty {
  int pointer_depth = 0;
  bool is_function = false;
  bool is_global = false;
  bool is_ternary_result = false;
  bool is_logical_result = false;
};

/**
 * @brief 记录变量的相关信息
 *
 * @todo 需要进一步进行重构，目前不完善
 * 创建一个新的Var的两种较为推荐的方式：
 * - globals.genTempVar()创建自动命名的临时变量
 * - 直接在作用域中通过createVariable()创建变量
 * 这样避免每次都要输入一长串的std::shared_ptr
 */
class Var : public Createable<Var>, public DebugDumpImpl {
  friend struct Createable<Var>;
private:
  static unsigned int temp_count;
  std::shared_ptr<frontend::Type> type;
  std::string var_name;

  static inline std::unordered_set<std::type_index> allowed_types_ = {
      std::type_index(typeid(ConstPropInfo)),
      std::type_index(typeid(LiveInfo)),
      std::type_index(typeid(RefInfo)),
      std::type_index(typeid(RegAllocInfo)),
      std::type_index(typeid(ScopeInfo)),
      std::type_index(typeid(BasicInfo))};

public:

  Var(std::shared_ptr<frontend::Type> type, std::string var_name)
      : type(type), var_name(std::move(var_name)) {}

  Var() : Var(nullptr, "") {}
  virtual ~Var() {
    try {
      utils::g_property_mgr.remove_all(this);
    } catch (const std::exception &e) {
      LOG_ERROR("Property cleanup failed: %s", e.what())
    } catch (...) {
      LOG_ERROR("Exception occurred during Var destruction")
#ifdef DEBUG_MODE
      panic("This should never happen")
#endif
    }
  }

  std::shared_ptr<frontend::Type> getType() const { return type; }
  const std::string &getName() const { return var_name; }
  static std::shared_ptr<Var> generateTemp(std::shared_ptr<frontend::Type> type,
                                           const std::string &prefix) {
    return Var::create(type, prefix + std::to_string(temp_count++));
  }

  bool isGlobal() const {
    const auto *info = prop<BasicInfo>();
    return info && info->is_global;
  }

  bool isFloatVar() const {
    auto t = getType();
    if (!t) return false;
    return t == frontend::TypeFactory::FloatTy;
  }

  bool isFunction() const {
    const auto *info = prop<BasicInfo>();
    return info && info->is_function;
  }

  bool isConst() const {
    const auto *info = prop<ConstPropInfo>();
    return info && info->is_compile_time_const;
  }

  int getPointerDepth() const {
    const auto *info = prop<BasicInfo>();
    return info ? info->pointer_depth : 0;
  }

  void setPointerDepth(int depth) { prop<BasicInfo>().pointer_depth = depth; }

  template <typename T> static bool is_allowed() {
    return allowed_types_.find(std::type_index(typeid(T))) !=
           allowed_types_.end();
  }

  template <typename T> T &prop() {
    if (!is_allowed<T>()) {
#ifdef DEBUG_MODE
      LOG_INFO("Illegal property read caught\n")
#endif
      throw std::invalid_argument("Type not allowed for Var property");
    }
    return utils::g_property_mgr.init<T>(this);
  }

  template <typename T> const T *prop() const {
    if (!is_allowed<T>()) {
#ifdef DEBUG_MODE
      LOG_INFO("Illegal property read caught\n")
#endif
      throw std::invalid_argument("Type not allowed for Var property");
    }
    return utils::g_property_mgr.get<T>(this);
  }
  std::ostream &dump(std::ostream &os) const override;
};
inline unsigned int Var::temp_count = 1000;

/**
 * @brief 函数
 * declaration of a function
 */
class Function : public Createable<Function>,
                    public DebugDumpImpl {
private:
  std::shared_ptr<frontend::FunctionType> functiontype;
  std::string name;
  std::vector<std::shared_ptr<Var>> parameters;
  [[maybe_unused]] bool va_args = false;
  [[maybe_unused]] int stack_frame_size = 8; ///< RV64 XLEN=64
  std::shared_ptr<backend::Function> fn = {};

public:
  Function(std::shared_ptr<frontend::FunctionType> functiontype, const std::string &name,
           bool va_args, std::shared_ptr<backend::Function> fn)
      : functiontype(functiontype), name(name), parameters(), va_args(va_args),
        stack_frame_size(8), fn(fn) {
    // parameters.reserve(config::MAX_PARAMS);
    if(functiontype == nullptr) return;
    int index = 0;
    for(auto type : functiontype->getFormals()) {
      parameters.push_back(Var::create(type, "fparm_"+std::to_string(index++)));
    }
  }
  Function() : Function(nullptr, "", false, nullptr){};
  Function(const Function &) = default;
  Function(Function &&) = default;
  Function &operator=(const Function &) = default;
  ~Function() = default;
  size_t getParametersCount() const { return this->parameters.size(); }
  std::vector<std::shared_ptr<Var>> getParameters() const {
    return this->parameters;
  }
  std::shared_ptr<frontend::Type> getReturnTypeName() const {
    return this->functiontype->getRetType();
  }
  std::vector<std::shared_ptr<frontend::Type>>
  getFormalsTypeName() const {
    return this->functiontype->getFormals();
  }
  std::shared_ptr<frontend::FunctionType>
  getFunctionTypeName() const {
    return this->functiontype;
  }
  std::string getName() const { return this->name; }
  std::shared_ptr<backend::Function> getFunctionBody() const {
    // return this->fn.lock();
    return this->fn;
  }
  void setFunctionBody(std::shared_ptr<backend::Function> func) {
    this->fn = func;
  }
  std::ostream &dump(std::ostream &)const override;
};

/**
 * @brief 符号
 */
struct Symbol {
  std::shared_ptr<Var> var = nullptr;
  int index_in_scope = 0;
  Symbol *next_in_scope = nullptr;
};
DEF_LIST(Symbol)

} // namespace sym
} // namespace aaac

#endif // AAAC_SYM_H
