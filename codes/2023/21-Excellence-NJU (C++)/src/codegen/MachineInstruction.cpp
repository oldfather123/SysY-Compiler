#include "codegen/MachineInstruction.hpp"
#include "codegen/MachineBlock.hpp"
#include "codegen/MachineOperand.hpp"
#include "codegen/utils/NotImplemented.hpp"
#include "codegen/utils/UtilFunctions.hpp"
#include "magic_enum.hpp"

#include <memory>

int MachineInstruction::counter = 0;

using v_ptr_iter = std::vector<MachineInstruction *>::iterator;

void MachineInstruction::insertBefore(MachineInstruction *inst) {
    auto& instructions = parent->inst_list;
    // auto it = std::find(instructions.begin(), instructions.end(), this);
    // instructions.insert(it, inst);
    auto cur = current_it;
    inst->current_it = instructions.insert(cur, inst);
}

void MachineInstruction::insertAfter(MachineInstruction *inst) {
    auto& instructions = parent->inst_list;
    // auto it = std::find(instructions.begin(), instructions.end(), this);
    // instructions.insert(++it, inst);
    auto cur = current_it;
    ++cur;
    inst->current_it = instructions.insert(cur, inst);
}

BinaryMInstruction::BinaryMInstruction(MachineBlock *mBlock,
                                       int opcode,
                                       MachineOperand *dst,
                                       MachineOperand *src1,
                                       MachineOperand *src2)
    : MachineInstruction(mBlock, MachineInstruction::BINARY, opcode)
{
    def_list.push_back(dst);
    use_list.push_back(src1);
    use_list.push_back(src2);
    dst->parent = src1->parent = src2->parent = this;
}

void BinaryMInstruction::output(std::ostream &os) const {
    os << "\t" << codegen_utils::lower(magic_enum::enum_name(static_cast<BinaryMInstruction::OpType>(optype)));
    switch (optype) {
        case FADD: case FSUB: case FDIV: case FMUL: case FMAX: case FMIN:
            os << ".s ";
        default:
            os << " ";
    }
    def_list[0]->output(os), os << ", ";
    use_list[0]->output(os), os << ", ";
    use_list[1]->output(os);
    switch (optype) {
        case FADD: case FSUB: case FDIV: case FMUL: /*case FMAX: case FMIN:*/
            // os << ", rtz";
            break;
        default:
            break;
    }
}

// ld rd, offset(rs1)
LoadMInstruction::LoadMInstruction(MachineBlock *mBlock,
                                   int opcode,
                                   MachineOperand *rd,
                                   MachineOperand *offset,
                                   MachineOperand *rs1)
    : MachineInstruction(mBlock, MachineInstruction::LOAD, opcode)
{
    def_list.push_back(rd);
    use_list.push_back(offset);
    use_list.push_back(rs1);
    rd->parent = offset->parent = rs1->parent = this;
}

void LoadMInstruction::output(std::ostream &os) const {
    os << "\t" << codegen_utils::lower(magic_enum::enum_name(static_cast<LoadMInstruction::OpType>(optype))), os << " ";
    def_list[0]->output(os), os << ", ";
    use_list[0]->output(os), os << "(";
    use_list[1]->output(os), os << ")";
//    Z : throw NotImplemented();
}

LoadImmInstruction::LoadImmInstruction(MachineBlock *mBlock, int opcode, MachineOperand *dst, MachineOperand *imm)
        : MachineInstruction(mBlock, MachineInstruction::LOAD, opcode){
    def_list.push_back(dst);
    use_list.push_back(imm);
    dst->parent = imm->parent = this;
}

void LoadImmInstruction::output(std::ostream &os) const {
    LoadImmInstruction::OpType t = static_cast<LoadImmInstruction::OpType>(optype);
    if(t == LI || t == LA){
        os << "\t" << codegen_utils::lower(magic_enum::enum_name(static_cast<LoadImmInstruction::OpType>(optype))), os << " ";
        def_list[0]->output(os), os << ", ";
        use_list[0]->output(os);
    }else if(t == LUI){
        os << "\t" << codegen_utils::lower(magic_enum::enum_name(static_cast<LoadImmInstruction::OpType>(optype))), os << " ";
        def_list[0]->output(os), os << ", ";
        os << "%hi(";
        use_list[0]->output(os), os << ")";
    }

//    throw NotImplemented();
}

// sd rs2, offset(rs1)
StoreMInstruction::StoreMInstruction(MachineBlock *mBlock,
                                     int opcode,
                                     MachineOperand *rs2,
                                     MachineOperand *offset,
                                     MachineOperand *rs1)
    : MachineInstruction(mBlock, MachineInstruction::STORE, opcode)
{
    use_list.push_back(rs2);
    use_list.push_back(offset);
    use_list.push_back(rs1);
    rs2->parent = offset->parent = rs1->parent = this;
}

void StoreMInstruction::output(std::ostream &os) const {
    // TODO
    os << "\t" << codegen_utils::lower(magic_enum::enum_name(static_cast<StoreMInstruction::OpType>(optype))), os << " ";
    use_list[0]->output(os), os << ", ";
    use_list[1]->output(os), os << "(";
    use_list[2]->output(os), os << ")";
}

JrMInstruction::JrMInstruction(MachineBlock *mBlock, int opcode, MachineOperand *ra)
    : MachineInstruction(mBlock, MachineInstruction::JUMP, opcode){
    use_list.push_back(ra);
    ra->parent=this;
}

void JrMInstruction::output(std::ostream &os) const {
//    ???
    os << "\t" << codegen_utils::lower(magic_enum::enum_name(static_cast<JrMInstruction::OpType>(optype))), os << " ";
    use_list[0]->output(os), os << "\t";
}

JalMInstruction::JalMInstruction(MachineBlock *mBlock, int opcode, MachineOperand *ra, MachineOperand *label)
        : MachineInstruction(mBlock, MachineInstruction::JUMP, opcode){
    use_list.push_back(ra);
    use_list.push_back(label);
    ra->parent=label->parent=this;
}

void JalMInstruction::output(std::ostream &os) const {
    os << "\t" << codegen_utils::lower(magic_enum::enum_name(static_cast<JalMInstruction::OpType>(optype))), os << " ";
    use_list[0]->output(os), os << ", ";
    use_list[1]->output(os);
}

BranchMInstruction::BranchMInstruction(MachineBlock *mBlock,
                                       int opcode,
                                       MachineOperand *reg1,
                                       MachineOperand *reg2,
                                       MachineOperand *label)
    : MachineInstruction(mBlock, MachineInstruction::BRANCH, opcode)
{
    use_list.push_back(reg1);
    use_list.push_back(reg2);
    use_list.push_back(label);
    reg1->parent=reg2->parent=label->parent=this;
}

void BranchMInstruction::output(std::ostream &os) const {
    os << "\t" << codegen_utils::lower(magic_enum::enum_name(static_cast<BranchMInstruction::OpType>(optype))), os << " ";
    use_list[0]->output(os), os << ", ";
    use_list[1]->output(os), os << ", ";
    use_list[2]->output(os);
}

CmpMInstruction::CmpMInstruction(MachineBlock *mBlock, int opcode, MachineOperand *dst, MachineOperand *src1, MachineOperand *src2)
    : MachineInstruction(mBlock, MachineInstruction::CMP, opcode)
{
    def_list.push_back(dst);
    use_list.push_back(src1);
    use_list.push_back(src2);
    dst->parent=src1->parent=src2->parent;
}

void CmpMInstruction::output(std::ostream &os) const {
    os << "\t" << codegen_utils::lower(magic_enum::enum_name(static_cast<CmpMInstruction::OpType>(optype)));
    switch (optype) {
        case FEQ: case FGE: case FGT: case FLE: case FLT:
            os << ".s ";
        default:
            os << " ";
    }
    def_list[0]->output(os), os << ", ";
    use_list[0]->output(os), os << ", ";
    use_list[1]->output(os);
}

JumpMInstruction::JumpMInstruction(MachineBlock *mBlock,
                                   int opcode,
                                   MachineOperand *operand)
    : MachineInstruction(mBlock, MachineInstruction::JUMP, opcode)
{
//    j,jr
    use_list.push_back(operand);
    operand->parent = this;
}

void JumpMInstruction::output(std::ostream &os) const {
    os << "\tj  " << this->use_list[0]->label;
}

MoveMInstruction::MoveMInstruction(MachineBlock *mBlock, int opcode, MachineOperand *dst, MachineOperand *src)
    :MachineInstruction(mBlock, MachineInstruction::MOVE, opcode){
    use_list.push_back(src);
    def_list.push_back(dst);
    dst->parent = src->parent = this;
}

void MoveMInstruction::output(std::ostream &os) const {
//    os << "\tfmv.s ";
    os << "\t" << codegen_utils::lower(magic_enum::enum_name(static_cast<MoveMInstruction::OpType>(optype)));
    if(optype == FMV) os << ".s "; else os << " ";
    def_list[0]->output(os), os << ", ";
    use_list[0]->output(os);
}

ConvertMInstruction::ConvertMInstruction(MachineBlock *mBlock, int opcode, MachineOperand *dst, MachineOperand *src)
    :MachineInstruction(mBlock, MachineInstruction::CONVERT, opcode){
    use_list.push_back(src);
    def_list.push_back(dst);
    dst->parent = src->parent = this;
}

void ConvertMInstruction::output(std::ostream &os) const {
    if(this->optype == 0){
//        F2I
        os << "\tfcvt.w.s ";
    }else if(this->optype == 1){
//        I2F
        os << "\tfcvt.s.w ";
    }
    def_list[0]->output(os), os << ", ";
    use_list[0]->output(os);
    os << ", rtz";
}

FnegMInstruction::FnegMInstruction(MachineBlock *mBlock, int type, MachineOperand *dst, MachineOperand *src)
        :MachineInstruction(mBlock, MachineInstruction::BINARY, type){
    use_list.push_back(src);
    def_list.push_back(dst);
    dst->parent = src->parent = this;
}

void FnegMInstruction::output(std::ostream &os) const {
    os << "\tfneg.s ";
    def_list[0]->output(os), os << ", ";
    use_list[0]->output(os);
}

CallFuncMInstruction::CallFuncMInstruction(MachineBlock *mBlock, int opcode, MachineOperand *dst)
        :MachineInstruction(mBlock, MachineInstruction::JUMP, opcode){
    use_list.push_back(dst);
    dst->parent = this;
}

void CallFuncMInstruction::output(std::ostream &os) const {
    os << "\tcall ";
    use_list[0]->output(os);
}
