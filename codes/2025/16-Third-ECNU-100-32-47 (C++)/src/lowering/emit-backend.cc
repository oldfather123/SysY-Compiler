/**
 * WIP!!!!!
 *
 * 目前中端和后端接口还没确定下来，任何代码都不要依赖这里的实现
 * 仅作为草稿用，请勿引用！！！
 */
#include "ADT/CFG.h"
#include "AST/type.h"
#include "backend.h"
#include "common.h"
#include "frontend.h"
#include "global.h"
#include "reg_alloc.h"
#include "riscv.h"
#include "sym.h"

#include <cassert>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace aaac {
namespace frontend {
static const std::vector<int>
getArrayTypeLengths(std::shared_ptr<ArrayType> arrayTy) {
  std::vector<int> lengths;
  auto currentType = arrayTy;
  while (currentType) {
    lengths.push_back(currentType->getArrayLen());
    currentType =
        std::dynamic_pointer_cast<ArrayType>(currentType->getBaseType());
  }
  // for(auto ind : lengths) {
  //   std::cout << ind << " ";
  // }
  // std::cout << std::endl;
  return lengths;
}

static Operand convertToOperand(std::variant<int, float> value) {
  return std::visit(
      [](auto &&arg) -> Operand {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int>) {
          return arg;
        } else if constexpr (std::is_same_v<T, float>) {
          return arg;
        } else {
          throw std::runtime_error(
              "Unexpected type in std::variant<int, float>");
        }
      },
      value);
}

static std::shared_ptr<sym::Var>
createOffsetOfArray(backend::InsnList &list,
                    std::shared_ptr<ArrayType> arrayType, OperandList indexes) {
  std::shared_ptr<sym::Var> offset_var =
      sym::Var::generateTemp(TypeFactory::IntTy, "offset_");
  auto lengths = getArrayTypeLengths(arrayType);
  // 当indexes的长度小于lengths的长度时，是获取某个子数组的地址
  for (int i = lengths.size() - indexes.size(); i > 0; --i) {
    indexes.push_back(Operand(0));
  }
  std::vector<int> post(lengths.size(), arrayType->getElementType()->getTypeSize());
  for (int i = 1; i < lengths.size(); i++) {
    post[i - 1] = lengths[i];
  }
  for (int i = post.size() - 2; i >= 0; i--) {
    post[i] = post[i] * post[i + 1];
  }
  auto len_iter = lengths.begin();
  auto index_iter = indexes.begin();
  int const_offset = 0;
  bool isConst = true;
  for (int i = 0; len_iter != lengths.end(); ++len_iter, ++index_iter, i++) {
    auto index = *index_iter;
    if (std::holds_alternative<int>(index)) {
      int val = std::get<int>(index);
      const_offset += post[i] * val;
    } else {
      isConst = false;
    }
  }
  list.push_back(backend::CommonInsn::create(backend::OpCode::Assign, offset_var, Operand(const_offset),nullptr));
  if (isConst) {
    // 所有的下标都是常数
    return offset_var;
  }
  std::shared_ptr<sym::Var> temp = sym::Var::generateTemp(TypeFactory::IntTy, "temp_");
  len_iter = lengths.begin();
  index_iter = indexes.begin();
  for (int i = 0; len_iter != lengths.end(); ++len_iter, ++index_iter, i++) {
    auto index = *index_iter;
    if (!std::holds_alternative<int>(index)) {
      list.push_back(backend::CommonInsn::create(backend::OpCode::Assign,temp, Operand(post[i]), nullptr));
      list.push_back(backend::CommonInsn::create(backend::OpCode::Mul64,temp,temp,*index_iter));
      list.push_back(backend::CommonInsn::create(backend::OpCode::Add64,offset_var,offset_var,temp));
    }
  }
  return offset_var;
  // list.push_back(backend::CommonInsn::create(backend::OpCode::Assign,
  //                                             offset_var, Operand(0), nullptr));
  // for (; len_iter != lengths.end(); ++len_iter, ++index_iter) {
  //   list.push_back(backend::CommonInsn::create(backend::OpCode::Mul64, offset_var,
  //                                               offset_var, Operand(*len_iter)));
  //   list.push_back(backend::CommonInsn::create(backend::OpCode::Add64, offset_var,
  //                                              offset_var, *index_iter));
  // }
  // // Assert(index_iter == indexes.end(), "Dimensions and indexes mismatch");
  // list.push_back(backend::CommonInsn::create(
  //     backend::OpCode::Mul64, offset_var, offset_var,
  //     Operand(arrayType->getElementType()->getTypeSize())));
  // return offset_var;
}

static int createStaticIndex(std::shared_ptr<ArrayType> arrayType,
                              const std::vector<int>& indexes) {
    auto lengths = getArrayTypeLengths(arrayType);

    int offset = 0;
    for (size_t i = 0; i < lengths.size(); ++i) {

        offset = offset * lengths[i] + indexes[i];
    }

    return offset;
}

static int getArraySize(std::shared_ptr<ArrayType> arrayType) {
  auto lengths = getArrayTypeLengths(arrayType);
  int size = std::accumulate(lengths.begin(), lengths.end(), 1,
                             std::multiplies<int>());
  return size;
}

static void addScopeInfo(std::shared_ptr<ArrayType> type,
                         std::shared_ptr<sym::Var> var) {
  var->prop<sym::ScopeInfo>().array_size = getArraySize(type);
}

[[nodiscard]] backend::InsnList Module::generateInsnList() {
  std::map<std::string,
           std::variant<int, std::vector<int>, float, std::vector<float>>>
      global_init;
  backend::InsnList list;
  for (auto var : this->global_vars) {
    if (auto array_type = std::dynamic_pointer_cast<ArrayType>(var->getType()))
      addScopeInfo(array_type, var);
    globals.addGlobalVar(var->getType(), var->getName());
    // 先把所有的全局变量加入到global_init中
    // 如果有相关的initializeNode就直接修改即可
    if (var->getType()->isArrayType()) {
      auto arr_type = std::dynamic_pointer_cast<ArrayType>(var->getType());
      int size = getArraySize(arr_type);
      if (arr_type->getElementType() == TypeFactory::IntTy) {
        // global_init[var->getName()] =
        //     backend::GlobalVarValue(var->getName(), std::vector<int>(size,
        //     0));
        global_init.insert(
            std::pair<std::string, std::variant<int, std::vector<int>, float,
                                                std::vector<float>>>(
                var->getName(), std::vector<int>(size, 0)));
      } else {
        global_init.insert(
            std::pair<std::string, std::variant<int, std::vector<int>, float,
                                                std::vector<float>>>(
                var->getName(), std::vector<float>(size, 0.0f)));
      }
    } else {
      if (var->getType() == TypeFactory::IntTy) {
        global_init.insert(
            std::pair<std::string, std::variant<int, std::vector<int>, float,
                                                std::vector<float>>>(
                var->getName(), 0));
      } else {
        global_init.insert(
            std::pair<std::string, std::variant<int, std::vector<int>, float,
                                                std::vector<float>>>(
                var->getName(), 0.0f));
      }
    }
  }
  for (auto binding : this->bindings) {
    if (auto initialize = std::dynamic_pointer_cast<InitializeNode>(binding)) {
      if (initialize->getVar()->getType()->isArrayType()) {
        auto array_type = std::dynamic_pointer_cast<ArrayType>(
            initialize->getVar()->getType());
        int size = createStaticIndex(array_type, initialize->getIndexes());
        if (array_type->getElementType() == TypeFactory::IntTy) {
          assert(std::get<std::vector<int>>(
              global_init.at(initialize->getVar()->getName())).size() > size);
          std::get<std::vector<int>>(
              global_init.at(initialize->getVar()->getName()))[size] =
              std::get<int>(initialize->getValue());
        } else {
          assert(std::get<std::vector<float>>(
              global_init.at(initialize->getVar()->getName())).size() > size);
          std::get<std::vector<float>>(
              global_init.at(initialize->getVar()->getName()))[size] =
              std::get<float>(initialize->getValue());
        }
      } else {
        if (initialize->getVar()->getType() == TypeFactory::IntTy) {
          std::get<int>(global_init.at(initialize->getVar()->getName())) =
              std::get<int>(initialize->getValue());
        } else {
          std::get<float>(global_init.at(initialize->getVar()->getName())) =
              std::get<float>(initialize->getValue());
        }
      }
    } else if (auto function =
                   std::dynamic_pointer_cast<FunctionNode>(binding)) {
      auto cfg = this->cfg_map.at(function.get());
      function->emitInsn(list); // Label & Start
      // Generate Alloca insn
      for (auto var : cfg->getLocals()) {
        if (var->getType()->isArrayType()) {
          int size = 1;
          for (auto dimension : getArrayTypeLengths(
                   std::dynamic_pointer_cast<ArrayType>(var->getType())))
            size *= dimension;

          list.push_back(backend::CommonInsn::create(
              backend::OpCode::Alloca, var, Operand(size), Operand(nullptr)));
        }
      }
      // auto backend_func = cfg->getFunction()->getFunctionBody();
      auto backend_func = std::make_shared<backend::Function>();
      std::unordered_map<std::shared_ptr<frontend::BasicBlock>,
               std::shared_ptr<backend::BasicBlock>> &map_fe_be = cfg->getBBMap();
      // std::shared_ptr<backend::BasicBlock> be_head =
      // globals.createBasicBlock(), be_tail = be_head;
      std::shared_ptr<backend::BasicBlock> be_bb_head = nullptr;
      ControlFlowGraph::TraverseHandler preorder =
      // generate backend bb based on frontend bb
      [&](std::shared_ptr<sym::Function> func,
          std::shared_ptr<BasicBlock> fe_bb) {
        backend::InsnList bb_list;
        for (auto insn : *fe_bb) {
          insn->emitInsn(bb_list);
        }
        list.insert(list.end(), bb_list.begin(), bb_list.end());

        std::string BBName = "<unknown>";
        if (auto label = std::dynamic_pointer_cast<backend::LabelledInsn>(bb_list.front())) {
          BBName = label->getLabel();
          // std::cout << "add " << BBName << "\n";
        }

        auto be_bb = globals.createBasicBlock();
        // auto backend_func = cfg->getFunction()->getFunctionBody();
        be_bb->setInsnList(bb_list);
        map_fe_be[fe_bb] = be_bb;
        std::cout << fe_bb->getLabel()->getName() << " " << fe_bb << " -> " << be_bb << "\n";
        if (be_bb_head == nullptr) {
          be_bb_head = be_bb;
        } else {
          be_bb_head->setNext(be_bb);
          be_bb_head = be_bb;
        }
        if (fe_bb->getSuccEdges().size() == 0) {
          backend_func->setExitBasicBlock(be_bb);
          // std::cout << "Exit BB " << BBName << "\n";
        }
        if (fe_bb == cfg->getStartBasicBlock()) {
          backend_func->setStartBasicBlock(be_bb);
          // std::cout << "Start BB " << BBName << "\n";
        }
      };

      auto cfg_copy = cfg;
      ControlFlowGraph::TraverseHandler postorder =
      [&](std::shared_ptr<sym::Function> func,
          std::shared_ptr<BasicBlock> fe_bb) {
        // for(auto &e : fe_bb->getSuccEdges()) {
        //   switch (e.getFlag()) {
        //     case BasicBlock::JUMP:
        //     map_fe_be[fe_bb]->setNext(map_fe_be[e.getBB()]); break; case
        //     BasicBlock::THEN:
        //     map_fe_be[fe_bb]->setThen(map_fe_be[e.getBB()]); break; case
        //     BasicBlock::ELSE:
        //     map_fe_be[fe_bb]->setElse(map_fe_be[e.getBB()]); break;
        //     default: break;
        //   }
        // }

        // 下面的算法存在问题
        // if (be_bb_head) {
        //   map_fe_be[fe_bb]->setNext(be_bb_head);
        //   be_bb_head = map_fe_be[fe_bb];
        // }
        // if (fe_bb->getSuccEdges().size() == 0) {
        //   be_bb_head = map_fe_be[fe_bb];
        //   backend_func->setExitBasicBlock(map_fe_be[fe_bb]);
        // }
        // if (fe_bb == cfg_copy->getStartBasicBlock()) {
        //   backend_func->setStartBasicBlock(map_fe_be[fe_bb]);
        //   auto list = be_bb_head->getInsnList();
        //   list.push_front(backend::StartInsn::create());
        //   be_bb_head->setInsnList(list);
        //   assert(be_bb_head == map_fe_be[fe_bb]);
        // }
      };

      cfg->forwardTraverse(preorder, postorder);
      backend_func->name = cfg->getFunction()->getName();

      for(auto esc : function->getEscape()) {
        backend_func->addEscape(esc);
      }
      // shift of view
      auto ViewShiftBB = globals.createBasicBlock();
      backend::InsnList shift_of_view;
      int int_index = 0, float_index = 0;
      shift_of_view.push_back(backend::LabelledInsn::create( backend::OpCode::Label, ".shift_of_view_" + function->getFunction()->getName()));
      for(auto parm : function->getFunction()->getParameters()) {
        if (parm->isFloatVar()) {
          if (float_index <= 7) {
            auto arg = std::make_shared<sym::Var>(frontend::TypeFactory::FloatTy, "f" + std::to_string(float_index));
            arg->prop<sym::RegAllocInfo>().consumed_registers = backend::reg_to_index(backend::RISCV_Reg::__fa0) + float_index;
            arg->prop<sym::RegAllocInfo>().precoloured = true;
            shift_of_view.push_back(backend::CommonInsn::create(backend::OpCode::Assign, parm, Operand(arg), nullptr));
          } else {
            int offset = std::max(float_index - 8, 0);
            offset += std::max(int_index - 8, 0);
            shift_of_view.push_back(backend::CommonInsn::create(backend::OpCode::Load, parm, Operand(8 * offset), nullptr));
          }
          float_index++;
        } else {
          if (int_index <= 7) {
            auto arg = std::make_shared<sym::Var>(frontend::TypeFactory::IntTy, "a" + std::to_string(int_index));
            arg->prop<sym::RegAllocInfo>().consumed_registers = backend::reg_to_index(backend::RISCV_Reg::__a0) + int_index;
            arg->prop<sym::RegAllocInfo>().precoloured = true;
            shift_of_view.push_back(backend::CommonInsn::create(backend::OpCode::Assign, parm, Operand(arg), nullptr));
          } else {
            int offset = std::max(float_index - 8, 0);
            offset += std::max(int_index - 8, 0);
            shift_of_view.push_back(backend::CommonInsn::create(backend::OpCode::Load, parm, Operand(8 * offset), nullptr));
          }
          int_index++;
        }
      }
      ViewShiftBB->setInsnList(shift_of_view);
      ViewShiftBB->setNext(backend_func->getStartBasicBlock());
      backend_func->setStartBasicBlock(ViewShiftBB);

      cfg->getFunction()->setFunctionBody(backend_func);
      globals.addFunction(cfg->getFunction());
    }
  }
  for (auto [name, value] : global_init) {
    if (std::holds_alternative<int>(value)) {
      globals.addInitGlobalVar(
          backend::GlobalVarValue(name, std::get<int>(value)));
    } else if (std::holds_alternative<float>(value)) {
      globals.addInitGlobalVar(
          backend::GlobalVarValue(name, std::get<float>(value)));
    } else if (std::holds_alternative<std::vector<int>>(value)) {
      globals.addInitGlobalVar(
          backend::GlobalVarValue(name, std::get<std::vector<int>>(value)));
    } else if (std::holds_alternative<std::vector<float>>(value)) {
      globals.addInitGlobalVar(
          backend::GlobalVarValue(name, std::get<std::vector<float>>(value)));
    }
  }
  return list;
}
void InitializeNode::emitInsn(backend::InsnList &list) {
  // auto val = convertToOperand(this->value);
  // if (auto arrayType =
  //         std::dynamic_pointer_cast<ArrayType>(this->var->getType())) {
  //   int offset = createStaticOffset(arrayType, this->indexes);
  //   auto pos = sym::Var::generateTemp(TypeFactory::IntTy, "gi_");
  //   list.push_back(backend::CommonInsn::create(
  //       backend::OpCode::Add, pos, Operand(this->var), Operand(offset)));
  //   list.push_back(backend::CommonInsn::create(backend::OpCode::Write,
  //   nullptr,
  //                                              val, Operand(pos)));
  // } else {
  //   list.push_back(backend::CommonInsn::create(backend::OpCode::Write,
  //   nullptr,
  //                                              val, Operand(this->var)));
  // }

  // 什么都不需要做，绑定变量和初值的逻辑在generateInsnList中
}
void FunctionNode::emitInsn(backend::InsnList &list) {
  list.push_back(backend::LabelledInsn::create(
      backend::OpCode::Label, this->function.lock()->getName()));
  list.push_back(backend::StartInsn::create());
}

void LabelNode::emitInsn(backend::InsnList &list) {
  list.push_back(
      backend::LabelledInsn::create(backend::OpCode::Label, this->label));
}
// Global Load/Store ? **TODO**
// Check global/local ?
// Use GlobalLoad/Store for global variables
void LoadNode::emitInsn(backend::InsnList &list) {
  bool IsGlobal = this->getArray()->isGlobal();
  auto backend_operand = this->getArray()->isGlobal()
                             ? backend::OpCode::GlobalLoad
                             : backend::OpCode::Read;
  list.push_back(backend::CommonInsn::create(backend::OpCode::Nop,nullptr,nullptr,nullptr));
  if (auto arrayType =
          std::dynamic_pointer_cast<ArrayType>(this->array->getType())) {
    auto offset_var = createOffsetOfArray(list, arrayType, this->indexes);
    std::shared_ptr<sym::Var> base = sym::Var::generateTemp(TypeFactory::IntTy, "basel_");
    std::string name = this->getArray()->getName();
    std::cout << "emit LoadNode " << name << " " << *this->getArray()->getType() << " base " << base->getName()  << " " << base->prop<sym::RegAllocInfo>().precoloured  << "\n";
    // base->prop<sym::RegAllocInfo>().precoloured = false;
    if (name.find("fparm_") == 0) {
      // 形参数组变量本身存的就是基地址
      // base = this->getArray();
      list.push_back(backend::CommonInsn::create(backend::OpCode::Assign, base, this->getArray(), nullptr));
    } else {
      if(IsGlobal) {
        list.push_back(backend::CommonInsn::create(backend::OpCode::LoadLabelAddress, base, Operand(0), Operand(0), Operand(this->getArray())));
      } else {
        list.push_back(backend::CommonInsn::create(backend::OpCode::LoadEscapeAddress, base, Operand(0), Operand(0), Operand(this->getArray())));
      }
    }
    // list.push_back(backend::CommonInsn::create(backend_operand, offset_var,Operand(offset_var),Operand(this->array)));
    list.push_back(backend::CommonInsn::create(backend::OpCode::Add64, offset_var, Operand(offset_var), Operand(base)));
    auto def = this->var;
    if (!def->getType()->isArrayType()) {
      list.push_back(backend::CommonInsn::create(backend::OpCode::Read, def,
                                                 Operand(offset_var), nullptr));
    } else {
      list.push_back(backend::CommonInsn::create(backend::OpCode::Assign, def, Operand(offset_var), nullptr));
    }
    std::cout << "emit LoadNode after " << name << " " << *this->getArray()->getType() << " base " << base->getName() << " " << base->prop<sym::RegAllocInfo>().precoloured << "\n";
  } else {
    assert(IsGlobal);
    // list.push_back(backend::CommonInsn::create(backend_operand, this->var,
                                              //  Operand(this->array), nullptr));
    std::shared_ptr<sym::Var> g_var = sym::Var::generateTemp(TypeFactory::IntTy, "g_var");
    g_var->prop<sym::RegAllocInfo>().precoloured = false;
    list.push_back(backend::CommonInsn::create(backend::OpCode::LoadLabelAddress, g_var, Operand(0), Operand(0), Operand(this->getArray())));
    list.push_back(backend::CommonInsn::create(backend::OpCode::Read, this->var, g_var, nullptr));

  }
  list.push_back(backend::CommonInsn::create(backend::OpCode::Nop,nullptr,nullptr,nullptr));
}
void StoreNode::emitInsn(backend::InsnList &list) {
  bool IsGlobal = this->getArray()->isGlobal();
  auto backend_operand = this->getArray()->isGlobal()
                             ? backend::OpCode::GlobalStore
                             : backend::OpCode::Write;
  if (auto arrayType =
          std::dynamic_pointer_cast<ArrayType>(this->array->getType())) {
    auto offset_var = createOffsetOfArray(list, arrayType, this->indexes);
    std::shared_ptr<sym::Var> base = sym::Var::generateTemp(TypeFactory::IntTy, "bases_");
    // base->prop<sym::RegAllocInfo>().precoloured = false;
    std::string name = this->getArray()->getName();
    std::cout << "emit StoreNode " << name << " " << *this->getArray()->getType() << " base " << base->getName()  << " " << base->prop<sym::RegAllocInfo>().precoloured << " " << base << "\n";
    if (name.find("fparm_") == 0) {
      // 形参数组变量本身存的就是基地址
      // base = this->getArray();
      list.push_back(backend::CommonInsn::create(backend::OpCode::Assign, base, this->getArray(), nullptr));
    } else {
      if(IsGlobal) {
        list.push_back(backend::CommonInsn::create(backend::OpCode::LoadLabelAddress, base, Operand(0), Operand(0), Operand(this->getArray())));
      } else {
        list.push_back(backend::CommonInsn::create(backend::OpCode::LoadEscapeAddress, base, Operand(0), Operand(0), Operand(this->getArray())));
      }
    }
    // list.push_back(backend::CommonInsn::create(backend_operand, offset_var,Operand(offset_var),Operand(this->array)));
    list.push_back(backend::CommonInsn::create(backend::OpCode::Add64, offset_var, Operand(offset_var), Operand(base)));
    list.push_back(backend::CommonInsn::create(backend::OpCode::Write, nullptr,
                                            value, Operand(offset_var)));
  } else {
    assert(IsGlobal);
    // list.push_back(backend::CommonInsn::create(
    //     backend_operand, nullptr, this->value, Operand(this->array)));
    std::shared_ptr<sym::Var> g_var = sym::Var::generateTemp(TypeFactory::IntTy, "g_var");
    list.push_back(backend::CommonInsn::create(backend::OpCode::LoadLabelAddress, g_var, Operand(0), Operand(0), Operand(this->getArray())));
    list.push_back(backend::CommonInsn::create(backend::OpCode::Write, nullptr, Operand(value), g_var));
  }
}
void CallNode::emitInsn(backend::InsnList &list) {
  int int_count = 0, float_count = 0;
  std::stack<Operand> st;

  // 这个容器记录所有 pre-coloured 的 temps
  std::vector<std::shared_ptr<sym::Var>> def_vars;

  for (auto arg : this->args) {
    std::visit(
        [&](auto arg) {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (std::is_same_v<T, std::shared_ptr<sym::Var>>) {
            bool isFloat = arg->isFloatVar();
            int &idx = isFloat ? float_count : int_count;
            if (idx < 8) {
              auto parm = sym::Var::generateTemp(isFloat ? TypeFactory::FloatTy : TypeFactory::IntTy,(isFloat ? "fparm" : "parm") + std::to_string(idx));
              auto &info = parm->prop<sym::RegAllocInfo>();
              info.precoloured = true;
              info.consumed_registers = reg_to_index(isFloat ? backend::RISCV_Reg::__fa0 : backend::RISCV_Reg::__a0) + idx;
              def_vars.push_back(parm);
              list.push_back(backend::CommonInsn::create(backend::OpCode::Push, nullptr, arg, nullptr));
              idx++;
            } else {
              st.push(arg);
            }
          } else if constexpr (std::is_same_v<T, int>) {
            if (int_count < 8) {
              auto parm = sym::Var::generateTemp(TypeFactory::IntTy, "parm" + std::to_string(int_count));
              auto &info = parm->prop<sym::RegAllocInfo>();
              info.precoloured = true;
              info.consumed_registers = reg_to_index(backend::RISCV_Reg::__a0) + int_count;
              std::cout << "args " << parm->getName() << " " << parm->getName() << " " << backend::reg_to_string(backend::RISCV_Reg(info.consumed_registers)) << " " << parm << "\n";
              list.push_back(backend::CommonInsn::create(backend::OpCode::Push, nullptr, arg, nullptr));
              def_vars.push_back(parm);
              int_count++;
            } else {
              st.push(arg);
            }
          } else if constexpr (std::is_same_v<T, float>) {
            if (float_count < 8) {
              auto parm = sym::Var::generateTemp(TypeFactory::FloatTy, "fparm" + std::to_string(float_count));
              auto &info = parm->prop<sym::RegAllocInfo>();
              info.precoloured = true;
              info.consumed_registers = reg_to_index(backend::RISCV_Reg::__fa0) + float_count;
              list.push_back(backend::CommonInsn::create(backend::OpCode::Push, nullptr, arg, nullptr));
              def_vars.push_back(parm);
              float_count++;
            } else {
              st.push(arg);
            }
          }
        },
        arg);
  }
  while (!st.empty()) {
    auto arg = st.top();
    st.pop();
    list.push_back(backend::CommonInsn::create(backend::OpCode::Push, nullptr,
                                            arg, nullptr));
  }
  // TODO 把 def-var 传进来
  list.push_back(backend::LabelledInsn::create(backend::OpCode::Call,
                                               ///* defs = */ def_vars,
                                               this->function->getName()));
  if (!this->returnValueDiscarded()) {
    list.push_back((backend::CommonInsn::create(
        backend::OpCode::FuncRet, this->return_value, nullptr, nullptr)));
  }
}
void ReturnNode::emitInsn(backend::InsnList &list) {
  list.push_back(backend::CommonInsn::create(backend::OpCode::Return, nullptr,
                                             Operand(this->value), nullptr));
}
void JumpNode::emitInsn(backend::InsnList &list) {
  list.push_back(backend::LabelledInsn::create(backend::OpCode::Jump,
                                               this->label->getName()));
}
/**
 * @brief 为AssignNode的子类生成emitInsn方法的宏。
 *
 * 该宏处理从前端AssignNode到一条或多条后端指令的转换。
 *
 * @param classname 操作的名称（例如，Add, Sub）。
 * @param opcode frontend::OpCode的枚举值。
 * @param numop 操作的操作数数量。
 * @param ivariant 操作的整数版本的后端::OpCode。
 * @param fvariant 操作的浮点数版本的后端::OpCode。
 */
#define DEF_ASSIGN_NODE(classname, opcode, numop, ivariant, fvariant)        \
  void aaac::frontend::classname##AssignNode::emitInsn(                      \
      aaac::backend::InsnList &list) {                                       \
    /* 根据目标变量的类型，选择整数或浮点数版本的后端操作码 */                    \
    backend::OpCode backend_op = result->getType() == TypeFactory::FloatTy   \
                                     ? backend::OpCode::fvariant             \
                                     : backend::OpCode::ivariant;            \
                                                                             \
    /* 根据操作数个数，调用不同参数的后端指令构造函数 */                         \
    if constexpr (numop == 1) {                                              \
      list.push_back(backend::CommonInsn::create(backend_op, result,         \
                                                 operands.front(), nullptr));\
    } else if constexpr (numop == 2) {                                       \
      list.push_back(backend::CommonInsn::create(                            \
          backend_op, result, operands.front(), operands.back()));           \
    }                                                                        \
  }

/**
 * @brief 为ConditionNode的子类生成emitInsn方法的宏。
 *
 * 该宏将前端的条件节点转换为后端的 "比较+分支" 指令序列。
 * 1. 生成一条比较指令，将结果存入一个临时变量。
 * 2. 生成一条分支指令，根据临时变量的值跳转到真/假标签。
 *
 * @param classname 比较操作的名称（例如，Eq, Lt）。
 * @param opcode frontend::OpCode的枚举值。
 * @param numop 比较操作的操作数数量。
 * @param backendOp 比较操作的后端::OpCode。
 */
#define DEF_CONDITION_NODE(classname, opcode, numop, backendOp)               \
  void aaac::frontend::classname##ConditionNode::emitInsn(                    \
      aaac::backend::InsnList &list) {                                        \
    /* 步骤 1: 创建一个临时变量来保存比较的布尔结果 (0 或 1) */                   \
    auto cond_result = sym::Var::generateTemp(TypeFactory::IntTy, "cond_");   \
    /* 步骤 2: 生成后端比较指令，将结果存入 cond_result */                       \
    if constexpr (numop == 1) {                                               \
      list.push_back(backend::CommonInsn::create(backend::OpCode::backendOp,  \
                                                 cond_result,                 \
                                                 operands.front(), nullptr)); \
    } else if constexpr (numop == 2) {                                        \
      list.push_back(                                                         \
          backend::CommonInsn::create(backend::OpCode::backendOp, cond_result,\
                                      operands.front(), operands.back()));    \
    }                                                                         \
                                                                              \
    /* 步骤 3: 生成后端分支指令，它会测试 cond_result 的值并进行跳转 */           \
    list.push_back(backend::BranchInsn::create(                               \
        cond_result, this->getTrueLabel()->getName(),                         \
        this->getFalseLabel()->getName()));                                   \
  }
// 包含定义文件以实例化宏
#include "nodetype.def"
} // namespace frontend
} // namespace aaac
