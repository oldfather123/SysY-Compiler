#include "../../include/ir/scev.h"
#include "../../include/ir/loop_analysis.h"
#include "../../include/ir/cfg.h"
#include <iostream>
#include <queue>
#include <algorithm>
#include <cassert>

namespace llvm_ir {
std::unique_ptr<SCEV> SCEVAnalysis::getSCEV(Value* value) {
    if (!value) {
        std::cout << "[scev]       getSCEV: null value" << std::endl;
        return nullptr;
    }

    // 如果当前有loop上下文且该值为循环外定义的指令，则优先按Unknown处理，避免跨循环缓存导致的错误AddRec复用
    if (currentLoop) {
        if (auto* inst = dynamic_cast<Instruction*>(value)) {
            if (!currentLoop->contains(inst)) {
                return std::make_unique<SCEVUnknown>(value);
            }
        }
    }

    // 检查缓存
    auto it = scevCache.find(value);
    if (it != scevCache.end()) {
        return it->second->clone();
    }

    // 检查是否正在计算中（避免无限递归）
    if (scevBeingComputed.find(value) != scevBeingComputed.end()) {
        std::cout << "[scev]       getSCEV: detected recursion for " << value->name 
                    << ", returning SCEVUnknown" << std::endl;
        return std::make_unique<SCEVUnknown>(value);
    }

    // 标记为正在计算
    scevBeingComputed.insert(value);

    // 创建新的SCEV表达式
    auto scev = createSCEVForValue(value);

    // 移除正在计算标记
    scevBeingComputed.erase(value);

    if (scev) {
        scevCache[value] = scev->clone();
    }
    return scev;
}

std::unique_ptr<SCEV> SCEVAnalysis::getSCEV(Instruction* inst) {
    if (!inst) return nullptr;

    // 如果当前有loop上下文且该指令为循环外定义，按Unknown处理
    if (currentLoop && !currentLoop->contains(inst)) {
        return std::make_unique<SCEVUnknown>(inst);
    }

    // 检查缓存
    auto it = scevCache.find(inst);
    if (it != scevCache.end()) {
        return it->second->clone();
    }

    // 创建新的SCEV表达式
    auto scev = createSCEVForInstruction(inst);
    if (scev) {
        scevCache[inst] = scev->clone();
    }

    return scev;
}

std::unique_ptr<SCEV> SCEVAnalysis::createSCEVForValue(Value* value) {
    if (!value) return nullptr;
    
    // 检查是否为常量
    if (auto* constInt = dynamic_cast<ConstantInt*>(value)) {
        return std::make_unique<SCEVConstant>(constInt->value, constInt->type);
    }
    
    if (auto* constFloat = dynamic_cast<ConstantFloat*>(value)) {
        // 对于浮点常量，我们暂时返回Unknown TODO(会有浮点常量吗)
        return std::make_unique<SCEVUnknown>(value);
    }
    
    // 检查是否为全局变量
    if (auto* globalVar = dynamic_cast<GlobalVariable*>(value)) {
        // 检查全局变量是否为循环不变量
        if (isLoopInvariant(globalVar)) {
            return std::make_unique<SCEVUnknown>(globalVar);  // 目前没办法知道全局变量是否是常量
        } else {
            return std::make_unique<SCEVUnknown>(globalVar);
        }
    }
    
    // 检查是否为指令
    if (auto* inst = dynamic_cast<Instruction*>(value)) {
        return createSCEVForInstruction(inst);
    }
    
    // 其他情况返回Unknown
    return std::make_unique<SCEVUnknown>(value);
}

std::unique_ptr<SCEV> SCEVAnalysis::createSCEVForInstruction(Instruction* inst) {
    if (!inst) return nullptr;
    
    switch (inst->opcode) { // 检查处理是否完善
        case Opcode::Add:
        case Opcode::FAdd:
        case Opcode::Sub:
        case Opcode::FSub:
        case Opcode::Mul:
        case Opcode::FMul:
        case Opcode::SDiv:
        case Opcode::UDiv:
        case Opcode::FDiv:
        case Opcode::SRem:
        case Opcode::URem:
        case Opcode::FRem:
            if (auto* binOp = dynamic_cast<BinaryOperator*>(inst)) {
                return createSCEVForBinaryOp(binOp);
            }
            break;
            
        case Opcode::PHI:
            if (auto* phi = dynamic_cast<PhiInst*>(inst)) {
                return createSCEVForPhi(phi);
            }
            break;
            
        case Opcode::Load:
            if (auto* loadInst = dynamic_cast<LoadInst*>(inst)) {
                Value* ptr = loadInst->getPointerOperand();
                if (auto* globalVar = dynamic_cast<GlobalVariable*>(ptr)) {
                    // 如果加载的是全局变量，检查该全局变量是否为循环不变量
                    if (isLoopInvariant(globalVar)) {
                        std::cout << "[scev]     Load from invariant global variable: " << globalVar->name << std::endl;
                        return std::make_unique<SCEVUnknown>(inst);
                    } else {
                        std::cout << "[scev]     Load from non-invariant global variable: " << globalVar->name << std::endl;
                        return std::make_unique<SCEVUnknown>(inst);
                    }
                }
            }
            break;
            
        default:
            break;
    }
    
    // 对于其他指令，返回Unknown
    return std::make_unique<SCEVUnknown>(inst);
}

std::unique_ptr<SCEV> SCEVAnalysis::createSCEVForBinaryOp(BinaryOperator* inst) {
    if (!inst || inst->operands.size() != 2) return nullptr;
    
    auto lhs = getSCEV(inst->operands[0]);
    auto rhs = getSCEV(inst->operands[1]);
    
    if (!lhs || !rhs) return nullptr;
    
    switch (inst->opcode) {
        case Opcode::Add:
        case Opcode::FAdd:
            return addSCEV(std::move(lhs), std::move(rhs));
            
        case Opcode::Sub:
        case Opcode::FSub:
            // 减法可以转换为加法：a - b = a + (-b)
            if (auto* constRhs = dynamic_cast<SCEVConstant*>(rhs.get())) {
                auto negRhs = std::make_unique<SCEVConstant>(-constRhs->value, constRhs->valueType);
                return addSCEV(std::move(lhs), std::move(negRhs));
            }
            break;
            
        case Opcode::Mul:
        case Opcode::FMul:
            return mulSCEV(std::move(lhs), std::move(rhs));
            
        default:
            break;
    }
    
    return std::make_unique<SCEVUnknown>(inst);
}

std::unique_ptr<SCEV> SCEVAnalysis::createSCEVForPhi(PhiInst* phi) {
    if (!phi || phi->incoming_values.empty()) return nullptr;
    
    // 检查是否为循环变量
    auto& loopVars = getCurrentLoopVariables();
    auto it = loopVars.find(phi);
    if (it != loopVars.end() && it->second.scev) {
        return it->second.scev->clone();
    }
    
    // 检查是否为循环不变量选择的PHI指令
    // 如果所有incoming values都是循环不变量，那么这个PHI也是循环不变量
    bool allInvariant = true;
    std::vector<std::unique_ptr<SCEV>> invariantSCEVs;
    
    for (const auto& incoming : phi->incoming_values) {
        Value* incomingValue = incoming.first;
        
        // 检查是否为循环不变量
        if (invariantValues.count(incomingValue) > 0) {
            // 获取该值的SCEV表达式
            auto scev = getSCEV(incomingValue);
            if (scev) {
                invariantSCEVs.push_back(std::move(scev));
            } else {
                allInvariant = false;
                break;
            }
        } else {
            allInvariant = false;
            break;
        }
    }
    
    if (allInvariant && !invariantSCEVs.empty()) {
        // 检查所有输入的不变量SCEV表达式是否相等
        bool allEqual = true;
        for (size_t i = 1; i < invariantSCEVs.size(); ++i) {
            if (!invariantSCEVs[0]->equals(invariantSCEVs[i].get())) {
                allEqual = false;
                break;
            }
        }
        
        if (allEqual) {
            // 只有当所有输入的不变量SCEV表达式都相等时，PHI的结果才是该不变量
            std::cout << "[scev]     Found invariant PHI: " << phi->name 
                        << " (all inputs are equal loop invariants)" << std::endl;
            return std::move(invariantSCEVs[0]);
        } else {
            // 如果输入的不变量不相等，PHI的结果不是确定的不变量
            std::cout << "[scev]     Found non-invariant PHI: " << phi->name 
                        << " (inputs are different loop invariants)" << std::endl;
            return std::make_unique<SCEVUnknown>(phi);
        }
    }
    
    // 对于其他PHI指令，返回Unknown
    return std::make_unique<SCEVUnknown>(phi);
}

std::unique_ptr<SCEV> SCEVAnalysis::simplifySCEV(std::unique_ptr<SCEV> scev) {
    if (!scev) return nullptr;
    
    // 简化常量表达式
    if (auto* constSCEV = dynamic_cast<SCEVConstant*>(scev.get())) {
        return scev; // 常量已经是简化形式
    }
    
    // 简化加法表达式
    if (auto* addSCEV = dynamic_cast<SCEVAddExpr*>(scev.get())) {
        std::vector<std::unique_ptr<SCEV>> simplified_ops;
        int64_t constant_sum = 0;
        
        for (auto& op : addSCEV->operands) {
            auto simplified_op = simplifySCEV(std::move(op));
            if (auto* const_op = dynamic_cast<SCEVConstant*>(simplified_op.get())) {
                constant_sum += const_op->value;
            } else {
                simplified_ops.push_back(std::move(simplified_op));
            }
        }
        
        // 添加常量部分
        if (constant_sum != 0) {
            simplified_ops.insert(simplified_ops.begin(), 
                                std::make_unique<SCEVConstant>(constant_sum));
        }
        
        if (simplified_ops.empty()) {
            return std::make_unique<SCEVConstant>(0);
        } else if (simplified_ops.size() == 1) {
            return std::move(simplified_ops[0]);
        } else {
            return std::make_unique<SCEVAddExpr>(std::move(simplified_ops), scev->valueType);
        }
    }
    
    return scev;
}

std::unique_ptr<SCEV> SCEVAnalysis::addSCEV(std::unique_ptr<SCEV> a, std::unique_ptr<SCEV> b) {
    if (!a || !b) return nullptr;
    
    // 如果两个都是常量，直接相加
    if (auto* constA = dynamic_cast<SCEVConstant*>(a.get())) {
        if (auto* constB = dynamic_cast<SCEVConstant*>(b.get())) {
            return std::make_unique<SCEVConstant>(constA->value + constB->value, constA->valueType);
        }
    }
    
    // 创建加法表达式
    std::vector<std::unique_ptr<SCEV>> operands;
    operands.push_back(std::move(a));
    operands.push_back(std::move(b));
    
    return std::make_unique<SCEVAddExpr>(std::move(operands), operands[0]->valueType);
}

std::unique_ptr<SCEV> SCEVAnalysis::mulSCEV(std::unique_ptr<SCEV> a, std::unique_ptr<SCEV> b) {
    if (!a || !b) return nullptr;
    
    // 如果两个都是常量，直接相乘
    if (auto* constA = dynamic_cast<SCEVConstant*>(a.get())) {
        if (auto* constB = dynamic_cast<SCEVConstant*>(b.get())) {
            return std::make_unique<SCEVConstant>(constA->value * constB->value, constA->valueType);
        }
    }
    
    // 创建乘法表达式
    std::vector<std::unique_ptr<SCEV>> operands;
    operands.push_back(std::move(a));
    operands.push_back(std::move(b));
    
    return std::make_unique<SCEVMulExpr>(std::move(operands), operands[0]->valueType);
}


}