#pragma once
#include<vector>
#include<iostream>
#include"vi_inst.h"

using namespace std;
namespace{

void build_cvt_type(Asm_Buffer* s, Vcvt_Type t);

void print_vopr(Asm_Buffer *s, VOper op);
void build_function_asm(Asm_Buffer *s, Func_Asm *func);
const char *get_branch_suffix(Branch_Condition condition);
Branch_Condition invert_cmp_cond(Branch_Condition cond);

void build_cvt_type(Asm_Buffer* s, Vcvt_Type t){
    switch(t){
        case U32 : {
            s->print("wu");
        } break;
        case S32 : {
            s->print("w");
        } break;
        case F32 : {
            s->print("s");
        } break;
        default : {
            assert(false);
        }
    }
}

void print_operand(VOper op) {
    Asm_Buffer s;
    print_vopr(&s, op);
    cerr << s.c_str();
}

void print_vopr(Asm_Buffer *s, VOper op) {
    switch(op.tag) {
        
        case REG:   {
            if (op.value == fp) s->print("s0"); // s0 = fp
            else if (op.value == sp) s->print("sp");
            else if (op.value == ra) s->print("ra");
            else if (op.value == pc) s->print("pc");
            else if (op.value == zero) s->print("zero");
            else if (op.value == x3) s->print("gp");
            else if (op.value == x4) s->print("tp");
            else if (op.value >= x5 && op.value <= x7) s->print("t%d", op.value - x5);
            else if (op.value == x9) s->print("s1");
            else if (op.value >= x10 && op.value <= x17) s->print("a%d", op.value - x10);
            else if (op.value >= x18 && op.value <= x27) s->print("s%d", op.value - x18 + 2);
            else if (op.value >= x28 && op.value <= x31) s->print("t%d", op.value - x28 + 3);
            else {
                cout << "unknown register ID: " << op.value << endl;
                assert(false);
            }
        } break;
        case FREG:  {
            if (op.value >= f10 && op.value <= f17) s->print("fa%d", op.value - f10);
            else if (op.value >= f0 && op.value <= f7) s->print("ft%d", op.value);
            else if (op.value >= f8 && op.value <= f9) s->print("fs%d", op.value - f8);
            else if (op.value >= f18 && op.value <= f27) s->print("fs%d", op.value - f18 + 2);
            else if (op.value >= f28 && op.value <= f31) s->print("ft%d", op.value - f28 + 8);
            else {
                s->print("f%d\n", op.value);
            }
        } break;

        case VREG:  { s->print("vr%d", op.value); } break;
        case IMM:   { s->print("%d", op.value); } break;
		case ADR_GLOBAL: { s->print("%s", op.adr); } break;
        case VFREG: { s->print("vfr%d", op.value);} break;
		case ERRORTYPE:
        default: { 
            printf("op.tag: %d\n", op.tag);
            assert(false && "unknown op tag");
        } break;

    }
}

void build_function_asm(Asm_Buffer *s, Func_Asm *func) {
    auto block_size=func->mbs.size();
	s->print("%s:\n", func->name.c_str());
    for(int i = 0; i < block_size; i++) {
        MachineBlock *next_bb = NULL;
        if (i != block_size-1) next_bb = func->mbs[i+1];
        s->print(".F%d_B%d:\n", func->index, func->mbs[i]->index);
        for(auto I = func->mbs[i]->inst; I; I=I->next) {
            if(I->tag != VI_FCAST)s->print("    ");
            switch(I->tag) {
                case VI_MOVE: {
                    auto mv = (VI_Move *)I;
                    if (mv->src.tag == IMM) {
                        s->print("li ");
                        print_vopr(s, mv->dst);
                        s->print(",");
                        print_vopr(s, mv->src);
                    } else {
                        s->print("mv ");
                        print_vopr(s, mv->dst);
                        s->print(",");
                        print_vopr(s, mv->src);
                    }
                } break;

                case VI_FMOVE : {
                    auto vmv = (VI_FMove *)I;
                    s->print("fmv.");
                    if (vmv->from_s32) {
                        s->print("w.x ");
                    }
                    else if(vmv->from_f32){
                        s->print("x.w ");
                    }
                    else {
                        s->print("s ");
                    }
					print_vopr(s, vmv->dst);
					s->print(",");
					print_vopr(s, vmv->src);
                } break;

                case VI_BINARY: {
                    auto bi = (VI_Binary *)I;
                    // L-Shield：如果是64位的加，就是add，否则就是addw
                    const char *instr_type = bi->is_dw ? "" : "w";
                    VOper rhs = bi->rhs;

                    switch(bi->op) {
                        case BINARY_ADD:         { bi->rhs.tag == IMM ? s->print("addi") : s->print("add"); } break;
                        case BINARY_SUBTRACT:    { bi->rhs.tag == IMM ? rhs.value = -rhs.value, s->print("addi") : s->print("sub"); } break;
                        case BINARY_MULTIPLY:    { s->print("mul"); } break;
                        case BINARY_DIVIDE:      { s->print("div"); } break;
                        case BINARY_MOD:         { s->print("rem"); } break;
            			case BINARY_LSL:         { bi->rhs.tag == IMM ? s->print("slli") : s->print("sll"); } break;
            			case BINARY_LSR:         { bi->rhs.tag == IMM ? s->print("srli") : s->print("srl"); } break;
            			case BINARY_ASR:         { bi->rhs.tag == IMM ? s->print("srai") : s->print("sra"); } break;
						case BINARY_BITWISE_AND: { bi->rhs.tag == IMM ? s->print("andi") : s->print("and"); } break;
						case BINARY_BITWISE_OR:  { bi->rhs.tag == IMM ? s->print("ori")  : s->print("or");  } break;
                        case BINARY_XOR:         { bi->rhs.tag == IMM ? s->print("xori") : s->print("xor"); } break;
                        case BINARY_SH2ADD:      { s->print("sh2add"); } break;
                        case ATOMIC_ADD:         { s->print("amoadd.w.aqrl"); }break;
                        default:                 { assert(false && "unknown binary asm instruction."); }
                    }
                    // arithmetic opeartion 加减乘除
                    if (bi->op >= BINARY_ADD && bi->op <= BINARY_MOD) {
                        s->print("%s", instr_type);
                    }
                    s->print(" ");
                    print_vopr(s, bi->dst);
                    s->print(",");
                    print_vopr(s, bi->lhs);
                    s->print(",");
                    if(bi->op == ATOMIC_ADD) s->print("(");
                    print_vopr(s, rhs);
                    if(bi->op == ATOMIC_ADD) s->print(")");
                } break;
                
                case VI_VBINARY : {
                    auto bi = (VI_VBinary *)I;

                    switch(bi->op) {
                        case BINARY_ADD:         { s->print("fadd.s"); } break;
                        case BINARY_SUBTRACT:    { s->print("fsub.s"); } break;
                        case BINARY_MULTIPLY:    { s->print("fmul.s"); } break;
                        case BINARY_DIVIDE:      { s->print("fdiv.s"); } break;
                        case F_NEG:              { s->print("fneg.s"); } break;
                        default:                 { assert(false && "unknown binary asm instruction."); }
                    }
                    s->print(" ");
                    print_vopr(s, bi->dst);
                    s->print(",");
                    print_vopr(s, bi->lhs);
                    if(bi->fneg==1)
                    {
                        break;
                    }
                    s->print(",");
                    print_vopr(s, bi->rhs);
                } break;

                case VI_SLT: {
                    auto slt = (VI_Slt *)I;

                    switch(slt->slt_tag) {
                        case SLT:    { s->print("slt "); } break;
                        case SLT_U:  { s->print("sltu "); } break;
                        case SLT_I:  { s->print("slti "); } break;
                        case SLT_IU: { s->print("sltiu "); } break;
                        default:     { assert(false && "unknown VI_Slt tag"); }
                    }
                    print_vopr(s, slt->dst);
                    s->print(",");
                    print_vopr(s, slt->lhs);
                    s->print(",");
                    print_vopr(s, slt->rhs);
                } break;

                case VI_SEQZ:{
                    auto seqz = (VI_Seqz* )I;
                    s->print("seqz ");
                    print_vopr(s, seqz->dst);
                    s->print(",");
                    print_vopr(s, seqz->src);
                } break;

                case VI_SNEZ:{
                    auto snez = (VI_Snez* )I;
                    s->print("snez ");
                    print_vopr(s, snez->dst);
                    s->print(",");
                    print_vopr(s, snez->src);
                } break;

                case VI_BRANCH: {
                    auto br = (VI_Branch *)I;
                    if (next_bb && next_bb->condified) break;
                    if (br->cond == NO_CONDITION) {
                        if (br->true_target->index != i+1) {
                            s->print("j .F%d_B%d", func->index, br->true_target->index);
                        }
                    }
                    else if (br->true_target->index == i+1) {
                        s->print("b%s ", get_branch_suffix(invert_branch_cond(br->cond)));
                        print_vopr(s, br->lhs);
                        s->print(",");
                        print_vopr(s, br->rhs);
                        s->print(",.F%d_B%d # jump to false\n", func->index, br->false_target->index);
                    } else if (br->false_target->index == i+1) {
                        s->print("b%s ", get_branch_suffix(br->cond));
                        print_vopr(s, br->lhs);
                        s->print(",");
                        print_vopr(s, br->rhs);
                        s->print(",.F%d_B%d # jump to true\n", func->index, br->true_target->index);
                    } else {
                        s->print("b%s ", get_branch_suffix(br->cond));
                        print_vopr(s, br->lhs);
                        s->print(",");
                        print_vopr(s, br->rhs);
                        s->print(",.F%d_B%d # jump to true\n", func->index, br->true_target->index);
                        s->print("    ");
                        s->print("j .F%d_B%d # jump to false", func->index, br->false_target->index);
                    }
                } break;
                
                case VI_VPUSH:
                case VI_PUSH: {
                    auto push = (VI_Push *)I;
                    int num_reg = push->operands.size();
                    int reg_size = num_reg * 8;
                    std::sort(push->operands.begin(), push->operands.end());

                    s->print("addi sp,sp,%d # push\n", -reg_size);

                    for (int i = num_reg - 1; i >= 0; i --) {
                        I->tag==VI_PUSH ? s->print("    sd "): s->print("    fsd ");
                        print_vopr(s, push->operands[i]);
                        s->print(",%d(sp)", 8 * i);
                        if (i != 0) s->print("\n");
                    }
                } break;

                case VI_POP:
                case VI_VPOP: {
                    auto pop = (VI_VPop *)I;
                    int num_reg = pop->operands.size();
                    int reg_size = num_reg * 8;
                    // yyy: 将寄存器排序
                    std::sort(pop->operands.begin(), pop->operands.end());

                    for (int i = 0; i < num_reg; i ++) {
                        if (i > 0) s->print("    ");
                        I->tag==VI_POP ? s->print("ld ") : s->print("fld ");
                        print_vopr(s, pop->operands[i]);
                        s->print(",%d(sp)\n", 8 * i);
                    }
                    s->print("    addi sp,sp,%d # pop", reg_size);
                } break;

                case VI_FCAST:{
                    auto fcast = (VI_Fcast *)I;
                    s->print(".yyyLabel%d:\n",fcast->labelNum);
                    s->print("    auipc a2, %%pcrel_hi(%s)\n",fcast->funcName.c_str());
                    s->print("    addi a2, a2, %%pcrel_lo(.yyyLabel%d)",fcast->labelNum);
                } break;

                case VI_FUNC_CALL: {
                    auto call = (VI_Func_Call *)I;
                    s->print("call %s", call->func_name);
                } break;

                case VI_LOAD:
                case VI_STORE: {
                    auto load_or_store = (VI_Load *)I;
                    const char *instr_type = load_or_store->is_dw ? "d" : "w";
					if (load_or_store->base.tag == ADR_GLOBAL) {
                        s->print("la ");
                        print_vopr(s, load_or_store->reg);
                        s->print(",");
                        print_vopr(s, load_or_store->base);
                        s->print(" # load global address");
					}
					else if (load_or_store->base.tag == IMM) {
						auto val = load_or_store->base.value;
                        s->print("li ");
                        print_vopr(s, load_or_store->reg);
                        s->print(",");
                        print_vopr(s, load_or_store->base);
					}
					else {
						s->print(I->tag == VI_LOAD ? "l" : "s");
                        s->print("%s ", instr_type);
						print_vopr(s, load_or_store->reg);
						s->print(",");
                        // yyy: 用于la,li这种
                        if (load_or_store->offset.tag != ERRORTYPE) {
							print_vopr(s, load_or_store->offset);
						}
                        s->print("(");
						print_vopr(s, load_or_store->base);
						s->print(")");
					}
                } break;

                case VI_VLOAD : 
                case VI_VSTORE : {
                    auto load_or_store = (VI_VLoad *)I;
					if (load_or_store->base.tag == ADR_GLOBAL) {
                        s->print("la ");
                        print_vopr(s, load_or_store->reg);
                        s->print(",");
                        print_vopr(s, load_or_store->base);
                        s->print(" # load float global address");
					}
					else {
						s->print(I->tag == VI_VLOAD ? "fl" : "fs");
                        s->print(load_or_store->is_dw ? "d " : "w ");
						print_vopr(s, load_or_store->reg);
						s->print(",");
                        if (load_or_store->offset.tag != ERRORTYPE) {
							print_vopr(s, load_or_store->offset);
						}
                        s->print("(");
						print_vopr(s, load_or_store->base);
						s->print(")");
					}
                } break;

                case VI_RETURN: { 
                    s->print("jr ra");
                } break;

                case VI_VCVT : {
                    auto cvt = (VI_VCvt *)I;
                    s->print("fcvt.");
                    build_cvt_type(s, cvt->to_type);
                    s->print(".");
                    build_cvt_type(s, cvt->from_type);
                    s->print(" ");
                    print_vopr(s, cvt->dst);
                    s->print(",");
                    print_vopr(s, cvt->src);

                    if (cvt->from_type == F32) {
                        s->print(",rtz");
                    }
                } break;

                case VI_FCMP_SET : {
                    auto Vcmp = (VI_Fcmp_Set *)I;
                    VOper lhs, rhs;
                    assert(Vcmp->dst.tag == REG || Vcmp->dst.tag == VREG);

                    // feq, flt, fle
                    if (is_fcmp_cond(Vcmp->cond)) {
                        lhs = Vcmp->lhs;
                        rhs = Vcmp->rhs;
                        s->print("f%s.s ", get_branch_suffix(Vcmp->cond));
                    }
                    else if (Vcmp->cond == NOT_EQUAL) {
                        s->print("feq.s ");
                        print_vopr(s, Vcmp->dst);
                        s->print(",");
                        print_vopr(s, Vcmp->lhs);
                        s->print(",");
                        print_vopr(s, Vcmp->rhs);
                        s->print("\n    ");

                        s->print("xori ");
                        lhs = Vcmp->dst;
                        rhs = create_imm(1);
                    }
                    else {
                        // L-Shield：所以这里我们将两个操作数交换，那么操作符也可以交换方向了
                        lhs = Vcmp->rhs;
                        rhs = Vcmp->lhs;
                        s->print("f%s.s ", get_branch_suffix(invert_cmp_cond(Vcmp->cond)));
                    }

                    print_vopr(s, Vcmp->dst);
                    s->print(",");
                    print_vopr(s, lhs);
                    s->print(",");
                    print_vopr(s, rhs);
                } break;
                default:
                {
                    cout << "unknow assembly instruction ID: " << I->tag << endl;
                    assert(false && "printing unknown assembly instruction.");
                }break;
            }
            s->print("\n");
        }
    }
}

void sequence_initial(VariablePtr globalarr, vector<Pair<string, int> > &init_inst){
    if(globalarr->type->isInt()){
        int val_use = dynamic_cast<Int *>(globalarr.get())->intVal;
        if(val_use){
            init_inst.emplace_back(make_pair(".word",val_use));
        }else{
            if(init_inst.empty()||init_inst.back().first==".word"){
                init_inst.emplace_back(make_pair(".zero",0));
            }
            init_inst.back().second+=4;
        }
        return ;
    }else if(globalarr->type->isFloat()){
        f2i.f_value = dynamic_cast<Float *>(globalarr.get())->floatVal;
        init_inst.emplace_back(make_pair(".word",(f2i.bin_value)));
        return ;
    }
    if(globalarr->zero()){
        if(init_inst.empty()||init_inst.back().first==".word"){
            init_inst.emplace_back(make_pair(".zero",0));
        }
        int zero_size = dynamic_cast<ArrType*>((globalarr->type).get())->getSize();
        init_inst.back().second += zero_size*4;
        return ;
    }else{
        auto arr_use = dynamic_cast<Arr*>(globalarr.get());
        int i = 0;
        for(; i < arr_use->inner.size(); i++){
            sequence_initial(arr_use->inner[i],init_inst);
        }
        if(arr_use->getElementType()->isArr()){
            for(;i<arr_use->getElementLength();i++){
                sequence_initial(arr_use->inner[i], init_inst);
            }
        }else if(arr_use->getElementType()->isInt()||arr_use->getElementType()->isFloat()){
            if(i != arr_use->getElementLength()){
                if(init_inst.empty()||init_inst.back().first==".word"){
                    init_inst.emplace_back(make_pair(".zero",0));
                }
                init_inst.back().second += (arr_use->getElementLength() - i) * 4;
            }
        }
    }
}

void build_globals(Asm_Buffer *s, vector<VariablePtr> & globals) {
    if(globals.size() == 0) return;
    s->print(".data\n.align 2\n");
    for (int i = 0; i < globals.size(); i++){
        if(globals[i]->type->isInt()){
            s->print("%s:\n", globals[i]->name.c_str());
            s->print("    ");
            if(globals[i]->type->ID == Int64ID){
                int64 val = dynamic_cast<Int*>(globals[i].get())->intVal;
                s->print(".dword %d\n", val);
            }
            else{
                int32 val = dynamic_cast<Int*>(globals[i].get())->intVal;
                s->print(".word %d\n", val);
            }
        }else if(globals[i]->type->isFloat()){
            s->print("%s:\n", globals[i]->name.c_str());
            s->print("    ");
            f2i.f_value = dynamic_cast<Float*>(globals[i].get())->floatVal;
            s->print(".word %d\n", f2i.bin_value);
        }else if(globals[i]->type->isArr()){
            vector<Pair<string, int> > init_inst;
            auto globalarr = dynamic_cast<Arr*>(globals[i].get());
            if(globalarr->zero()){
                continue;
            }else{
                sequence_initial(globals[i],init_inst);
                s->print("%s:\n", globals[i]->name.c_str());
                for(int j = 0; j < init_inst.size(); j++){
                    s->print("    ");
                    s->print("%s  %d\n", init_inst[j].first.c_str(), init_inst[j].second);
                }
            }
        }else if(globals[i]->type->isPtr()){
            s->print("%s:\n",globals[i]->name.c_str());
            s->print("    ");
            s->print(".dword 0\n");
        }else{
            fprintf(stderr, "globals error type\n");
        }
    }
    s->print(".bss\n");
    s->print(".align 2\n");
    for (int i = 0; i < globals.size(); i++){
        if(globals[i]->type->isArr()){
            auto globalarr = dynamic_cast<Arr*>(globals[i].get());
            if(globalarr->zero()){
                int zero_size = dynamic_cast<ArrType*>((globalarr->type).get())->getSize();
                zero_size *=4;
                s->print("    .type %s, @object\n", globals[i]->name.c_str());
                s->print("    .size %s, %d\n", globals[i]->name.c_str(), zero_size);
                s->print("%s:\n", globals[i]->name.c_str());
                s->print("    .zero %d\n", zero_size);
            }
        }
    }
    return;
}

const char *get_branch_suffix(Branch_Condition condition) {
    switch(condition) {
        case NO_CONDITION:          { return ""; } break;
        case LESS_THAN:             { return "lt"; } break;
        case GREATER_THAN:          { return "gt"; } break;
        case LESS_THAN_OR_EQUAL:    { return "le"; } break;
        case GREATER_THAN_OR_EQUAL: { return "ge"; } break;
        case NOT_EQUAL:             { return "ne"; } break;
        case EQUAL:                 { return "eq"; } break;
        default: { assert(false); return ""; }
    }
}

// yyy: 将符号逆转，而不是条件
Branch_Condition invert_cmp_cond(Branch_Condition cond) {
    if (cond == NO_CONDITION) return cond;
    if (cond == LESS_THAN) return GREATER_THAN;
    else if (cond == GREATER_THAN) return LESS_THAN;
    else if (cond == GREATER_THAN_OR_EQUAL) return LESS_THAN_OR_EQUAL;
    else if (cond == LESS_THAN_OR_EQUAL) return GREATER_THAN_OR_EQUAL;
    assert(false);
    return NO_CONDITION;
}

}