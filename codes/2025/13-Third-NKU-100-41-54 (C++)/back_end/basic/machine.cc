#include"machine.h"
#include "assert.h"
#include<unordered_map>
#include "../inst_process/inst_print/inst_print.h"
MachineDataType INT32(MachineDataType::INT, MachineDataType::B32);
MachineDataType INT64(MachineDataType::INT, MachineDataType::B64);
MachineDataType INT128(MachineDataType::INT, MachineDataType::B128);

MachineDataType FLOAT_32(MachineDataType::FLOAT, MachineDataType::B32);
MachineDataType FLOAT64(MachineDataType::FLOAT, MachineDataType::B64);
MachineDataType FLOAT128(MachineDataType::FLOAT, MachineDataType::B128);
std::unordered_map<int, RiscV64RegisterInfo> RiscV64Registers = {
    {RISCV_x0,   {"x0"}},
    {RISCV_x1,   {"ra"}},
    {RISCV_x2,   {"sp"}},
    {RISCV_x3,   {"gp"}},
    {RISCV_x4,   {"tp"}},
    {RISCV_x5,   {"t0"}},
    {RISCV_x6,   {"t1"}},
    {RISCV_x7,   {"t2"}},
    {RISCV_x8,   {"fp"}},
    {RISCV_x9,   {"s1"}},
    {RISCV_x10,  {"a0"}},
    {RISCV_x11,  {"a1"}},
    {RISCV_x12,  {"a2"}},
    {RISCV_x13,  {"a3"}},
    {RISCV_x14,  {"a4"}},
    {RISCV_x15,  {"a5"}},
    {RISCV_x16,  {"a6"}},
    {RISCV_x17,  {"a7"}},
    {RISCV_x18,  {"s2"}},
    {RISCV_x19,  {"s3"}},
    {RISCV_x20,  {"s4"}},
    {RISCV_x21,  {"s5"}},
    {RISCV_x22,  {"s6"}},
    {RISCV_x23,  {"s7"}},
    {RISCV_x24,  {"s8"}},
    {RISCV_x25,  {"s9"}},
    {RISCV_x26,  {"s10"}},
    {RISCV_x27,  {"s11"}},
    {RISCV_x28,  {"t3"}},
    {RISCV_x29,  {"t4"}},
    {RISCV_x30,  {"t5"}},
    {RISCV_x31,  {"t6"}},
    {RISCV_f0,   {"ft0"}},
    {RISCV_f1,   {"ft1"}},
    {RISCV_f2,   {"ft2"}},
    {RISCV_f3,   {"ft3"}},
    {RISCV_f4,   {"ft4"}},
    {RISCV_f5,   {"ft5"}},
    {RISCV_f6,   {"ft6"}},
    {RISCV_f7,   {"ft7"}},
    {RISCV_f8,   {"fs0"}},
    {RISCV_f9,   {"fs1"}},
    {RISCV_f10,  {"fa0"}},
    {RISCV_f11,  {"fa1"}},
    {RISCV_f12,  {"fa2"}},
    {RISCV_f13,  {"fa3"}},
    {RISCV_f14,  {"fa4"}},
    {RISCV_f15,  {"fa5"}},
    {RISCV_f16,  {"fa6"}},
    {RISCV_f17,  {"fa7"}},
    {RISCV_f18,  {"fs2"}},
    {RISCV_f19,  {"fs3"}},
    {RISCV_f20,  {"fs4"}},
    {RISCV_f21,  {"fs5"}},
    {RISCV_f22,  {"fs6"}},
    {RISCV_f23,  {"fs7"}},
    {RISCV_f24,  {"fs8"}},
    {RISCV_f25,  {"fs9"}},
    {RISCV_f26,  {"fs10"}},
    {RISCV_f27,  {"fs11"}},
    {RISCV_f28,  {"ft8"}},
    {RISCV_f29,  {"ft9"}},
    {RISCV_f30,  {"ft10"}},
    {RISCV_f31,  {"ft11"}},
    {RISCV_INVALID,   {"INVALID"}},
    {RISCV_spilled_in_memory, {"spilled_in_memory"}}
};
Register RISCVregs[] = {
Register(false, RISCV_x0, INT64),    Register(false, RISCV_x1, INT64),    Register(false, RISCV_x2, INT64),
Register(false, RISCV_x3, INT64),    Register(false, RISCV_x4, INT64),    Register(false, RISCV_x5, INT64),
Register(false, RISCV_x6, INT64),    Register(false, RISCV_x7, INT64),    Register(false, RISCV_x8, INT64),
Register(false, RISCV_x9, INT64),    Register(false, RISCV_x10, INT64),   Register(false, RISCV_x11, INT64),
Register(false, RISCV_x12, INT64),   Register(false, RISCV_x13, INT64),   Register(false, RISCV_x14, INT64),
Register(false, RISCV_x15, INT64),   Register(false, RISCV_x16, INT64),   Register(false, RISCV_x17, INT64),
Register(false, RISCV_x18, INT64),   Register(false, RISCV_x19, INT64),   Register(false, RISCV_x20, INT64),
Register(false, RISCV_x21, INT64),   Register(false, RISCV_x22, INT64),   Register(false, RISCV_x23, INT64),
Register(false, RISCV_x24, INT64),   Register(false, RISCV_x25, INT64),   Register(false, RISCV_x26, INT64),
Register(false, RISCV_x27, INT64),   Register(false, RISCV_x28, INT64),   Register(false, RISCV_x29, INT64),
Register(false, RISCV_x30, INT64),   Register(false, RISCV_x31, INT64),   Register(false, RISCV_f0, FLOAT64),
Register(false, RISCV_f1, FLOAT64),  Register(false, RISCV_f2, FLOAT64),  Register(false, RISCV_f3, FLOAT64),
Register(false, RISCV_f4, FLOAT64),  Register(false, RISCV_f5, FLOAT64),  Register(false, RISCV_f6, FLOAT64),
Register(false, RISCV_f7, FLOAT64),  Register(false, RISCV_f8, FLOAT64),  Register(false, RISCV_f9, FLOAT64),
Register(false, RISCV_f10, FLOAT64), Register(false, RISCV_f11, FLOAT64), Register(false, RISCV_f12, FLOAT64),
Register(false, RISCV_f13, FLOAT64), Register(false, RISCV_f14, FLOAT64), Register(false, RISCV_f15, FLOAT64),
Register(false, RISCV_f16, FLOAT64), Register(false, RISCV_f17, FLOAT64), Register(false, RISCV_f18, FLOAT64),
Register(false, RISCV_f19, FLOAT64), Register(false, RISCV_f20, FLOAT64), Register(false, RISCV_f21, FLOAT64),
Register(false, RISCV_f22, FLOAT64), Register(false, RISCV_f23, FLOAT64), Register(false, RISCV_f24, FLOAT64),
Register(false, RISCV_f25, FLOAT64), Register(false, RISCV_f26, FLOAT64), Register(false, RISCV_f27, FLOAT64),
Register(false, RISCV_f28, FLOAT64), Register(false, RISCV_f29, FLOAT64), Register(false, RISCV_f30, FLOAT64),
Register(false, RISCV_f31, FLOAT64),
};
//OpTable.at(key)
std::unordered_map<int, RvOpInfo> OpTable = {
    {RISCV_SLL,    {RvOpInfo::R_type,   "sll",    1}},
    {RISCV_SLLI,   {RvOpInfo::I_type,   "slli",   1}},
    {RISCV_SRL,    {RvOpInfo::R_type,   "srl",    1}},
    {RISCV_SRLI,   {RvOpInfo::I_type,   "srli",   1}},
    {RISCV_SRA,    {RvOpInfo::R_type,   "sra",    1}},
    {RISCV_SRAI,   {RvOpInfo::I_type,   "srai",   1}},
    {RISCV_ADD,    {RvOpInfo::R_type,   "add",    1}},
    {RISCV_ADDI,   {RvOpInfo::I_type,   "addi",   1}},
    {RISCV_SUB,    {RvOpInfo::R_type,   "sub",    1}},
    {RISCV_LUI,    {RvOpInfo::U_type,   "lui",    1}},
    {RISCV_AUIPC,  {RvOpInfo::U_type,   "auipc",  1}},
    {RISCV_XOR,    {RvOpInfo::R_type,   "xor",    1}},
    {RISCV_XORI,   {RvOpInfo::I_type,   "xori",   1}},
    {RISCV_OR,     {RvOpInfo::R_type,   "or",     1}},
    {RISCV_ORI,    {RvOpInfo::I_type,   "ori",    1}},
    {RISCV_AND,    {RvOpInfo::R_type,   "and",    1}},
    {RISCV_ANDI,   {RvOpInfo::I_type,   "andi",   1}},
    {RISCV_SLT,    {RvOpInfo::R_type,   "slt",    1}},
    {RISCV_SLTI,   {RvOpInfo::I_type,   "slti",   1}},
    {RISCV_SLTU,   {RvOpInfo::R_type,   "sltu",   1}},
    {RISCV_SLTIU,  {RvOpInfo::I_type,   "sltiu",  1}},
    {RISCV_BEQ,    {RvOpInfo::B_type,   "beq",    1}},
    {RISCV_BNE,    {RvOpInfo::B_type,   "bne",    1}},
    {RISCV_BLT,    {RvOpInfo::B_type,   "blt",    1}},
    {RISCV_BGE,    {RvOpInfo::B_type,   "bge",    1}},
    {RISCV_BLTU,   {RvOpInfo::B_type,   "bltu",   1}},
    {RISCV_BGEU,   {RvOpInfo::B_type,   "bgeu",   1}},
    {RISCV_JAL,    {RvOpInfo::J_type,   "jal",    1}},
    {RISCV_JALR,   {RvOpInfo::I_type,   "jalr",   1}},
    {RISCV_LB,     {RvOpInfo::I_type,   "lb",     3}},
    {RISCV_LH,     {RvOpInfo::I_type,   "lh",     3}},
    {RISCV_LBU,    {RvOpInfo::I_type,   "lbu",    3}},
    {RISCV_LHU,    {RvOpInfo::I_type,   "lhu",    3}},
    {RISCV_LW,     {RvOpInfo::I_type,   "lw",     3}},
    {RISCV_SB,     {RvOpInfo::S_type,   "sb",     1}},
    {RISCV_SH,     {RvOpInfo::S_type,   "sh",     1}},
    {RISCV_SW,     {RvOpInfo::S_type,   "sw",     1}},
    {RISCV_SLLW,   {RvOpInfo::R_type,   "sllw",   1}},
    {RISCV_SLLIW,  {RvOpInfo::I_type,   "slliw",  1}},
    {RISCV_SRLW,   {RvOpInfo::R_type,   "srlw",   1}},
    {RISCV_SRLIW,  {RvOpInfo::I_type,   "srliw",  1}},
    {RISCV_SRAW,   {RvOpInfo::R_type,   "sraw",   1}},
    {RISCV_SRAIW,  {RvOpInfo::I_type,   "sraiw",  1}},
    {RISCV_ADDW,   {RvOpInfo::R_type,   "addw",   1}},
    {RISCV_ADDIW,  {RvOpInfo::I_type,   "addiw",  1}},
    {RISCV_SUBW,   {RvOpInfo::R_type,   "subw",   1}},
    {RISCV_LWU,    {RvOpInfo::I_type,   "lwu",    3}},
    {RISCV_LD,     {RvOpInfo::I_type,   "ld",     3}},
    {RISCV_SD,     {RvOpInfo::S_type,   "sd",     1}},
    {RISCV_MUL,    {RvOpInfo::R_type,   "mul",    3}},
    {RISCV_MULH,   {RvOpInfo::R_type,   "mulh",   3}},
    {RISCV_MULHSU, {RvOpInfo::R_type,   "mulhsu", 3}},
    {RISCV_MULHU,  {RvOpInfo::R_type,   "mulhu",  3}},
    {RISCV_DIV,    {RvOpInfo::R_type,   "div",    30}},
    {RISCV_DIVU,   {RvOpInfo::R_type,   "divu",   30}},
    {RISCV_REM,    {RvOpInfo::R_type,   "rem",    30}},
    {RISCV_REMU,   {RvOpInfo::R_type,   "remu",   30}},
    {RISCV_MULW,   {RvOpInfo::R_type,   "mulw",   3}},
    {RISCV_DIVW,   {RvOpInfo::R_type,   "divw",   30}},
    {RISCV_REMW,   {RvOpInfo::R_type,   "remw",   30}},
    {RISCV_REMUW,  {RvOpInfo::R_type,   "remuw",  30}},
    {RISCV_FMV_W_X,{RvOpInfo::R2_type,  "fmv.w.x",2}},
    {RISCV_FMV_X_W,{RvOpInfo::R2_type,  "fmv.x.w",1}},
    {RISCV_FCVT_S_W,{RvOpInfo::R2_type, "fcvt.s.w",2}},
    {RISCV_FCVT_D_W,{RvOpInfo::R2_type, "fcvt.d.w",2}},
    {RISCV_FCVT_S_WU,{RvOpInfo::R2_type,"fcvt.s.wu",2}},
    {RISCV_FCVT_D_WU,{RvOpInfo::R2_type,"fcvt.d.wu",2}},
    {RISCV_FCVT_W_S,{RvOpInfo::R2_type, "fcvt.w.s",4}},
    {RISCV_FCVT_W_D,{RvOpInfo::R2_type, "fcvt.w.d",4}},
    {RISCV_FCVT_WU_S,{RvOpInfo::R2_type,"fcvt.wu.s",4}},
    {RISCV_FCVT_WU_D,{RvOpInfo::R2_type,"fcvt.wu.d",4}},
    {RISCV_FLW,    {RvOpInfo::I_type,   "flw",    2}},
    {RISCV_FLD,    {RvOpInfo::I_type,   "fld",    2}},
    {RISCV_FSW,    {RvOpInfo::S_type,   "fsw",    4}},
    {RISCV_FSD,    {RvOpInfo::S_type,   "fsd",    4}},
    {RISCV_FADD_S, {RvOpInfo::R_type,   "fadd.s", 5}},
    {RISCV_FADD_D, {RvOpInfo::R_type,   "fadd.d", 7}},
    {RISCV_FSUB_S, {RvOpInfo::R_type,   "fsub.s", 5}},
    {RISCV_FSUB_D, {RvOpInfo::R_type,   "fsub.d", 7}},
    {RISCV_FMUL_S, {RvOpInfo::R_type,   "fmul.s", 5}},
    {RISCV_FMUL_D, {RvOpInfo::R_type,   "fmul.d", 7}},
    {RISCV_FDIV_S, {RvOpInfo::R_type,   "fdiv.s", 30}},
    {RISCV_FDIV_D, {RvOpInfo::R_type,   "fdiv.d", 30}},
    {RISCV_FSQRT_S,{RvOpInfo::R2_type,  "fsqrt.s",15}},
    {RISCV_FSQRT_D,{RvOpInfo::R2_type,  "fsqrt.d",30}},
    {RISCV_FMADD_S,{RvOpInfo::R4_type,  "fmadd.s",5}},
    {RISCV_FMADD_D,{RvOpInfo::R4_type,  "fmadd.d",7}},
    {RISCV_FMSUB_S,{RvOpInfo::R4_type,  "fmsub.s",5}},
    {RISCV_FMSUB_D,{RvOpInfo::R4_type,  "fmsub.d",7}},
    {RISCV_FNMSUB_S,{RvOpInfo::R4_type, "fnmsub.s",5}},
    {RISCV_FNMSUB_D,{RvOpInfo::R4_type, "fnmsub.d",7}},
    {RISCV_FNMADD_S,{RvOpInfo::R4_type, "fnmadd.s",5}},
    {RISCV_FNMADD_D,{RvOpInfo::R4_type, "fnmadd.d",7}},
    {RISCV_FSGNJ_S, {RvOpInfo::R_type,   "fsgnj.s",2}},
    {RISCV_FSGNJ_D, {RvOpInfo::R_type,   "fsgnj.d",2}},
    {RISCV_FSGNJN_S,{RvOpInfo::R_type,  "fsgnjn.s",2}},
    {RISCV_FSGNJN_D,{RvOpInfo::R_type,  "fsgnjn.d",2}},
    {RISCV_FSGNJX_S,{RvOpInfo::R_type,  "fsgnjx.s",2}},
    {RISCV_FSGNJX_D,{RvOpInfo::R_type,  "fsgnjx.d",2}},
    {RISCV_FMIN_S,  {RvOpInfo::R_type,   "fmin.s", 2}},
    {RISCV_FMIN_D,  {RvOpInfo::R_type,   "fmin.d", 2}},
    {RISCV_FMAX_S,  {RvOpInfo::R_type,   "fmax.s", 2}},
    {RISCV_FMAX_D,  {RvOpInfo::R_type,   "fmax.d", 2}},
    {RISCV_FEQ_S,   {RvOpInfo::R_type,   "feq.s",  4}},
    {RISCV_FEQ_D,   {RvOpInfo::R_type,   "feq.d",  4}},
    {RISCV_FLT_S,   {RvOpInfo::R_type,   "flt.s",  4}},
    {RISCV_FLT_D,   {RvOpInfo::R_type,   "flt.d",  4}},
    {RISCV_FLE_S,   {RvOpInfo::R_type,   "fle.s",  4}},
    {RISCV_FLE_D,   {RvOpInfo::R_type,   "fle.d",  4}},
    {RISCV_FCLASS_S,{RvOpInfo::R2_type, "fclass.s",4}},
    {RISCV_FCLASS_D,{RvOpInfo::R2_type, "fclass.d",4}},
    {RISCV_FMV_D_X, {RvOpInfo::R2_type,  "fmv.d.x",6}},
    {RISCV_FMV_X_D, {RvOpInfo::R2_type,  "fmv.x.d",1}},
    {RISCV_FCVT_S_L,{RvOpInfo::R2_type, "fcvt.s.l",4}},
    {RISCV_FCVT_D_L,{RvOpInfo::R2_type, "fcvt.d.l",6}},
    {RISCV_FCVT_S_LU,{RvOpInfo::R2_type,"fcvt.s.lu",4}},
    {RISCV_FCVT_D_LU,{RvOpInfo::R2_type,"fcvt.d.lu",6}},
    {RISCV_FCVT_L_S,{RvOpInfo::R2_type, "fcvt.l.s",4}},
    {RISCV_FCVT_L_D,{RvOpInfo::R2_type, "fcvt.l.d",4}},
    {RISCV_FCVT_LU_S,{RvOpInfo::R2_type,"fcvt.lu.s",4}},
    {RISCV_FCVT_LU_D,{RvOpInfo::R2_type,"fcvt.lu.d",4}},
    {RISCV_LI,      {RvOpInfo::U_type,   "li",     1}},
	{RISCV_LA,      {RvOpInfo::U_type,   "la",     1}},
    {RISCV_CALL,    {RvOpInfo::CALL_type,"call",   1}},
    {RISCV_BGT,     {RvOpInfo::B_type,   "bgt",    1}},
    {RISCV_BLE,     {RvOpInfo::B_type,   "ble",    1}},
    {RISCV_BGTU,    {RvOpInfo::B_type,   "bgtu",   1}},
    {RISCV_BLEU,    {RvOpInfo::B_type,   "bleu",   1}},
    {RISCV_FMV_S,   {RvOpInfo::R2_type,  "fmv.s",  2}},
    {RISCV_FMV_D,   {RvOpInfo::R2_type,  "fmv.d",  2}},
    {RISCV_SH1ADD,  {RvOpInfo::R_type,   "sh1add", 1}},
    {RISCV_SH2ADD,  {RvOpInfo::R_type,   "sh2add", 1}},
    {RISCV_SH3ADD,  {RvOpInfo::R_type,   "sh3add", 1}},
    {RISCV_SH1ADDUW,{RvOpInfo::R_type,  "sh1add.uw",1}},
    {RISCV_SH2ADDUW,{RvOpInfo::R_type,  "sh2add.uw",1}},
    {RISCV_SH3ADDUW,{RvOpInfo::R_type,  "sh3add.uw",1}},
    {RISCV_MIN,     {RvOpInfo::R_type,   "min",    1}},
    {RISCV_MAX,     {RvOpInfo::R_type,   "max",    1}},
    {RISCV_MINU,    {RvOpInfo::R_type,   "minu",   1}},
    {RISCV_MAXU,    {RvOpInfo::R_type,   "maxu",   1}},
    {RISCV_FCVT_D_S,{RvOpInfo::R2_type,  "fcvt.d.s",2}},
    {RISCV_ZEXT_W,  {RvOpInfo::R2_type,  "zext.w", 1}},
    {RISCV_FNEG_S,  {RvOpInfo::R2_type,  "fneg.s", 2}},
    {RISCV_FNEG_D,  {RvOpInfo::R2_type,  "fneg.d", 2}}
};


void MachineCFG::AssignEmptyNode(int id, MachineBlock *Mblk) {
    if (id > this->max_label) {
        this->max_label = id;
    }
    MachineCFGNode *node = new MachineCFGNode;
    node->Mblock = Mblk;
    block_map[id] = node;
    while (G.size() < id + 1) {
        G.push_back({});
        // G.resize(id + 1);
    }
    while (invG.size() < id + 1) {
        invG.push_back({});
        // invG.resize(id + 1);
    }
}

// Just modify CFG edge, no change on branch instructions
void MachineCFG::MakeEdge(int edg_begin, int edg_end) {
    Assert(block_map.find(edg_begin) != block_map.end());
    Assert(block_map.find(edg_end) != block_map.end());
    Assert(block_map[edg_begin] != nullptr);
    Assert(block_map[edg_end] != nullptr);
    G[edg_begin].push_back(block_map[edg_end]);
    invG[edg_end].push_back(block_map[edg_begin]);
}

// Just modify CFG edge, no change on branch instructions
void MachineCFG::RemoveEdge(int edg_begin, int edg_end) {
    assert(block_map.find(edg_begin) != block_map.end());
    assert(block_map.find(edg_end) != block_map.end());
    auto it = G[edg_begin].begin();
    for (; it != G[edg_begin].end(); ++it) {
        if ((*it)->Mblock->getLabelId() == edg_end) {
            break;
        }
    }
    G[edg_begin].erase(it);

    auto jt = invG[edg_end].begin();
    for (; jt != invG[edg_end].end(); ++jt) {
        if ((*jt)->Mblock->getLabelId() == edg_begin) {
            break;
        }
    }
    invG[edg_end].erase(jt);
}

Register MachineFunction::GetNewRegister(int regtype, int reglength, bool save_across_call) {
    static int new_regno = 0;
    Register new_reg;
    new_reg.is_virtual = true;
    new_reg.reg_no = new_regno++;
    new_reg.type.data_type = regtype;
    new_reg.type.data_length = reglength;
    //new_reg.save_across_call = save_across_call;
    // InitializeNewVirtualRegister(new_regno);
    // extern int frameend;
    // if(frameend){std::cout<<new_regno<<std::endl;}
    //检验是否死循环
    return new_reg;
}

Register MachineFunction::GetNewReg(MachineDataType type) { return GetNewRegister(type.data_type, type.data_length); }

MachineBlock *MachineFunction::InitNewBlock() {
    int new_id = ++max_exist_label;
    MachineBlock *new_block =new RiscV64Block(new_id);
    new_block->setParent(this);
    blocks.push_back(new_block);
    mcfg->AssignEmptyNode(new_id, new_block);
    return new_block;
}

void MachineBlock::display() {
    std::cerr << "\n[基本块 B" << label_id << "] 指令列表:" << std::endl;
    RiscV64Printer printer(std::cerr, parent->getParentMachineUnit());
    printer.SyncBlock(this);
    printer.SyncFunction(parent);
    for (auto ins : instructions) {
        std::cerr << "  ";
        printer.printAsm(ins);
        std::cerr << std::endl;
    }
}

std::list<MachineBaseInstruction *>::iterator RiscV64Block::getInsertBeforeBrIt() {
    auto it = --instructions.end();
    auto jal_pos = it;
    if (instructions.empty()) {
        return instructions.end();
    }
    for (auto it = --instructions.end(); it != --instructions.begin(); --it) {
        if ((*it)->arch == MachineBaseInstruction::PHI) {
            continue;
        }
        if ((*it)->arch != MachineBaseInstruction::RiscV) {
            return jal_pos;
        }
        // Assert((*it)->arch == MachineBaseInstruction::RiscV);
        auto rvlast = (RiscV64Instruction *)(*it);
        if (rvlast->getOpcode() == RISCV_JALR) {
            return it;
        }
        if (rvlast->getOpcode() == RISCV_JAL) {
            jal_pos = it;
            continue;
        }
        if (OpTable[rvlast->getOpcode()].ins_formattype == RvOpInfo::B_type) {
            return it;
        } else {
            return jal_pos;
        }
    }
    return it;
}


void MachineCFG::SetMachineDomTree(DominatorTree* domtree){

    DomTree->sdom_map.insert(domtree->sdom_map.begin(), domtree->sdom_map.end());
    
    for(auto &llvmblocks:domtree->dom_tree){
        std::vector<MachineBlock*> vecs;
        for(auto llvmblock:llvmblocks){
            int block_id=llvmblock->block_id;
            vecs.push_back(GetNodeByBlockId(block_id)->Mblock);
        }
        DomTree->dom_tree.push_back(vecs);
    }

    return ;
}

bool MachineDominatorTree::IsDominate(int id1, int id2){
    MachineBlock* a = C->GetNodeByBlockId(id1)->Mblock;
    MachineBlock* b = C->GetNodeByBlockId(id1)->Mblock;
    if (a == nullptr || b == nullptr) {
		return false;
	}
	
	int current_id = id2;
	while (current_id != -1) {
		if (current_id == id1) return true;
		if (current_id == 0) break;  // no exit loop
		current_id = sdom_map.count(current_id) ? sdom_map[current_id] : -1;
	}
	return false;
}