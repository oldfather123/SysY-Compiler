#include "../../include/ir/loop_interchange.h"
#include "../../include/ir/cfg.h"
#include <iostream>
#include <algorithm>
#include <functional>

namespace llvm_ir {
namespace loop_interchange {

// ===== Helper utilities =====
static bool isLoadOrStore(Instruction* inst) { return inst->opcode == Opcode::Load || inst->opcode == Opcode::Store; }

// 递归遍历一个值是否依赖 target，限定最大深度防止环
static bool recursiveDepends(Value* root, Value* target, int depth = 0) {
    if (!root) return false;
    if (root == target) return true;
    if (depth > 32) return false;
    if (auto* inst = dynamic_cast<Instruction*>(root)) {
        for (auto* op : inst->operands) {
            if (recursiveDepends(op, target, depth + 1)) return true;
        }
    }
    return false;
}

// 递归分析索引表达式的复杂度
static bool analyzeIndexComplexity(Value* val, Value* outerIndVar, Value* innerIndVar,
                                  int& outerCost, int& innerCost, bool& isComplex, int depth) {
    if (!val || depth > 10) {
        isComplex = true;
        return false;
    }
    
    // 如果是归纳变量，增加对应的成本
    if (val == outerIndVar) {
        outerCost++;
        return true;
    }
    if (val == innerIndVar) {
        innerCost++;
        return true;
    }
    
    // 如果是常量，认为是简单的
    if (dynamic_cast<ConstantInt*>(val) || dynamic_cast<ConstantFloat*>(val)) {
        return true;
    }
    
    // 如果是全局变量或函数参数，认为是简单的
    if (dynamic_cast<GlobalVariable*>(val) || !dynamic_cast<Instruction*>(val)) {
        return true;
    }
    
    // 如果是指令，递归分析
    auto* inst = dynamic_cast<Instruction*>(val);
    if (!inst) return true;
    
    switch (inst->opcode) {
        case Opcode::Add:
        case Opcode::Sub:
        case Opcode::Mul:
            // 简单的算术运算，递归检查操作数
            for (auto* op : inst->operands) {
                if (!analyzeIndexComplexity(op, outerIndVar, innerIndVar, outerCost, innerCost, isComplex, depth + 1)) {
                    return false;
                }
            }
            return true;
            
        case Opcode::SExt:
        case Opcode::ZExt:
        case Opcode::Trunc:
            // 类型转换，检查操作数
            if (inst->operands.size() > 0) {
                return analyzeIndexComplexity(inst->operands[0], outerIndVar, innerIndVar, outerCost, innerCost, isComplex, depth + 1);
            }
            return true;
            
        case Opcode::Load:
            // Load指令表示复杂的间接访问
            isComplex = true;
            return false;
            
        case Opcode::Call:
            // 函数调用表示复杂的计算
            isComplex = true;
            return false;
            
        default:
            // 其他指令认为是复杂的
            isComplex = true;
            return false;
    }
}

// 分析GEP指令的复杂度，计算对外层和内层归纳变量的依赖程度
static bool analyzeGEPComplexity(Instruction* gep, Value* outerIndVar, Value* innerIndVar, 
                                int& outerCost, int& innerCost, bool& isComplex) {
    if (!gep || gep->opcode != Opcode::GetElementPtr) return false;
    
    // 检查GEP的索引操作数（跳过第一个基指针）
    for (size_t i = 1; i < gep->operands.size(); ++i) {
        Value* idx = gep->operands[i];
        
        // 递归分析索引表达式的复杂度
        if (!analyzeIndexComplexity(idx, outerIndVar, innerIndVar, outerCost, innerCost, isComplex, 0)) {
            return false;
        }
        
        if (isComplex) {
            return false;
        }
    }
    
    return true;
}

// 检查一个值是否是常量1
static bool isConstantOne(Value* val) {
    if (auto* constInt = dynamic_cast<ConstantInt*>(val)) {
        return constInt->value == 1;
    }
    return false;
}

// 检查是否是相邻访问模式 (如 j 和 j+1)
static bool isAdjacentAccess(Value* idx1, Value* idx2, Value* indVar) {
    // 模式1: idx1 = indVar, idx2 = indVar + 1
    if (idx1 == indVar) {
        if (auto* inst = dynamic_cast<Instruction*>(idx2)) {
            if (inst->opcode == Opcode::Add && inst->operands.size() >= 2) {
                if ((inst->operands[0] == indVar && isConstantOne(inst->operands[1])) ||
                    (inst->operands[1] == indVar && isConstantOne(inst->operands[0]))) {
                    return true;
                }
            }
        }
    }
    
    // 模式2: idx2 = indVar, idx1 = indVar + 1
    if (idx2 == indVar) {
        if (auto* inst = dynamic_cast<Instruction*>(idx1)) {
            if (inst->opcode == Opcode::Add && inst->operands.size() >= 2) {
                if ((inst->operands[0] == indVar && isConstantOne(inst->operands[1])) ||
                    (inst->operands[1] == indVar && isConstantOne(inst->operands[0]))) {
                    return true;
                }
            }
        }
    }
    
    return false;
}

// 分析两个GEP指令的索引依赖关系
static bool analyzeIndexDependency(Instruction* gep1, Instruction* gep2, Value* outerIndVar, Value* innerIndVar) {
    // 简化分析：主要检查最后一个索引（通常是最重要的维度）
    if (gep1->operands.size() < 2 || gep2->operands.size() < 2) return false;
    
    Value* idx1 = gep1->operands[gep1->operands.size() - 1];
    Value* idx2 = gep2->operands[gep2->operands.size() - 1];
    
    // 检查是否是相邻访问模式，如 arr[j] 和 arr[j+1]
    if (isAdjacentAccess(idx1, idx2, innerIndVar)) {
        std::cout << "[LIC][ArrayDep] Adjacent access pattern detected: " 
                  << (idx1 ? idx1->toString() : "null") << " and " 
                  << (idx2 ? idx2->toString() : "null") << std::endl;
        return true; // 存在依赖
    }
    
    // 检查是否都依赖于内层归纳变量（同一迭代内的不同访问）
    bool idx1DependsInner = recursiveDepends(idx1, innerIndVar, 0);
    bool idx2DependsInner = recursiveDepends(idx2, innerIndVar, 0);
    
    if (idx1DependsInner && idx2DependsInner) {
        // 如果两个访问都依赖内层归纳变量，且在同一基本块内，可能存在依赖
        if (gep1->parent == gep2->parent) {
            std::cout << "[LIC][ArrayDep] Same-iteration dependency detected in block " 
                      << gep1->parent->label << std::endl;
            return true;
        }
    }
    
    return false;
}

// 检查两个内存访问指令之间是否存在依赖关系
static bool hasMemoryDependency(Instruction* inst1, Instruction* inst2, Value* outerIndVar, Value* innerIndVar) {
    Value* addr1 = nullptr;
    Value* addr2 = nullptr;
    
    // 获取内存地址
    if (inst1->opcode == Opcode::Load && inst1->operands.size() > 0) {
        addr1 = inst1->operands[0];
    } else if (inst1->opcode == Opcode::Store && inst1->operands.size() > 1) {
        addr1 = inst1->operands[1];
    }
    
    if (inst2->opcode == Opcode::Load && inst2->operands.size() > 0) {
        addr2 = inst2->operands[0];
    } else if (inst2->opcode == Opcode::Store && inst2->operands.size() > 1) {
        addr2 = inst2->operands[1];
    }
    
    if (!addr1 || !addr2) return false;
    
    // 检查是否都是GEP指令
    auto* gep1 = dynamic_cast<Instruction*>(addr1);
    auto* gep2 = dynamic_cast<Instruction*>(addr2);
    
    if (!gep1 || !gep2 || gep1->opcode != Opcode::GetElementPtr || gep2->opcode != Opcode::GetElementPtr) {
        return false;
    }
    
    // 检查基址是否相同
    if (gep1->operands.size() == 0 || gep2->operands.size() == 0) return false;
    Value* base1 = gep1->operands[0];
    Value* base2 = gep2->operands[0];
    
    if (base1 != base2) return false; // 不同数组，无依赖
    
    // 分析索引模式
    return analyzeIndexDependency(gep1, gep2, outerIndVar, innerIndVar);
}

// 检查数组依赖关系 - 检测不安全的访存模式
static bool checkArrayDependencies(Loop* outerLoop, Loop* innerLoop, Value* outerIndVar, Value* innerIndVar) {
    std::vector<Instruction*> loads, stores;
    
    // 收集所有的Load和Store指令
    auto allBlocks = outerLoop->getBlocks();
    for (BasicBlock* block : allBlocks) {
        for (auto& inst : block->instructions) {
            if (inst->opcode == Opcode::Load) {
                loads.push_back(inst.get());
            } else if (inst->opcode == Opcode::Store) {
                stores.push_back(inst.get());
            }
        }
    }
    
    // 检查Store-Store依赖关系
    for (size_t i = 0; i < stores.size(); ++i) {
        for (size_t j = i + 1; j < stores.size(); ++j) {
            if (hasMemoryDependency(stores[i], stores[j], outerIndVar, innerIndVar)) {
                std::cout << "[LIC][ArrayDep] Store-Store dependency detected: " 
                          << stores[i]->toString() << " <-> " << stores[j]->toString() << std::endl;
                return false;
            }
        }
    }
    
    // 检查Load-Store依赖关系  
    for (auto* load : loads) {
        for (auto* store : stores) {
            if (hasMemoryDependency(load, store, outerIndVar, innerIndVar)) {
                std::cout << "[LIC][ArrayDep] Load-Store dependency detected: " 
                          << load->toString() << " <-> " << store->toString() << std::endl;
                return false;
            }
        }
    }
    
    return true;
}

// DFS评估访存指令的成本 - 参考Java代码的dfsEval实现
static void dfsEval(Value* val, int cost, Loop* loop, Value* outerIndVar, Value* innerIndVar,
                   int& outerCost, int& innerCost, bool& isComplex,
                   const std::map<Value*, const SCEVLoopVariableInfo*>& scevInfo) {
    if (isComplex) return;
    
    // 检查值是否在循环内定义
    if (auto* inst = dynamic_cast<Instruction*>(val)) {
        if (!loop->contains(inst)) return; // 不在循环内定义
    } else {
        return; // 非指令值，如常量或参数
    }
    
    auto* inst = dynamic_cast<Instruction*>(val);
    if (!inst) return;
    
    if (inst->opcode == Opcode::Add) {
        // 加法指令，递归处理操作数
        if (inst->operands.size() >= 2) {
            dfsEval(inst->operands[0], cost, loop, outerIndVar, innerIndVar, outerCost, innerCost, isComplex, scevInfo);
            dfsEval(inst->operands[1], cost, loop, outerIndVar, innerIndVar, outerCost, innerCost, isComplex, scevInfo);
        }
    } else if (inst->opcode == Opcode::Mul) {
        // 乘法指令，增加成本后递归处理操作数
        if (inst->operands.size() >= 2) {
            dfsEval(inst->operands[0], cost + 1, loop, outerIndVar, innerIndVar, outerCost, innerCost, isComplex, scevInfo);
            dfsEval(inst->operands[1], cost + 1, loop, outerIndVar, innerIndVar, outerCost, innerCost, isComplex, scevInfo);
        }
    } else if (auto* phi = dynamic_cast<PhiInst*>(inst)) {
        // PHI指令，检查是否为归纳变量
        if (phi == outerIndVar) {
            outerCost += cost;
            std::cout << "[LIC][Cost] Outer cost += " << cost << " (total: " << outerCost << ")" << std::endl;
        } else if (phi == innerIndVar) {
            innerCost += cost;
            std::cout << "[LIC][Cost] Inner cost += " << cost << " (total: " << innerCost << ")" << std::endl;
        } else {
            // 检查是否为已知的SCEV变量
            auto it = scevInfo.find(phi);
            if (it == scevInfo.end()) {
                isComplex = true;
                std::cout << "[LIC][Cost] Complex: unknown PHI " << phi->toString() << std::endl;
            }
        }
    } else {
        // 其他指令类型认为是复杂的
        isComplex = true;
        std::cout << "[LIC][Cost] Complex: unsupported instruction " << inst->toString() << std::endl;
    }
}

// 判断一个loop除 header/latch/exiting/exit 之外是否只有 1 个 body 基本块
static BasicBlock* getSingleBodyBlock(Loop* L) {
    BasicBlock* body = nullptr;
    for (auto* bb : L->getBlocks()) {
        if (bb == L->getHeader()) continue;
        if (L->getLatches().count(bb)) continue;
        if (L->getExitingNodes().count(bb)) continue;
        if (L->getExitBlocks().count(bb)) continue;
        if (body && body != bb) return nullptr; // 多个 body
        body = bb;
    }
    return body; // 可能为 nullptr（空循环体）
}

bool LoopInterchangePass::isStructurallySimplePair(Loop* outer, Loop* inner) const {
    if (!outer || !inner) return false;
    // 直接父子
    if (inner->getParent() != outer) return false;
    // 结构：单 latch 单 exit 单 exiting 且 preheader 存在
    auto simple = [&](Loop* L){
        return L->getLatches().size()==1 && L->getExitBlocks().size()==1 && L->getExitingNodes().size()==1 && L->getPreheader()!=nullptr; };
    if (!simple(outer) || !simple(inner)) return false;
    // outer body 只有 inner 的块 + 头/尾 控制，不含其它 side-effect 指令
    // 粗略：除去 header/latch/exiting/exit/inner 所有块，剩余块不含 store/call
    for (auto* bb : outer->getBlocks()) {
        if (inner->getBlocks().count(bb)) continue; // 属于内层
        for (auto& inst : bb->instructions) {
            if (inst->opcode == Opcode::Call || inst->opcode == Opcode::Store) {
                return false;
            }
        }
    }
    std::cout << "[LIC] isStructurallySimplePair"<<std::endl;
    return true;
}

bool LoopInterchangePass::isSafeToInterchange(const InterchangeCandidateInfo& info) const {
    Loop* outerLoop = info.outerLoop;
    Loop* innerLoop = info.innerLoop;
    
    // 1. 基本结构检查
    // 只有一个子循环
    if (outerLoop->getChildren().size() != 1) {
        std::cout << "[LIC][Reject] Outer loop has " << outerLoop->getChildren().size() << " children, expected 1" << std::endl;
        return false;
    }
    
    // 单一退出块
    if (outerLoop->getExitBlocks().size() != 1 || innerLoop->getExitBlocks().size() != 1) {
        std::cout << "[LIC][Reject] Non-unique exit blocks" << std::endl;
        return false;
    }
    
    // 2. 退出条件检查
    BasicBlock* outerExit = *outerLoop->getExitBlocks().begin();
    BasicBlock* innerExit = *innerLoop->getExitBlocks().begin();
    
    // 检查外层循环的退出条件（应该从header直接退出）
    auto outerExitPreds = outerExit->predecessors;
    if (outerExitPreds.size() != 1 || outerExitPreds[0] != outerLoop->getHeader()) {
        std::cout << "[LIC][Reject] Complex outer loop exit condition" << std::endl;
        return false;
    }
    
    // 检查内层循环的退出条件（应该从header直接退出）
    auto innerExitPreds = innerExit->predecessors;
    if (innerExitPreds.size() != 1 || innerExitPreds[0] != innerLoop->getHeader()) {
        std::cout << "[LIC][Reject] Complex inner loop exit condition" << std::endl;
        return false;
    }
    
    // 3. 归纳变量和比较指令检查
    // 检查外层循环的终结指令
    Instruction* outerTerminator = outerLoop->getHeader()->getTerminator();
    if (!outerTerminator || outerTerminator->opcode != Opcode::Br) {
        std::cout << "[LIC][Reject] Invalid outer loop terminator" << std::endl;
        return false;
    }
    
    // 找到外层循环的比较指令
    Instruction* outerCmp = nullptr;
    if (outerTerminator->operands.size() > 0) {
        if (auto* cmp = dynamic_cast<Instruction*>(outerTerminator->operands[0])) {
            if (cmp->opcode == Opcode::ICmp) {
                outerCmp = cmp;
            }
        }
    }
    
    if (!outerCmp) {
        std::cout << "[LIC][Reject] Cannot find outer loop comparison instruction" << std::endl;
        return false;
    }
    
    // 检查比较指令是否使用了正确的归纳变量
    bool outerCmpUsesIndvar = false;
    for (auto* operand : outerCmp->operands) {
        if (operand == info.outerIndVar) {
            outerCmpUsesIndvar = true;
            break;
        }
    }
    
    if (!outerCmpUsesIndvar) {
        std::cout << "[LIC][Reject] Outer loop comparison doesn't use induction variable" << std::endl;
        return false;
    }
    
    // 检查内层循环的终结指令
    Instruction* innerTerminator = innerLoop->getHeader()->getTerminator();
    if (!innerTerminator || innerTerminator->opcode != Opcode::Br) {
        std::cout << "[LIC][Reject] Invalid inner loop terminator" << std::endl;
        return false;
    }
    
    // 找到内层循环的比较指令
    Instruction* innerCmp = nullptr;
    if (innerTerminator->operands.size() > 0) {
        if (auto* cmp = dynamic_cast<Instruction*>(innerTerminator->operands[0])) {
            if (cmp->opcode == Opcode::ICmp) {
                innerCmp = cmp;
            }
        }
    }
    
    if (!innerCmp) {
        std::cout << "[LIC][Reject] Cannot find inner loop comparison instruction" << std::endl;
        return false;
    }
    
    // 检查比较指令是否使用了正确的归纳变量
    bool innerCmpUsesIndvar = false;
    for (auto* operand : innerCmp->operands) {
        if (operand == info.innerIndVar) {
            innerCmpUsesIndvar = true;
            break;
        }
    }
    
    if (!innerCmpUsesIndvar) {
        std::cout << "[LIC][Reject] Inner loop comparison doesn't use induction variable" << std::endl;
        return false;
    }
    
    // 4. 检查归纳变量是否为PHI指令
    auto* outerPhi = dynamic_cast<PhiInst*>(info.outerIndVar);
    auto* innerPhi = dynamic_cast<PhiInst*>(info.innerIndVar);
    
    if (!outerPhi || !innerPhi) {
        std::cout << "[LIC][Reject] Induction variables are not PHI instructions" << std::endl;
        return false;
    }
    
    // // 5. 检查内层循环的退出块是否是外层循环的latch
    // BasicBlock* outerLatch = *outerLoop->getLatches().begin();
    // if (innerExit != outerLatch) {
    //     std::cout << "[LIC][Reject] Inner loop exit is not outer loop latch" << std::endl;
    //     return false;
    // }
    
    // 6. 检查内层循环的初始值是否依赖外层归纳变量
    for (auto& incoming : innerPhi->incoming_values) {
        if (incoming.second == innerLoop->getPreheader()) {
            if (recursiveDepends(incoming.first, info.outerIndVar, 0)) {
                std::cout << "[LIC][Reject] Inner loop initial value depends on outer induction variable: " 
                          << (incoming.first ? incoming.first->toString() : "null") << std::endl;
                return false;
            }
            break;
        }
    }
    
    // 7. 访存模式检查 - 检查是否有复杂的访存模式
    bool isComplex = false;
    int outerCost = 0;
    int innerCost = 0;
    
    // 检查所有循环块中的指令
    auto allBlocks = outerLoop->getBlocks();
    for (BasicBlock* block : allBlocks) {
        for (auto& inst : block->instructions) {
            if (inst->opcode == Opcode::Br) continue; // 跳过终结指令
            
            if (inst->opcode == Opcode::Load) {
                // 检查Load指令的地址
                if (inst->operands.size() > 0) {
                    Value* addr = inst->operands[0];
                    if (auto* gep = dynamic_cast<Instruction*>(addr)) {
                        if (gep->opcode == Opcode::GetElementPtr) {
                            // 分析GEP指令的复杂度
                            if (!analyzeGEPComplexity(gep, info.outerIndVar, info.innerIndVar, outerCost, innerCost, isComplex)) {
                                if (isComplex) {
                                    std::cout << "[LIC][Reject] Complex memory access pattern in load: " << inst->toString() << std::endl;
                                    return false;
                                }
                            }
                        }
                    }
                }
            } else if (inst->opcode == Opcode::Store) {
                // 检查Store指令的地址
                if (inst->operands.size() > 1) {
                    Value* addr = inst->operands[1];
                    if (auto* gep = dynamic_cast<Instruction*>(addr)) {
                        if (gep->opcode == Opcode::GetElementPtr) {
                            // 分析GEP指令的复杂度
                            if (!analyzeGEPComplexity(gep, info.outerIndVar, info.innerIndVar, outerCost, innerCost, isComplex)) {
                                if (isComplex) {
                                    std::cout << "[LIC][Reject] Complex memory access pattern in store: " << inst->toString() << std::endl;
                                    return false;
                                }
                            }
                        }
                    }
                }
            } else if (inst->opcode == Opcode::Call) {
                // 拒绝有副作用的指令
                std::cout << "[LIC][Reject] Call instruction found: " << inst->toString() << std::endl;
                return false;
            }
            // 可以添加其他有副作用指令的检查
        }
    }
    
    std::cout << "[LIC] Access analysis - outer_cost: " << outerCost << ", inner_cost: " << innerCost << std::endl;
    
    // 8. 关键：检查数组依赖关系 - 检测像 arr[j] 和 arr[j+1] 这样的相邻访问模式
    if (!checkArrayDependencies(outerLoop, innerLoop, info.outerIndVar, info.innerIndVar)) {
        std::cout << "[LIC][Reject] Unsafe array access dependencies detected" << std::endl;
        return false;
    }
    
    // 9. 检查完美嵌套结构
    // 外层循环的body entry应该只包含跳转到内层循环的指令
    BasicBlock* outerBodyEntry = nullptr;
    for (BasicBlock* block : outerLoop->getBlocks()) {
        if (block != outerLoop->getHeader() && 
            !outerLoop->getLatches().count(block) &&
            !outerLoop->getExitBlocks().count(block) &&
            !innerLoop->getBlocks().count(block)) {
            if (outerBodyEntry != nullptr) {
                std::cout << "[LIC][Reject] Multiple outer body blocks found" << std::endl;
                return false;
            }
            outerBodyEntry = block;
        }
    }
    
    // if (outerBodyEntry) {
    //     // 检查body entry是否只包含必要的指令
    //     for (auto& inst : outerBodyEntry->instructions) {
    //         if (inst->opcode == Opcode::Br) break; // 终结指令OK
    //         std::cout << "[LIC][Reject] Non-terminator instruction in outer body entry: " << inst->toString() << std::endl;
    //         return false;
    //     }
    // }
    
    // // 9. 检查内层循环的退出块（应该是外层循环的latch）
    // BasicBlock* innerExitBlock = *innerLoop->getExitBlocks().begin();
    // if (innerExitBlock != outerLatch) {
    //     std::cout << "[LIC][Reject] Inner exit block is not outer latch" << std::endl;
    //     return false;
    // }
    
    // // 检查外层latch只包含外层递增指令和终结指令
    // Instruction* outerInc = nullptr;
    // for (auto& incoming : outerPhi->incoming_values) {
    //     if (incoming.second == outerLatch) {
    //         outerInc = dynamic_cast<Instruction*>(incoming.first);
    //         break;
    //     }
    // }
    
    // if (outerInc) {
    //     for (auto& inst : innerExitBlock->instructions) {
    //         if (inst->opcode == Opcode::Add || inst->opcode == Opcode::Sub) {
    //             if (inst.get() != outerInc) {
    //                 std::cout << "[LIC][Reject] Unexpected arithmetic instruction in outer latch" << std::endl;
    //                 return false;
    //             }
    //             continue;
    //         }
    //         if (inst->opcode == Opcode::Br) {
    //             break; // 终结指令OK
    //         }
    //         std::cout << "[LIC][Reject] Unexpected instruction in outer latch: " << inst->toString() << std::endl;
    //         return false;
    //     }
    // }

    return true;
    
}

bool LoopInterchangePass::isProfitable(const InterchangeCandidateInfo& info) const {
    std::cout << "[LIC] Analyzing profitability for loop interchange..." << std::endl;
    
    Loop* outerLoop = info.outerLoop;
    Loop* innerLoop = info.innerLoop;
    Value* outerIndVar = info.outerIndVar;
    Value* innerIndVar = info.innerIndVar;
    
    // 收益分析：基于Java代码的完整实现
    bool isComplex = false;
    int outerCost = 0;
    int innerCost = 0;
    
    // 构建SCEV信息映射，用于dfsEval中的变量识别
    std::map<Value*, const SCEVLoopVariableInfo*> combinedScevInfo;
    
    // 获取外层循环的SCEV信息
    SCEVAnalysis analyzer(nullptr, &module);
    analyzer.analyzeLoop(outerLoop);
    auto& outerVars = analyzer.getLoopVariables(outerLoop);
    for (const auto& [var, info] : outerVars) {
        combinedScevInfo[var] = &info;
    }
    
    // 获取内层循环的SCEV信息
    analyzer.analyzeLoop(innerLoop);
    auto& innerVars = analyzer.getLoopVariables(innerLoop);
    for (const auto& [var, info] : innerVars) {
        combinedScevInfo[var] = &info;
    }
    
    // 分析所有循环块中的访存指令
    std::map<Value*, int> loadStoreMap; // 0=无访问, 1=只读, 2=只写, 3=读写
    auto allBlocks = outerLoop->getBlocks();
    
    for (BasicBlock* block : allBlocks) {
        for (auto& inst : block->instructions) {
            if (inst->opcode == Opcode::Br) continue; // 跳过终结指令
            
            if (inst->opcode == Opcode::Load) {
                // 处理Load指令
                if (inst->operands.size() > 0) {
                    Value* ptr = inst->operands[0];
                    
                    // 简化的基址分析（这里我们直接使用指针本身作为基址）
                    Value* baseAddr = ptr;
                    if (auto* gep = dynamic_cast<Instruction*>(ptr)) {
                        if (gep->opcode == Opcode::GetElementPtr && gep->operands.size() > 0) {
                            baseAddr = gep->operands[0]; // 使用GEP的第一个操作数作为基址
                        }
                    }
                    
                    loadStoreMap[baseAddr] |= 1; // 标记为读访问
                    
                    // 如果是GEP指令，分析其索引
                    if (auto* gep = dynamic_cast<Instruction*>(ptr)) {
                        if (gep->opcode == Opcode::GetElementPtr) {
                            // 分析GEP的索引操作数（跳过第一个基指针）
                            for (size_t i = 1; i < gep->operands.size() && !isComplex; ++i) {
                                dfsEval(gep->operands[i], 0, outerLoop, outerIndVar, innerIndVar, 
                                       outerCost, innerCost, isComplex, combinedScevInfo);
                            }
                        }
                    }
                    
                    if (isComplex) {
                        std::cout << "[LIC][Reject] Complex load instruction: " << inst->toString() << std::endl;
                        return false;
                    }
                }
            } else if (inst->opcode == Opcode::Store) {
                // 处理Store指令
                if (inst->operands.size() > 1) {
                    Value* ptr = inst->operands[1]; // Store的第二个操作数是地址
                    
                    // 简化的基址分析
                    Value* baseAddr = ptr;
                    if (auto* gep = dynamic_cast<Instruction*>(ptr)) {
                        if (gep->opcode == Opcode::GetElementPtr && gep->operands.size() > 0) {
                            baseAddr = gep->operands[0];
                        }
                    }
                    
                    loadStoreMap[baseAddr] |= 2; // 标记为写访问
                    
                    // 如果是GEP指令，分析其索引
                    if (auto* gep = dynamic_cast<Instruction*>(ptr)) {
                        if (gep->opcode == Opcode::GetElementPtr) {
                            for (size_t i = 1; i < gep->operands.size() && !isComplex; ++i) {
                                dfsEval(gep->operands[i], 0, outerLoop, outerIndVar, innerIndVar, 
                                       outerCost, innerCost, isComplex, combinedScevInfo);
                            }
                        }
                    }
                    
                    if (isComplex) {
                        std::cout << "[LIC][Reject] Complex store instruction: " << inst->toString() << std::endl;
                        return false;
                    }
                }
            } else {
                // 检查其他指令是否有副作用
                if (inst->opcode == Opcode::Call) {
                    std::cout << "[LIC][Reject] Call instruction with side effects: " << inst->toString() << std::endl;
                    return false;
                }
                // 可以添加更多有副作用指令的检查
                // 目前简化为只检查Call指令
            }
        }
    }
    
    std::cout << "[LIC] Cost analysis - outer_cost: " << outerCost << ", inner_cost: " << innerCost << std::endl;
    
    // 关键决策：只有当inner_cost > outer_cost时才认为交换有收益
    if (innerCost <= outerCost) {
        std::cout << "[LIC][Reject] No profitability: inner_cost (" << innerCost 
                  << ") <= outer_cost (" << outerCost << ")" << std::endl;
        return false;
    }
    
    // 可选：读写冲突检查（Java代码中被注释掉的部分）
    // 这里我们跳过这个检查，因为原始Java代码也将其注释掉了
    
    std::cout << "[LIC] Profitable interchange: inner_cost (" << innerCost 
              << ") > outer_cost (" << outerCost << ")" << std::endl;
    return true;
}

void LoopInterchangePass::collectCandidates(Function& F, LoopAnalysis& LA, SCEVAnalysis& analyzer, std::vector<InterchangeCandidateInfo>& out) {
    std::cout << "[LIC] collectCandidates" << std::endl;
    for (auto& loopPtr : LA.loops) {
        Loop* outer = loopPtr.get();
        if (!outer || outer->getChildren().size()!=1) continue; // 仅考虑恰好一个子循环
        if (outer->getParent()) continue; // TODO, 嵌套循环暂时不处理
        Loop* inner = outer->getChildren()[0];
        if (!isStructurallySimplePair(outer, inner)) continue;
        // 为每个循环运行单独 SCEV (重用同一分析器递归已分析, 这里直接查询)
        // SCEVAnalysis 在构造时 analyzeLoopRecursively 已记录所有loop变量信息
        // SCEVAnalysis analyzer(&F, &module);
        // analyzer.analyzeLoop(outer); // 单独分析 outer, 内部也会找到 inner 变量（保守）
        Value* outerPrimary = analyzer.getPrimaryLoopVariable(outer);
        Value* innerPrimary = analyzer.getPrimaryLoopVariable(inner);
        // if (outerPrimary) {
        //     // 针对 inner 重新分析
        //     analyzer.analyzeLoop(inner);
        //     innerPrimary = analyzer.getPrimaryLoopVariable(inner);
        // }
        if (!outerPrimary || !innerPrimary) continue;
        auto& outerVars = analyzer.getLoopVariables(outer);
        auto& innerVars = analyzer.getLoopVariables(inner);
        auto oit = outerVars.find(outerPrimary);
        auto iit = innerVars.find(innerPrimary);
        if (oit==outerVars.end() || iit==innerVars.end()) {std::cout << "[LIC] oit==outerVars.end() || iit==innerVars.end()" <<std::endl; continue;}
        InterchangeCandidateInfo info;
        info.outerLoop = outer; info.innerLoop = inner;
        info.outerIndVar = outerPrimary; info.innerIndVar = innerPrimary;
        info.outerVarInfo = const_cast<SCEVLoopVariableInfo*>(&oit->second);
        info.innerVarInfo = const_cast<SCEVLoopVariableInfo*>(&iit->second);
        info.safe = isSafeToInterchange(info);
        if (!info.safe) continue;
        info.profitable = isProfitable(info);
        out.push_back(info);
    }
}

bool LoopInterchangePass::transformNestedLoops(InterchangeCandidateInfo& info) {
    std::cout << "[LIC] Candidate (outer=" << info.outerLoop->getHeader()->label
              << ", inner=" << info.innerLoop->getHeader()->label << ") is safe=" << info.safe
              << ", profitable=" << info.profitable << std::endl;
    
    if (!info.safe || !info.profitable) { //|| !info.profitable
        std::cout << "[LIC][Reject] Candidate is not safe or profitable" << std::endl;
        return false;
    }
    
    Loop* outerLoop = info.outerLoop;
    Loop* innerLoop = info.innerLoop;
    
    std::cout << "[LIC] Transforming loops: outer=" << outerLoop->getHeader()->label 
              << ", inner=" << innerLoop->getHeader()->label << std::endl;
    
    // 获取循环的关键组件
    BasicBlock* outerHeader = outerLoop->getHeader();
    BasicBlock* innerHeader = innerLoop->getHeader();
    BasicBlock* outerLatch = *outerLoop->getLatches().begin(); // 已验证只有一个latch
    BasicBlock* innerLatch = *innerLoop->getLatches().begin();
    BasicBlock* outerPreheader = outerLoop->getPreheader();
    BasicBlock* innerPreheader = innerLoop->getPreheader();
    
    // 找到归纳变量对应的PHI指令
    PhiInst* outerPhi = nullptr;
    PhiInst* innerPhi = nullptr;
    
    // 在header中找到对应的PHI指令
    std::cout << "[LIC] info.outerIndVar: "<< info.outerIndVar->toString() << std::endl;
    std::cout << "[LIC] info.innerIndVar: "<< info.innerIndVar->toString() << std::endl;
    for (auto& phi : outerHeader->phi_instructions) {
        if (phi.get() == info.outerIndVar) {
            outerPhi = phi.get();
            break;
        }
    }
    
    for (auto& phi : innerHeader->phi_instructions) {
        if (phi.get() == info.innerIndVar) {
            innerPhi = phi.get();
            break;
        }
    }
    
    if (!outerPhi || !innerPhi) {
        std::cout << "[LIC] Error: Could not find PHI instructions for induction variables" << std::endl;
        return false;
    }
    
    // 找到递增指令
    Instruction* outerInc = nullptr;
    Instruction* innerInc = nullptr;
    
    // 查找外层循环的递增指令
    // 递增指令应该是PHI指令的回边值来源
    for (auto& incoming : outerPhi->incoming_values) {
        if (incoming.second != outerPreheader) { // 回边值（非初始值）
            if (auto* inst = dynamic_cast<Instruction*>(incoming.first)) {
                if ((inst->opcode == Opcode::Add || inst->opcode == Opcode::Sub) &&
                    inst->operands.size() >= 2) {
                    // 检查是否有一个操作数是PHI本身
                    if (inst->operands[0] == outerPhi || inst->operands[1] == outerPhi) {
                        outerInc = inst;
                        break;
                    }
                }
            }
        }
    }
    
    // 查找内层循环的递增指令
    for (auto& incoming : innerPhi->incoming_values) {
        if (incoming.second != innerPreheader) { // 回边值（非初始值）
            if (auto* inst = dynamic_cast<Instruction*>(incoming.first)) {
                if ((inst->opcode == Opcode::Add || inst->opcode == Opcode::Sub) &&
                    inst->operands.size() >= 2) {
                    // 检查是否有一个操作数是PHI本身
                    if (inst->operands[0] == innerPhi || inst->operands[1] == innerPhi) {
                        innerInc = inst;
                        break;
                    }
                }
            }
        }
    }
    
    if (!outerInc || !innerInc) {
        std::cout << "[LIC] Error: Could not find increment instructions" << std::endl;
        return false;
    }
    std::cout << "[LIC] outerInc: " << outerInc->toString() << std::endl;
    std::cout << "[LIC] innerInc: " << innerInc->toString() << std::endl;
    
    // 找到比较指令
    Instruction* outerCmp = nullptr;
    Instruction* innerCmp = nullptr;
    
    // 找到所有相关的边界计算指令
    std::vector<Instruction*> outerBoundInsts;
    std::vector<Instruction*> innerBoundInsts;
    
    // 查找外层循环的比较指令及其依赖的边界计算
    for (auto& inst : outerHeader->instructions) {
        if (inst->opcode == Opcode::ICmp) {
            // 检查比较指令是否直接使用了外层归纳变量
            bool usesOuterPhi = false;
            for (auto* operand : inst->operands) {
                if (operand == outerPhi) {
                    usesOuterPhi = true;
                    // 收集该比较指令依赖的所有指令
                    for (auto* op : inst->operands) {
                        if (auto* opInst = dynamic_cast<Instruction*>(op)) {
                            if (opInst != outerPhi && opInst->parent == outerHeader) {
                                outerBoundInsts.push_back(opInst);
                            }
                        }
                    }
                    break;
                }
            }
            if (usesOuterPhi) {
                outerCmp = inst.get();
                break;
            }
        }
    }
    
    // 查找内层循环的比较指令及其依赖的边界计算
    for (auto& inst : innerHeader->instructions) {
        if (inst->opcode == Opcode::ICmp) {
            // 检查比较指令是否直接使用了内层归纳变量
            bool usesInnerPhi = false;
            for (auto* operand : inst->operands) {
                if (operand == innerPhi) {
                    usesInnerPhi = true;
                    // 收集该比较指令依赖的所有指令
                    for (auto* op : inst->operands) {
                        if (auto* opInst = dynamic_cast<Instruction*>(op)) {
                            if (opInst != innerPhi && opInst->parent == innerHeader) {
                                innerBoundInsts.push_back(opInst);
                            }
                        }
                    }
                    break;
                }
            }
            if (usesInnerPhi) {
                innerCmp = inst.get();
                break;
            }
        }
    }
    
    if (!outerCmp || !innerCmp) {
        std::cout << "[LIC] Error: Could not find comparison instructions" << std::endl;
        return false;
    }
    
    // 开始交换操作
    std::cout << "[LIC] Starting loop interchange transformation..." << std::endl;
    
    // 1. 交换PHI指令的位置
    // 先从原位置移除PHI指令的所有权，但保留指令对象
    std::unique_ptr<PhiInst> outerPhiPtr = nullptr;
    std::unique_ptr<PhiInst> innerPhiPtr = nullptr;
    
    // 从外层header移除外层PHI
    auto& outerPhiInsts = outerHeader->phi_instructions;
    for (auto it = outerPhiInsts.begin(); it != outerPhiInsts.end(); ++it) {
        if (it->get() == outerPhi) {
            outerPhiPtr = std::move(*it);
            outerPhiInsts.erase(it);
            break;
        }
    }
    
    // 从内层header移除内层PHI
    auto& innerPhiInsts = innerHeader->phi_instructions;
    for (auto it = innerPhiInsts.begin(); it != innerPhiInsts.end(); ++it) {
        if (it->get() == innerPhi) {
            innerPhiPtr = std::move(*it);
            innerPhiInsts.erase(it);
            break;
        }
    }
    
    if (!outerPhiPtr || !innerPhiPtr) {
        std::cout << "[LIC] Error: Failed to extract PHI instructions" << std::endl;
        return false;
    }
    
    // 将外层PHI移动到内层header
    outerPhiPtr->parent = innerHeader;
    innerHeader->phi_instructions.insert(innerHeader->phi_instructions.begin(), std::move(outerPhiPtr));
    
    // 将内层PHI移动到外层header
    innerPhiPtr->parent = outerHeader;
    outerHeader->phi_instructions.insert(outerHeader->phi_instructions.begin(), std::move(innerPhiPtr));
    
    // 2. 更新PHI指令的incoming值
    // 更新外层PHI（现在在内层header中）
    for (auto& incoming : outerPhi->incoming_values) {
        if (incoming.second == outerPreheader) {
            incoming.second = innerPreheader;
        } else if (incoming.second == outerLatch) {
            incoming.second = innerLatch;
        }
    }
    
    // 更新内层PHI（现在在外层header中）
    for (auto& incoming : innerPhi->incoming_values) {
        if (incoming.second == innerPreheader) {
            incoming.second = outerPreheader;
        } else if (incoming.second == innerLatch) {
            incoming.second = outerLatch;
        }
    }
    
    // 3. 移动递增指令
    // 先从原位置移除递增指令的所有权，但保留指令对象
    std::unique_ptr<Instruction> outerIncPtr = nullptr;
    std::unique_ptr<Instruction> innerIncPtr = nullptr;
    
    // 找到并移除外层递增指令
    BasicBlock* outerIncParent = outerInc->parent;
    auto& outerIncInsts = outerIncParent->instructions;
    for (auto it = outerIncInsts.begin(); it != outerIncInsts.end(); ++it) {
        if (it->get() == outerInc) {
            outerIncPtr = std::move(*it);
            outerIncInsts.erase(it);
            break;
        }
    }
    
    // 找到并移除内层递增指令
    BasicBlock* innerIncParent = innerInc->parent;
    auto& innerIncInsts = innerIncParent->instructions;
    for (auto it = innerIncInsts.begin(); it != innerIncInsts.end(); ++it) {
        if (it->get() == innerInc) {
            innerIncPtr = std::move(*it);
            innerIncInsts.erase(it);
            break;
        }
    }
    
    if (!outerIncPtr || !innerIncPtr) {
        std::cout << "[LIC] Error: Failed to extract increment instructions" << std::endl;
        return false;
    }
    std::cout << "[LIC] outerIncPtr: " << outerIncPtr->toString() << std::endl;
    std::cout << "[LIC] innerIncPtr: " << innerIncPtr->toString() << std::endl;
    
    // 将外层递增指令移到内层latch
    // 将外层递增指令插入到内层latch的最后一条指令之前
    outerIncPtr->parent = innerLatch;
    if (!innerLatch->instructions.empty()) {
        auto it = innerLatch->instructions.end();
        --it;
        innerLatch->instructions.insert(it, std::move(outerIncPtr));
    } else {
        innerLatch->instructions.push_back(std::move(outerIncPtr));
    }

    // 将内层递增指令插入到外层latch的最后一条指令之前
    innerIncPtr->parent = outerLatch;
    if (!outerLatch->instructions.empty()) {
        auto it = outerLatch->instructions.end();
        --it;
        outerLatch->instructions.insert(it, std::move(innerIncPtr));
    } else {
        outerLatch->instructions.push_back(std::move(innerIncPtr));
    }
    
    // 更新递增指令的操作数引用（关键修复）
    // 外层递增指令（现在在内层latch中）应该引用外层PHI（现在在内层header中）
    outerInc->replaceOperand(outerPhi, outerPhi); // 这个引用是正确的，因为PHI已经移动
    // 内层递增指令（现在在外层latch中）应该引用内层PHI（现在在外层header中）
    innerInc->replaceOperand(innerPhi, innerPhi); // 这个引用是正确的，因为PHI已经移动
    
    // 但是还需要更新PHI指令的incoming值中对递增指令的引用
    // 外层PHI（现在在内层header中）的回边值应该来自外层递增指令（现在在内层latch中）
    for (auto& incoming : outerPhi->incoming_values) {
        if (incoming.first == outerInc) {
            // 引用已经正确，无需更改
            break;
        }
    }
    
    // 内层PHI（现在在外层header中）的回边值应该来自内层递增指令（现在在外层latch中）
    for (auto& incoming : innerPhi->incoming_values) {
        if (incoming.first == innerInc) {
            // 引用已经正确，无需更改
            break;
        }
    }
    
    std::cout << "[LIC] Updated increment instruction operands" << std::endl;
    
    // 4. 移动比较指令和边界计算指令
    std::unique_ptr<Instruction> outerCmpPtr = nullptr;
    std::unique_ptr<Instruction> innerCmpPtr = nullptr;
    std::vector<std::unique_ptr<Instruction>> outerBoundPtrs;
    std::vector<std::unique_ptr<Instruction>> innerBoundPtrs;
    
    // 移动外层边界计算指令
    for (auto* boundInst : outerBoundInsts) {
        auto& outerHeaderInsts = outerHeader->instructions;
        for (auto it = outerHeaderInsts.begin(); it != outerHeaderInsts.end(); ++it) {
            if (it->get() == boundInst) {
                outerBoundPtrs.push_back(std::move(*it));
                outerHeaderInsts.erase(it);
                break;
            }
        }
    }
    
    // 移动内层边界计算指令
    for (auto* boundInst : innerBoundInsts) {
        auto& innerHeaderInsts = innerHeader->instructions;
        for (auto it = innerHeaderInsts.begin(); it != innerHeaderInsts.end(); ++it) {
            if (it->get() == boundInst) {
                innerBoundPtrs.push_back(std::move(*it));
                innerHeaderInsts.erase(it);
                break;
            }
        }
    }
    
    if (outerCmp) {
        // 找到并移除外层比较指令
        auto& outerHeaderInsts = outerHeader->instructions;
        for (auto it = outerHeaderInsts.begin(); it != outerHeaderInsts.end(); ++it) {
            if (it->get() == outerCmp) {
                outerCmpPtr = std::move(*it);
                outerHeaderInsts.erase(it);
                break;
            }
        }
    }
    
    if (innerCmp) {
        // 找到并移除内层比较指令
        auto& innerHeaderInsts = innerHeader->instructions;
        for (auto it = innerHeaderInsts.begin(); it != innerHeaderInsts.end(); ++it) {
            if (it->get() == innerCmp) {
                innerCmpPtr = std::move(*it);
                innerHeaderInsts.erase(it);
                break;
            }
        }
    }
    
    // 移动边界计算指令到交换后的位置
    // 外层边界计算指令移动到内层header
    for (auto& boundPtr : outerBoundPtrs) {
        boundPtr->parent = innerHeader;
        // 插入到PHI指令之后，比较指令之前
        if (!innerHeader->instructions.empty()) {
            auto it = innerHeader->instructions.begin();
            innerHeader->instructions.insert(it, std::move(boundPtr));
        } else {
            innerHeader->instructions.push_back(std::move(boundPtr));
        }
    }
    
    // 内层边界计算指令移动到外层header
    for (auto& boundPtr : innerBoundPtrs) {
        boundPtr->parent = outerHeader;
        // 插入到PHI指令之后，比较指令之前
        if (!outerHeader->instructions.empty()) {
            auto it = outerHeader->instructions.begin();
            outerHeader->instructions.insert(it, std::move(boundPtr));
        } else {
            outerHeader->instructions.push_back(std::move(boundPtr));
        }
    }
    
    // 只有当两个比较指令都存在时才进行交换
    if (outerCmpPtr && innerCmpPtr) {
        // 将外层比较指令移到内层header的最后一条指令之前
        outerCmpPtr->parent = innerHeader;
        if (!innerHeader->instructions.empty()) {
            auto it = innerHeader->instructions.end();
            --it;
            innerHeader->instructions.insert(it, std::move(outerCmpPtr));
        } else {
            innerHeader->instructions.push_back(std::move(outerCmpPtr));
        }

        // 将内层比较指令移到外层header的最后一条指令之前
        innerCmpPtr->parent = outerHeader;
        if (!outerHeader->instructions.empty()) {
            auto it = outerHeader->instructions.end();
            --it;
            outerHeader->instructions.insert(it, std::move(innerCmpPtr));
        } else {
            outerHeader->instructions.push_back(std::move(innerCmpPtr));
        }
        
        // 更新分支指令中的操作数引用
        outerHeader->getTerminator()->replaceOperand(outerCmp, innerCmp);
        innerHeader->getTerminator()->replaceOperand(innerCmp, outerCmp);
        
        // 更新本地指针引用
        outerCmp = outerHeader->instructions.back().get();
        innerCmp = innerHeader->instructions.back().get();
        
        std::cout << "[LIC] Successfully exchanged comparison instructions and boundary calculations" << std::endl;
    } else {
        std::cout << "[LIC] Warning: Could not exchange comparison instructions - some may depend on non-IV values" << std::endl;
        // 如果比较指令交换失败，这可能意味着循环边界条件复杂，应该放弃交换
        if (!outerCmp || !innerCmp) {
            std::cout << "[LIC] Error: Missing comparison instructions, aborting interchange" << std::endl;
            return false;
        }
    }
    
    outerHeader->comments.push_back("LIC");
    
    // 6. 重要：不要交换Loop对象的header，因为这会破坏循环嵌套关系
    // 循环交换的本质是交换循环体内的归纳变量和控制流，而不是交换Loop对象本身
    // std::swap(outerLoop->header, innerLoop->header); // 移除这行
    
    // 重新构建Use-Def链
    if (outerHeader->parent && outerHeader->parent->name != "") {
        cfg::rebuildUseDefChainsOnFunction(*outerHeader->parent, false);
    }
    
    std::cout << "[LIC] Loop interchange completed successfully!" << std::endl;
    return true;
}

// === 多层循环处理 ===
// 评估一个循环在其子树（排除更内层循环）中的“地址相关度”作为启发式：统计该层 header/其直接 body 中的访存指令对该层 induction var 的依赖次数。
static int addressAffinity(Loop* L, Value* indVar) {
    if (!L || !indVar) return 0;
    int score = 0;
    for (auto* bb : L->getBlocks()) {
        // 仅统计属于当前层但不属于任何直接子循环的 basicblock
        bool inChild = false;
        for (auto* child : L->getChildren()) {
            if (child->getBlocks().count(bb)) { inChild = true; break; }
        }
        if (inChild) continue;
        for (auto& inst : bb->instructions) {
            if (inst->opcode==Opcode::Load || inst->opcode==Opcode::Store || inst->opcode==Opcode::GetElementPtr) {
                for (auto* op : inst->operands) {
                    if (recursiveDepends(op, indVar, 0)) score++;
                }
            }
        }
    }
    return score;
}

// 收集从最外层开始的完美嵌套（连续每层只有一个子循环）
static std::vector<Loop*> collectPerfectNest(Loop* outerMost) {
    std::vector<Loop*> nest;   
    Loop* cur = outerMost;
    while (cur) {
        nest.push_back(cur);
        if (cur->getChildren().size() != 1) break;
        cur = cur->getChildren()[0];
    }
    std::cout << "[LIC] Perfectly nested loops collected: " << nest.size() << std::endl;
    return nest;
}
 
// 尝试对 perfect nest 进行基于启发式的重排（泡排序式相邻交换），使 addressAffinity 高的放最内层
// 判断内层循环的边界（上界或退出条件涉及变量）是否依赖外层 induction variable，若依赖则该内层不能提升到外层（会改变迭代域，如 j<i 的三角形）。
static bool boundDependsOnOuter(SCEVLoopVariableInfo* innerInfo, Value* outerIndVar) {
    if (!innerInfo || !outerIndVar) return false;
    auto dep = [&](Value* v){ return recursiveDepends(v, outerIndVar, 0); };
    if (dep(innerInfo->upperBound) || dep(innerInfo->lowerBound)) return true;
    // 也可能 exit icmp 中直接引用 outerIndVar（已包含在 upper/lowerBound 中，保守再判）
    return false;
}

// 检查循环边界是否包含复杂的依赖关系（如数组访问）
static bool hasComplexBoundDependency(Loop* loop, Value* indVar) {
    if (!loop || !indVar) return false;
    
    // 检查循环header中的比较指令
    for (auto& inst : loop->getHeader()->instructions) {
        if (inst->opcode == Opcode::ICmp) {
            for (auto* operand : inst->operands) {
                if (operand == indVar) {
                    // 找到使用归纳变量的比较指令，检查另一个操作数
                    for (auto* otherOperand : inst->operands) {
                        if (otherOperand != indVar) {
                            // 检查是否包含Load指令（可能是数组访问）
                            if (auto* otherInst = dynamic_cast<Instruction*>(otherOperand)) {
                                if (otherInst->opcode == Opcode::Load) {
                                    std::cout << "[LIC][ComplexBound] Found load in loop bound: " << otherInst->toString() << std::endl;
                                    return true;
                                }
                                // 检查是否包含GEP指令（数组索引计算）
                                if (otherInst->opcode == Opcode::GetElementPtr) {
                                    std::cout << "[LIC][ComplexBound] Found GEP in loop bound: " << otherInst->toString() << std::endl;
                                    return true;
                                }
                                // 递归检查复杂表达式
                                std::function<bool(Value*)> checkComplexity = [&](Value* val) -> bool {
                                    if (auto* checkInst = dynamic_cast<Instruction*>(val)) {
                                        if (checkInst->opcode == Opcode::Load || 
                                            checkInst->opcode == Opcode::GetElementPtr ||
                                            checkInst->opcode == Opcode::Call) {
                                            return true;
                                        }
                                        // 检查操作数
                                        for (auto* op : checkInst->operands) {
                                            if (checkComplexity(op)) return true;
                                        }
                                    }
                                    return false;
                                };
                                if (checkComplexity(otherOperand)) {
                                    std::cout << "[LIC][ComplexBound] Found complex dependency in loop bound" << std::endl;
                                    return true;
                                }
                            }
                        }
                    }
                    break;
                }
            }
        }
    }
    return false;
}

// 增强的边界依赖检查
static bool enhancedBoundDependsOnOuter(SCEVLoopVariableInfo* innerInfo, Value* outerIndVar, Loop* innerLoop, Loop* outerLoop) {
    if (!innerInfo || !outerIndVar || !innerLoop || !outerLoop) return false;
    
    // 1. 原有的SCEV信息检查
    auto dep = [&](Value* v){ return recursiveDepends(v, outerIndVar, 0); };
    if (dep(innerInfo->upperBound) || dep(innerInfo->lowerBound)) {
        std::cout << "[LIC][BoundDep] Inner loop bound depends on outer IV via SCEV" << std::endl;
        return true;
    }
    
    // 2. 检查内层循环的比较指令是否直接或间接依赖外层归纳变量
    for (auto& inst : innerLoop->getHeader()->instructions) {
        if (inst->opcode == Opcode::ICmp) {
            for (auto* operand : inst->operands) {
                if (recursiveDepends(operand, outerIndVar, 0)) {
                    std::cout << "[LIC][BoundDep] Inner loop comparison depends on outer IV: " << inst->toString() << std::endl;
                    return true;
                }
            }
        }
    }
    
    // 3. 检查是否有复杂的边界条件
    if (hasComplexBoundDependency(innerLoop, innerInfo->variable)) {
        std::cout << "[LIC][BoundDep] Inner loop has complex bound dependency" << std::endl;
        return true;
    }
    
    return false;
}

static bool performMultiLevelInterchange(Function& F, std::vector<Loop*>& nest, SCEVAnalysis& analyzer, LoopInterchangePass* pass) {
    if (nest.size() < 2) return false;
    std::cout << "[LIC] Performing multi-level interchange on " << nest.size() << " nested loops." << std::endl;
    
    // 打印嵌套结构以便调试
    for (size_t i = 0; i < nest.size(); ++i) {
        std::cout << "[LIC] Level " << i << ": " << nest[i]->getHeader()->label << std::endl;
    }
    
    bool changed = false;
    // 预先分析所有层的主变量
    std::vector<Value*> ivs;
    ivs.reserve(nest.size());
    std::vector<SCEVLoopVariableInfo*> infos;
    for (auto* L : nest) {
        Value* iv = analyzer.getPrimaryLoopVariable(L); // 使用全函数递归分析结果
        ivs.push_back(iv);
        SCEVLoopVariableInfo* vi = nullptr;
        if (iv) {
            auto& mp = analyzer.getLoopVariables(L);
            auto it = mp.find(iv);
            if (it!=mp.end()) vi = const_cast<SCEVLoopVariableInfo*>(&it->second);
        }
        infos.push_back(vi);
    }
    // 计算初始得分
    auto getScore = [&](int idx){ return addressAffinity(nest[idx], ivs[idx]); };
    
    // 只进行一轮交换，避免重复
    std::cout << "[LIC] Analyzing loop interchange..." << std::endl;
    for (size_t i=0;i+1<nest.size();++i) {
        int sA = getScore(i);
        int sB = getScore(i+1);
        std::cout << "[LIC] Comparing loops: " << nest[i]->getHeader()->label
                  << " (score=" << sA << ") <-> " << nest[i+1]->getHeader()->label
                  << " (score=" << sB << ")" << std::endl;
        if (sA < sB) {
            // 执行相邻交换 i 与 i+1
            InterchangeCandidateInfo info;
            info.outerLoop = nest[i];
            info.innerLoop = nest[i+1];
            info.outerIndVar = analyzer.getPrimaryLoopVariable(info.outerLoop);
            info.innerIndVar = analyzer.getPrimaryLoopVariable(info.innerLoop);
            if (!info.outerIndVar || !info.innerIndVar) {
                std::cout << "[LIC][Reject] Unable to find induction variables: "
                          << info.outerLoop->getHeader()->label << " -> " << info.innerLoop->getHeader()->label << std::endl;
                continue;
            } // 无法交换
            auto& outerVars = analyzer.getLoopVariables(info.outerLoop);
            auto& innerVars = analyzer.getLoopVariables(info.innerLoop);
            auto oit = outerVars.find(info.outerIndVar);
            auto iit = innerVars.find(info.innerIndVar);
            if (oit==outerVars.end() || iit==innerVars.end()) {
                std::cout << "[LIC][Reject] Unable to find induction variables: "
                          << info.outerLoop->getHeader()->label << " -> " << info.innerLoop->getHeader()->label << std::endl;
                continue;
            }
            info.outerVarInfo = const_cast<SCEVLoopVariableInfo*>(&oit->second);
            info.innerVarInfo = const_cast<SCEVLoopVariableInfo*>(&iit->second);
            // 附加：若 inner 的边界依赖 outer induction variable，则禁止提升（保持迭代域）
            if (enhancedBoundDependsOnOuter(info.innerVarInfo, info.outerIndVar, info.innerLoop, info.outerLoop)) {
                std::cout << "[LIC][Reject] inner loop bound depends on outer IV: "
                          << info.outerLoop->getHeader()->label << " -> " << info.innerLoop->getHeader()->label << std::endl;
                continue;
            }
            info.safe = pass->isSafeToInterchange(info) && pass->isStructurallySimplePair(info.outerLoop, info.innerLoop);
            if (!info.safe) {
                std::cout << "[LIC][Reject] unsafe interchange candidate: "
                          << info.outerLoop->getHeader()->label << " -> " << info.innerLoop->getHeader()->label << std::endl;
                continue;
            }
            if (pass->transformNestedLoops(info)) {
                std::cout << "[LIC] Successfully interchanged loops " << nest[i]->getHeader()->label 
                          << " and " << nest[i+1]->getHeader()->label << std::endl;
                std::swap(nest[i], nest[i+1]);
                std::swap(ivs[i], ivs[i+1]);
                std::swap(infos[i], infos[i+1]);
                changed = true;
                break; // 只进行一次交换
            }
        } else {
            std::cout << "[LIC][Reject] No interchange performed (score not beneficial)" << std::endl;
        }
    }
    
    if (changed) {
        // 重建 use-def
        cfg::rebuildUseDefChainsOnFunction(F, false);
    }
    return changed;
}

bool LoopInterchangePass::runOnFunction(Function& F) {
    std::cout << "[LIC] Running on function: " << F.name << std::endl;
    // 构建支配树与循环分析
    cfg::DominatorTree DT; DT.runOnFunction(F);
    LoopAnalysis LA; LA.runOnFunction(F, DT); LA.normalizeLoops(F); LA.runLCSSA(F);
    // 使用全函数递归 SCEV 分析（不会在后续相邻交换中被清空）
    SCEVAnalysis SCEVA = analyzeSCEV(F, module, LA);

    bool changed = false;
    
    // 首先使用collectCandidates收集所有潜在的交换候选
    std::vector<InterchangeCandidateInfo> candidates;
    collectCandidates(F, LA, SCEVA, candidates);
    
    std::cout << "[LIC] Found " << candidates.size() << " interchange candidates" << std::endl;
    
    // 对每个候选进行交换
    for (auto& candidate : candidates) {
        if (candidate.safe && candidate.profitable) {
            std::cout << "[LIC] Attempting to transform candidate: outer=" 
                      << candidate.outerLoop->getHeader()->label 
                      << ", inner=" << candidate.innerLoop->getHeader()->label << std::endl;
            
            if (transformNestedLoops(candidate)) {
                changed = true;
                std::cout << "[LIC] Successfully transformed loops" << std::endl;
                // 只进行一次交换后重新分析
                break;
            }
        } else {
            std::cout << "[LIC] Skipping candidate: outer=" 
                      << candidate.outerLoop->getHeader()->label 
                      << ", inner=" << candidate.innerLoop->getHeader()->label
                      << " (safe=" << candidate.safe << ", profitable=" << candidate.profitable << ")" << std::endl;
        }
    }
    
    // 如果没有找到直接的候选者，尝试多层循环交换（作为备选方案）
    // if (!changed && candidates.empty()) {
    //     std::cout << "[LIC] No direct candidates found, trying multi-level interchange..." << std::endl;
    //     for (auto& loopPtr : LA.loops) {
    //         if (!loopPtr) continue;
    //         std::cout << "[LIC] Analyzing loop: " << loopPtr->header->label << std::endl;
    //         Loop* start = loopPtr.get();
    //         auto nest = collectPerfectNest(start);
    //         if (nest.size() < 2) continue;
    //         changed |= performMultiLevelInterchange(F, nest, SCEVA, this);
    //         if (changed) break; // 只进行一次交换
    //     }
    // }
    
    return changed;
}

bool LoopInterchangePass::run() {
    bool changed=false;
    for (auto& fn : module.functions) {
        // 假定有 isDeclaration
        if (fn->isDeclaration() || fn->name == "global") continue; // TODO DBEUG
        changed = runOnFunction(*fn);
    }
    return changed;
}

} // namespace loop_interchange
} // namespace llvm_ir
