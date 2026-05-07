#include "../../include/ir/llvm_ir.h"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace llvm_ir {

// 普通指令克隆
// valueMap: 值映射，用于处理指令中的值，如果值在valueMap中，则使用valueMap中的值，否则使用原始值
std::unique_ptr<Instruction> cloneInstruction(const Instruction* orig, std::map<Value*, Value*>& valueMap, const std::string& namePrefix) {
    auto get_mapped_value = [&](Value* v) -> Value* {
        return valueMap.count(v) ? valueMap.at(v) : v;
    };

    // Handle name properly to avoid duplicate % prefixes
    std::string new_name;
    if (orig->name.empty()) {
        new_name = namePrefix;
    } else {
        // If original name has % prefix, remove it before concatenating
        std::string baseName = orig->name;
        if (baseName[0] == '%') {
            baseName = baseName.substr(1);
        }
        new_name = "%" + baseName + namePrefix;
    }

    switch (orig->opcode) {
        case Opcode::Add: case Opcode::FAdd: case Opcode::Sub: case Opcode::FSub:
        case Opcode::Mul: case Opcode::FMul: case Opcode::SDiv: case Opcode::FDiv:
        case Opcode::SRem: case Opcode::And: case Opcode::Or: case Opcode::Xor:
        case Opcode::Shl: case Opcode::LShr: case Opcode::AShr:
        {
            auto binOp = static_cast<const BinaryOperator*>(orig);
            return std::make_unique<BinaryOperator>(binOp->opcode, 
                                                   get_mapped_value(binOp->operands[0]), 
                                                   get_mapped_value(binOp->operands[1]), 
                                                   new_name);
        }
        case Opcode::Alloca: {
            auto allocaInst = static_cast<const AllocaInst*>(orig);
            if (allocaInst->arraySize > 0) {
                return std::make_unique<AllocaInst>(allocaInst->allocatedType, 
                                                   allocaInst->arraySize, 
                                                   new_name);
            } else {
                return std::make_unique<AllocaInst>(allocaInst->allocatedType, 
                                                   new_name);
            }
        }
        case Opcode::Load: {
            auto loadInst = static_cast<const LoadInst*>(orig);
            return std::make_unique<LoadInst>(get_mapped_value(loadInst->getPointerOperand()), 
                                             new_name, 
                                             loadInst->type);
        }
        case Opcode::Store: {
            auto storeInst = static_cast<const StoreInst*>(orig);
            return std::make_unique<StoreInst>(get_mapped_value(storeInst->getValueOperand()), 
                                              get_mapped_value(storeInst->getPointerOperand()));
        }
        case Opcode::ICmp: {
            auto iCmpInst = static_cast<const ICmpInst*>(orig);
            return std::make_unique<ICmpInst>(iCmpInst->condition, 
                                             get_mapped_value(iCmpInst->operands[0]), 
                                             get_mapped_value(iCmpInst->operands[1]), 
                                             new_name);
        }
        case Opcode::FCmp: {
            auto fCmpInst = static_cast<const FCmpInst*>(orig);
            return std::make_unique<FCmpInst>(fCmpInst->condition, 
                                             get_mapped_value(fCmpInst->operands[0]), 
                                             get_mapped_value(fCmpInst->operands[1]), 
                                             new_name);
        }
        case Opcode::GetElementPtr: {
            auto gepInst = static_cast<const GetElementPtrInst*>(orig);
            return std::make_unique<GetElementPtrInst>(get_mapped_value(gepInst->operands[0]), 
                                                      get_mapped_value(gepInst->operands[1]), 
                                                      new_name, 
                                                      gepInst->elementType, 
                                                      gepInst->arraySize);
        }
        case Opcode::Call: {
            auto callInst = static_cast<const CallInst*>(orig);
            std::vector<Value*> args;
            for(const auto& arg : callInst->operands) {
                args.push_back(get_mapped_value(arg));
            }
            return std::make_unique<CallInst>(callInst->functionName, 
                                             args, 
                                             new_name, 
                                             callInst->type);
        }
        case Opcode::ZExt: {
            auto zextInst = static_cast<const ZExtInst*>(orig);
            return std::make_unique<ZExtInst>(get_mapped_value(zextInst->operands[0]), 
                                             zextInst->targetType, 
                                             new_name);
        }
        case Opcode::SExt: {
            auto sextInst = static_cast<const SExtInst*>(orig);
            return std::make_unique<SExtInst>(get_mapped_value(sextInst->operands[0]), 
                                             sextInst->targetType, 
                                             new_name);
        }
        case Opcode::Trunc: {
            auto truncInst = static_cast<const TruncInst*>(orig);
            return std::make_unique<TruncInst>(get_mapped_value(truncInst->operands[0]), 
                                              truncInst->targetType, 
                                              new_name);
        }
        case Opcode::SIToFP: {
            auto sitofpInst = static_cast<const SIToFPInst*>(orig);
            return std::make_unique<SIToFPInst>(get_mapped_value(sitofpInst->operands[0]), 
                                               sitofpInst->targetType, 
                                               new_name);
        }
        case Opcode::FPToSI: {
            auto fptosiInst = static_cast<const FPToSIInst*>(orig);
            return std::make_unique<FPToSIInst>(get_mapped_value(fptosiInst->operands[0]), 
                                               fptosiInst->targetType, 
                                               new_name);
        }
        case Opcode::FPToUI: {
            auto fptouiInst = static_cast<const FPToUIInst*>(orig);
            return std::make_unique<FPToUIInst>(get_mapped_value(fptouiInst->operands[0]), 
                                               fptouiInst->targetType, 
                                               new_name);
        }
        case Opcode::UIToFP: {
            auto uitofpInst = static_cast<const UIToFPInst*>(orig);
            return std::make_unique<UIToFPInst>(get_mapped_value(uitofpInst->operands[0]), 
                                               uitofpInst->targetType, 
                                               new_name);
        }
        case Opcode::FPTrunc: {
            auto fptruncInst = static_cast<const FPTruncInst*>(orig);
            return std::make_unique<FPTruncInst>(get_mapped_value(fptruncInst->operands[0]), 
                                                fptruncInst->targetType, 
                                                new_name);
        }
        case Opcode::FPExt: {
            auto fpextInst = static_cast<const FPExtInst*>(orig);
            return std::make_unique<FPExtInst>(get_mapped_value(fpextInst->operands[0]), 
                                              fpextInst->targetType, 
                                              new_name);
        }
        case Opcode::PtrToInt: {
            auto ptrtointInst = static_cast<const PtrToIntInst*>(orig);
            return std::make_unique<PtrToIntInst>(get_mapped_value(ptrtointInst->operands[0]), 
                                                 ptrtointInst->targetType, 
                                                 new_name);
        }
        case Opcode::IntToPtr: {
            auto inttoptrInst = static_cast<const IntToPtrInst*>(orig);
            return std::make_unique<IntToPtrInst>(get_mapped_value(inttoptrInst->operands[0]), 
                                                 inttoptrInst->targetType, 
                                                 new_name);
        }
        case Opcode::BitCast: {
            auto bitcastInst = static_cast<const BitCastInst*>(orig);
            return std::make_unique<BitCastInst>(get_mapped_value(bitcastInst->operands[0]), 
                                                bitcastInst->targetType, 
                                                new_name);
        }
        case Opcode::Select: {
            auto selectInst = static_cast<const SelectInst*>(orig);
            return std::make_unique<SelectInst>(get_mapped_value(selectInst->operands[0]), 
                                               get_mapped_value(selectInst->operands[1]), 
                                               get_mapped_value(selectInst->operands[2]), 
                                               new_name);
        }
        case Opcode::Br: {
            auto branchInst = static_cast<const BranchInst*>(orig);
            if (branchInst->isConditional()) {
                // Conditional branch
                return std::make_unique<BranchInst>(get_mapped_value(branchInst->operands[0]), 
                                                   branchInst->trueLabel, 
                                                   branchInst->falseLabel);
            } else {
                // Unconditional branch
                return std::make_unique<BranchInst>(branchInst->trueLabel);
            }
        }
        default:
            // Don't clone terminators, PHIs, or other special instructions
            std::cerr << "[ERROR] cloneInstruction: Unsupported instruction type: " << orig->getOpcodeName() << std::endl;
            return nullptr;
    }
}

// PHI指令克隆，不包含incoming_values
std::unique_ptr<PhiInst> clonePhi(const PhiInst* orig, const std::string& namePrefix) {
    std::string new_name;
    if (orig->name.empty()) {
        new_name = ""; // Respect empty names
    } else {
        std::string baseName = orig->name;
        if (baseName.length() > 0 && baseName[0] == '%') {
            baseName = baseName.substr(1);
        }
        new_name = "%" + baseName + namePrefix;
    }

    auto newPhi = std::make_unique<PhiInst>(orig->type, new_name);
    // Incoming values must be added by the caller.
    return newPhi;
}

// --- Value 实现 ---
std::vector<Instruction*>& Value::getUses() {
    return uses;
}

const std::vector<Instruction*>& Value::getUses() const {
    return uses;
}

void Value::addUse(Instruction* user) {
    // 检查是否已经存在，避免重复添加
    for (auto* existing : uses) {
        if (existing == user) {
            return; // 已经存在，不重复添加
        }
    }
    uses.push_back(user);
}

void Value::removeUse(Instruction* user) {
    uses.erase(std::remove(uses.begin(), uses.end(), user), uses.end());
}

// --- Instruction 实现 ---
void Instruction::replaceAllUsesWith(Value* newValue) {
    auto& usesVec = getUses();
    // 为新值添加所有使用者
    for (auto user : usesVec) {
        newValue->addUse(user);
    }
    // 替换所有操作数中对this的引用
    for (auto user : usesVec) {
        for (size_t i = 0; i < user->operands.size(); ++i) {
            if (user->operands[i] == this) {
                user->operands[i] = newValue;
            }
        }
        // 特殊处理PHI指令中的incoming values
        if (auto phi = dynamic_cast<PhiInst*>(user)) {
            for (auto& incoming : phi->incoming_values) {
                if (incoming.first == this) {
                    incoming.first = newValue;
                }
            }
        }
    }
    usesVec.clear(); // 此指令不再有使用者
}

void Instruction::removeFromParent() {
    if (!parent) {
        std::cerr << "[ERROR] Instruction::removeFromParent: parent is nullptr" << std::endl;
        return;
    };

    if (this->getUses().size() > 0) {
        std::cerr << "[Warning] Instruction::removeFromParent: instruction has uses" << std::endl;
        std::cerr << "[Warning] Instruction: " << this->toString() << std::endl;
    }

    // 移除对所有操作数的use关系
    for (auto op : operands) {
        op->removeUse(this);
    }
    
    // 如果是PHI指令，从phi_instructions中删除
    if (auto phi = dynamic_cast<PhiInst*>(this)) {
        auto& phiInsts = parent->phi_instructions;
        for (auto it = phiInsts.begin(); it != phiInsts.end(); ++it) {
            if (it->get() == this) {
                phiInsts.erase(it);
                break;
            }
        }
    } else {
        // 普通指令，从instructions中删除
        auto& insts = parent->instructions;
        for (auto it = insts.begin(); it != insts.end(); ++it) {
            if (it->get() == this) {
                insts.erase(it);
                break;
            }
        }
    }
}

// 在指令的operands中将oldValue替换为newValue，并更新use关系
void Instruction::replaceOperand(Value* oldValue, Value* newValue) {
    for (auto& op : operands) {
        if (op == oldValue) {
            oldValue->removeUse(this);
            op = newValue;
            newValue->addUse(this);
        }
    }
}

// 在PhiInst类中重写replaceOperand
void PhiInst::replaceOperand(Value* oldValue, Value* newValue) {
    for (auto& incoming : incoming_values) {
        if (incoming.first == oldValue) {
            oldValue->removeUse(this);
            incoming.first = newValue;
            newValue->addUse(this);
        }
    }
}

// --- BasicBlock 实现 ---
void BasicBlock::addInstruction(std::unique_ptr<Instruction> inst) {
    inst->parent = this;
    instructions.push_back(std::move(inst));
}

void BasicBlock::insertInstructionAt(std::unique_ptr<Instruction> inst, size_t position) {
    inst->parent = this;
    auto it = instructions.begin();
    std::advance(it, position);
    instructions.insert(it, std::move(inst));
}

void BasicBlock::addPhi(std::unique_ptr<PhiInst> phi) {
    phi->parent = this;
    phi_instructions.push_back(std::move(phi));
}

void BasicBlock::insertPhiAt(std::unique_ptr<PhiInst> phi, size_t position) {
    phi->parent = this;
    phi_instructions.insert(phi_instructions.begin() + position, std::move(phi));
}

// 替换指令，只能用于同类型指令！！！！
// 注意oldinst不会变成nullptr，它依旧指向原来的内存地址，只是被newinst接管了
void BasicBlock::replaceInstructionWith(Instruction* oldinst, std::unique_ptr<Instruction> newinst) {
    // 查找指向oldinst的迭代器
    auto it = std::find_if(instructions.begin(), instructions.end(),
        [oldinst](const std::unique_ptr<Instruction>& ptr) {
            return ptr.get() == oldinst;
        });
    if (it != instructions.end()) {
        // 先替换所有use
        it->get()->replaceAllUsesWith(newinst.get());
        // 释放原有指令的use关系
        for (auto* op : it->get()->operands) {
            if (op) op->removeUse(it->get());
        }
        // 设置parent
        newinst->parent = this;
        // 替换unique_ptr对象
        *it = std::move(newinst);
        // oldinst指针现在指向的内存已经被newinst接管
    }
}

Instruction* BasicBlock::getTerminator() const {
    if (instructions.empty()) return nullptr;
    Opcode lastOp = instructions.back()->opcode;
    if (lastOp == Opcode::Br || lastOp == Opcode::Ret || lastOp == Opcode::Switch) {
        return instructions.back().get();
    }
    return nullptr;
}

// --- Function 实现 ---
void Function::addBasicBlock(std::unique_ptr<BasicBlock> bb) {
    bb->parent = this;
    basicBlocks.push_back(std::move(bb));
}

void Function::insertBasicBlockAt(std::unique_ptr<BasicBlock> bb, size_t position) {
    bb->parent = this;
    if (position >= basicBlocks.size()) {
        // 如果位置超出范围，添加到末尾
        basicBlocks.push_back(std::move(bb));
    } else {
        // 在指定位置插入
        basicBlocks.insert(basicBlocks.begin() + position, std::move(bb));
    }
}

void Function::deleteBasicBlock(BasicBlock* bb) {
    // 从所有后继块的PHI节点中移除来自本块的边
    for (auto successor : bb->successors) {
        // 从后继块的PHI指令中移除来自本块的incoming值
        for (auto& phi : successor->phi_instructions) {
            auto it = phi->incoming_values.begin();
            while (it != phi->incoming_values.end()) {
                if (it->second == bb) {
                    // 移除这个incoming值
                    if (it->first) it->first->removeUse(phi.get());
                    it = phi->incoming_values.erase(it);
                } else {
                    ++it;
                }
            }
        }
        
        // 从后继块的前驱列表中移除本块
        auto predIt = std::find(successor->predecessors.begin(), 
                               successor->predecessors.end(), bb);
        if (predIt != successor->predecessors.end()) {
            successor->predecessors.erase(predIt);
        }
    }
    
    // 从函数中删除该块
    basicBlocks.erase(std::remove_if(basicBlocks.begin(), basicBlocks.end(),
        [bb](const std::unique_ptr<BasicBlock>& b) { return b.get() == bb; }),
        basicBlocks.end());
}

BasicBlock* Function::getEntryBlock() const {
    if (basicBlocks.empty()) return nullptr;
    return basicBlocks.front().get();
}

BasicBlock* Function::getBlockByName(const std::string& name) {
    for (auto& bb : basicBlocks) {
        if (bb->label == name) {
            return bb.get();
        }
    }
    return nullptr;
}

// Value类型名称转换
std::string Value::getTypeName() const {
    switch (type) {
        case Type::Void: return "void";
        case Type::I1: return "i1";
        case Type::I8: return "i8";
        case Type::I32: return "i32";
        case Type::I64: return "i64";
        case Type::Float: return "float";
        case Type::Double: return "double";
        case Type::Ptr: return "ptr";
        case Type::Array: return "array";
        case Type::Function: return "function";
        default: return "unknown";
    }
}

// 指令操作码名称转换
std::string Instruction::getOpcodeName() const {
    switch (opcode) {
        case Opcode::Add: return "add";
        case Opcode::FAdd: return "fadd";
        case Opcode::Sub: return "sub";
        case Opcode::FSub: return "fsub";
        case Opcode::Mul: return "mul";
        case Opcode::FMul: return "fmul";
        case Opcode::SDiv: return "sdiv";
        case Opcode::FDiv: return "fdiv";
        case Opcode::SRem: return "srem";
        case Opcode::And: return "and";
        case Opcode::Or: return "or";
        case Opcode::Xor: return "xor";
        case Opcode::Shl: return "shl";
        case Opcode::LShr: return "lshr";
        case Opcode::AShr: return "ashr";
        case Opcode::ICmp: return "icmp";
        case Opcode::FCmp: return "fcmp";
        case Opcode::ZExt: return "zext";
        case Opcode::Alloca: return "alloca";
        case Opcode::Load: return "load";
        case Opcode::Store: return "store";
        case Opcode::GetElementPtr: return "getelementptr";
        case Opcode::Call: return "call";
        case Opcode::Ret: return "ret";
        case Opcode::Br: return "br";
        default: return "unknown";
    }
}

// ICmp指令toString实现
std::string getICmpConditionName(ICmpCond condition) {
    switch (condition) {
        case ICmpCond::EQ: return "==";
        case ICmpCond::NE: return "!=";
        case ICmpCond::UGT: return ">";
        case ICmpCond::UGE: return ">=";
        case ICmpCond::ULT: return "<";
        case ICmpCond::ULE: return "<=";
        case ICmpCond::SGT: return ">";
        case ICmpCond::SGE: return ">=";
        case ICmpCond::SLT: return "<";
        case ICmpCond::SLE: return "<=";
        default: return "??";
    }
}
std::string ICmpInst::toString() const {
    std::string condStr;
    switch (condition) {
        case ICmpCond::EQ: condStr = "eq"; break;
        case ICmpCond::NE: condStr = "ne"; break;
        case ICmpCond::UGT: condStr = "ugt"; break;
        case ICmpCond::UGE: condStr = "uge"; break;
        case ICmpCond::ULT: condStr = "ult"; break;
        case ICmpCond::ULE: condStr = "ule"; break;
        case ICmpCond::SGT: condStr = "sgt"; break;
        case ICmpCond::SGE: condStr = "sge"; break;
        case ICmpCond::SLT: condStr = "slt"; break;
        case ICmpCond::SLE: condStr = "sle"; break;
    }
    return name + " = icmp " + condStr + " " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]) + ", " + getOperandValue(operands[1]);
}

// FCmp指令toString实现
std::string FCmpInst::toString() const {
    std::string condStr;
    switch (condition) {
        case FCmpCond::FALSE: condStr = "false"; break;
        case FCmpCond::OEQ: condStr = "oeq"; break;
        case FCmpCond::OGT: condStr = "ogt"; break;
        case FCmpCond::OGE: condStr = "oge"; break;
        case FCmpCond::OLT: condStr = "olt"; break;
        case FCmpCond::OLE: condStr = "ole"; break;
        case FCmpCond::ONE: condStr = "one"; break;
        case FCmpCond::ORD: condStr = "ord"; break;
        case FCmpCond::UEQ: condStr = "ueq"; break;
        case FCmpCond::UGT: condStr = "ugt"; break;
        case FCmpCond::UGE: condStr = "uge"; break;
        case FCmpCond::ULT: condStr = "ult"; break;
        case FCmpCond::ULE: condStr = "ule"; break;
        case FCmpCond::UNE: condStr = "une"; break;
        case FCmpCond::UNO: condStr = "uno"; break;
        case FCmpCond::TRUE: condStr = "true"; break;
    }
    return name + " = fcmp " + condStr + " " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]) + ", " + getOperandValue(operands[1]);
}

// Alloca指令toString实现
std::string AllocaInst::toString() const {
    std::string typeStr;
    switch (allocatedType) {
        case Type::I32: typeStr = "i32"; break;
        case Type::Float: typeStr = "float"; break;
        case Type::I8: typeStr = "i8"; break;
        case Type::I64: typeStr = "i64"; break;
        case Type::Double: typeStr = "double"; break;
        case Type::Ptr: typeStr = "ptr"; break;
        default: typeStr = "i32"; // 默认为i32而不是unknown
    }
    
    // 处理数组分配
    if (arraySize > 0) {
        return name + " = alloca [" + std::to_string(arraySize) + " x " + typeStr + "]";
    } else {
        return name + " = alloca " + typeStr;
    }
}

// Call指令toString实现
std::string CallInst::toString() const {
    std::ostringstream oss;
    if (type != Type::Void) {
        oss << name << " = ";
    }
    oss << "call " << getTypeName() << " @" << functionName << "(";
    
    for (size_t i = 0; i < operands.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << operands[i]->getTypeName() << " " << getOperandValue(operands[i]);
    }
    oss << ")";
    return oss.str();
}

// Branch指令toString实现
std::string BranchInst::toString() const {
    if (operands.empty()) {
        // 无条件分支
        return "br label %" + trueLabel;
    } else {
        // 条件分支
        return "br " + operands[0]->getTypeName() + " " + getOperandValue(operands[0]) + ", label %" + trueLabel + ", label %" + falseLabel;
    }
}

// PhiInst指令toString实现
std::string PhiInst::toString() const {
    std::string str = name + " = phi " + getTypeName();
    for (size_t i = 0; i < incoming_values.size(); ++i) {
        if (!incoming_values[i].first) {
            std::cerr << "[ERROR] PhiInst " << name << " incoming_values[" << i << "].first is nullptr!" << std::endl;
            continue;
        }
        if (!incoming_values[i].second) {
            std::cerr << "[ERROR] PhiInst " << name << " incoming_values[" << i << "].second (block) is nullptr!" << std::endl;
            continue;
        }
        str += (i == 0 ? " " : ", ");
        str += "[ " + getOperandValue(incoming_values[i].first) + ", %" + incoming_values[i].second->label + " ]";
    }
    return str;
}

// BasicBlock toString实现
std::string BasicBlock::toString() const {
    std::ostringstream oss;
    oss << label << ":" << (comments.empty() ? "" : "     ;");
    for (size_t i = 0; i < comments.size(); ++i) {
        oss << comments[i];
        if (i != comments.size() - 1) {
            oss << "; ";
        }
    }
    oss << "\n";
    // 输出 phi 指令
    for (const auto& phi : phi_instructions) {
        if (phi) {
            oss << "  " << phi->toString() << "\n";
        }
    }
    // 输出普通指令
    for (const auto& inst : instructions) {
        if (inst) {
            oss << "  " << inst->toString() << "\n";
        }
    }
    return oss.str();
}

// Function toString实现
std::string Function::toString() const {
    std::ostringstream oss;
    
    // 如果是内置函数（没有基本块），则生成声明
    if (basicBlocks.empty()) {
        oss << "declare " << Value("", returnType).getTypeName() << " @" << name << "(";
        
        for (size_t i = 0; i < parameters.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << parameters[i]->getTypeName();
            if (!parameters[i]->name.empty()) {
                oss << " " << parameters[i]->name;
            }
        }
        oss << ")\n\n";
        return oss.str();
    }
    
    // 否则生成函数定义
    oss << "define " << Value("", returnType).getTypeName() << " @" << name << "(";
    
    for (size_t i = 0; i < parameters.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << parameters[i]->getTypeName() << " " << parameters[i]->name;
    }
    oss << ") {\n";
    
    for (const auto& bb : basicBlocks) {
        // 跳过空的基本块（没有指令的基本块）
        if (!bb->instructions.empty()) {
            oss << bb->toString();
        }
    }
    oss << "}\n\n";
    return oss.str();
}

// GlobalVariable toString实现
std::string GlobalVariable::toString() const {
    std::ostringstream oss;
    oss << "@" << name << " = ";
    // if (isConstant) oss << "constant "; // 我们统一用了zeroinitializer，然后对于有值的在global会进行store
    // else oss << "global ";
    oss << "global ";
    
    if (arraySize > 0) {
        // 数组类型 - 使用正确的元素类型
        std::string elemTypeStr = "i32"; // 默认
        switch (elementType) {
            case Type::I32: elemTypeStr = "i32"; break;
            case Type::Float: elemTypeStr = "float"; break;
            case Type::I8: elemTypeStr = "i8"; break;
            case Type::I64: elemTypeStr = "i64"; break;
            default: elemTypeStr = "i32"; break;
        }
        oss << "[" << arraySize << " x " << elemTypeStr << "]";
    } else {
        // 普通类型
        oss << getTypeName();
    }
    
    // 统一使用 zeroinitializer，不管是否有initializer
    oss << " zeroinitializer";
    return oss.str();
}

// Module toString实现
std::string Module::toString() const {
    std::ostringstream oss;
    oss << "; ModuleID = '" << name << "'\n\n";
    
    // 输出全局变量
    for (const auto& gvar : globalVariables) {
        oss << gvar->toString() << "\n";
    }
    if (!globalVariables.empty()) oss << "\n";
    
    // 输出函数
    for (const auto& func : functions) {
        oss << func->toString();
    }
    
    return oss.str();
}

// IRBuilder实现
void IRBuilder::insertInstruction(std::unique_ptr<Instruction> inst) {
    if (currentBlock) {
        currentBlock->addInstruction(std::move(inst));
    }
}

BinaryOperator* IRBuilder::createAdd(Value* lhs, Value* rhs, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<BinaryOperator>(Opcode::Add, lhs, rhs, instName);
    BinaryOperator* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

BinaryOperator* IRBuilder::createFAdd(Value* lhs, Value* rhs, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<BinaryOperator>(Opcode::FAdd, lhs, rhs, instName);
    BinaryOperator* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

BinaryOperator* IRBuilder::createSub(Value* lhs, Value* rhs, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<BinaryOperator>(Opcode::Sub, lhs, rhs, instName);
    BinaryOperator* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

BinaryOperator* IRBuilder::createFSub(Value* lhs, Value* rhs, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<BinaryOperator>(Opcode::FSub, lhs, rhs, instName);
    BinaryOperator* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

BinaryOperator* IRBuilder::createMul(Value* lhs, Value* rhs, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<BinaryOperator>(Opcode::Mul, lhs, rhs, instName);
    BinaryOperator* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

BinaryOperator* IRBuilder::createFMul(Value* lhs, Value* rhs, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<BinaryOperator>(Opcode::FMul, lhs, rhs, instName);
    BinaryOperator* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

BinaryOperator* IRBuilder::createSDiv(Value* lhs, Value* rhs, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<BinaryOperator>(Opcode::SDiv, lhs, rhs, instName);
    BinaryOperator* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

BinaryOperator* IRBuilder::createFDiv(Value* lhs, Value* rhs, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<BinaryOperator>(Opcode::FDiv, lhs, rhs, instName);
    BinaryOperator* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

BinaryOperator* IRBuilder::createSRem(Value* lhs, Value* rhs, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<BinaryOperator>(Opcode::SRem, lhs, rhs, instName);
    BinaryOperator* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

ICmpInst* IRBuilder::createICmp(ICmpCond cond, Value* lhs, Value* rhs, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<ICmpInst>(cond, lhs, rhs, instName);
    ICmpInst* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

FCmpInst* IRBuilder::createFCmp(FCmpCond cond, Value* lhs, Value* rhs, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<FCmpInst>(cond, lhs, rhs, instName);
    FCmpInst* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

ZExtInst* IRBuilder::createZExt(Value* val, Type toType, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<ZExtInst>(val, toType, instName);
    ZExtInst* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

SIToFPInst* IRBuilder::createSIToFP(Value* val, Type toType, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<SIToFPInst>(val, toType, instName);
    SIToFPInst* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

FPToSIInst* IRBuilder::createFPToSI(Value* val, Type toType, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<FPToSIInst>(val, toType, instName);
    FPToSIInst* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

AllocaInst* IRBuilder::createAlloca(Type type, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<AllocaInst>(type, instName);
    AllocaInst* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

AllocaInst* IRBuilder::createAlloca(Type type, int arraySize, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<AllocaInst>(type, arraySize, instName);
    AllocaInst* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

LoadInst* IRBuilder::createLoad(Value* ptr, const std::string& name, Type loadType) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<LoadInst>(ptr, instName, loadType);
    LoadInst* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

StoreInst* IRBuilder::createStore(Value* val, Value* ptr) {
    auto inst = std::make_unique<StoreInst>(val, ptr);
    StoreInst* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

GetElementPtrInst* IRBuilder::createGEP(Value* ptr, Value* index, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<GetElementPtrInst>(ptr, index, instName);
    GetElementPtrInst* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

GetElementPtrInst* IRBuilder::createGEP(Value* ptr, Value* index, const std::string& name, Type elemType, int arraySize) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<GetElementPtrInst>(ptr, index, instName, elemType, arraySize);
    GetElementPtrInst* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

CallInst* IRBuilder::createCall(const std::string& funcName, const std::vector<Value*>& args, 
                               const std::string& name, Type retType) {
    std::string instName = name.empty() && retType != Type::Void ? getNextTempName() : name;
    auto inst = std::make_unique<CallInst>(funcName, args, instName, retType);
    CallInst* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

ReturnInst* IRBuilder::createRet(Value* val) {
    auto inst = std::make_unique<ReturnInst>(val);
    ReturnInst* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

BranchInst* IRBuilder::createBr(const std::string& label) {
    auto inst = std::make_unique<BranchInst>(label);
    BranchInst* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

BranchInst* IRBuilder::createCondBr(Value* cond, const std::string& trueLabel, const std::string& falseLabel) {
    auto inst = std::make_unique<BranchInst>(cond, trueLabel, falseLabel);
    BranchInst* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

BinaryOperator* IRBuilder::createAnd(Value* lhs, Value* rhs, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<BinaryOperator>(Opcode::And, lhs, rhs, instName);
    BinaryOperator* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

BinaryOperator* IRBuilder::createOr(Value* lhs, Value* rhs, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<BinaryOperator>(Opcode::Or, lhs, rhs, instName);
    BinaryOperator* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

BinaryOperator* IRBuilder::createXor(Value* lhs, Value* rhs, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<BinaryOperator>(Opcode::Xor, lhs, rhs, instName);
    BinaryOperator* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

BinaryOperator* IRBuilder::createShl(Value* lhs, Value* rhs, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<BinaryOperator>(Opcode::Shl, lhs, rhs, instName);
    BinaryOperator* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

BinaryOperator* IRBuilder::createLShr(Value* lhs, Value* rhs, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<BinaryOperator>(Opcode::LShr, lhs, rhs, instName);
    BinaryOperator* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

BinaryOperator* IRBuilder::createAShr(Value* lhs, Value* rhs, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<BinaryOperator>(Opcode::AShr, lhs, rhs, instName);
    BinaryOperator* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

PhiInst* IRBuilder::createPHI(Type type, const std::string& name) {
    std::string instName = name.empty() ? getNextTempName() : name;
    auto inst = std::make_unique<PhiInst>(type, instName);
    PhiInst* result = inst.get();
    insertInstruction(std::move(inst));
    return result;
}

} // namespace llvm_ir
