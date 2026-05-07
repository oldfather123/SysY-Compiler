#include "riscv_generator.hpp"
#include "ir_type.hpp"
#include "ir_instruction.hpp"
#include "ir_basic_block.hpp"
#include "ir_function.hpp"
#include "riscv_basic_block.hpp"
#include "riscv_instruction.hpp"
#include "riscv_function.hpp"
#include "register.hpp"
#include "reg_alloc.hpp"

unordered_map<IRInstID, RiscvInstID> iid_to_riid = {
    {IRInstID::Add, RiscvInstID::ADD},
    {IRInstID::Sub, RiscvInstID::SUB},
    {IRInstID::Mul, RiscvInstID::MUL},
    {IRInstID::Div, RiscvInstID::DIV},
    {IRInstID::Rem, RiscvInstID::REM},
    {IRInstID::FAdd, RiscvInstID::FADD},
    {IRInstID::FSub, RiscvInstID::FSUB},
    {IRInstID::FMul, RiscvInstID::FMUL},
    {IRInstID::FDiv, RiscvInstID::FDIV},
    {IRInstID::Shl, RiscvInstID::SHL},
    {IRInstID::AShr, RiscvInstID::ASHR},
    {IRInstID::LShr, RiscvInstID::LSHR},
    {IRInstID::ICmp, RiscvInstID::ICMP},
    {IRInstID::FCmp, RiscvInstID::FCMP},
    {IRInstID::FpToSi, RiscvInstID::FPTOSI},
    {IRInstID::SiToFp, RiscvInstID::SITOFP},
    {IRInstID::Load, RiscvInstID::LW},
    {IRInstID::Store, RiscvInstID::SW},
    {IRInstID::Call, RiscvInstID::CALL},
    {IRInstID::Ret, RiscvInstID::RET}
};

RiscvBinaryInst* RiscvGenerator::translate_binary_inst(RegAlloc* reg_alloc, BinaryInst* binary_inst, RiscvBasicBlock* rbb) {
    if(!iid_to_riid.count(binary_inst->iid)) {
        cerr << "[Error] Unsupported binary operation: " << static_cast<int>(binary_inst->iid) << endl;
        exit(1);
    }
    auto riid = iid_to_riid[binary_inst->iid];
    // NOTE: 中间代码优化部分会进行常量折叠，此处不需要再次进行
    return new RiscvBinaryInst(riid, reg_alloc->get_reg_op(binary_inst->get_op(0), rbb),
        reg_alloc->get_reg_op(binary_inst->get_op(1), rbb),reg_alloc->get_reg_op(binary_inst, rbb, nullptr, false), rbb, true);
}

RiscvCallInst* RiscvGenerator::translate_call_inst(RegAlloc* reg_alloc, CallInst* call_inst, RiscvBasicBlock* rbb) {
    return new RiscvCallInst(get_rfunc_from_func(static_cast<Function*>(call_inst->operands[call_inst->operands.size() - 1])), rbb);
}

RiscvReturnInst* RiscvGenerator::translate_ret_inst(RegAlloc* reg_alloc, ReturnInst* ret_inst, RiscvBasicBlock* rbb, RiscvFunction* rfunc) {
    if(!ret_inst->operands.empty()) { // 有返回值，写到 a0/fa0 中
        RiscvValue* reg_to_save = nullptr;
        auto ret_val = ret_inst->operands[0];
        // 选择寄存器类型，整型寄存器 or 浮点寄存器
        if(ret_val->type->tid == TypeID::IntegerTy) {
            reg_to_save = get_reg_op_named("a0");
        } else if(ret_val->type->tid == TypeID::FloatTy) {
            reg_to_save = get_reg_op_named("fa0");
        }
        auto inst = reg_alloc->write_back_reg(reg_to_save, rbb); // 如果寄存器保存了信息，将寄存器内容写回内存
        auto reg = reg_alloc->get_reg_op(ret_val, rbb, nullptr);
        rbb->add_rinst_back(new RiscvMoveInst(reg_to_save, reg, rbb)); // 添加语句，将返回值写入 a0/fa0
        reg_alloc->clear_reg(reg); // 删除寄存器的映射关系
    }
    return new RiscvReturnInst(rbb);
}

vector<RiscvInstruction*> RiscvGenerator::translate_load_inst(RegAlloc* reg_alloc, LoadInst* load_inst, RiscvBasicBlock* rbb) {
    auto cur_type = static_cast<PointerType*>(load_inst->operands[0]->type);
    vector<RiscvInstruction*> rinsts;
    auto reg_op = reg_alloc->get_reg_op(static_cast<Value*>(load_inst), rbb, nullptr, 0);
    rinsts.push_back(new RiscvLoadInst(cur_type->pointed_type, reg_op, reg_alloc->get_mem_op(load_inst->operands[0], rbb, nullptr, false), rbb));
    return rinsts;
}

vector<RiscvInstruction*> RiscvGenerator::translate_store_inst(RegAlloc* reg_alloc, StoreInst* store_inst, RiscvBasicBlock* rbb) {
    vector<RiscvInstruction*> rinsts;
    if(auto const_int = dynamic_cast<ConstInt*>(store_inst->operands[0])) { // 将一个整数常量储存到指定位置
        auto reg_op = get_reg_op_named("t0");
        // 首先使用 LI 指令加载立即数
        rinsts.push_back(new RiscvLiInst(reg_op, const_int->val, rbb));
        // 指针类型找ptr
        rinsts.push_back(new RiscvStoreInst(store_inst->operands[0]->type, reg_op, reg_alloc->get_mem_op(store_inst->operands[1], rbb, nullptr, 0), rbb));
        return rinsts;
    }
    if(store_inst->operands[1]->type->tid == TypeID::PointerTy) {
        auto pointer_type = static_cast<PointerType*>(store_inst->operands[1]->type);
        RiscvStoreInst* inst = new RiscvStoreInst(pointer_type->pointed_type, reg_alloc->get_reg_op(store_inst->operands[0], rbb, nullptr),
                                                reg_alloc->get_mem_op(store_inst->operands[1], rbb, nullptr, 0), rbb);
        // return {inst};
        rinsts = {inst};
    }
    return rinsts;
}

RiscvInstruction* RiscvGenerator::translate_icmp_inst(RegAlloc* reg_alloc, ICmpInst* icmp_inst, BranchInst* br_inst, RiscvBasicBlock* rbb) {
    if(!icmp_to_str.count(icmp_inst->icmp_op)) { // 进行符号转换和操作数互换
        swap(icmp_inst->operands[0], icmp_inst->operands[1]);
        icmp_inst->icmp_op = icmp_transfer[icmp_inst->icmp_op];
    }
    RiscvICmpInst* inst = new RiscvICmpInst(icmp_inst->icmp_op, reg_alloc->get_reg_op(icmp_inst->operands[0], rbb),
        reg_alloc->get_reg_op(icmp_inst->operands[1], rbb), get_rbb_from_bb(static_cast<BasicBlock*>(br_inst->operands[1])),
        get_rbb_from_bb(static_cast<BasicBlock*>(br_inst->operands[2])), rbb);
    rbb->add_rinst_back(inst);
    return inst;
}

RiscvInstruction* RiscvGenerator::translate_icmp_set_inst(RegAlloc* reg_alloc, ICmpInst* icmp_inst, RiscvBasicBlock* rbb) {
    if(!icmp_set_to_str.count(icmp_inst->icmp_op)) { // 进行符号转换和操作数互换
        swap(icmp_inst->operands[0], icmp_inst->operands[1]);
        icmp_inst->icmp_op = icmp_transfer[icmp_inst->icmp_op];
    }
    RiscvICmpSetInst* inst = new RiscvICmpSetInst(icmp_inst->icmp_op, reg_alloc->get_reg_op(icmp_inst->operands[0], rbb, nullptr),
        reg_alloc->get_reg_op(icmp_inst->operands[1], rbb, nullptr), reg_alloc->get_reg_op(icmp_inst, rbb, nullptr, 0), rbb);
    rbb->add_rinst_back(inst);
    if(icmp_inst->icmp_op == ICmpOp::GE) { // 针对 >=，使用 < 的结果取反
        auto inst_reg = reg_alloc->get_reg_op(icmp_inst, rbb, nullptr, 0);
        rbb->add_rinst_back(new RiscvBinaryInst(RiscvInstID::XORI, inst_reg, new RiscvConst(1), inst_reg, rbb));
    }
    return inst;
}

RiscvInstruction* RiscvGenerator::translate_fcmp_inst(RegAlloc* reg_alloc, FCmpInst* fcmp_inst, RiscvBasicBlock* rbb) {
    if(!fcmp_to_str.count(fcmp_inst->fcmp_op)) { // 进行符号转换和操作数互换
        swap(fcmp_inst->operands[0], fcmp_inst->operands[1]);
        fcmp_inst->fcmp_op = fcmp_transfer[fcmp_inst->fcmp_op];
    }
    RiscvFCmpInst* inst = new RiscvFCmpInst(fcmp_inst->fcmp_op, reg_alloc->get_reg_op(fcmp_inst->operands[0], rbb),
        reg_alloc->get_reg_op(fcmp_inst->operands[1], rbb), reg_alloc->get_reg_op(fcmp_inst, rbb, nullptr, false), rbb);
        rbb->add_rinst_back(inst);
    if(fcmp_inst->fcmp_op == FCmpOp::FNE) { // 针对 !=，使用 == 的结果取反
        auto inst_reg = reg_alloc->get_reg_op(fcmp_inst, rbb, nullptr, 0);
        rbb->add_rinst_back(new RiscvBinaryInst(RiscvInstID::XORI, inst_reg, new RiscvConst(1), inst_reg, rbb));
        return inst;
    }
    return inst;
}

RiscvFpToSiInst* RiscvGenerator::translate_fptosi_inst(RegAlloc* reg_alloc, UnaryInst* fptosi_inst, RiscvBasicBlock* rbb) {
    return new RiscvFpToSiInst(reg_alloc->get_reg_op(fptosi_inst->operands[0], rbb, nullptr),
        reg_alloc->get_reg_op(static_cast<Value*>(fptosi_inst), rbb, nullptr, 0), rbb);
}

RiscvSiToFpInst* RiscvGenerator::translate_sitofp_inst(RegAlloc* reg_alloc, UnaryInst* sitofp_inst, RiscvBasicBlock* rbb) {
    return new RiscvSiToFpInst(reg_alloc->get_reg_op(sitofp_inst->operands[0], rbb, nullptr),
        reg_alloc->get_reg_op(static_cast<Value*>(sitofp_inst), rbb, nullptr, 0), rbb);
}

RiscvInstruction* RiscvGenerator::translate_br_inst(RegAlloc* reg_alloc, BranchInst* br_inst, RiscvBasicBlock* rbb) {
    RiscvInstruction* inst;
    if(br_inst->operands.size() == 1) {
        inst = new RiscvBranchInst(nullptr, nullptr, get_rbb_from_bb(static_cast<BasicBlock*>(br_inst->operands[0])), rbb);
        // inst = new RiscvJmpInst(get_rbb_from_bb(static_cast<BasicBlock*>(br_inst->operands[0])), rbb);
    } else {
        inst = new RiscvBranchInst(reg_alloc->get_reg_op(br_inst->operands[0], rbb, nullptr), get_rbb_from_bb(static_cast<BasicBlock*>(br_inst->operands[1])),
            get_rbb_from_bb(static_cast<BasicBlock*>(br_inst->operands[2])), rbb);
    }
    return inst;
}

// vector<RiscvInstruction*> RiscvGenerator::translate_br_inst(RegAlloc* reg_alloc, BranchInst* br_inst, RiscvBasicBlock* rbb) {
//     if(br_inst->operands.size() == 1) {
//         return {new RiscvBranchInst(nullptr, nullptr, get_rbb_from_bb(static_cast<BasicBlock*>(br_inst->operands[0])), rbb)};
//         // return {new RiscvJmpInst(get_rbb_from_bb(static_cast<BasicBlock*>(br_inst->operands[0])), rbb)};
//     } else {
//         vector<RiscvInstruction*> rinsts;
//         rinsts.push_back(new RiscvBranchInst(reg_alloc->get_reg_op(br_inst->operands[0], rbb, nullptr), get_rbb_from_bb(static_cast<BasicBlock*>(br_inst->operands[1])),
//             get_rbb_from_bb(static_cast<BasicBlock*>(br_inst->operands[2])), rbb));
//         rinsts.push_back(new RiscvJmpInst(get_rbb_from_bb(static_cast<BasicBlock*>(br_inst->operands[2])), rbb));
//         return rinsts;
//     }
// }

RiscvInstruction* RiscvGenerator::translate_gep_inst(RegAlloc* reg_alloc, GetElementPtrInst* gep_inst, RiscvBasicBlock* rbb) {
    if(gep_inst->has_base) {
        GetElementPtrInst* base = gep_inst->base;
        int offset = gep_inst->offset;
        rbb->add_rinst_back(new RiscvBinaryInst(RiscvInstID::ADDI, reg_alloc->get_reg_op(base, rbb, nullptr), new RiscvConst(offset),
            reg_alloc->get_reg_op(gep_inst, rbb, nullptr, 0), rbb));
        return nullptr;
    }
    Value* op0 = gep_inst->get_op(0);
    RiscvValue* target_reg_op = get_reg_op_named("t2");
    bool is_const = true; // 能否用确定的形如 -12(sp)访问
    // TODO: 此处 Instruction 和 Argument 的处理方式相同
    if(dynamic_cast<GlobalVariable*>(op0) != nullptr) {
        // 全局变量：使用la指令取基础地址
        is_const = false;
        rbb->add_rinst_back(new RiscvLoadAddrInst(target_reg_op, op0->name, rbb));
    } else if(dynamic_cast<Instruction*>(op0)) {
        // 获取指针指向的地址
        rbb->add_rinst_back(new RiscvMoveInst(target_reg_op, reg_alloc->get_reg_op(op0, rbb), rbb));
    } else if(dynamic_cast<Argument*>(op0)) {
        rbb->add_rinst_back(new RiscvMoveInst(target_reg_op, reg_alloc->get_reg_op(op0, rbb), rbb));
    }
    int cur_type_size = 0;
    int total_offset = 0;
    Type* cur_type = static_cast<PointerType*>(gep_inst->get_op(0)->type)->pointed_type;
    for(int i = 1; i <= gep_inst->operands.size() - 1; ++i) {
        if(i > 1) {
            cur_type = static_cast<ArrayType*>(cur_type)->element_type;
        }
        Value* cur_op = gep_inst->get_op(i);
        cur_type_size = calculate_type_size(cur_type);
        if(auto const_int = dynamic_cast<ConstInt*>(cur_op)) {
            total_offset += const_int->val * cur_type_size;
        } else {
            // 存在变量参与偏移量计算
            is_const = false;
            // 考虑目标数是int还是float
            RiscvValue* mul_reg_op = get_reg_op_named("t3");
            rbb->add_rinst_back(new RiscvLiInst(mul_reg_op, cur_type_size, rbb));
            rbb->add_rinst_back(new RiscvBinaryInst(RiscvInstID::MUL, reg_alloc->get_reg_op(cur_op, rbb, nullptr), mul_reg_op, mul_reg_op, rbb));
            rbb->add_rinst_back(new RiscvBinaryInst(RiscvInstID::ADD, mul_reg_op, target_reg_op, target_reg_op, rbb));
        }
    }
    rbb->add_rinst_back(new RiscvBinaryInst(RiscvInstID::ADDI, target_reg_op, new RiscvConst(total_offset), target_reg_op, rbb));
    rbb->add_rinst_back(new RiscvStoreInst(gep_inst->type, target_reg_op, reg_alloc->get_mem_op(gep_inst), rbb));
    return nullptr;
}

void RiscvGenerator::translate_phi_inst(RegAlloc* reg_alloc, BasicBlock* bb) {
    set<Instruction*> handled_phi; //用来记录前面扫描过的 phi 语句
    // 用于记录每个 phi 指令的每个操作数对应的 block 集合
    unordered_map<Instruction*, unordered_set<BasicBlock*>> phi_pre_bbs;
    unordered_map<Instruction*, RiscvInstruction*> phi_pre_vals;
    // 检查 phi inst 是否被分配对应的内存空间
    for(auto inst: bb->inst_list) {
        if(inst->iid != IRInstID::Phi) {
            continue;
        }
        handled_phi.insert(inst);
        // 寻找 Phi 指令的 Value 在内存中的位置
        auto mem_op = reg_alloc->get_mem_op(inst);
        if(mem_op == nullptr) {
            cerr << "[ERROR] [RiscvGenerator::translate_phi_inst] phi instruction " << inst->name << " has no memory slot allocated.\n";
            exit(1);
        }
        for(int i = 0; i < inst->operands.size(); i += 2) {
            phi_pre_bbs[inst].insert(dynamic_cast<BasicBlock*>(inst->operands[i + 1]));
            //对于操作数是常数的情况，不需要特殊处理
            if(inst->operands[i]->is_const()) {
                auto cur_bb = dynamic_cast<BasicBlock*>(inst->operands[i + 1]);
                auto cur_rbb = bb_to_rbb[cur_bb];
                auto const_int = dynamic_cast<ConstInt*>(inst->operands[i]);
                vector<RiscvInstruction*> rinsts;
                if(const_int) { // 整型常量
                    auto reg_op = get_reg_op_named("t0"); // First：获取寄存器操作数
                    rinsts.push_back(new RiscvLiInst(reg_op, const_int->val, cur_rbb));
                    phi_pre_vals[inst] = rinsts.back();
                    rinsts.push_back(new RiscvStoreInst(inst->type, reg_op, mem_op, cur_rbb));
                } else { // 其他情况
                    int temp = cur_rbb->rinst_list.size();
                    auto reg_op = reg_alloc->get_reg_op(inst->operands[i], cur_rbb);
                    if(temp + 2 != cur_rbb->rinst_list.size()) {
                        cerr << "[ERROR] [RiscvGenerator::translate_phi_inst] get_reg_op failed to add TWO instructions to cur_bb.\n";
                        exit(1);
                    }
                    for(int t = 2; t >= 1; t--) {
                        // NOTE: 此处因为 rinst_list 改为了 list 结构，利用下标进行随机访问不成功，特别重新创建一个下标访问方法
                        rinsts.push_back(cur_rbb->get_rinst_by_index(cur_rbb->rinst_list.size() - t));
                    }
                    for(int t = 1; t <= 2; t++) {
                        cur_rbb->delete_rinst(cur_rbb->rinst_list.back());
                    }
                    rinsts.push_back(new RiscvStoreInst(inst->operands[i]->type, reg_op, mem_op, cur_rbb));
                }
                // 将指令插入基本块后端
                for(auto new_rinst: rinsts) {
                    cur_rbb->add_rinst_before_terminator(new_rinst);
                }
            } else if(dynamic_cast<ValUndef*>(inst->operands[i])) { // 这种情况直接返回
                continue;
            } else {
                //先正常生成指令
                auto cur_bb = dynamic_cast<BasicBlock*>(inst->operands[i + 1]);
                auto cur_rbb = bb_to_rbb[cur_bb];
                vector<RiscvInstruction*> rinsts;
                if(inst->type->tid == TypeID::IntegerTy) {
                    auto reg_op = get_reg_op_named("t0");
                    rinsts.push_back(new RiscvLoadInst(inst->type, reg_op, reg_alloc->get_mem_op(inst->operands[i], cur_rbb, nullptr, false), cur_rbb));
                    phi_pre_vals[inst] = rinsts.back();
                    rinsts.push_back(new RiscvStoreInst(inst->type, reg_op, mem_op, cur_rbb));
                } else {
                    auto reg_op = get_reg_op_named("fs1");
                    auto src_mem = reg_alloc->get_mem_op(inst->operands[i], cur_rbb, nullptr, false);
                    if(!src_mem) {
                        cerr<< "[ERROR] no mem slot for " << inst->operands[i]->name << endl;
                        continue;
                    }
                    rinsts.push_back(new RiscvLoadInst(inst->type, reg_op, src_mem, cur_rbb));
                    rinsts.push_back(new RiscvStoreInst(inst->type, reg_op, mem_op, cur_rbb));
                }
                //如果操作数是变量 需要判断operand是否是之前的phi 且是公共前驱
                BasicBlock* pre_bb = dynamic_cast<BasicBlock*>(inst->operands[i + 1]);
                //如果operand不是之前的phi 正常处理
                if(handled_phi.find(static_cast<Instruction*>(inst->operands[i])) == handled_phi.end()
                    || phi_pre_bbs[static_cast<Instruction*>(inst->operands[i])].find(pre_bb) == phi_pre_bbs[static_cast<Instruction*>(inst->operands[i])].end()) {
                    for(auto new_rinst: rinsts) {
                        // 将指令插入基本块后端
                        for(auto new_rinst: rinsts) {
                            cur_rbb->add_rinst_before_terminator(new_rinst);
                        }
                    }
                } else { // 要对前驱的phi语句进行调整 不是插在最后了，要插在这个前面
                    for(auto old_rinst: cur_rbb->rinst_list) {
                        if(old_rinst == phi_pre_vals[static_cast<Instruction*>(inst->operands[i])]) {
                            for(auto new_rinst: rinsts){
                                cur_rbb->add_rinst_before_rinst(new_rinst,old_rinst);
                            }
                            break;
                        }
                    }
                }
            }
        }
    }
}

RiscvBasicBlock* RiscvGenerator::transfer_rbb_from_bb(BasicBlock* bb, RiscvFunction* rfunc) {
    RiscvBasicBlock* rbb = get_rbb_from_bb(bb); // 创建新的 RiscvBasicBlock
    bool has_br = false; // 是否发现 br、ret 指令
    for(auto it = bb->inst_list.begin(); it != bb->inst_list.end(); ) {
        Instruction* inst = *it;
        it++;
        switch(inst->iid) {
            case IRInstID::Ret: { // 返回指令：在返回前，需要将寄存器内的数据写回内存，以保存计算出来的数据
                rfunc->reg_alloc->write_back_all_regs(rbb);
                has_br = true;
                // 在翻译过程中先指ret，恢复寄存器等操作在第二遍扫描的时候再插入
                rbb->add_rinst_back(translate_ret_inst(rfunc->reg_alloc, static_cast<ReturnInst*>(inst), rbb, rfunc));
                break;
            }
            // 分支指令
            case IRInstID::Br: {
                rfunc->reg_alloc->write_back_all_regs(rbb);
                has_br = true;
                // vector<RiscvInstruction*> rinsts = translate_br_inst(rfunc->reg_alloc, static_cast<BranchInst*>(inst), rbb);
                // for(auto rinst: rinsts) {
                //     rbb->add_rinst_back(rinst);
                // }
                rbb->add_rinst_back(translate_br_inst(rfunc->reg_alloc, static_cast<BranchInst*>(inst), rbb));
                break;
            }
            case IRInstID::Add:
            case IRInstID::Sub:
            case IRInstID::Mul:
            case IRInstID::Div:
            case IRInstID::Rem:
            case IRInstID::FAdd:
            case IRInstID::FSub:
            case IRInstID::FMul:
            case IRInstID::FDiv:
            case IRInstID::Shl:
            case IRInstID::LShr:
            case IRInstID::AShr: {
                rbb->add_rinst_back(translate_binary_inst(rfunc->reg_alloc, static_cast<BinaryInst*>(inst), rbb));
                break;
            }
            case IRInstID::Phi:
            case IRInstID::ZExt:
            case IRInstID::BitCast:
            case IRInstID::Alloca: {
                break;
            }
            case IRInstID::FpToSi: {
                rbb->add_rinst_back(translate_fptosi_inst(rfunc->reg_alloc, static_cast<UnaryInst*>(inst), rbb));
                break;
            }
            case IRInstID::SiToFp: {
                rbb->add_rinst_back(translate_sitofp_inst(rfunc->reg_alloc, static_cast<UnaryInst*>(inst), rbb));
                break;
            }
            case IRInstID::GetElementPtr: {
                translate_gep_inst(rfunc->reg_alloc, static_cast<GetElementPtrInst*>(inst), rbb);
                break;
            }
            case IRInstID::Load: {
                auto rinsts = translate_load_inst(rfunc->reg_alloc, static_cast<LoadInst*>(inst), rbb);
                for(auto rinst: rinsts)
                    rbb->add_rinst_back(rinst);
                break;
            }
            case IRInstID::Store: {
                auto rinsts = translate_store_inst(rfunc->reg_alloc, static_cast<StoreInst*>(inst), rbb);
                for(auto rinst: rinsts) {
                    rbb->add_rinst_back(rinst);
                }
                break;
            }
            case IRInstID::ICmp: { // 整型比较指令
                auto icmp_inst = static_cast<ICmpInst*>(inst);
                // auto next_inst = *it;
                // if(icmp_inst->use_list.size() == 1 && next_inst == icmp_inst->use_list.front().val && next_inst->is_br()) {
                //     it++;
                //     has_br = true;
                //     translate_icmp_inst(rfunc->reg_alloc, icmp_inst, static_cast<BranchInst*>(next_inst), rbb);
                // } else {
                    translate_icmp_set_inst(rfunc->reg_alloc, icmp_inst, rbb);
                // }
                break;
            }
            case IRInstID::FCmp: { // 浮点比较指令
                translate_fcmp_inst(rfunc->reg_alloc, static_cast<FCmpInst*>(inst), rbb);
                break;
            }
            case IRInstID::Call: {
                CallInst* call_inst = static_cast<CallInst*>(inst);
                RiscvFunction* callee_rfunc = get_rfunc_from_func(static_cast<Function*>(call_inst->operands.back()));
                // 根据函数调用约定，按需传递参数。
                // 需要先保护 a0~a7, fa0~fa7 中的数据
                rfunc->reg_alloc->write_back_reg(get_reg_op_named("a0"), rbb, nullptr);
                rfunc->reg_alloc->write_back_reg(get_reg_op_named("a1"), rbb, nullptr);
                rfunc->reg_alloc->write_back_reg(get_reg_op_named("a2"), rbb, nullptr);
                rfunc->reg_alloc->write_back_reg(get_reg_op_named("a3"), rbb, nullptr);
                rfunc->reg_alloc->write_back_reg(get_reg_op_named("a4"), rbb, nullptr);
                rfunc->reg_alloc->write_back_reg(get_reg_op_named("a5"), rbb, nullptr);
                rfunc->reg_alloc->write_back_reg(get_reg_op_named("a6"), rbb, nullptr);
                rfunc->reg_alloc->write_back_reg(get_reg_op_named("a7"), rbb, nullptr);
                rfunc->reg_alloc->write_back_reg(get_reg_op_named("fa0"), rbb, nullptr);
                rfunc->reg_alloc->write_back_reg(get_reg_op_named("fa1"), rbb, nullptr);
                rfunc->reg_alloc->write_back_reg(get_reg_op_named("fa2"), rbb, nullptr);
                rfunc->reg_alloc->write_back_reg(get_reg_op_named("fa3"), rbb, nullptr);
                rfunc->reg_alloc->write_back_reg(get_reg_op_named("fa4"), rbb, nullptr);
                rfunc->reg_alloc->write_back_reg(get_reg_op_named("fa5"), rbb, nullptr);
                rfunc->reg_alloc->write_back_reg(get_reg_op_named("fa6"), rbb, nullptr);
                rfunc->reg_alloc->write_back_reg(get_reg_op_named("fa7"), rbb, nullptr);
                if(call_inst->operands.size() > 1) {
                    // TODO: 设置 PUSH 和 POP 伪指令，分割逻辑
                    // NOTE: 下面这段代码为所有参数都开辟栈空间，为了防止函数嵌套调用后参数寄存器被覆盖导致没有值的情况，为所有参数都开辟一份栈上空间
                    // TODO: 待函数静态分析后，针对通过寄存器传递的参数，如果存在嵌套调用之后不需要使用本函数参数的情况，可以将其从栈上拿走
                    // 计算存储所有参数需要的额外栈帧大小，并按 16 字节对齐
                    int sp_shift_for_params = (call_inst->operands.size() - 1) * VARIABLE_ALIGN_BYTE; // 每个参数占用 8 字节
                    sp_shift_for_params += 16 - ((abs(rfunc->base) + sp_shift_for_params) & 15);
                    // 为参数申请栈帧
                    rbb->add_rinst_back(new RiscvBinaryInst(RiscvInstID::ADDI, get_reg_op_named("sp"), new RiscvConst(-sp_shift_for_params), get_reg_op_named("sp"), rbb));
                    // 将参数移动至寄存器与内存中
                    int used_int_reg_count = 0, used_float_reg_count = 0;
                    int param_offset = 0;
                    for(int i = 0; i < call_inst->operands.size() - 1; i++) {
                        string reg_alias = "";
                        auto cur_op = call_inst->operands[i];
                        if(cur_op->type->tid == TypeID::IntegerTy || cur_op->type->tid == TypeID::PointerTy) {
                            if(used_int_reg_count < 8) {
                                reg_alias = "a" + to_string(used_int_reg_count);
                            }
                            used_int_reg_count++;
                        } else if(cur_op->type->tid == TypeID::FloatTy) {
                            if(used_float_reg_count < 8) {
                                reg_alias = "fa" + to_string(used_float_reg_count);
                            }
                            used_float_reg_count++;
                        } else {
                            cerr << "[ERROR] Invalid operand type for call instruction: " << cur_op->type->tid << endl;
                            exit(1);
                        }
                        // 将额外的参数直接写入内存中
                        if(reg_alias.empty()) {
                            if(cur_op->type->tid != TypeID::FloatTy) {
                                rbb->add_rinst_back(new RiscvStoreInst(cur_op->type, rfunc->reg_alloc->get_specified_reg_op(cur_op, "t1", rbb), new RiscvIntMem("sp", param_offset), rbb));
                            } else {
                                rbb->add_rinst_back(new RiscvStoreInst(cur_op->type, rfunc->reg_alloc->get_specified_reg_op(cur_op, "ft1", rbb), new RiscvIntMem("sp", param_offset), rbb));
                            }
                        } else {
                            rfunc->reg_alloc->get_specified_reg_op(cur_op, reg_alias, rbb, nullptr);
                        }
                        param_offset += VARIABLE_ALIGN_BYTE; // Add operand size lastly
                    }
                    // 函数调用
                    rbb->add_rinst_back(translate_call_inst(rfunc->reg_alloc, call_inst, rbb));
                    // 释放栈帧
                    rbb->add_rinst_back(new RiscvBinaryInst(RiscvInstID::ADDI, get_reg_op_named("sp"), new RiscvConst(sp_shift_for_params), get_reg_op_named("sp"), rbb));
                } else { // 如果没有参数需要传递，则直接调用函数
                    // NOTE: 函数的序言块会进行栈帧的 16 字节对齐，只要不需要额外的栈空间，则不需要做调整便以对齐
                    rbb->add_rinst_back(translate_call_inst(rfunc->reg_alloc, call_inst, rbb));
                }
                // 处理返回值
                if(call_inst->type->tid != TypeID::VoidTy) {
                    if(call_inst->type->tid != TypeID::FloatTy) {
                        rbb->add_rinst_back(new RiscvStoreInst(new IntegerType(32), get_reg_op_named("a0"), rfunc->reg_alloc->get_mem_op(call_inst), rbb));
                    } else {
                        rbb->add_rinst_back(new RiscvStoreInst(new Type(TypeID::FloatTy), get_reg_op_named("fa0"), rfunc->reg_alloc->get_mem_op(call_inst), rbb));
                    }
                }
                // 调用完成，删除寄存器映射
                rfunc->reg_alloc->clear_reg(get_reg_op_named("a0"));
                rfunc->reg_alloc->clear_reg(get_reg_op_named("a1"));
                rfunc->reg_alloc->clear_reg(get_reg_op_named("a2"));
                rfunc->reg_alloc->clear_reg(get_reg_op_named("a3"));
                rfunc->reg_alloc->clear_reg(get_reg_op_named("a4"));
                rfunc->reg_alloc->clear_reg(get_reg_op_named("a5"));
                rfunc->reg_alloc->clear_reg(get_reg_op_named("a6"));
                rfunc->reg_alloc->clear_reg(get_reg_op_named("a7"));
                rfunc->reg_alloc->clear_reg(get_reg_op_named("fa0"));
                rfunc->reg_alloc->clear_reg(get_reg_op_named("fa1"));
                rfunc->reg_alloc->clear_reg(get_reg_op_named("fa2"));
                rfunc->reg_alloc->clear_reg(get_reg_op_named("fa3"));
                rfunc->reg_alloc->clear_reg(get_reg_op_named("fa4"));
                rfunc->reg_alloc->clear_reg(get_reg_op_named("fa5"));
                rfunc->reg_alloc->clear_reg(get_reg_op_named("fa6"));
                rfunc->reg_alloc->clear_reg(get_reg_op_named("fa7"));
                break;
            }
        }
    }
    if(!has_br) {
        rfunc->reg_alloc->write_back_all_regs(rbb);
    }
    return rbb;
}