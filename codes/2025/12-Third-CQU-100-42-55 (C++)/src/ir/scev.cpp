#include "../../include/ir/scev.h"
#include "../../include/ir/loop_analysis.h"
#include "../../include/ir/cfg.h"
#include <iostream>
#include <queue>
#include <algorithm>
#include <cassert>

namespace llvm_ir {
// 便捷函数：分析模块中的所有函数
SCEVAnalysis analyzeSCEV(Function& F, Module& M, const LoopAnalysis& LA) {
    std::cout << "[scev] Analyzing function: " << F.name << std::endl;
    
    // 创建单一的SCEV分析器实例，用于整个函数
    SCEVAnalysis analyzer(&F, &M);
    
    // 找到所有顶层循环（没有父循环的循环）
    std::vector<Loop*> topLevelLoops;
    for (auto& loop_ptr : LA.loops) {
        if (!loop_ptr) continue;
        if (loop_ptr->getParent() == nullptr) {
            topLevelLoops.push_back(loop_ptr.get());
        }
    }
    
    std::cout << "[scev] Found " << topLevelLoops.size() << " top-level loops in function " << F.name << std::endl;
    
    // 对每个顶层循环进行递归分析
    for (Loop* loop : topLevelLoops) {
        analyzer.analyzeLoopRecursively(loop, 0); // 0表示嵌套深度
    }
    
    // // 打印函数级别的分析总结
    // analyzer.printFunctionSummary();

    return analyzer;
}
// 递归分析循环嵌套结构
void SCEVAnalysis::analyzeLoopRecursively(Loop* loop, int depth) {
    if (!loop) return;
    
    std::string indent(depth * 2, ' ');
    std::cout << "[scev] " << indent << "Analyzing loop at depth " << depth 
              << " with header: " << loop->getHeader()->label << std::endl;
    
    // 保存当前循环上下文
    Loop* oldLoop = currentLoop;
    
    // 设置当前循环并进行分析
    currentLoop = loop;
    
    // 分析当前循环（不清空全局缓存）
    findLoopInvariants();
    findLoopVariables();
    analyzeRecurrences();
    analyzeLoopBounds();
    findLoopExitConditions();
    identifyPrimaryLoopVariable();
    computeLoopTripCounts();
    
    // 打印当前循环的分析结果
    printAnalysisResults();
    
    // 递归分析所有子循环
    const std::vector<Loop*>& children = loop->getChildren();
    if (!children.empty()) {
        std::cout << "[scev] " << indent << "Analyzing " << children.size() 
                  << " child loop(s)..." << std::endl;
        for (Loop* childLoop : children) {
            analyzeLoopRecursively(childLoop, depth + 1);
        }
    }
    
    // 恢复上一层循环的上下文
    currentLoop = oldLoop;
    
    std::cout << "[scev] " << indent << "Completed analysis for loop with header: " 
              << loop->getHeader()->label << std::endl;
}
// 分析单个loop
void SCEVAnalysis::analyzeLoop(Loop* loop) {
    if (!loop || !function) return;
    
    currentLoop = loop;
    std::cout << "[scev] Analyzing loop with header: " << loop->getHeader()->label << std::endl;
    
    scevCache.clear();
    allLoopInfo.clear();
    invariantValues.clear();
    // 为当前循环创建新的变量信息存储
    allLoopInfo[loop] = std::map<Value*, SCEVLoopVariableInfo>();
    
    // 清空递归检测集合
    scevBeingComputed.clear();
    
    // 执行分析步骤
    findLoopInvariants();
    findLoopVariables();
    analyzeLoopBounds();
    findLoopExitConditions();
    analyzeRecurrences();
    identifyPrimaryLoopVariable();
    computeLoopTripCounts();
    
    // 打印分析结果
    printAnalysisResults();
    
    // 循环变量信息存储在allLoopInfo[currentLoop]中
}

// 查找循环不变量
void SCEVAnalysis::findLoopInvariants() {
    if (!currentLoop) return;
    
    std::cout << "[scev] Finding loop invariants..." << std::endl;
    
    // 使用工作列表算法识别循环不变量
    std::queue<Value*> worklist;
    std::set<Value*> processed;
    
    // 初始化：将循环外的值加入工作列表
    for (auto& bb_ptr : function->basicBlocks) {
        BasicBlock* bb = bb_ptr.get();
        if (!currentLoop->contains(bb)) {
            // 将基本块中的所有指令结果加入工作列表
            // 注意：不要跳过无名指令，它们也可能是循环不变量
            for (auto& inst : bb->instructions) {
                worklist.push(&(*inst));
            }
            for (auto& phi : bb->phi_instructions) {
                worklist.push(&(*phi));
            }
        }
    }
    
    // 添加全局变量到工作列表
    if (module) {
        for (auto& globalVar : module->globalVariables) {
            worklist.push(&(*globalVar));
        }
    }
    
    // 工作列表迭代：如果一个指令的所有操作数都已经被确定为不变量，那么这个指令也是不变量
    bool changed = true;
    while (changed) {
        changed = false;
        
        while (!worklist.empty()) {
            Value* value = worklist.front();
            worklist.pop();
            
            if (processed.count(value)) continue;
            processed.insert(value);
            
            // 检查值是否为循环不变量
            bool canBeInvariant = false;
            
            // 1. 常量和循环外定义的值直接标记为不变量
            if (isLoopInvariant(value)) {
                canBeInvariant = true;
            } else if (auto* inst = dynamic_cast<Instruction*>(value)) {
                // 2. 对于循环内的指令，检查是否为特殊情况
                
                // 检查全局变量的修改情况
                if (auto* globalVar = dynamic_cast<GlobalVariable*>(value)) {
                    bool isModified = false;
                    for (BasicBlock* bb : currentLoop->getBlocks()) {
                        for (auto& inst : bb->instructions) {
                            if (auto* storeInst = dynamic_cast<StoreInst*>(&(*inst))) {
                                if (storeInst->getPointerOperand()->name.substr(1) == globalVar->name) {
                                    isModified = true;
                                    break;
                                }
                            }
                        }
                        if (isModified) break;
                    }
                    canBeInvariant = !isModified;
                } else if (currentLoop->contains(inst)) {
                    // 3. 对于循环内的指令，检查其所有操作数是否都是不变量
                    bool allOperandsInvariant = true;
                    
                    // 特殊处理PHI指令
                    if (auto* phi = dynamic_cast<PhiInst*>(inst)) {
                        for (const auto& incoming : phi->incoming_values) {
                            if (!isLoopInvariant(incoming.first)) {
                                allOperandsInvariant = false;
                                break;
                            }
                        }
                    } else {
                        // 普通指令：检查所有操作数
                        for (Value* operand : inst->operands) {
                            if (!isLoopInvariant(operand)) {
                                allOperandsInvariant = false;
                                break;
                            }
                        }
                    }
                    
                    canBeInvariant = allOperandsInvariant;
                }
            }
            
            if (canBeInvariant) {
                invariantValues.insert(value);
                changed = true;
                // std::cout << "[scev]   Found invariant: " << value->name << std::endl;
                
                // 将使用该值的指令加入工作列表
                for (Instruction* user : value->getUses()) {
                    if (currentLoop->contains(user) && !processed.count(user)) {
                        worklist.push(user);
                    }
                }
            }
        }
    }
}

// 查找循环变量
void SCEVAnalysis::findLoopVariables() {
    if (!currentLoop) return;
    
    std::cout << "[scev] Finding loop variables..." << std::endl;
    
    // 获取当前循环的变量信息引用
    auto& loopVariables = allLoopInfo[currentLoop];
    
    // 查找PHI指令作为潜在的循环变量
    for (auto& bb_ptr : function->basicBlocks) {
        BasicBlock* bb = bb_ptr.get();
        if (!currentLoop->contains(bb)) continue;
        
        for (auto& phi : bb->phi_instructions) {
            
            // 分析PHI指令的循环变量模式
            Value* var = &(*phi);
            Value* initialValue = nullptr;
            Value* stepValue = nullptr;
            bool isMultiplication = false;
            
            if (analyzePhiAsLoopVariable(&(*phi), initialValue, stepValue, isMultiplication)) {
                SCEVLoopVariableInfo info;
                info.variable = var;
                info.initialValue = initialValue;
                info.stepValue = stepValue;
                
                // 根据步长值判断是递增还是递减
                if (auto* constStep = dynamic_cast<ConstantInt*>(stepValue)) {
                    info.isIncrement = (constStep->value > 0);
                } else if (auto* constFloat = dynamic_cast<ConstantFloat*>(stepValue)) {
                    info.isIncrement = (constFloat->value > 0.0f);
                } else {
                    // 对于非常量步长，默认为递增（保持向后兼容）
                    info.isIncrement = true;
                }
                
                info.isSimple = !isMultiplication; // 乘法不是简单循环
                
                // std::cout << "[scev]   Successfully analyzed PHI " << var->name 
                //          << " as loop variable (initial: " << (initialValue ? initialValue->name : "null")
                //          << ", step: " << (stepValue ? stepValue->name : "null") 
                //          << ", " << (info.isIncrement ? "increment" : "decrement") << ")" << std::endl;
                
                loopVariables[var] = std::move(info);
            } else {
                // std::cout << "[scev]   PHI " << var->name << " is not a loop variable" << std::endl;
            }
        }
    }
    
    // 在SSA形式的IR中，所有循环变量都应该通过PHI指令表示
    // 如果没有找到PHI指令的循环变量，说明这个循环没有循环携带的依赖关系
    if (loopVariables.empty()) {
        std::cout << "[scev]   No PHI-based loop variables found. This loop has no loop-carried dependencies." << std::endl;
    }
}

void SCEVAnalysis::analyzeRecurrences() {
    if (!currentLoop) return;
    
    // 获取当前循环的变量信息引用
    auto& loopVariables = allLoopInfo[currentLoop];
    
    std::cout << "[scev] Analyzing recurrences for " << loopVariables.size() << " loop variables..." << std::endl;
    
    // 为每个循环变量构建SCEV表达式
    for (auto& [var, info] : loopVariables) {
        // 检查是否已经在缓存中存在（避免重复创建）
        if (scevCache.find(var) != scevCache.end()) {
            if (!info.scev) {
                info.scev = scevCache[var]->clone();
            }
            continue;
        }
        
        if (!info.initialValue || !info.stepValue) {
            std::cout << "[scev]     Skipping " << var->name << ": missing initial value or step value" << std::endl;
            continue;
        }
        
        // 创建初始值的SCEV
        auto startSCEV = getSCEV(info.initialValue);
        if (!startSCEV) {
            std::cout << "[scev]     Failed to create SCEV for initial value of " << var->name << std::endl;
            continue;
        }
        
        // 创建步长/因子的SCEV
        auto stepSCEV = getSCEV(info.stepValue);
        if (!stepSCEV) {
            std::cout << "[scev]     Failed to create SCEV for step value of " << var->name << std::endl;
            continue;
        }
        
        // 根据循环变量的类型创建不同的SCEV表达式
        if (info.isSimple) {
            // 简单循环：加法递归
            std::cout << "[scev]     Creating AddRec SCEV for " << var->name << "..." << std::endl;
            auto addRecSCEV = std::make_unique<SCEVAddRecExpr>(
                startSCEV->clone(), stepSCEV->clone(), currentLoop, var->type);
            
            info.scev = std::move(addRecSCEV);
            scevCache[var] = info.scev->clone();
            
            std::cout << "[scev]   Created AddRec SCEV for " << var->name 
                        << ": " << info.scev->toString() << std::endl;
        } else {
            // 复杂循环：乘法递归
            std::cout << "[scev]     Creating MulRec SCEV for " << var->name << "..." << std::endl;
            auto mulRecSCEV = std::make_unique<SCEVMulRecExpr>(
                startSCEV->clone(), stepSCEV->clone(), currentLoop, var->type);
            
            info.scev = std::move(mulRecSCEV);
            scevCache[var] = info.scev->clone();
            
            std::cout << "[scev]   Created MulRec SCEV for " << var->name 
                        << ": " << info.scev->toString() << std::endl;
        }
    }
}

// 检查值是否为循环不变量（基于已计算的结果，避免递归）
bool SCEVAnalysis::isLoopInvariant(Value* value) {
    if (!value || !currentLoop) return true;

    // 1. 常量是不变量
    if (dynamic_cast<Constant*>(value)) {
        return true;
    }

    // 2. 检查是否已经计算过并记录在不变量集合中
    if (invariantValues.count(value)) {
        return true;
    }

    // 3. 如果是指令，检查其定义位置
    if (auto* inst = dynamic_cast<Instruction*>(value)) {
        // 如果指令在循环外定义，它是不变量
        if (!currentLoop->contains(inst)) {
            return true;
        }
    }

    // 4. 对于函数参数等非指令值，认为是不变量
    if (!dynamic_cast<Instruction*>(value)) {
        return true;
    }

    // 否则，我们目前无法确定它是不变量（在worklist算法的单次检查中）
    return false;
}

// 分析PHI指令作为循环变量
bool SCEVAnalysis::analyzePhiAsLoopVariable(PhiInst* phi, Value*& initialValue, Value*& stepValue, bool& isMultiplication) {
    if (!phi || phi->incoming_values.empty()) {
        std::cout << "[scev]         analyzePhiAsLoopVariable: null phi or empty incoming values" << std::endl;
        return false;
    }
    // std::cout << "[scev]         analyzePhiAsLoopVariable: analyzing " << phi->toString() << std::endl;
    
    // 分离来自循环外和循环内的输入
    std::vector<std::pair<Value*, BasicBlock*>> fromOutside;
    std::vector<std::pair<Value*, BasicBlock*>> fromInside;
    
    for (const auto& incoming : phi->incoming_values) {
        if (currentLoop->contains(incoming.second)) {
            fromInside.push_back(incoming);
        } else {
            fromOutside.push_back(incoming);
        }
    }
    
    // 检查是否有来自循环外的输入（初始值）
    if (fromOutside.empty()) {
        // std::cout << "[scev]       PHI has " << fromOutside.size() << " inputs from outside loop, checking if it's a loop-invariant PHI" << std::endl;
        
        // 检查是否所有输入都是循环不变量且相等（这种情况下PHI本身也是不变量）
        bool allInvariant = true;
        Value* commonValue = nullptr;
        
        for (const auto& incoming : phi->incoming_values) {
            if (!isLoopInvariant(incoming.first)) {
                allInvariant = false;
                break;
            }
            if (commonValue == nullptr) {
                commonValue = incoming.first;
            } else if (commonValue != incoming.first) {
                allInvariant = false;
                break;
            }
        }
        
        if (allInvariant && commonValue) {
            // std::cout << "[scev]       PHI is a loop-invariant with common value: " << commonValue->name << std::endl;
            // 这不是循环变量，而是循环不变量
            return false;
        }
        
        // std::cout << "[scev]       PHI cannot be analyzed as loop variable" << std::endl;
        return false;
    }
    
    if (fromOutside.size() > 1) {
        std::cerr << "[scev]       PHI has " << fromOutside.size() << " inputs from outside loop (expected 1), taking the first one" << std::endl;
        // 不应该发生，因为我们的循环只有一个唯一的preHeader
    }
    
    initialValue = fromOutside[0].first;
    
    // 分析来自循环内的所有输入
    if (fromInside.empty()) {
        // std::cout << "[scev]       PHI has no inputs from inside loop" << std::endl;
        return false;
    }
    
    // 检查所有来自循环内的输入是否都归结为同一个更新模式
    Opcode expectedOpcode = Opcode::Add; // 默认期望加法
    Value* expectedStep = nullptr;
    bool firstIteration = true;
    std::unique_ptr<SCEV> expectedStepSCEV; // 基于SCEV的步长等价缓存
    
    for (const auto& incoming : fromInside) {
        Value* incomingValue = incoming.first;
        
        // 检查是否为二元操作
        if (auto* binOp = dynamic_cast<BinaryOperator*>(incomingValue)) {
            if (binOp->operands.size() != 2) {
                return false;
            }
            
            // 检查是否有一个操作数是PHI指令本身
            Value* otherOperand = nullptr;
            if (binOp->operands[0] == phi) {
                otherOperand = binOp->operands[1];
            } else if (binOp->operands[1] == phi) {
                otherOperand = binOp->operands[0];
            } else {
                return false;
            }
            
            // 将 Sub规范化为加法步长（取负）
            Opcode op = binOp->opcode;
            std::unique_ptr<SCEV> otherSCEV = getSCEV(otherOperand);
            // 归一化步长的 Value 表示，用于外部记录
            Value* normalizedStepVal = otherOperand;
            if (op == Opcode::Sub) {
                op = Opcode::Add;
                if (auto* cst = dynamic_cast<SCEVConstant*>(otherSCEV.get())) {
                    otherSCEV = std::make_unique<SCEVConstant>(-cst->value, cst->valueType);
                    // 若是常量，连同 Value 也归一化成负常量，避免后续丢失方向
                    if (auto* cVal = dynamic_cast<ConstantInt*>(otherOperand)) {
                        normalizedStepVal = new ConstantInt(-cVal->value);
                    } else {
                        // 非常量步长且为减法，目前不支持安全归一化
                        return false;
                    }
                } else {
                    // 无法取负的复杂步长，暂不支持
                    return false;
                }
            }
            
            // 检查操作类型
            if (firstIteration) {
                expectedOpcode = op;
                expectedStep = normalizedStepVal;
                expectedStepSCEV = std::move(otherSCEV);
                firstIteration = false;
            } else {
                // 所有更新操作必须使用相同的操作类型和步长（基于SCEV语义等价而非指针相等）
                if (op != expectedOpcode) {
                    return false;
                }
                auto currentStepSCEV = std::move(otherSCEV);
                bool stepEqual = false;
                if (!expectedStepSCEV && !currentStepSCEV) {
                    stepEqual = true;
                } else if (expectedStepSCEV && currentStepSCEV) {
                    stepEqual = currentStepSCEV->equals(expectedStepSCEV.get());
                }
                if (!stepEqual) {
                    return false;
                }
            }
        } else {
            // 处理“回边值本身是一个PHI（如latch上的合并）”的情况
            if (auto* latchPhi = dynamic_cast<PhiInst*>(incomingValue)) {
                for (const auto& innerIncoming : latchPhi->incoming_values) {
                    Value* innerVal = innerIncoming.first;
                    if (auto* innerBin = dynamic_cast<BinaryOperator*>(innerVal)) {
                        if (innerBin->operands.size() != 2) {
                            return false;
                        }
                        Value* innerOther = nullptr;
                        if (innerBin->operands[0] == phi) {
                            innerOther = innerBin->operands[1];
                        } else if (innerBin->operands[1] == phi) {
                            innerOther = innerBin->operands[0];
                        } else {
                            return false;
                        }
                        if (firstIteration) {
                            // 处理减法归一化
                            Opcode opInner = innerBin->opcode;
                            std::unique_ptr<SCEV> stepS = getSCEV(innerOther);
                            Value* normalizedStepVal = innerOther;
                            if (opInner == Opcode::Sub) {
                                opInner = Opcode::Add;
                                if (auto* cst = dynamic_cast<SCEVConstant*>(stepS.get())) {
                                    stepS = std::make_unique<SCEVConstant>(-cst->value, cst->valueType);
                                    if (auto* cVal = dynamic_cast<ConstantInt*>(innerOther)) {
                                        normalizedStepVal = new ConstantInt(-cVal->value);
                                    } else {
                                        return false;
                                    }
                                } else {
                                    return false;
                                }
                            }
                            expectedOpcode = opInner;
                            expectedStep = normalizedStepVal;
                            expectedStepSCEV = std::move(stepS);
                            firstIteration = false;
                        } else {
                            Opcode opInner = innerBin->opcode;
                            if (opInner == Opcode::Sub) opInner = Opcode::Add; // 语义归一化
                            if (opInner != expectedOpcode) {
                                return false;
                            }
                            auto currentStepSCEV = getSCEV(innerOther);
                            if (innerBin->opcode == Opcode::Sub) {
                                if (auto* cst = dynamic_cast<SCEVConstant*>(currentStepSCEV.get())) {
                                    currentStepSCEV = std::make_unique<SCEVConstant>(-cst->value, cst->valueType);
                                } else {
                                    return false;
                                }
                            }
                            bool stepEqual = false;
                            if (!expectedStepSCEV && !currentStepSCEV) {
                                stepEqual = true;
                            } else if (expectedStepSCEV && currentStepSCEV) {
                                stepEqual = currentStepSCEV->equals(expectedStepSCEV.get());
                            }
                            if (!stepEqual) {
                                return false;
                            }
                        }
                    } else {
                        // 允许“无更新”的路径：直接传回header PHI 自身
                        if (innerVal != phi) {
                            return false;
                        }
                    }
                }
                // 已在上面遍历并校验完 latch PHI 的各个分支
            } else {
                // 如果不是二元操作或PHI合并，检查是否直接是PHI指令本身（无更新）
                if (incomingValue != phi) {
                    return false;
                }
            }
        }
    }
    
    // 检查操作类型是否支持
    if (expectedOpcode != Opcode::Add && expectedOpcode != Opcode::Mul) {
        return false;
    }
    
    stepValue = expectedStep;
    isMultiplication = (expectedOpcode == Opcode::Mul);
    
    // std::cout << "[scev]     Analyzed PHI as loop variable: " << phi->name 
    //          << " with " << fromInside.size() << " loop inputs" << std::endl;
    
    return true;
}

void SCEVAnalysis::analyzeLoopBounds() {
    if (!currentLoop) return;
    
    std::cout << "[scev] Analyzing loop bounds..." << std::endl;
    
    // 查找循环头中的比较指令
    BasicBlock* header = currentLoop->getHeader();
    if (!header) return;
    
    for (auto& inst : header->instructions) {
        if (inst->opcode == Opcode::ICmp) {
            Value* var = nullptr;
            Value* bound = nullptr;
            bool isUpperBound = false;
            ICmpCond condition = ICmpCond::EQ;
            
            if (isSimpleComparison(&(*inst), var, bound, isUpperBound, condition)) {
                // 找到循环变量的边界
                auto& loopVars = getCurrentLoopVariables();
                auto it = loopVars.find(var);
                if (it != loopVars.end()) {
                    if (isUpperBound) {
                        it->second.upperBound = bound;
                        std::cout << "[scev]   Found upper bound for " << var->name 
                                 << ": " << (bound ? bound->name : "unknown") << std::endl;
                    } else {
                        it->second.lowerBound = bound;
                        std::cout << "[scev]   Found lower bound for " << var->name 
                                 << ": " << (bound ? bound->name : "unknown") << std::endl;
                    }
                    it->second.exitCondition = condition;
                }
            }
        }
    }
}

void SCEVAnalysis::findLoopExitConditions() {
    if (!currentLoop) return;
    
    std::cout << "[scev] Finding loop exit conditions..." << std::endl;
    
    // 查找循环退出块中的条件
    for (BasicBlock* exitBlock : currentLoop->getExitBlocks()) {
        for (auto& inst : exitBlock->instructions) {
            if (inst->opcode == Opcode::ICmp) {
                Value* var = nullptr;
                Value* bound = nullptr;
                bool isUpperBound = false;
                ICmpCond condition = ICmpCond::EQ;
                
                if (isSimpleComparison(&(*inst), var, bound, isUpperBound, condition)) {
                    auto& loopVars = getCurrentLoopVariables();
                    auto it = loopVars.find(var);
                    if (it != loopVars.end()) {
                        if (isUpperBound) {
                            it->second.upperBound = bound;
                            std::cout << "[scev]   Found exit upper bound for " << var->name 
                                     << ": " << (bound ? bound->name : "unknown") << std::endl;
                        } else {
                            it->second.lowerBound = bound;
                            std::cout << "[scev]   Found exit lower bound for " << var->name 
                                     << ": " << (bound ? bound->name : "unknown") << std::endl;
                        }
                        it->second.exitCondition = condition;
                    }
                }
            }
        }
    }
}

// 识别主循环变量
void SCEVAnalysis::identifyPrimaryLoopVariable() {
    if (!currentLoop || getCurrentLoopVariables().empty()) return;
    
    std::cout << "[scev] Identifying primary loop variable..." << std::endl;
    
    Value* primaryVariable = nullptr;
    int maxScore = -1;
    
    // 为每个循环变量计算得分
    auto& loopVars = getCurrentLoopVariables();
    for (auto& [var, info] : loopVars) {
        int score = 0;
        
        // 评分标准1: 简单循环变量得分更高（递增和递减都支持）
        if (info.isSimple) {
            score += 10;
            // 递增循环稍微优先（因为更常见）
            if (info.isIncrement) {
                score += 2;
            }
        }
        
        // 评分标准2: 有明确边界的变量得分更高
        // 对于递增循环，上界更重要；对于递减循环，下界更重要
        if (info.isIncrement) {
            if (info.upperBound) {
                score += 8;
                if (info.lowerBound) score += 2; // 有下界额外加分
            }
        } else {
            // 递减循环
            if (info.lowerBound) {
                score += 8;
                if (info.upperBound) score += 2; // 有上界额外加分
            }
        }
        
        // 评分标准3: 步长为常量1或-1的变量得分更高
        if (auto* constStep = dynamic_cast<ConstantInt*>(info.stepValue)) {
            if (constStep->value == 1 || constStep->value == -1) {
                score += 6;
            } else if (constStep->value > 0) {
                score += 3;
            } else if (constStep->value < 0) {
                score += 3; // 递减循环也给予相同分数
            }
        }
        
        // 评分标准4: 初始值为0或简单常量的变量得分更高
        if (auto* constInit = dynamic_cast<ConstantInt*>(info.initialValue)) {
            if (constInit->value == 0) {
                score += 4;
            } else {
                score += 2;
            }
        }
        
        // 评分标准5: 在循环退出条件中使用的变量得分更高
        if (info.exitCondition != ICmpCond::EQ) { // 有明确的退出条件
            score += 7;
        }
        
        // 评分标准6: 变量名包含典型循环计数器名称的得分更高
        std::string varName = var->name;
        if (varName.find("i") != std::string::npos || 
            varName.find("j") != std::string::npos || 
            varName.find("k") != std::string::npos ||
            varName.find("count") != std::string::npos ||
            varName.find("index") != std::string::npos) {
            score += 3;
        }
        
        // 评分标准7: 有常量循环次数的变量得分更高
        if (info.constantTripCount) {
            score += 5;
        }
        
        // 评分标准8: 循环变量与边界的关系合理性检查
        if (info.isIncrement && info.upperBound && info.initialValue) {
            // 递增循环：初始值应该小于上界
            if (auto* constInit = dynamic_cast<ConstantInt*>(info.initialValue)) {
                if (auto* constUpper = dynamic_cast<ConstantInt*>(info.upperBound)) {
                    if (constInit->value < constUpper->value) {
                        score += 3; // 合理的递增循环
                    }
                }
            }
        } else if (!info.isIncrement && info.lowerBound && info.initialValue) {
            // 递减循环：初始值应该大于下界
            if (auto* constInit = dynamic_cast<ConstantInt*>(info.initialValue)) {
                if (auto* constLower = dynamic_cast<ConstantInt*>(info.lowerBound)) {
                    if (constInit->value > constLower->value) {
                        score += 3; // 合理的递减循环
                    }
                }
            }
        }
        
        std::cout << "[scev]   Variable " << var->name << " score: " << score 
                 << " (increment: " << (info.isIncrement ? "yes" : "no") << ")" << std::endl;
        
        if (score > maxScore) {
            maxScore = score;
            primaryVariable = var;
        }
    }
    
    // 标记主循环变量
    if (primaryVariable && maxScore > 0) {
        getCurrentLoopVariables()[primaryVariable].isPrimary = true;
        const auto& primaryInfo = getCurrentLoopVariables()[primaryVariable];
        std::cout << "[scev]   Primary loop variable identified: " << primaryVariable->name 
                 << " (score: " << maxScore << ", increment: " << (primaryInfo.isIncrement ? "yes" : "no") << ")" << std::endl;
    } else {
        std::cout << "[scev]   No primary loop variable identified" << std::endl;
    }
}

Value* SCEVAnalysis::findInitialValue(Value* var) {
    if (!var || !function) return nullptr;
    
    // 查找变量的初始值
    // 首先检查是否为PHI指令，从incoming_values中查找来自循环外的值
    if (auto* phi = dynamic_cast<PhiInst*>(var)) {
        for (const auto& incoming : phi->incoming_values) {
            if (!currentLoop || !currentLoop->contains(incoming.second)) {
                return incoming.first;
            }
        }
    }
    
    // 在SSA形式中，变量的初始值应该通过PHI指令的incoming_values确定
    // 这里不再依赖变量名匹配，因为SSA形式中每个变量只被赋值一次
    
    // 检查是否为函数参数（通过指针比较而不是名字比较）
    for (Value* param : function->parameters) {
        if (param == var) {
            return param;
        }
    }
    
    // 检查是否为全局变量
    if (auto* globalVar = dynamic_cast<GlobalVariable*>(var)) {
        // 返回全局变量的初始值
        if (globalVar->initializer) {
            return globalVar->initializer;
        } else {
            // 如果没有初始值，返回默认值
            if (globalVar->type == Type::I32) {
                return new ConstantInt(0);
            } else if (globalVar->type == Type::Float) {
                return new ConstantFloat(0.0f);
            }
        }
    }
    
    // 如果都找不到，返回nullptr而不是abort
    std::cout << "[scev]     findInitialValue: Could not find initial value for " << var->name 
             << " (type: " << (var->type == Type::I32 ? "I32" : var->type == Type::Float ? "Float" : "Other") << ")" << std::endl;
    
    return nullptr;
}

// 检查分支条件是否为简单的比较
bool SCEVAnalysis::isSimpleComparison(Instruction* inst, Value*& var, Value*& bound, 
                                     bool& isUpperBound, ICmpCond& condition) {
    if (!inst || inst->opcode != Opcode::ICmp) return false;
    
    auto* icmp = dynamic_cast<ICmpInst*>(inst);
    if (!icmp || icmp->operands.size() != 2) return false;
    
    // 检查是否为循环变量与常量的比较
    Value* lhs = icmp->operands[0];
    Value* rhs = icmp->operands[1];
    
    // 检查其中一个操作数是否为循环变量
    auto& loopVars = getCurrentLoopVariables();
    bool lhsIsLoopVar = loopVars.count(lhs) > 0;
    bool rhsIsLoopVar = loopVars.count(rhs) > 0;
    
    // 如果还没有识别出循环变量，尝试检查是否为PHI指令
    if (!lhsIsLoopVar && !rhsIsLoopVar) {
        if (auto* phi = dynamic_cast<PhiInst*>(lhs)) {
            if (phi->parent == currentLoop->getHeader()) {
                lhsIsLoopVar = true;
            }
        }
        if (auto* phi = dynamic_cast<PhiInst*>(rhs)) {
            if (phi->parent == currentLoop->getHeader()) {
                rhsIsLoopVar = true;
            }
        }
    }
    
    if (!lhsIsLoopVar && !rhsIsLoopVar) return false;

    // 如果当前loop的header不是exiting，说明比较条件有复合，返回false
    for (auto* exitingBlock : currentLoop->getExitingNodes()) {
        if (exitingBlock == currentLoop->getHeader()) {
            continue;
        } else {
            return false;
        }
    }

    if (icmp->condition == ICmpCond::EQ || icmp->condition == ICmpCond::NE) {
        return false; // TODO, 暂不支持相等和不相等
    }
    
    if (lhsIsLoopVar) {
        var = lhs;
        bound = rhs;
        isUpperBound = (icmp->condition == ICmpCond::SLT || icmp->condition == ICmpCond::SLE ||
                       icmp->condition == ICmpCond::ULT || icmp->condition == ICmpCond::ULE);
    } else {
        var = rhs;
        bound = lhs;
        isUpperBound = (icmp->condition == ICmpCond::SGT || icmp->condition == ICmpCond::SGE ||
                       icmp->condition == ICmpCond::UGT || icmp->condition == ICmpCond::UGE);
    }
    
    condition = icmp->condition;
    
    std::cout << "[scev]     Found comparison: " << var->name 
             << " " << getICmpConditionName(icmp->condition) << " " 
             << (bound ? bound->name : "unknown") << std::endl;
    
    return true;
}

bool SCEVAnalysis::isLoopVariable(Value* value) {
    if (!value || !currentLoop) return false;
    
    // 在SSA形式的IR中，循环变量必须通过PHI指令表示
    // 检查是否为PHI指令
    if (auto* phi = dynamic_cast<PhiInst*>(value)) {
        // 检查PHI指令是否在循环头中
        if (phi->parent == currentLoop->getHeader()) {
            // 检查是否有来自循环外的输入和来自循环内的输入
            bool hasFromOutside = false;
            bool hasFromInside = false;
            
            for (const auto& incoming : phi->incoming_values) {
                if (currentLoop->contains(incoming.second)) {
                    hasFromInside = true;
                } else {
                    hasFromOutside = true;
                }
            }
            
            return hasFromOutside && hasFromInside;
        }
    }
    
    return false;
}

// 计算循环次数：仅处理典型情形
// 形式：
//   - 条件：i < B, i <= B, i > B, i >= B（含有符号/无符号）
//   - 起始：i0 = 常量（或可求常量的SCEV）
//   - 步长：常量 +/-k（k>0），对加法递增/递减
//   - 乘法递归暂不计算
// 计算规则（已对齐 LLVM 惯例）：
//   i from S to bound B, step = +k：
//     S < B：ceil((B - S) / k)；S >= B：0
//   i from S to bound B, step = -k：
//     S > B：ceil((S - B) / k)；S <= B：0
void SCEVAnalysis::computeLoopTripCounts() {
    if (!currentLoop) return;
    auto& loopVars = getCurrentLoopVariables();
    if (loopVars.empty()) return;

    auto getConst = [&](Value* v) -> std::optional<int64_t> {
        if (!v) return std::nullopt;
        if (auto* c = dynamic_cast<ConstantInt*>(v)) return c->value;
        auto s = getSCEV(v);
        if (s) return s->getConstantValue();
        return std::nullopt;
    };

    for (auto& [var, info] : allLoopInfo[currentLoop]) {
        info.constantTripCount.reset();

        // 仅支持简单加法递归
        if (!info.isSimple || !info.stepValue || !info.initialValue) continue;
        auto stepOpt = getConst(info.stepValue);
        auto initOpt = getConst(info.initialValue);
        auto loOpt = getConst(info.lowerBound);
        auto hiOpt = getConst(info.upperBound);

        // 只处理整型常量边界与初值、步长为常量
        if (!stepOpt || !initOpt) continue;
        if (!loOpt && !hiOpt) continue;

        int64_t step = *stepOpt;
        if (step == 0) continue;

        int64_t S = *initOpt;
        std::optional<int64_t> B;
        bool upper = false;
        if (hiOpt) { B = *hiOpt; upper = true; }
        else if (loOpt) { B = *loOpt; upper = false; }
        else continue;

        // 只支持严格/非严格比较的常见组合
        auto cond = info.exitCondition;
        auto ceil_div = [](int64_t num, int64_t den) -> int64_t {
            // den > 0
            if (num <= 0) return 0;
            return (num + den - 1) / den;
        };

        // 对称处理递增与递减
        if (step > 0) {
            // 递增：上界有效
            if (!upper || !B) continue;
            int64_t bound = *B;
            // 仅处理 signed lt/le 情况（无符号与其它可扩展）
            switch (cond) {
                case ICmpCond::SLT:
                case ICmpCond::ULT:
                    info.constantTripCount = (S < bound) ? std::optional<int64_t>(ceil_div(bound - S, step)) : std::optional<int64_t>(0);
                    break;
                case ICmpCond::SLE:
                case ICmpCond::ULE:
                    info.constantTripCount = (S <= bound) ? std::optional<int64_t>(ceil_div(bound + 1 - S, step)) : std::optional<int64_t>(0);
                    break;
                default:
                    break;
            }
        } else { // step < 0
            // 递减：下界有效
            if (upper || !B) continue;
            int64_t bound = *B;
            int64_t k = -step; // 正数
            switch (cond) {
                case ICmpCond::SGT:
                case ICmpCond::UGT:
                    info.constantTripCount = (S > bound) ? std::optional<int64_t>(ceil_div(S - bound, k)) : std::optional<int64_t>(0);
                    break;
                case ICmpCond::SGE:
                case ICmpCond::UGE:
                    info.constantTripCount = (S >= bound) ? std::optional<int64_t>(ceil_div(S - (bound - 1), k)) : std::optional<int64_t>(0);
                    break;
                default:
                    break;
            }
        }
    }
}

std::optional<int64_t> SCEVAnalysis::getLoopTripCount(Loop* loop) const {
    if (!loop) loop = currentLoop;
    if (!loop) return std::nullopt;
    auto it = allLoopInfo.find(loop);
    if (it == allLoopInfo.end()) return std::nullopt;
    const auto& vars = it->second;
    // 优先主变量
    for (const auto& [var, info] : vars) {
        if (info.isPrimary && info.constantTripCount) return info.constantTripCount;
    }
    // 其次任一可解析
    for (const auto& [var, info] : vars) {
        if (info.constantTripCount) return info.constantTripCount;
    }
    return std::nullopt;
}

// =========辅助函数=========
// 帮助函数：获取当前循环的变量信息映射
std::map<Value*, SCEVLoopVariableInfo>& SCEVAnalysis::getCurrentLoopVariables() {
    if (!currentLoop) {
        static std::map<Value*, SCEVLoopVariableInfo> empty;
        return empty;
    }
    return allLoopInfo[currentLoop];
}

const std::map<Value*, SCEVLoopVariableInfo>& SCEVAnalysis::getCurrentLoopVariables() const {
    if (!currentLoop) {
        static std::map<Value*, SCEVLoopVariableInfo> empty;
        return empty;
    }
    auto it = allLoopInfo.find(currentLoop);
    if (it != allLoopInfo.end()) {
        return it->second;
    }
    static std::map<Value*, SCEVLoopVariableInfo> empty;
    return empty;
}

// =========Log=========
void SCEVAnalysis::printAnalysisResults() const {
    if (!currentLoop) return;
    
    std::cout << "[scev] ========================================" << std::endl;
    std::cout << "[scev] SCEV Analysis Results" << std::endl;
    std::cout << "[scev] ========================================" << std::endl;
    
    // 打印循环基本信息
    std::cout << "[scev] Loop Information:" << std::endl;
    std::cout << "[scev]   Header: " << currentLoop->getHeader()->label << std::endl;
    std::cout << "[scev]   Blocks: ";
    for (BasicBlock* bb : currentLoop->getBlocks()) {
        std::cout << bb->label << " ";
    }
    std::cout << std::endl;
    
    if (currentLoop->getPreheader()) {
        std::cout << "[scev]   Preheader: " << currentLoop->getPreheader()->label << std::endl;
    }
    if (!currentLoop->getLatches().empty()) {
        std::cout << "[scev]   Latches: ";
        for (BasicBlock* latch : currentLoop->getLatches()) {
            std::cout << latch->label << " ";
        }
        std::cout << std::endl;
    }
    // 打印可解析的循环次数（若有）
    auto trip = getLoopTripCount();
    if (trip) {
        std::cout << "[scev]   TripCount: " << *trip << std::endl;
    }
    std::cout << std::endl;
    
    // 打印循环不变量
    std::cout << "[scev] Loop Invariants (" << invariantValues.size() << " found):" << std::endl;
    // if (invariantValues.empty()) {
    //     std::cout << "[scev]   None" << std::endl;
    // } else {
    //     for (Value* invariant : invariantValues) {
    //         if (invariant->name.empty()) {
    //             std::cout << "[scev]   <unnamed>";
    //         } else {
    //             std::cout << "[scev]   " << invariant->name;
    //         }
    //         if (auto* constVal = dynamic_cast<ConstantInt*>(invariant)) {
    //             std::cout << " (constant: " << constVal->value << ")";
    //         } else if (auto* constFloat = dynamic_cast<ConstantFloat*>(invariant)) {
    //             std::cout << " (constant: " << constFloat->value << ")";
    //         }
    //         std::cout << std::endl;
    //     }
    // }
    // std::cout << std::endl;
    
    // 打印循环变量详细信息
    auto& loopVars = getCurrentLoopVariables();
    std::cout << "[scev] Loop Variables (" << loopVars.size() << " found):" << std::endl;
    if (loopVars.empty()) {
        std::cout << "[scev]   None" << std::endl;
    } else {
        for (const auto& [var, info] : loopVars) {
            std::cout << "[scev]   Variable: " << var->name;
            if (info.isPrimary) {
                std::cout << " [PRIMARY]";
            }
            std::cout << std::endl;
            std::cout << "[scev]     Type: " << (info.isSimple ? "Simple" : "Complex") 
                     << " " << (info.isIncrement ? "Increment" : "Decrement") << std::endl;
            
            if (info.initialValue) {
                std::cout << "[scev]     Initial Value: " << info.initialValue->name;
                if (auto* constVal = dynamic_cast<ConstantInt*>(info.initialValue)) {
                    std::cout << " (" << constVal->value << ")";
                } else if (auto* constFloat = dynamic_cast<ConstantFloat*>(info.initialValue)) {
                    std::cout << " (" << constFloat->value << ")";
                }
                std::cout << std::endl;
            }
            
            if (info.stepValue) {
                std::cout << "[scev]     Step/Factor: " << info.stepValue->name;
                if (auto* constVal = dynamic_cast<ConstantInt*>(info.stepValue)) {
                    std::cout << " (" << constVal->value << ")";
                } else if (auto* constFloat = dynamic_cast<ConstantFloat*>(info.stepValue)) {
                    std::cout << " (" << constFloat->value << ")";
                }
                std::cout << std::endl;
            }
            
            if (info.lowerBound) {
                std::cout << "[scev]     Lower Bound: " << info.lowerBound->name;
                if (auto* constVal = dynamic_cast<ConstantInt*>(info.lowerBound)) {
                    std::cout << " (" << constVal->value << ")";
                }
                std::cout << std::endl;
            }
            
            if (info.upperBound) {
                std::cout << "[scev]     Upper Bound: " << info.upperBound->name;
                if (auto* constVal = dynamic_cast<ConstantInt*>(info.upperBound)) {
                    std::cout << " (" << constVal->value << ")";
                }
                std::cout << std::endl;
            }
            
            if (info.exitCondition != ICmpCond::EQ) {
                std::cout << "[scev]     Exit Condition: " << getICmpConditionName(info.exitCondition) << std::endl;
            }
            if (info.constantTripCount) {
                std::cout << "[scev]     TripCount: " << *info.constantTripCount << std::endl;
            }
            
            if (info.scev) {
                std::cout << "[scev]     SCEV Expression: " << info.scev->toString() << std::endl;
            }
            std::cout << std::endl;
        }
    }
    
    // 打印SCEV缓存
    std::cout << "[scev] SCEV Cache (" << scevCache.size() << " entries):" << std::endl;
    if (scevCache.empty()) {
        std::cout << "[scev]   Empty" << std::endl;
    } else {
        for (const auto& [value, scev] : scevCache) {
            std::cout << "[scev]   " << value->name << " -> " << scev->toString() << std::endl;
        }
    }
    std::cout << std::endl;
    
    // 打印循环分析统计信息
    std::cout << "[scev] Analysis Statistics:" << std::endl;
    std::cout << "[scev]   Total loop invariants: " << invariantValues.size() << std::endl;
    std::cout << "[scev]   Total loop variables: " << getCurrentLoopVariables().size() << std::endl;
    std::cout << "[scev]   Total SCEV cache entries: " << scevCache.size() << std::endl;
    
    // 统计不同类型的循环变量
    int simpleVars = 0, complexVars = 0;
    int addRecVars = 0, mulRecVars = 0;
    
    for (const auto& [var, info] : getCurrentLoopVariables()) {
        if (info.isSimple) {
            simpleVars++;
            if (info.scev && dynamic_cast<SCEVAddRecExpr*>(info.scev.get())) {
                addRecVars++;
            }
        } else {
            complexVars++;
            if (info.scev && dynamic_cast<SCEVMulRecExpr*>(info.scev.get())) {
                mulRecVars++;
            }
        }
    }
    
    std::cout << "[scev]   Simple loop variables: " << simpleVars << std::endl;
    std::cout << "[scev]   Complex loop variables: " << complexVars << std::endl;
    std::cout << "[scev]   AddRec expressions: " << addRecVars << std::endl;
    std::cout << "[scev]   MulRec expressions: " << mulRecVars << std::endl;
    
    std::cout << "[scev] ========================================" << std::endl;
}

// 打印函数级别的分析总结
void SCEVAnalysis::printFunctionSummary() const {
    if (!function) return;
    
    std::cout << "[scev] ========================================" << std::endl;
    std::cout << "[scev] Function-level SCEV Analysis Summary" << std::endl;
    std::cout << "[scev] ========================================" << std::endl;
    std::cout << "[scev] Function: " << function->name << std::endl;
    std::cout << "[scev] Total SCEV cache entries: " << scevCache.size() << std::endl;
    // 统计所有循环的变量总数
    int totalLoopVars = 0;
    for (const auto& [loop, vars] : allLoopInfo) {
        totalLoopVars += vars.size();
    }
    std::cout << "[scev] Total loop variables across all loops: " << totalLoopVars << std::endl;
    std::cout << "[scev] Total loop invariants: " << invariantValues.size() << std::endl;
    
    // 统计不同类型的SCEV表达式
    int constants = 0, unknowns = 0, addExprs = 0, mulExprs = 0, addRecExprs = 0, mulRecExprs = 0;
    
    for (const auto& [value, scev] : scevCache) {
        switch (scev->type) {
            case SCEVType::Constant: constants++; break;
            case SCEVType::Unknown: unknowns++; break;
            case SCEVType::AddExpr: addExprs++; break;
            case SCEVType::MulExpr: mulExprs++; break;
            case SCEVType::AddRecExpr: addRecExprs++; break;
            case SCEVType::MulRecExpr: mulRecExprs++; break;
            default: break;
        }
    }
    
    std::cout << "[scev] SCEV Expression Types:" << std::endl;
    std::cout << "[scev]   Constants: " << constants << std::endl;
    std::cout << "[scev]   Unknowns: " << unknowns << std::endl;
    std::cout << "[scev]   Add Expressions: " << addExprs << std::endl;
    std::cout << "[scev]   Mul Expressions: " << mulExprs << std::endl;
    std::cout << "[scev]   AddRec Expressions: " << addRecExprs << std::endl;
    std::cout << "[scev]   MulRec Expressions: " << mulRecExprs << std::endl;
    
    // 统计主循环变量
    int primaryVars = 0;
    for (const auto& [loop, vars] : allLoopInfo) {
        for (const auto& [var, info] : vars) {
            if (info.isPrimary) {
                primaryVars++;
            }
        }
    }
    std::cout << "[scev] Primary loop variables: " << primaryVars << std::endl;
    
    std::cout << "[scev] ========================================" << std::endl;
}
} // namespace llvm_ir 