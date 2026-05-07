#include "IR.h"
#include <cstdint>
std::map<Instruction::OpID, std::string> instr_id2string_ = {
    {Instruction::Ret, "ret"}, {Instruction::Br, "br"}, {Instruction::FNeg, "fneg"}, {Instruction::Add, "add"}, {Instruction::Sub, "sub"}, {Instruction::Mul, "mul"}, {Instruction::SDiv, "sdiv"}, {Instruction::SRem, "srem"}, {Instruction::UDiv, "udiv"}, {Instruction::URem, "urem"}, {Instruction::FAdd, "fadd"}, {Instruction::FSub, "fsub"}, {Instruction::FMul, "fmul"}, {Instruction::FDiv, "fdiv"}, {Instruction::Shl, "shl"}, {Instruction::LShr, "lshr"}, {Instruction::AShr, "ashr"}, {Instruction::And, "and"}, {Instruction::Or, "or"}, {Instruction::Xor, "xor"}, {Instruction::Alloca, "alloca"}, {Instruction::Load, "load"}, {Instruction::Store, "store"}, {Instruction::GetElementPtr, "getelementptr"}, {Instruction::ZExt, "zext"}, {Instruction::FPtoSI, "fptosi"}, {Instruction::SItoFP, "sitofp"}, {Instruction::BitCast, "bitcast"}, {Instruction::ICmp, "icmp"}, {Instruction::FCmp, "fcmp"}, {Instruction::PHI, "phi"}, {Instruction::Call, "call"}}; // Instruction from opid to string
const std::map<ICmpInst::ICmpOp, std::string> ICmpInst::ICmpOpName = {{ICmpInst::ICmpOp::ICMP_EQ, "eq"}, {ICmpInst::ICmpOp::ICMP_NE, "ne"}, {ICmpInst::ICmpOp::ICMP_UGT, "hi"}, {ICmpInst::ICmpOp::ICMP_UGE, "cs"}, {ICmpInst::ICmpOp::ICMP_ULT, "cc"}, {ICmpInst::ICmpOp::ICMP_ULE, "ls"}, {ICmpInst::ICmpOp::ICMP_SGT, "gt"}, {ICmpInst::ICmpOp::ICMP_SGE, "ge"}, {ICmpInst::ICmpOp::ICMP_SLT, "lt"}, {ICmpInst::ICmpOp::ICMP_SLE, "le"}};
const std::map<FCmpInst::FCmpOp, std::string> FCmpInst::FCmpOpName = {{FCmpInst::FCmpOp::FCMP_FALSE, "nv"}, {FCmpInst::FCmpOp::FCMP_OEQ, "eq"}, {FCmpInst::FCmpOp::FCMP_OGT, "gt"}, {FCmpInst::FCmpOp::FCMP_OGE, "ge"}, {FCmpInst::FCmpOp::FCMP_OLT, "cc"}, {FCmpInst::FCmpOp::FCMP_OLE, "ls"}, {FCmpInst::FCmpOp::FCMP_ONE, "ne"}, {FCmpInst::FCmpOp::FCMP_ORD, "vc"}, {FCmpInst::FCmpOp::FCMP_UNO, "vs"}, {FCmpInst::FCmpOp::FCMP_UEQ, "eq"}, {FCmpInst::FCmpOp::FCMP_UGT, "hi"}, {FCmpInst::FCmpOp::FCMP_UGE, "cs"}, {FCmpInst::FCmpOp::FCMP_ULT, "lt"}, {FCmpInst::FCmpOp::FCMP_ULE, "le"}, {FCmpInst::FCmpOp::FCMP_UNE, "ne"}, {FCmpInst::FCmpOp::FCMP_TRUE, "al"}};
std::string print_as_op(Value *v, bool print_ty);
std::string print_cmp_IRType(ICmpInst::ICmpOp op);
std::string print_fcmp_IRType(FCmpInst::FCmpOp op);

std::string IRType::print()
{
    std::string IRType_ir;
    
    if (tid_ == VoidTyID) {
        IRType_ir += "void";
    } else if (tid_ == LabelTyID) {
        IRType_ir += "label";
    } else if (tid_ == IntegerTyID) {
        IRType_ir += "i";
        IRType_ir += std::to_string(static_cast<IntegerIRType*>(this)->num_bits_);
    } else if (tid_ == FloatTyID) {
        IRType_ir += "float";
    } else if (tid_ == FunctionTyID) {
        IRType_ir += static_cast<FunctionIRType*>(this)->result_->print();
        IRType_ir += " (";
        for (size_t i = 0; i < static_cast<FunctionIRType*>(this)->args_.size(); i++) {
            if (i)
                IRType_ir += ", ";
            IRType_ir += static_cast<FunctionIRType*>(this)->args_[i]->print();
        }
        IRType_ir += ")";
    } else if (tid_ == PointerTyID) {
        IRType_ir += static_cast<PointerIRType*>(this)->contained_->print();
        IRType_ir += "*";
    } else if (tid_ == ArrayTyID) {
        IRType_ir += "[";
        IRType_ir += std::to_string(static_cast<ArrayIRType*>(this)->num_elements_);
        IRType_ir += " x ";
        IRType_ir += static_cast<ArrayIRType*>(this)->contained_->print();
        IRType_ir += "]";
    }
    return IRType_ir;
}

bool Value::is_constant() { return name_[0] == 0; }

std::string ConstantInt::print()
{
    std::string code;
    if (this->IRType_->tid_ == IRType::IntegerTyID && static_cast<IntegerIRType *>(this->IRType_)->num_bits_ == 1)
        code += (this->value_ == 0) ? "0" : "1";
    else
        code += std::to_string(this->value_);
    return code;
}

std::string ConstantFloat::print()
{
    std::stringstream fp_ir_ss;
    std::string fp_ir;
    double val = this->value_;
    fp_ir_ss << "0x" << std::hex << *(uint64_t *)&val << std::endl;
    fp_ir_ss >> fp_ir;
    return fp_ir;
}

std::string ConstantFloat::print32()
{
    std::stringstream fp_ir_ss;
    std::string fp_ir;
    float val = this->value_;
    fp_ir_ss << "0x" << std::hex << *(uint32_t *)&val << std::endl;
    fp_ir_ss >> fp_ir;
    return fp_ir;
}

std::string ConstantArray::print() {
    std::stringstream ss;
    // 获取数组类型及元素类型，避免重复转换
    auto* arrayType = static_cast<ArrayIRType*>(IRType_);
    std::string elemTypeStr = arrayType->contained_->print();
    ss << "[";  // 数组起始标记
    // 处理第一个元素
    if (!const_array.empty()) {
        ss << elemTypeStr << " " << const_array[0]->print();

        // 处理剩余元素，统一添加逗号前缀
        for (size_t i = 1; i < const_array.size(); ++i) {
            ss << ", " << elemTypeStr << " " << const_array[i]->print();
        }
    }
    ss << "]";  // 数组结束标记
    return ss.str();
}

std::string ConstantZero::print()
{
    return "zeroinitializer";
}


std::string Module::print() {
    std::stringstream ss;

    // 打印全局变量列表
    for (auto& gloVal : global_list_) {  // 使用引用避免拷贝
        ss << gloVal->print() << "\n";
    }
    // 打印函数列表
    for (auto& Fun : function_list_) {   // 使用引用避免拷贝
        ss << Fun->print() << "\n";
    }

    return ss.str();
}
Function *Module::getMainFunc()
{
    for (auto fun : function_list_)
    {
        if (fun->name_ == "main")
        {
            return fun;
        }
    }
    return nullptr;
}

std::string GlobalVariable::print() {
    std::stringstream ss;
    
    auto* ptrType = static_cast<PointerIRType*>(IRType_);
    std::string containedType = ptrType->contained_->print();
    
    // 拼接全局变量声明
    ss << print_as_op(this, false) << " = "
       << (is_const_ ? "constant " : "global ")  // 根据常量性选择关键字
       << containedType << " ";                 // 拼接元素类型
    
    if (init_val_ != nullptr) {
        ss << init_val_->print();
    }
    
    return ss.str();
}

std::string Function::print() {
    if (name_ == "llvm.memset.p0.i32") {
        return "declare void @llvm.memset.p0.i32(i32*, i8, i32, i1)";
    }

    set_instr_name();
    std::stringstream ss;

    ss << (is_declaration() ? "declare " : "define ")
       << get_return_IRType()->print() << " "
       << print_as_op(this, false) << "(";

    if (is_declaration()) {
        auto* funcType = static_cast<FunctionIRType*>(IRType_);
        for (size_t i = 0; i < arguments_.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << funcType->args_[i]->print();
        }
    } else {
        for (auto it = arguments_.begin(); it != arguments_.end(); ++it) {
            if (it != arguments_.begin()) ss << ", ";
            ss << static_cast<Argument*>(*it)->print();
        }
    }
    ss << ")";

    if (!is_declaration()) {
        ss << " {\n";
        for (auto& bb : basic_blocks_) {  
            ss << bb->print();
        }
        ss << "}";
    }
    return ss.str();
}


std::string Argument::print() {
    std::stringstream ss;
    ss << IRType_->print() << " %" << name_;
    return ss.str();
}

void Function::remove_bb(BasicBlock *bb)
{
    basic_blocks_.erase(std::remove(basic_blocks_.begin(), basic_blocks_.end(), bb), basic_blocks_.end());
    for (auto pre : bb->pre_bbs_)
    {
        pre->remove_succ_basic_block(bb);
    }
    for (auto succ : bb->succ_bbs_)
    {
        succ->remove_pre_basic_block(bb);
    }
}

BasicBlock *Function::getRetBB()
{
    for (auto bb : basic_blocks_)
    {
        if (bb->get_terminator()->is_ret())
        {
            return bb;
        }
    }
    return nullptr;
}

std::string BasicBlock::print()
{
    std::string bb_ir;
    bb_ir += this->name_;
    bb_ir += ":";
    if (!this->pre_bbs_.empty())
    {
        bb_ir += "                                                ; preds = ";
    }
    for (auto bb : this->pre_bbs_)
    {
        if (bb != *this->pre_bbs_.begin())
            bb_ir += ", ";
        bb_ir += print_as_op(bb, false);
    }

    if (!this->parent_)
    {
        bb_ir += "\n";
        bb_ir += "; Error: Block without parent!";
    }
    bb_ir += "\n";
    for (auto instr : this->instr_list_)
    {
        bb_ir += "  ";
        bb_ir += instr->print();
        bb_ir += "\n";
    }

    return bb_ir;
}

Instruction *BasicBlock::get_terminator()
{
    if (instr_list_.empty())
        return nullptr;
    switch (instr_list_.back()->op_id_)
    {
    case Instruction::Ret:
    case Instruction::Br:
        return instr_list_.back();
    default:
        return nullptr;
    }
}

bool BasicBlock::delete_instr(Instruction *instr)
{
    if ((!instr) || instr->pos_in_bb.size() != 1 || instr->parent_ != this)
        return false;
    this->instr_list_.erase(instr->pos_in_bb.back());
    instr->remove_use_of_ops();
    instr->pos_in_bb.clear(); // 保证指令自由身
    instr->parent_ = nullptr;
    return true;
}

bool BasicBlock::add_instruction(Instruction *instr)
{
    if (instr->pos_in_bb.size() != 0)
    { // 指令已经插入到某个地方了
        return false;
    }
    else
    {
        instr_list_.push_back(instr);
        std::list<Instruction *>::iterator tail = instr_list_.end();
        instr->pos_in_bb.emplace_back(--tail);
        instr->parent_ = this;
        return true;
    }
}

bool BasicBlock::add_instruction_front(Instruction *instr)
{
    if (instr->pos_in_bb.size() != 0)
    { // 指令已经插入到某个地方了
        return false;
    }
    else
    {
        instr_list_.push_front(instr);
        std::list<Instruction *>::iterator head = instr_list_.begin();
        instr->pos_in_bb.emplace_back(head);
        instr->parent_ = this;
        return true;
    }
}

bool BasicBlock::add_instruction_before_terminator(Instruction *instr)
{
    if (instr->pos_in_bb.size() != 0)
    { 
        return false;
    }
    else if (instr_list_.empty())
    { 
        return false;
    }
    else
    {
        auto it = std::end(instr_list_);     // 最后一位的后一位位置
        instr_list_.emplace(--it, instr);    // 插入使得代替最后一位（--it）的位置，此时it是最后一位，插入的是倒数第二位
        instr->pos_in_bb.emplace_back(--it); // 记录插入的结果
        instr->parent_ = this;
        return true;
    }
}

bool BasicBlock::add_instruction_before_inst(Instruction *new_instr, Instruction *instr)
{
    if ((!instr) || instr->pos_in_bb.size() != 1 || instr->parent_ != this)
        return false;
    if (new_instr->pos_in_bb.size() != 0) // 指令已经插入到某个地方了
        return false;
    else if (instr_list_.empty()) // bb原本没有指令，那instr不在bb内，那为啥instr->parent_== this
        return false;
    else
    {
        auto it = instr->pos_in_bb.back();       //
        instr_list_.emplace(it, new_instr);      // 插入使得代替最后一位（--it）的位置，此时it是最后一位，插入的是倒数第二位
        new_instr->pos_in_bb.emplace_back(--it); // 记录插入的结果
        new_instr->parent_ = this;
        return true;
    }
}

// 从bb移出一个指令
bool BasicBlock::remove_instr(Instruction *instr)
{
    if ((!instr) || instr->pos_in_bb.size() != 1 || instr->parent_ != this)
        return false;
    this->instr_list_.erase(instr->pos_in_bb.back());
    instr->pos_in_bb.clear(); // 保证指令自由身
    instr->parent_ = nullptr;
    return true;
}

std::string BinaryInst::print() {
    std::stringstream ss;
    
    auto* op0Type = operands_[0]->IRType_;
    
    ss << "%" << name_ << " = "
       << instr_id2string_[op_id_] << " "
       << op0Type->print() << " "
       << print_as_op(get_operand(0), false) << ", "
       << print_as_op(get_operand(1), false);
    
    assert(op0Type->tid_ == get_operand(1)->IRType_->tid_);
    
    return ss.str();
}
std::string UnaryInst::print() {
    std::stringstream ss;
    // 获取操作数类型（
    auto* opType = operands_[0]->IRType_;

    // 拼接指令基础部分
    ss << "%" << name_ << " = "
       << instr_id2string_[op_id_] << " "
       << opType->print() << " "
       << print_as_op(get_operand(0), false);

    if (op_id_ == Instruction::ZExt) {
        assert(IRType_->tid_ == IRType::IntegerTyID);
        ss << " to i32";
    } else if (op_id_ == Instruction::FPtoSI) {
        assert(IRType_->tid_ == IRType::IntegerTyID);
        ss << " to i32";
    } else if (op_id_ == Instruction::SItoFP) {
        assert(IRType_->tid_ == IRType::FloatTyID);
        ss << " to float";
    } else {
        assert(0 && "UnaryInst opID invalid!");
    }

    return ss.str();
}

std::string ICmpInst::print() {
    std::stringstream ss;
    auto* op0Type = get_operand(0)->IRType_;
    // 判断两个操作数类型是否一致，决定操作数1的打印方式
    bool typesMatch = (op0Type->tid_ == get_operand(1)->IRType_->tid_);

    ss << "%" << name_ << " = "
       << instr_id2string_[op_id_] << " "
       << print_cmp_IRType(icmp_op_) << " "
       << op0Type->print() << " "
       << print_as_op(get_operand(0), false) << ", "
       << print_as_op(get_operand(1), !typesMatch);  

    return ss.str();
}

std::string FCmpInst::print() {
    std::stringstream ss;
    // 获取第一个操作数的类型
    auto* op0Type = get_operand(0)->IRType_;
    // 判断两个操作数类型是否一致，决定操作数1的打印参数
    const bool typesMatch = (op0Type->tid_ == get_operand(1)->IRType_->tid_);

    // 拼接指令
    ss << "%" << name_ << " = "
       << instr_id2string_[op_id_] << " "
       << print_fcmp_IRType(fcmp_op_) << " "
       << op0Type->print() << " "
       << print_as_op(get_operand(0), false) << ", "
       << print_as_op(get_operand(1), !typesMatch);  

    return ss.str();
}

std::string CallInst::print()
{
    std::string instr_ir;
    if (!(this->IRType_->tid_ == IRType::VoidTyID))
    {
        instr_ir += "%";
        instr_ir += this->name_;
        instr_ir += " = ";
    }
    instr_ir += instr_id2string_[this->op_id_];
    instr_ir += " ";
    unsigned int numops = this->num_ops_;
    instr_ir += static_cast<FunctionIRType *>(this->get_operand(numops - 1)->IRType_)->result_->print();

    instr_ir += " ";
    assert(dynamic_cast<Function *>(this->get_operand(numops - 1)) && "Wrong call operand function");
    if (dynamic_cast<Function *>(this->get_operand(numops - 1))->name_ == "__aeabi_memclr4")
    {
        instr_ir += "@llvm.memset.p0.i32(";
        instr_ir += this->get_operand(0)->IRType_->print();
        instr_ir += " ";
        instr_ir += print_as_op(this->get_operand(0), false);
        instr_ir += ", i8 0, ";
        instr_ir += this->get_operand(1)->IRType_->print();
        instr_ir += " ";
        instr_ir += print_as_op(this->get_operand(1), false);
        instr_ir += ", i1 false)";
        return instr_ir;
    }

    instr_ir += print_as_op(this->get_operand(numops - 1), false);
    instr_ir += "(";
    for (unsigned int i = 0; i < numops - 1; i++)
    {
        if (i > 0)
            instr_ir += ", ";
        instr_ir += this->get_operand(i)->IRType_->print();
        instr_ir += " ";
        instr_ir += print_as_op(this->get_operand(i), false);
    }
    instr_ir += ")";
    return instr_ir;
}

std::string BranchInst::print() {
    std::stringstream ss;
    
    // 拼接基础分支指令和第一个操作数
    ss << instr_id2string_[op_id_] << " "
       << print_as_op(get_operand(0), true);
    
    // 处理三操作数情况（条件分支）
    if (num_ops_ == 3) {
        ss << ", "
           << print_as_op(get_operand(1), true) << ", "
           << print_as_op(get_operand(2), true);
    }
    
    return ss.str();
}

std::string ReturnInst::print() {
    std::stringstream ss;
    
    // 拼接返回指令基础部分
    ss << instr_id2string_[op_id_] << " ";
    
    // 根据操作数数量决定后续内容
    if (num_ops_ != 0) {
        // 有返回值：打印类型和操作数
        auto* opType = get_operand(0)->IRType_;
        ss << opType->print() << " "
           << print_as_op(get_operand(0), false);
    } else {
        // 无返回值：打印void
        ss << "void";
    }
    
    return ss.str();
}

std::string GetElementPtrInst::print()
{
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->name_;
    instr_ir += " = ";
    instr_ir += instr_id2string_[this->op_id_];
    instr_ir += " ";
    assert(this->get_operand(0)->IRType_->tid_ == IRType::PointerTyID);
    instr_ir += static_cast<PointerIRType *>(this->get_operand(0)->IRType_)->contained_->print();
    instr_ir += ", ";
    for (unsigned int i = 0; i < this->num_ops_; i++)
    {
        if (i > 0)
            instr_ir += ", ";
        instr_ir += this->get_operand(i)->IRType_->print();
        instr_ir += " ";
        instr_ir += print_as_op(this->get_operand(i), false);
    }
    return instr_ir;
}

std::string StoreInst::print() {
    std::stringstream ss;
    // 获取第一个操作数的类型（用于打印）
    auto* op0Type = get_operand(0)->IRType_;

    // 拼接存储指令完整格式：指令名 类型 操作数0, 操作数1
    ss << instr_id2string_[op_id_] << " "
       << op0Type->print() << " "
       << print_as_op(get_operand(0), false) << ", "
       << print_as_op(get_operand(1), true);

    return ss.str();
}

std::string LoadInst::print() {
    std::stringstream ss;
    // 获取操作数0（指针类型）及其包含的元素类型
    auto* ptrOperand = get_operand(0);
    auto* ptrType = static_cast<PointerIRType*>(ptrOperand->IRType_);
    std::string containedType = ptrType->contained_->print();

    // 验证操作数0确实是指针类型（保留原断言）
    assert(ptrOperand->IRType_->tid_ == IRType::PointerTyID);

    // 拼接加载指令完整格式：%name = load 元素类型, 指针操作数
    ss << "%" << name_ << " = "
       << instr_id2string_[op_id_] << " "
       << containedType << ", "
       << print_as_op(ptrOperand, true);

    return ss.str();
}

std::string AllocaInst::print() {
    std::stringstream ss;
    // 拼接分配指令格式：%name = 指令名 类型
    ss << "%" << name_ << " = "
       << instr_id2string_[op_id_] << " "
       << alloca_ty_->print();
    return ss.str();
}

std::string ZextInst::print() {
    std::stringstream ss;
    // 获取源操作数的类型（用于打印）
    auto* srcType = get_operand(0)->IRType_;

    // 拼接零扩展指令格式：%name = 指令名 源类型 操作数 to 目标类型
    ss << "%" << name_ << " = "
       << instr_id2string_[op_id_] << " "
       << srcType->print() << " "
       << print_as_op(get_operand(0), false) << " to "
       << ZextInstTy->print();

    return ss.str();
}

std::string FpToSiInst::print() {
    std::stringstream ss;
    // 获取源操作数（浮点类型）的类型信息
    auto* srcType = get_operand(0)->IRType_;

    // 拼接浮点转有符号整数指令格式：%name = 指令名 源类型 操作数 to 目标类型
    ss << "%" << name_ << " = "
       << instr_id2string_[op_id_] << " "
       << srcType->print() << " "
       << print_as_op(get_operand(0), false) << " to "
       << FpToSiInstTy->print();

    return ss.str();
}
std::string SiToFpInst::print() {
    std::stringstream ss;
    // 获取源操作数（整数类型）的类型信息
    auto* srcType = get_operand(0)->IRType_;

    // 拼接有符号整数转浮点指令格式：%name = 指令名 源类型 操作数 to 目标类型
    ss << "%" << name_ << " = "
       << instr_id2string_[op_id_] << " "
       << srcType->print() << " "
       << print_as_op(get_operand(0), false) << " to "
       << SiToFpInstTy->print();

    return ss.str();
}

std::string Bitcast::print() {
    std::stringstream ss;
    // 获取源操作数的类型信息
    auto* srcType = get_operand(0)->IRType_;

    // 拼接位转换指令格式：%name = 指令名 源类型 操作数 to 目标类型
    ss << "%" << name_ << " = "
       << instr_id2string_[op_id_] << " "
       << srcType->print() << " "
       << print_as_op(get_operand(0), false) << " to "
       << BitcastTy->print();

    return ss.str();
}

std::string PhiInst::print()
{
    std::string instr_ir;
    instr_ir += "%";
    instr_ir += this->name_;
    instr_ir += " = ";
    instr_ir += instr_id2string_[this->op_id_];
    instr_ir += " ";
    instr_ir += this->get_operand(0)->IRType_->print();
    instr_ir += " ";
    for (int i = 0; i < this->num_ops_ / 2; i++)
    {
        if (i > 0)
            instr_ir += ", ";
        instr_ir += "[ ";
        instr_ir += print_as_op(this->get_operand(2 * i), false);
        instr_ir += ", ";
        instr_ir += print_as_op(this->get_operand(2 * i + 1), false);
        instr_ir += " ]";
    }
    if (this->num_ops_ / 2 < this->parent_->pre_bbs_.size())
    {
        for (auto pre_bb : this->parent_->pre_bbs_)
        {
            if (std::find(this->operands_.begin(), this->operands_.end(), static_cast<Value *>(pre_bb)) == this->operands_.end())
            {
                instr_ir += ", [ undef, " + print_as_op(pre_bb, false) + " ]";
            }
        }
    }
    return instr_ir;
}

std::string print_as_op(Value* v, bool print_ty) {
    std::stringstream ss;

    // 若需要打印类型，先拼接类型字符串
    if (print_ty) {
        ss << v->IRType_->print() << " ";
    }

    // 根据Value具体类型拼接不同格式的操作数
    if (dynamic_cast<GlobalVariable*>(v)) {
        ss << "@" << v->name_;
    } else if (dynamic_cast<Function*>(v)) {
        ss << "@" << v->name_;
    } else if (dynamic_cast<Constant*>(v)) {
        ss << v->print();
    } else {
        // 其他类型（如指令、参数等）使用%前缀
        ss << "%" << v->name_;
    }

    return ss.str();
}
std::string print_cmp_IRType(ICmpInst::ICmpOp op) {
    if (op == ICmpInst::ICMP_SGE) {
        return "sge";
    } else if (op == ICmpInst::ICMP_SGT) {
        return "sgt";
    } else if (op == ICmpInst::ICMP_SLE) {
        return "sle";
    } else if (op == ICmpInst::ICMP_SLT) {
        return "slt";
    } else if (op == ICmpInst::ICMP_EQ) {
        return "eq";
    } else if (op == ICmpInst::ICMP_NE) {
        return "ne";
    } else {
        return "wrong cmpop";
    }
}

std::string print_fcmp_IRType(FCmpInst::FCmpOp op) {
    if (op == FCmpInst::FCMP_UGE) {
        return "uge";
    } else if (op == FCmpInst::FCMP_UGT) {
        return "ugt";
    } else if (op == FCmpInst::FCMP_ULE) {
        return "ule";
    } else if (op == FCmpInst::FCMP_ULT) {
        return "ult";
    } else if (op == FCmpInst::FCMP_UEQ) {
        return "ueq";
    } else if (op == FCmpInst::FCMP_UNE) {
        return "une";
    } else {
        return "wrong fcmpop";
    }
}

void Function::set_instr_name()
{
    std::map<Value *, int> seq;
    for (auto arg : this->arguments_)
    {
        if (!seq.count(arg))
        {
            auto seq_num = seq.size() + seq_cnt_;
            if (arg->name_ == "")
            {
                arg->name_ = "arg_" + std::to_string(seq_num);
                seq.insert({arg, seq_num});
            }
        }
    }
    for (auto bb : basic_blocks_)
    {
        if (!seq.count(bb))
        {
            auto seq_num = seq.size() + seq_cnt_;
            if (bb->name_.length() <= 6 || bb->name_.substr(0, 6) != "label_")
            {
                bb->name_ = "label_" + std::to_string(seq_num);
                seq.insert({bb, seq_num});
            }
        }
        for (auto instr : bb->instr_list_)
        {
            if (instr->IRType_->tid_ != IRType::VoidTyID && !seq.count(instr))
            {
                auto seq_num = seq.size() + seq_cnt_;
                if (instr->name_ == "")
                {
                    instr->name_ = "v" + std::to_string(seq_num);
                    seq.insert({instr, seq_num});
                }
            }
        }
    }
    seq_cnt_ += seq.size();
}
