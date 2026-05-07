#include "../../include/ir/constant_propagation.h"
#include "../../include/ir/cfg.h"
#include <unordered_map>
#include <set>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cassert>

using namespace llvm_ir;

namespace ir_opt {

// SSA常量传播格结构
enum class LatticeValue { UNDEF, CONSTANT, NAC };
struct LatticeState {
    LatticeValue type = LatticeValue::UNDEF;
    int32_t intVal = 0;
    float floatVal = 0.0;
    Type valueType = Type::I32;
    
    LatticeState() = default;
    LatticeState(int32_t v, Type t = Type::I32) : type(LatticeValue::CONSTANT), intVal(v), floatVal(0.0), valueType(t) {}
    LatticeState(float v, Type t = Type::Float) : type(LatticeValue::CONSTANT), intVal(0), floatVal(v), valueType(t) {}
    LatticeState(LatticeValue lv) : type(lv), intVal(0), floatVal(0.0), valueType(Type::I32) {}
    
    bool operator==(const LatticeState& rhs) const {
        if (type != rhs.type) return false;
        if (type == LatticeValue::CONSTANT) {
            if (valueType == Type::Float || valueType == Type::Double)
                return floatVal == rhs.floatVal && valueType == rhs.valueType;
            else
                return intVal == rhs.intVal && valueType == rhs.valueType;
        }
        return true;
    }
    bool operator!=(const LatticeState& rhs) const { return !(*this == rhs); }
};

// Meet函数：计算两个格状态的交汇
LatticeState meet(const LatticeState& a, const LatticeState& b) {
    if (a.type == LatticeValue::UNDEF) return b;
    if (b.type == LatticeValue::UNDEF) return a;
    if (a.type == LatticeValue::NAC || b.type == LatticeValue::NAC) 
        return LatticeState(LatticeValue::NAC);
    // Both are CONSTANT
    if (a == b) return a;
    return LatticeState(LatticeValue::NAC);
}

// 经典SCCP实现
bool sparseConditionalConstantPropagation(Module& module) {
    bool hasChanged = false;
    for (auto& funcPtr : module.functions) {
        auto& func = *funcPtr;
        if (func.isDeclaration()) continue;
        std::cout << "[SCCP] Function: " << func.name << std::endl;
        
        std::unordered_map<Value*, LatticeState> latticeValues; // 映射表，记录每个变量/表达式当前的状态（LatticeValue）
        std::set<BasicBlock*> reachableBlocks; // 记录哪些基本块（代码块）是“可达的”，也就是程序运行时有可能会执行到。
        std::vector<std::pair<Instruction*, BasicBlock*>> ssaEdgeWorklist; // “工作队列”，里面存放着“需要重新分析的指令和它所在的基本块”。SCCP 会不断从这里取出任务，直到没有新任务。

        // 初始化参数为NAC
        for (auto* param : func.parameters) {
            latticeValues[param] = LatticeState(LatticeValue::NAC);
        }

        // 初始化常量识别
        for (auto& bbPtr : func.basicBlocks) {
            for (auto& instPtr : bbPtr->instructions) {
                Instruction* inst = instPtr.get();
                if (inst->toString().empty()) continue;
                
                // 初始化所有指令为UNDEF
                latticeValues[inst] = LatticeState();
                
                // 识别常量指令
                if (auto ci = dynamic_cast<ConstantInt*>(inst)) {
                    latticeValues[inst] = LatticeState(ci->value, ci->type);
                    // std::cout << "[SCCP] Found constant: " << ci->value << std::endl;
                } else if (auto cf = dynamic_cast<ConstantFloat*>(inst)) {
                    latticeValues[inst] = LatticeState(cf->value, cf->type);
                    // std::cout << "[SCCP] Found constant: " << cf->value << std::endl;
                }
                
                // 识别指令操作数中的常量
                for (auto operand : inst->operands) {
                    if (auto ci = dynamic_cast<ConstantInt*>(operand)) {
                        latticeValues[operand] = LatticeState(ci->value, ci->type);
                        // std::cout << "[SCCP] Found constant operand: " << ci->value << std::endl;
                    } else if (auto cf = dynamic_cast<ConstantFloat*>(operand)) {
                        latticeValues[operand] = LatticeState(cf->value, cf->type);
                        // std::cout << "[SCCP] Found constant operand: " << cf->value << std::endl;
                    }
                }
            }
            
            // 处理PHI指令
            for (auto& phiPtr : bbPtr->phi_instructions) {
                PhiInst* phi = phiPtr.get();
                latticeValues[phi] = LatticeState();
                
                // 检查PHI指令的incoming_values中的常量
                for (auto& [val, pred] : phi->incoming_values) {
                    if (auto ci = dynamic_cast<ConstantInt*>(val)) {
                        latticeValues[val] = LatticeState(ci->value, ci->type);
                        // std::cout << "[SCCP] Found PHI constant: " << ci->value << std::endl;
                    } else if (auto cf = dynamic_cast<ConstantFloat*>(val)) {
                        latticeValues[val] = LatticeState(cf->value, cf->type);
                        // std::cout << "[SCCP] Found PHI constant: " << cf->value << std::endl;
                    }
                }
            }
        }

        if (func.basicBlocks.empty()) continue;
        auto* entry = func.basicBlocks[0].get();

        // 定义lambda函数
        auto markBlockReachable = [&](BasicBlock* bb) {
            if (!reachableBlocks.insert(bb).second) return;
            // std::cout << "[SCCP] Marking block " << bb->label << " as reachable" << std::endl;
            for (auto& phi : bb->phi_instructions) ssaEdgeWorklist.emplace_back(phi.get(), bb);
            for (auto& inst : bb->instructions) ssaEdgeWorklist.emplace_back(inst.get(), bb);
        };

        auto visitPhi = [&](PhiInst* phi, BasicBlock* parent) {
            // 如果包含 phi 的基本块不可达，跳过处理
            if (!reachableBlocks.count(parent)) {
                std::cout << "[SCCP] Skipping PHI " << phi->name << " in unreachable block " << parent->label << std::endl;
                return;
            }
            
            LatticeState oldState = latticeValues[phi];
            LatticeState meetResult;
            std::vector<std::pair<Value*, BasicBlock*>> reachableInputs;
            
            // 只收集来自可达前驱块的输入
            for (auto& [val, pred] : phi->incoming_values) {
                if (reachableBlocks.count(pred)) {
                    reachableInputs.push_back({val, pred});
                }
            }
            
            // 基于可达输入重新计算 PHI 值
            if (reachableInputs.empty()) {
                // 没有可达输入，保持 UNDEF
                meetResult.type = LatticeValue::UNDEF;
            } else if (reachableInputs.size() == 1) {
                // 只有一个可达输入，直接使用该值的状态
                auto& [val, pred] = reachableInputs[0];
                meetResult = latticeValues[val];
            } else {
                // 多个可达输入，使用 meet 合并
                meetResult.type = LatticeValue::UNDEF; // 初始化为 UNDEF
                for (auto& [val, pred] : reachableInputs) {
                    meetResult = meet(meetResult, latticeValues[val]);
                    // std::cout << "[SCCP] PHI " << phi->name << " meet with input from " << pred->label << ", current result: " << (int)meetResult.type << std::endl;
                }
                
                // 检查是否所有可达输入都是相同的常量
                if (reachableInputs.size() > 1 && meetResult.type == LatticeValue::CONSTANT) {
                    std::cout << "[SCCP] PHI " << phi->name << " folded to constant: " << meetResult.intVal << std::endl;
                }
            }
            
            // std::cout << "[SCCP] PHI " << phi->name << " final result: " << (int)meetResult.type;
            // if (meetResult.type == LatticeValue::CONSTANT) {
            //     std::cout << " (constant value: " << meetResult.intVal << ")";
            // }
            // std::cout << std::endl;
            
            // std::cout << "[SCCP] PHI " << phi->name << " old state: " << (int)oldState.type;
            // if (oldState.type == LatticeValue::CONSTANT) {
            //     std::cout << " (old constant value: " << oldState.intVal << ")";
            // }
            // std::cout << std::endl;
            
            if (meetResult != oldState) { //每次有变量/指令的状态发生变化，就把它的“用户”也加入工作队列
                latticeValues[phi] = meetResult;
                std::cout << "[SCCP] Updated PHI " << phi->name << " from " << (int)oldState.type << " to " << (int)meetResult.type << std::endl;
                for (auto* user : phi->getUses()) {
                    if (auto* uinst = dynamic_cast<Instruction*>(user))
                        ssaEdgeWorklist.emplace_back(uinst, parent);
                }
            }
        };

        auto addReachableEdge = [&](BasicBlock* from, BasicBlock* to) {
            if (!reachableBlocks.count(to)) markBlockReachable(to);
            for (auto& phi : to->phi_instructions) ssaEdgeWorklist.emplace_back(phi.get(), to);
        };
        
        // 分析分支指令（if/else），如果条件是常量，就只标记会被执行的分支为“可达”

        auto visitTerminator = [&](BranchInst* br, BasicBlock* parent) {
            if (br->operands.empty()) {
                // 无条件跳转
                auto* to = func.getBlockByName(br->trueLabel);
                if (to) addReachableEdge(parent, to);
            } else {
                auto cond = br->operands[0];
                auto state = latticeValues[cond];
                // std::cout << "[SCCP] Conditional branch, condition state: " << (int)state.type << std::endl;
                if (state.type == LatticeValue::CONSTANT) {
                    // 条件为常量，只有一个分支可达
                    std::string targetLabel = state.intVal ? br->trueLabel : br->falseLabel;
                    std::string deadLabel = state.intVal ? br->falseLabel : br->trueLabel;
                    auto* to = func.getBlockByName(targetLabel);
                    // std::cout << "[SCCP] Constant condition (" << state.intVal << "), branching to " << targetLabel << std::endl;
                    // std::cout << "[SCCP] Dead branch: " << deadLabel << " will NOT be marked reachable" << std::endl;
                    if (to) addReachableEdge(parent, to);
                    
                    // 分支简化：将条件分支替换为无条件分支 TODO
                    // std::cout << "[SCCP] [TODO] Simplifying branch condition to " << (state.intVal ? "true" : "false") << std::endl;
                    // 这里可以添加分支简化逻辑，但需要更复杂的IR修改
                    // 不能在这里修改，因为icmp的值还没有完全确定是不是常量，可能第一次被判断是常量，第二次判断不是，比如phi指令的情况
                    // 而且后面貌似会处理，把条件分支改成无条件分支

                } else if (state.type == LatticeValue::NAC) {
                    // 条件不是常量，两个后继都可达
                    auto* t = func.getBlockByName(br->trueLabel);
                    auto* f = func.getBlockByName(br->falseLabel);
                    if (t) addReachableEdge(parent, t);
                    if (f) addReachableEdge(parent, f);
                } else {
                    // 条件为 UNDEF，不标记任何后继为可达，等待条件确定
                    // std::cout << "[SCCP] UNDEF condition, waiting for condition to be determined. Not marking any branch reachable yet." << std::endl;
                    // 不做任何操作，等待条件的lattice值更新后重新处理
                }
            }
        };

        auto visitInstruction = [&](Instruction* inst, BasicBlock* parent) {
            // 如果包含指令的基本块不可达，跳过处理
            if (!reachableBlocks.count(parent)) {
                // std::cout << "[SCCP] Skipping instruction " << inst->toString() << " in unreachable block " << parent->label << std::endl;
                return;
            }
            
            LatticeState oldState = latticeValues[inst];
            LatticeState newState;
            // std::cout << "[SCCP] Processing instruction: " << inst->toString() << " in block " << parent->label << std::endl;
            
            if (auto bin = dynamic_cast<BinaryOperator*>(inst)) {
                auto lhs = bin->operands[0];
                auto rhs = bin->operands[1];
                auto& l = latticeValues[lhs];
                auto& r = latticeValues[rhs];
                
                if (l.type == LatticeValue::NAC || r.type == LatticeValue::NAC) {
                    newState.type = LatticeValue::NAC;
                } else if (l.type == LatticeValue::CONSTANT && r.type == LatticeValue::CONSTANT) {
                    if (bin->type == Type::I32 || bin->type == Type::I1) {
                        int32_t res = 0;
                        switch (bin->opcode) {
                            case Opcode::Add: res = l.intVal + r.intVal; break;
                            case Opcode::Sub: res = l.intVal - r.intVal; break;
                            case Opcode::Mul: res = l.intVal * r.intVal; break;
                            case Opcode::UDiv: if (!r.intVal) { newState.type = LatticeValue::NAC; } else { res = l.intVal / r.intVal; } break;
                            case Opcode::SDiv: if (!r.intVal) { newState.type = LatticeValue::NAC; } else { res = l.intVal / r.intVal; } break;
                            case Opcode::URem: if (!r.intVal) { newState.type = LatticeValue::NAC; } else { res = l.intVal % r.intVal; } break;
                            case Opcode::SRem: if (!r.intVal) { newState.type = LatticeValue::NAC; } else { res = l.intVal % r.intVal; } break;
                            case Opcode::Shl: res = l.intVal << r.intVal; break;
                            case Opcode::LShr: res = (uint32_t)l.intVal >> r.intVal; break;
                            case Opcode::AShr: res = l.intVal >> r.intVal; break;
                            case Opcode::And: res = l.intVal & r.intVal; break;
                            case Opcode::Or:  res = l.intVal | r.intVal; break;
                            case Opcode::Xor: res = l.intVal ^ r.intVal; break;
                            default: newState.type = LatticeValue::NAC; break;
                        }
                        if (newState.type != LatticeValue::NAC) {
                            newState = LatticeState(res, bin->type);
                            std::cout << "[SCCP] Folded binary op to constant: " << res << std::endl;
                        }
                    } else if (bin->type == Type::Float) {
                        float res = 0.0;
                        switch (bin->opcode) {
                            case Opcode::FAdd: res = l.floatVal + r.floatVal; break;
                            case Opcode::FSub: res = l.floatVal - r.floatVal; break;
                            case Opcode::FMul: res = l.floatVal * r.floatVal; break;
                            case Opcode::FDiv: if (r.floatVal == 0.0f) { newState.type = LatticeValue::NAC; } else { res = l.floatVal / r.floatVal; } break;
                            case Opcode::FRem: if (r.floatVal == 0.0f) { newState.type = LatticeValue::NAC; } else { res = fmod(l.floatVal, r.floatVal); } break;
                            default: newState.type = LatticeValue::NAC; break;
                        }
                        if (newState.type != LatticeValue::NAC) {
                            newState = LatticeState(res, bin->type);
                            std::cout << "[SCCP] Folded binary op to constant: " << res << std::endl;
                        }
                    }
                } else if (l.type == LatticeValue::UNDEF || r.type == LatticeValue::UNDEF) {
                    newState.type = LatticeValue::UNDEF;
                }
            } else if (auto icmp = dynamic_cast<ICmpInst*>(inst)) {
                auto lhs = icmp->operands[0];
                auto rhs = icmp->operands[1];
                auto& l = latticeValues[lhs];
                auto& r = latticeValues[rhs];
                
                // std::cout << "[SCCP] ICmp " << inst->toString() << " lhs type: " << (int)l.type << " rhs type: " << (int)r.type << std::endl;
                
                if (l.type == LatticeValue::CONSTANT && r.type == LatticeValue::CONSTANT) {
                    bool res = false;
                    switch (icmp->condition) {
                        case ICmpCond::EQ:  res = (l.intVal == r.intVal); break;
                        case ICmpCond::NE:  res = (l.intVal != r.intVal); break;
                        case ICmpCond::SGT: res = (l.intVal > r.intVal); break;
                        case ICmpCond::SGE: res = (l.intVal >= r.intVal); break;
                        case ICmpCond::SLT: res = (l.intVal < r.intVal); break;
                        case ICmpCond::SLE: res = (l.intVal <= r.intVal); break;
                        default: newState.type = LatticeValue::NAC; break;
                    }
                    if (newState.type != LatticeValue::NAC) {
                        newState = LatticeState((int32_t)res, Type::I1);
                        std::cout << "[SCCP] Folded icmp to constant: " << res << std::endl;
                    }
                } else if (l.type == LatticeValue::NAC || r.type == LatticeValue::NAC) {
                    newState.type = LatticeValue::NAC;
                } else if (l.type == LatticeValue::UNDEF || r.type == LatticeValue::UNDEF) {
                    newState.type = LatticeValue::UNDEF;
                }
            } else if (auto fcmp = dynamic_cast<FCmpInst*>(inst)) {
                auto lhs = fcmp->operands[0];
                auto rhs = fcmp->operands[1];
                auto& l = latticeValues[lhs];
                auto& r = latticeValues[rhs];
                if (l.type == LatticeValue::CONSTANT && r.type == LatticeValue::CONSTANT) {
                    bool res = false;
                    switch (fcmp->condition) {
                        case FCmpCond::OEQ: res = (l.floatVal == r.floatVal); break;
                        case FCmpCond::ONE: res = (l.floatVal != r.floatVal); break;
                        case FCmpCond::OGT: res = (l.floatVal > r.floatVal); break;
                        case FCmpCond::OGE: res = (l.floatVal >= r.floatVal); break;
                        case FCmpCond::OLT: res = (l.floatVal < r.floatVal); break;
                        case FCmpCond::OLE: res = (l.floatVal <= r.floatVal); break;
                        case FCmpCond::UEQ: res = (l.floatVal == r.floatVal); break;
                        case FCmpCond::UNE: res = (l.floatVal != r.floatVal); break;
                        case FCmpCond::UGT: res = (l.floatVal > r.floatVal); break;
                        case FCmpCond::UGE: res = (l.floatVal >= r.floatVal); break;
                        case FCmpCond::ULT: res = (l.floatVal < r.floatVal); break;
                        case FCmpCond::ULE: res = (l.floatVal <= r.floatVal); break;
                        default: newState.type = LatticeValue::NAC; break;
                    }
                    if (newState.type != LatticeValue::NAC) {
                        newState = LatticeState((int32_t)res, Type::I1);
                        std::cout << "[SCCP] Folded fcmp to constant: " << res << std::endl;
                    }
                } else if (l.type == LatticeValue::NAC || r.type == LatticeValue::NAC) {
                    newState.type = LatticeValue::NAC;
                } else if (l.type == LatticeValue::UNDEF || r.type == LatticeValue::UNDEF) {
                    newState.type = LatticeValue::UNDEF;
                }
            } else if (auto zext = dynamic_cast<ZExtInst*>(inst)) {
                // 零扩展指令
                auto operand = zext->operands[0];
                auto& opState = latticeValues[operand];
                
                if (opState.type == LatticeValue::CONSTANT) {
                    // 零扩展常量
                    int32_t res = opState.intVal; // 零扩展不改变值
                    newState = LatticeState(res, zext->type);
                    std::cout << "[SCCP] Folded zext to constant: " << res << std::endl;
                } else if (opState.type == LatticeValue::NAC) {
                    newState.type = LatticeValue::NAC;
                } else if (opState.type == LatticeValue::UNDEF) {
                    newState.type = LatticeValue::UNDEF;
                }
            } else if (inst->opcode == Opcode::SExt) {
                // 符号扩展指令 - 通过opcode识别
                auto operand = inst->operands[0];
                auto& opState = latticeValues[operand];
                
                if (opState.type == LatticeValue::CONSTANT) {
                    // 符号扩展常量
                    int32_t res = opState.intVal; // 符号扩展不改变值
                    newState = LatticeState(res, inst->type);
                    std::cout << "[SCCP] Folded sext to constant: " << res << std::endl;
                } else if (opState.type == LatticeValue::NAC) {
                    newState.type = LatticeValue::NAC;
                } else if (opState.type == LatticeValue::UNDEF) {
                    newState.type = LatticeValue::UNDEF;
                }
            } else if (inst->opcode == Opcode::Trunc) {
                // 截断指令
                auto operand = inst->operands[0];
                auto& opState = latticeValues[operand];
                
                if (opState.type == LatticeValue::CONSTANT) {
                    // 截断常量
                    int32_t res = opState.intVal;
                    if (inst->type == Type::I1) {
                        res = (opState.intVal & 1);
                    }
                    newState = LatticeState(res, inst->type);
                    std::cout << "[SCCP] Folded trunc to constant: " << res << std::endl;
                } else if (opState.type == LatticeValue::NAC) {
                    newState.type = LatticeValue::NAC;
                } else if (opState.type == LatticeValue::UNDEF) {
                    newState.type = LatticeValue::UNDEF;
                }
            } else if (inst->opcode == Opcode::UIToFP) {
                // 无符号整数转浮点
                auto operand = inst->operands[0];
                auto& opState = latticeValues[operand];
                
                if (opState.type == LatticeValue::CONSTANT) {
                    float res = static_cast<float>((uint64_t)opState.intVal);
                    newState = LatticeState(res, inst->type);
                    std::cout << "[SCCP] Folded uitofp to constant: " << res << std::endl;
                } else if (opState.type == LatticeValue::NAC) {
                    newState.type = LatticeValue::NAC;
                } else if (opState.type == LatticeValue::UNDEF) {
                    newState.type = LatticeValue::UNDEF;
                }
            } else if (inst->opcode == Opcode::FPToUI) {
                // 浮点转无符号整数
                auto operand = inst->operands[0];
                auto& opState = latticeValues[operand];
                
                if (opState.type == LatticeValue::CONSTANT) {
                    int32_t res = static_cast<int32_t>((uint64_t)opState.floatVal);
                    newState = LatticeState(res, inst->type);
                    std::cout << "[SCCP] Folded fptoui to constant: " << res << std::endl;
                } else if (opState.type == LatticeValue::NAC) {
                    newState.type = LatticeValue::NAC;
                } else if (opState.type == LatticeValue::UNDEF) {
                    newState.type = LatticeValue::UNDEF;
                }
            } else if (inst->opcode == Opcode::FPTrunc) {
                // 浮点截断
                auto operand = inst->operands[0];
                auto& opState = latticeValues[operand];
                
                if (opState.type == LatticeValue::CONSTANT) {
                    float res = static_cast<float>(opState.floatVal);
                    newState = LatticeState(static_cast<float>(res), inst->type);
                    std::cout << "[SCCP] Folded fptrunc to constant: " << res << std::endl;
                } else if (opState.type == LatticeValue::NAC) {
                    newState.type = LatticeValue::NAC;
                } else if (opState.type == LatticeValue::UNDEF) {
                    newState.type = LatticeValue::UNDEF;
                }
            } else if (inst->opcode == Opcode::FPExt) {
                // 浮点扩展
                auto operand = inst->operands[0];
                auto& opState = latticeValues[operand];
                
                if (opState.type == LatticeValue::CONSTANT) {
                    float res = static_cast<float>(opState.floatVal);
                    newState = LatticeState(res, inst->type);
                    std::cout << "[SCCP] Folded fpext to constant: " << res << std::endl;
                } else if (opState.type == LatticeValue::NAC) {
                    newState.type = LatticeValue::NAC;
                } else if (opState.type == LatticeValue::UNDEF) {
                    newState.type = LatticeValue::UNDEF;
                }
            } else if (inst->opcode == Opcode::PtrToInt) {
                // 指针转整数 - 通常不能折叠为常量
                newState.type = LatticeValue::NAC;
                // std::cout << "[SCCP] PtrToInt instruction marked as NAC: " << inst->toString() << std::endl;
            } else if (inst->opcode == Opcode::IntToPtr) {
                // 整数转指针 - 通常不能折叠为常量
                newState.type = LatticeValue::NAC;
                // std::cout << "[SCCP] IntToPtr instruction marked as NAC: " << inst->toString() << std::endl;
            } else if (inst->opcode == Opcode::BitCast) {
                // 位转换 - 通常不能折叠为常量
                newState.type = LatticeValue::NAC;
                // std::cout << "[SCCP] BitCast instruction marked as NAC: " << inst->toString() << std::endl;
            } else if (inst->opcode == Opcode::Select) {
                // Select指令 - 条件选择
                if (inst->operands.size() >= 3) {
                    auto cond = inst->operands[0];
                    auto trueVal = inst->operands[1];
                    auto falseVal = inst->operands[2];
                    auto& condState = latticeValues[cond];
                    auto& trueState = latticeValues[trueVal];
                    auto& falseState = latticeValues[falseVal];
                    
                    if (condState.type == LatticeValue::CONSTANT) {
                        // 条件为常量
                        if (condState.intVal) {
                            // 选择true值
                            newState = trueState;
                            std::cout << "[SCCP] Folded select to true value" << std::endl;
                        } else {
                            // 选择false值
                            newState = falseState;
                            std::cout << "[SCCP] Folded select to false value" << std::endl;
                        }
                    } else if (condState.type == LatticeValue::NAC) {
                        newState = meet(trueState, falseState);
                    } else if (condState.type == LatticeValue::UNDEF) {
                        newState.type = LatticeValue::UNDEF;
                    }
                } else {
                    newState.type = LatticeValue::NAC;
                }
            } else if (auto sitofp = dynamic_cast<SIToFPInst*>(inst)) {
                // 有符号整数转浮点
                auto operand = sitofp->operands[0];
                auto& opState = latticeValues[operand];
                
                if (opState.type == LatticeValue::CONSTANT) {
                    float res = static_cast<float>(opState.intVal);
                    newState = LatticeState(res, sitofp->type);
                    std::cout << "[SCCP] Folded sitofp to constant: " << res << std::endl;
                } else if (opState.type == LatticeValue::NAC) {
                    newState.type = LatticeValue::NAC;
                } else if (opState.type == LatticeValue::UNDEF) {
                    newState.type = LatticeValue::UNDEF;
                }
            } else if (auto fptosi = dynamic_cast<FPToSIInst*>(inst)) {
                // 浮点转有符号整数
                auto operand = fptosi->operands[0];
                auto& opState = latticeValues[operand];
                
                if (opState.type == LatticeValue::CONSTANT) {
                    int32_t res = static_cast<int32_t>(opState.floatVal);
                    newState = LatticeState(res, fptosi->type);
                    std::cout << "[SCCP] Folded fptosi to constant: " << res << std::endl;
                } else if (opState.type == LatticeValue::NAC) {
                    newState.type = LatticeValue::NAC;
                } else if (opState.type == LatticeValue::UNDEF) {
                    newState.type = LatticeValue::UNDEF;
                }
            } else if (auto load = dynamic_cast<LoadInst*>(inst)) {
                // Load指令 - 通常不能折叠为常量，除非我们知道指针指向的常量
                // 对于简单的常量传播，我们将其标记为NAC
                newState.type = LatticeValue::NAC;
                // std::cout << "[SCCP] Load instruction marked as NAC: " << inst->toString() << std::endl;
            } else if (auto store = dynamic_cast<StoreInst*>(inst)) {
                // Store指令 - 不产生值，不需要处理
                return;
            } else if (auto alloca = dynamic_cast<AllocaInst*>(inst)) {
                // Alloca指令 - 分配内存，不能折叠为常量
                newState.type = LatticeValue::NAC;
                // std::cout << "[SCCP] Alloca instruction marked as NAC: " << inst->toString() << std::endl;
            } else if (auto gep = dynamic_cast<GetElementPtrInst*>(inst)) {
                // GetElementPtr指令 - 地址计算，通常不能折叠为常量
                newState.type = LatticeValue::NAC;
                // std::cout << "[SCCP] GEP instruction marked as NAC: " << inst->toString() << std::endl;
            } else if (auto call = dynamic_cast<CallInst*>(inst)) {
                // Call指令 - 函数调用，通常不能折叠为常量
                // 除非是内联的纯函数，但这里我们简化处理
                newState.type = LatticeValue::NAC;
                // std::cout << "[SCCP] Call instruction marked as NAC: " << inst->toString() << std::endl;
            } else if (auto ret = dynamic_cast<ReturnInst*>(inst)) {
                // Return指令 - 不产生值，不需要处理
                return;
            } else if (inst->opcode == Opcode::Switch) {
                // Switch指令 - 通常不能折叠为常量，但可以用于控制流分析
                newState.type = LatticeValue::NAC;
                // std::cout << "[SCCP] Switch instruction marked as NAC: " << inst->toString() << std::endl;
            } else if (auto phi = dynamic_cast<PhiInst*>(inst)) {
                visitPhi(phi, parent);
                return;
            } else if (auto br = dynamic_cast<BranchInst*>(inst)) {
                visitTerminator(br, parent);
                return;
            } else {
                // 其他未处理的指令类型
                newState.type = LatticeValue::NAC;
                std::cerr << "[SCCP][Warning] Unhandled instruction type marked as NAC: " << inst->toString() 
                          << " (opcode: " << (int)inst->opcode << ")" << std::endl;
            }
            if (newState != oldState) {
                latticeValues[inst] = newState;
                // std::cout << "[SCCP] Updated " << inst->toString() << " from state type " << (int)oldState.type << " to " << (int)newState.type << std::endl;
                for (auto* user : inst->getUses()) {
                    if (auto* uinst = dynamic_cast<Instruction*>(user))
                        ssaEdgeWorklist.emplace_back(uinst, parent);
                }
            }
        };

        // 启动
        markBlockReachable(entry);

        // 主循环
        while (!ssaEdgeWorklist.empty()) {
            auto [inst, parent] = ssaEdgeWorklist.back();
            ssaEdgeWorklist.pop_back();
            if (auto* phi = dynamic_cast<PhiInst*>(inst)) {
                visitPhi(phi, parent);
            } else {
                visitInstruction(inst, parent);
            }
        }

        // 替换和死代码删除阶段
        std::set<Instruction*> toDelete;
        
        for (auto& bbPtr : func.basicBlocks) {
            if (reachableBlocks.find(bbPtr.get()) == reachableBlocks.end()) {
                std::cout << "[SCCP] Skip unreachable block: " << bbPtr->label << std::endl;
                continue; // 跳过不可达块
            }
            // 处理 PHI 指令
            for (auto& phiPtr : bbPtr->phi_instructions) {
                Instruction* inst = phiPtr.get();
                auto it = latticeValues.find(inst);
                // 如果phi的incoming_values是常量，则替换为常量
                if (it != latticeValues.end() && it->second.type == LatticeValue::CONSTANT) {
                    Value* c = nullptr;
                    if (it->second.valueType == Type::Float || it->second.valueType == Type::Double)
                        c = new ConstantFloat(it->second.floatVal, it->second.valueType);
                    else
                        c = new ConstantInt(it->second.intVal, it->second.valueType);
                    std::cout << "[SCCP] Replace PHI " << inst->toString() << " with constant " << it->second.intVal << std::endl;
                    
                    // 先替换所有uses，再标记删除
                    inst->replaceAllUsesWith(c);
                    hasChanged = true;
                    toDelete.insert(inst);
                }
            }
            
            // 处理普通指令
            for (auto& instPtr : bbPtr->instructions) {
                Instruction* inst = instPtr.get();
                auto it = latticeValues.find(inst);
                if (it != latticeValues.end() && it->second.type == LatticeValue::CONSTANT) {
                    Value* c = nullptr;
                    if (it->second.valueType == Type::Float || it->second.valueType == Type::Double)
                        c = new ConstantFloat(it->second.floatVal, it->second.valueType);
                    else
                        c = new ConstantInt(it->second.intVal, it->second.valueType);
                    std::cout << "[SCCP] Replace " << inst->toString() << " with constant " << it->second.intVal << std::endl;
                    inst->replaceAllUsesWith(c);
                    hasChanged = true;
                    toDelete.insert(inst);
                }
            }
        }
        
        // 基于当前CFG重新计算从入口可达的基本块集合，避免早期NAC导致的过度可达标记
        auto recomputeReachable = [&]() -> std::set<BasicBlock*> {
            std::set<BasicBlock*> newReachable;
            if (func.basicBlocks.empty()) return newReachable;
            std::vector<BasicBlock*> worklist;
            BasicBlock* entryBB = func.basicBlocks[0].get();
            worklist.push_back(entryBB);
            while (!worklist.empty()) {
                BasicBlock* bb = worklist.back();
                worklist.pop_back();
                if (!newReachable.insert(bb).second) continue;
                if (bb->instructions.empty()) continue;
                Instruction* term = bb->instructions.back().get();
                if (auto* br = dynamic_cast<BranchInst*>(term)) {
                    if (br->operands.empty()) {
                        if (auto* to = func.getBlockByName(br->trueLabel)) worklist.push_back(to);
                    } else {
                        Value* cond = br->operands[0];
                        if (auto* ci = dynamic_cast<ConstantInt*>(cond)) {
                            const std::string& target = ci->value ? br->trueLabel : br->falseLabel;
                            if (auto* to = func.getBlockByName(target)) worklist.push_back(to);
                        } else {
                            if (auto* t = func.getBlockByName(br->trueLabel)) worklist.push_back(t);
                            if (auto* f = func.getBlockByName(br->falseLabel)) worklist.push_back(f);
                        }
                    }
                } else if (term->opcode == Opcode::Switch) {
                    // 保守处理：对Switch不判断具体case，保持现状（如需更精确可扩展）
                    // 不添加任何后继以避免错误假阳性，后续分支修复和删除会处理
                }
            }
            return newReachable;
        };
        std::set<BasicBlock*> recomputedReachable = recomputeReachable();
        
        // 删除不可达块 - 首先收集不可达块的指针（基于重算的可达集合）
        std::vector<BasicBlock*> unreachableBlocks;
        std::set<std::string> unreachableLabels;
        for (auto& bbPtr : func.basicBlocks) {
            if (!recomputedReachable.count(bbPtr.get())) {
                unreachableBlocks.push_back(bbPtr.get());
                unreachableLabels.insert(bbPtr->label);
                std::cout << "[SCCP] Found unreachable block: " << bbPtr->label << std::endl;
            }
        }
        
        // 修复分支指令中对不可达块的引用（优先使用重算可达结果）
        auto createDefaultRetValue = [&](Function& F) -> Value* {
            switch (F.returnType) {
                case Type::Void:  return nullptr;
                case Type::I1:    return new ConstantInt(0, Type::I1);
                case Type::I32:   return new ConstantInt(0, Type::I32);
                case Type::Float: return new ConstantFloat(0.0f, Type::Float);
                default:          return new UndefValue(F.returnType);
            }
        };
        for (auto& bbPtr : func.basicBlocks) {
            if (recomputedReachable.count(bbPtr.get()) && !bbPtr->instructions.empty()) {
                auto& lastInst = bbPtr->instructions.back();
                if (auto br = dynamic_cast<BranchInst*>(lastInst.get())) {
                    // 检查条件分支是否引用了不可达块
                    if (!br->operands.empty()) {
                        auto cond = br->operands[0];
                        int condConstFlag = -1; // -1 unknown, 0 false, 1 true
                        if (auto* ciCond = dynamic_cast<ConstantInt*>(cond)) {
                            condConstFlag = (ciCond->value != 0) ? 1 : 0;
                        } else {
                            auto it = latticeValues.find(cond);
                            if (it != latticeValues.end() && it->second.type == LatticeValue::CONSTANT) {
                                condConstFlag = (it->second.intVal != 0) ? 1 : 0;
                            }
                        }
                        if (condConstFlag != -1) {
                            // 条件是常量，应该已经简化过了
                            std::string targetLabel = condConstFlag ? br->trueLabel : br->falseLabel;
                            std::string deadLabel = condConstFlag ? br->falseLabel : br->trueLabel;
                            
                            if (unreachableLabels.count(deadLabel)) {
                                std::cout << "[SCCP] Fixing branch in " << bbPtr->label 
                                          << ", removing reference to unreachable " << deadLabel << std::endl;
                                // 将条件分支替换为无条件分支
                                bbPtr->getTerminator()->removeFromParent(); // 这里已经删除了br指令，不需要再加入toDelete
                                bbPtr->addInstruction(std::make_unique<BranchInst>(targetLabel));
                            }
                        } else {
                            // 条件不是常量，检查两个分支是否都可达
                            if (unreachableLabels.count(br->trueLabel) || unreachableLabels.count(br->falseLabel)) {
                                std::cout << "[SCCP][Warning] Non-constant branch references unreachable block in " 
                                          << bbPtr->label << std::endl;
                                std::cout << "[SCCP][Warning] Branch instruction: " << br->toString() << std::endl;
                                // 对于非常量条件但引用了不可达块的情况，我们需要修复
                                // 找到可达的后继块作为替代
                                std::string fallbackLabel = "";
                                if (!unreachableLabels.count(br->trueLabel)) {
                                    fallbackLabel = br->trueLabel;
                                } else if (!unreachableLabels.count(br->falseLabel)) {
                                    fallbackLabel = br->falseLabel;
                                } else {
                                    std::cerr << "[SCCP][Warning] Both branches are unreachable!! Converting to return." << std::endl;
                                    // 两个分支都不可达：用一个合法的返回终止该块，避免制造无来源的边
                                    Value* retVal = createDefaultRetValue(func);
                                    bbPtr->getTerminator()->removeFromParent();
                                    bbPtr->addInstruction(std::unique_ptr<Instruction>(new ReturnInst(retVal)));
                                    hasChanged = true;
                                    continue; // 当前块已修复为ret
                                }
                                
                                if (!fallbackLabel.empty()) {
                                    std::cout << "[SCCP][Warning] Fixing non-constant branch in " << bbPtr->label 
                                              << ", redirecting to " << fallbackLabel << std::endl;
                                    // 将条件分支替换为无条件分支到可达的后继
                                    bbPtr->getTerminator()->removeFromParent(); // 这里已经删除了br指令，不需要再加入toDelete
                                    bbPtr->addInstruction(std::make_unique<BranchInst>(fallbackLabel));
                                }
                            }
                        }
                    } else {
                        // 无条件分支
                        if (unreachableLabels.count(br->trueLabel)) {
                            std::cerr << "[SCCP][Error] Unconditional branch to unreachable block " 
                                      << br->trueLabel << " in " << bbPtr->label << std::endl;
                            // 两个分支情形不存在，这里唯一目标不可达：直接把块终止成return，避免创造假边
                            Value* retVal = createDefaultRetValue(func);
                            bbPtr->getTerminator()->removeFromParent();
                            bbPtr->addInstruction(std::unique_ptr<Instruction>(new ReturnInst(retVal)));
                            hasChanged = true;
                        }
                    }
                }
            }
        }
        
        // 从所有 PHI 节点中移除对不可达块的引用
        for (auto& bbPtr : func.basicBlocks) {
            if (recomputedReachable.count(bbPtr.get())) {
                for (auto& phiPtr : bbPtr->phi_instructions) {
                    PhiInst* phi = phiPtr.get();
                    auto& incomingValues = phi->incoming_values;
                    
                    // 移除来自不可达块的 incoming value
                    auto originalSize = incomingValues.size();
                    incomingValues.erase(
                        std::remove_if(incomingValues.begin(), incomingValues.end(),
                            [&unreachableBlocks](const std::pair<Value*, BasicBlock*>& pair) {
                                return std::find(unreachableBlocks.begin(), unreachableBlocks.end(), pair.second) != unreachableBlocks.end();
                            }),
                        incomingValues.end()
                    );
                    
                    if (incomingValues.size() != originalSize) {
                        std::cout << "[SCCP] Cleaned up PHI " << phi->name 
                                  << ", removed " << (originalSize - incomingValues.size()) << " unreachable incoming values" << std::endl;
                        hasChanged = true;
                    }
                }
            }
        }
        
        //如果 PHI 指令的所有 incoming_values 都是undef, 删除
        for (auto& bbPtr : func.basicBlocks) {
            // 处理 PHI 指令
            for (auto& phiPtr : bbPtr->phi_instructions) {
                Instruction* inst = phiPtr.get();
                // 检查所有 incoming_values 是否都是 UndefValue 类型
                bool allUndef = true;
                for (auto& [val, pred] : phiPtr->incoming_values) {
                    if (dynamic_cast<UndefValue*>(val) == nullptr) {
                        allUndef = false;
                        break;
                    }
                }
                if (allUndef) {
                    Value* c =  new UndefValue(inst->type);
                    std::cout << "[SCCP] Replace PHI " << inst->toString() << " with UndefValue because all incoming values are UndefValue" << std::endl;
                    
                    // 先替换所有uses，再标记删除
                    inst->replaceAllUsesWith(c);
                    hasChanged = true;
                    std::cout << "[SCCP] Remove PHI " << inst->toString() << " because all incoming values are UndefValue" << std::endl;
                    toDelete.insert(inst);
                }
            }
        }
        
        // 再次重算可达性，考虑上一步对终止指令的修改
        std::set<BasicBlock*> finalReachable = recomputeReachable();

        // 额外的 PHI 清理：移除不再是前驱的来边；如果PHI只剩一个来边则折叠
        auto hasEdgeTo = [&](BasicBlock* from, const std::string& toLabel) -> bool {
            if (!from || from->instructions.empty()) return false;
            Instruction* term = from->instructions.back().get();
            if (auto* br = dynamic_cast<BranchInst*>(term)) {
                if (br->operands.empty()) {
                    return br->trueLabel == toLabel;
                } else {
                    return br->trueLabel == toLabel || br->falseLabel == toLabel;
                }
            }
            // ret 或其他终止：无后继
            return false;
        };

        for (auto& bbPtr : func.basicBlocks) {
            if (!finalReachable.count(bbPtr.get())) continue;
            for (auto& phiPtr : bbPtr->phi_instructions) {
                PhiInst* phi = phiPtr.get();
                auto& incoming = phi->incoming_values;
                size_t before = incoming.size();
                incoming.erase(
                    std::remove_if(incoming.begin(), incoming.end(), [&](const std::pair<Value*, BasicBlock*>& pr) {
                        BasicBlock* pred = pr.second;
                        return !finalReachable.count(pred) || !hasEdgeTo(pred, bbPtr->label);
                    }),
                    incoming.end()
                );
                if (incoming.size() != before) {
                    std::cout << "[SCCP] PHI cleaned dangling preds in block " << bbPtr->label << ", removed "
                              << (before - incoming.size()) << " entries" << std::endl;
                    hasChanged = true;
                }
                if (incoming.size() == 1) {
                    Value* onlyVal = incoming[0].first;
                    std::cout << "[SCCP] Fold PHI " << phi->name << " in block " << bbPtr->label << " to its only incoming value" << std::endl;
                    phi->replaceAllUsesWith(onlyVal);
                    toDelete.insert(phi);
                    hasChanged = true;
                } else if (incoming.empty()) {
                    // 没有任何来边，替换为undef
                    std::cout << "[SCCP] Remove PHI " << phi->name << " in block " << bbPtr->label << " due to no incoming edges, replace with undef" << std::endl;
                    Value* uv = new UndefValue(phi->type);
                    phi->replaceAllUsesWith(uv);
                    toDelete.insert(phi);
                    hasChanged = true;
                }
            }
        }

        // 重建基本块列表，只保留可达块（使用最终重算的可达集合）
        std::vector<std::unique_ptr<BasicBlock>> reachableBlocksVec;
        reachableBlocksVec.reserve(finalReachable.size());
        
        for (auto& bbPtr : func.basicBlocks) {
            if (finalReachable.count(bbPtr.get())) {
                reachableBlocksVec.push_back(std::move(bbPtr));
            } else {
                std::cout << "[SCCP] Removing unreachable block: " << bbPtr->label << std::endl;
                hasChanged = true;
                // bbPtr 会在作用域结束时自动释放
            }
        }
        
        // 先删除所有可删除指令
        for (auto* inst : toDelete) {
            // 安全检查：确保指令还未被删除
            if (!inst->parent) {
                std::cout << "[SCCP][Warning] Instruction " << inst->toString() << " has already been deleted, skipping" << std::endl;
                continue;
            }
            std::cout << "[SCCP] Delete instruction: " << inst->toString() << std::endl;
            inst->removeFromParent();
            hasChanged = true;
        }
        // 再替换函数的基本块列表
        func.basicBlocks = std::move(reachableBlocksVec);
        // 结构性修改后，重建CFG与use-def链，保证后续Pass的鲁棒性
        llvm_ir::cfg::buildCFG(func);
        llvm_ir::cfg::rebuildUseDefChainsOnFunction(func, /*debug=*/false);
        std::cout << "[SCCP] Removed unreachable blocks, remaining blocks: " << func.basicBlocks.size() << std::endl;
        std::cout << "[SCCP] Final reachable blocks: ";
        for (auto* bb : finalReachable) {
            std::cout << bb->label << " ";
        }
        std::cout << std::endl;
    }
    
    return hasChanged;
}

} // namespace ir_opt 