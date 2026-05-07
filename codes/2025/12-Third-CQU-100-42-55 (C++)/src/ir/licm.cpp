#include "../../include/ir/licm.h"
#include "../../include/ir/loop_analysis.h"
#include "../../include/ir/cfg.h" // For DominatorTree
#include "../../include/ir/function_analysis.h"
#include "../../include/ir/scev.h"
#include <iostream>
#include <set>
#include <vector>
#include <algorithm>

namespace llvm_ir {
namespace licm {

// Forward declarations for helpers
static bool loopMayWriteToMemory(Loop* loop, LICMPass* licm_pass); // 判断循环是否可能写入内存
static bool isMovable(Instruction* I, Loop* loop, LICMPass* licm_pass); // 判断指令是否可以移动
static bool isInvariant(Instruction* I, Loop* loop, const std::set<Instruction*>& current_invariants, Function* F, LICMPass* licm_pass, SCEVAnalysis* scev_analyzer); // 判断指令是否是循环不变量（结合SCEV）
static bool moveInvariants(const std::vector<Instruction*>& invariants, Loop* loop); // 将循环不变量移动到循环的preheader
static void fixupParentPointers(Function& F); // 修复父指针

// 新增：别名分析相关函数
static Value* getBasePointer(Value* ptr); // 递归地查找一个指针的最终基指针
static bool pointersMayAlias(Value* P1, Value* P2); // 简化版的别名检查
static bool isLoadInvariantInLoop(Instruction* load_inst, Loop* loop, LICMPass* licm_pass); // 检查一个load指令是否在循环中是不变的

// 辅助函数：判断一个Value是否为函数参数
static bool isFunctionParameter(Value* v, Function* F) {
    for (auto* param : F->parameters) {
        if (param == v) return true;
    }
    return false;
}

// 查找函数定义
Function* LICMPass::findFunction(const std::string& name) {
    for (auto& func : module.functions) {
        if (func->name == name) {
            return func.get();
        }
    }
    return nullptr;
}

// Main function for the pass on a single function
bool LICMPass::runOnFunction(Function& F) {
    bool hasChanged = false;
    std::cout << "[licm] Run LICM on function: " << F.name << std::endl;
    fixupParentPointers(F); // Fix parent pointers before analysis
    cfg::DominatorTree DT;
    DT.runOnFunction(F);
    LoopAnalysis LA;
    LA.runOnFunction(F, DT);
    LA.ensurePreheaders(F, DT);

    // 为本函数准备 SCEV 分析器
    SCEVAnalysis scev_analyzer(&F, &module);

    for (auto& loop_ptr : LA.loops) {
        Loop* loop = loop_ptr.get();
        std::cout << "[licm] Analyzing loop with header: " << (loop->getHeader() ? loop->getHeader()->label : "null") << std::endl;
        if (!loop->getPreheader()) {
            std::cout << "[licm][Warning] Skipping loop in " << F.name << " because it has no preheader." << std::endl;
            continue;
        }
        
        // 使用 SCEV 分析当前循环（仅作为信号，不直接移动）
        scev_analyzer.analyzeLoop(loop);
        const auto& scev_invariants_vals = scev_analyzer.getInvariantValues();

        // 基于闭包的迭代搜索：只有当依赖满足（在环外或已在集合中）时才加入，避免支配性破坏
        std::set<Instruction*> invariant_set;
        bool changed = true;
        int iter = 0;
        size_t scev_candidate_seen = 0;
        while (changed) {
            changed = false;
            iter++;
            std::cout << "[licm] Invariant search iteration " << iter << std::endl;
            for (BasicBlock* bb : loop->getBlocks()) {
                for (auto& inst_ptr : bb->instructions) {
                    Instruction* I = inst_ptr.get();
                    if (!I) continue;
                    if (invariant_set.count(I)) continue;
                    if (scev_invariants_vals.count(I)) {
                        // 统计被 SCEV 判定为不变量的候选数量
                        scev_candidate_seen++;
                    }
                    if (isInvariant(I, loop, invariant_set, &F, this, &scev_analyzer)) {
                        std::cout << "[licm]   Found invariant: " << I->toString() << std::endl;
                        invariant_set.insert(I);
                        changed = true;
                    }
                }
            }
        }
        if (invariant_set.empty()) {
            std::cout << "[licm] No invariants found in loop (SCEV candidates: " << scev_candidate_seen << ")." << std::endl;
            continue;
        }
        std::cout << "[licm] Found " << invariant_set.size() << " movable invariants (from " << scev_candidate_seen << " SCEV candidates) in a loop in " << F.name << std::endl;

        std::vector<Instruction*> ordered_invariants;
        for (auto& bb_ptr : F.basicBlocks) {
            if (loop->contains(bb_ptr.get())) {
                for (auto& inst_ptr : bb_ptr->instructions) {
                    if (invariant_set.count(inst_ptr.get())) {
                        ordered_invariants.push_back(inst_ptr.get());
                    }
                }
            }
        }
        std::cout << "[licm] Moving " << ordered_invariants.size() << " invariants to preheader." << std::endl;
        bool moved = moveInvariants(ordered_invariants, loop);
        hasChanged = hasChanged || moved;
    }
    
    return hasChanged;
}

// Entry point for the whole module
bool runOnModule(Module& M) {
    bool hasChanged = false;
    std::cout << "[LICM] Start LICM on module" << std::endl;
    for (auto& func : M.functions) {
        if (!func->isDeclaration()) {
            LICMPass licm_pass(M);
            bool funcChanged = licm_pass.runOnFunction(*func);
            hasChanged = hasChanged || funcChanged;
        }
    }
    return hasChanged;
}

// --- Helper Implementations ---

static void fixupParentPointers(Function& F) {
    // std::cout << "[licm] Fixing parent pointers for function " << F.name << std::endl;
    for (auto& bb_ptr : F.basicBlocks) {
        BasicBlock* B = bb_ptr.get();
        for (auto& phi_ptr : B->phi_instructions) {
            if (phi_ptr->parent != B) {
                phi_ptr->parent = B;
                std::cout << "[licm][Warning] Fix phi parent pointer: " << phi_ptr->toString() << " to " << B->label << std::endl;
            }
        }
        for (auto& inst_ptr : B->instructions) {
            if (inst_ptr->parent != B) {
                inst_ptr->parent = B;
                std::cout << "[licm][Warning] Fix inst parent pointer: " << inst_ptr->toString() << " to " << B->label << std::endl;
            }
        }
    }
}

static bool loopMayWriteToMemory(Loop* loop, LICMPass* licm_pass = nullptr) {
    for (BasicBlock* bb : loop->getBlocks()) {
        for (auto& inst_ptr : bb->instructions) {
            Opcode op = inst_ptr->opcode;
            if (op == Opcode::Store) {
                return true;
            } else if (op == Opcode::Call) {
                // 检查函数调用是否安全
                auto callInst = dynamic_cast<CallInst*>(inst_ptr.get());
                if (callInst && licm_pass) {
                    // 使用LICMPass的方法检查函数调用是否安全
                    function_analysis::FunctionAttributeAnalyzer analyzer;
                    if (!analyzer.isCallSafe(callInst)) {
                        return true;
                    }
                } else {
                    // 如果没有LICMPass或不是CallInst，保守地假设可能写入内存
                    return true;
                }
            }
        }
    }
    return false;
}

// 递归地查找一个指针的最终基指针
static Value* getBasePointer(Value* ptr) {
    if (!ptr) return nullptr;
    
    if (auto* gep = dynamic_cast<GetElementPtrInst*>(ptr)) {
        return getBasePointer(gep->operands[0]); // 递归查找基指针
    }
    // 其他指令如 BitCast, AddrSpaceCast 也可以在这里处理
    // 对于简化版本，我们只处理GEP指令
    return ptr;
}

// 简化版的别名检查
static bool pointersMayAlias(Value* P1, Value* P2) {
    if (!P1 || !P2) return false;
    
    // 1. 如果指针完全相同，则必定别名
    if (P1 == P2) return true;

    // 2. 获取基指针
    Value* Base1 = getBasePointer(P1);
    Value* Base2 = getBasePointer(P2);

    // 3. 如果基指针是不同的 alloca 或不同的 global，它们不可能别名
    bool is_alloca1 = dynamic_cast<AllocaInst*>(Base1) != nullptr;
    bool is_alloca2 = dynamic_cast<AllocaInst*>(Base2) != nullptr;
    bool is_global1 = dynamic_cast<GlobalVariable*>(Base1) != nullptr;
    bool is_global2 = dynamic_cast<GlobalVariable*>(Base2) != nullptr;

    if ((is_alloca1 || is_global1) && (is_alloca2 || is_global2)) {
        if (Base1 != Base2) {
            std::cout << "[licm]    指针别名分析：不同基指针，不可能别名" << std::endl;
            return false; // 来自不同的局部变量或全局变量，不可能别名
        }
    }
    
    // 4. 保守策略：如果无法证明它们不别名，就假设它们可能别名。
    // 这会处理从函数参数、load 结果等来的指针。
    std::cout << "[licm]    指针别名分析：保守假设可能别名" << std::endl;
    return true;
}

// 检查一个 load 指令是否在循环中是不变的
static bool isLoadInvariantInLoop(Instruction* load_inst, Loop* loop, LICMPass* licm_pass) {
    // load_inst 必须是 load 指令
    auto* load = dynamic_cast<LoadInst*>(load_inst);
    if (!load) return false;
    
    Value* load_ptr = load->getPointerOperand();
    if (!load_ptr) return false;

    std::cout << "[licm]  检查Load指令是否循环不变: " << load_inst->toString() << std::endl;

    // 遍历循环中的所有基本块和指令
    for (BasicBlock* bb : loop->getBlocks()) {
        for (auto& inst_ptr : bb->instructions) {
            Instruction* I = inst_ptr.get();

            // 检查是否有 store 指令可能写入 load 的地址
            if (auto* store = dynamic_cast<StoreInst*>(I)) {
                Value* store_ptr = store->getPointerOperand();
                // 在这里进行别名检查
                if (pointersMayAlias(load_ptr, store_ptr)) {
                    std::cout << "[licm]     Load aliases with store: " << store->toString() << std::endl;
                    return false; // 发现一个可能的写操作，load 不安全
                }
            }
            // 检查是否有 call 指令可能修改内存
            else if (auto* call = dynamic_cast<CallInst*>(I)) {
                if (licm_pass) {
                    Function* callee = licm_pass->findFunction(call->functionName);
                    // 如果 call 不是 readonly 或 readnone，它就可能修改任何内存
                    if (!callee || (!callee->attributes.count(FunctionAttribute::ReadOnly) && !callee->attributes.count(FunctionAttribute::ReadNone))) {
                        std::cout << "[licm]     Load invalidated by non-readonly call: " << call->toString() << std::endl;
                        return false; // 发现一个可能修改内存的调用
                    }
                } else {
                    // 如果没有LICMPass，保守地假设所有调用都可能修改内存
                    std::cout << "[licm]     Load invalidated by unknown call: " << call->toString() << std::endl;
                    return false;
                }
            }
        }
    }

    // 没有在循环中找到任何可能修改该地址的指令
    std::cout << "[licm]     Load指令在循环中是不变的" << std::endl;
    return true;
}

static bool isMovable(Instruction* I, Loop* loop, LICMPass* licm_pass = nullptr) {
    std::cout << "[licm] Check if movable: " << I->toString() << std::endl;
    bool movable = false;
    switch (I->opcode) {
        // Purely computational and movable
        case Opcode::Add: case Opcode::FAdd:
        case Opcode::Sub: case Opcode::FSub:
        case Opcode::Mul: case Opcode::FMul:
        case Opcode::SDiv: case Opcode::FDiv:
        case Opcode::SRem: case Opcode::FRem:
        case Opcode::And: case Opcode::Or: case Opcode::Xor:
        case Opcode::ICmp: case Opcode::FCmp:
        case Opcode::ZExt: case Opcode::SExt:
        case Opcode::SIToFP: case Opcode::FPToSI:
        case Opcode::GetElementPtr:
        case Opcode::Shl: case Opcode::LShr: case Opcode::AShr:
            movable = true;
            break;

        // Load is movable only if it's invariant in the loop
        case Opcode::Load:
            movable = isLoadInvariantInLoop(I, loop, licm_pass);
            break;

        // Not movable
        case Opcode::Alloca:  // Must stay in entry block
        case Opcode::Store:   // Has side effects
        case Opcode::Ret:     // Terminator
        case Opcode::Br:      // Terminator
        case Opcode::PHI:     // Tied to basic block
            movable = false;
            break;
            
        case Opcode::Call: {
            // 检查函数调用的可移动性
            auto* call_inst = dynamic_cast<CallInst*>(I);
            if (call_inst && licm_pass) {
                // 使用LICMPass的方法查找被调用的函数
                Function* callee = licm_pass->findFunction(call_inst->functionName);
                
                if (callee) {
                    // 如果函数是 readnone，它可以像算术指令一样被移动
                    if (callee->attributes.count(FunctionAttribute::ReadNone)) {
                        std::cout << "[licm]   Call to readnone function " << callee->name << ", movable." << std::endl;
                        movable = true;
                    }
                    // 如果函数是 readonly，它可以像 load 指令一样被移动
                    // （即，只有当循环中没有内存写入时才安全）
                    else if (callee->attributes.count(FunctionAttribute::ReadOnly)) {
                        std::cout << "[licm]   Call to readonly function " << callee->name << ", checking for loop writes..." << std::endl;
                        movable = !loopMayWriteToMemory(loop, licm_pass);
                    }
                    // 否则，保守地认为它不可移动
                    else {
                        std::cout << "[licm]   Call to function " << callee->name << " without readnone/readonly, not movable." << std::endl;
                        movable = false;
                    }
                } else {
                    // 如果是间接调用或找不到函数定义，我们无法知道被调函数是什么，必须保守
                    std::cout << "[licm]   Call to unknown function " << call_inst->functionName << ", not movable." << std::endl;
                    movable = false;
                }
            } else {
                // 如果没有LICMPass或不是CallInst，保守地认为不可移动
                std::cout << "[licm]   Call instruction without LICMPass context, not movable." << std::endl;
                movable = false;
            }
            break;
        }
        
        default:
            // Conservatively assume not movable
            movable = false;
            break;
    }
    if (!movable) {
        std::cout << "[licm]   Not movable." << std::endl;
    }
    return movable;
}

static bool isInvariant(Instruction* I, Loop* loop, const std::set<Instruction*>& current_invariants, Function* F, LICMPass* licm_pass, SCEVAnalysis* scev_analyzer) {
    std::cout << "[licm] Check if invariant: " << I->toString() << std::endl;
    if (!isMovable(I, loop, licm_pass)) {
        return false;
    }
    // 若提供了 SCEV，仅作为提示信号，不绕过依赖检查
    bool scev_marks_invariant = false;
    if (scev_analyzer) {
        const auto& invs = scev_analyzer->getInvariantValues();
        scev_marks_invariant = invs.count(I) > 0;
        if (scev_marks_invariant) {
            std::cout << "[licm]     SCEV hints invariant, verifying operands..." << std::endl;
        }
    }

    // 兼容：退化为原有基于操作数的保守判定
    for (Value* op : I->operands) {
        if (dynamic_cast<Constant*>(op) || isFunctionParameter(op, F) || dynamic_cast<GlobalVariable*>(op)) {
            std::cout << "[licm]     Operand is constant/param/global: " << op->toString() << std::endl;
            continue;
        }

        if (auto op_inst = dynamic_cast<Instruction*>(op)) {
            if (!loop->contains(op_inst->parent)) {
                std::cout << "[licm]     Operand defined outside loop: " << op_inst->toString() << std::endl;
                continue;
            }
            if (current_invariants.count(op_inst)) {
                std::cout << "[licm]     Operand is already an invariant: " << op_inst->toString() << std::endl;
                continue;
            }
            return false;
        } else {
            std::cout << "[licm]     Operand is non-instruction, assuming outside loop: " << op->toString() << std::endl;
        }
    }
    std::cout << "[licm]     All operands are loop-invariant. Hoisting." << std::endl;
    return true;
}

static bool moveInvariants(const std::vector<Instruction*>& invariants, Loop* loop) {
    bool hasChanged = false;
    BasicBlock* preheader = loop->getPreheader();
    if (!preheader) {
        std::cout << "[licm] No preheader, cannot move invariants." << std::endl;
        return false;
    }
    auto& ph_instructions = preheader->instructions;
    auto insert_pos = ph_instructions.end();
    if (!ph_instructions.empty()) {
        Opcode last_op = ph_instructions.back()->opcode;
        if (last_op == Opcode::Br || last_op == Opcode::Ret) {
            insert_pos = std::prev(ph_instructions.end());
        }
    }
    for (Instruction* I : invariants) {
        BasicBlock* original_parent = I->parent;
        if (!original_parent) continue;
        auto& source_instructions = original_parent->instructions;
        for (auto it = source_instructions.begin(); it != source_instructions.end(); ++it) {
            if (it->get() == I) {
                std::cout << "[licm] Move invariant: " << I->toString() << " from " << original_parent->label << " to " << preheader->label << std::endl;
                ph_instructions.splice(insert_pos, source_instructions, it);
                I->parent = preheader;
                hasChanged = true;
                break;
            }
        }
    }
    return hasChanged;
}

} // namespace licm
} // namespace llvm_ir