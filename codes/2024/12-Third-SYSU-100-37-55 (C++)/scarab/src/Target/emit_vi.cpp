#include "emit_vi.h"
#include "Module.h"
#include <cmath>
#include <algorithm>

using namespace std;

#define is_in(key,set) (((set).find(key)) != ((set).end()))
#define max_dims  40
#define func_arg  4

int vreg_num;
int vfreg_num;

float2int f2i;

static map<ValuePtr,ValuePtr> arr_map;
static map<ValuePtr,int> stack_val_map;
Gep_address_map gep_map;

ValuePtr cur_arr_base;
VOper create_imm(int32 constant)       { auto imm = VOper(IMM, constant); int_val_map[imm] = constant; return imm; }
VOper create_reg(uint8 reg)            { return VOper(REG, reg); }
VOper create_vreg(int32 vreg_index)    { return VOper(VREG, vreg_index); }
VOper create_freg(uint8 freg)          { return VOper(FREG, freg); }
VOper create_vfreg(int32 vfreg_index)  { return VOper(VFREG, vfreg_index); }
VOper create_voper(ValuePtr v, MachineBlock *mb, bool no_imm);

VOper create_legal_imm(int32 constant, MachineBlock *mb) {
    if(!can_be_imm12(constant)) {
        auto vreg = create_vreg(vreg_num++);
        auto ldr = generate_li(vreg, constant, mb);
        return vreg;
    }
    else {
        return create_imm(constant);
    }
}

// yyy:函数参数4位后入栈
void deal_with_more_func_arg(ValuePtr a, VOper vreg, MachineBlock *mb, int cum_offset, int arg_size, bool is_float) {
    auto ldr = new VI_MemOp;
    ldr->tag = is_float?VI_VLOAD:VI_LOAD;
    ldr->mem_tag = MEM_LOAD_ARG;
    ldr->reg = vreg;
    ldr->base = create_reg(sp);  
    ldr->offset = create_imm(cum_offset);
    ldr->is_dw = arg_size == 8;
    push_end(ldr, mb);
}

// yyy: 溢出的参数会入栈
VOper get_stack_val(ValuePtr v, MachineBlock* mb,Func_Asm *func_asm)
{
    int val=0;
    val=func_asm->stack_size-stack_val_map[v];
    auto nreg=create_vreg(vreg_num++);
    auto bi_I = new VI_Binary(BINARY_ADD,nreg,create_reg(sp),create_legal_imm(val, mb));
    bi_I->is_dw = true;
	push(bi_I, mb);
    return bi_I->dst;
}

void insert(VI *vi, VI *before) {
    if (before->prev == NULL) {
        before->mb->inst = vi;
    } else {
        before->prev->next = vi;
    }
    vi->mb = before->mb;
    vi->prev = before->prev;
    vi->next = before;
    before->prev = vi;
}

template<typename T>
void push_end(T* vi, MachineBlock* mb){
    if (mb->control_transfer_inst) {
        insert((VI*)vi, mb->control_transfer_inst);
    } else {
        push((VI*)vi, mb);
    }
}

void emit_load_of_global_ref(Func_Asm *func_asm, ValuePtr v, VOper vreg,  MachineBlock *mb) {
    const char *addr = new char[1024];
    // yyy: 获取addr的名字，即位置
	addr = v->name.c_str();
    VOper adr_global(ADR_GLOBAL, addr);
    auto ldr = new VI_Load;
	ldr->reg = vreg;
	ldr->base = adr_global;
	ldr->mem_tag = MEM_LOAD_GLOBAL_REF;
    val_vopr_map[v] = vreg;
	push(ldr, mb);
}

void push(VI *VI, MachineBlock *mb) {
    VI->mb = mb;
    if (mb->last_inst) {
        mb->last_inst->next = VI;
        VI->prev = mb->last_inst;
    } else {
        mb->inst = VI;
    }
    mb->last_inst = VI;
}

void generate_icmp_vi(IcmpInstruction *icmp, MachineBlock *mb) {
    Branch_Condition cmp_type = string_to_branchCondition[icmp->op];
    VOper lhs,rhs;
    if(cmp_type == GREATER_THAN || cmp_type == LESS_THAN_OR_EQUAL){
        lhs = create_voper(icmp->a, mb, false);
        rhs = create_voper(icmp->b, mb, true);
    }else{
        lhs = create_voper(icmp->a, mb, true);
        rhs = create_voper(icmp->b, mb, false);
    }
        
    auto dst = create_voper(icmp->reg, mb, true);
    VOper tmp = create_vreg(vreg_num++);
    assert(cmp_type >= LESS_THAN && cmp_type <= EQUAL);

    switch (cmp_type)
    {
        case EQUAL:{
            auto vi_xor = new VI_Binary(BINARY_XOR, tmp, lhs, rhs);
            auto seqz = new VI_Seqz(dst, tmp);
            push((VI*)vi_xor, mb);
            push((VI*)seqz,mb);
        } break;
        
        case NOT_EQUAL:{
            auto vi_xor = new VI_Binary(BINARY_XOR, tmp, lhs, rhs);
            auto snez = new VI_Snez(dst, tmp);
            push((VI*)vi_xor, mb);
            push((VI*)snez,mb);
        } break;

        case LESS_THAN:{
            auto slt = new VI_Slt(dst, lhs, rhs);
            push((VI*)slt,mb);
        } break;

        case GREATER_THAN:{
            auto slt = new VI_Slt(dst, rhs, lhs);
            push((VI*)slt,mb);
        } break;

        case LESS_THAN_OR_EQUAL:{
            auto slt = new VI_Slt(tmp, rhs, lhs);
            push((VI*)slt,mb);
            auto vi_xor = new VI_Binary(BINARY_XOR, dst, tmp, create_imm(1));
            push((VI*)vi_xor,mb);
        } break;

        case GREATER_THAN_OR_EQUAL:{
            auto slt = new VI_Slt(tmp, lhs, rhs);
            push((VI*)slt,mb);
            auto vi_xor = new VI_Binary(BINARY_XOR, dst, tmp, create_imm(1));
            push((VI*)vi_xor,mb);
        } break;

        default:{
            assert(false);
        } break;
    }
}

void emit_branch_asm(Func_Asm *func_asm, Branch_Condition cond, BrInstruction* br_I, 
    MachineBlock *mb, bool alwaysTrue, VOper lhs=create_imm(0), VOper rhs=create_imm(0), bool is_float=false) {
    if(is_float) {
        auto vcmp_dst = create_vreg(vreg_num++);
        auto fcmp = new VI_Fcmp_Set(cond, vcmp_dst, lhs, rhs);
        push(fcmp, mb);

        // if ((lhs cond rhs) != 0), jump to true
        // if ((lhs cond rhs) == 0), jump to false
        lhs = vcmp_dst;
        rhs = create_reg(zero);
        cond = alwaysTrue?NOT_EQUAL:EQUAL;
    }
    auto br = new VI_Branch();
    br->cond = cond;
    if (cond != NO_CONDITION) {
        br->lhs = lhs;
        br->rhs = rhs;
    }
    auto lab=br_I->label2;

    int idx1=0;
    int idx2=0;
    for(int i=0;i<func_asm->mbs.size();i++)
    {
        if(func_asm->mbs[i]->label->name==br_I->label1->name)
        {
            idx1=i;
        }
        if(lab!=NULL&&func_asm->mbs[i]->label->name==lab->name)
        {
            idx2=i;
        }
    }
    br->true_target = alwaysTrue?func_asm->mbs[idx1]:func_asm->mbs[idx2];
    br->false_target = alwaysTrue?func_asm->mbs[idx2]:func_asm->mbs[idx1];

    push((VI *)br, mb);
    mb->control_transfer_inst = (VI *)br;
}

VI_Move *generate_move(VOper dst, VOper src, MachineBlock *mb = NULL) {
    auto mv = new VI_Move(dst, src);
    if(int_val_map.count(src) && src.tag != REG)
        int_val_map[dst]=int_val_map[src];
    if (mb) {
        push_end(mv, mb);
    }
    return mv;
}

VI_FMove *generate_fmove(VOper dst, VOper src, MachineBlock *mb = NULL, bool from_f32 = false) {
    if (src.tag == IMM) {
        auto new_src = create_vreg(vreg_num++);
        auto load_const = generate_li(new_src, src.value, mb);
        src = new_src;
    }
    auto mv = new VI_FMove(dst, src, false);
    if (!is_float_moperand(src)) {
        mv->from_s32 = true;
    }else{
        mv->from_f32 = from_f32;
    } 
    if (mb) {
        push_end(mv, mb);
    }
    return mv;
}


VI_Load *generate_li(VOper vreg, int32 constant, MachineBlock *mb, VI *I) {
    auto ldr = new VI_Load;
    ldr->mem_tag = MEM_LOAD_FROM_LITERAL_POOL;
    ldr->reg = vreg;
    ldr->base = create_imm(constant);
    int_val_map[vreg] = constant;
    if (mb) {
        if (I)
            insert(ldr, I);
        else
            push_end(ldr,mb);
    }
    return ldr;
}

// 获得操作数的api, 将你所用到的MB换为可操作的对象,在map中添加记录，必要时还会添加move指令，返回vreg或者imm
VOper create_voper(ValuePtr v, MachineBlock *mb, bool no_imm=false) {
    int tp=-1;
    if(v->isConst)tp=0;
    else if(v->isReg);
    else if(v->isVoid);
    else if(v->type->isPtr())tp=1;
    else if(v->type->isInt())tp=2;
    else if(v->type->isArr())tp=3;

    auto opr = val_vopr_map.find(v);
    if (opr != val_vopr_map.end()) {
        return opr->second;
    }

    switch(tp) {
        // const
        case 0: {
            int32 constant = dynamic_cast<Const*>(v.get())->intVal;
            if (v->type->isFloat()) {
                f2i.f_value = dynamic_cast<Const*>(v.get())->floatVal;
                constant = f2i.bin_value;
                no_imm = true;
            }
            if (no_imm) {
                auto vreg = create_vreg(vreg_num++);
                int_val_map[vreg] = constant;
                VI_Load *ldr = generate_li(vreg, constant, mb);
                if(v->type->isFloat()) {
                    auto vfreg = create_vfreg(vfreg_num++);
                    vreg = generate_fmove(vfreg, vreg, mb)->dst;
                }
                return vreg;
            } else {
                return create_legal_imm(constant, mb);
            }
        } break;
        // ptr
        case 1: {
            // if (no_imm) {
                auto vreg = create_vreg(vreg_num++);
                val_vopr_map[v] = vreg;
                return vreg;
            // } else {
            //     assert(false && "can not make a ptr");
            //     return create_imm(0);
            // }
        } break;
        // int
        case 2: {
            int32 constant = dynamic_cast<Int*>(v.get())->intVal;
            auto imm = create_imm(constant);
            if (no_imm) {
                auto vreg = create_vreg(vreg_num++);
                VI_Load *ldr = generate_li(vreg, constant, mb);
                int_val_map[vreg] = constant;
                return vreg;
            } else {
                return imm;
            }
        } break;
        // arr
        case 3: {
            if (no_imm) {
                auto vreg = create_vreg(vreg_num++);
                val_vopr_map[v] = vreg;
                return vreg;
            } else {
                assert(false && "can not make an array");
                return create_imm(0);
            }
        } break;

        //isReg  isVoid isBool isFloat
        default: {
            VOper new_vreg;
            new_vreg = v->type->isFloat()?create_vfreg(vfreg_num++):create_vreg(vreg_num++);
            val_vopr_map[v] = new_vreg;
            return new_vreg;
        }
    }
}

Func_Asm* emit_func_vi(FunctionPtr func, int idx, vector<VariablePtr> &globalValues, int& labelNum)
{
    auto func_asm = initialize_func_vi(func,idx,globalValues);

    // L-Shield: 前面这一部分只是将llvmIR的基本块的映射关系打到VI上，同样以多个基本块的结构
    for (int i = 0; i < func_arg && i < func->formalArguments.size(); i++) {
        auto arg = func->formalArguments[i];
        if(arg->type->isFloat())
        {
            auto vfreg= create_vfreg(vfreg_num++);
            val_vopr_map[arg]=vfreg;
            generate_fmove(vfreg, create_freg(i + fa0), func_asm->mbs[0]);
        }else{
            auto vreg = create_vreg(vreg_num++);
            val_vopr_map[arg] = vreg;
            generate_move(vreg, create_reg(i + a0), func_asm->mbs[0]);
        }
    }
    int cum_offset = 0;
    for (int i = func_arg; i < func->formalArguments.size(); i++) {
        auto arg = func->formalArguments[i];
        int arg_size;
        bool is_float = arg->type->isFloat();
        auto a = is_float ? create_vfreg(vfreg_num++) : create_vreg(vreg_num++);

        arg_size = (arg->type->ID >= 5)?8:4;
        deal_with_more_func_arg(arg, a, func_asm->mbs.front(), cum_offset, arg_size, is_float);
        cum_offset += arg_size;
        val_vopr_map[arg] = a;
    }

    int bbidx=0;
    for(auto bb : func->basicBlocks) {
        gep_map.clear();
        vopr_offset_map.clear();
        MachineBlock *mb = func_asm->mbs[bbidx];
        bbidx++;
        for(int i = 0; i < bb->instructions.size(); i++) {
            auto I = bb->instructions[i];
            // yyy: 将instruction（llvm)变成 binary_ir，中间用make_operand处理map表，插入bb
            switch(I->type) {
                case Br: {
                    auto br_I = dynamic_cast<BrInstruction*>(I.get());
                    auto expr=br_I->exp;
                    auto cond=NO_CONDITION;
                    if(expr!=NULL)
                    {
                        if(expr->toString()=="true"||expr->toString()=="false"){
                            emit_branch_asm(func_asm, cond, br_I, mb, expr->toString()=="true");
                        }else{
                            emit_branch_asm(func_asm, EQUAL, br_I, mb, false, create_voper(expr, mb, true), create_reg(zero));
                        }
                    }
                    else 
                    {
                        emit_branch_asm(func_asm, cond, br_I, mb, true);
                    }
                    break;
                }

                case Fneg:{
                    auto bi_I = dynamic_cast<FnegInstruction *>(I.get());
                    auto op_type = F_NEG;
                    if(bi_I->a->type->isFloat())
                    {
                        auto lhs = create_voper(bi_I->a, mb, true);
                        auto bii=new VI_VBinary(op_type, create_voper(bi_I->reg, mb, true), lhs, VOper());
                        bii->fneg = 1;
                        push((VI *)bii, mb);
                    }
                    break;
                }

                case FuncCast:{
                    auto funcCast = dynamic_cast<FuncCastInstruction *>(I.get());
                    assert(i + 1 < bb->instructions.size());
                    auto next_inst = bb->instructions[i + 1];
                    auto call = dynamic_cast<CallInstruction *>(next_inst.get());
                    auto funcName = call->func->name.c_str();
                    // assert(funcName=="scarabsParallelFor");
                    
                    auto fcast = new VI_Fcast(labelNum++, funcCast->funcName);
                    push(fcast, mb);
                } break;

                case Binary: {
                    auto bi_I = dynamic_cast<BinaryInstruction *>(I.get());
                    string myop;
                    myop+=bi_I->op;
                    auto op_type = Binary_ir2asm[myop];
                    if(op_type==UNARY_XOR)
                    {
                        assert(!bi_I->a->isConst);
                        auto bi_xor = new VI_Binary;
                        bi_xor->op = BINARY_XOR;
                        auto imm = create_imm(1);
                        bi_xor->lhs = create_voper(bi_I->a, mb, true);
                        bi_xor->rhs = imm;
                        bi_xor->dst = create_voper(bi_I->reg, mb);
                        push((VI *)bi_xor, mb);
                        if(int_val_map.count(bi_xor->lhs)){
                            int_val_map[bi_xor->dst] = int_val_map[bi_xor->lhs] ^ 1;
                        }
                    }
                    else{
                        VOper lhs,rhs,dst;
                        if(bi_I->a->type->isFloat() || bi_I->b->type->isFloat())
                        {
                            lhs = create_voper(bi_I->a, mb, true);
                            rhs = create_voper(bi_I->b, mb, true);
                            assert(is_float_reg(lhs) || is_float_reg(rhs));
                            is_float_reg(lhs)||conver_intReg_to_floatReg(bi_I,lhs,mb);
                            is_float_reg(rhs)||conver_intReg_to_floatReg(bi_I,rhs,mb,false);
                        }else{
                            lhs = create_voper(bi_I->a, mb, true);
                            if(rhs.tag == IMM)rhs = create_legal_imm(rhs.value,mb);
                            else rhs = create_voper(bi_I->b, mb, (bi_I->op == '*' || bi_I->op == '/' || bi_I->op == '%'));
                        }
                        dst = create_voper(bi_I->reg, mb, true);
                        auto bi = new VI_VBinary(op_type, dst, lhs, rhs);
                        if(!bi_I->a->type->isFloat() && !bi_I->b->type->isFloat()){
                            bi->is_dw = bi_I->isrv64;
                            bi->tag = VI_BINARY;
                            put_vi_binary_into_offset(dst,lhs,rhs,bi_I->op);
                        }
                        push(bi, mb);
                    }
                    break;
                }

                case AtomicAdd:{
                    auto atom_I = dynamic_cast<AtomicAddInstruction*>(I.get());
                    bool tag = true;
                    VOper nvreg;
                    for(auto v: globalValues){
                        if(v==atom_I->desVar){
                            nvreg = create_vreg(vreg_num++);
                            emit_load_of_global_ref(func_asm, atom_I->desVar, nvreg, mb);
                            tag = false;
                            break;
                        }
                    }
                    assert(!tag);
                    auto vi_amadd = new VI_Binary(ATOMIC_ADD, create_reg(zero), create_voper(atom_I->addValue, mb, true), nvreg);
                    push(vi_amadd, mb);
                    break;
                }

                case Select:{
                    auto sel_I = dynamic_cast<SelectInstruction*>(I.get());
                    auto expr = sel_I->exp;
                    auto expr_opr = create_voper(expr,mb,true);
                    auto sel_return = create_voper(sel_I->reg, mb);
                    bool isfloat = sel_I->reg->type->isFloat();
                    if(expr->toString()=="true"||expr->toString()=="false"){
                        auto result = expr->toString()=="true"?sel_I->val1:sel_I->val2;
                        auto val_opr = create_voper(result,mb);
                        auto mv = generate_move(sel_return, val_opr);
                        if(isfloat)mv->tag = VI_FMOVE;
                        push((VI*)mv, mb);
                    }else{
                        auto val_opr1 = create_voper(sel_I->val1,mb);
                        auto val_opr2 = create_voper(sel_I->val2,mb);
                        if(isfloat){
                            auto tmp1 = create_vreg(vreg_num++);
                            auto tmp2 = create_vreg(vreg_num++);
                            auto vmv1 = new VI_FMove(tmp1,val_opr1,false,true);
                            auto vmv2 = new VI_FMove(tmp2,val_opr2,false,true);
                            push((VI*)vmv1,mb);
                            push((VI*)vmv2,mb);
                            val_opr1 = tmp1;
                            val_opr2 = tmp2;
                        }
                        auto mask1 = create_vreg(vreg_num++);
                        auto VI_add = new VI_Binary(BINARY_SUBTRACT,mask1,expr_opr,create_imm(1));
                        push((VI*)VI_add, mb);
                        vopr_offset_map[make_pair(mask1,expr_opr)] = -1;
                        auto mask2 = create_vreg(vreg_num++);
                        auto VI_xor = new VI_Binary(BINARY_XOR,mask2,mask1,create_legal_imm(-1,mb));
                        push((VI*)VI_xor, mb);
                        auto mask_value1 = create_vreg(vreg_num++);
                        auto mask_value2 = create_vreg(vreg_num++);
                        auto VI_and1 = new VI_Binary(BINARY_BITWISE_AND,mask_value1,mask2,val_opr1);
                        auto VI_and2 = new VI_Binary(BINARY_BITWISE_AND,mask_value2,mask1,val_opr2);
                        push((VI*)VI_and1,mb);
                        push((VI*)VI_and2,mb);
                        if(isfloat){
                            auto tmp = create_vreg(vreg_num++);
                            auto VI_or = new VI_Binary(BINARY_BITWISE_OR,tmp,mask_value1,mask_value2);
                            push((VI*)VI_or,mb);
                            auto vmv = new VI_FMove(sel_return,tmp,true);
                            push((VI*)vmv,mb);
                        }else{
                            auto VI_or = new VI_Binary(BINARY_BITWISE_OR,sel_return,mask_value1,mask_value2);
                            push((VI*)VI_or,mb);
                        }
                    }
                }break;
                
				case Load: {
                    bool isFloat=false;
                    auto Read_I = dynamic_cast<LoadInstruction*>(I.get());
                    auto type = Read_I->from->type.get();
                    while(type->isPtr()) {
                        type = dynamic_cast<PtrType *>(type)->inner.get();
                    }
                    isFloat = type->isFloat();
                    loadOrStoreTranslator(Read_I->from, Read_I->to, mb, globalValues, func_asm, isFloat, true);
				} break;

				case Store: {
                    bool isFloat=false;
                    auto Write_I = dynamic_cast<StoreInstruction*>(I.get());
                    isFloat = Write_I->value->type->isFloat();
                    loadOrStoreTranslator(Write_I->des, Write_I->value, mb, globalValues, func_asm, isFloat, false);

				} break;

                case Icmp: {
                    assert(i + 1 < bb->instructions.size());
                    auto next_inst = bb->instructions[i + 1];
                    if(next_inst->type == Br) {
                        i++;
                        auto icmp_I = dynamic_cast<IcmpInstruction*>(I.get());
                        auto lhs = create_voper(icmp_I->a, mb, true);
                        auto rhs = create_voper(icmp_I->b, mb, true);
                        auto br_I = (BrInstruction *)(next_inst.get());
                        emit_branch_asm(func_asm, string_to_branchCondition[icmp_I->op], br_I, mb, true, lhs, rhs);
                    }
                    else {
                        generate_icmp_vi(dynamic_cast<IcmpInstruction*>(I.get()), mb);
                    }
                    break;
                }

                case Fcmp: {
                    auto next_inst = bb->instructions[i + 1];
                    auto fcmp_I = dynamic_cast<FcmpInstruction *>(I.get());
                    auto lhs = create_voper(fcmp_I->a, mb, true);
                    auto rhs = create_voper(fcmp_I->b, mb, true);
                    assert(is_float_reg(lhs) || is_float_reg(rhs));
                    is_float_reg(lhs)||conver_intReg_to_floatReg(fcmp_I,lhs,mb);
                    is_float_reg(rhs)||conver_intReg_to_floatReg(fcmp_I,rhs,mb,false);

                    if(next_inst->type == Br){
                        i++;
                        auto br_I = (BrInstruction *)(next_inst.get());
                        emit_branch_asm(func_asm, string_to_branchCondition[fcmp_I->op], br_I, mb, true , lhs, rhs, true);
                    }else{
                        auto result_vreg = create_voper(fcmp_I->reg, mb, true);
                        auto cmp = new VI_Fcmp_Set(string_to_branchCondition[fcmp_I->op], result_vreg, lhs, rhs);
                        val_vopr_map[fcmp_I->reg] = result_vreg;
                        push(cmp, mb);
                    }
                    break;
                }

                case Sitofp:
                {   
                    auto conv_I = dynamic_cast<SignToFloatInstruction*>(I.get());
                    auto src = create_voper(conv_I->from, mb, true);
                    auto dst = create_voper(conv_I->reg, mb, true);
                    auto vi = new VI_VCvt(S32,F32);
                    vi->dst = dst;
                    vi->src = src;
                    push(vi,mb);
                }break;

                case Fptosi:
                {   
                    auto conv_I = dynamic_cast<FloatToSignInstruction*>(I.get());
                    auto src = create_voper(conv_I->from, mb, true);
                    auto dst = create_voper(conv_I->reg, mb, true);
                    auto vi = new VI_VCvt(F32,S32);
                    vi->dst = dst;
                    vi->src = src;
                    push(vi,mb);
                }break;

                // yyy: not implement loopgep
                case GEP:
                {
                    auto ptr_I = dynamic_cast<GetElementPtrInstruction *>(I.get());
                    
                    if (arr_map.find(ptr_I->from) != arr_map.end()){
                        arr_map[ptr_I->reg] = arr_map[ptr_I->from];
                    }else{
                        arr_map[ptr_I->reg] = ptr_I->from;
                    }
                    val_vopr_map[ptr_I->reg] = create_vreg(vreg_num++);
                    // yyy: 不是由之前的gep计算出来的arrayidx那么就是全局变量（或者局部alloc)的
                    if (ptr_I->from->name.find("arrayidx") == ptr_I->from->name.npos)
                    {
                        VOper nvreg;
                        // yyy: 但是局部alloca的不会影响
                        for (auto x : globalValues)
                        {
                            if (ptr_I->from == x)
                            {
                                nvreg = create_vreg(vreg_num++);
                                emit_load_of_global_ref(func_asm, ptr_I->from, nvreg, mb);
                                break;
                            }
                        }
                    }

                    combineGep(ptr_I, mb);
                    // deal_with_gep(ptr_I, mb);
                } break;

                // yyy: sign extension
                case Ext:
                {
                    auto ext_I=dynamic_cast<ExtInstruction*>(I.get());
                    auto oreg=create_voper(ext_I->from,mb,true);
                    val_vopr_map[ext_I->reg]=oreg;

                }break;
                
				case Alloca: {
					auto alloc = dynamic_cast<AllocaInstruction*>(I.get());
                    auto tmp=dynamic_cast<Variable*>(alloc->des.get());
                    int tp=tmp->type->getID();
                    auto const_value=0;
                    switch(tp)
                    {
                        case PtrID:
                        case IntID:
                        {
                            func_asm->stack_size += 4;
                        }break;

                        case ArrID:
                        {
                            auto mytype = dynamic_cast<ArrType *>(tmp->type.get());
                            auto ptr = mytype->inner;
                            const_value = mytype->getSize() * 4;
                            func_asm->stack_size += const_value;
                        }break;

                        default: {
                            printf("Alloca initial type: %d\n", tp);
                            assert(false && "unknown initial value type");
                        }
                    }
                    if(tp == IntID)
                        break;

                    // 16B栈对齐
                    if (func_asm->stack_size % 16 != 0) 
                    {
                        func_asm->stack_size += 16 - (func_asm->stack_size % 16);
                    }
					auto bi_I = new VI_Binary(BINARY_SUBTRACT,create_voper(alloc->des, mb, true),create_reg(sp),create_imm(func_asm->stack_size));
                    bi_I->is_dw = true;
                    func_asm->local_array_bases.push_back(bi_I);
					push((VI *)bi_I, mb);
				}	break;

                case Call: {
                    func_asm->call_function_num++;
                    auto func_I = dynamic_cast<CallInstruction*>(I.get());

                    // yyy: deal with call llvm.memset.p0i8.i64
                    if(deal_with_memset(func_I,mb))break;

                    auto call = new VI_Func_Call(func_I->func->name.c_str());
                    call->arg_count = func_I->func->formalArguments.size();
                    int offset = 0;
                    for(int i = 0; i < func_I->func->formalArguments.size(); i++) {
                        auto arg_value = func_I->argv[i];
                        VOper arg_operand;
                        if (i < func_arg) {
                            if(stack_val_map.find(arg_value)!=stack_val_map.end())
                            {
                                auto ldr = new VI_Load;
                                ldr->reg = create_reg(i + a0);
                                ldr->base = get_stack_val(arg_value,mb,func_asm);
                                push(ldr, mb);
                            }
                            else
                            {
                                arg_operand = create_reg(i + a0);
                                bool is_float = arg_value->type->isFloat();
                                auto arg_opr = create_voper(arg_value, mb, is_float);
                                if(array_ptr_map.find(arg_value) != array_ptr_map.end())
                                {
                                    arg_opr = array_ptr_map[arg_value];
                                    if(is_float){
                                        generate_move(arg_operand, arg_opr, mb);
                                    } 
                                }
                                else if(arr_map.find(arg_value)!=arr_map.end())
                                {
                                    // yyy: 常量offset
                                    auto opr = ptr_val_map.find(arg_value);
                                    auto offset_opr = create_legal_imm(opr->second * 4,mb);
                                    arg_opr = cal_new_gep_base(create_voper(arg_value, mb), offset_opr, mb);

                                    // yyy: array_ptr_map在此处被计算
                                    array_ptr_map[arg_value] = arg_opr;
                                    generate_move(arg_operand, arg_opr, mb);
                                }else if(is_float){ // yyy: 直接就是float数
                                    arg_operand = create_freg(i + fa0);
                                    generate_fmove(arg_operand, arg_opr, mb);
                                }

                                if(is_float)continue;
                                generate_move(arg_operand, arg_opr, mb);
                            }
                            
                        } else {
                            if(ptr_val_map.find(arg_value)!=ptr_val_map.end())
                            {
                                // L-Shield：某个指针可以转换成数组base+偏移量offset
                                cur_arr_base = arr_map[arg_value];
                                auto opr = ptr_val_map.find(arg_value);
                                VOper new_base = cal_new_gep_base(create_voper(cur_arr_base, mb),create_legal_imm(opr->second*4, mb),mb);
                                auto str = new VI_Store;
                                if(!can_be_imm12(offset)) {
                                    VOper offset_opr = create_vreg(vreg_num++);
                                    generate_li(offset_opr, offset, mb);
                                    str->base = cal_new_gep_base(create_reg(sp), offset_opr, mb);
                                    str->offset = create_imm(0);
                                }
                                else {
                                    str->base = create_reg(sp);
                                    str->offset = create_imm(offset);
                                }
                                str->mem_tag = MEM_PREP_ARG;
                                str->reg = new_base;
                                str->is_dw = true;
                                push((VI *)str, mb);
                                offset += 8;
                            }
                            else
                            {
                                auto str = new VI_Store;
                                if(arg_value->type->isFloat())str->tag=VI_VSTORE;
                                if(!can_be_imm12(offset)) {
                                    VOper offset_opr = create_vreg(vreg_num++);
                                    generate_li(offset_opr, offset, mb);
                                    str->base = cal_new_gep_base(create_reg(sp), offset_opr, mb);
                                    str->offset = create_imm(0);
                                }
                                else {
                                    str->base = create_reg(sp);
                                    str->offset = create_imm(offset);
                                }
                                str->mem_tag = MEM_PREP_ARG;
                                str->reg = create_voper(arg_value, mb, true);
                                // yyy: ArrID、PtrID、Int64ID
                                if (arg_value->type->ID >= 5) {
                                    str->is_dw = true;
                                    offset += 8;
                                }
                                else offset += 4;
                                push((VI *)str, mb);
                            }
                        }
                    }

                    call->arg_stack_size = offset;
                    push((VI *)call, mb);

                    ValuePtr ret = func_I->func->returnValue;
                    if (!ret->isVoid) {
                        auto result = create_voper(func_I->reg, mb,true);
                        if(ret->type->isFloat()){
                            generate_fmove(result, create_freg(fa0), mb);
                        }else{
                            generate_move(result, create_reg(a0), mb);
                        }
                    }
                } break;

                case Return: {
                    auto ret_I = dynamic_cast<ReturnInstruction*>(I.get());
                    auto ret = new VI_Return;
                    auto retv=ret_I->retValue;
                    auto myrev=0;
                    if(retv->type->isFloat())
                    {
                        auto ret_value = create_voper(ret_I->retValue, mb);
                        val_vopr_map[ret_I->retValue] = ret_value;
                        generate_fmove(create_freg(fa0), ret_value, mb);

                    }else if(retv->isConst) {
                        myrev = dynamic_cast<Const*>(retv.get())->intVal;
                        generate_move(create_reg(a0), create_legal_imm(myrev, mb), mb);
                    }else if(!retv->isVoid) {
                        auto ret_value = create_voper(retv, mb);
                        generate_move(create_reg(a0), ret_value, mb);
                    }
                    push((VI *)ret, mb);

                    mb->control_transfer_inst = (VI *)ret;
                } break;

                case Bitcast:
                {
                    auto bi_I = dynamic_cast<BitCastInstruction *>(I.get());
                    auto oreg = create_voper(bi_I->from, mb);
                    val_vopr_map[bi_I->reg] = oreg;
                } break;
                
                case Phi:
                    // yyy: phi 最后处理
                    break;
                
                default: {
                    assert(false && "translating unknown IR instructions to assembly");
                }break;
       
            }
        }
        mb->vopr_offset_map = vopr_offset_map;
    }

    // phi
    int ii=0;
    for(auto bb : func->basicBlocks) {
        auto mb = func_asm->mbs[ii];
        ii++;
        for(int i = 0; i < bb->instructions.size(); i++) {
            if (bb->instructions[i]->type == Phi) {
                auto phi = dynamic_cast<PhiInstruction *>(bb->instructions[i].get());
                bool isFloat = phi->reg->type->isFloat();
                auto incoming = isFloat?create_vfreg(vfreg_num++):create_vreg(vreg_num++);

                auto phi_as_operand = create_voper(phi->reg, mb);
                if(isFloat){
                    auto mv = generate_fmove(phi_as_operand, incoming);
                    insert(mv, mb->inst);
                }else{
                    auto mv = generate_move(phi_as_operand, incoming);
                    insert(mv, mb->inst);
                }
                int_val_map.erase(phi_as_operand);

                for(int p = 0; p < phi->from.size(); p++) {
                    auto phi_opr = phi->from[p];
                    auto pred_bb = phi_opr.second;
                    int idx=0;
                    for(auto bb: func->basicBlocks)
                    {
                        if(bb==pred_bb)
                            break;
                        idx++;
                    }
                    auto pred_mb = func_asm->mbs[idx];

                    // float -> int
                    if (!isFloat && phi_opr.first->type->isFloat()) {
                        auto mv = new VI_VCvt(F32, S32);
                        mv->src = create_voper(phi_opr.first, pred_mb);
                        mv->dst = incoming;
                        push_end(mv, pred_mb);
                    }
                    // int -> int
                    else {
                        if(isFloat){
                            auto mv = generate_fmove(incoming, create_voper(phi_opr.first, pred_mb), pred_mb);
                        }else{
                            auto mv = generate_move(incoming, create_voper(phi_opr.first, pred_mb), pred_mb);
                        }
                    }
                }
            }
        }
    }
    map<VOper, int> new_int_val_map;
    for(auto p: int_val_map) {
        if(p.first.tag != REG && p.first.tag != FREG)
            new_int_val_map.emplace(p.first, p.second);
    }
    new_int_val_map[create_reg(0)]=0;
    // L-Shield：两种vreg，是要找到需要分配寄存器的那些value
    func_asm->vreg_num = vreg_num;
    func_asm->vfreg_num = vfreg_num;
    func_asm->int_val_map = new_int_val_map;
    return func_asm;
}

// yyy: 条件取非
Branch_Condition invert_branch_cond(Branch_Condition cond) {
    if (cond == NO_CONDITION) return cond;
    if (cond >= 1 && cond <= 3) {
        return (Branch_Condition)(cond + 3);
    } else if (cond >= 4 && cond <= 6) {
        return (Branch_Condition)(cond - 3);
    }
    assert(false);
    return NO_CONDITION;
}

template<typename T>
bool conver_intReg_to_floatReg(T *bi_I, VOper& intReg, MachineBlock *mb, bool left, bool s32tof32){
    auto fcvt = s32tof32?new VI_VCvt(S32, F32):new VI_VCvt(F32,S32);
    auto new_rhs = create_vfreg(vfreg_num++);
    fcvt->src = intReg;
    fcvt->dst = new_rhs;
    push((VI *)fcvt, mb);
    intReg =  new_rhs;
    val_vopr_map[left?bi_I->a:bi_I->b] = new_rhs;
    return true;
}

void replace_inst_operand(VI *I, VOper *old_opr, VOper *new_opr, Func_Asm *func){
    switch (I->tag)
    {
    case VI_MOVE:{
        auto mv = (VI_Move *)I;
        mv->src = *new_opr;
    } break;
    case VI_BINARY:{
        auto bi = (VI_Binary *)I;
        if(bi->rhs==*old_opr)bi->rhs=*new_opr;
        if(bi->rhs==*old_opr)bi->rhs=*new_opr;
    }break;
    case VI_SLT:{
        auto slt = (VI_Slt *)I;
        if(slt->rhs==*old_opr)slt->rhs=*new_opr;
        if(slt->rhs==*old_opr)slt->rhs=*new_opr;
    }break;
    case VI_LOAD:
    case VI_STORE:{
        auto load_or_store = (VI_Load *)I;
        load_or_store->base=*new_opr;
    }break;
    case VI_FMOVE:{
        auto mv = (VI_FMove *)I;
        mv->src = *new_opr;
    }break;
    case VI_VCVT : {
        auto cvt = (VI_VCvt *)I;
        cvt->src=*new_opr;
    }break;
    case VI_VLOAD : 
    case VI_VSTORE : {
        auto load_or_store = (VI_VLoad *)I;
        load_or_store->base = *new_opr;
    }break;
    case VI_VBINARY : {
        auto bi = (VI_VBinary *)I;
        if(bi->rhs==*old_opr)bi->rhs=*new_opr;
        if(bi->rhs==*old_opr)bi->rhs=*new_opr;
    }break;
    case VI_FCMP_SET : {
        auto Vcmp = (VI_Fcmp_Set *)I;
        if(Vcmp->rhs==*old_opr)Vcmp->rhs=*new_opr;
        if(Vcmp->rhs==*old_opr)Vcmp->rhs=*new_opr;
    }break;
    case VI_BRANCH:{
        auto br = (VI_Branch *)I;
        if(br->rhs==*old_opr)br->rhs=*new_opr;
        if(br->rhs==*old_opr)br->rhs=*new_opr;
    }break;
    default:
        cout << "unknow assembly instruction ID: " << I->tag << endl;
            assert(false && "printing unknown assembly instruction.");
        break;
    }
}

// yyy: 执行寄存器count，和machineblock的赋值
Func_Asm* initialize_func_vi(FunctionPtr func, int idx, vector<VariablePtr> &globalValues){
    vreg_num = 0;
    vfreg_num = 0;
    val_vopr_map.clear();
    int_val_map.clear();
    ptr_val_map.clear();
    gep_map.clear();
    vopr_offset_map.clear();
    int_val_map[create_reg(zero)]=0;

    auto func_asm = new Func_Asm;
    func_asm->name = func->name;
    func_asm->index = idx;
    func_asm->stack_size = 0;

    int ii=0;
    for(auto bb : func->basicBlocks){
        func_asm->mbs.push_back(new MachineBlock());
        func_asm->mbs.back()->index = ii;
        func_asm->mbs.back()->label = bb->label;
        ii++;
        func_asm->mbs.back()->loop_depth = bb->loopDepth;
    }

    for(int i=0;i<func->basicBlocks.size();i++){
        auto outerBlock = func->basicBlocks[i];
        for(int j=0;j<func->basicBlocks.size();j++){
            auto innerBlock = func->basicBlocks[j];
            if(is_in(innerBlock,outerBlock->succBasicBlocks)){
                func_asm->mbs[i]->succs.push_back(func_asm->mbs[j]);
            }
            if(is_in(innerBlock,outerBlock->predBasicBlocks)){
                func_asm->mbs[i]->preds.push_back(func_asm->mbs[j]);
            }
        }
    }
    return func_asm;
}

VOper cal_new_gep_base(VOper old_base, VOper old_offset, MachineBlock* mb){
    auto new_base = create_vreg(vreg_num++);
    auto bi_add = new VI_Binary(BINARY_ADD, new_base, old_base, old_offset);
    put_vi_binary_into_offset(new_base,old_base,old_offset,'+');
    bi_add->is_dw = true;
    push((VI*)bi_add, mb);
    return new_base;
}

VOper move_f32_to_s32(ValuePtr v, MachineBlock *mb){
    if(v->isConst){
        f2i.f_value = dynamic_cast<Const*>(v.get())->floatVal;
        int32 constant = f2i.bin_value;
        auto imm = create_imm(constant);
        auto vreg = create_vreg(vreg_num++);
        int_val_map[vreg] = constant;
        VI_Load *ldr = generate_li(vreg, constant, mb);
        return vreg;
    }else{
        auto opr = val_vopr_map.find(v);
        VOper old_vreg;
        if (opr != val_vopr_map.end()) {
            old_vreg = opr->second;
        }
        else  old_vreg = create_vfreg(vfreg_num++);
        val_vopr_map[v] = old_vreg;

        VOper new_vreg = create_vreg(vreg_num++);
        auto bi_vmv = new VI_FMove(new_vreg, old_vreg, false, true);
        return new_vreg;
    }
}

template<typename T>
VOper deal_with_gep(T* ptr_I, MachineBlock* mb){

    /*
        yyy: 在55_sort_test中会遇到局部传入参数的arr，此时就是arrayInitElement，传入局部数组arr，会用arrayidx0作为起始位置,此时reg就是arrayInitElement
        此时如果被内联，就会直接用之前的arrayidx0计算位置，否则，用传入的指针i32* arr做地址计算位置
    */ 
    bool arrayInitElement = (ptr_I->reg->name.find("arrayinit") != ptr_I->reg->name.npos);
    // yyy: arrayidx考虑数组嵌套取，所以两边均有可能是arrayidx
    // bool arrayInitElement = (ptr_I->from->name.find("arrayidx")!=ptr_I->from->name.npos)?false:true;

    TypePtr cur_arr = ptr_I->from->type;
    int offset = 0;
    
    bool ConstOffset = (ptr_val_map.find(ptr_I->from) != ptr_val_map.end());

    // yyy: 不是常量offset
    bool ff = (val_vopr_map.find(ptr_I->from) != val_vopr_map.end() && ptr_I->from->name.find("arrayinit") != ptr_I->from->name.npos && !ConstOffset);
    for (auto x : ptr_I->index)
    {
        if (!x->isConst){
            ff = true;
            break;
        }
    }
    
    VOper mul_regs[max_dims + 1];
    vector<VI *> mul_instr(max_dims + 1, nullptr);

    int cnt = 0;

    // yyy: index 记录valuePtr
    for (int i = arrayInitElement?0:1; i < ptr_I->index.size(); i++)
    {
        cnt++;
        auto x = ptr_I->index[i];
        int const_val = 0;

        if (x->isConst) const_val = dynamic_cast<Const *>(x.get())->intVal;

        int const_size = 1;
        if (i == 0)
        {
            // yyy: arrayInitElement 本体可能是一个传入的数组指针
            if(ptr_I->from->type->ID == PtrID) {
                auto tmp = dynamic_cast<PtrType *>(ptr_I->from->type.get());
                if (tmp->inner->isArr())
                    const_size = dynamic_cast<ArrType *>(tmp->inner.get())->getSize();
            }
            else if(ptr_I->from->type->ID == IntID || ptr_I->from->type->ID == FLoatID) {
                const_size = 1;
            }
            else {
                assert(false && "can not handle array type");
            }
        }
        else if(i>=1){
            auto tmp = dynamic_cast<ArrType *>(cur_arr.get());
            if(tmp->inner->isArr())
                const_size = dynamic_cast<ArrType *>(tmp->inner.get())->getSize();
            cur_arr = tmp->inner;
        }
        if (!ff){
            // yyy: 计算每个index的偏移量，比如a[3][2]那么就是2*第一维的值
            offset += const_size * const_val;
        }
        else
        {
            if (x != ptr_I->index.back() || const_size != 1)
            {
                // 不是global,不是const，作为立即数
                ValuePtr imptr = ValuePtr(new Int("asdf", false, false, const_size));
                assert(cnt > 0 && cnt < max_dims);
                mul_regs[cnt] = create_vreg(vreg_num++);
                VOper lhs =  create_voper(x, mb, true);
                VOper rhs = create_voper(imptr, mb, true);
                auto bi_mul = new VI_Binary(BINARY_MULTIPLY, mul_regs[cnt], lhs, rhs);
                put_vi_binary_into_offset(mul_regs[cnt],lhs,rhs,'*');
                mul_instr[cnt] = bi_mul;
            }
            else
            {
                assert(cnt > 0 && cnt < max_dims);
                mul_regs[cnt] = create_voper(x, mb, true);
            }
        }
    }
    
    if (!ff)
    {
        // L-Shield：所有index都是常数
        ptr_val_map[ptr_I->reg] = offset;
        if(ptr_val_map.count(ptr_I->from) != 0) {
            ptr_val_map[ptr_I->reg] += ptr_val_map[ptr_I->from];
        }
        val_vopr_map[ptr_I->reg] = val_vopr_map[ptr_I->from];
        ptr_I->isConstIdx = true;
        return create_imm(offset);
    }
    else
    {
        VI_Binary *bi_add;
        assert(cnt < max_dims);
        int i;
        auto last_reg = mul_regs[1];
        if(mul_instr[1] != nullptr) {
            push(mul_instr[1], mb);
        }
        for (i = 1; i < cnt; i ++) {
            if(mul_instr[i + 1] != nullptr) {
                push(mul_instr[i + 1], mb);
            }
            VOper dst = create_vreg(vreg_num++);
            bi_add = new VI_Binary(BINARY_ADD, dst, last_reg, mul_regs[i + 1]);
            put_vi_binary_into_offset(dst,last_reg,mul_regs[i + 1],'+');
            bi_add->is_dw = true;
            push((VI *)bi_add, mb);
            last_reg = bi_add->dst;
        }
        // L-Shield：前面这里也是一对一的翻译，就是翻译多维数组的访问（因为在llvmIR里面都集中成一条GEP了）

        auto offset = last_reg;
        auto old_base = create_voper(ptr_I->from, mb);
        auto new_base = create_voper(ptr_I->reg, mb, true);
        if(ptr_val_map.count(ptr_I->from) != 0) {
            auto old_offset = create_legal_imm(ptr_val_map[ptr_I->from] * 4, mb);
            old_base = cal_new_gep_base(old_base, old_offset, mb);
        }
        auto bi_sh2add = new VI_Binary(BINARY_SH2ADD, new_base, offset, old_base);
        if(int_val_map.count(offset)){
            int value = int_val_map[offset];
            vopr_offset_map[make_pair(new_base,old_base)] = value*4;
        }
        push(bi_sh2add, mb);
        return new_base;
    }
}

void Initial_ir_asm_op_map(){
    Binary_ir2asm["+"] = BINARY_ADD;
	Binary_ir2asm["-"] = BINARY_SUBTRACT;
	Binary_ir2asm["*"] = BINARY_MULTIPLY;
	Binary_ir2asm["/"] = BINARY_DIVIDE;
	Binary_ir2asm["%"] = BINARY_MOD;
    Binary_ir2asm["!"] = UNARY_XOR;
    Binary_ir2asm[","] = BINARY_LSL;
    Binary_ir2asm["."] = BINARY_ASR;

    string_to_branchCondition[">"] = GREATER_THAN;
	string_to_branchCondition[">="] = GREATER_THAN_OR_EQUAL;
	string_to_branchCondition["<"] = LESS_THAN;
	string_to_branchCondition["<="] = LESS_THAN_OR_EQUAL;
    string_to_branchCondition["=="] = EQUAL;
    string_to_branchCondition["!="] = NOT_EQUAL;
}

bool deal_with_memset(CallInstruction* func_I, MachineBlock * mb){
    auto func_name=func_I->func->name;
    if(func_name.size() > 15 && func_name.find("memset") != func_name.npos)
    {
        auto call = new VI_Func_Call("memset");
        call->arg_count = func_I->func->formalArguments.size()-1;
        for(int i = 0; i < call->arg_count; i++) {
            auto arg_value = func_I->argv[i];
            generate_move(create_reg(i + a0), create_voper(arg_value, mb), mb);
        }

        push(call, mb);

        ValuePtr ret = func_I->func->returnValue;

        if (!ret->isVoid) {
            auto result = create_voper(func_I->reg, mb,true);
            if (ret->type->isFloat()) {
                generate_fmove(result, create_reg(fa0), mb);
            }
            else {
                generate_move(result, create_reg(a0), mb);
            }
        }
        return true;
    }
    return false;
}

void loadOrStoreTranslator(ValuePtr from, ValuePtr to, MachineBlock* mb, vector<VariablePtr> &globalValues, Func_Asm* func_asm, bool isFloat, bool isLoad){
    auto ldr = new VI_MemOp;
    ldr->base = create_voper(from, mb);
    if(isLoad){
        ldr->tag = VI_LOAD;
        if(isFloat)ldr->tag = VI_VLOAD;
    }else{
        ldr->tag = VI_STORE;
        if(isFloat)ldr->tag = VI_VSTORE;
    }

    // if(from->type->ID == Int64ID || to->type->ID == Int64ID){
    //     ldr->is_dw = true;
    // }

    // yyy: from对应的I会是gep指令
    if(arr_map.find(from)!=arr_map.end())
    {
        assert(from->I && from->I->type == GEP);
        // yyy: 有常量offset的array,对应!ff
        if(ptr_val_map.find(from)!=ptr_val_map.end())
        {
            ldr->reg = create_voper(to, mb, true);
            auto opr = ptr_val_map.find(from);
            int offset = opr->second * 4;
            auto cur_arr_base_opr = create_voper(from, mb);

            if(!can_be_imm12(offset)) {
                auto offset_opr = create_legal_imm(offset,mb);
                ldr->base = cal_new_gep_base(cur_arr_base_opr, offset_opr, mb);
                ldr->offset = create_imm(0);
            }
            else {
                ldr->base = cur_arr_base_opr;
                ldr->offset = create_imm(offset);
            }
            cur_arr_base=NULL;
        }
        // yyy: 没有常量offset的array,代表gep已经算好base了，只用0（base）
        else
        {
            ldr->base = create_voper(from, mb);
            ldr->reg = create_voper(to, mb, true);
            ldr->offset = create_imm(0);
            cur_arr_base=NULL;
        }
    }
    else
    {
        int flag=0;
        for(auto v:globalValues)
        {
            if(v==from)
            {
                flag=1;
                break;
            }
        }
        if(stack_val_map.find(from)!=stack_val_map.end())
            flag=2;
        switch (flag)
        {
            // yyy: 简单寄存器
            case 0:{
                ldr->reg = create_voper(to,mb,true);
                ldr->base = create_voper(from, mb, true);
                isLoad?generate_move(ldr->reg,ldr->base,mb):generate_move(ldr->base,ldr->reg,mb);
                return;
            } break;

            // yyy: 全局地址,对于store来说，from就是全局地址的位置，to是要存的值
            case 1:{
                auto nvreg = create_vreg(vreg_num++);
                // yyy: la 目标寄存器是nvreg
                emit_load_of_global_ref(func_asm,from,nvreg,mb);
                ldr->base = nvreg;
                if(to->type->isArr()&&ldr->tag==VI_STORE ||
                   to->type->isArr()&&ldr->tag==VI_VSTORE){
                    assert(val_vopr_map.count(to));
                    auto array_base = val_vopr_map[to];
                    ldr->tag = VI_STORE;
                    ldr->offset = create_imm(0);
                    ldr->reg = array_base;
                    ldr->is_dw = true;
                    push((VI *)ldr, mb);
                    return;
                }
                if(to->type->isArr()&&ldr->tag==VI_LOAD ||
                   to->type->isArr()&&ldr->tag==VI_VLOAD){
                    // yyy: to is arrType
                    ldr->reg = create_voper(to, mb, true);
                    ldr->tag = VI_LOAD;
                    ldr->offset = create_imm(0);
                    ldr->is_dw = true;
                    push(ldr, mb);
                    return;
                }
                // store不能是立即数
                ldr->reg = create_voper(to, mb, !isLoad);
                if(isFloat&&int_val_map.count(ldr->base))
                    int_val_map[ldr->reg]=int_val_map[ldr->base];

            } break;
            
            // yyy: 存在栈中
            default:{
                ldr->reg = create_voper(to,mb,true);
                ldr->base = get_stack_val(from,mb,func_asm);
                if(isFloat&&int_val_map.count(ldr->base))
                    int_val_map[ldr->reg]=int_val_map[ldr->base];
            } break;
        }
    }
    push((VI *)ldr, mb);
}

template<typename T>
void combineGep(T* ptr_I, MachineBlock* mb){
    if(gep_map.mydeque.empty()){
        auto vopr =  deal_with_gep(ptr_I, mb);
        gep_map.insert(make_pair(ptr_I, vopr));
    }else{
        auto last_geps = gep_map.get();
        for(int i=0;i<gep_map.mydeque.size();i++){
            auto gep_ptr = last_geps[i].first;
            int offset;
            if(compare_gep(gep_ptr, ptr_I, offset)){
                auto vreg = create_voper(ptr_I->reg, mb, true);
                VOper last_vreg = last_geps[i].second;
                if(last_geps[i].second.tag == IMM){
                    auto vopr =  deal_with_gep(ptr_I, mb);
                    gep_map.insert(make_pair(ptr_I, vopr));
                    return ;
                }
                auto new_offset = create_legal_imm(offset*4, mb);
                auto bi_add = new VI_Binary(BINARY_ADD, vreg, last_vreg, new_offset);
                put_vi_binary_into_offset(vreg, last_vreg, new_offset,'+');
                bi_add->is_dw = true;
                push(bi_add, mb);
                gep_map.insert(make_pair(ptr_I, vreg));
                return;
            }
        }
        auto vopr = deal_with_gep(ptr_I, mb);
        gep_map.insert(make_pair(ptr_I, vopr));
    }
}

// yyy: ptr1是上一次的gep
bool compare_gep(GetElementPtrInstruction* ptr1, GetElementPtrInstruction* ptr2, int& offset){
    if(ptr1->from != ptr2->from || ptr1->index.size() != ptr2->index.size()) return false;
    else{
        size_t size = ptr1->index.size();
        auto vec = extract_array_sizes(ptr1->from->type->toString());
        vector<int> v1,v2;
        if(ptr1->index[0]!=ptr2->index[0]){
            bool tag = true;
            bool tag2 = true;
            if(val_vopr_map.count(ptr1->index[0])&&val_vopr_map.count(ptr2->index[0])){
                auto vreg1 = val_vopr_map[ptr1->index[0]];
                auto vreg2 = val_vopr_map[ptr2->index[0]];
                if(int_val_map.count(vreg1)&&int_val_map.count(vreg2)){
                    offset = int_val_map[vreg2] - int_val_map[vreg1];
                    tag = (int_val_map[vreg2] != int_val_map[vreg1]);
                    tag2 = false;
                }
                else if(find_in_value_offset(vreg2,vreg1,offset)){
                    tag = (offset != 0);
                    tag2 = false;
                }
            }
            if(tag){
                if(vec.empty()){
                    if(!tag2) return true;
                }
                return false;
            }
        }
        for(int i=1;i<size;i++){
            if(ptr1->index[i] != ptr2->index[i]){
                try{
                    int s1 = stoi(ptr1->index[i]->toString());
                    int s2 = stoi(ptr2->index[i]->toString());
                    v1.push_back(s1);
                    v2.push_back(s2);
                }catch (const std::invalid_argument&) {
                    int tmp = 0;
                    bool tag = true;
                    if(val_vopr_map.count(ptr1->index[i])&&val_vopr_map.count(ptr2->index[i])){
                        auto vreg1 = val_vopr_map[ptr1->index[i]];
                        auto vreg2 = val_vopr_map[ptr2->index[i]];
                        if(int_val_map.count(vreg1)&&int_val_map.count(vreg2)){
                            v1.push_back(int_val_map[vreg1]);
                            v2.push_back(int_val_map[vreg2]);
                            tag = false;
                        }
                        else if(find_in_value_offset(vreg2,vreg1,tmp)){
                            v1.push_back(0);
                            v2.push_back(tmp);
                            tag = false;
                        }
                    }
                    if(tag)return false;
                }
            }else{
                v1.push_back(0);
                v2.push_back(0);
            }
        }
        if(!vec.empty())  offset = calculateOffset(vec,v1,v2);
        return true;
    }
}

std::vector<int> extract_array_sizes(const std::string& type_str) {
    std::vector<int> sizes;
    std::string current_number;
    bool parsing_number = false;

    for (char c : type_str) {
        if (std::isdigit(c)) {
            // 如果字符是数字，继续解析当前数字
            current_number += c;
            parsing_number = true;
        } else {
            if (parsing_number) {
                // 当我们结束一个数字的解析时，将其转换为整数并存储
                sizes.push_back(std::stoi(current_number));
                current_number.clear();
                parsing_number = false;
            }

            // 如果遇到'i'，停止解析，因为我们已经提取完所有数组维度
            if (c == 'i') {
                break;
            }
        }
    }

    return sizes;
}

int calculateOffset(const std::vector<int>& dimensions, const std::vector<int>& pos1, const std::vector<int>& pos2) {
    int offset = 0;
    int stride = 1;
    
    // 从最后一维度开始计算偏移量
    for (int i = dimensions.size() - 1; i >= 0; --i) {
        offset += (pos2[i] - pos1[i]) * stride;
        stride *= dimensions[i];
    }
    
    return offset;
}

// yyy: find_in_value_offset，能保证在vopr_offset_map中的pair均是之前定义过的（否则ir是过不了的）,所以存储在function中
bool find_in_value_offset(VOper v1, VOper v2, int& result){
    auto pair1 = make_pair(v1,v2);
    auto pair2 = make_pair(v2,v1);
    if(vopr_offset_map.count(pair1)){
        result = vopr_offset_map[pair1];
        return true;
    }else if(vopr_offset_map.count(pair2)){
        result = 0-vopr_offset_map[pair2];
        return true;
    }
    return false;
}

void put_vi_binary_into_offset(VOper& dst, VOper& lhs, VOper& rhs, char op){
    if(rhs.tag == IMM || int_val_map.count(rhs)){
        if(int_val_map.count(lhs)){
            int32 value1 = (rhs.tag == IMM)?rhs.value:int_val_map[rhs];
            int32 value2 = int_val_map[lhs];
            switch (op)
            {
                case '+':{
                    int value = 0;
                    if(!__builtin_add_overflow(value1, value2, &value))
                        int_val_map[dst] = value;
                } break;

                case '-':{
                    int value = 0;
                    if(!__builtin_sub_overflow(value2, value1 , &value))
                        int_val_map[dst] = value;
                } break;

                case '*':{
                    int value=0;
                    if(!__builtin_mul_overflow(value1, value2, &value))
                        int_val_map[dst] = value;
                } break;

            default:
                break;
            }
        }
        else{
            int32 value1 = (rhs.tag == IMM)?rhs.value:int_val_map[rhs];
            switch (op)
            {
                case '+':{
                    vopr_offset_map[make_pair(dst,lhs)] = value1; 
                } break;

                case '-':{
                    int value = 0;
                    if(!__builtin_sub_overflow(0, value1, &value))
                        vopr_offset_map[make_pair(dst,lhs)] = value;
                } break;
            }
        }
    }
}