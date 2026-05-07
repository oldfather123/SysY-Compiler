#include "arm.h"
#include "Module.h"
#include <cmath>
using namespace std;

int vreg_count;
int vsreg_count;

float2int f2i;
bool opt = false;

int ori_func_stack = 0;

// int offset=0;

static Map<ValuePtr, MOperand> value_map;
static Map<ValuePtr, MOperand> float_value_map;
static Map<MOperand, MOperand> imm_map;
static Map<ValuePtr, int> ptr_val_map;
static map<MOperand, int> int_val_map;
static map<string, Binary_Op_Type> Binary_ir2asm;
static map<ValuePtr, ValuePtr> my_arr_base;
static map<ValuePtr, int> stack_val_map;
ValuePtr cur_arr_base;
MOperand make_imm(int32 constant) { return MOperand(IMM, constant); }
MOperand make_reg(uint8 reg) { return MOperand(REG, reg); }
MOperand make_simm(float constant) { return MOperand(SIMM, constant); }
MOperand make_vreg(int32 vreg_index) { return MOperand(VREG, vreg_index); }
MOperand make_sreg(uint8 sreg) { return MOperand(SREG, sreg); }
MOperand make_vsreg(int32 vsreg_index) { return MOperand(VSREG, vsreg_index); }

uint32_t BitcastToUInt(float value) { return *reinterpret_cast<uint32_t *>(&value); }

bool IsImmediateValue(uint32_t value)
{
    int lowbit;

    if ((value & ~0xffU) == 0)
        return true;

    /* Get the number of trailing zeros.  */
    lowbit = log2(value & (-value)) - 1;

    lowbit &= ~1;

    if ((value & ~(0xffU << lowbit)) == 0)
        return true;

    if (lowbit <= 4)
    {
        if ((value & ~0xc000003fU) == 0 || (value & ~0xf000000fU) == 0 || (value & ~0xfc000003U) == 0)
        {
            return true;
        }
    }
    return false;
}

// this is used to put the later args(e.g. last four) on the stack into the func
//  @todo config it well
void emit_load_of_later_arg(ValuePtr a, MOperand vreg, Machine_Block *mb, int idx)
{
    // assert(idx >= 4);
    auto offset_relative_to_fp = (idx - 4) * 4;
    auto ldr = new MI_Load;
    ldr->mem_tag = MEM_LOAD_ARG;
    ldr->reg = vreg;
    ldr->base = make_reg(sp);
    ldr->offset = make_imm(offset_relative_to_fp);

    if (mb->control_transfer_inst)
    {
        insert(ldr, mb->control_transfer_inst);
    }
    else
    {
        push(ldr, mb);
    }
}

// this is the function to get the stack value
// because we put all the int into the stack
MOperand get_stack_val(ValuePtr v, Machine_Block *mb, Func_Asm *func_asm)
{
    int val = 0;
    if (ori_func_stack != 0)
        val = ori_func_stack - stack_val_map[v];
    else
        val = func_asm->stack_size - stack_val_map[v];
    auto bi_I = new MI_Binary;
    auto nreg = make_vreg(vreg_count++);
    bi_I->op = BINARY_ADD;
    bi_I->lhs = make_reg(sp);
    bi_I->rhs = make_imm(val);
    bi_I->dst = nreg;
    push((MI *)bi_I, mb);
    return bi_I->dst;
}

// insert the first MI before the second
void insert(MI *mi, MI *before)
{
    if (before->prev == NULL)
    {
        before->mb->inst = mi;
    }
    else
    {
        before->prev->next = mi;
    }
    mi->mb = before->mb;
    mi->prev = before->prev;
    mi->next = before;
    before->prev = mi;
}

// this is the function to load a global variable
void emit_load_of_global_ref(Func_Asm *func_asm, ValuePtr v, MOperand vreg, Machine_Block *mb)
{
    const char *addr = new char[1024];
    addr = v->name.c_str();
    MOperand adr_global(ADR_GLOBAL, addr);
    auto ldr = new MI_Load;
    ldr->reg = vreg;
    ldr->base = adr_global;
    ldr->mem_tag = MEM_LOAD_GLOBAL_REF;
    push((MI *)ldr, mb);
}

void push(MI *mi, Machine_Block *mb)
{
    // assert(mb->last_inst == NULL || (mb->last_inst->tag != MI_BRANCH && mb->last_inst->tag != MI_RETURN));
    // assert(mi->next == NULL);
    mi->mb = mb;
    if (mb->last_inst)
    {
        mb->last_inst->next = mi;
        mi->prev = mb->last_inst;
    }
    else
    {
        mb->inst = mi;
    }
    mb->last_inst = mi;
}

Branch_Condition binary_op_to_branch_cond(Binary_Op_Type op_type)
{
    switch (op_type)
    {
    case BINARY_LESS_THAN:
    {
        return LESS_THAN;
    }
    break;
    case BINARY_GREATER_THAN:
    {
        return GREATER_THAN;
    }
    break;
    case BINARY_LESS_THAN_OR_EQUAL_TO:
    {
        return LESS_THAN_OR_EQUAL;
    }
    break;
    case BINARY_GREATER_THAN_OR_EQUAL_TO:
    {
        return GREATER_THAN_OR_EQUAL;
    }
    break;
    case BINARY_NOT_EQUAL_TO:
    {
        return NOT_EQUAL;
    }
    break;
    case BINARY_EQUAL_TO:
    {
        return EQUAL;
    }
    break;
    case FLOAT_GE:
    {
        return float_ge;
    }
    break;
    default:
    {
        exit(31);
        report_error("Internal Compiler Error: converting unknown binary op type to branch type.");
    }
    }
    return NO_CONDITION;
}

// this is the function to handle a branch
void emit_branch_asm(Func_Asm *func_asm, Branch_Condition cond, BrInstruction *br_I, Machine_Block *mb)
{
    auto br = new MI_Branch();

    br->cond = cond;
    cout << cond << endl;
    auto l1 = br_I->label1;
    auto l2 = br_I->label2;

    int idx1 = 0;
    int idx2 = 0;
    cout << l1->name << endl;
    for (int i = 0; i < func_asm->mbs.size(); i++)
    {
        // cout<<i<<" "<<func_asm->mbs[i]->label->name;
        if (func_asm->mbs[i]->label->name == l1->name)
        {
            // cout<<func_asm->mbs[i]->label->name<<" "<<l1->name<<endl;
            idx1 = i;
        }
        if (l2 != NULL && func_asm->mbs[i]->label->name == l2->name)
        {
            idx2 = i;
        }
    }
    br->true_target = func_asm->mbs[idx1];
    cout << "branch: " << idx1 << " " << idx2 << endl;
    br->false_target = func_asm->mbs[idx2];

    push((MI *)br, mb);
    mb->control_transfer_inst = (MI *)br;
}

// this is a copy of branch, use to hack the false case
void emit_branch_asm_2(Func_Asm *func_asm, Branch_Condition cond, BrInstruction *br_I, Machine_Block *mb)
{
    auto br = new MI_Branch();

    br->cond = cond;
    cout << cond << endl;
    auto l1 = br_I->label1;
    auto l2 = br_I->label2;
    // if(l2==NULL)
    //     cout<<"!!!"<<endl;

    int idx1 = 0;
    int idx2 = 0;
    cout << l1->name << endl;
    for (int i = 0; i < func_asm->mbs.size(); i++)
    {
        // cout<<i<<" "<<func_asm->mbs[i]->label->name;
        if (func_asm->mbs[i]->label->name == l1->name)
        {
            // cout<<func_asm->mbs[i]->label->name<<" "<<l1->name<<endl;
            idx1 = i;
        }
        if (l2 != NULL && func_asm->mbs[i]->label->name == l2->name)
        {
            idx2 = i;
        }
    }
    br->true_target = func_asm->mbs[idx2];
    cout << "branch: " << idx1 << " " << idx2 << endl;
    br->false_target = func_asm->mbs[idx1];

    push((MI *)br, mb);
    mb->control_transfer_inst = (MI *)br;
}

// this is a function to move the value from one reg to another
MI_Move *emit_move(MOperand dst, MOperand src, Machine_Block *mb = NULL)
{
    auto mv = new MI_Move(dst, src);
    int_val_map[dst] = int_val_map[src];
    if (mb)
    {
        if (mb->control_transfer_inst)
        {
            insert(mv, mb->control_transfer_inst);
        }
        else
        {
            push(mv, mb);
        }
    }
    return mv;
}

MI_VMove *emit_vmove(MOperand dst, MOperand src, Machine_Block *mb = NULL)
{
    auto mv = new MI_VMove(dst, src, false);
    int_val_map[dst] = int_val_map[src];
    if (mb)
    {
        if (mb->control_transfer_inst)
        {
            insert(mv, mb->control_transfer_inst);
        }
        else
        {
            push(mv, mb);
        }
    }
    return mv;
}

// this is an api to load a constant to the reg
MI_Load *emit_load_of_constant(MOperand vreg, int32 constant)
{
    auto ldr = new MI_Load;
    ldr->mem_tag = MEM_LOAD_FROM_LITERAL_POOL;
    ldr->reg = vreg;
    ldr->base = make_imm(constant);
    return ldr;
}

// 获得操作数的api, 将你所用到的MB换为可操作的对象
//  this is to make an operand
MOperand make_operand(ValuePtr v, Machine_Block *mb, bool no_imm = false)
{
    // auto s=v->getStr();
    int ID = v->type->getID();
    int tp = -1;
    if (v->isConst)
        tp = 0;
    else if (v->isReg)
    {
        tp = 1;
        cout << "type is reg!!" << endl;
    }

    else if (v->isVoid)
        tp = 2;
    else
    {
        if (v->type->isArr())
            tp = 4;
        else if (v->type->isInt())
            tp = 3;
        else if (v->type->isPtr())
            tp = 5;
        else if (v->type->isBool())
            tp = 6;
    }

    auto opr = value_map.find(v);
    if (opr != value_map.end())
    {
        cout << "Find!!!" << endl;
        return opr->second;
    }

    cout << "type is :" << tp << endl;

    switch (tp)
    {

    case 0:
    {

        if (v->type->isFloat())
        {
            float tmpconstant = dynamic_cast<Const *>(v.get())->floatVal;

            cout << "float is " << tmpconstant << endl;

            uint32_t constant = BitcastToUInt(tmpconstant);

            no_imm = !IsImmediateValue(constant);

            if (IsImmediateValue(constant))
            {
                cout << "immediate yes!!!" << endl;
            }
            else
            {
                cout << "immediate no!!!" << endl;
            }
            auto imm = make_imm(constant);
            // if (no_imm)
            // {
            auto vreg = make_vreg(vreg_count++);
            // // @FIXME: DO NOT PUSH DIRECTLY
            MI_Load *ldr = emit_load_of_constant(vreg, constant);
            if (mb)
            {
                if (mb->control_transfer_inst)
                {
                    insert(ldr, mb->control_transfer_inst);
                }
                else
                {
                    push(ldr, mb);
                }
            }
            // value_map[v] = vreg;
            int_val_map[vreg] = constant;
            return vreg;
            // }
            // else
            // {
            //     return imm;
            // }
        }

        else
        {
            int32 constant = dynamic_cast<Const *>(v.get())->intVal;
            auto imm = make_imm(constant);
            if (no_imm)
            {
                auto vreg = make_vreg(vreg_count++);
                // // @FIXME: DO NOT PUSH DIRECTLY
                MI_Load *ldr = emit_load_of_constant(vreg, constant);
                if (mb)
                {
                    if (mb->control_transfer_inst)
                    {
                        insert(ldr, mb->control_transfer_inst);
                    }
                    else
                    {
                        push(ldr, mb);
                    }
                }
                // value_map[v] = vreg;
                int_val_map[vreg] = constant;
                return vreg;
            }
            else
            {
                return imm;
            }
        }
    }
    break;

    case 5:
    {
        int32 constant = 0;
        auto imm = make_imm(constant);
        if (no_imm)
        {
            auto vreg = make_vreg(vreg_count++);
            // // @FIXME: DO NOT PUSH DIRECTLY
            MI_Load *ldr = emit_load_of_constant(vreg, constant);
            if (mb)
            {
                if (mb->control_transfer_inst)
                {
                    insert(ldr, mb->control_transfer_inst);
                }
                else
                {
                    push(ldr, mb);
                }
            }
            value_map[v] = vreg;
            int_val_map[vreg] = constant;
            return vreg;
        }
        else
        {
            return imm;
        }
    }
    break;

    case 3:
    {
        int32 constant = dynamic_cast<Int *>(v.get())->intVal;
        if(!can_be_imm_ror(constant))
        {
            printf("%d can not be imm ror\n",constant);
        }

        auto imm = make_imm(constant);
        cout << constant << endl;
        if (no_imm || !can_be_imm_ror(constant))
        {
            auto vreg = make_vreg(vreg_count++);
            // // @FIXME: DO NOT PUSH DIRECTLY
            MI_Load *ldr = emit_load_of_constant(vreg, constant);
            if (mb)
            {
                if (mb->control_transfer_inst)
                {
                    insert(ldr, mb->control_transfer_inst);
                }
                else
                {
                    push(ldr, mb);
                }
            }
            // value_map[v] = vreg;
            int_val_map[vreg] = constant;
            return vreg;
        }
        else
        {
            return imm;
        }
    }
    break;

    case 4:
    {
        int32 constant = 0;
        auto imm = make_imm(constant);
        if (no_imm)
        {
            auto vreg = make_vreg(vreg_count++);
            // // @FIXME: DO NOT PUSH DIRECTLY
            MI_Load *ldr = emit_load_of_constant(vreg, constant);
            if (mb)
            {
                if (mb->control_transfer_inst)
                {
                    insert(ldr, mb->control_transfer_inst);
                }
                else
                {
                    push(ldr, mb);
                }
            }
            value_map[v] = vreg;
            int_val_map[vreg] = constant;
            return vreg;
        }
        else
        {
            return imm;
        }
    }
    break;

    default:
    {
        auto opr = value_map.find(v);
        if (opr != value_map.end())
        {
            return opr->second;
        }
        auto new_vreg = make_vreg(vreg_count++);
        value_map[v] = new_vreg;
        return new_vreg;
    }
    }
}

// this is to emit an binary instruction
void emit_binary_value_naive(InstructionPtr I, Machine_Block *mb)
{
    switch (I->type)
    {
    case Fneg:
    {
        cout << "111" << endl;
        auto bi_I = dynamic_cast<FnegInstruction *>(I.get());
        auto op_type = F_NEG;
        // cout<<"bi_i->op:"<<myop<<endl;
        cout << "optype: " << op_type << endl;

        if (bi_I->a->type->isFloat())
        {
            auto bii = new MI_VBinary;
            auto name1 = bi_I->a->name;
            auto lhs = make_operand(bi_I->a, mb, true);
            auto rhs = lhs;

            // auto dst = make_vsreg(vsreg_count++);
            value_map[bi_I->reg] = make_operand(bi_I->a, mb, true);
            bii->dst = make_operand(bi_I->a, mb, true);
            bii->op = op_type;
            bii->lhs = lhs;
            // bii->rhs=rhs;
            // auto bi = new MI_Binary(op_type, dst, lhs, rhs);
            bii->fneg = 1;
            push((MI *)bii, mb);
            break;
        }

        // todo: and or binary_type???
    }
    break;
    case Binary:
    {
        auto bi_I = dynamic_cast<BinaryInstruction *>(I.get());
        string myop;
        myop += bi_I->op;
        auto op_type = Binary_ir2asm[myop];
        cout << "bi_i->op:" << myop << endl;
        cout << "optype: " << op_type << endl;
        if (op_type == BINARY_MOD)
        {
            auto bi_divide = new MI_Binary;
            bi_divide->op = BINARY_DIVIDE;
            bi_divide->lhs = make_operand(bi_I->a, mb, true);
            bi_divide->rhs = make_operand(bi_I->b, mb, true);
            bi_divide->dst = make_vreg(vreg_count++);
            push((MI *)bi_divide, mb);
            auto bi_mul = new MI_Binary;
            bi_mul->op = BINARY_MULTIPLY;
            bi_mul->lhs = bi_divide->dst;
            bi_mul->rhs = bi_divide->rhs;
            bi_mul->dst = make_vreg(vreg_count++);
            push((MI *)bi_mul, mb);
            auto bi_sub = new MI_Binary;
            bi_sub->op = BINARY_SUBTRACT;
            bi_sub->lhs = bi_divide->lhs;
            bi_sub->rhs = bi_mul->dst;
            bi_sub->dst = make_operand(bi_I->reg, mb);
            push((MI *)bi_sub, mb);
        }
        else if (op_type == UNARY_XOR)
        {
            cout << bi_I->a->getStr() << endl;
            cout << bi_I->b->getStr() << endl;

            auto bi_sub = new MI_Binary;
            bi_sub->op = BINARY_SUBTRACT;
            auto imm = make_imm(1);
            auto imm_reg = make_vreg(vreg_count++);
            emit_move(imm_reg, imm, mb);
            bi_sub->lhs = imm_reg;
            bi_sub->rhs = make_operand(bi_I->a, mb, true);
            bi_sub->dst = make_operand(bi_I->reg, mb);
            push((MI *)bi_sub, mb);
        }
        else
        {
            if (bi_I->a->type->isFloat())
            {
                auto bii = new MI_VBinary;
                auto name1 = bi_I->a->name;
                auto name2 = bi_I->b->name;
                cout << "myname is " << name1 << endl;
                cout << "my name is" << name2 << endl;
                auto lhs = make_operand(bi_I->a, mb, true);
                MOperand rhs;
                if (bi_I->op == '*' || bi_I->op == '/')
                    rhs = make_operand(bi_I->b, mb, /* no imm for mul and sdiv */ true);
                else
                    rhs = make_operand(bi_I->b, mb);

                cout << "lhs is " << lhs.tag << " "
                     << "rhs is " << rhs.tag << endl;

                if (lhs.tag != SREG && lhs.tag != VSREG)
                {
                    auto mydes = lhs;
                    auto tp = mydes;
                    // if (mydes.tag != VSREG)
                    // {
                    //     tp = make_vreg(vreg_count++);
                    //     emit_move(tp, mydes, mb);
                    // }
                    auto tpsreg = make_vsreg(vsreg_count++);
                    emit_vmove(tpsreg, tp, mb);
                    auto mi = new MI_VCvt;
                    mi->to_type = F32;
                    mi->from_type = S32;
                    mi->dst = tpsreg;
                    mi->src = tpsreg;
                    if (!bi_I->a->type->isFloat())
                        push((MI *)mi, mb);
                    lhs = tpsreg;
                }
                if (rhs.tag != SREG && rhs.tag != VSREG)
                {
                    auto mydes = rhs;
                    auto tp = mydes;
                    // if (mydes.tag != VSREG)
                    // {
                    //     tp = make_vreg(vreg_count++);
                    //     emit_move(tp, mydes, mb);
                    // }
                    auto tpsreg = make_vsreg(vsreg_count++);
                    emit_vmove(tpsreg, tp, mb);
                    auto mi = new MI_VCvt;
                    mi->to_type = F32;
                    mi->from_type = S32;
                    mi->dst = tpsreg;
                    mi->src = tpsreg;
                    if (!bi_I->b->type->isFloat())
                        push((MI *)mi, mb);
                    rhs = tpsreg;
                }
                auto dst = make_vsreg(vsreg_count++);
                value_map[bi_I->reg] = dst;
                bii->dst = dst;
                bii->op = op_type;
                bii->lhs = lhs;
                bii->rhs = rhs;
                // auto bi = new MI_Binary(op_type, dst, lhs, rhs);
                push((MI *)bii, mb);
                break;
            }
            auto name1 = bi_I->a->name;
            auto name2 = bi_I->b->name;
            auto lhs = make_operand(bi_I->a, mb, true);
            MOperand rhs;
            if (bi_I->op == '*' || bi_I->op == '/')
                rhs = make_operand(bi_I->b, mb, /* no imm for mul and sdiv */ true);
            else
                rhs = make_operand(bi_I->b, mb);

            auto dst = make_operand(bi_I->reg, mb);
            auto bi = new MI_Binary(op_type, dst, lhs, rhs);
            push((MI *)bi, mb);
            // if(bi_I->a->type->isFloat()&&bi_I->b->type->isFloat()){
            //     auto bi = new MI_VBinary(op_type, dst, lhs, rhs);
            //     push((MI *)bi, mb);
            // }else{
            //     auto bi = new MI_Binary(op_type, dst, lhs, rhs);
            //     push((MI *)bi, mb);
            // }
        } // todo: and or binary_type???
    }
    break;

    case Icmp:
    {
        auto icmp_I = dynamic_cast<IcmpInstruction *>(I.get());
        auto lhs = make_operand(icmp_I->a, mb, true);
        auto rhs = make_operand(icmp_I->b, mb);
        auto cmp = new MI_Compare(lhs, rhs);
        push(cmp, mb);

        auto result_vreg = make_operand(icmp_I->reg, mb);

        auto false_mv = new MI_Move(result_vreg, make_imm(0));
        auto true_mv = new MI_Move(result_vreg, make_imm(1));
        true_mv->cond = binary_op_to_branch_cond(Binary_ir2asm[icmp_I->op]);

        push(false_mv, mb);
        push(true_mv, mb);
    }
    break;

    case Fcmp:
    {
        auto fcmp_I = dynamic_cast<FcmpInstruction *>(I.get());
        auto lhs = make_operand(fcmp_I->a, mb, true);

        cout << "This is my Fcmp" << endl;

        cout << fcmp_I->a->getStr() << endl;
        cout << fcmp_I->b->getStr() << endl;

        if (fcmp_I->a->type->isFloat())
        {
            // MOperand arg_operand = make_sreg(s8);
            auto mydes = lhs;
            auto tp = mydes;
            if (mydes.tag != VSREG)
            {
                tp = make_vreg(vreg_count++);
                emit_move(tp, mydes, mb);
            }
            auto tpsreg = make_vsreg(vsreg_count++);
            emit_vmove(tpsreg, tp, mb);
            auto mi = new MI_VCvt;
            mi->to_type = F32;
            mi->from_type = S32;
            mi->dst = tpsreg;
            mi->src = tpsreg;
            push((MI *)mi, mb);
            lhs = tpsreg;
            // emit_vmove(arg_operand, tpsreg, mb);
        }
        auto rhs = make_operand(fcmp_I->b, mb);

        if (fcmp_I->b->type->isFloat())
        {
            // MOperand arg_operand = make_sreg(s9);
            auto mydes = rhs;
            auto tp = mydes;
            if (mydes.tag != VSREG)
            {
                tp = make_vreg(vreg_count++);
                emit_move(tp, mydes, mb);
            }
            auto tpsreg = make_vsreg(vsreg_count++);
            emit_vmove(tpsreg, tp, mb);
            auto mi = new MI_VCvt;
            mi->to_type = F32;
            mi->from_type = S32;
            mi->dst = tpsreg;
            mi->src = tpsreg;
            push((MI *)mi, mb);
            // emit_vmove(arg_operand, tpsreg, mb);
            rhs = tpsreg;
        }
        auto cmp = new MI_VCompare(lhs, rhs);
        push(cmp, mb);

        auto result_vreg = make_operand(fcmp_I->reg, mb);

        auto false_mv = new MI_Move(result_vreg, make_imm(0));
        auto true_mv = new MI_Move(result_vreg, make_imm(1));
        true_mv->cond = binary_op_to_branch_cond(Binary_ir2asm[fcmp_I->op]);

        push(false_mv, mb);
        push(true_mv, mb);
    }
    break;

    default:
    {
        auto bi_I = dynamic_cast<BinaryInstruction *>(I.get());
        string myop;
        myop += bi_I->op;
        auto op_type = Binary_ir2asm[myop];
        // auto op_type = Binary_ir2asm[string(bi_I->op,1)];
        auto lhs = make_operand(bi_I->a, mb, true);
        MOperand rhs;
        if (bi_I->op == '*' || bi_I->op == '/')
            rhs = make_operand(bi_I->b, mb, /* no imm for mul and sdiv */ true);
        else
            rhs = make_operand(bi_I->b, mb);

        auto dst = make_operand(bi_I->reg, mb);
        auto bi = new MI_Binary(op_type, dst, lhs, rhs);
        push((MI *)bi, mb);
    }
    break;
    }
}

// this is to handle the branch instruction or the Binary instruction
void emit_legacy(Func_Asm *func_asm, InstructionPtr I, BasicBlockPtr bb, Machine_Block *mb, int32 &i)
{
    switch (I->type)
    {
    case Br:
    {
        auto br_I = dynamic_cast<BrInstruction *>(I.get());
        auto tp = br_I->exp;
        if (tp != NULL)
            cout << tp->getStr() << endl;

        auto cond = NO_CONDITION;
        if (tp != NULL)
        {
            cout << "mymy" << endl;
            if (tp->getStr() == "true")
            {
                cout << 222 << endl;
                emit_branch_asm(func_asm, cond, br_I, mb);
                break;
            }
            else if (tp->getStr() == "false")
            {
                cout << 333 << endl;
                emit_branch_asm_2(func_asm, cond, br_I, mb);
                break;
            }
            auto cmp = new MI_Compare(make_operand(br_I->exp, mb, true), make_imm(0));
            push((MI *)cmp, mb);
            cond = binary_op_to_branch_cond(BINARY_NOT_EQUAL_TO);
            cout << cond << endl;
            emit_branch_asm(func_asm, NOT_EQUAL, br_I, mb);
        }
        else
        {
            cout << "jiji" << endl;
            emit_branch_asm(func_asm, cond, br_I, mb);
        }
    }
    break;
    case Fneg:
    case Binary:
    {
        auto next_inst = bb->instructions[i + 1]; // todo : maybe get some wrong answers now
        if (next_inst->type != Br)
        {
            emit_binary_value_naive(I, mb);
        }
        else
        {
            emit_binary_value_naive(I, mb);
            // auto lhs = make_operand(dynamic_cast<BinaryInstruction*>(I.get())->reg, mb, true);
            // auto cmp = new MI_Compare(lhs, make_imm(0));
            // mb->control_transfer_inst=cmp;

            // push((MI *)cmp, mb);
            // i++; // handle the branch IR at the same time

            // auto br_I = (BrInstruction *)(next_inst.get());
            // emit_branch_asm(func_asm, NOT_EQUAL, br_I, mb);
            // mb->control_transfer_inst=cmp;
        }
    }
    break;

    case Icmp:
    {
        auto next_inst = bb->instructions[i + 1]; // todo : maybe get some wrong answers now
        if (next_inst->type != Br)
        {
            emit_binary_value_naive(I, mb);
        }
        else
        {
            auto icmp_i = dynamic_cast<IcmpInstruction *>(I.get());
            auto lhs = make_operand(icmp_i->a, mb, true);
            auto rhs = make_operand(icmp_i->b, mb);
            auto cmp = new MI_Compare(lhs, rhs);
            if (cmp->rhs.tag == IMM && cmp->rhs.value < 0)
            {
                cmp->rhs.value *= -1;
                cmp->neg = true; // cmn
            }
            push((MI *)cmp, mb);
            i++;
            auto br_I = (BrInstruction *)(next_inst.get());
            emit_branch_asm(func_asm, binary_op_to_branch_cond(Binary_ir2asm[icmp_i->op]), br_I, mb);
            mb->control_transfer_inst = cmp;
        }
    }
    break;

    case Fcmp:
    {
        auto next_inst = bb->instructions[i + 1]; // todo : maybe get some wrong answers now
        if (next_inst->type != Br)
        {
            emit_binary_value_naive(I, mb);
        }
        else
        {
            auto fcmp_I = dynamic_cast<FcmpInstruction *>(I.get());
            auto lhs = make_operand(fcmp_I->a, mb, true);
            cout << "This is my Fcmp" << endl;

            cout << fcmp_I->a->getStr() << endl;
            cout << fcmp_I->b->getStr() << endl;

            if (fcmp_I->a->type->isFloat())
            {
                // MOperand arg_operand = make_sreg(s8);
                auto mydes = lhs;
                auto tp = mydes;
                if (mydes.tag != VSREG)
                {
                    tp = make_vreg(vreg_count++);
                    emit_move(tp, mydes, mb);
                }
                auto tpsreg = make_vsreg(vsreg_count++);
                emit_vmove(tpsreg, tp, mb);
                auto mi = new MI_VCvt;
                mi->to_type = F32;
                mi->from_type = S32;
                mi->dst = tpsreg;
                mi->src = tpsreg;
                push((MI *)mi, mb);
                // emit_vmove(arg_operand, tpsreg, mb);
                lhs = tpsreg;
            }
            auto rhs = make_operand(fcmp_I->b, mb);

            if (fcmp_I->b->type->isFloat())
            {
                // MOperand arg_operand = make_sreg(s9);
                auto mydes = rhs;
                auto tp = mydes;
                if (mydes.tag != VSREG)
                {
                    tp = make_vreg(vreg_count++);
                    emit_move(tp, mydes, mb);
                }
                auto tpsreg = make_vsreg(vsreg_count++);
                emit_vmove(tpsreg, tp, mb);
                auto mi = new MI_VCvt;
                mi->to_type = F32;
                mi->from_type = S32;
                mi->dst = tpsreg;
                mi->src = tpsreg;
                push((MI *)mi, mb);
                // emit_vmove(arg_operand, tpsreg, mb);
                rhs = tpsreg;
            }
            // auto rhs = make_operand(fcmp_I->b, mb);
            auto cmp = new MI_VCompare(lhs, rhs);
            push(cmp, mb);

            auto result_vreg = make_operand(fcmp_I->reg, mb);

            auto false_mv = new MI_Move(result_vreg, make_imm(0));
            auto true_mv = new MI_Move(result_vreg, make_imm(1));
            true_mv->cond = binary_op_to_branch_cond(Binary_ir2asm[fcmp_I->op]);

            cout << "FCMP op is" << fcmp_I->op << endl;

            i++;
            auto br_I = (BrInstruction *)(next_inst.get());
            emit_branch_asm(func_asm, true_mv->cond, br_I, mb);
            mb->control_transfer_inst = cmp;
            push(false_mv, mb);
            push(true_mv, mb);
        }
    }
    break;
    }
}

// this is to handle the asm of a function
Func_Asm *emit_function_asm(FunctionPtr func, int idx, Module program_module, vector<VariablePtr> &globalValues)
{
    cout << func->name << " "
         << "222" << endl;
    vreg_count = 0;
    vsreg_count = 0;
    value_map.clear();

    auto func_asm = new Func_Asm;
    func_asm->index = idx;
    func_asm->name = func->name;

    func_asm->stack_size = 100;

    // copy first four arguments to vregs
    cout << 2233 << endl;
    // cout<<func->formArguments.size();

    // cout<<333<<endl;

    auto mybb = func->block;
    int ii = 0;
    cout << "before: " << mybb->basicBlocks[0]->instructions.size() << endl;
    // vector<ValuePtr> mygv;
    // mygv.push_back(VariablePtr(new Int("useless", false, false, 0)));
    // auto ins = shared_ptr<CallInstruction>(new CallInstruction(program_module.globalFunctions[3], mygv, mybb->basicBlocks[0]));
    // // mybb->basicBlocks[0]->instructions.emplace(++std::begin(mybb->basicBlocks[0]->instructions),ins);
    // // mybb->basicBlocks[0]->pushEndInstruction(ins);
    // vector<InstructionPtr> tmp;
    // tmp.emplace_back(ins);
    // for(auto x: mybb->basicBlocks[0]->instructions)
    // {
    //     tmp.emplace_back(x);
    // }
    // mybb->basicBlocks[0]->instructions=tmp;

    cout << "after: " << mybb->basicBlocks[0]->instructions.size() << endl;

    int n = mybb->basicBlocks.size();
    // cout<<2233<<endl;

    for (auto bb : mybb->basicBlocks)
    {
        // if(ii==n-1)
        //     break;
        func_asm->mbs.push(new Machine_Block());
        func_asm->mbs.back()->i = ii;
        func_asm->mbs.back()->label = bb->label;
        // if(ii!=0)
        // {
        //     // func_asm->mbs[ii-1]->succs.push(func_asm->mbs[ii]);
        //     func_asm->mbs[ii]->preds.push(func_asm->mbs[ii-1]);
        // }
        // for(auto x: bb->succBasicBlocks)
        // {
        //     func_asm->mbs[ii]->succs.push(x.first);
        // }

        // for(auto x: bb->predBasicBlocks)
        // {
        //     func_asm->mbs[ii]->preds.push(x.first);
        // }
        ii++;
        func_asm->mbs.back()->loop_depth = bb->loopDepth;
        // func_asm->mbs.back()->belongs_to_loop = bb->belongs_to_loop;
    }

    int ii2 = 0;

    for (auto bb : mybb->basicBlocks)
    {
        for (auto x : bb->succBasicBlocks)
        {
            int pidx = 0;
            for (auto y : mybb->basicBlocks)
            {
                if (x.first == y)
                    break;
                pidx++;
            }
            func_asm->mbs[ii2]->succs.push(func_asm->mbs[pidx]);
        }
        for (auto x : bb->predBasicBlocks)
        {
            int pidx = 0;
            for (auto y : mybb->basicBlocks)
            {
                if (x.first == y)
                    break;
                pidx++;
            }
            func_asm->mbs[ii2]->preds.push(func_asm->mbs[pidx]);
        }
        ii2++;
    }

    for (int i = 0; i < 4 && i < func->formArguments.size(); i++)
    {
        cout << "curr " << i << endl;
        auto arg = func->formArguments[i];

        // if (arg) continue; // arg is not used!

        // assert(arg->arg_index >= 0 && arg->arg_index < 4);
        if (arg->type->isFloat())
        {
            auto vsreg = make_vsreg(vsreg_count++);
            value_map[arg] = vsreg;
            emit_vmove(vsreg, make_sreg(i), func_asm->mbs[0]);
            continue;
        }
        auto vreg = make_vreg(vreg_count++);
        // cout<<vreg_count<<endl;
        value_map[arg] = vreg;
        // cout<<vreg_count<<endl;

        emit_move(vreg, make_reg(i), func_asm->mbs[0]);
    }

    // want to load more args to the function
    // @ todo make it correct
    for (int i = 4; i < func->formArguments.size(); i++)
    {
        cout << "curr " << i << endl;
        auto arg = func->formArguments[i];
        auto a = make_vreg(vreg_count++);
        // auto b=make_vreg(vreg_count++);
        emit_load_of_later_arg(arg, a, func_asm->mbs[0], i);
        // cout<<vreg_count<<endl;
        // auto ldr = new MI_Load;
        // ldr->mem_tag = MEM_LOAD_FROM_LITERAL_POOL;
        // ldr->reg = b;
        // ldr->base = a;
        // push((MI *)ldr, func_asm->mbs[0]);
        value_map[arg] = a;
        // cout<<vreg_count<<endl;
    }

    int bbidx = 0;

    for (auto bb : mybb->basicBlocks)
    {

        Machine_Block *mb = func_asm->mbs[bbidx];
        bbidx++;
        for (int i = 0; i < bb->instructions.size(); i++)
        {
            auto I = bb->instructions[i];
            cout << "----" << endl;
            I->print();
            cout << I->type << endl;
            cout << "----" << endl;
            //     // I->print();
            switch (I->type)
            {
            case Br:
            {
                if (!opt)
                {
                    emit_legacy(func_asm, I, bb, mb, i);
                    break;
                }
            }
            break;

            case Fneg:
            case Binary:
            {
                if (!opt)
                {
                    emit_legacy(func_asm, I, bb, mb, i);
                    break;
                }
            }

            case Icmp:
            {
                if (!opt)
                {
                    emit_legacy(func_asm, I, bb, mb, i);
                    break;
                }
            }

            case Fcmp:
            {
                if (!opt)
                {
                    emit_legacy(func_asm, I, bb, mb, i);
                    break;
                }
            }

            case Sitofp:
            {
                auto mim = new MI_VMove;
                auto mi = new MI_VCvt;
                auto conv_I = dynamic_cast<SitofpInstruction *>(I.get());
                auto src = conv_I->from;
                auto dst = make_vsreg(vsreg_count++);
                value_map[conv_I->reg] = dst;
                mim->src = make_operand(src, mb, true);
                mim->dst = dst;
                mi->to_type = F32;
                mi->from_type = S32;
                push((MI *)mim, mb);

                mi->dst = dst;
                mi->src = dst;

                push((MI *)mi, mb);
            }
            break;

            case Fptosi:
            {
                auto mim = new MI_VMove;
                auto mi = new MI_VCvt;
                auto conv_I = dynamic_cast<FptosiInstruction *>(I.get());
                auto src = conv_I->from;
                auto dst = make_vreg(vreg_count++);
                auto tpsreg = make_vsreg(vsreg_count++);
                value_map[conv_I->reg] = dst;

                mi->to_type = S32;
                mi->from_type = F32;

                mi->dst = tpsreg;
                mi->src = value_map[src];

                push((MI *)mi, mb);
                mim->src = tpsreg;
                mim->dst = dst;
                push((MI *)mim, mb);
            }
            break;

            // this is to handle the get pointer instruction
            case GEP:
            {
                auto ptr_I = dynamic_cast<GetElementPtrInstruction *>(I.get());
                int flag2 = 0;
                auto sname = ptr_I->from->name;
                int isarrele = 0;
                // cout<<ptr_I->reg->name<<endl;

                if (ptr_I->from->name.substr(0, 8) != "arrayidx")
                {
                    if (my_arr_base.find(ptr_I->from) == my_arr_base.end())
                        my_arr_base[ptr_I->reg] = ptr_I->from;
                    else
                        my_arr_base[ptr_I->reg] = my_arr_base[ptr_I->from];
                    int flag = 0;
                    for (auto x : globalValues)
                    {
                        if (ptr_I->from == x)
                        {
                            flag = 1;
                            break;
                        }
                    }
                    if (flag)
                    {
                        auto nvreg = make_vreg(vreg_count++);
                        emit_load_of_global_ref(func_asm, ptr_I->from, nvreg, mb);
                        value_map[ptr_I->from] = nvreg;
                    }
                    if (ptr_I->reg->name.substr(0, 9) == "arrayinit")
                        isarrele = 1;
                }
                else
                {
                    my_arr_base[ptr_I->reg] = my_arr_base[ptr_I->from];
                }

                cout << "mybase is" << my_arr_base[ptr_I->reg]->getStr() << endl;

                auto tp = make_operand(ptr_I->from, mb);
                // int const_size=1;
                auto x = ptr_I->index.back();
                auto tpp = dynamic_cast<ArrType *>(ptr_I->from->type.get());
                TypePtr cur_arr = NULL;
                auto reg4 = make_imm(0);
                int cff = 0;
                int offset = 0;
                int ff = 0;
                if (ptr_val_map.find(ptr_I->from) != ptr_val_map.end())
                {
                    offset = ptr_val_map.find(ptr_I->from)->second;
                    cff = 1;
                }

                if (value_map.find(ptr_I->from) != value_map.end() && ptr_I->from->name.substr(0, 9) == "arrayinit" && !cff)
                {
                    // cff=1;
                    ff = 1;
                    reg4 = value_map[ptr_I->from];
                }

                auto mul_reg1 = make_imm(0);
                auto mul_reg2 = make_imm(0);
                auto mul_reg3 = make_imm(0);
                auto mul_reg4 = make_imm(0);
                auto mul_reg5 = make_imm(0);
                auto mul_reg6 = make_imm(0);
                auto mul_reg7 = make_imm(0);

                int cnt = 0;
                cout << "arrele is" << isarrele << endl;
                for (auto x : ptr_I->index)
                {
                    if (!x->isConst)
                        ff = 1;
                }
                cout << "ff is" << ff << endl;
                if (isarrele)
                {
                    for (int i = 0; i < ptr_I->index.size(); i++)
                    {
                        cnt++;
                        auto x = ptr_I->index[i];
                        cout << x->getStr() << endl;
                        int const_val = 0;

                        if (x->isConst)
                            const_val = dynamic_cast<Const *>(x.get())->intVal;
                        else
                        {
                            ff = 1;
                        }
                        cout << "my_const " << const_val << endl;

                        int const_size = 1;
                        if (i == 0)
                        {
                            // auto tmp=ptr_I->from->type->getID();
                            // cout<<"id is "<<tmp<<endl;
                            auto tmp = dynamic_cast<PtrType *>(ptr_I->from->type.get());
                            if (tmp->inner->isArr())
                                const_size = dynamic_cast<ArrType *>(tmp->inner.get())->getSize();
                            cout << "const size is" << const_size << endl;
                            // if(tmp->inner->isArr())
                            //     const_size=dynamic_cast<ArrType *>(tmp->inner.get())->getSize();
                        }
                        if (i == 1)
                        {
                            auto tmp = dynamic_cast<ArrType *>(ptr_I->from->type.get());
                            if (tmp->inner->isArr())
                                const_size = dynamic_cast<ArrType *>(tmp->inner.get())->getSize();
                            cur_arr = tmp->inner;
                        }
                        else if (i > 1)
                        {
                            auto nc = dynamic_cast<ArrType *>(cur_arr.get());
                            if (nc->inner->isArr())
                                const_size = dynamic_cast<ArrType *>(nc->inner.get())->getSize();
                            cur_arr = nc->inner;
                        }
                        if (!ff)
                            offset += const_size * const_val;
                        else
                        {
                            if (x != ptr_I->index.back() || const_size != 1)
                            {
                                ValuePtr imptr = ValuePtr(new Int("asdf", false, false, const_size));
                                auto bi_mul = new MI_Binary;
                                bi_mul->op = BINARY_MULTIPLY;
                                bi_mul->lhs = make_operand(x, mb, true);
                                bi_mul->rhs = make_operand(imptr, mb, true);
                                if (cnt == 1)
                                {
                                    mul_reg1 = make_vreg(vreg_count++);
                                    bi_mul->dst = mul_reg1;
                                }
                                else if (cnt == 2)
                                {
                                    mul_reg2 = make_vreg(vreg_count++);
                                    bi_mul->dst = mul_reg2;
                                }
                                else if (cnt == 3)
                                {
                                    mul_reg3 = make_vreg(vreg_count++);
                                    bi_mul->dst = mul_reg3;
                                }
                                else if (cnt == 4)
                                {
                                    mul_reg4 = make_vreg(vreg_count++);
                                    bi_mul->dst = mul_reg4;
                                }
                                else if (cnt == 5)
                                {
                                    mul_reg5 = make_vreg(vreg_count++);
                                    bi_mul->dst = mul_reg5;
                                }
                                else if (cnt == 6)
                                {
                                    mul_reg6 = make_vreg(vreg_count++);
                                    bi_mul->dst = mul_reg6;
                                }
                                else if (cnt == 7)
                                {
                                    mul_reg7 = make_vreg(vreg_count++);
                                    bi_mul->dst = mul_reg7;
                                }
                                push((MI *)bi_mul, mb);
                            }
                            else
                            {
                                if (cnt == 1)
                                    mul_reg1 = make_operand(x, mb, true);
                                else if (cnt == 2)
                                    mul_reg2 = make_operand(x, mb, true);
                                else if (cnt == 3)
                                    mul_reg3 = make_operand(x, mb, true);
                                else if (cnt == 4)
                                    mul_reg4 = make_operand(x, mb, true);
                                else if (cnt == 5)
                                    mul_reg5 = make_operand(x, mb, true);
                                else if (cnt == 6)
                                    mul_reg6 = make_operand(x, mb, true);
                                else if (cnt == 7)
                                    mul_reg7 = make_operand(x, mb, true);
                            }

                            cout << "arrele" << endl;
                        }
                    }
                }
                else
                {
                    for (int i = 1; i < ptr_I->index.size(); i++)
                    {
                        auto x = ptr_I->index[i];
                        int const_val = 0;
                        cout << x->getStr() << endl;
                        cnt++;

                        if (x->isConst)
                            const_val = dynamic_cast<Const *>(x.get())->intVal;
                        else
                        {
                            ff = 1;
                        }
                        cout << "my_const " << const_val << endl;

                        int const_size = 1;
                        if (i == 1)
                        {
                            auto tmp = dynamic_cast<ArrType *>(ptr_I->from->type.get());
                            if (tmp->inner->isArr())
                                const_size = dynamic_cast<ArrType *>(tmp->inner.get())->getSize();
                            cur_arr = tmp->inner;
                        }
                        else
                        {
                            auto nc = dynamic_cast<ArrType *>(cur_arr.get());
                            if (nc->inner->isArr())
                                const_size = dynamic_cast<ArrType *>(nc->inner.get())->getSize();
                            cur_arr = nc->inner;
                        }
                        if (!ff)
                            offset += const_size * const_val;
                        else
                        {

                            if (x != ptr_I->index.back())
                            {
                                ValuePtr imptr = ValuePtr(new Int("asdf", false, false, const_size));
                                auto bi_mul = new MI_Binary;
                                bi_mul->op = BINARY_MULTIPLY;
                                bi_mul->lhs = make_operand(x, mb, true);
                                bi_mul->rhs = make_operand(imptr, mb, true);
                                if (cnt == 1)
                                {
                                    mul_reg1 = make_vreg(vreg_count++);
                                    bi_mul->dst = mul_reg1;
                                }
                                else if (cnt == 2)
                                {
                                    mul_reg2 = make_vreg(vreg_count++);
                                    bi_mul->dst = mul_reg2;
                                }
                                else if (cnt == 3)
                                {
                                    mul_reg3 = make_vreg(vreg_count++);
                                    bi_mul->dst = mul_reg3;
                                }
                                else if (cnt == 4)
                                {
                                    mul_reg4 = make_vreg(vreg_count++);
                                    bi_mul->dst = mul_reg4;
                                }
                                else if (cnt == 5)
                                {
                                    mul_reg5 = make_vreg(vreg_count++);
                                    bi_mul->dst = mul_reg5;
                                }
                                else if (cnt == 6)
                                {
                                    mul_reg6 = make_vreg(vreg_count++);
                                    bi_mul->dst = mul_reg6;
                                }
                                else if (cnt == 7)
                                {
                                    mul_reg7 = make_vreg(vreg_count++);
                                    bi_mul->dst = mul_reg7;
                                }
                                push((MI *)bi_mul, mb);
                            }
                            else
                            {
                                if (cnt == 1)
                                    mul_reg1 = make_operand(x, mb, true);
                                else if (cnt == 2)
                                    mul_reg2 = make_operand(x, mb, true);
                                else if (cnt == 3)
                                    mul_reg3 = make_operand(x, mb, true);
                                else if (cnt == 4)
                                    mul_reg4 = make_operand(x, mb, true);
                                else if (cnt == 5)
                                    mul_reg5 = make_operand(x, mb, true);
                                else if (cnt == 6)
                                    mul_reg6 = make_operand(x, mb, true);
                                else if (cnt == 7)
                                    mul_reg7 = make_operand(x, mb, true);
                            }
                        }
                    }
                }
                if (!ff)
                {
                    cout << "offset is" << offset << endl;
                    auto myimm = make_imm(offset);
                    auto curreg = make_operand(ptr_I->reg, mb, true);
                    curreg.value = offset;
                    ptr_val_map[ptr_I->reg] = offset;
                    // emit_move(curreg,myimm,mb);
                }
                else
                {
                    if (cnt == 1)
                    {
                        auto bi_add = new MI_Binary;
                        bi_add->op = BINARY_ADD;
                        bi_add->lhs = mul_reg1;
                        bi_add->rhs = reg4;
                        bi_add->dst = make_operand(ptr_I->reg, mb, true);
                        push((MI *)bi_add, mb);
                        cout << "print here" << endl;
                        print_operand(mul_reg1);
                    }
                    else if (cnt == 2)
                    {
                        auto bi_add = new MI_Binary;
                        bi_add->op = BINARY_ADD;
                        bi_add->lhs = mul_reg1;
                        bi_add->rhs = mul_reg2;
                        bi_add->dst = make_operand(ptr_I->reg, mb, true);
                        push((MI *)bi_add, mb);
                    }
                    else if (cnt == 3)
                    {
                        auto tppreg = make_vreg(vreg_count++);
                        auto bi_add = new MI_Binary;
                        bi_add->op = BINARY_ADD;
                        bi_add->lhs = mul_reg1;
                        bi_add->rhs = mul_reg2;
                        bi_add->dst = tppreg;
                        push((MI *)bi_add, mb);
                        auto bi_add2 = new MI_Binary;
                        bi_add2->op = BINARY_ADD;
                        bi_add2->lhs = tppreg;
                        bi_add2->rhs = mul_reg3;
                        bi_add2->dst = make_operand(ptr_I->reg, mb, true);
                        push((MI *)bi_add2, mb);
                    }
                    else if (cnt == 4)
                    {
                        auto tppreg = make_vreg(vreg_count++);
                        auto tppreg2 = make_vreg(vreg_count++);
                        auto bi_add = new MI_Binary;
                        bi_add->op = BINARY_ADD;
                        bi_add->lhs = mul_reg1;
                        bi_add->rhs = mul_reg2;
                        bi_add->dst = tppreg;
                        push((MI *)bi_add, mb);
                        auto bi_add2 = new MI_Binary;
                        bi_add2->op = BINARY_ADD;
                        bi_add2->lhs = tppreg;
                        bi_add2->rhs = mul_reg3;
                        bi_add2->dst = tppreg2;
                        push((MI *)bi_add2, mb);
                        auto bi_add3 = new MI_Binary;
                        bi_add3->op = BINARY_ADD;
                        bi_add3->lhs = tppreg2;
                        bi_add3->rhs = mul_reg4;
                        bi_add3->dst = make_operand(ptr_I->reg, mb, true);
                        push((MI *)bi_add3, mb);
                    }
                    else if (cnt == 5)
                    {
                        auto tppreg = make_vreg(vreg_count++);
                        auto tppreg2 = make_vreg(vreg_count++);
                        auto tppreg3 = make_vreg(vreg_count++);
                        auto bi_add = new MI_Binary;
                        bi_add->op = BINARY_ADD;
                        bi_add->lhs = mul_reg1;
                        bi_add->rhs = mul_reg2;
                        bi_add->dst = tppreg;
                        push((MI *)bi_add, mb);
                        auto bi_add2 = new MI_Binary;
                        bi_add2->op = BINARY_ADD;
                        bi_add2->lhs = tppreg;
                        bi_add2->rhs = mul_reg3;
                        bi_add2->dst = tppreg2;
                        push((MI *)bi_add2, mb);
                        auto bi_add3 = new MI_Binary;
                        bi_add3->op = BINARY_ADD;
                        bi_add3->lhs = tppreg2;
                        bi_add3->rhs = mul_reg4;
                        bi_add3->dst = tppreg3;
                        push((MI *)bi_add3, mb);
                        auto bi_add4 = new MI_Binary;
                        bi_add4->op = BINARY_ADD;
                        bi_add4->lhs = tppreg3;
                        bi_add4->rhs = mul_reg5;
                        bi_add4->dst = make_operand(ptr_I->reg, mb, true);
                        push((MI *)bi_add4, mb);
                    }
                    else if (cnt == 6)
                    {
                        auto tppreg = make_vreg(vreg_count++);
                        auto tppreg2 = make_vreg(vreg_count++);
                        auto tppreg3 = make_vreg(vreg_count++);
                        auto tppreg4 = make_vreg(vreg_count++);
                        auto bi_add = new MI_Binary;
                        bi_add->op = BINARY_ADD;
                        bi_add->lhs = mul_reg1;
                        bi_add->rhs = mul_reg2;
                        bi_add->dst = tppreg;
                        push((MI *)bi_add, mb);
                        auto bi_add2 = new MI_Binary;
                        bi_add2->op = BINARY_ADD;
                        bi_add2->lhs = tppreg;
                        bi_add2->rhs = mul_reg3;
                        bi_add2->dst = tppreg2;
                        push((MI *)bi_add2, mb);
                        auto bi_add3 = new MI_Binary;
                        bi_add3->op = BINARY_ADD;
                        bi_add3->lhs = tppreg2;
                        bi_add3->rhs = mul_reg4;
                        bi_add3->dst = tppreg3;
                        push((MI *)bi_add3, mb);
                        auto bi_add4 = new MI_Binary;
                        bi_add4->op = BINARY_ADD;
                        bi_add4->lhs = tppreg3;
                        bi_add4->rhs = mul_reg5;
                        bi_add4->dst = tppreg4;
                        push((MI *)bi_add4, mb);
                        auto bi_add5 = new MI_Binary;
                        bi_add5->op = BINARY_ADD;
                        bi_add5->lhs = tppreg4;
                        bi_add5->rhs = mul_reg6;
                        bi_add5->dst = make_operand(ptr_I->reg, mb, true);
                        push((MI *)bi_add5, mb);
                    }
                    else if (cnt == 7)
                    {
                        auto tppreg = make_vreg(vreg_count++);
                        auto tppreg2 = make_vreg(vreg_count++);
                        auto tppreg3 = make_vreg(vreg_count++);
                        auto tppreg4 = make_vreg(vreg_count++);
                        auto tppreg5 = make_vreg(vreg_count++);
                        auto bi_add = new MI_Binary;
                        bi_add->op = BINARY_ADD;
                        bi_add->lhs = mul_reg1;
                        bi_add->rhs = mul_reg2;
                        bi_add->dst = tppreg;
                        push((MI *)bi_add, mb);
                        auto bi_add2 = new MI_Binary;
                        bi_add2->op = BINARY_ADD;
                        bi_add2->lhs = tppreg;
                        bi_add2->rhs = mul_reg3;
                        bi_add2->dst = tppreg2;
                        push((MI *)bi_add2, mb);
                        auto bi_add3 = new MI_Binary;
                        bi_add3->op = BINARY_ADD;
                        bi_add3->lhs = tppreg2;
                        bi_add3->rhs = mul_reg4;
                        bi_add3->dst = tppreg3;
                        push((MI *)bi_add3, mb);
                        auto bi_add4 = new MI_Binary;
                        bi_add4->op = BINARY_ADD;
                        bi_add4->lhs = tppreg3;
                        bi_add4->rhs = mul_reg5;
                        bi_add4->dst = tppreg4;
                        push((MI *)bi_add4, mb);
                        auto bi_add5 = new MI_Binary;
                        bi_add5->op = BINARY_ADD;
                        bi_add5->lhs = tppreg4;
                        bi_add5->rhs = mul_reg6;
                        bi_add5->dst = tppreg5;
                        push((MI *)bi_add5, mb);
                        auto bi_add6 = new MI_Binary;
                        bi_add6->op = BINARY_ADD;
                        bi_add6->lhs = tppreg5;
                        bi_add6->rhs = mul_reg7;
                        bi_add6->dst = make_operand(ptr_I->reg, mb, true);
                        push((MI *)bi_add6, mb);
                    }
                }
            }
            break;

            // this is to handle the return value
            case Return:
            {
                auto ret_I = dynamic_cast<ReturnInstruction *>(I.get());
                auto ret = new MI_Return;
                auto retv = ret_I->retValue;
                auto myrev = 0;
                if (retv->type->isFloat())
                {
                    // auto nsvreg = make_vsreg(vsreg_count++);
                    // value_map[ret_I->retValue] = nsvreg;
                    auto ret_value = make_operand(ret_I->retValue, mb);
                    emit_vmove(make_sreg(s0), ret_value, mb);
                    push((MI *)ret, mb);
                    // emit_vmove(nsvreg, make_sreg(s0), mb);

                    cout << "111" << endl;

                    mb->control_transfer_inst = (MI *)ret;
                    break;
                }
                if (retv->isConst)
                    myrev = dynamic_cast<Const *>(retv.get())->intVal;
                auto ret_value = make_operand(ret_I->retValue, mb);
                emit_move(make_reg(r0), ret_value, mb);

                push((MI *)ret, mb);

                cout << "111" << endl;

                mb->control_transfer_inst = (MI *)ret;
            }
            break;

            // deal with the ext instruction
            case Ext:
            {
                auto ext_I = dynamic_cast<ExtInstruction *>(I.get());
                // auto nreg=make_operand(ext_I->reg,mb,true);
                auto oreg = make_operand(ext_I->from, mb, true);
                // emit_move(nreg,oreg,mb);
                value_map[ext_I->reg] = oreg;
            }
            break;

            // deal with the call instruction
            // @todo: fix the function call of many args
            case Call:
            {
                auto func_I = dynamic_cast<CallInstruction *>(I.get());
                auto func_name = func_I->func->name;
                if (func_name.size() > 15 && func_name.substr(5, 6) == "memset")
                {
                    auto cut_name = func_name.substr(5, 6);
                    cout << cut_name << endl;
                    auto call = new MI_Func_Call("memset");
                    call->arg_count = func_I->func->formArguments.size() - 1;
                    for (int i = 0; i < func_I->func->formArguments.size() - 1; i++)
                    {
                        auto arg_value = func_I->argv[i];

                        MOperand arg_operand;
                        // memset's argument always less than 4
                        // if (i < 4) {
                        arg_operand = make_reg(i);
                        emit_move(arg_operand, make_operand(arg_value, mb), mb);
                        // } else {
                        //     // move argument to the stack
                        //     auto str = new MI_Store;
                        //     str->mem_tag = MEM_PREP_ARG;
                        //     str->reg = make_operand(arg_value, mb, true);
                        //     str->base = make_reg(sp);
                        //     str->offset = make_imm((i-4)*4);
                        //     push((MI *)str, mb);
                        // }
                    }

                    push((MI *)call, mb);

                    ValuePtr ret = func_I->func->retVal;
                    cout << "ret" << ret->type->getID() << endl;

                    if (!ret->isVoid)
                    {
                        // cout<<"ret!!!"<<endl;
                        auto result = make_operand(func_I->reg, mb, true);
                        emit_move(result, make_reg(0), mb);
                        // 如果有返回值必须存放到r0的寄存器
                        //  func_I->reg=
                    }
                    break;
                }
                // 我觉得是把普通参数和浮点数参数分开
                // 分别是(int 数组) 和 (float) 二者都是用4这个范围区分开
                // 和原来的区别就是浮点数寄存器的特殊指令，以及类型转换，其他可以复用
                auto call = new MI_Func_Call(func_I->func->name.c_str());
                call->arg_count = func_I->func->formArguments.size();
                int myff = 0;
                for (int i = 0; i < func_I->func->formArguments.size(); i++)
                {
                    auto arg_value = func_I->argv[i];
                    cout << "mycall ==" << arg_value->getStr() << endl;
                    // if(arg_value->name=="useless")
                    //     myff=1;
                    MOperand arg_operand;
                    if (i < 4)
                    {
                        if (stack_val_map.find(arg_value) != stack_val_map.end())
                        {
                            arg_operand = make_reg(i);
                            auto ldr = new MI_Load;
                            ldr->reg = make_vreg(vreg_count++);
                            ldr->base = get_stack_val(arg_value, mb, func_asm);
                            push((MI *)ldr, mb);
                            emit_move(arg_operand, ldr->reg, mb);
                        }
                        else
                        {
                            arg_operand = make_reg(i);
                            if (arg_value->type->isFloat())
                            {
                                arg_operand = make_sreg(i);
                                auto mydes = make_operand(arg_value, mb, false);
                                cout << "tag is " << mydes.tag << endl;
                                auto tp = mydes;
                                if (mydes.tag != VSREG && mydes.tag != SREG)
                                {
                                    tp = make_vreg(vreg_count++);
                                    emit_move(tp, mydes, mb);
                                }
                                auto tpsreg = make_vsreg(vsreg_count++);
                                emit_vmove(tpsreg, tp, mb);
                                auto mi = new MI_VCvt;
                                mi->to_type = F32;
                                mi->from_type = S32;
                                mi->dst = tpsreg;
                                mi->src = tpsreg;
                                if (!arg_value->type->isFloat())
                                    push((MI *)mi, mb);
                                emit_vmove(arg_operand, tpsreg, mb);
                                continue;
                            }
                            auto mydes = make_operand(arg_value, mb, false);
                            if (my_arr_base.find(arg_value) != my_arr_base.end())
                            {

                                auto arr_base = my_arr_base.find(arg_value)->second;
                                auto my_offset = make_imm(0);

                                int arrflag =0;
                                if (ptr_val_map.find(arg_value) == ptr_val_map.end())
                                {
                                    my_offset = make_vreg(vreg_count++);
                                    auto bi_I = new MI_Binary;
                                    bi_I->op = BINARY_LSL;
                                    bi_I->lhs = make_operand(arg_value, mb, true);
                                    bi_I->rhs = make_imm(2);
                                    bi_I->dst = my_offset;
                                    push((MI *)bi_I, mb);
                                    arrflag =1;
                                }

                                else
                                    my_offset = make_imm(ptr_val_map[arg_value] * 4);
                                    
                                    
                                mydes = make_vreg(vreg_count++);
                                auto bi_I = new MI_Binary;
                                bi_I->op = BINARY_ADD;
                                bi_I->lhs = make_operand(arr_base, mb, true);
                                bi_I->rhs = my_offset;
                                bi_I->dst = mydes;
                                push((MI *)bi_I, mb);
                            }
                            emit_move(arg_operand, mydes, mb);
                        }
                    }
                    else
                    {
                        cout << 2323 << endl;

                        if (ptr_val_map.find(arg_value) != ptr_val_map.end())
                        {
                            cur_arr_base = my_arr_base[arg_value];
                            auto opr = ptr_val_map.find(arg_value);
                            auto bi_add = new MI_Binary;
                            bi_add->op = BINARY_ADD;
                            bi_add->lhs = make_operand(cur_arr_base, mb);
                            bi_add->rhs = make_imm(opr->second * 4);
                            bi_add->dst = make_vreg(vreg_count++);
                            push((MI *)bi_add, mb);
                            // auto ldr = new MI_Load;
                            // ldr->reg = make_vreg(vreg_count++);
                            // cur_arr_base=my_arr_base[arg_value];
                            // ldr->base = make_operand(cur_arr_base, mb);
                            // auto opr=ptr_val_map.find(arg_value);
                            // ldr->offset.tag = IMM;
                            // ldr->offset.value = opr->second*4;
                            cout << 2424 << endl;
                            // push((MI *)ldr, mb);
                            // emit_move(arg_operand, ldr->reg, mb);
                            auto str = new MI_Store;
                            str->mem_tag = MEM_PREP_ARG;
                            str->reg = bi_add->dst;
                            str->base = make_reg(sp);
                            str->offset = make_imm((i - 4) * 4);
                            push((MI *)str, mb);
                        }
                        // move argument to the stack
                        // move argument to the stack
                        else
                        {
                            auto str = new MI_Store;
                            str->mem_tag = MEM_PREP_ARG;
                            str->reg = make_operand(arg_value, mb, true);
                            str->base = make_reg(sp);
                            str->offset = make_imm((i - 4) * 4);
                            push((MI *)str, mb);
                        }
                    }
                }
                if (!myff)
                    push((MI *)call, mb);

                ValuePtr ret = func_I->func->retVal;
                cout << "ret" << ret->type->getID() << endl;

                if (!ret->isVoid)
                {
                    if (ret->type->isFloat())
                    {
                        auto nsreg = make_vsreg(vsreg_count++);
                        value_map[func_I->reg] = nsreg;
                        // auto result = make_operand(func_I->reg, mb, true);
                        emit_vmove(nsreg, make_sreg(s0), mb);
                    }
                    else
                    {
                        auto result = make_operand(func_I->reg, mb, true);
                        emit_move(result, make_reg(0), mb);
                    }
                }
            }
            break;

            // this is to deal with the allocate instruction
            case Alloca:
            {
                // cout<<"Alloca"<<endl;
                auto alloc = dynamic_cast<AllocaInstruction *>(I.get());
                auto tmp = dynamic_cast<Variable *>(alloc->des.get());
                // auto mytp=tmp->type;
                int tp = tmp->type->getID();
                auto const_value = 0;
                // func_asm->stack_size = 100;
                cout << "Alloca type: " << tp << endl;
                switch (tp)
                {
                case IntID:
                {
                    auto intptr = dynamic_cast<Int *>(tmp);
                    const_value = 4;
                    func_asm->stack_size += const_value;
                    // stack_val_map[alloc->des]=func_asm->stack_size;
                }
                break;

                case ArrID:
                {
                    // if(tmp->name)
                    auto mytype = dynamic_cast<ArrType *>(tmp->type.get());
                    // auto arrptr=dynamic_cast<Arr *>(tmp);
                    auto myname = tmp->name;
                    auto ptr = mytype->inner;

                    const_value = mytype->getSize() * 4;
                    // if(ptr->isFloat())
                    // {
                    //     cout<<"ArrFloating"<<endl;
                    //     const_value*=2;
                    // }
                    cout << const_value << endl;
                    func_asm->stack_size += const_value;
                    // cout<<"ArrID "<<endl;
                }
                break;

                case PtrID:
                {
                    // auto intptr=dynamic_cast<Int *>(tmp);
                    const_value = 4;
                    func_asm->stack_size += const_value;
                }
                break;
                }
                if (tp == IntID)
                    break;
                auto bi_I = new MI_Binary;
                bi_I->op = BINARY_SUBTRACT;
                bi_I->lhs = make_reg(sp);
                bi_I->rhs = make_imm(func_asm->stack_size);
                bi_I->dst = make_operand(alloc->des, mb, true);
                func_asm->local_array_bases.push(bi_I);
                push((MI *)bi_I, mb);
            }
            break;

            // bitcast instruction: no need to modify the reg
            case Bitcast:
            {
                auto bi_I = dynamic_cast<BitCastInstruction *>(I.get());
                // auto nreg=make_operand(bi_I->reg,mb);
                auto oreg = make_operand(bi_I->from, mb);
                // emit_move(nreg,oreg,mb);
                value_map[bi_I->reg] = oreg;
            }
            break;

            // load instruction
            // 1: array type, has const offset
            // 2: array type, no const offset
            // 3: reg, simple
            // 4: reg, global
            // 5: reg, store in stack
            case Load:
            {
                auto Read_I = dynamic_cast<LoadInstruction *>(I.get());
                auto ldr = new MI_Load;

                ldr->base = make_operand(Read_I->from, mb);

                if (Read_I->from->type->isFloat())
                {
                    auto ldr2 = new MI_VLoad;

                    ldr2->base = make_operand(Read_I->from, mb);
                    // cout<<"Floating!!"<<endl;
                    if (my_arr_base.find(Read_I->from) != my_arr_base.end())
                    {
                        if (ptr_val_map.find(Read_I->from) != ptr_val_map.end())
                        {
                            // ldr2->reg = make_operand(Read_I->to, mb, true);
                            ldr2->reg = make_vsreg(vsreg_count++);
                            value_map[Read_I->to] = ldr2->reg;
                            cur_arr_base = my_arr_base[Read_I->from];
                            ldr2->base = make_operand(cur_arr_base, mb);
                            auto opr = ptr_val_map.find(Read_I->from);
                            ldr2->offset.tag = IMM;
                            ldr2->offset.value = opr->second * 4;
                            cur_arr_base = NULL;
                        }
                        else
                        {
                            // ldr->reg = make_operand(Read_I->to, mb, true);
                            ldr2->reg = make_vsreg(vsreg_count++);
                            value_map[Read_I->to] = ldr2->reg;
                            cur_arr_base = my_arr_base[Read_I->from];
                            auto tp = make_vreg(vreg_count++);

                            auto bi_lsl = new MI_Binary;
                            bi_lsl->op = BINARY_LSL;
                            bi_lsl->lhs = make_operand(Read_I->from, mb, true);
                            bi_lsl->rhs = make_imm(2);
                            bi_lsl->dst = make_operand(Read_I->from, mb, true);
                            push((MI *)bi_lsl, mb);
                            auto bi_add = new MI_Binary;
                            bi_add->op = BINARY_ADD;
                            bi_add->lhs = make_operand(cur_arr_base, mb);
                            bi_add->rhs = make_operand(Read_I->from, mb, true);
                            bi_add->dst = tp;
                            push((MI *)bi_add, mb);
                            ldr2->base = tp;
                            // ldr2->offset.tag = IMM;
                            // ldr2->offset.value = 0;
                            cur_arr_base = NULL;
                        }
                    }
                    else
                    {
                        cout << "loadd reg" << endl;
                        int flag = 0;
                        for (auto v : globalValues)
                        {
                            if (v == Read_I->from)
                            {
                                flag = 1;
                                break;
                            }
                        }
                        if (stack_val_map.find(Read_I->from) != stack_val_map.end())
                            flag = 2;
                        if (flag == 0)
                        {
                            cout << Read_I->to->getStr() << endl;
                            ldr2->reg = make_vreg(vreg_count++);
                            value_map[Read_I->to] = ldr2->reg;
                            ldr2->base = make_operand(Read_I->from, mb);
                            emit_move(ldr2->reg, ldr2->base, mb);
                            break;
                            // ldr->base.tag=IMM;
                            int_val_map[ldr2->reg] = int_val_map[ldr2->base];
                        }
                        else if (flag == 1)
                        {
                            // ldr->base = make_operand(Read_I->from, mb);
                            auto nvreg = make_vreg(vreg_count++);
                            emit_load_of_global_ref(func_asm, Read_I->from, nvreg, mb);
                            ldr2->base = nvreg;
                            ldr2->reg = make_operand(Read_I->to, mb);
                            value_map[Read_I->to] = ldr2->reg;
                            int_val_map[ldr2->reg] = int_val_map[ldr2->base];
                        }
                        else
                        {
                            cout << Read_I->to->getStr() << endl;
                            ldr2->reg = make_vreg(vreg_count++);
                            value_map[Read_I->to] = ldr2->reg;
                            ldr2->base = get_stack_val(Read_I->from, mb, func_asm);
                            // ldr->base.tag=IMM;
                            int_val_map[ldr2->reg] = int_val_map[ldr2->base];
                        }
                    }
                    cout << "my vload!!" << endl;
                    push((MI *)ldr2, mb);
                    break;
                }
                if (my_arr_base.find(Read_I->from) != my_arr_base.end())
                {
                    if (ptr_val_map.find(Read_I->from) != ptr_val_map.end())
                    {
                        ldr->reg = make_operand(Read_I->to, mb, true);
                        cur_arr_base = my_arr_base[Read_I->from];
                        ldr->base = make_operand(cur_arr_base, mb);
                        auto opr = ptr_val_map.find(Read_I->from);
                        ldr->offset.tag = IMM;
                        ldr->offset.value = opr->second * 4;
                        cur_arr_base = NULL;
                    }
                    else
                    {
                        ldr->reg = make_operand(Read_I->to, mb, true);
                        cur_arr_base = my_arr_base[Read_I->from];
                        ldr->base = make_operand(cur_arr_base, mb);
                        ldr->offset = make_operand(Read_I->from, mb, true);
                        ldr->offset.s_tag = LSL;
                        ldr->offset.s_value = 2;
                        cur_arr_base = NULL;
                    }
                }
                else
                {
                    cout << "loadd reg" << endl;
                    int flag = 0;
                    for (auto v : globalValues)
                    {
                        if (v == Read_I->from)
                        {
                            flag = 1;
                            break;
                        }
                    }
                    if (stack_val_map.find(Read_I->from) != stack_val_map.end())
                        flag = 2;
                    if (flag == 0)
                    {
                        cout << Read_I->to->getStr() << endl;
                        ldr->reg = make_vreg(vreg_count++);
                        value_map[Read_I->to] = ldr->reg;
                        ldr->base = make_operand(Read_I->from, mb);
                        emit_move(ldr->reg, ldr->base, mb);
                        break;
                        // ldr->base.tag=IMM;
                        int_val_map[ldr->reg] = int_val_map[ldr->base];
                    }
                    else if (flag == 1)
                    {
                        // ldr->base = make_operand(Read_I->from, mb);
                        auto nvreg = make_vreg(vreg_count++);
                        emit_load_of_global_ref(func_asm, Read_I->from, nvreg, mb);
                        ldr->base = nvreg;
                        ldr->reg = make_operand(Read_I->to, mb);
                        value_map[Read_I->to] = ldr->reg;
                        int_val_map[ldr->reg] = int_val_map[ldr->base];
                    }
                    else
                    {
                        cout << Read_I->to->getStr() << endl;
                        ldr->reg = make_vreg(vreg_count++);
                        value_map[Read_I->to] = ldr->reg;
                        ldr->base = get_stack_val(Read_I->from, mb, func_asm);
                        // ldr->base.tag=IMM;
                        int_val_map[ldr->reg] = int_val_map[ldr->base];
                    }
                }
                push((MI *)ldr, mb);
            }
            break;

            // store instruction
            // 1: array type, has const offset
            // 2: array type, no const offset
            // 3: reg, simple
            // 4: reg, global
            // 5: reg, store in stack
            case Store:
            {

                auto Write_I = dynamic_cast<StoreInstruction *>(I.get());
                auto str = new MI_Store;

                if (Write_I->des->type->isFloat())
                {
                    auto vstr = new MI_VStore;
                    if (my_arr_base.find(Write_I->des) != my_arr_base.end())
                    {
                        if (ptr_val_map.find(Write_I->des) != ptr_val_map.end())
                        {
                            auto tp = make_operand(Write_I->value, mb, true);
                            vstr->reg = make_vsreg(vsreg_count++);
                            emit_vmove(vstr->reg, tp, mb);
                            auto mi = new MI_VCvt;
                            mi->to_type = F32;
                            mi->from_type = S32;
                            mi->dst = vstr->reg;
                            mi->src = vstr->reg;
                            // if (mydes.tag != VSREG && mydes.tag != SREG)
                            if(!Write_I->value->type->isFloat())
                            push((MI *)mi, mb);
                            // emit_vmove(arg_operand, tpsreg, mb);
                            cur_arr_base = my_arr_base[Write_I->des];
                            // cout<<Write_I->des->name<<" "<<Write_I->des->type->getID()<<endl;
                            vstr->base = make_operand(cur_arr_base, mb);
                            // cout<<Write_I->value->name<<" "<<Write_I->des->type->getID()<<endl;
                            auto opr = ptr_val_map.find(Write_I->des);
                            vstr->offset.tag = IMM;
                            vstr->offset.value = opr->second * 4;
                            cur_arr_base = NULL;
                        }
                        else
                        {
                            vstr->reg = make_operand(Write_I->value, mb, true);
                            cur_arr_base = my_arr_base[Write_I->des];
                            // cout<<Write_I->des->name<<" "<<Write_I->des->type->getID()<<endl;
                            auto tp = make_vreg(vreg_count++);

                            auto bi_lsl = new MI_Binary;
                            bi_lsl->op = BINARY_LSL;
                            bi_lsl->lhs = make_operand(Write_I->des, mb, true);
                            bi_lsl->rhs = make_imm(2);
                            bi_lsl->dst = make_operand(Write_I->des, mb, true);
                            push((MI *)bi_lsl, mb);
                            auto bi_add = new MI_Binary;
                            bi_add->op = BINARY_ADD;
                            bi_add->lhs = make_operand(cur_arr_base, mb);
                            bi_add->rhs = make_operand(Write_I->des, mb, true);
                            bi_add->dst = tp;
                            push((MI *)bi_add, mb);
                            vstr->base = tp;
                            // vstr->offset.tag=IMM;
                            // vstr->offset.value = 0;
                            cur_arr_base = NULL;
                        }
                    }
                    else
                    {
                        int flag = 0;
                        for (auto v : globalValues)
                        {
                            if (v == Write_I->des)
                            {
                                flag = 1;
                                break;
                            }
                        }
                        if (stack_val_map.find(Write_I->des) != stack_val_map.end())
                            flag = 2;
                        if (flag == 0)
                        {
                            cout << Write_I->des->getStr() << endl;
                            vstr->base = make_operand(Write_I->des, mb, true);
                            // value_map[Write_I->des] = vstr->base;
                            cout << Write_I->value->getStr() << endl;
                            // vstr->offset= make_imm(0);
                            // vstr->base.tag=IMM;
                            vstr->reg = make_operand(Write_I->value, mb, true);
                            emit_move(vstr->base, vstr->reg, mb);
                            int_val_map[vstr->base] = int_val_map[vstr->reg];
                        }
                        else if (flag == 1)
                        {
                            vstr->reg = make_operand(Write_I->value, mb, true);

                            auto nvreg = make_vreg(vreg_count++);
                            emit_load_of_global_ref(func_asm, Write_I->des, nvreg, mb);
                            vstr->base = nvreg;
                            value_map[Write_I->des] = vstr->base;
                            int_val_map[vstr->base] = int_val_map[vstr->reg];

                            // vstr->base.tag=IMM;
                        }
                        else
                        {
                            cout << Write_I->des->getStr() << endl;
                            vstr->base = get_stack_val(Write_I->des, mb, func_asm);
                            // value_map[Write_I->des] = vstr->base;
                            cout << Write_I->value->getStr() << endl;
                            // vstr->offset= make_imm(0);
                            // vstr->base.tag=IMM;
                            vstr->reg = make_operand(Write_I->value, mb, true);
                            int_val_map[vstr->base] = int_val_map[vstr->reg];
                        }

                        // vstr->base.tag=3;
                    }

                    push((MI *)vstr, mb);
                    break;
                }

                if (my_arr_base.find(Write_I->des) != my_arr_base.end())
                {
                    if (ptr_val_map.find(Write_I->des) != ptr_val_map.end())
                    {
                        str->reg = make_operand(Write_I->value, mb, true);
                        cur_arr_base = my_arr_base[Write_I->des];
                        // cout<<Write_I->des->name<<" "<<Write_I->des->type->getID()<<endl;
                        str->base = make_operand(cur_arr_base, mb);
                        // cout<<Write_I->value->name<<" "<<Write_I->des->type->getID()<<endl;
                        auto opr = ptr_val_map.find(Write_I->des);
                        str->offset.tag = IMM;
                        str->offset.value = opr->second * 4;
                        cur_arr_base = NULL;
                    }
                    else
                    {
                        str->reg = make_operand(Write_I->value, mb, true);
                        cur_arr_base = my_arr_base[Write_I->des];
                        // cout<<Write_I->des->name<<" "<<Write_I->des->type->getID()<<endl;
                        str->base = make_operand(cur_arr_base, mb);
                        str->offset = make_operand(Write_I->des, mb, true);
                        str->offset.s_tag = LSL;
                        str->offset.s_value = 2;
                        cur_arr_base = NULL;
                    }
                }
                else
                {
                    int flag = 0;
                    for (auto v : globalValues)
                    {
                        if (v == Write_I->des)
                        {
                            flag = 1;
                            break;
                        }
                    }
                    if (stack_val_map.find(Write_I->des) != stack_val_map.end())
                        flag = 2;
                    if (flag == 0)
                    {
                        cout << Write_I->des->getStr() << endl;
                        str->base = make_operand(Write_I->des, mb, true);
                        // value_map[Write_I->des] = str->base;
                        cout << Write_I->value->getStr() << endl;
                        // str->offset= make_imm(0);
                        // str->base.tag=IMM;
                        str->reg = make_operand(Write_I->value, mb, true);
                        emit_move(str->base, str->reg, mb);
                        int_val_map[str->base] = int_val_map[str->reg];
                    }
                    else if (flag == 1)
                    {
                        str->reg = make_operand(Write_I->value, mb, true);

                        auto nvreg = make_vreg(vreg_count++);
                        emit_load_of_global_ref(func_asm, Write_I->des, nvreg, mb);
                        str->base = nvreg;
                        value_map[Write_I->des] = str->base;
                        int_val_map[str->base] = int_val_map[str->reg];

                        // str->base.tag=IMM;
                    }
                    else
                    {
                        cout << Write_I->des->getStr() << endl;
                        str->base = get_stack_val(Write_I->des, mb, func_asm);
                        // value_map[Write_I->des] = str->base;
                        cout << Write_I->value->getStr() << endl;
                        // str->offset= make_imm(0);
                        // str->base.tag=IMM;
                        str->reg = make_operand(Write_I->value, mb, true);
                        int_val_map[str->base] = int_val_map[str->reg];
                    }

                    // str->base.tag=3;
                }

                push((MI *)str, mb);
            }
            break;
            default:
            {
                // exit(51);
                // assert(false && "translating unknown IR instructions to assembly");
            }
            break;
            }
        }
    }

    cout << "<<<<< PHI" << endl;
    // @todo handle the phi instrutions
    ii = 0;
    for (auto bb : mybb->basicBlocks)
    {
        auto mb = func_asm->mbs[ii];
        ii++;
        for (int i = 0; i < bb->instructions.size(); i++)
        {
            if (bb->instructions[i]->type == Phi)
            {
                auto phi = dynamic_cast<PhiInstruction *>(bb->instructions[i].get());
                if (phi->reg->type->isFloat())
                {
                    auto incoming = make_vsreg(vsreg_count++);
                    // auto resreg=make_vsreg(vsreg_count++);
                    // value_map[phi->reg]=resreg;

                    auto phi_as_operand = make_operand(phi->reg, mb);
                    auto mv = emit_vmove(phi_as_operand, incoming);
                    insert((MI *)mv, mb->inst);

                    cout << "loop2" << endl;

                    for (int p = 0; p < phi->from.size(); p++)
                    {
                        auto phi_opr = phi->from[p];
                        auto pred_bb = phi_opr.second;
                        int idx = 0;
                        for (auto bb : mybb->basicBlocks)
                        {
                            if (bb == pred_bb)
                                break;
                            idx++;
                        }
                        auto pred_mb = func_asm->mbs[idx];

                        cout << "phi name is" << phi_opr.first->getStr() << endl;

                        auto mv = emit_vmove(incoming, make_operand(phi_opr.first, pred_mb));
                        cout << "loop3" << endl;
                        if (pred_mb->control_transfer_inst)
                        {
                            insert((MI *)mv, pred_mb->control_transfer_inst);
                        }
                        else
                        {
                            push((MI *)mv, pred_mb);
                        }
                    }
                    continue;
                }
                auto incoming = make_vreg(vreg_count++);

                auto phi_as_operand = make_operand(phi->reg, mb);
                auto mv = emit_move(phi_as_operand, incoming);
                insert((MI *)mv, mb->inst);

                cout << "loop2" << endl;

                for (int p = 0; p < phi->from.size(); p++)
                {
                    auto phi_opr = phi->from[p];
                    auto pred_bb = phi_opr.second;
                    int idx = 0;
                    for (auto bb : mybb->basicBlocks)
                    {
                        if (bb == pred_bb)
                            break;
                        idx++;
                    }
                    auto pred_mb = func_asm->mbs[idx];

                    auto mv = emit_move(incoming, make_operand(phi_opr.first, pred_mb));
                    cout << "loop3" << endl;
                    if (pred_mb->control_transfer_inst)
                    {
                        insert((MI *)mv, pred_mb->control_transfer_inst);
                    }
                    else
                    {
                        push((MI *)mv, pred_mb);
                    }
                }
            }
        }
    }
    cout << vreg_count << endl;
    func_asm->vreg_count = vreg_count;
    func_asm->vsreg_count = vsreg_count;
    return func_asm;
}

void build_cvt_type(String_Builder *s, Vcvt_Type t)
{
    switch (t)
    {
    case U32:
    {
        s->append(".u32");
    }
    break;
    case S32:
    {
        s->append(".s32");
    }
    break;
    case F32:
    {
        s->append(".f32");
    }
    break;
    default:
    {
        assert(false);
    }
    }
}

void print_operand(MOperand op)
{
    String_Builder s;
    build_operand(&s, op);
    printf("%s", s.c_str());
}

void build_operand(String_Builder *s, MOperand op)
{

    // cout<<op.tag<<endl;
    switch (op.tag)
    {

    case REG:
    {
        if (op.value == fp)
            s->append("fp");
        else if (op.value == sp)
            s->append("sp");
        else if (op.value == lr)
            s->append("lr");
        else if (op.value == pc)
            s->append("pc");
        else
            s->append("r%d", op.value);
    }
    break;

    case VREG:
    {
        s->append("vr%d", op.value);
    }
    break;
    case IMM:
    {
        s->append("#%d", op.value);
    }
    break;
    case ADR_GLOBAL:
    {
        s->append("%s", op.adr);
    }
    break;
    case SREG:
    {
        s->append("s%d", op.value);
    }
    break;
    case VSREG:
    {
        s->append("vsr%d", op.value);
    }
    break;
    case SIMM:
    {
        cout << "still error" << endl;
    }
    break;
    case ERRORTYPE:
    {
        // exit(52); assert(false);
    }
    break;
    }
    switch (op.s_tag)
    {
    case Nothing:
        break;
    case LSL:
    {
        s->append(", lsl #%d", op.s_value);
    }
    break;
    case LSR:
    {
        s->append(", lsr #%d", op.s_value);
    }
    break;
    case ASL:
    {
        s->append(", asl #%d", op.s_value);
    }
    break;
    case ASR:
    {
        s->append(", asr #%d", op.s_value);
    }
    break;
    }
}

void print_function_asm(Func_Asm *func)
{
    String_Builder s;
    build_function_asm(&s, func);
    printf("%s", s.c_str());
}

const char *get_branch_suffix(Branch_Condition condition)
{
    switch (condition)
    {
    case NO_CONDITION:
    {
        return "";
    }
    break;
    case LESS_THAN:
    {
        return "lt";
    }
    break;
    case GREATER_THAN:
    {
        return "gt";
    }
    break;
    case LESS_THAN_OR_EQUAL:
    {
        return "le";
    }
    break;
    case GREATER_THAN_OR_EQUAL:
    {
        return "ge";
    }
    break;
    case NOT_EQUAL:
    {
        return "ne";
    }
    break;
    case EQUAL:
    {
        return "eq";
    }
    break;
    case float_ge:
    {
        return "pl";
    }
    break;
    default:
    {
        assert(false);
        return "";
    }
    }
}

Branch_Condition invert_branch_cond(Branch_Condition cond)
{
    if (cond == NO_CONDITION || cond >= 7)
        return cond;
    if (cond >= 1 && cond <= 3)
    {
        return (Branch_Condition)(cond + 3);
    }
    else if (cond >= 4 && cond <= 6)
    {
        return (Branch_Condition)(cond - 3);
    }
    assert(false);
    return NO_CONDITION;
}

bool can_be_imm_ror(int32 x)
{
    uint32 v = x;
    for (int r = 0; r < 31; r += 2)
    {
        if ((v & 0xff) == v)
        {
            return true;
        }

        v = (v >> 2) | (v << 30);
    }
    return false;
}

void build_function_asm(String_Builder *s, Func_Asm *func)
{
    // cout<<111<<endl;
    auto ss = func->mbs.len;
    // cout<<ss<<endl;
    s->append("%s:\n", func->name.c_str());
    cout << func->name << endl;
    // cout<<func->mbs.len<<endl;
    for (int i = 0; i < func->mbs.len; i++)
    {
        // cout<<i<<endl;
        Machine_Block *next_bb = NULL;
        if (i != func->mbs.len - 1)
            next_bb = func->mbs[i + 1];
        // if(func->name=="main")
        //     cout<<3223<<endl;
        s->append(".L%d_%d:\n", func->index, func->mbs[i]->i);
        for (auto I = func->mbs[i]->inst; I; I = I->next)
        {
            // if(func->name=="main")
            //     cout<<3223<<endl;
            // cout<<I->tag<<endl;

            s->append("    ");
            const char *cond = get_branch_suffix(I->cond);
            const char *set_flags = I->update_flags ? "s" : "";
            // cout<<I->tag<<endl;
            switch (I->tag)
            {
            case MI_MOVE:
            {
                auto mv = (MI_Move *)I;
                if (mv->neg)
                {
                    s->append("mvn%s%s ", set_flags, cond);
                }
                else
                {
                    s->append("mov%s%s ", set_flags, cond);
                }
                build_operand(s, mv->dst);
                s->append(", ");
                build_operand(s, mv->src);
            }
            break;

            case MI_CLZ:
            {
                auto clz = (MI_Clz *)I;
                s->append("clz%s%s ", set_flags, cond);
                build_operand(s, clz->dst);
                s->append(", ");
                build_operand(s, clz->operand);
            }
            break;

            case MI_BINARY:
            {
                auto bi = (MI_Binary *)I;

                switch (bi->op)
                {
                case BINARY_ADD:
                {
                    s->append("add");
                }
                break;
                case BINARY_SUBTRACT:
                {
                    s->append("sub");
                }
                break;
                case BINARY_MULTIPLY:
                {
                    s->append("mul");
                }
                break;
                case BINARY_DIVIDE:
                {
                    s->append("sdiv");
                }
                break;
                case BINARY_LSL:
                {
                    s->append("lsl");
                }
                break;
                case BINARY_LSR:
                {
                    s->append("lsr");
                }
                break;
                case BINARY_ASL:
                {
                    s->append("asl");
                }
                break;
                case BINARY_ASR:
                {
                    s->append("asr");
                }
                break;
                case BINARY_RSB:
                {
                    s->append("rsb");
                }
                break;
                case BINARY_BITWISE_AND:
                {
                    s->append("and");
                }
                break;
                case BINARY_BITWISE_OR:
                {
                    s->append("orr");
                }
                break;
                case BINARY_BIC:
                {
                    s->append("bic");
                }
                break;
                case BINARY_SMMUL:
                {
                    s->append("smmul");
                }
                break;
                default:
                {
                    exit(53);
                    assert(false && "unknown binary asm instruction.");
                }
                }
                s->append("%s%s", set_flags, cond);
                s->append(" ");
                build_operand(s, bi->dst);
                s->append(", ");
                build_operand(s, bi->lhs);
                s->append(", ");
                build_operand(s, bi->rhs);
            }
            break;

            case MI_COMPARE:
            {
                auto cmp = (MI_Compare *)I;

                if (cmp->neg)
                {
                    s->append("cmn ");
                }
                else
                {
                    s->append("cmp ");
                }
                build_operand(s, cmp->lhs);
                s->append(", ");
                build_operand(s, cmp->rhs);
            }
            break;

            case MI_BRANCH:
            {
                auto br = (MI_Branch *)I;
                if (next_bb && next_bb->condified)
                    break;
                if (br->cond == NO_CONDITION)
                {
                    // cout<<"!!!"<<endl;
                    if (br->true_target->i != i + 1)
                    {
                        s->append("b .L%d_%d", func->index, br->true_target->i);
                    }
                }
                else if (br->true_target->i == i + 1)
                {
                    // invert the branch cond
                    s->append("b%s .L%d_%d\n", get_branch_suffix(invert_branch_cond(br->cond)), func->index, br->false_target->i);
                }
                else if (br->false_target->i == i + 1)
                {
                    s->append("b%s .L%d_%d\n", get_branch_suffix(br->cond), func->index, br->true_target->i);
                }
                else
                {
                    s->append("b%s .L%d_%d\n", get_branch_suffix(br->cond), func->index, br->true_target->i);
                    s->append("    ");
                    s->append("b .L%d_%d", func->index, br->false_target->i);
                }
            }
            break;

            case MI_PUSH:
            case MI_POP:
            {
                auto push_or_pop = (MI_Push *)I;

                s->append((I->tag == MI_PUSH) ? "push {" : "pop {");
                for (int i = 0; i < push_or_pop->operands.len; i++)
                {
                    build_operand(s, push_or_pop->operands[i]);
                    if (i != push_or_pop->operands.len - 1)
                        s->append(", ");
                }
                s->append("}");
            }
            break;

            case MI_VPUSH:
            case MI_VPOP:
            {
                // auto push_or_pop = (MI_VPush *)I;

                // s->append((I->tag == MI_VPUSH) ? "vpush {" : "vpop {");
                // for (int i = 0; i < push_or_pop->operands.len; i++)
                // {
                //     build_operand(s, push_or_pop->operands[i]);
                //     if (i != push_or_pop->operands.len - 1)
                //         s->append(", ");
                // }
                // s->append("}");
            }
            break;

            case MI_FUNC_CALL:
            {
                auto call = (MI_Func_Call *)I;
                s->append("bl%s %s", cond, call->func_name);
            }
            break;

            case MI_LOAD:
            case MI_STORE:
            {
                auto load_or_store = (MI_Load *)I;

                // cout<<"store!"<<endl;

                // cout<<load_or_store->base.tag<<endl;

                if (load_or_store->base.tag == ADR_GLOBAL)
                {
                    s->append("movw%s ", cond);
                    build_operand(s, load_or_store->reg);
                    s->append(", ");
                    s->append(":lower16:");
                    build_operand(s, load_or_store->base);
                    s->append("\n    movt%s ", cond);
                    build_operand(s, load_or_store->reg);
                    s->append(", ");
                    s->append(":upper16:");
                    build_operand(s, load_or_store->base);
                }
                else if (load_or_store->base.tag == IMM)
                {
                    auto val = load_or_store->base.value;
                    // cout<<"base.val "<<val<<endl;
                    if (can_be_imm_ror(val))
                    {
                        s->append("mov%s ", cond);
                        build_operand(s, load_or_store->reg);
                        s->append(", ");
                        build_operand(s, load_or_store->base);
                    }
                    else if ((val & 0xFFFF) == val)
                    {
                        s->append("movw%s ", cond);
                        build_operand(s, load_or_store->reg);
                        s->append(", ");
                        build_operand(s, load_or_store->base);
                    }
                    else if (val < 0 && val > -258)
                    {
                        s->append("mvn%s ", cond);
                        build_operand(s, load_or_store->reg);
                        s->append(", ");
                        auto valn = -val - 1;
                        MOperand valn_imm(IMM, valn);
                        build_operand(s, valn_imm);
                    }
                    else
                    {
                        auto vall = val & 0xFFFF;
                        auto valh = (val >> 16) & 0xFFFF;
                        MOperand vall_imm(IMM, vall);
                        MOperand valh_imm(IMM, valh);
                        s->append("movw%s ", cond);
                        build_operand(s, load_or_store->reg);
                        s->append(", ");
                        build_operand(s, vall_imm);
                        s->append("\n    movt%s ", cond);
                        build_operand(s, load_or_store->reg);
                        s->append(", ");
                        build_operand(s, valh_imm);
                    }
                }
                else
                {

                    s->append(I->tag == MI_LOAD ? "ldr" : "str");
                    s->append(cond);
                    s->append(" ");
                    build_operand(s, load_or_store->reg);
                    s->append(", [");
                    build_operand(s, load_or_store->base);
                    if (load_or_store->offset.tag != ERRORTYPE)
                    {
                        s->append(", ");
                        build_operand(s, load_or_store->offset);
                    }
                    s->append("]");
                }
            }
            break;

            case MI_RETURN:
            {
                s->append("bx lr");
            }
            break;

            case MI_VMOVE:
            {
                auto vmv = (MI_VMove *)I;
                if (vmv->both_float)
                {
                    s->append("vmov.f32%s%s ", set_flags, cond);
                }
                else
                {
                    s->append("vmov%s%s ", set_flags, cond);
                }
                build_operand(s, vmv->dst);
                s->append(", ");
                build_operand(s, vmv->src);
            }
            break;

            case MI_VCVT:
            {
                auto cvt = (MI_VCvt *)I;
                s->append("vcvt%s%s", set_flags, cond);
                build_cvt_type(s, cvt->to_type);
                build_cvt_type(s, cvt->from_type);
                s->append(" ");
                build_operand(s, cvt->dst);
                s->append(", ");
                build_operand(s, cvt->src);
            }
            break;

            case MI_VLOAD:
            case MI_VSTORE:
            {
                cout << "vldr!!!" << endl;
                auto load_or_store = (MI_VLoad *)I;

                if (load_or_store->base.tag == ADR_GLOBAL)
                {
                    s->append("movw%s ", cond);
                    build_operand(s, load_or_store->reg);
                    s->append(", ");
                    s->append(":lower16:");
                    build_operand(s, load_or_store->base);
                    s->append("\n    movt%s ", cond);
                    build_operand(s, load_or_store->reg);
                    s->append(", ");
                    s->append(":upper16:");
                    build_operand(s, load_or_store->base);
                } // todo: SIMM to add
                else if (load_or_store->base.tag == SIMM)
                {
                    auto val = load_or_store->base.value;
                    if (can_be_imm_ror(val))
                    {
                        s->append("mov%s ", cond);
                        build_operand(s, load_or_store->reg);
                        s->append(", ");
                        build_operand(s, load_or_store->base);
                    }
                    else if ((val & 0xFFFF) == val)
                    {
                        s->append("movw%s ", cond);
                        build_operand(s, load_or_store->reg);
                        s->append(", ");
                        build_operand(s, load_or_store->base);
                    }
                    else if (val < 0 && val > -258)
                    {
                        s->append("mvn%s ", cond);
                        build_operand(s, load_or_store->reg);
                        s->append(", ");
                        auto valn = -val - 1;
                        MOperand valn_imm(IMM, valn);
                        build_operand(s, valn_imm);
                    }
                    else
                    {
                        auto vall = val & 0xFFFF;
                        auto valh = (val >> 16) & 0xFFFF;
                        MOperand vall_imm(IMM, vall);
                        MOperand valh_imm(IMM, valh);
                        s->append("movw%s ", cond);
                        build_operand(s, load_or_store->reg);
                        s->append(", ");
                        build_operand(s, vall_imm);
                        s->append("\n    movt%s ", cond);
                        build_operand(s, load_or_store->reg);
                        s->append(", ");
                        build_operand(s, valh_imm);
                    }
                }
                else
                {

                    s->append(I->tag == MI_VLOAD ? "vldr.32" : "vstr.32");
                    s->append(cond);
                    s->append(" ");
                    build_operand(s, load_or_store->reg);
                    s->append(", [");
                    build_operand(s, load_or_store->base);
                    if (load_or_store->offset.tag != ERRORTYPE)
                    {
                        s->append(", ");
                        build_operand(s, load_or_store->offset);
                    }
                    s->append("]");
                }
            }
            break;

            case MI_VBINARY:
            {
                auto bi = (MI_VBinary *)I;

                switch (bi->op)
                {
                case BINARY_ADD:
                {
                    s->append("vadd.f32");
                }
                break;
                case BINARY_SUBTRACT:
                {
                    s->append("vsub.f32");
                }
                break;
                case BINARY_MULTIPLY:
                {
                    s->append("vmul.f32");
                }
                break;
                case BINARY_DIVIDE:
                {
                    s->append("vdiv.f32");
                }
                break;
                case F_NEG:
                {
                    s->append("vneg.f32");
                }
                break;
                default:
                {
                    exit(53);
                    assert(false && "unknown binary asm instruction.");
                }
                }
                s->append("%s%s", set_flags, cond);
                s->append(" ");
                build_operand(s, bi->dst);
                s->append(", ");
                build_operand(s, bi->lhs);
                if (bi->fneg == 1)
                {
                    break;
                }
                s->append(", ");
                build_operand(s, bi->rhs);
            }
            break;

            // todo : fix it for case 95
            case MI_VCOMPARE:
            {
                auto Vcmp = (MI_VCompare *)I;

                s->append("vcmpe.f32 ");
                build_operand(s, Vcmp->lhs);
                s->append(", ");
                build_operand(s, Vcmp->rhs);
                s->append("\n");
                s->append("    ");
                s->append("vmrs	APSR_nzcv, FPSCR");
            }
            break;
            default:
            {
                cout << "???nono" << endl;
            }
            break;
                // default: { exit(54); assert(false && "printing unknown assembly instruction."); } break;
            }
            s->append("\n");
        }
    }
}

void get_init_sequence(VariablePtr globalarr, vector<Pair<string, int>> &init_inst)
{
    if (globalarr->type->isInt())
    {
        int val_use = dynamic_cast<Int *>(globalarr.get())->intVal;
        if (val_use)
        {
            init_inst.emplace_back(make_pair(".word", val_use));
        }
        else
        {
            if (init_inst.empty() || init_inst.back().first == ".word")
            {
                init_inst.emplace_back(make_pair(".zero", 0));
            }
            init_inst.back().second += 4;
        }
        return;
    }
    else if (globalarr->type->isFloat())
    {
        f2i.f_value = dynamic_cast<Float *>(globalarr.get())->floatVal;
        init_inst.emplace_back(make_pair(".word", (f2i.bin_value)));
        return;
    }
    if (globalarr->zero())
    {
        if (init_inst.empty() || init_inst.back().first == ".word")
        {
            init_inst.emplace_back(make_pair(".zero", 0));
        }
        int zero_size = dynamic_cast<ArrType *>((globalarr->type).get())->getSize();
        init_inst.back().second += zero_size * 4;
        return;
    }
    else
    {
        auto arr_use = dynamic_cast<Arr *>(globalarr.get());
        int i = 0;
        for (; i < arr_use->inner.size(); i++)
        {
            get_init_sequence(arr_use->inner[i], init_inst);
        }
        if (arr_use->getElementType()->isArr())
        {
            for (; i < arr_use->getElementLength(); i++)
            {
                get_init_sequence(arr_use->inner[i], init_inst);
            }
        }
        else if (arr_use->getElementType()->isInt())
        {
            if (i != arr_use->getElementLength())
            {
                if (init_inst.empty() || init_inst.back().first == ".word")
                {
                    init_inst.emplace_back(make_pair(".zero", 0));
                }
                init_inst.back().second += (arr_use->getElementLength() - i) * 4;
            }
        }
        else if (arr_use->getElementType()->isFloat())
        {
            if (i != arr_use->getElementLength())
            {
                if (init_inst.empty() || init_inst.back().first == ".word")
                {
                    init_inst.emplace_back(make_pair(".zero", 0));
                }
                init_inst.back().second += (arr_use->getElementLength() - i) * 4;
            }
        }
    }
}

void build_globals(String_Builder *s, vector<VariablePtr> &globals)
{
    if (globals.size() == 0)
        return;
    s->append(".data\n.align 2\n");
    for (int i = 0; i < globals.size(); i++)
    {
        if (globals[i]->type->isInt())
        {
            s->append("%s:\n", globals[i]->name.c_str());
            s->append("    ");
            int32 val = dynamic_cast<Int *>(globals[i].get())->intVal;
            s->append(".word %d\n", val);
        }
        else if (globals[i]->type->isFloat())
        {
            s->append("%s:\n", globals[i]->name.c_str());
            s->append("    ");
            f2i.f_value = dynamic_cast<Float *>(globals[i].get())->floatVal;
            s->append(".word %d\n", f2i.bin_value);
        }
        else if (globals[i]->type->isArr())
        {
            // int complete?
            vector<Pair<string, int>> init_inst;
            auto globalarr = dynamic_cast<Arr *>(globals[i].get());
            if (globalarr->zero())
            {
                continue;
            }
            else
            {
                get_init_sequence(globals[i], init_inst);
                s->append("%s:\n", globals[i]->name.c_str());
                for (int j = 0; j < init_inst.size(); j++)
                {
                    s->append("    ");
                    s->append("%s  %d\n", init_inst[j].first.c_str(), init_inst[j].second);
                }
            }
        }
        else
        {
            printf("globals error type\n");
        }
    }
    s->append(".bss\n");
    s->append(".align 2\n");
    for (int i = 0; i < globals.size(); i++)
    {
        if (globals[i]->type->isArr())
        {
            auto globalarr = dynamic_cast<Arr *>(globals[i].get());
            if (globalarr->zero())
            {
                int zero_size = dynamic_cast<ArrType *>((globalarr->type).get())->getSize();
                zero_size *= 4;
                s->append("%s:\n", globals[i]->name.c_str());
                s->append("    .space %d\n", zero_size);
                s->append("    .type %s, %%object\n", globals[i]->name.c_str());
                s->append("    .size %s, %d\n", globals[i]->name.c_str(), zero_size);
            }
        }
    }
}

void print_program_asm(Program_Asm *pro, vector<VariablePtr> &globalVariables)
{
    String_Builder s;
    build_program_asm(&s, pro, globalVariables);
    printf("%s", s.c_str());
}

void build_program_asm(String_Builder *s, Program_Asm *pro, vector<VariablePtr> &globalVariables)
{
    // cout<<"program"<<endl;
    s->append(".arch armv8-a\n");
    // s->append(".arch_extension crc\n");

    build_globals(s, globalVariables);
    // cout<<"nono"<<endl;
    s->append("\n");

    s->append(".text\n");
    s->append(".align 2\n");
    s->append(".syntax unified\n");
    s->append(".arm\n");
    s->append(".fpu neon\n");

    s->append(".global main\n\n");
    // cout<<pro->functions.len<<endl;
    for (int i = 0; i < pro->functions.len; i++)
    {
        // cout<<i<<" sdf "<<endl;
        build_function_asm(s, pro->functions[i]);
        // cout<<"nono"<<endl;
        s->append("\n\n");
    }
    // cout<<"nono"<<endl;
}

Program_Asm *emit_asm(Module program_module)
{
    auto program_asm = new Program_Asm;

    // add some map
    Binary_ir2asm["+"] = BINARY_ADD;
    Binary_ir2asm["-"] = BINARY_SUBTRACT;
    Binary_ir2asm["*"] = BINARY_MULTIPLY;
    Binary_ir2asm["/"] = BINARY_DIVIDE;
    Binary_ir2asm["%"] = BINARY_MOD;
    Binary_ir2asm["!="] = BINARY_NOT_EQUAL_TO;
    Binary_ir2asm["!"] = UNARY_XOR;
    Binary_ir2asm[">"] = BINARY_GREATER_THAN;
    Binary_ir2asm[">="] = BINARY_GREATER_THAN_OR_EQUAL_TO;
    Binary_ir2asm["<"] = BINARY_LESS_THAN;
    Binary_ir2asm["<="] = BINARY_LESS_THAN_OR_EQUAL_TO;
    Binary_ir2asm["=="] = BINARY_EQUAL_TO;
    // do only one for now
    // program_asm->functions.push(emit_function_asm(program_IR->procedures[0], 0));
    int i = 0;
    for (auto func : program_module.globalFunctions)
    {
        if (func->isLib)
            continue;
        i++;
        // cout<<func->name<<endl;
        auto func_asm = emit_function_asm(func, i, program_module, program_module.globalVariables);
        program_asm->functions.push(func_asm);
    }
    print_program_asm(program_asm, program_module.globalVariables);

    return program_asm;
}

bool is_callee_save(uint8 reg)
{
    return (reg >= r4 && reg <= r11) || reg == lr;
}

bool s_is_callee_save(uint8 reg)
{
    return (reg >= s1 && reg <= s11);
}

bool is_caller_save(uint8 reg)
{
    return (reg >= r0 && reg <= r3) || reg == r12;
}

bool s_is_caller_save(uint8 reg)
{
    return (reg >= s0 && reg <= s3) || reg == s12;
}

bool can_be_imm12(int32 x)
{
    return (x >= -4095) && (x <= 4095);
}

void Machine_Block::erase_marked_values()
{
    static Array<MI *> unmarked_values;
    unmarked_values.len = 0;

    for (MI *I = inst; I; I = I->next)
    {
        if (!I->marked)
        {
            unmarked_values.push(I);
        }
    }

    if (unmarked_values.len == 0)
    {
        inst = last_inst = control_transfer_inst = 0;
        succs = {};
    }
    else
    {
        inst = unmarked_values[0];
        last_inst = control_transfer_inst = unmarked_values[unmarked_values.len - 1];

        inst->prev = NULL;
        last_inst->next = NULL;

        unmarked_values.push(NULL);
        for (int i = 0; i < unmarked_values.len - 1; i++)
        {
            auto p = unmarked_values[i];
            auto n = unmarked_values[i + 1];
            p->next = n;
            if (n)
                n->prev = p;
        }
    }
}