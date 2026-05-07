#include "../../include/ir/global_to_local.h"
#include "../../include/ir/cfg.h"
#include <unordered_map>
#include <set>
#include <iostream>
#include <algorithm>

namespace llvm_ir {
namespace global_to_local {

static bool isScalarType(Type t) {
    return t == Type::I32 || t == Type::Float || t == Type::I8 || t == Type::I64;
}

// 在函数入口插入指令，位置在已有的alloca之后
static Instruction* insertIntoFunctionEntry(Function& F, std::unique_ptr<Instruction> I) {
    BasicBlock* entry = F.getEntryBlock();
    if (!entry) return nullptr;
    auto& insts = entry->instructions;
    auto it = insts.begin();
    while (it != insts.end() && (*it)->opcode == Opcode::Alloca) ++it;
    I->parent = entry;
    return (insts.insert(it, std::move(I)))->get();
}

bool runOnModule(Module& M) {
    bool changed = false;

    // 收集每个全局变量的使用函数集合
    std::unordered_map<GlobalVariable*, std::set<Function*>> gvarToFuncs;
    for (auto& func_up : M.functions) {
        Function* F = func_up.get();
        if (F->isDeclaration() || F->name == "global") continue;
        for (auto& bb_up : F->basicBlocks) {
            BasicBlock* BB = bb_up.get();
            // 普通指令
            for (auto& inst_up : BB->instructions) {
                Instruction* I = inst_up.get();
                for (auto* op : I->operands) {
                    if (!op) continue;
                    if (auto* gv = dynamic_cast<GlobalVariable*>(op)) {
                        gvarToFuncs[gv].insert(F);
                    } else {
                        // 名字以@开头也可能代表全局
                        if (!op->name.empty() && op->name[0] == '@') {
                            // 在 Module.globalVariables 中找到匹配者
                            for (auto& gpu : M.globalVariables) {
                                if ("@" + gpu->name == op->name) {
                                    gvarToFuncs[gpu.get()].insert(F);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            // PHI 指令
            for (auto& phi_up : BB->phi_instructions) {
                PhiInst* P = phi_up.get();
                for (auto& inc : P->incoming_values) {
                    Value* op = inc.first;
                    if (!op) continue;
                    if (auto* gv = dynamic_cast<GlobalVariable*>(op)) {
                        gvarToFuncs[gv].insert(F);
                    } else if (!op->name.empty() && op->name[0] == '@') {
                        for (auto& gpu : M.globalVariables) {
                            if ("@" + gpu->name == op->name) {
                                gvarToFuncs[gpu.get()].insert(F);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    // std::cout << "[global_to_local] gvarToFuncs size: " << gvarToFuncs.size() << std::endl;
    // for (auto& gv : gvarToFuncs) {
    //     std::cout << "[global_to_local] gv: " << gv.first->name << " " << gv.second.size() << std::endl;
    //     for (auto& func : gv.second) {
    //         std::cout << "[global_to_local] func: " << func->name << std::endl;
    //     }
    // }

    // 计划需要降级的全局变量列表
    std::vector<GlobalVariable*> toDemote;
    for (auto& g_up : M.globalVariables) {
        GlobalVariable* GV = g_up.get();
        // 仅处理标量全局变量
        if (GV->type == Type::Array) continue;
        if (!isScalarType(GV->type)) continue;
        auto it = gvarToFuncs.find(GV);
        if (it == gvarToFuncs.end()) {
            // 无使用，可以考虑忽略；这里跳过
            continue;
        }
        if (it->second.size() == 1) {
            toDemote.push_back(GV);
        }
    }

    if (toDemote.empty()) {
        // std::cout << "toDemote is empty" << std::endl;
        return false;
    }

    // 对每个目标执行降级
    for (auto* GV : toDemote) {
        Function* targetFunc = *gvarToFuncs[GV].begin();
        if (!targetFunc || targetFunc->isDeclaration()) continue;

        // 在入口插入alloca，并对其零初始化
        IRBuilder builder;
        BasicBlock* entry = targetFunc->getEntryBlock();
        if (!entry) continue;
        builder.setInsertPoint(entry);
        // 在已有alloca之后插入
        auto allocaInst = std::make_unique<AllocaInst>(GV->type, "%" + GV->name + "_local");
        AllocaInst* allocaPtr = static_cast<AllocaInst*>(insertIntoFunctionEntry(*targetFunc, std::move(allocaInst)));
        if (!allocaPtr) continue;

        // store 初值（使用全局变量的初始值或零初始化）
        Value* initValue = nullptr;
        if (GV->initializer) {
            // 使用全局变量的初始值
            initValue = GV->initializer;
            // std::cout << "[global_to_local] using initializer: " << initValue->toString() << std::endl;
        } else {
            // 如果没有初始值，使用零初始化
            std::cout << "[global_to_local] no initializer found!" << std::endl;
            if (GV->type == Type::I32) initValue = new ConstantInt(0);
            else if (GV->type == Type::I8) initValue = new ConstantInt(0, Type::I8);
            else if (GV->type == Type::I64) initValue = new ConstantInt(0, Type::I64);
            else if (GV->type == Type::Float) initValue = new ConstantFloat(0.0f);
        }
        if (initValue) {
            auto storeInit = std::make_unique<StoreInst>(initValue, allocaPtr);
            insertIntoFunctionEntry(*targetFunc, std::move(storeInit));
        }

        // 替换该函数中对 @GV 的所有使用：
        // 用一个临时 ptr 值表示这个局部地址（保持类型为 ptr）
        // 由于我们的 IR 中对 load/store 都期望 ptr 操作数，这里直接用 alloca 指令值即可
        for (auto& bb_up : targetFunc->basicBlocks) {
            BasicBlock* BB = bb_up.get();
            // 普通指令
            for (auto& inst_up : BB->instructions) {
                Instruction* I = inst_up.get();
                for (auto& op : I->operands) {
                    if (!op) continue;
                    bool match = false;
                    if (auto* gv = dynamic_cast<GlobalVariable*>(op)) {
                        match = (gv == GV);
                    } else if (!op->name.empty() && op->name[0] == '@') {
                        match = (op->name == ("@" + GV->name));
                    }
                    if (match) {
                        I->replaceOperand(op, allocaPtr);
                    }
                }
            }
            // PHI 指令
            for (auto& phi_up : BB->phi_instructions) {
                PhiInst* P = phi_up.get();
                for (auto& inc : P->incoming_values) {
                    Value* op = inc.first;
                    if (!op) continue;
                    bool match = false;
                    if (auto* gv = dynamic_cast<GlobalVariable*>(op)) {
                        match = (gv == GV);
                    } else if (!op->name.empty() && op->name[0] == '@') {
                        match = (op->name == ("@" + GV->name));
                    }
                    if (match) {
                        P->replaceOperand(op, allocaPtr);
                    }
                }
            }
        }

        changed = true;
    }

    if (changed) {
        // 从 Module 中移除已降级的全局变量
        // M.globalVariables.erase(
        //     std::remove_if(M.globalVariables.begin(), M.globalVariables.end(), [&](const std::unique_ptr<GlobalVariable>& g) {
        //         return std::find(toDemote.begin(), toDemote.end(), g.get()) != toDemote.end();
        //     }),
        //     M.globalVariables.end());
        // // 从global函数中移除已降级的全局变量
        // for (auto& func_up : M.functions) {
        //     if (func_up->name == "global") {
                
        //     }
        // }

        // 重建 use-def 和 CFG 以确保后续 pass 正常
        cfg::rebuildUseDefChainsModule(M);
        for (auto& func_up : M.functions) {
            cfg::buildCFG(*func_up);
        }
    }

    return changed;
}

} // namespace global_to_local
} // namespace llvm_ir 