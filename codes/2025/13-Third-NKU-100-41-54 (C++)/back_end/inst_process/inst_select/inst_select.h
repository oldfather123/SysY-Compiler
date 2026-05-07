#ifndef INST_SELECT_H
#define INST_SELECT_H
#include"../../basic/machine.h"
#include"../../../include/ir.h"
#include "dominator_tree.h"
//参考原架构的MachineUnit，MachineSelector及其继承类;MachinePass与其中一个继承类RiscV64LowerFrame
//为了方便理解和调试，原来代码的类定义注释后附在文件后方

const int imm12Min = -2048, imm12Max= 2047;
const int immF12Min = -2048, immF12Max= 2047;
#define IMM32_MASKNUM 0xFFF;
#define COMPLE_MASKNUM 0xFFFFFFFF;
#define BITS_ELEVEN 2048
#define ELEVEN_BITS 2047
#define TWENTYTHERE_BITS 8388607
#define EIGHT_BITS 255

class MachineUnit {
public:
    // 指令选择时，对全局变量不作任何处理，直接保留到MachineUnit中
    std::unordered_set<Instruction> global_def{};
    std::vector<MachineFunction *> functions;
 
public:
    LLVMIR *IR;//指向中间代码阶段的信息
    MachineUnit(LLVMIR *IR) : IR(IR) {}
    virtual void SelectInstructionAndBuildCFG() = 0;
    virtual void LowerFrame() = 0;
    virtual void LowerStack() = 0;
};

class RiscV64Unit : public MachineUnit
{
private:
    int cur_offset;    	// 局部变量在栈中的偏移
	MachineFunction *cur_func;
    MachineBlock *cur_block;

	CFG *cur_cfg;		

	int phiseq; 		// phiweb个数

	std::map<int, int> phi_temp_phiseqmap;			// 左值为resultregno, 右值为phiseq
	std::map<int, Register> phi_temp_registermap;	// 左值为phiseq, 右值为对应的临时寄存器
	std::map<int, bool> block_terminal_type;		// 左值为基本块号, 右值为是否为 brcond

	std::map<int, MachineBlock*> curblockmap;

    // TODO(): 添加更多你需要的成员变量和函数
	// llvm中寄存器操作数Operand的寄存器编号和rv64中的寄存器Register的映射
	std::map<int, Register> llvm2rv_regmap;
	Register GetNewRegister(int llvm_regNo, MachineDataType type);
	// 构造临时虚拟寄存器，不存在llvm_reg到rv64_reg的映射，即不更新llvm2rv_regmap
	Register GetNewTempRegister(MachineDataType type);
    Register GetI32A(int a_num);
    Register GetF32A(int a_num);

	// llvm中寄存器操作数Operand的寄存器编号和rv64中栈偏移stack_offset的映射
	std::map<int, int> llvmReg_offset_map;

	// 预存比较指令结果寄存器与比较指令的映射, 例如 %t2.vreg -> %t2 = icmp ne i32 %t1, 0	
	std::map<Register, Instruction> resReg_cmpInst_map;

	// 每个machineblock应该在终止指令前生成的指令
	std::map<int, std::vector<MachineBaseInstruction *>> phi_instr_map;
	// 每个machineblock的所有终止指令之前的最后一个指令
	std::map<int, MachineBaseInstruction *> terminal_instr_map;
	// 由于 mem2reg 析构生成的跨基本块中间寄存器
	std::set<Register> mem2reg_destruc_set;

	/* cite from https://llvm.org/docs/LangRef.html#icmp-instruction
	 * %cmp1 = icmp eq i32 %x, %y    ->    beq x1, x2, label    // if x1 == x2, jump to label
	 * %cmp2 = icmp ne i32 %x, %y    ->    bne x1, x2, label    // if x1 != x2, jump to label
	 * %cmp3 = icmp ugt i32 %x, %y   ->    bgtu x1, x2, label   // if unsigned x1 > x2, jump to label
	 * %cmp4 = icmp uge i32 %x, %y   ->    bgeu x1, x2, label   // if unsigned x1 >= x2, jump to label
	 * %cmp5 = icmp ult i32 %x, %y   ->    bltu x1, x2, label   // if unsigned x1 < x2, jump to label
	 * %cmp6 = icmp ule i32 %x, %y   ->    bleu x1, x2, label   // if unsigned x1 <= x2, jump to label
	 * %cmp7 = icmp sgt i32 %x, %y   ->    bgt x1, x2, label    // if signed x1 > x2, jump to label
	 * %cmp8 = icmp sge i32 %x, %y   ->    bge x1, x2, label    // if signed x1 >= x2, jump to label
	 * %cmp9 = icmp slt i32 %x, %y   ->    blt x1, x2, label    // if signed x1 < x2, jump to label
	 * %cmp10 = icmp sle i32 %x, %y  ->    ble x1, x2, label    // if signed x1 <= x2, jump to label
	*/
	std::map<BasicInstruction::IcmpCond, RISCV_INST> IcmpSelect_map = {
		{BasicInstruction::IcmpCond::eq , RISCV_BEQ },
		{BasicInstruction::IcmpCond::ne , RISCV_BNE },
		{BasicInstruction::IcmpCond::ugt, RISCV_BGTU},
		{BasicInstruction::IcmpCond::uge, RISCV_BGEU},
		{BasicInstruction::IcmpCond::ult, RISCV_BLTU},
		{BasicInstruction::IcmpCond::ule, RISCV_BLEU},
		{BasicInstruction::IcmpCond::sgt, RISCV_BGT },
		{BasicInstruction::IcmpCond::sge, RISCV_BGE },
		{BasicInstruction::IcmpCond::slt, RISCV_BLT },
		{BasicInstruction::IcmpCond::sle, RISCV_BLE }
	};

	/* %cmp1 = fcmp oeq float %x, %y    ->    feq.s f1, f2, f3    // if f2 == f3, set f1
	 * %cmp2 = fcmp ogt float %x, %y    ->    fle.s f1, f2, f3    // if f2 > f3 , set f1   !(f2 <= f3)
	 * %cmp3 = fcmp oge float %x, %y    ->    flt.s f1, f3, f2    // if f2 >= f3, set f1   !(f2 <  f3)
	 * %cmp4 = fcmp olt float %x, %y    ->    flt.s f1, f2, f3    // if f2 < f3 , set f1
	 * %cmp5 = fcmp ole float %x, %y    ->    fle.s f1, f2, f3    // if f2 <= f3, set f1
	 * %cmp6 = fcmp one float %x, %y    ->    feq.s f1, f2, f3    // if f2 <= f3, set f1   !(f2 == f3)
	 * Currently unimplemented instructions : FALSE、UEQ、UEQ、UGT、UGE、ULT、ULE、UNO、TRUE
	*/
	std::map<BasicInstruction::FcmpCond, RISCV_INST> FcmpSelect_map = {
		{BasicInstruction::FcmpCond::OEQ, RISCV_FEQ_S },
		{BasicInstruction::FcmpCond::OGT, RISCV_FLE_S },
		{BasicInstruction::FcmpCond::OGE, RISCV_FLT_S },
		{BasicInstruction::FcmpCond::OLT, RISCV_FLT_S },
		{BasicInstruction::FcmpCond::OLE, RISCV_FLE_S },
		{BasicInstruction::FcmpCond::ONE, RISCV_FEQ_S }
	};

	// not 意味着，比较指令的结果为0的时候成真，跳转到ture_label
	std::map<BasicInstruction::FcmpCond, RISCV_INST> Not_map = {  
		{BasicInstruction::FcmpCond::OEQ, RISCV_BNE },
		{BasicInstruction::FcmpCond::OGT, RISCV_BEQ },  // not
		{BasicInstruction::FcmpCond::OGE, RISCV_BEQ },  // not
		{BasicInstruction::FcmpCond::OLT, RISCV_BNE },
		{BasicInstruction::FcmpCond::OLE, RISCV_BNE },
		{BasicInstruction::FcmpCond::ONE, RISCV_BEQ }	// not
	};

	void BuildPhiWeb(CFG *C);


public:
    RiscV64Unit(LLVMIR *IR):MachineUnit(IR){}

	/* 指令选择相关 */
	void SelectInstructionAndBuildCFG();
    void ClearFunctionSelectState();
    void InsertImmI32Instruction(Register resultRegister, Operand op, MachineBlock* insert_block, bool isPhiPost = 0);
    void InsertImmFloat32Instruction(Register resultRegister, Operand op, MachineBlock* insert_block, bool isPhiPost = 0);
    template <class Instruction> void ConvertAndAppend(Instruction);
	void DomtreeDfs(BasicBlock* ubb, CFG *C);
	int InsertArgInStack(BasicInstruction::LLVMType type, Operand arg_op, int &arg_off, MachineBlock* insert_block, int &nr_iarg, int &nr_farg);

    void LowerFrame();//替代原来的RiscV64LowerFrame(m_unit).Execute();
    void LowerStack();//替代原来的RiscV64LowerStack(m_unit).Execute();
};

#endif