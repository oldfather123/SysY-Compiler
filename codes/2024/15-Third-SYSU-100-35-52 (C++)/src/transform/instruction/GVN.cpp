#include "Function.h"
#include "Instruction.h"
#include "Value.h"
#include "domTreeAnalysis.h"
#include "GVN.h"
#include <utility>


//static bool isNoSideEffectExpr(InstructionPtr inst) {
//    if(!inst->canbeOperand())
//        return false;
//    if(inst->isTerminator())
//        return false;
//    switch(inst->insId) {
//        case InsID::Store:
//        case InsID::AtomicAdd: {
//            return false;
//        }
//        case InsID::Call: {
//            // TODO:  判断 no-side-effect call
//            //const auto callee = inst.lastOperand();
//            //auto callee = inst->getOperand(inst->getNumOperands() - 1);
//            //if(auto func = dynamic_cast<Function*>(callee)) {
//            //    auto& attr = func->attr();
//            //    return attr.hasAttr(FunctionAttribute::NoSideEffect) && attr.hasAttr(FunctionAttribute::Stateless) &&
//            //        !attr.hasAttr(FunctionAttribute::NoReturn);
//            //}
//            return false;
//        }
//        default:
//            break;
//    }
//
//    return true;
//}
//static bool isNonZero(Value* x) {
//    if(x->is<Const>() && x->as<Const>()->type->isInt()) {
//        return x->as<Const>()->intVal != 0;
//    }
//    return false;
//}
//
//static bool isMovableExpr(InstructionPtr inst, bool relaxedCtx) {
//    if(!isNoSideEffectExpr(inst))
//        return false;
//    switch(inst->insId) {
//        case InsID::Alloca:
//        case InsID::Phi:
//        case InsID::Load:
//            return false;
//        // It is not safe to speculate division, since SIGFPE may be raised.
//        //case InsID::SDiv:
//        //case InsID::UDiv:
//        //case InsID::SRem:
//        //case InsID::URem:
//            return !relaxedCtx || isNonZero(inst->getOperand(1));
//        default:
//            return true;
//    }
//}
//
//struct GlobalInstHasher final {
//    std::unordered_map<const Instruction*, size_t>& cachedHash;
//    std::function<uint32_t(Value*)>& getNumber;
//    size_t operator()(const Instruction* inst) const {
//        if(const auto iter = cachedHash.find(inst); iter != cachedHash.cend())
//            return iter->second;
//
//        size_t hashValue = std::hash<InsID>{}(inst->insId);
//        const auto operands = inst->getOperands();
//        for(auto& operand : operands)
//            hashValue = hashValue * 131 + std::hash<uint32_t>{}(getNumber(operand->val));
//        hashValue = std::hash<size_t>{}(hashValue);
//        cachedHash.emplace(inst, hashValue);
//        return hashValue;
//    }
//};
//
//struct GlobalInstEqual final {
//    std::function<uint32_t(Value*)>& getNumber;
//    bool operator()(Instruction* lhs, Instruction* rhs) const {
//        if(!lhs->isEqual(rhs))
//            return false;
//
//        auto lhsOperands = lhs->getOperands();
//        auto rhsOperands = rhs->getOperands();
//        if(lhsOperands.size() != rhsOperands.size())
//            return false;
//        return std::equal(lhsOperands.begin(), lhsOperands.end(), rhsOperands.begin(),
//                          [&](Value* lhsVal, Value* rhsVal) { return getNumber(lhsVal) == getNumber(rhsVal); });
//    }
//};
//
//bool runGVN(FunctionPtr func) {
//    //const auto& dom = analysis.get<DominateAnalysis>(func);
//    auto cfg = runCFGAnalysis(func);
//    auto dom = runDominateTreeAnalysis(func, cfg);
//    //const auto& blockFreq = analysis.get<BlockTripCountEstimation>(func);
//    //auto& target = analysis.module().getTarget();
//    bool modified = false;
//
//    uint32_t allocateID = 0;
//    std::unordered_map<Value*, uint32_t> valueNumber;
//    const auto getValueNumber = [&](Value* value) {
//        const auto iter = valueNumber.find(value);
//        if(iter != valueNumber.cend())
//            return iter->second;
//        const auto id = allocateID++;
//        valueNumber.emplace(value, id);
//        return id;
//    };
//    std::function<uint32_t(Value*)> getNumber;
//    std::unordered_map<const Instruction*, size_t> cachedHash;
//
//    GlobalInstHasher hasher{ cachedHash, getNumber };
//    GlobalInstEqual equal{ getNumber };
//    std::unordered_map<size_t, std::vector<std::pair<uint32_t, std::unordered_set<Instruction*>>>> instNumber;
//
//    const auto getInstNumber = [&](Instruction* inst) {
//        const auto hash = hasher(inst);
//        auto& val = instNumber[hash];
//
//        for(auto& [id, rhs] : val) {
//            if(rhs.count(inst))
//                return id;
//            if(equal(inst, *rhs.begin())) {
//                rhs.insert(inst);
//                return id;
//            }
//        }
//        const auto id = allocateID++;
//        val.emplace_back(id, std::unordered_set<Instruction*>{ inst });
//        return id;
//    };
//
//    getNumber = [&](Value* value) -> uint32_t {
//        const auto root = value;
//        if(root->isInstruction()) {
//            const auto inst = root->as<Instruction>();
//            if(isMovableExpr(*inst, false))
//                return getInstNumber(inst);
//        }
//
//        return getValueNumber(root);
//    };
//
//    std::unordered_map<uint32_t, std::vector<Instruction*>> instructions;
//
//    for(auto block : dom.blocks()) {
//        for(auto& inst : block->instructions()) {
//            if(!isMovableExpr(inst, false))
//                continue;
//
//            const auto id = getInstNumber(&inst);
//            instructions[id].push_back(&inst);
//            // inst->dump(std::cerr);
//            // std::cerr << "->" << id << std::endl;
//        }
//    }
//
//    std::vector<Value*> operandMap(allocateID);
//    for(auto [key, val] : valueNumber)
//        operandMap[val] = key;
//
//    for(uint32_t id = 0; id < allocateID; ++id) {
//        const auto iter = instructions.find(id);
//        if(iter == instructions.cend())
//            continue;
//
//        const auto& sameInstructions = iter->second;
//
//        if(sameInstructions.size() == 1) {
//            operandMap[id] = sameInstructions.front();
//            continue;
//        }
//
//        Block* block = nullptr;
//        for(auto inst : sameInstructions) {
//            if(block == nullptr)
//                block = inst->getBlock();
//            else {
//                block = dom.lca(block, inst->getBlock());
//            }
//        }
//
//        Instruction* replaceInst = nullptr;
//        for(auto inst : sameInstructions) {
//            if(block == inst->getBlock()) {
//                replaceInst = inst;
//                break;
//            }
//        }
//
//        // hoisting
//        if(replaceInst == nullptr) {
//            if(!blockFreq.isAvailable())
//                continue;
//            const auto hoistFreq = blockFreq.query(block);
//            double prevFreq = 0.0;
//            for(auto inst : sameInstructions)
//                prevFreq += blockFreq.query(inst->getBlock());
//            if(prevFreq < hoistFreq - 1e-6)
//                continue;
//            // Don't do hoisting for comparison instructions if targeting TAC
//            const auto base = sameInstructions.front();
//            if(!target.isNativeSupported(base->getInstID()))
//                continue;
//            // FIXME
//            if(base->getInstID() == InstructionID::Call)
//                continue;
//            if(!isMovableExpr(*base, true))
//                continue;
//
//            bool operandValid = true;
//            for(auto operand : base->operands())
//                if(operand->getBlock() && !dom.dominate(operand->getBlock(), block)) {
//                    operandValid = false;
//                    break;
//                }
//            if(!operandValid)
//                continue;
//
//            const auto inst = base->clone();
//            for(auto& operand : inst->mutableOperands()) {
//                if(operand->value->isInstruction()) {
//                    const auto operandId = getNumber(operand->value);
//                    const auto rhs = operandMap.at(operandId);
//                    if(rhs)
//                        operand->resetValue(rhs);
//                }
//            }
//            inst->insertBefore(block, std::prev(block->instructions().end()));
//            replaceInst = inst;
//        }
//
//        assert(replaceInst);
//        operandMap[id] = replaceInst;
//
//        for(auto inst : sameInstructions) {
//            if(replaceInst != inst) {
//                modified |= inst->replaceWith(replaceInst);
//            }
//        }
//    }
//
//    return modified;
//}
//