#include "../include/midend/ActiveVarAnalysis.h"
#include <map>
#include <vector>
#include "../include/frontend/IR.h"

/**
 * @file ActiveVarAnalysis.cpp
 *
 * @brief 活跃变量分析的源文件
 */
namespace sysy {

/**
 * @brief 获取指令的使用变量集合
 *
 * @param [in] inst 所要分析的指令
 * @return 使用变量集合
 */
auto ActiveVarAnalysis::getUsedSet(Instruction *inst) -> std::set<User *> {
  using Kind = Instruction::Kind;
  std::vector<User *> operands;
  for (const auto &operand : inst->getOperands()) {
    operands.emplace_back(dynamic_cast<User *>(operand->getValue()));
  }
  std::set<User *> result;
  switch (inst->getKind()) {
    // phi op
    case Kind::kPhi:
    case Kind::kCall:
      result.insert(std::next(operands.begin()), operands.end());
      break;
    case Kind::kCondBr:
      result.insert(operands[0]);
      break;
    case Kind::kBr:
    case Kind::kAlloca:  // 维度应是常量表达式
      break;
    // mem op
    case Kind::kStore:
      result.insert(operands[0]);
      result.insert(operands.begin() + 2, operands.end());
      break;
    case Kind::kLoad:
    case Kind::kLa: {
      auto variable = dynamic_cast<AllocaInst *>(operands[0]);
      auto global = dynamic_cast<GlobalValue *>(operands[0]);
      auto constArray = dynamic_cast<ConstantVariable *>(operands[0]);
      if ((variable != nullptr && variable->getNumDims() == 0) || (global != nullptr && global->getNumDims() == 0) ||
          (constArray != nullptr && constArray->getNumDims() == 0)) {
        result.insert(operands[0]);
      }
      result.insert(std::next(operands.begin()), operands.end());
      break;
    }
    case Kind::kGetSubArray: {
      for (unsigned i = 2; i < operands.size(); i++) {
        result.insert(operands[i]);
      }
      break;
    }
    case Kind::kMemset: {
      result.insert(std::next(operands.begin()), operands.end());
      break;
    }
    case Kind::kInvalid:
    // Binary
    case Kind::kAdd:
    case Kind::kSub:
    case Kind::kMul:
    case Kind::kDiv:
    case Kind::kRem:
    case Kind::kICmpEQ:
    case Kind::kICmpNE:
    case Kind::kICmpLT:
    case Kind::kICmpLE:
    case Kind::kICmpGT:
    case Kind::kICmpGE:
    case Kind::kFAdd:
    case Kind::kFSub:
    case Kind::kFMul:
    case Kind::kFDiv:
    case Kind::kFCmpEQ:
    case Kind::kFCmpNE:
    case Kind::kFCmpLT:
    case Kind::kFCmpLE:
    case Kind::kFCmpGT:
    case Kind::kFCmpGE:
    case Kind::kAnd:
    case Kind::kOr:
    // Unary
    case Kind::kNeg:
    case Kind::kNot:
    case Kind::kFNot:
    case Kind::kFNeg:
    case Kind::kFtoI:
    case Kind::kItoF:
    // terminator
    case Kind::kReturn:
      result.insert(operands.begin(), operands.end());
      break;
    default:
      assert(false);
      break;
  }
  result.erase(nullptr);
  return result;
}
/**
 * @brief 获取指令所定义的变量
 *
 * @param [in] inst 所要分析的指令
 * @return 所定义的变量
 */
auto ActiveVarAnalysis::getDefine(Instruction *inst) -> User * {
  User *result = nullptr;
  ///< @note 暂时不考虑数组
  if (inst->isStore()) {
    auto store = dynamic_cast<StoreInst *>(inst);
    auto operand = store->getPointer();
    auto variable = dynamic_cast<AllocaInst *>(operand);
    auto global = dynamic_cast<GlobalValue *>(operand);
    if ((variable != nullptr && variable->getNumDims() != 0) || (global != nullptr && global->getNumDims() != 0)) {
      result = nullptr;
    } else {
      result = dynamic_cast<User *>(operand);
    }
  } else if (inst->isPhi()) {
    result = dynamic_cast<User *>(inst->getOperand(0));
  } else if (inst->isBinary() || inst->isUnary() || inst->getKind() == Instruction::Kind::kCall ||
             inst->getKind() == Instruction::Kind::kLoad || inst->isLa()) {
    result = inst;
  }
  return result;
}
/**
 * @brief 初始化
 *
 * @param [in] pModule 所要分析的模块
 * @return 无返回值
 */
void ActiveVarAnalysis::init(Module *pModule) {
  for (const auto &function : pModule->getFunctions()) {
    for (const auto &block : function.second->getBasicBlocks()) {
      activeTable.emplace(block.get(), std::vector<std::set<User *>>{});
      for (unsigned i = 0; i < block->getNumInstructions() + 1; i++) {
        activeTable.at(block.get()).emplace_back();
      }
    }
  }
}
/**
 * @brief 基本块内的活跃变量分析
 *
 * @param [in] pModule 所要分析的模块
 * @param [in] block 所要分析的基本块
 * @return 活跃变量信息是否被更新
 */
auto ActiveVarAnalysis::analyze(Module *pModule, BasicBlock *block) -> bool {
  bool changed = false;
  std::set<User *> activeSet{};

  for (const auto &succ : block->getSuccessors()) {
    auto succActiveSet = activeTable.at(succ).front();
    activeSet.merge(succActiveSet);
  }

  const auto &instructions = block->getInstructions();
  const auto numInstructions = instructions.size();
  const auto &oldEndActiveSet = activeTable.at(block)[numInstructions];
  if (!std::equal(activeSet.begin(), activeSet.end(), oldEndActiveSet.begin(), oldEndActiveSet.end())) {
    changed = true;
    activeTable.at(block)[numInstructions] = activeSet;
  }

  auto instructionIter = instructions.end();
  instructionIter--;
  for (unsigned i = numInstructions; i > 0; i--) {
    auto inst = instructionIter->get();
    auto used = getUsedSet(inst);
    User *defined = getDefine(inst);

    activeSet.erase(defined);
    activeSet.merge(used);
    const auto &oldActiveSet = activeTable.at(block)[i - 1];
    if (!std::equal(activeSet.begin(), activeSet.end(), oldActiveSet.begin(), oldActiveSet.end())) {
      changed = true;
      activeTable.at(block)[i - 1] = activeSet;
    }
    instructionIter--;
  }

  return changed;
}
/**
 * @brief 获取活跃变量信息表
 *
 * @return 活跃变量信息表
 */
auto ActiveVarAnalysis::getActiveTable() const -> const std::map<BasicBlock *, std::vector<std::set<User *>>> & {
  return activeTable;
}

/**
 * @brief 清空内部变量
 *
 * @return 无返回值
 */
void ActiveVarAnalysis::clear() { activeTable.clear(); }
}  // namespace sysy
