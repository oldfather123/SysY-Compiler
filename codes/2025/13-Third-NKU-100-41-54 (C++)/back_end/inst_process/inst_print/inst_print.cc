#include"inst_print.h"
#include"../phi_process.h"
#include <assert.h>
#include<string.h>
bool isMemFormatOp(int opcode) {
    return opcode == RISCV_LB || opcode == RISCV_LBU || opcode == RISCV_LH || opcode == RISCV_LHU ||
           opcode == RISCV_LW || opcode == RISCV_LWU || opcode == RISCV_LD || opcode == RISCV_FLW ||
           opcode == RISCV_FLD || opcode == RISCV_FSW || opcode == RISCV_FSD;
}

template <> void RiscV64Printer::printRVfield<Register *>(Register *printee) {
    if (!printee->is_virtual) {
        s << RiscV64Registers.at(printee->reg_no).name;
    } else {
        s << "%" << printee->reg_no;
    }
}

template <> void RiscV64Printer::printRVfield<Register>(Register printee) {
    if (!printee.is_virtual) {
        s << RiscV64Registers.at(printee.reg_no).name;
    } else {
        s << "%" << printee.reg_no;
    }
}

template <> void RiscV64Printer::printRVfield<RiscVLabel *>(RiscVLabel *ins) {
    if (ins->is_data_symbol()) {
        if (ins->is_hi()) {
            s << "%hi(" << ins->get_data_name() << ")";
        } else {
            s << "%lo(" << ins->get_data_name() << ")";
        }
    } else {
        s << "." << current_func->getFunctionName() << "_" << ins->get_jmp_id();
    }
}

template <> void RiscV64Printer::printRVfield<RiscVLabel>(RiscVLabel ins) {
    if (ins.is_data_symbol()) {
        if (ins.is_hi()) {
            s << "%hi(" << ins.get_data_name() << ")";
        } else {
            s << "%lo(" << ins.get_data_name() << ")";
        }
    } else {
        s << "." << current_func->getFunctionName() << "_" << ins.get_jmp_id();
    }
}

template <> void RiscV64Printer::printRVfield<MachineBaseOperand *>(MachineBaseOperand *op) {
    if (op->op_type == MachineBaseOperand::REG) {
        auto reg_op = (MachineRegister *)op;
        printRVfield(reg_op->reg);
    } else if (op->op_type == MachineBaseOperand::IMMI) {
        auto immi_op = (MachineImmediateInt *)op;
        s << immi_op->imm32;
    } else if (op->op_type == MachineBaseOperand::IMMF) {
        auto immf_op = (MachineImmediateFloat *)op;
        s << immf_op->fimm32;
    }
}

template <> void RiscV64Printer::printAsm<MachineBaseInstruction *>(MachineBaseInstruction *ins);
template <> void RiscV64Printer::printAsm<RiscV64Instruction *>(RiscV64Instruction *ins) {
    // First, handle pseudo-instructions that don't have an OpTable entry.
    if (ins->arch == MachineBaseInstruction::LOCAL_LABEL) {
        s << ins->getLocalLabelId() << ":";
        return; // This is just a label, print and exit.
    }

    // Now we know it's a real RiscV instruction, so it's safe to access OpTable.
    s << OpTable.at(ins->getOpcode()).name << "\t\t";
    if (strlen(OpTable.at(ins->getOpcode()).name) <= 3) {
        s << "\t";
    }
    switch (OpTable.at(ins->getOpcode()).ins_formattype) {
    case RvOpInfo::R_type:
        printRVfield(ins->getRd());
        s << ",";
        printRVfield(ins->getRs1());
        s << ",";
        printRVfield(ins->getRs2());
        break;
    case RvOpInfo::R2_type:
        printRVfield(ins->getRd());
        s << ",";
        printRVfield(ins->getRs1());
        if (ins->getOpcode() == RISCV_FCVT_W_S || ins->getOpcode() == RISCV_FCVT_WU_S) {
            s << ",rtz";
        }
        break;
    case RvOpInfo::R4_type:
        printRVfield(ins->getRd());
        s << ",";
        printRVfield(ins->getRs1());
        s << ",";
        printRVfield(ins->getRs2());
        s << ",";
        printRVfield(ins->getRs3());
        break;
    case RvOpInfo::I_type:
        printRVfield(ins->getRd());
        s << ",";
        if (!isMemFormatOp(ins->getOpcode())) {
            printRVfield(ins->getRs1());
            s << ",";
            if (ins->getUseLabel()) {
                if (ins->getRelocType() == PCREL_LO) {
                    s << "%pcrel_lo(" << ins->getLocalLabelId() << "b)";
                } else {
                    printRVfield(ins->getLabel());
                }
            } else {
                s << ins->getImm();
            }
        } else {
            if (ins->getUseLabel()) {
                if (ins->getRelocType() == PCREL_LO) {
                    s << "%pcrel_lo(" << ins->getLocalLabelId() << "b)";
                } else {
                    printRVfield(ins->getLabel());
                }
            } else {
                s << ins->getImm();
            }
            s << "(";
            printRVfield(ins->getRs1());
            s << ")";
        }
        break;
    case RvOpInfo::S_type:
        printRVfield(ins->getRs1());
        s << ",";
        if (ins->getUseLabel()) {
            if (ins->getRelocType() == PCREL_LO) {
                s << "%pcrel_lo(" << ins->getLocalLabelId() << "b)";
            } else {
                printRVfield(ins->getLabel());
            }
        } else {
            s << ins->getImm();
        }
        s << "(";
        printRVfield(ins->getRs2());
        s << ")";
        break;
    case RvOpInfo::B_type:
        printRVfield(ins->getRs1());
        s << ",";
        printRVfield(ins->getRs2());
        s << ",";
        if (ins->getUseLabel()) {
            printRVfield(ins->getLabel());
        } else {
            s << ins->getImm();
        }
        break;
    case RvOpInfo::U_type:
        printRVfield(ins->getRd());
        s << ",";
        if (ins->getUseLabel()) {
			if(ins->getOpcode() == RISCV_LA) 
				s << ins->getLabel().get_data_name();
			else if (ins->getRelocType() == PCREL_HI) {
                s << "%pcrel_hi(" << ins->getLabel().get_data_name() << ")";
            }
			else 
				printRVfield(ins->getLabel());
        } else {
			// special case : li 指令作为 u 型指令处理，但是可以传入无符号整数
			if(ins->getOpcode() == RISCV_LI) {
				s << ins->getImm();
			} else { // 只保留20位无符号立即数
				s << (0x000FFFFF & (unsigned int)ins->getImm());
			}
        }
        break;
    case RvOpInfo::J_type:
        printRVfield(ins->getRd());
        s << ",";
        if (ins->getUseLabel()) {
            printRVfield(ins->getLabel());
        } else {
            s << ins->getImm();
        }
        break;
    case RvOpInfo::CALL_type:
        s << ins->getLabel().get_data_name();
        break;
    //default:
        //ERROR("Unexpected instruction format");
    }
}

template <> void RiscV64Printer::printAsm<MachinePhiInstruction *>(MachinePhiInstruction *ins) {
    // Lazy("Phi Output");
    // s << "# ";
    printRVfield(ins->GetResult());
    s << " = " << ins->GetResult().type.toString() << " PHI ";
    for (auto [label, op] : ins->GetPhiList()) {
        s << "[";
        printRVfield(op);
        s << ",%L" << label;
        s << "] ";
    }
}

template <> void RiscV64Printer::printAsm<MachineBaseInstruction *>(MachineBaseInstruction *ins) {
    if (ins->arch == MachineBaseInstruction::RiscV) {
        printAsm<RiscV64Instruction *>((RiscV64Instruction *)ins);
        return;
    } else if (ins->arch == MachineBaseInstruction::PHI) {
        printAsm<MachinePhiInstruction *>((MachinePhiInstruction *)ins);
        return;
    } else if (ins->arch == MachineBaseInstruction::LOCAL_LABEL) {
        // This case is handled inside printAsm<RiscV64Instruction*>, 
        // but we need to call it to print the label.
        printAsm<RiscV64Instruction *>((RiscV64Instruction *)ins);
        return;
    } else {
        //ERROR("Unknown Instruction");
        return;
    }
}

void RiscV64Printer::SyncFunction(MachineFunction *func) { current_func = func; }

void RiscV64Printer::SyncBlock(MachineBlock *block) { cur_block = block; }

void add_parallel_loop_constant_100(std::ostream &s) {	
	s << "\t.extern\tmemcpy\n";
	s << "\t.extern\tpthread_create\n";
	s << "\t.extern\tpthread_join\n";
	s << "\t.globl\t___parallel_loop_constant_100\n";
	s << "\t.type\t___parallel_loop_constant_100, @function\n";
	s << "___parallel_loop_constant_100:\n";
	s << "\taddi\tsp,sp,-176\n";
	s << "\tsd\ts1,120(sp)\n";
	s << "\tmv\ts1,a3\n";
	s << "\tslliw\ta3,a4,1\n";
	s << "\taddw\ta3,a3,s1\n";
	s << "\tsd\ts3,104(sp)\n";
	s << "\tslliw\ts3,a3,2\n";
	s << "\taddiw\ts3,s3,12\n";
	s << "\taddi\tt1,s3,15\n";
	s << "\tsd\ts0,128(sp)\n";
	s << "\tsd\ts2,112(sp)\n";
	s << "\tsd\ts4,96(sp)\n";
	s << "\tsd\ts5,88(sp)\n";
	s << "\tsd\ts6,80(sp)\n";
	s << "\tsd\ts8,64(sp)\n";
	s << "\tsd\tra,136(sp)\n";
	s << "\tsd\ts7,72(sp)\n";
	s << "\tsd\ts9,56(sp)\n";
	s << "\taddi\ts0,sp,144\n";
	s << "\tandi\tt1,t1,-16\n";
	s << "\tsub\tsp,sp,t1\n";
	s << "\tmv\ts8,sp\n";
	s << "\tsub\tsp,sp,t1\n";
	s << "\tsd\ta5,8(s0)\n";
	s << "\tsd\ta6,16(s0)\n";
	s << "\tsd\ta7,24(s0)\n";
	s << "\tmv\ts6,sp\n";
	s << "\tsub\tsp,sp,t1\n";
	s << "\tmv\ts5,sp\n";
	s << "\tsw\tzero,0(s8)\n";
	s << "\tsub\tsp,sp,t1\n";
	s << "\tsw\ta1,4(s8)\n";
	s << "\tsw\ta2,8(s8)\n";
	s << "\tmv\ts2,a0\n";
	s << "\tmv\ts4,sp\n";
	s << "\tbne\ta3,zero,.L17\n";
	s << ".L2:\n";
	s << "\tmv\ta2,s3\n";
	s << "\tmv\ta1,s8\n";
	s << "\tmv\ta0,s6\n";
	s << "\tcall\tmemcpy\n";
	s << "\tmv\ta2,s3\n";
	s << "\tmv\ta1,s8\n";
	s << "\tmv\ta0,s5\n";
	s << "\tcall\tmemcpy\n";
	s << "\tmv\ta2,s3\n";
	s << "\tmv\ta1,s8\n";
	s << "\tmv\ta0,s4\n";
	s << "\tcall\tmemcpy\n";
	s << "\tli\ta5,1\n";
	s << "\tsw\ta5,0(s6)\n";
	s << "\tli\ta5,2\n";
	s << "\tsw\ta5,0(s5)\n";
	s << "\tli\ta5,3\n";
	s << "\tsw\ta5,0(s4)\n";
	s << "\tmv\ta2,s2\n";
	s << "\tli\ta1,0\n";
	s << "\tmv\ta3,s8\n";
	s << "\taddi\ta0,s0,-128\n";
	s << "\tcall\tpthread_create\n";
	s << "\tmv\ta2,s2\n";
	s << "\tmv\ta3,s6\n";
	s << "\tli\ta1,0\n";
	s << "\taddi\ta0,s0,-120\n";
	s << "\tcall\tpthread_create\n";
	s << "\tmv\ta2,s2\n";
	s << "\tmv\ta3,s5\n";
	s << "\tli\ta1,0\n";
	s << "\taddi\ta0,s0,-112\n";
	s << "\tcall\tpthread_create\n";
	s << "\tmv\ta2,s2\n";
	s << "\tmv\ta3,s4\n";
	s << "\tli\ta1,0\n";
	s << "\taddi\ta0,s0,-104\n";
	s << "\tcall\tpthread_create\n";
	s << "\taddi\ts1,s0,-128\n";
	s << "\taddi\ts2,s0,-96\n";
	s << ".L8:\n";
	s << "\tld\ta0,0(s1)\n";
	s << "\tli\ta1,0\n";
	s << "\taddi\ts1,s1,8\n";
	s << "\tcall\tpthread_join\n";
	s << "\tbne\ts2,s1,.L8\n";
	s << "\taddi\tsp,s0,-144\n";
	s << "\tld\tra,136(sp)\n";
	s << "\tld\ts0,128(sp)\n";
	s << "\tld\ts1,120(sp)\n";
	s << "\tld\ts2,112(sp)\n";
	s << "\tld\ts3,104(sp)\n";
	s << "\tld\ts4,96(sp)\n";
	s << "\tld\ts5,88(sp)\n";
	s << "\tld\ts6,80(sp)\n";
	s << "\tld\ts7,72(sp)\n";
	s << "\tld\ts8,64(sp)\n";
	s << "\tld\ts9,56(sp)\n";
	s << "\taddi\tsp,sp,176\n";
	s << "\tjr\tra\n";
	s << ".L17:\n";
	s << "\tslli\ta2,s1,2\n";
	s << "\taddi\ta5,a2,15\n";
	s << "\tandi\ta5,a5,-16\n";
	s << "\taddi\ta6,s0,8\n";
	s << "\tmv\ts7,sp\n";
	s << "\tsd\ta6,-136(s0)\n";
	s << "\tsub\tsp,sp,a5\n";
	s << "\tmv\ts9,a4\n";
	s << "\tmv\ta1,sp\n";
	s << "\tble\ts1,zero,.L3\n";
	s << "\tslli\ta0,s1,3\n";
	s << "\tmv\ta5,a6\n";
	s << "\tmv\ta3,a1\n";
	s << "\tadd\ta0,a6,a0\n";
	s << ".L4:\n";
	s << "\tlw\ta4,0(a5)\n";
	s << "\taddi\ta5,a5,8\n";
	s << "\taddi\ta3,a3,4\n";
	s << "\tsw\ta4,-4(a3)\n";
	s << "\tbne\ta5,a0,.L4\n";
	s << "\tslli\ta4,s1,32\n";
	s << "\tsrli\ta5,a4,29\n";
	s << "\tadd\ta6,a6,a5\n";
	s << "\tsd\ta6,-136(s0)\n";
	s << ".L3:\n";
	s << "\taddi\ta0,s8,12\n";
	s << "\taddiw\ts1,s1,3\n";
	s << "\tcall\tmemcpy\n";
	s << "\tslliw\ts1,s1,2\n";
	s << "\tble\ts9,zero,.L7\n";
	s << "\tld\ta1,-136(s0)\n";
	s << "\tslli\ta4,s9,3\n";
	s << "\tadd\ta5,s8,s1\n";
	s << "\tadd\ta4,a1,a4\n";
	s << ".L6:\n";
	s << "\tld\ta3,0(a1)\n";
	s << "\taddi\ta1,a1,8\n";
	s << "\taddi\ta5,a5,8\n";
	s << "\tsd\ta3,-8(a5)\n";
	s << "\tbne\ta4,a1,.L6\n";
	s << ".L7:\n";
	s << "\tmv\tsp,s7\n";
	s << "\tj\t.L2\n";
}

void RiscV64Printer::emit() {
    s << "\t.text\n\t.globl main\n";
	// to support loop parallelism, we need use more instructions
    // s << "\t.attribute arch, \"rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_zicsr2p0_zifencei2p0_zba1p0_zbb1p0\"\n";
    s << "\t.attribute arch, \"rv64gc\"\n";

	// 添加并行循环运行时库函数
	// add_parallel_loop_constant_100(s);
	// 裸机不支持 pthread, 需要使用系统调用实现轻量化并行库

	for (auto func : printee->functions) {
        current_func = func;
        s << func->getFunctionName() << ":\n";

        // 这里直接采用顺序输出的方式，当然这种汇编代码布局效率非常低下，你可以自行编写一个更好的代码布局方法
        // 你可以搜索指令cache, 软件分支预测等关键字来了解代码布局的作用及方法
        for (auto block : func->blocks) {
            int block_id = block->getLabelId();
            s << "." << func->getFunctionName() << "_" << block_id << ":\n";
            cur_block = block;
            for (auto ins : *block) {
                if (ins->arch == MachineBaseInstruction::RiscV) {
                    s << "\t";
                    printAsm((RiscV64Instruction *)ins);
                    s << "\n";
                } else if (ins->arch == MachineBaseInstruction::PHI) {
                    s << "\t";
                    printAsm((MachinePhiInstruction *)ins);
                    s << "\n";
                } else if (ins->arch == MachineBaseInstruction::LOCAL_LABEL) {
                    // Local labels don't need indentation
                    printAsm((RiscV64Instruction *)ins);
                    s << "\n";
                } else {
                    //ERROR("Unexpected arch");
                }
            }
        }
    }
    s << "\t.data\n";    // 输出全局变量定义指令
    for (auto global : printee->IR->global_def) {
        if (global->GetOpcode() == BasicInstruction::GLOBAL_VAR) {
            auto global_ins = (GlobalVarDefineInstruction *)global;
            s << global_ins->name << ":\n";
            if (global_ins->type == BasicInstruction::I32) {
                if (global_ins->arrayval.dims.empty()) {
                    if (global_ins->init_val != nullptr) {
                        Assert(global_ins->init_val->GetOperandType() == BasicOperand::IMMI32);
                        auto imm_op = (ImmI32Operand *)global_ins->init_val;
                        s << "\t.word\t" << imm_op->GetIntImmVal() << "\n";
                    } else {
                        s << "\t.word\t0\n";
                    }
                } else {
                    int zero_cum = 0;
                    for (auto val : global_ins->arrayval.IntInitVals) {
                        if (val == 0) {
                            zero_cum += 4;
                        } else {
                            if (zero_cum != 0) {
                                s << "\t.zero\t" << zero_cum << "\n";
                                zero_cum = 0;
                            }
                            s << "\t.word\t" << val << "\n";
                        }
                    }
                    if (global_ins->arrayval.IntInitVals.empty()) {
                        int prod = 1;
                        for (auto dim : global_ins->arrayval.dims) {
                            prod *= dim;
                        }
                        s << "\t.zero\t" << prod * 4 << "\n";
                    }
                    if (zero_cum != 0) {
                        s << "\t.zero\t" << zero_cum << "\n";
                        zero_cum = 0;
                    }
                }
            } else if (global_ins->type == BasicInstruction::FLOAT32) {
                if (global_ins->arrayval.dims.empty()) {
                    if (global_ins->init_val != nullptr) {
                        Assert(global_ins->init_val->GetOperandType() == BasicOperand::IMMF32);
                        auto imm_op = (ImmF32Operand *)global_ins->init_val;
                        auto immf = imm_op->GetFloatVal();
                        s << "\t.word\t" << *(int *)&immf << "\n";
                    } else {
                        s << "\t.word\t0\n";
                    }
                } else {
                    int zero_cum = 0;
                    for (auto val : global_ins->arrayval.FloatInitVals) {
                        if (val == 0) {
                            zero_cum += 4;
                        } else {
                            if (zero_cum != 0) {
                                s << "\t.zero\t" << zero_cum << "\n";
                                zero_cum = 0;
                            }
                            s << "\t.word\t" << *(int *)&val << "\n";
                        }
                    }
                    if (global_ins->arrayval.FloatInitVals.empty()) {
                        int prod = 1;
                        for (auto dim : global_ins->arrayval.dims) {
                            prod *= dim;
                        }
                        s << "\t.zero\t" << prod * 4 << "\n";
                    }
                    if (zero_cum != 0) {
                        s << "\t.zero\t" << zero_cum << "\n";
                        zero_cum = 0;
                    }
                }
            } else if (global_ins->type == BasicInstruction::I64) {
                Assert(global_ins->arrayval.dims.empty());
                if (global_ins->init_val != nullptr) {
                    Assert(global_ins->init_val->GetOperandType() == BasicOperand::IMMI64);
                    auto imm_op = (ImmI64Operand *)global_ins->init_val;
                    auto imm_ll = imm_op->GetLlImmVal();
                    s << "\t.quad\t" << imm_ll << "\n";
                } else {
                    s << "\t.quad\t0\n";
                }
            }
        } else if (global->GetOpcode() == BasicInstruction::GLOBAL_STR) {
            auto str_ins = (GlobalStringConstInstruction *)global;
            s << str_ins->str_name << ":\n";
            s << "\t.asciz\t"
              << "\"" << str_ins->str_val << "\""
              << "\n";
        } else {
            //ERROR("Unexpected global define instruction");
        }
    }
}
