#include "../../include/ir/loop_unroll.h"
#include "../../include/ir/cfg.h"
#include "../../include/ir/scev.h"
#include "../../include/ir/cfg_simplify.h"
#include "../../include/ir/dce.h"
#include "../../include/ir/merge_bb.h"
#include "../../debug.h"
#include <iostream>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>

namespace llvm_ir {
namespace loop_unroll {

// 静态成员初始化
int LoopUnroller::blockCounter = 0;

// --- LoopUnroller分析函数实现 ---

bool LoopUnrollPass::isSimpleLoop(Loop* loop, LoopBoundInfo info) {
    // 检查循环是否为简单循环（只有一个latch，一个退出点等）
    if (loop->getLatches().size() != 1) {
        std::cout << "[LoopUnroll][Reject] Loop has " << loop->getLatches().size() << " latches, not simple" << std::endl;
        return false;
    }
    if (loop->getExitBlocks().size() != 1) {
        std::cout << "[LoopUnroll][Reject] Loop has " << loop->getExitBlocks().size() << " exit blocks, not simple" << std::endl;
        return false;
    }
    if (loop->getExitingNodes().size() != 1) {
        std::cout << "[LoopUnroll][Reject] Loop has " << loop->getExitingNodes().size() << " exiting nodes, not simple" << std::endl;
        return false;
    }
    if (loop->getPreheader() == nullptr) {
        std::cout << "[LoopUnroll][Reject] Loop has no preheader, not simple" << std::endl;
        return false;
    }

    size_t blockCount = loop->getBlocks().size();
    if (blockCount > LOOP_UNROLL_BLOCK_LIMIT) {
        std::cout << "[LoopUnroll][Reject] Loop has " << blockCount << " blocks, too large" << std::endl;
        return false;
    }

    // 计算循环中的指令数量
    int instNumber = 0;
    for (auto bb : loop->getBlocks()) {
        instNumber += bb->instructions.size();
        instNumber += bb->phi_instructions.size();
    }
    
    if (instNumber > 512 && info.tripCount != 1) {
        std::cout << "[LoopUnroll][Reject] Too many instructions in loop: " << (instNumber) << std::endl;
        return false;
    }

    // 不依赖tripCount是否为常量
    std::cout << "[LoopUnroll][Accept] Loop is simple (structure-only)" << std::endl;
    return true;
}

// 只有在常量loop时才进行判断
bool LoopUnrollPass::canFullyUnrollLoop(Loop* loop, LoopBoundInfo info) {
    int iterations = info.tripCount;
    if (iterations <= 0) {
        std::cout << "[LoopUnroll][Reject] Invalid iterations: " << iterations << std::endl;
        return false;
    }

    // 计算循环中的指令数量
    int instNumber = 0;
    for (auto bb : loop->getBlocks()) {
        instNumber += bb->instructions.size();
        instNumber += bb->phi_instructions.size();
    }
    
    // 检查展开后的指令数量
    if (iterations * instNumber > 256) {
        std::cout << "[LoopUnroll][Reject] Too many instructions after unrolling: " << (iterations * instNumber) << std::endl;
        return false;
    }
    
    // 检查函数总指令数量
    Function* F = loop->getHeader()->parent;
    int allInstNumber = 0;
    for (auto& bb : F->basicBlocks) {
        allInstNumber += bb->instructions.size();
        allInstNumber += bb->phi_instructions.size();
    }
    
    if (allInstNumber >= 4096 && info.tripCount != 1) {
        std::cout << "[LoopUnroll][Reject] Function already has too many instructions: " << allInstNumber << std::endl;
        return false;
    }

    if (loop->getBlocks().size() > 4) {
        std::cout << "[LoopUnroll][Reject] Loop has too many blocks: " << loop->getBlocks().size() << std::endl;
        return false;
    }

    std::cout << "[LoopUnroll][Accept] Can unroll loop with " << iterations << " iterations" << std::endl;
    return true;
}

bool LoopUnroller::isLoopEnd(int i, int ub, ICmpCond cond) {
    assert(cond != ICmpCond::EQ && cond != ICmpCond::NE);
    
    switch (cond) {
        case ICmpCond::SLE:
            return i > ub;
        case ICmpCond::SGE:
            return i < ub;
        case ICmpCond::SGT:
            return i <= ub;
        case ICmpCond::SLT:
            return i >= ub;
        default:
            assert(false && "Invalid condition");
            return true;
    }
}

// --- LoopUnroller实现 ---
int lablecounter = 0;
inline std::string LoopUnroller::generateUniqueLabel(const std::string& base) {
    // std::set<std::string> existingLabels;
    // for (const auto& bb : function.basicBlocks) {
    //     existingLabels.insert(bb->label);
    // }
    
    // std::string candidate = base;
    // int idx = 1;
    // while (existingLabels.count(candidate)) {
    //     candidate = base + "_" + std::to_string(idx++);
    // }
    return base + "_" + std::to_string(lablecounter++);
}

BasicBlock* LoopUnroller::createNewBlock(const std::string& label) {
    auto newBlock = std::make_unique<BasicBlock>(label, &function);
    BasicBlock* newBlockPtr = newBlock.get();
    function.insertBasicBlockAt(std::move(newBlock), insertionIndex);
    ++insertionIndex;
    return newBlockPtr;
}

// 生成唯一的变量名称
int value_name_counter = 0;
inline static std::string getUniqueValueName(const std::string& base, const Function& F) {
    // std::set<std::string> value_names;
    // for (const auto& bb_ptr : F.basicBlocks) {
    //     // 收集PHI指令名称
    //     for (const auto& phi_ptr : bb_ptr->phi_instructions) {
    //         value_names.insert(phi_ptr->name);
    //     }
    //     // 收集普通指令名称
    //     for (const auto& inst_ptr : bb_ptr->instructions) {
    //         if (!inst_ptr->name.empty()) {
    //             value_names.insert(inst_ptr->name);
    //         }
    //     }
    // }
    // std::string candidate = base;
    // int idx = 1;
    // while (value_names.count(candidate)) {
    //     candidate = base + "_" + std::to_string(idx++);
    // }
    return base + "_" + std::to_string(value_name_counter++);
}

std::unique_ptr<Instruction> LoopUnroller::copyInstruction(Instruction* inst, std::string newName) {
    // 使用已有的cloneInstruction函数
    std::map<Value*, Value*> valueMap; // 空的映射，因为我们不替换值
    
    // 为指令生成唯一的名称
    std::string uniqueName = getUniqueValueName(newName, function);
    
    // 使用cloneInstruction函数
    auto clonedInst = cloneInstruction(inst, valueMap, uniqueName);
    
    if (clonedInst) {
        // 设置唯一的名称
        clonedInst->name = uniqueName;
    }
    
    return clonedInst;
}

BasicBlock* LoopUnroller::copyBasicBlock(BasicBlock* original, const std::string& newLabel, int iteration) {
    BasicBlock* newBlock = createNewBlock(newLabel); // 在这里将创建的basicblock添加到function中了
    
    // 复制PHI指令
    for (auto& phi : original->phi_instructions) {
        std::string newName = phi->name;
        if (newName.find("unroll") != std::string::npos) {
            size_t last_underscore = newName.rfind('_');
            if (last_underscore != std::string::npos) {
                newName.resize(last_underscore);
            }
            newName = newName + "_" + std::to_string(abs(iteration));
        } else {
            newName = newName + "_unroll_" + std::to_string(abs(iteration));
        }
        
        
        std::string namePrefix = "unroll_" + std::to_string(blockCounter++);
        auto newPhi = clonePhi(phi.get(), newName); // 克隆不会添加incoming values
        
        if (newPhi) {
            // 设置唯一的名称
            std::string uniqueName = getUniqueValueName(newName, function);
            newPhi->name = uniqueName;
            
            // 复制incoming values
            for (auto& incoming : phi->incoming_values) {
                newPhi->addIncoming(incoming.first, incoming.second);
            }
            
            newBlock->addPhi(std::move(newPhi));
        }
    }
    
    // 复制普通指令
    for (auto& inst : original->instructions) {
        std::string newName = inst->name;
        if (newName.find("unroll") != std::string::npos) {
            size_t last_underscore = newName.rfind('_');
            if (last_underscore != std::string::npos) {
                newName.resize(last_underscore);
            }
            newName = newName + "_" + std::to_string(abs(iteration));
        } else {
            newName = newName + "_unroll_" + std::to_string(abs(iteration));
        }

        auto newInst = copyInstruction(inst.get(), newName);
        if (newInst) {
            newBlock->addInstruction(std::move(newInst));
        }
    }
    
    return newBlock;
}

int LoopUnroller::guardCounter = 0;

// ===== 判断某个值是否是在循环内定义的指令 =====
static bool isInstructionInLoop(Value* v, Loop* loop) {
    if (!v) return false;
    if (auto* inst = dynamic_cast<Instruction*>(v)) {
        BasicBlock* bb = inst->parent;
        return bb && loop->contains(bb);
    }
    return false;
}

// ===== 递归检测：表达式是否包含 target 变量（限Add Sub Mul） =====
static bool containsPrimaryVar(Value* expr, Value* target, int depth = 0) {
    if (!expr) return false;
    if (expr == target) return true;
    if (depth > 16) {
        std::cerr << "[LoopUnroll][Warning] containsPrimaryVar: depth limit exceeded" << std::endl;
        return false; // 深度上限
    }

    if (auto* bop = dynamic_cast<BinaryOperator*>(expr)) {
        switch (bop->opcode) {
            case Opcode::Add:
            case Opcode::Sub:
            case Opcode::Mul:
                return containsPrimaryVar(bop->operands[0], target, depth + 1) ||
                       containsPrimaryVar(bop->operands[1], target, depth + 1);
            default:
                return false;
        }
    }
    // 可在此允许少量零代价 cast，如有
    return false;
}

// ===== 在remainder上下文安全克隆纯表达式（仅限常量与Add/Sub/Mul链） =====
// 说明：
//  - 优先使用 regReplaceMap 中已有的映射（即循环内定义的值）
//  - 若是常量，直接返回
//  - 若是 BinaryOperator 且为 Add/Sub/Mul，且其操作数可递归克隆，则在 insertBB 按 insertIndex 之前插入
//  - 其他情况返回 nullptr（不安全或不支持）
static Value* clonePureExprForRemainder(Value* v,
                                        BasicBlock* insertBB,
                                        std::map<Value*, Value*>& regReplaceMap,
                                        Loop* loop,
                                        Function& function,
                                        size_t& insertIndex) {
    if (!v) return nullptr;

    // 循环内定义：应当已经在 regReplaceMap 中
    auto it = regReplaceMap.find(v);
    if (it != regReplaceMap.end()) return it->second;

    // 如果不是指令（如函数参数、立即数、全局符号引用等），可直接使用
    if (!dynamic_cast<Instruction*>(v)) {
        return v;
    }

    // 若是指令，但定义在循环外，直接复用（不克隆），通常来自preheader/入口，已支配remainder header
    if (auto* inst = dynamic_cast<Instruction*>(v)) {
        if (!loop->contains(inst->parent)) {
            return v;
        }
    }

    // 常量：直接使用（虽然常量不是指令，这里冗余保底）
    if (dynamic_cast<ConstantInt*>(v)) {
        return v;
    }

    // 仅允许克隆 Add/Sub/Mul 的纯表达式（且在循环内）
    if (auto* bop = dynamic_cast<BinaryOperator*>(v)) {
        switch (bop->opcode) {
            case Opcode::Add:
            case Opcode::Sub:
            case Opcode::Mul:
                break;
            default:
                std::cerr << "[LoopUnroll][Remainder] Unsafe opcode in bound expr: " << (int)bop->opcode << std::endl;
                return nullptr;
        }
        // 递归克隆操作数
        if (bop->operands.size() != 2) return nullptr;
        Value* lhs = clonePureExprForRemainder(bop->operands[0], insertBB, regReplaceMap, loop, function, insertIndex);
        Value* rhs = clonePureExprForRemainder(bop->operands[1], insertBB, regReplaceMap, loop, function, insertIndex);
        if (!lhs || !rhs) return nullptr;
        // 在 insertBB 中 icmp 之前插入，保持def-use顺序：按insertIndex递增
        std::string newName = getUniqueValueName(bop->name.empty() ? "%rem_bound" : bop->name, function);
        auto cloned = std::make_unique<BinaryOperator>(bop->opcode, lhs, rhs, newName);
        Value* out = cloned.get();
        insertBB->insertInstructionAt(std::move(cloned), insertIndex);
        ++insertIndex;
        return out;
    }

    // 其他类型（如Load/GEP/PHI/Call等在循环内）拒绝克隆
    std::cerr << "[LoopUnroll][Remainder] Cannot materialize non-pure bound expr safely (opcode)" << std::endl;
    return nullptr;
}

bool LoopUnroller::generateGuardLogic(Loop* loop, Value* primaryVar, const SCEVLoopVariableInfo& primaryVarInfo, LoopBoundInfo info) {
    std::cout << "[LoopUnroll] Adding guard logic to preheader using primaryVar: " << (primaryVar ? primaryVar->name : "<null>") << std::endl;
    // 获取循环的关键块
    auto exit = *loop->getExitBlocks().begin();
    auto exiting = *loop->getExitingNodes().begin();

    BasicBlock* oldHeader = loop->getHeader();
    BasicBlock* oldExiting = exiting;
    BasicBlock* oldPreheader = loop->getPreheader();
    
    // 构造guard icmp：复用header原始icmp形状，将变量侧替换为initialValue
    std::string guardName = generateUniqueLabel("%guard" + std::to_string(guardCounter++));
    
    ICmpInst* headerCmp = nullptr;
    if (auto* term = dynamic_cast<BranchInst*>(oldHeader->getTerminator())) {
        if (term->isConditional()) {
            headerCmp = dynamic_cast<ICmpInst*>(term->operands[0]);
        }
    }

    if (!primaryVarInfo.initialValue) {
        std::cerr << "[LoopUnroll] Error: initialValue is null, cannot build guard" << std::endl;
        return false;
    }

    // 统一的边界加载函数：根据bound候选在preheader构造可用的值
    auto materializeBoundInPreheader = [&](Value* boundCand) -> Value* {
        if (!boundCand) return nullptr;
        // 若候选本身是Load结果，尝试克隆
        if (auto* load = dynamic_cast<LoadInst*>(boundCand)) {
            Value* ptr = load->getPointerOperand();
            std::string lname = getUniqueValueName("%guard_bound", function);
            auto newLoad = std::make_unique<LoadInst>(ptr, lname, load->type);
            Value* bv = newLoad.get();
            size_t pos = oldPreheader->instructions.size() ? (oldPreheader->instructions.size() - 1) : 0;
            oldPreheader->insertInstructionAt(std::move(newLoad), pos);
            return bv;
        }
        // 若候选是全局或GEP基于全局，插入load
        if (auto* gv = dynamic_cast<GlobalVariable*>(boundCand)) {
            std::string lname = getUniqueValueName("%guard_bound", function);
            auto newLoad = std::make_unique<LoadInst>(gv, lname, Type::I32);
            Value* bv = newLoad.get();
            size_t pos = oldPreheader->instructions.size() ? (oldPreheader->instructions.size() - 1) : 0;
            oldPreheader->insertInstructionAt(std::move(newLoad), pos);
            return bv;
        }
        if (auto* gep = dynamic_cast<GetElementPtrInst*>(boundCand)) {
            // 仅当base为全局时，复制GEP并随后load
            Value* base = gep->operands[0];
            if (dynamic_cast<GlobalVariable*>(base)) {
                // 简单克隆GEP
                auto newGEP = std::make_unique<GetElementPtrInst>(gep->operands[0], gep->operands[1], getUniqueValueName("%guard_gep", function));
                Value* gepVal = newGEP.get();
                size_t pos = oldPreheader->instructions.size() ? (oldPreheader->instructions.size() - 1) : 0;
                oldPreheader->insertInstructionAt(std::move(newGEP), pos);
                std::string lname = getUniqueValueName("%guard_bound", function);
                auto newLoad = std::make_unique<LoadInst>(gepVal, lname, Type::I32);
                Value* bv = newLoad.get();
                oldPreheader->insertInstructionAt(std::move(newLoad), pos);
                return bv;
            }
        }
        // 其他情况：若是常量或预先在preheader/外部定义的值，可直接使用
        if (auto* inst = dynamic_cast<Instruction*>(boundCand)) {
            if (loop->contains(inst)) {
                // 不能安全使用循环内定义的复杂表达式
                return nullptr;
            }
        }
        return boundCand;
    };

    // 选择边界候选
    Value* preferBound = primaryVarInfo.isIncrement ? primaryVarInfo.upperBound : (primaryVarInfo.lowerBound ? primaryVarInfo.lowerBound : primaryVarInfo.upperBound);
    Value* guardBound = materializeBoundInPreheader(preferBound);

    std::unique_ptr<ICmpInst> guardCmp;
    bool emitted = false;
    if (headerCmp) {
        // 识别变量侧（允许更深层表达式）
        Value* lhs = headerCmp->operands[0];
        Value* rhs = headerCmp->operands[1];
        bool varOnLHS = containsPrimaryVar(lhs, primaryVar);
        bool varOnRHS = containsPrimaryVar(rhs, primaryVar);
        if (guardBound) {
            if (varOnLHS && !varOnRHS) {
                guardCmp = std::make_unique<ICmpInst>(headerCmp->condition, primaryVarInfo.initialValue, guardBound, guardName);
                emitted = true;
            } else if (varOnRHS && !varOnLHS) {
                guardCmp = std::make_unique<ICmpInst>(headerCmp->condition, guardBound, primaryVarInfo.initialValue, guardName);
                emitted = true;
            }
        }
    }

    if (!emitted) {
        // 兜底：使用info.condition与( init, guardBound )
        if (!guardBound) {
            std::cerr << "[LoopUnroll] Warning: cannot materialize guard bound; skip guard generation" << std::endl;
            return false;
        }
        guardCmp = std::make_unique<ICmpInst>(info.condition, primaryVarInfo.initialValue, guardBound, guardName);
    }

    auto guardBr = std::make_unique<BranchInst>(guardCmp.get(), oldHeader->label, exit->label);
    std::cout << "[LoopUnroll] guardBr: " << guardBr->toString() << std::endl;
    std::cout << "[LoopUnroll] guardCmp: " << guardCmp->toString() << std::endl;
    std::cout << "[LoopUnroll] oldPreheader: " << oldPreheader->getTerminator()->toString() << std::endl;
    oldPreheader->getTerminator()->parent = oldPreheader; // TODO
    oldPreheader->getTerminator()->removeFromParent();
    oldPreheader->addInstruction(std::move(guardCmp));
    oldPreheader->addInstruction(std::move(guardBr));
    
    // 更新exit block的PHI指令，添加来自preheader的incoming
    for (auto& exitPhi : exit->phi_instructions) {
        // 找到循环变量在preheader中的值
        Value* oldHeaderValue = nullptr;
        for (auto& incoming : exitPhi->incoming_values) {
            if (incoming.second == oldHeader) {
                oldHeaderValue = incoming.first;
                break;
            }
        }
        // 通过oldHeaderValue找到preheaderValue
        Value* preheaderValue = nullptr;
        for (auto& headerPhi : oldHeader->phi_instructions) {
            if (headerPhi.get() == oldHeaderValue) {
                for (auto& incoming : headerPhi->incoming_values) {
                    if (incoming.second == oldPreheader) {
                        preheaderValue = incoming.first;
                        break;
                    }
                }
                if (preheaderValue) break;
            }
        }
        
        if (preheaderValue) {
            // 添加来自preheader的incoming（当循环条件不满足时）
            exitPhi->addIncoming(preheaderValue, oldPreheader);
            std::cout << "[LoopUnroll] updated exit phi with preheader incoming: " << exitPhi->toString() << std::endl;
        }
    }

    std::cout << "[LoopUnroll] Guard added: branch %preheader -> (" << oldHeader->label << ", " << exit->label << ")" << std::endl;
    return true;
}

bool LoopUnroller::partiallyUnrollLoop(Loop* loop, Value* primaryVar, const SCEVLoopVariableInfo& primaryVarInfo, LoopBoundInfo info, bool fullyUnrollMode) {
    std::cout << "[LoopUnroll] Partially unrolling loop with " << info.tripCount << " iterations" << std::endl;
    
    int totalIterations = info.tripCount;                       std::cout << "totalIterations: " << totalIterations << std::endl;
    bool hasConstTrip = (totalIterations > 0);
    // fullyUnrollMode: 直接将unrollFactor提升为tripCount，实现完全展开
    int effectiveUnrollFactor = unrollFactor;
    if (fullyUnrollMode) {
        if (!hasConstTrip || totalIterations <= 0) {
            std::cerr << "[LoopUnroll][Reject] Cannot fully unroll non-constant or non-positive tripCount" << std::endl;
            return false;
        }
        effectiveUnrollFactor = totalIterations;
        std::cout << "[LoopUnroll] Fully-unroll mode: effectiveUnrollFactor=" << effectiveUnrollFactor << std::endl;
    }
    int remainderIterations = hasConstTrip ? (totalIterations % effectiveUnrollFactor) : -1;   std::cout << "remainderIterations: " << (hasConstTrip ? std::to_string(remainderIterations) : "unknown") << std::endl;
    int mainIterations = hasConstTrip ? (totalIterations - remainderIterations) : 0; std::cout << "mainIterations: " << (hasConstTrip ? std::to_string(mainIterations) : "unknown") << std::endl;

    if (fullyUnrollMode) effectiveUnrollFactor += 1; // 多创建一个循环，用于收尾

    // 获取循环的关键块
    auto exit = *loop->getExitBlocks().begin();
    auto exiting = *loop->getExitingNodes().begin();

    // 记录循环的所有块
    std::vector<BasicBlock*> allLoopBlocks;
    
    // 保存原始循环信息
    BasicBlock* originalHeader = loop->getHeader();
    BasicBlock* oldHeader = loop->getHeader();
    BasicBlock* oldExiting = exiting;
    BasicBlock* oldLatch = *loop->getLatches().begin();
    BasicBlock* oldPreheader = loop->getPreheader();
    std::set<BasicBlock*> oldLoopNodes = loop->getBlocks();

    allLoopBlocks.push_back(oldPreheader);
    for (auto bb : oldLoopNodes) allLoopBlocks.push_back(bb);

    // ===== 预检查：仅当需要余数循环时，确保能在余数头部安全地重建/映射上界 =====
    if (!hasConstTrip || remainderIterations > 0) {
        do {
            BranchInst* hdrBr = dynamic_cast<BranchInst*>(originalHeader->getTerminator());
            if (!hdrBr || !hdrBr->isConditional()) break;
            auto* oldIcmp = dynamic_cast<ICmpInst*>(hdrBr->operands[0]);
            if (!oldIcmp) break;
            Value* lhsOld = oldIcmp->operands[0];
            Value* rhsOld = oldIcmp->operands[1];
            bool varOnLHSOld = containsPrimaryVar(lhsOld, primaryVar);
            bool varOnRHSOld = containsPrimaryVar(rhsOld, primaryVar);
            Value* oldBound = nullptr;
            if (varOnLHSOld && !varOnRHSOld) oldBound = rhsOld; else if (varOnRHSOld && !varOnLHSOld) oldBound = lhsOld; else oldBound = rhsOld;

            // 若上界在循环内定义，则后续复制会给出映射 -> OK
            if (isInstructionInLoop(oldBound, loop)) break;

            // 若是常量或由纯Add/Sub/Mul常量链组成（不在循环内定义），则可安全克隆 -> OK
            std::function<bool(Value*)> canClone;
            canClone = [&](Value* v)->bool{
                if (!v) return false;
                // 非指令（函数参数、全局符号引用、常量等）可直接复用
                if (!dynamic_cast<Instruction*>(v)) return true;
                // 指令但在循环外：可直接复用
                if (auto* inst = dynamic_cast<Instruction*>(v)) {
                    if (!loop->contains(inst->parent)) return true;
                }
                if (dynamic_cast<ConstantInt*>(v)) return true;
                if (isInstructionInLoop(v, loop)) return true; // 将由复制映射
                if (auto* bop = dynamic_cast<BinaryOperator*>(v)) {
                    switch (bop->opcode) {
                        case Opcode::Add:
                        case Opcode::Sub:
                        case Opcode::Mul:
                            return canClone(bop->operands[0]) && canClone(bop->operands[1]);
                        default:
                            return false;
                    }
                }
                // 其他（Load/GEP/PHI/Call等在循环内）不允许
                return false;
            };
            if (!canClone(oldBound)) {
                std::cerr << "[LoopUnroll][Reject] Unsafe or non-materializable bound for remainder; skip unrolling." << std::endl;
                return false;
            }
        } while(false);
    } else {
        std::cout << "[LoopUnroll] No remainder required; skip remainder precheck" << std::endl;
    }

    // 计算原循环块的有序序列（按Function.basicBlocks顺序）以及插入起点（原循环之后）
    {
        size_t insertionPos = 0;
        for (size_t i = 0; i < function.basicBlocks.size(); ++i) {
            BasicBlock* b = function.basicBlocks[i].get();
            if (oldLoopNodes.count(b)) {
                if (i + 1 > insertionPos) insertionPos = i + 1;
            }
        }
        // if (fullyUnrollMode && insertionPos > 1) insertionPos -= 1; //TODO
        setInsertionIndex(insertionPos);
    }

    // 在preheader中添加守卫逻辑，如果不满足循环条件，直接跳转到exit block //TODO 似乎不需要guard，因为循环头自己会判断，全展开模式应该是要的？
    // if (loop->getParent() && primaryVar != nullptr){
    //     if (!generateGuardLogic(loop, primaryVar, primaryVarInfo, info)) {
    //         std::cerr << "[LoopUnroll] Warning: cannot generate guard logic; skip unrolling" << std::endl;
    //         return false;
    //     }
    // }

    // 调整主循环header的退出比较，使其确保剩余迭代数>=effectiveUnrollFactor
    if (!fullyUnrollMode && primaryVar != nullptr) {
        int k = effectiveUnrollFactor - 1;
        int stepAbs = std::abs(info.step);
        int offsetVal = k * stepAbs;
        if (offsetVal != 0) {
            if (auto* term = dynamic_cast<BranchInst*>(originalHeader->getTerminator())) {
                if (term->isConditional()) {
                    if (auto* icmp = dynamic_cast<ICmpInst*>(term->operands[0])) {
                        auto* offsetConst = new ConstantInt(offsetVal);
                        std::string adjName = getUniqueValueName(primaryVar->name + "_adj", function);
                        std::unique_ptr<Instruction> adjInst;
                        if (primaryVarInfo.isIncrement) {
                            adjInst = std::make_unique<BinaryOperator>(Opcode::Add, primaryVar, offsetConst, adjName);
                        } else {
                            adjInst = std::make_unique<BinaryOperator>(Opcode::Sub, primaryVar, offsetConst, adjName);
                        }
                        Instruction* adjPtr = adjInst.get();
                        size_t insertPos = originalHeader->instructions.size() ? (originalHeader->instructions.size() - 1) : 0;
                        originalHeader->insertInstructionAt(std::move(adjInst), insertPos-1);
                        if (icmp->operands[0] == primaryVar) {
                            icmp->replaceOperand(icmp->operands[0], adjPtr);
                        } else if (icmp->operands[1] == primaryVar) {
                            icmp->replaceOperand(icmp->operands[1], adjPtr);
                        }
                        std::cout << "[LoopUnroll] Adjusted header compare to use " << (primaryVarInfo.isIncrement ? "+" : "-")
                                  << " (" << offsetVal << ") on primaryVar" << std::endl;
                    }
                }
            }
        }
    }
    
    // 展开循环
    int iter = info.initvalue;
    for (int i = 1; i < effectiveUnrollFactor; ++i) {
        std::cout << "\n[LoopUnroll] =========================================" << std::endl;
        std::cout << "[LoopUnroll] iter: " << iter << std::endl;
        std::cout << "[LoopUnroll] =========================================\n" << std::endl;
        std::map<Value*, Value*> regReplaceMap;
        std::map<BasicBlock*, BasicBlock*> labelReplaceMap;
        
        BasicBlock* newHeader = nullptr;
        BasicBlock* newExiting = nullptr;
        BasicBlock* newLatch = nullptr;
        std::set<BasicBlock*> newLoopNodes;
        
        // 按函数顺序收集当前循环块
        std::vector<BasicBlock*> currLoopOrder;
        for (size_t bi = 0; bi < function.basicBlocks.size(); ++bi) {
            BasicBlock* b = function.basicBlocks[bi].get();
            if (oldLoopNodes.count(b)) currLoopOrder.push_back(b);
        }
        
        // 复制所有循环块（按原函数顺序），建立label映射
        std::cout << "[LoopUnroll] copy loop blocks: " << currLoopOrder.size() << std::endl;
        for (auto bb : currLoopOrder) {
            std::string newLabel;
            if (bb == oldHeader) {
                newLabel = generateUniqueLabel("header_unroll_" + std::to_string(abs(iter)));
            }
            else if (bb == oldExiting) {
                newLabel = generateUniqueLabel("exiting_unroll_" + std::to_string(abs(iter)));
            }
            else if (bb == oldLatch) {
                newLabel = generateUniqueLabel("latch_unroll_" + std::to_string(abs(iter)));
            } else {
                newLabel = generateUniqueLabel("body_unroll_" + std::to_string(abs(iter)));
            }
            BasicBlock* newbb = copyBasicBlock(bb, newLabel, iter);
            newLoopNodes.insert(newbb);
            labelReplaceMap[bb] = newbb;
            allLoopBlocks.push_back(newbb);
            
            if (bb == oldHeader) {
                newHeader = newbb;
                std::cout << "[LoopUnroll] newHeader: " << newHeader->label << std::endl;
            }
            if (bb == oldExiting) {
                newExiting = newbb;
                std::cout << "[LoopUnroll] newExiting: " << newExiting->label << std::endl;
            }
            if (bb == oldLatch) {
                newLatch = newbb;
                std::cout << "[LoopUnroll] newLatch: " << newLatch->label << std::endl;
            }
        }
        std::cout << "[LoopUnroll] copy loop blocks done" << std::endl;
        
        // 建立值映射关系（保持函数顺序）
        for (auto bb : currLoopOrder) {
            BasicBlock* newbb = labelReplaceMap[bb];
            
            // 映射PHI指令
            for (size_t j = 0; j < bb->phi_instructions.size() && j < newbb->phi_instructions.size(); ++j) {
                regReplaceMap[bb->phi_instructions[j].get()] = newbb->phi_instructions[j].get();
            }
            
            // 映射普通指令
            auto oldIt = bb->instructions.begin();
            auto newIt = newbb->instructions.begin();
            while (oldIt != bb->instructions.end() && newIt != newbb->instructions.end()) {
                // if (!(*oldIt)->name.empty()) { // 不要依赖名称是否为空！！！！！！
                //     regReplaceMap[oldIt->get()] = newIt->get();
                // }
                regReplaceMap[oldIt->get()] = newIt->get();
                ++oldIt;
                ++newIt;
            }
        }
        
        // 更新新块中的所有引用
        for (auto bb : newLoopNodes) {
            // 更新PHI指令的操作数和来源
            for (auto& phi : bb->phi_instructions) {
                std::cout << "[LoopUnroll] updating phi: " << phi->toString() << std::endl;
                for (auto& incoming : phi->incoming_values) {
                    // 更新值引用
                    auto regIt = regReplaceMap.find(incoming.first);
                    if (regIt != regReplaceMap.end()) {
                        std::cout << "[LoopUnroll]   incoming value: " << incoming.first->name << " -> " << regIt->second->name << std::endl;
                        incoming.first = regIt->second;
                    }
                    
                    // 更新块引用
                    auto labelIt = labelReplaceMap.find(incoming.second);
                    if (labelIt != labelReplaceMap.end()) {
                        std::cout << "[LoopUnroll]   block reference: " << incoming.second->label << " -> " << labelIt->second->label << std::endl;
                        incoming.second = labelIt->second;
                    }
                }
                std::cout << "[LoopUnroll]   final phi: " << phi->toString() << std::endl;
            }
            
            // 更新普通指令的操作数
            for (auto& inst : bb->instructions) {
                for (size_t j = 0; j < inst->operands.size(); ++j) {
                    auto it = regReplaceMap.find(inst->operands[j]);
                    if (it != regReplaceMap.end()) {
                        std::cout << "[LoopUnroll] updated operand: " << inst->name << " -> " << it->second->name << std::endl;
                        inst->operands[j] = it->second;
                    }
                }
                
                // 更新分支指令的标签
                if (auto* br = dynamic_cast<BranchInst*>(inst.get())) {
                    for (auto& pair : labelReplaceMap) {
                        if (br->trueLabel == pair.first->label) {
                            br->trueLabel = pair.second->label;
                        }
                        if (br->isConditional() && br->falseLabel == pair.first->label) {
                            br->falseLabel = pair.second->label;
                        }
                    }
                }
            }
        }
        
        // 删除边 (old_exiting -> exit)
        if (oldExiting != originalHeader){
            if (auto* br = dynamic_cast<BranchInst*>(oldExiting->getTerminator())) {
                if (br->isConditional()) {
                    if (br->trueLabel == exit->label) {
                        auto newBr = std::make_unique<BranchInst>(br->falseLabel);
                        br->removeFromParent();
                        oldExiting->addInstruction(std::move(newBr));
                    } else if (br->falseLabel == exit->label) {
                        auto newBr = std::make_unique<BranchInst>(br->trueLabel);
                        br->removeFromParent();
                        oldExiting->addInstruction(std::move(newBr));
                    }
                }
            }
        }
        
        // 删除newHeader中来自oldPreheader的incoming
        std::vector<Value*> valuesFromPreheader;
        for (auto& phi : oldHeader->phi_instructions) {
            Value* valFromPreheader = nullptr;
            for (auto& incoming : phi->incoming_values) {
                if (incoming.second == oldPreheader) {
                    valFromPreheader = incoming.first;
                    break;
                }
            }
            if (valFromPreheader) {
                valuesFromPreheader.push_back(valFromPreheader);
            } else {
                std::cout << "[LoopUnroll] no value from preheader: " << phi->toString() << std::endl;
            }
        }
        for (size_t i = 0; i < newHeader->phi_instructions.size() && i < valuesFromPreheader.size(); ++i) {
            auto& newPhi = newHeader->phi_instructions[i];
            auto& incoming_values = newPhi->incoming_values;
            incoming_values.erase(
                std::remove_if(incoming_values.begin(), incoming_values.end(),
                    [oldPreheader](const std::pair<Value*, BasicBlock*>& incoming) {
                        return incoming.second == oldPreheader;
                    }),
                incoming_values.end()
            );
        }
        
        // 删除old_header中来自old_latch的incoming, 同时将这个incoming添加到new_header中
        std::cout << "[LoopUnroll] deleting incoming values from old_latch: " << oldLatch->label << std::endl;
        for (size_t i = 0; i < oldHeader->phi_instructions.size() && i < newHeader->phi_instructions.size(); ++i) {
            auto& oldPhi = oldHeader->phi_instructions[i];
            auto& newPhi = newHeader->phi_instructions[i];
            auto& incoming_values = oldPhi->incoming_values;
            auto oldSize = incoming_values.size();

            // 先找到所有来自oldLatch的incoming
            std::vector<std::pair<Value*, BasicBlock*>> latchIncomings;
            for (auto& incoming : incoming_values) {
                if (incoming.second == oldLatch) {
                    latchIncomings.push_back(incoming);
                }
            }

            // 删除oldHeader中来自oldLatch的incoming
            incoming_values.erase(
                std::remove_if(incoming_values.begin(), incoming_values.end(),
                    [oldLatch](const std::pair<Value*, BasicBlock*>& incoming) {
                        return incoming.second == oldLatch;
                    }),
                incoming_values.end()
            );
            std::cout << "[LoopUnroll] Removed " << (oldSize - incoming_values.size()) 
                     << " incoming values from " << oldPhi->toString() << std::endl;

            // 将这些incoming添加到newHeader对应的phi中
            for (auto& incoming : latchIncomings) {
                // 这里要注意，值需要做一次regReplaceMap映射（如果有的话）
                // 真的需要吗
                Value* mappedVal = incoming.first;
                newPhi->incoming_values.emplace_back(mappedVal, oldLatch);
                std::cout << "[LoopUnroll] Added incoming (" << mappedVal->name << ", " << oldLatch->label << ") to " << newPhi->toString() << std::endl;
            }
        }
        
        // 添加边 (old_latch -> new_header)
        std::cout << "[LoopUnroll] adding edge: " << oldLatch->label << " -> " << newHeader->label << std::endl;
        auto newLatchBr = std::make_unique<BranchInst>(newHeader->label);
        oldLatch->getTerminator()->removeFromParent();
        oldLatch->addInstruction(std::move(newLatchBr));

        // 更新exit块中的PHI指令（未知trip或非整除或fullyUnrollMode都需要）
        if (!hasConstTrip || remainderIterations > 0 || fullyUnrollMode) {
            for (auto& phi : exit->phi_instructions) {
                for (auto& incoming : phi->incoming_values) {
                    std::cout << "[LoopUnroll] updating exit phi: " << incoming.first->name << std::endl;
                    auto labelIt = labelReplaceMap.find(incoming.second);
                    if (labelIt != labelReplaceMap.end() && incoming.second != oldPreheader) {
                        std::cout << "   block reference: " << incoming.second->label << " -> " << labelIt->second->label << std::endl;
                        incoming.second = labelIt->second;
                    }
                    
                    auto regIt = regReplaceMap.find(incoming.first);
                    if (regIt != regReplaceMap.end() && incoming.second != oldPreheader) {
                        std::cout << "   incoming value: " << incoming.first->name << " -> " << regIt->second->name << std::endl;
                        incoming.first = regIt->second;
                    }
                }
            }
        }
        
        // 更新循环变量
        std::cout << "[LoopUnroll] updating loop variables" << std::endl;
        oldHeader = newHeader;
        oldExiting = newExiting;
        oldPreheader = oldLatch;  // 下一次迭代的preheader是当前的oldLatch
        oldLatch = newLatch;
        oldLoopNodes = newLoopNodes;
        
        iter += info.step;
    }

    // 最后的latch(oldLatch) -> originalHeader
    if (!fullyUnrollMode) {
        if (auto* br = dynamic_cast<BranchInst*>(oldLatch->getTerminator())) {
            if (!br->isConditional()) {
                br->trueLabel = originalHeader->label;
                std::cout << "[LoopUnroll] latch(oldLatch) -> originalHeader" << br->toString() << std::endl;
            }
        } else {
            std::cerr << "[LoopUnroll][Error] latch(oldLatch) -> originalHeader: cannot find BranchInst terminator" << std::endl;
        }
    } else {
        // 最后一次迭代：删除latch->header边，保持exiting->exit边
        std::cout << "[LoopUnroll] deleting latch->header edge" << std::endl;
        std::cout << "[LoopUnroll] oldHeader: " << oldHeader->label << std::endl;
        std::cout << "[LoopUnroll] oldLatch: " << oldLatch->label << std::endl;
        for (auto& phi : oldHeader->phi_instructions) {
            auto& incoming_values = phi->incoming_values;
            incoming_values.erase(
                std::remove_if(incoming_values.begin(), incoming_values.end(),
                    [oldLatch](const std::pair<Value*, BasicBlock*>& incoming) {
                        return incoming.second == oldLatch;
                    }),
                incoming_values.end()
            );
        }

        // 删除oldLoopNodes中除了oldHeader的所有块
        // 这里的oldLoopNodes是最后一次展开的循环的Nodes
        std::cout << "[LoopUnroll] deleting all unrolled loop blocks in the last iteration except oldHeader" << std::endl;
        auto& basicBlocks = function.basicBlocks;
        for (auto* bb : oldLoopNodes) {
            if (bb == oldHeader) continue;
            auto it = std::find_if(basicBlocks.begin(), basicBlocks.end(),
                [bb](const std::unique_ptr<BasicBlock>& block) {
                    return block.get() == bb;
                });
            if (it != basicBlocks.end()) {
                std::cout << "[LoopUnroll] deleting block: " << bb->label << std::endl;
                allLoopBlocks.erase(std::remove(allLoopBlocks.begin(), allLoopBlocks.end(), bb), allLoopBlocks.end());
                basicBlocks.erase(it);
            }
        }
        
        // 更新最后的exiting->exit分支为无条件分支
        std::cout << "[LoopUnroll] updating exiting->exit branch" << std::endl;
        if (auto* br = dynamic_cast<BranchInst*>(oldExiting->getTerminator())) {
            if (br->isConditional()) {
                std::string exitLabel = exit->label;
                if (br->trueLabel == exitLabel) {
                    auto newBr = std::make_unique<BranchInst>(br->trueLabel);
                    br->removeFromParent();
                    oldExiting->addInstruction(std::move(newBr));
                } else if (br->falseLabel == exitLabel) {
                    auto newBr = std::make_unique<BranchInst>(br->falseLabel);
                    br->removeFromParent();
                    oldExiting->addInstruction(std::move(newBr));
                }
            }
        }

        // 删除边originalHeader(originalExiting) -> originalExit
        if (auto* br = dynamic_cast<BranchInst*>(originalHeader->getTerminator())) {
            if (br->isConditional()) {
                std::string exitLabel = exit->label;
                if (br->trueLabel == exitLabel) {
                    auto newBr = std::make_unique<BranchInst>(br->falseLabel);
                    br->removeFromParent();
                    originalHeader->addInstruction(std::move(newBr));
                } else if (br->falseLabel == exitLabel) {
                    auto newBr = std::make_unique<BranchInst>(br->trueLabel);
                    br->removeFromParent();
                    originalHeader->addInstruction(std::move(newBr));
                }
            }
        }
    }

    // ============remainder逻辑============
    if (!hasConstTrip || remainderIterations > 0) {
        std::cout << "\n================remainder逻辑=================\n" << std::endl;
        std::cout << "[LoopUnroll] Creating remainder loop for " << (hasConstTrip ? remainderIterations : -1) << " iterations" << std::endl;
        std::map<Value*, Value*> regReplaceMap;
        std::map<BasicBlock*, BasicBlock*> labelReplaceMap;
        
        BasicBlock* newHeader = nullptr;
        BasicBlock* newExiting = nullptr;
        BasicBlock* newLatch = nullptr;
        std::set<BasicBlock*> newLoopNodes;
        
        // 复制所有循环块（按原函数顺序），建立label映射
        std::vector<BasicBlock*> currLoopOrder;
        for (size_t bi = 0; bi < function.basicBlocks.size(); ++bi) {
            BasicBlock* b = function.basicBlocks[bi].get();
            if (oldLoopNodes.count(b)) currLoopOrder.push_back(b);
        }
        for (auto bb : currLoopOrder) {
            std::string newLabel;
            if (bb == oldHeader) {
                newLabel = generateUniqueLabel("header_rem_" + std::to_string(abs(iter)));
            }
            else if (bb == oldExiting) {
                newLabel = generateUniqueLabel("exiting_rem_" + std::to_string(abs(iter)));
            }
            else if (bb == oldLatch) {
                newLabel = generateUniqueLabel("latch_rem_" + std::to_string(abs(iter)));
            } else {
                newLabel = generateUniqueLabel("body_rem_" + std::to_string(abs(iter)));
            }
            BasicBlock* newbb = copyBasicBlock(bb, newLabel, iter);
            newLoopNodes.insert(newbb);
            labelReplaceMap[bb] = newbb;
            
            if (bb == oldHeader) {
                newHeader = newbb;
                std::cout << "[LoopUnroll] newHeader: " << newHeader->label << std::endl;
            }
            if (bb == oldExiting) {
                newExiting = newbb;
                std::cout << "[LoopUnroll] newExiting: " << newExiting->label << std::endl;
            }
            if (bb == oldLatch) {
                newLatch = newbb;
                std::cout << "[LoopUnroll] newLatch: " << newLatch->label << std::endl;
            }
        }
        
        // 建立值映射关系（保持函数顺序）
        for (auto bb : currLoopOrder) {
            BasicBlock* newbb = labelReplaceMap[bb];
            
            // 映射PHI指令
            for (size_t j = 0; j < bb->phi_instructions.size() && j < newbb->phi_instructions.size(); ++j) {
                regReplaceMap[bb->phi_instructions[j].get()] = newbb->phi_instructions[j].get();
            }
            
            // 映射普通指令
            auto oldIt = bb->instructions.begin();
            auto newIt = newbb->instructions.begin();
            while (oldIt != bb->instructions.end() && newIt != newbb->instructions.end()) {
                regReplaceMap[oldIt->get()] = newIt->get();
                ++oldIt;
                ++newIt;
            }
        }
        
        // 更新新块中的所有引用
        for (auto bb : newLoopNodes) {
            // 更新PHI指令的操作数和来源
            for (auto& phi : bb->phi_instructions) {
                std::cout << "[LoopUnroll] updating phi: " << phi->toString() << std::endl;
                for (auto& incoming : phi->incoming_values) {
                    // 更新值引用
                    auto regIt = regReplaceMap.find(incoming.first);
                    if (regIt != regReplaceMap.end()) {
                        std::cout << "[LoopUnroll]   incoming value: " << incoming.first->name << " -> " << regIt->second->name << std::endl;
                        incoming.first = regIt->second;
                    }
                    
                    // 更新块引用
                    auto labelIt = labelReplaceMap.find(incoming.second);
                    if (labelIt != labelReplaceMap.end()) {
                        std::cout << "[LoopUnroll]   block reference: " << incoming.second->label << " -> " << labelIt->second->label << std::endl;
                        incoming.second = labelIt->second;
                    }
                }
                std::cout << "[LoopUnroll]   final phi: " << phi->toString() << std::endl;
            }
            
            // 更新普通指令的操作数
            for (auto& inst : bb->instructions) {
                for (size_t j = 0; j < inst->operands.size(); ++j) {
                    auto it = regReplaceMap.find(inst->operands[j]);
                    if (it != regReplaceMap.end()) {
                        std::cout << "[LoopUnroll] updated operand: " << inst->name << " -> " << it->second->name << std::endl;
                        inst->operands[j] = it->second;
                    }
                }
                
                // 更新分支指令的标签
                if (auto* br = dynamic_cast<BranchInst*>(inst.get())) {
                    for (auto& pair : labelReplaceMap) {
                        if (br->trueLabel == pair.first->label) {
                            br->trueLabel = pair.second->label;
                        }
                        if (br->isConditional() && br->falseLabel == pair.first->label) {
                            br->falseLabel = pair.second->label;
                        }
                    }
                }
            }
        }
                
        // 在remainder header恢复原始比较，不使用偏移adj
        {
            bool restored = false;
            int k = effectiveUnrollFactor - 1;
            int stepAbs = std::abs(info.step);
            int offsetVal = k * stepAbs;
            if (offsetVal != 0) {
                if (auto* term = dynamic_cast<BranchInst*>(newHeader->getTerminator())) {
                    if (term->isConditional()) {
                        if (auto* icmp = dynamic_cast<ICmpInst*>(term->operands[0])) {
                            for (int oi = 0; oi < 2; ++oi) {
                                if (auto* bop = dynamic_cast<BinaryOperator*>(icmp->operands[oi])) {
                                    if ((bop->opcode == Opcode::Add || bop->opcode == Opcode::Sub) && bop->operands.size() == 2) {
                                        ConstantInt* c0 = dynamic_cast<ConstantInt*>(bop->operands[0]);
                                        ConstantInt* c1 = dynamic_cast<ConstantInt*>(bop->operands[1]);
                                        ConstantInt* cst = c1 ? c1 : c0;
                                        if (cst && std::abs(cst->value) == offsetVal) {
                                            Value* varOp = c1 ? bop->operands[0] : bop->operands[1];
                                            icmp->replaceOperand(icmp->operands[oi], varOp);
                                            restored = true;
                                            std::cout << "[LoopUnroll] Remainder header: restored compare to original variable" << std::endl;
                                        }
                                    }
                                }
                            }
                            
                            // 计算映射后的primaryVar（递归判定变量侧）
                            Value* mappedPrimaryVar = primaryVar;
                            auto itPV = regReplaceMap.find(primaryVar);
                            if (itPV != regReplaceMap.end()) mappedPrimaryVar = itPV->second;

                            // 额外：同步边界为oldHeader中icmp的边界对应的新值（鲁棒克隆）
                            if (auto* oldTerm = dynamic_cast<BranchInst*>(oldHeader->getTerminator())) {
                                if (oldTerm->isConditional()) {
                                    if (auto* oldIcmp = dynamic_cast<ICmpInst*>(oldTerm->operands[0])) {
                                        Value* lhsOld = oldIcmp->operands[0];
                                        Value* rhsOld = oldIcmp->operands[1];
                                        bool varOnLHSOld = containsPrimaryVar(lhsOld, primaryVar);
                                        bool varOnRHSOld = containsPrimaryVar(rhsOld, primaryVar);
                                        Value* oldBound = varOnLHSOld && !varOnRHSOld ? rhsOld : (varOnRHSOld && !varOnLHSOld ? lhsOld : rhsOld);

                                        // 找到新icmp中"变量侧"的索引：递归匹配 mappedPrimaryVar
                                        int varIdx = -1;
                                        if (containsPrimaryVar(icmp->operands[0], mappedPrimaryVar)) varIdx = 0;
                                        else if (containsPrimaryVar(icmp->operands[1], mappedPrimaryVar)) varIdx = 1;
                                        
                                        // 尝试通过映射拿到 boundNew
                                        Value* boundNew = nullptr;
                                        auto itMap = regReplaceMap.find(oldBound);
                                        if (itMap != regReplaceMap.end()) {
                                            boundNew = itMap->second;
                                        } else {
                                            // 兜底：在 newHeader 内克隆纯表达式（确保插入于icmp之前）
                                            size_t icmpIdx = 0;
                                            size_t currIdx = 0;
                                            for (auto it = newHeader->instructions.begin(); it != newHeader->instructions.end(); ++it, ++currIdx) {
                                                if (it->get() == icmp) { icmpIdx = currIdx; break; }
                                            }
                                            size_t insertIdx = icmpIdx;
                                            boundNew = clonePureExprForRemainder(oldBound, newHeader, regReplaceMap, loop, function, insertIdx);
                                        }

                                        if (!boundNew) {
                                            std::cerr << "[LoopUnroll][Error] Cannot materialize remainder bound; aborting unroll." << std::endl;
                                            return false;
                                        }

                                        // 确定替换边（若无法识别变量侧，则默认替换非变量侧）
                                        int boundIdx = (varIdx == 0 ? 1 : (varIdx == 1 ? 0 : 1));
                                        icmp->replaceOperand(icmp->operands[boundIdx], boundNew);
                                        std::cout << "[LoopUnroll] Remainder header: synced bound from original header (robust-recursive)" << std::endl;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (!restored) {
                std::cerr << "[LoopUnroll][Warn] cannot restore compare to original variable (no offset found)" << std::endl;
            }
        }
                
        // 删除newHeader中来自oldPreheader的incoming
        std::vector<Value*> valuesFromPreheader2;
        for (auto& phi : oldHeader->phi_instructions) {
            Value* valFromPreheader = nullptr;
            for (auto& incoming : phi->incoming_values) {
                if (incoming.second == oldPreheader) {
                    valFromPreheader = incoming.first;
                    break;
                }
            }
            if (valFromPreheader) {
                valuesFromPreheader2.push_back(valFromPreheader);
            } else {
                std::cout << "[LoopUnroll] no value from preheader: " << phi->toString() << std::endl;
            }
        }
        for (size_t i = 0; i < newHeader->phi_instructions.size() && i < valuesFromPreheader2.size(); ++i) {
            auto& newPhi = newHeader->phi_instructions[i];
            auto& incoming_values = newPhi->incoming_values;
            incoming_values.erase(
                std::remove_if(incoming_values.begin(), incoming_values.end(),
                    [oldPreheader](const std::pair<Value*, BasicBlock*>& incoming) {
                        return incoming.second == oldPreheader;
                    }),
                incoming_values.end()
            );
        }

        // 删除边 (old_exiting -> exit)
        if (oldExiting != originalHeader){
            if (auto* br = dynamic_cast<BranchInst*>(oldExiting->getTerminator())) {
                if (br->isConditional()) {
                    if (br->trueLabel == exit->label) {
                        auto newBr = std::make_unique<BranchInst>(br->falseLabel);
                        br->removeFromParent();
                        oldExiting->addInstruction(std::move(newBr));
                    } else if (br->falseLabel == exit->label) {
                        auto newBr = std::make_unique<BranchInst>(br->trueLabel);
                        br->removeFromParent();
                        oldExiting->addInstruction(std::move(newBr));
                    }
                }
            }
        }
        
        // 添加边 (newlatch -> new_header)
        std::cout << "[LoopUnroll] adding edge: " << newLatch->label << " -> " << newHeader->label << std::endl;
        auto newLatchBr2 = std::make_unique<BranchInst>(newHeader->label);
        newLatch->getTerminator()->removeFromParent();
        newLatch->addInstruction(std::move(newLatchBr2));

        // 删除old_header中来自old_latch的incoming, 同时将这个incoming添加到originalHeader中
        std::cout << "[LoopUnroll] deleting incoming values from old_latch: " << oldLatch->label << std::endl;
        for (size_t i = 0; i < oldHeader->phi_instructions.size() && i < originalHeader->phi_instructions.size(); ++i) {
            auto& oldPhi = oldHeader->phi_instructions[i];
            auto& newPhi = originalHeader->phi_instructions[i];
            auto& incoming_values = oldPhi->incoming_values;
            auto oldSize = incoming_values.size();

            // 先找到所有来自oldLatch的incoming
            std::vector<std::pair<Value*, BasicBlock*>> latchIncomings;
            for (auto& incoming : incoming_values) {
                if (incoming.second == oldLatch) {
                    latchIncomings.push_back(incoming);
                }
            }

            // 删除oldHeader中来自oldLatch的incoming
            incoming_values.erase(
                std::remove_if(incoming_values.begin(), incoming_values.end(),
                    [oldLatch](const std::pair<Value*, BasicBlock*>& incoming) {
                        return incoming.second == oldLatch;
                    }),
                incoming_values.end()
            );
            std::cout << "[LoopUnroll] Removed " << (oldSize - incoming_values.size()) 
                    << " incoming values from " << oldPhi->toString() << std::endl;

            // 将这些incoming添加到newHeader对应的phi中
            for (auto& incoming : latchIncomings) {
                Value* mappedVal = incoming.first;
                newPhi->incoming_values.emplace_back(mappedVal, oldLatch);
                std::cout << "[LoopUnroll] Added incoming (" << mappedVal->name << ", " << oldLatch->label << ") to " << newPhi->toString() << std::endl;
            }
        }
        
        // 更新exit块中的PHI指令
        for (auto& phi : exit->phi_instructions) {
            for (auto& incoming : phi->incoming_values) {
                std::cout << "[LoopUnroll] updating exit phi: " << incoming.first->name << std::endl;
                auto labelIt = labelReplaceMap.find(incoming.second);
                if (labelIt != labelReplaceMap.end() && incoming.second != oldPreheader) {
                    std::cout << "   block reference: " << incoming.second->label << " -> " << labelIt->second->label << std::endl;
                    incoming.second = labelIt->second;
                }
                
                auto regIt = regReplaceMap.find(incoming.first);
                if (regIt != regReplaceMap.end() && incoming.second != oldPreheader) {
                    std::cout << "   incoming value: " << incoming.first->name << " -> " << regIt->second->name << std::endl;
                    incoming.first = regIt->second;
                }
            }
        }

        // (originalHeader -> exit) -> (originalHeader -> remHeader), 仅在已知trip时更新退出比较值
        for (auto& inst : originalHeader->instructions) {
            if (auto* br = dynamic_cast<BranchInst*>(inst.get())) {
                if (br->isConditional()) {
                    if (br->trueLabel == exit->label) {
                        br->trueLabel = newHeader->label;
                    } else if (br->falseLabel == exit->label) {
                        br->falseLabel = newHeader->label;
                    }

                }
            }
        }
        // 额外修复：exit的PHI仍可能保留来自originalHeader的incoming，这里将其替换为remainder对应的前驱与值
        {
            BasicBlock* replacementPred = newExiting ? newExiting : newHeader;
            for (auto& phi : exit->phi_instructions) {
                for (auto& incoming : phi->incoming_values) {
                    if (incoming.second == originalHeader) {
                        // 更新前驱块为remainder的前驱
                        incoming.second = replacementPred;
                        // 优先使用regReplaceMap做值映射
                        auto it = regReplaceMap.find(incoming.first);
                        if (it != regReplaceMap.end()) {
                            incoming.first = it->second;
                        } else {
                            // 回退：若是originalHeader的某个phi，按索引映射到newHeader对应phi
                            bool replaced = false;
                            size_t maxPhi = std::min(originalHeader->phi_instructions.size(), newHeader->phi_instructions.size());
                            for (size_t i = 0; i < maxPhi; ++i) {
                                if (incoming.first == originalHeader->phi_instructions[i].get()) {
                                    incoming.first = newHeader->phi_instructions[i].get();
                                    replaced = true;
                                    break;
                                }
                            }
                            if (!replaced) {
                                std::cout << "[LoopUnroll][Warn] exit phi incoming from originalHeader kept value: "
                                          << incoming.first->name << std::endl;
                            }
                        }
                    }
                }
            }
        }
        
        // 更新remHeader中的phi指令，接收来自originalHeader中的值
        for (size_t i = 0; i < originalHeader->phi_instructions.size() && i < newHeader->phi_instructions.size(); ++i) {
            auto& oldPhi = originalHeader->phi_instructions[i];
            auto& newPhi = newHeader->phi_instructions[i];
            auto& incoming_values = oldPhi->incoming_values;
            
            newPhi->addIncoming(oldPhi.get(), originalHeader);
        }


    } else if (!fullyUnrollMode) {
        // 删除边 (old_exiting -> exit)
        if (oldExiting != originalHeader){
            if (auto* br = dynamic_cast<BranchInst*>(oldExiting->getTerminator())) {
                if (br->isConditional()) {
                    if (br->trueLabel == exit->label) {
                        auto newBr = std::make_unique<BranchInst>(br->falseLabel);
                        br->removeFromParent();
                        oldExiting->addInstruction(std::move(newBr));
                    } else if (br->falseLabel == exit->label) {
                        auto newBr = std::make_unique<BranchInst>(br->trueLabel);
                        br->removeFromParent();
                        oldExiting->addInstruction(std::move(newBr));
                    }
                }
            }
        }

        // 删除old_header中来自old_latch的incoming, 同时将这个incoming添加到originalHeader中
        std::cout << "[LoopUnroll] deleting incoming values from old_latch: " << oldLatch->label << std::endl;
        for (size_t i = 0; i < oldHeader->phi_instructions.size() && i < originalHeader->phi_instructions.size(); ++i) {
            auto& oldPhi = oldHeader->phi_instructions[i];
            auto& newPhi = originalHeader->phi_instructions[i];
            auto& incoming_values = oldPhi->incoming_values;
            auto oldSize = incoming_values.size();

            // 先找到所有来自oldLatch的incoming
            std::vector<std::pair<Value*, BasicBlock*>> latchIncomings;
            for (auto& incoming : incoming_values) {
                if (incoming.second == oldLatch) {
                    latchIncomings.push_back(incoming);
                }
            }

            // 删除oldHeader中来自oldLatch的incoming
            incoming_values.erase(
                std::remove_if(incoming_values.begin(), incoming_values.end(),
                    [oldLatch](const std::pair<Value*, BasicBlock*>& incoming) {
                        return incoming.second == oldLatch;
                    }),
                incoming_values.end()
            );
            std::cout << "[LoopUnroll] Removed " << (oldSize - incoming_values.size()) 
                    << " incoming values from " << oldPhi->toString() << std::endl;

            // 将这些incoming添加回oldHeader对应的phi（无需映射）
            for (auto& incoming : latchIncomings) {
                Value* mappedVal = incoming.first;
                newPhi->incoming_values.emplace_back(mappedVal, oldLatch);
                std::cout << "[LoopUnroll] Added incoming (" << mappedVal->name << ", " << oldLatch->label << ") to " << newPhi->toString() << std::endl;
            }
        }
    }
    
    // 重建CFG和use-def链
    std::cout << "[LoopUnroll] rebuilding CFG and use-def chains" << std::endl;
    cfg::buildCFG(function);
    cfg::rebuildUseDefChainsOnFunction(function, false);
    
    // 合并基本块
    // std::cout << "[LoopUnroll] allLoopBlocks: ";
    // for (auto& bb : allLoopBlocks) {
    //     if (bb) {std::cout << bb->label << " ";}
    //     else {std::cerr << "found null block!";}
    // }
    std::cout << std::endl;
    #if OPEN_MERGE_BB
    std::vector<BasicBlock*> excluded;
    excluded.clear();
    merge_bb::MergeBasicBlocks(function, allLoopBlocks, excluded);
    #endif
    
    // std::cout << "[LoopUnroll] Loop " << loop->header->label << " unrolling completed successfully" << std::endl;
    return true;
}

// --- LoopUnrollPass实现 ---
bool LoopUnrollPass::run() {
    bool changed = false;
    for (auto& func : module.functions) {
        if (!func->isDeclaration()) {
            changed |= runOnFunction(*func);
        }
    }
    return changed; // TODO
}

bool LoopUnrollPass::runOnFunction(Function& F) {
    std::cout << "[LoopUnrollPass] Running on function: " << F.name << std::endl;
    
    std::cout << "[LoopUnrollPass] Starting constant loop fully unroll" << std::endl;
    
    // 构建支配树
    cfg::DominatorTree DT;
    DT.runOnFunction(F);
    
    // 分析循环
    LoopAnalysis loopAnalysis;
    loopAnalysis.runOnFunction(F, DT);
    
    // 规范化循环
    loopAnalysis.normalizeLoops(F);
    loopAnalysis.runLCSSA(F);
    
    // DFS遍历所有循环并尝试展开
    // TODO: 改成迭代式地工作，直到达到一个不动点
    bool isUnrolled = false;
    for (auto& loop_ptr : loopAnalysis.loops) {
        if (!loop_ptr) continue;
        if (!loop_ptr->getParent()) {  // 只处理最外层循环
            dfsUnroll(F, loop_ptr.get(), isUnrolled, loopAnalysis);
            std::cout << "return form dfs" << std::endl;
        }
    }
    
    std::cout << "[LoopUnrollPass] Constant loop fully unroll completed" << std::endl;
    
    return isUnrolled;
}

void LoopUnrollPass::dfsUnroll(Function& F, Loop* loop, bool& isUnrolled, LoopAnalysis& loopAnalysis) {
    std::cout << "[LoopUnrollPass] DFS unrolling loop: " << loop->getHeader()->label << std::endl;
    // DFS遍历循环树
    for (auto child : loop->getChildren()) {
        dfsUnroll(F, child, isUnrolled, loopAnalysis);
    }

    if (loop->getChildren().size()) return;

    // if (isUnrolled) { // TODO: 改成迭代式地工作，直到达到一个不动点
    //     return;
    // }

    cfg::DominatorTree DT;
    DT.runOnFunction(F);
    
    // 分析循环
    loopAnalysis.reAnalysisLoop(loop, F, DT);

    // loop->printLoopInfo();

    SCEVAnalysis analyzer(&F, &module);
    analyzer.analyzeLoop(loop);

    Value* primaryVar = analyzer.getPrimaryLoopVariable(loop);

    LoopBoundInfo info;
    
    if (primaryVar) {
        std::cout << "Primary loop variable: " << primaryVar->name << std::endl;
        
        // 获取该变量的详细信息
        const auto& loopVars = analyzer.getLoopVariables(loop);
        if (loopVars.empty()) {
            std::cout << "[LoopUnrollPass][Reject] no loop variables found" << std::endl;
            return;
        }
        auto it = loopVars.find(primaryVar);
        if (it != loopVars.end()) {
            // 修正类型限定符丢弃问题，primaryVarInfo应为const引用
            const SCEVLoopVariableInfo& primaryVarInfo = it->second;
            std::cout << "[LoopUnrollPass] Primary variable: " << (primaryVarInfo.variable ? primaryVarInfo.variable->name : "null") << std::endl;
            
            if (!primaryVarInfo.isSimple) {
                std::cout << "[LoopUnrollPass][Reject] primary loop variable is not simple" << std::endl;
                return;
            }
            if (primaryVarInfo.initialValue == nullptr) {
                std::cout << "[LoopUnrollPass][Reject] primary loop variable initial value is null" << std::endl;
                return;
            }
            if (auto* constInt = dynamic_cast<ConstantInt*>(primaryVarInfo.initialValue)) {
                info.initvalue = constInt->value;
            }
            else {
                std::cout << "[LoopUnrollPass][Reject] primary loop variable initial value is not a constant" << std::endl;
                return;
            }

            bool hasBound = false;
            if (primaryVarInfo.upperBound == nullptr) {
                std::cout << "[LoopUnrollPass] primary loop variable upper bound is null" << std::endl;
            } else {
                if (auto* constInt = dynamic_cast<ConstantInt*>(primaryVarInfo.upperBound)) {
                    info.upperBound = constInt->value;
                }
                else {
                    std::cout << "[LoopUnrollPass][Info] primary loop variable upper bound is not a constant (dynamic bound)" << std::endl;
                }
                hasBound = true;
            }
            if (primaryVarInfo.lowerBound == nullptr) {
                std::cout << "[LoopUnrollPass] primary loop variable lower bound is null" << std::endl;
            } else {
                if (auto* constInt = dynamic_cast<ConstantInt*>(primaryVarInfo.lowerBound)) {
                    info.lowerBound = constInt->value;
                }
                else {
                    std::cout << "[LoopUnrollPass][Reject] primary loop variable lower bound is not a constant" << std::endl;
                }
                hasBound = true;
            }
            if (!hasBound) {
                std::cout << "[LoopUnrollPass][Reject] primary loop variable has no bound" << std::endl;
                return;
            }

            if (primaryVarInfo.stepValue == nullptr) {
                std::cout << "[LoopUnrollPass][Reject] primary loop variable step value is null" << std::endl;
                return;
            }
            if (auto* constInt = dynamic_cast<ConstantInt*>(primaryVarInfo.stepValue)) {
                info.step = constInt->value;
            }
            else {
                std::cout << "[LoopUnrollPass][Reject] primary loop variable step value is not a constant" << std::endl;
                return;
            }

            if (primaryVarInfo.constantTripCount) {
                info.tripCount = primaryVarInfo.constantTripCount.value();
            } else {
                // 支持非常量trip count：设为-1并继续
                info.tripCount = -1;
                std::cout << "[LoopUnrollPass][Info] primary loop variable constant trip count not found; proceed with dynamic trip" << std::endl;
            }
            info.condition = primaryVarInfo.exitCondition; // 比较条件
            // 通过primaryVarInfo.exitCondition判断isUpperBoundClosed
            // 只有"等于"、"大于等于"、"小于等于"、"大于等于(无符号)"、"小于等于(无符号)"为闭区间
            switch (primaryVarInfo.exitCondition) {
                case ICmpCond::EQ: case ICmpCond::UGE:
                case ICmpCond::ULE: case ICmpCond::SGE:
                case ICmpCond::SLE:
                    info.isUpperBoundClosed = true;
                    break;
                default:
                    info.isUpperBoundClosed = false;
                    break;
            }

            // 尝试展开当前循环
            if (isSimpleLoop(loop, info)) {                
                LoopUnroller unroller(F, LOOP_UNROLL_FACTOR);

                bool array_flag = false;
                for (auto& bb : loop->getBlocks()) {
                    for (auto& inst : bb->instructions) {
                        if (inst->opcode == Opcode::Load || inst->opcode == Opcode::Store) {
                            array_flag = true;
                            break;
                        }
                    }
                    if (array_flag) break;
                }
                if (array_flag) unroller.unrollFactor = 8;

                if (info.tripCount > 0) { // 常量trip
                    // 如果tripCount不能被unrollFactor整除，则尝试能整除的最大因子
                    int defaultUnroll = unroller.unrollFactor;
                    int trip = info.tripCount;
                    int chosenUnroll = defaultUnroll;
                    if (trip % defaultUnroll != 0) {
                        // 依次尝试
                        for (int f = 8; f >= 2; --f) {
                            if (trip % f == 0) {
                                chosenUnroll = f;
                                break;
                            }
                        }
                    }
                    unroller.unrollFactor = chosenUnroll;
                    if (canFullyUnrollLoop(loop, info)) {
                        isUnrolled |= unroller.partiallyUnrollLoop(loop, primaryVar, primaryVarInfo, info, /*fullyUnrollMode=*/true);
                    } else {
                        isUnrolled |= unroller.partiallyUnrollLoop(loop, primaryVar, primaryVarInfo, info, /*fullyUnrollMode=*/false);
                    }
                } else {
                    // 非常量trip：跳过canUnrollLoop基于次数的限制
                    isUnrolled |= unroller.partiallyUnrollLoop(loop, primaryVar, primaryVarInfo, info, /*fullyUnrollMode=*/false);
                }
            }
        }
    } else {
        std::cout << "[LoopUnrollPass][Reject] no primary loop variable found" << std::endl;
        return;
    }
}

} // namespace loop_unroll
} // namespace llvm_ir 