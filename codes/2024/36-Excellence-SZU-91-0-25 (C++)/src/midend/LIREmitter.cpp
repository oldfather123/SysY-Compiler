#include "../midend/LIREmitter.h"
#include "../utility/Product.h"

LIREmitter lir_emitter;

// void MIRLocalVarTable::Add(int mir_localvar_number,VirtualRegister vreg)
// {
//     table[mir_localvar_number]=vreg;
// }

// bool MIRLocalVarTable::Exist(int mir_localvar_number)
// {
//     return table.find(mir_localvar_number)!=table.end();
// }

// VirtualRegister MIRLocalVarTable::Visit(int mir_localvar_number)
// {
//     return table[mir_localvar_number];
// }

// void MIRLocalVarTable::Clear()
// {   
//     table.clear();
// }



LIRFunction& LIREmitter::InFunction()
{
    return lir.functions[in_function_number];
}

LIRBasicBlock& LIREmitter::InBasicBlock()
{
    return lir.functions[in_function_number].basic_blocks[in_block_number];
}



void LIREmitter::EmitLIR()
{
    //Transform global variables
    for(int i=0;i<mir.global_var_number;i++)
    {
        //if(!mir.global_variables[i].is_const) @Bug
            lir.global_variables[i]=mir.global_variables[i];
    }

    //Emit function
    for(int i=0;i<mir.function_number;i++)
    {
        if(!mir.functions[i].is_lib)
        {
            in_function_number=lir.NewFunction();
            in_function_name=mir.functions[i].name;

            EmitLIRFunction(mir.functions[i]);
        }
    }
}

void LIREmitter::EmitLIRFunction(MIRFunction& mir_function)
{
    vreg_manager.Clear();
    
    InFunction().name=mir_function.name;
    InFunction().entry_block_number=mir_function.entry_block_number;
    InFunction().exit_block_number=mir_function.exit_block_number;

    //Transform local variables
    for(int i=0;i<mir_function.local_var_number;i++)
    {
        if(!mir_function.local_variables[i].is_const)
            InFunction().local_variables[i]=mir_function.local_variables[i];
    }

    //Emit basic blocks
    for(MIRBasicBlock& mir_block:mir_function.basic_blocks)
    {
        in_block_number=InFunction().NewBasicBlock();

        //Store parameters
        if(in_block_number==InFunction().entry_block_number)
        {
            
            vreg_manager.ArgFree();
            for(int i=0;i<mir_function.parameter_number;i++)
            {
                Variable& parameter=mir_function.local_variables[i];
                shared_ptr<ValueVariable> value_parameter=make_shared<ValueVariable>(
                    parameter.store_type,parameter.data_type,parameter.number
                );

                VirtualRegister parameter_vreg;
                if(value_parameter->data_type==I32)
                    parameter_vreg=vreg_manager.NewVRegS(4);
                else if(value_parameter->IsPointer())
                    parameter_vreg=vreg_manager.NewVRegS(8);
                else if(value_parameter->data_type==F32)
                    parameter_vreg=vreg_manager.NewVRegG(4);
                
                Store(parameter_vreg,value_parameter);
            }
        }

        EmitLIRBasicBlock(mir_block);
    }
}

void LIREmitter::EmitLIRBasicBlock(MIRBasicBlock& mir_block)
{
    InBasicBlock().precursors=mir_block.precursors;
    InBasicBlock().succeeds=mir_block.succeeds;

    for(shared_ptr<MIRInstruction>& mir_instruction_ptr:mir_block.instructions)
        EmitLIRInstruction(mir_instruction_ptr);
}

void LIREmitter::EmitLIRInstruction(shared_ptr<MIRInstruction>& mir_instruction_ptr)
{
    switch(mir_instruction_ptr->mir_instruction_type)
    {
        //Copy
        case COPY:{
            shared_ptr<Copy> copy=static_pointer_cast<Copy>(mir_instruction_ptr);
            EmitCopy(copy);
            break;
        }

        //Control flow
        case BR:{
            shared_ptr<Branch> branch=static_pointer_cast<Branch>(mir_instruction_ptr);
            EmitBranch(branch);
            break;
        }
        case JUMP:{
            shared_ptr<Jump> jump=static_pointer_cast<Jump>(mir_instruction_ptr);
            EmitJump(jump);
            break;
        }
        case CALLL:{
            shared_ptr<Calll> call=static_pointer_cast<Calll>(mir_instruction_ptr);
            EmitCall(call);
            break;
        }
        case RETT:{
            shared_ptr<Return> ret=static_pointer_cast<Return>(mir_instruction_ptr);
            EmitReturn(ret);
            break;
        }

        //Arithmetic
        case OR:{
            shared_ptr<Or> _or=static_pointer_cast<Or>(mir_instruction_ptr);
            EmitOr(_or);
            break;
        }
        case AND:{
            shared_ptr<And> _and=static_pointer_cast<And>(mir_instruction_ptr);
            EmitAnd(_and);
            break;
        }
        case NOT:{
            shared_ptr<Not> _not=static_pointer_cast<Not>(mir_instruction_ptr);
            EmitNot(_not);
            break;
        }
        case NEG:{
            shared_ptr<Negative> neg=static_pointer_cast<Negative>(mir_instruction_ptr);
            EmitNegative(neg);
            break;
        }
        case EQU:{
            shared_ptr<Equal> equal=static_pointer_cast<Equal>(mir_instruction_ptr);
            EmitEqual(equal);
            break;
        }
        case NEQU:{
            shared_ptr<NotEqual> not_equal=static_pointer_cast<NotEqual>(mir_instruction_ptr);
            EmitNotEqual(not_equal);
            break;
        }
        case LES:{
            shared_ptr<Less> less=static_pointer_cast<Less>(mir_instruction_ptr);
            EmitLess(less);
            break;
        }
        case LEQU:{
            shared_ptr<LessEqual> less_equal=static_pointer_cast<LessEqual>(mir_instruction_ptr);
            EmitLessEqual(less_equal);
            break;
        }
        case GRE:{
            shared_ptr<Greater> greater=static_pointer_cast<Greater>(mir_instruction_ptr);
            EmitGreater(greater);
            break;
        }
        case GEQU:{
            shared_ptr<GreaterEqual> greater_equal=static_pointer_cast<GreaterEqual>(mir_instruction_ptr);
            EmitGreaterEqual(greater_equal);
            break;
        }
        case ADD:{
            shared_ptr<Add> add=static_pointer_cast<Add>(mir_instruction_ptr);
            EmitAdd(add);
            break;
        }
        case SUB:{
            shared_ptr<Sub> sub=static_pointer_cast<Sub>(mir_instruction_ptr);
            EmitSub(sub);
            break;
        }
        case MUL:{
            shared_ptr<Mul> mul=static_pointer_cast<Mul>(mir_instruction_ptr);
            EmitMul(mul);
            break;
        }
        case DIV:{
            shared_ptr<Div> div=static_pointer_cast<Div>(mir_instruction_ptr);
            EmitDiv(div);
            break;
        }
        case MOD:{
            shared_ptr<Mod> mod=static_pointer_cast<Mod>(mir_instruction_ptr);
            EmitMod(mod);
            break;
        }

        //Convert
        case I2F:{
            shared_ptr<ItoF> i2f=static_pointer_cast<ItoF>(mir_instruction_ptr);
            EmitItoF(i2f);
            break;
        }
        case F2I:{
            shared_ptr<FtoI> f2i=static_pointer_cast<FtoI>(mir_instruction_ptr);
            EmitFtoI(f2i); 
            break;
        }
    }
}



//Copy
void LIREmitter::EmitCopy(shared_ptr<Copy>& copy)
{
    Store(Load(copy->value),copy->variable);
}



//Control flow
void LIREmitter::EmitBranch(shared_ptr<Branch>& branch)
{
    shared_ptr<Bnez> bnez=make_shared<Bnez>();
    bnez->src_i=Load(branch->condition);
    bnez->number=branch->true_number;
    AddInsturction(bnez);

    shared_ptr<Jmp> jmp=make_shared<Jmp>();
    jmp->number=branch->false_number;
    AddInsturction(jmp);
}

void LIREmitter::EmitJump(shared_ptr<Jump>& jump)
{
    shared_ptr<Jmp> jmp=make_shared<Jmp>();
    jmp->number=jump->number;
    AddInsturction(jmp);
}

void LIREmitter::EmitCall(shared_ptr<Calll>& calll)
{
    //Move arguments
    vreg_manager.ArgFree();

    //To calculate stack for parameter
    vector<int> VRegSSize;
    vector<int> VRegGSize;

    for(shared_ptr<Value>& value_argument:calll->arguments)
    {
        DataType argument_data_type=ValueDataType(value_argument);
    
        if(argument_data_type==I32||argument_data_type==I32_PTR
         ||argument_data_type==F32_PTR)
         {
            shared_ptr<MoveI> moveI=make_shared<MoveI>();
            moveI->src_i=Load(value_argument);
            if(argument_data_type==I32_PTR||argument_data_type==F32_PTR)
                moveI->des_i=vreg_manager.NewVRegS(8);
            else if(argument_data_type==I32)
                moveI->des_i=vreg_manager.NewVRegS(4);
            AddInsturction(moveI);

            if(argument_data_type==I32_PTR||argument_data_type==F32_PTR) VRegSSize.push_back(8);
            else if(argument_data_type==I32) VRegGSize.push_back(8);
         }
        else if(argument_data_type==F32)
        {
            shared_ptr<MoveF> moveF=make_shared<MoveF>();
            moveF->src_f=Load(value_argument);
            moveF->des_f=vreg_manager.NewVRegG(4);
            AddInsturction(moveF);

            VRegGSize.push_back(8);
        }
    }

    //Calculate spill parameter
    int parameter_spill=0;
    for(int i=8;i<VRegSSize.size();i++)parameter_spill+=VRegSSize[i];
    for(int i=8;i<VRegGSize.size();i++)parameter_spill+=VRegGSize[i];
    InFunction().stack_parameter_use=max(InFunction().stack_parameter_use,parameter_spill);

    //Call
    shared_ptr<Call> call=make_shared<Call>();
    call->function=calll->function_name;
    AddInsturction(call);

    //Get return value
    if(calll->get_retvalue)
    {
        if(calll->get_retvalue->data_type==I32)
            Store(vreg_manager.GetVRegB(4),calll->get_retvalue);
        else if(calll->get_retvalue->IsPointer())
            Store(vreg_manager.GetVRegB(8),calll->get_retvalue);
        else if(calll->get_retvalue->data_type==F32)
            Store(vreg_manager.GetVRegW(4),calll->get_retvalue);
    }
}

void LIREmitter::EmitReturn(shared_ptr<Return>& _return)
{
    if(_return->return_value)
    {
        if(ValueDataType(_return->return_value)==I32)
        {
            shared_ptr<MoveI> moveI=make_shared<MoveI>();
            moveI->src_i=Load(_return->return_value);
            moveI->des_i=vreg_manager.GetVRegB(4);
            AddInsturction(moveI);
        }
        else if(ValueDataType(_return->return_value)==F32)
        {
            shared_ptr<MoveF> moveF=make_shared<MoveF>();
            moveF->src_f=Load(_return->return_value);
            moveF->des_f=vreg_manager.GetVRegW(4);
            AddInsturction(moveF);
        }
    }

    shared_ptr<Jmp> jmp=make_shared<Jmp>();
    jmp->number=InFunction().exit_block_number;
    AddInsturction(jmp);
}



//Arithmetic
void LIREmitter::EmitOr(shared_ptr<Or>& _or)
{
    shared_ptr<OrI> orI=make_shared<OrI>();
    orI->src_i1=Load(_or->a);
    orI->src_i2=Load(_or->b);
    orI->des_i=vreg_manager.NewVRegR(4);
    AddInsturction(orI);

    Store(orI->des_i,_or->c);
}

void LIREmitter::EmitAnd(shared_ptr<And>& _and)
{
    shared_ptr<AndI> andI=make_shared<AndI>();
    andI->src_i1=Load(_and->a);
    andI->src_i2=Load(_and->b);
    andI->des_i=vreg_manager.NewVRegR(4);
    AddInsturction(andI);

    Store(andI->des_i,_and->c);
}

void LIREmitter::EmitNot(shared_ptr<Not>& _not)
{
    shared_ptr<EquzI> equzI=make_shared<EquzI>();
    equzI->src_i=Load(_not->a);
    equzI->des_i=vreg_manager.NewVRegR(4);
    AddInsturction(equzI);

    Store(equzI->des_i,_not->b);
}

void LIREmitter::EmitNegative(shared_ptr<Negative>& neg)
{
    if(neg->data_type==I32)
    {
        shared_ptr<NegI> negI=make_shared<NegI>();
        negI->src_i=Load(neg->a);
        negI->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(negI);

        Store(negI->des_i,neg->b);
    }
    else if(neg->data_type==F32)
    {
        shared_ptr<NegF> negF=make_shared<NegF>();
        negF->src_f=Load(neg->a);
        negF->des_f=vreg_manager.NewVRegF(4);
        AddInsturction(negF);

        Store(negF->des_f,neg->b);
    }
}

void LIREmitter::EmitEqual(shared_ptr<Equal>& equal)
{
    if(equal->data_type==I32)
    {
        shared_ptr<XorI> xorI=make_shared<XorI>();
        xorI->src_i1=Load(equal->a);
        xorI->src_i2=Load(equal->b);
        xorI->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(xorI);

        shared_ptr<EquzI> equzI=make_shared<EquzI>();
        equzI->src_i=xorI->des_i;
        equzI->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(equzI);

        Store(equzI->des_i,equal->c);
    }
    else if(equal->data_type==F32)
    {
        shared_ptr<EquF> equF=make_shared<EquF>();
        equF->src_f1=Load(equal->a);
        equF->src_f2=Load(equal->b);
        equF->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(equF);

        Store(equF->des_i,equal->c);
    }
}

void LIREmitter::EmitNotEqual(shared_ptr<NotEqual>& not_equal)
{
    if(not_equal->data_type==I32)
    {
        shared_ptr<XorI> xorI=make_shared<XorI>();
        xorI->src_i1=Load(not_equal->a);
        xorI->src_i2=Load(not_equal->b);
        xorI->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(xorI);

        shared_ptr<NequzI> nequzI=make_shared<NequzI>();
        nequzI->src_i=xorI->des_i;
        nequzI->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(nequzI);

        Store(nequzI->des_i,not_equal->c);
    }
    else if(not_equal->data_type==F32)
    {
        shared_ptr<EquF> equF=make_shared<EquF>();
        equF->src_f1=Load(not_equal->a);
        equF->src_f2=Load(not_equal->b);
        equF->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(equF);

        shared_ptr<XorImm> xorImm=make_shared<XorImm>();
        xorImm->src_i=equF->des_i;
        xorImm->src_imm=1;
        xorImm->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(xorImm);

        Store(xorImm->des_i,not_equal->c);
    }
}

void LIREmitter::EmitLess(shared_ptr<Less>& less)
{
    if(less->data_type==I32)
    {
        shared_ptr<LessI> lessI=make_shared<LessI>();
        lessI->src_i1=Load(less->a);
        lessI->src_i2=Load(less->b);
        lessI->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(lessI);

        Store(lessI->des_i,less->c);
    }
    else if(less->data_type==F32)
    {
        shared_ptr<LessF> lessF=make_shared<LessF>();
        lessF->src_f1=Load(less->a);
        lessF->src_f2=Load(less->b);
        lessF->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(lessF);

        Store(lessF->des_i,less->c);
    }
}

void LIREmitter::EmitLessEqual(shared_ptr<LessEqual>& less_equal)
{
    if(less_equal->data_type==I32)
    {
        shared_ptr<LessI> lessI=make_shared<LessI>();
        lessI->src_i1=Load(less_equal->b);
        lessI->src_i2=Load(less_equal->a);
        lessI->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(lessI);

        shared_ptr<XorImm> xorImm=make_shared<XorImm>();
        xorImm->src_i=lessI->des_i;
        xorImm->src_imm=1;
        xorImm->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(xorImm);

        Store(xorImm->des_i,less_equal->c);
    }
    else if(less_equal->data_type==F32)
    {
        shared_ptr<LequF> lequF=make_shared<LequF>();
        lequF->src_f1=Load(less_equal->a);
        lequF->src_f2=Load(less_equal->b);
        lequF->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(lequF);

        Store(lequF->des_i,less_equal->c);
    }
}

void LIREmitter::EmitGreater(shared_ptr<Greater>& greater)
{
    if(greater->data_type==I32)
    {
        shared_ptr<LessI> lessI=make_shared<LessI>();
        lessI->src_i1=Load(greater->b);
        lessI->src_i2=Load(greater->a);
        lessI->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(lessI);

        Store(lessI->des_i,greater->c);
    }
    else if(greater->data_type==F32)
    {
        shared_ptr<LessF> lessF=make_shared<LessF>();
        lessF->src_f1=Load(greater->b);
        lessF->src_f2=Load(greater->a);
        lessF->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(lessF);

        Store(lessF->des_i,greater->c);
    }
}

void LIREmitter::EmitGreaterEqual(shared_ptr<GreaterEqual>& greater_equal)
{
    if(greater_equal->data_type==I32)
    {
        shared_ptr<LessI> lessI=make_shared<LessI>();
        lessI->src_i1=Load(greater_equal->a);
        lessI->src_i2=Load(greater_equal->b);
        lessI->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(lessI);

        shared_ptr<XorImm> xorImm=make_shared<XorImm>();
        xorImm->src_i=lessI->des_i;
        xorImm->src_imm=1;
        xorImm->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(xorImm);

        Store(xorImm->des_i,greater_equal->c);        
    }
    else if(greater_equal->data_type==F32)
    {
        shared_ptr<LequF> lequF=make_shared<LequF>();
        lequF->src_f1=Load(greater_equal->b);
        lequF->src_f2=Load(greater_equal->a);
        lequF->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(lequF);

        Store(lequF->des_i,greater_equal->c);
    }
}

void LIREmitter::EmitAdd(shared_ptr<Add>& add)
{
    if(add->data_type==I32)
    {
        shared_ptr<AddI> addI=make_shared<AddI>();
        addI->src_i1=Load(add->a);
        addI->src_i2=Load(add->b);
        addI->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(addI);

        Store(addI->des_i,add->c);
    }
    else if(add->data_type==F32)
    {
        shared_ptr<AddF> addF=make_shared<AddF>();
        addF->src_f1=Load(add->a);
        addF->src_f2=Load(add->b);
        addF->des_f=vreg_manager.NewVRegF(4);
        AddInsturction(addF);

        Store(addF->des_f,add->c);
    }
    else if(add->data_type==I32_PTR||add->data_type==F32_PTR)
    {
        shared_ptr<AddL> addL=make_shared<AddL>();
        addL->src_i1=Load(add->a);
        addL->src_i2=Load(add->b);
        addL->des_i=vreg_manager.NewVRegR(8);
        AddInsturction(addL);

        Store(addL->des_i,add->c);
    }
}

void LIREmitter::EmitSub(shared_ptr<Sub>& sub)
{
    if(sub->data_type==I32)
    {
        shared_ptr<SubI> subI=make_shared<SubI>();
        subI->src_i1=Load(sub->a);
        subI->src_i2=Load(sub->b);
        subI->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(subI);

        Store(subI->des_i,sub->c);
    }
    else if(sub->data_type==F32)
    {
        shared_ptr<SubF> subF=make_shared<SubF>();
        subF->src_f1=Load(sub->a);
        subF->src_f2=Load(sub->b);
        subF->des_f=vreg_manager.NewVRegF(4);
        AddInsturction(subF);

        Store(subF->des_f,sub->c);
    }
}

void LIREmitter::EmitMul(shared_ptr<Mul>& mul)
{
    if(mul->data_type==I32)
    {
        shared_ptr<MulI> mulI=make_shared<MulI>();
        mulI->src_i1=Load(mul->a);
        mulI->src_i2=Load(mul->b);
        mulI->des_i=vreg_manager.NewVRegR(8);
        AddInsturction(mulI);

        Store(mulI->des_i,mul->c);
    }
    else if(mul->data_type==F32)
    {
        shared_ptr<MulF> mulF=make_shared<MulF>();
        mulF->src_f1=Load(mul->a);
        mulF->src_f2=Load(mul->b);
        mulF->des_f=vreg_manager.NewVRegF(4);
        AddInsturction(mulF);

        Store(mulF->des_f,mul->c);
    }
}

void LIREmitter::EmitDiv(shared_ptr<Div>& div)
{
    if(div->data_type==I32)
    {
        shared_ptr<DivI> divI=make_shared<DivI>();
        divI->src_i1=Load(div->a);
        divI->src_i2=Load(div->b);
        divI->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(divI);

        Store(divI->des_i,div->c);
    }
    else if(div->data_type==F32)
    {
        shared_ptr<DivF> divF=make_shared<DivF>();
        divF->src_f1=Load(div->a);
        divF->src_f2=Load(div->b);
        divF->des_f=vreg_manager.NewVRegF(4);
        AddInsturction(divF);

        Store(divF->des_f,div->c);
    }
}

void LIREmitter::EmitMod(shared_ptr<Mod>& mod)
{
    shared_ptr<RemI> remI=make_shared<RemI>();
    remI->src_i1=Load(mod->a);
    remI->src_i2=Load(mod->b);
    remI->des_i=vreg_manager.NewVRegR(4);
    AddInsturction(remI);

    Store(remI->des_i,mod->c);
}



//Convert
void LIREmitter::EmitItoF(shared_ptr<ItoF>& i2f)
{
    shared_ptr<CvtI2F> cvtI2F=make_shared<CvtI2F>();
    cvtI2F->src_i=Load(i2f->a_i);
    cvtI2F->des_f=vreg_manager.NewVRegF(4);
    AddInsturction(cvtI2F);

    Store(cvtI2F->des_f,i2f->b_f);
}

void LIREmitter::EmitFtoI(shared_ptr<FtoI>& f2i)
{
    shared_ptr<CvtF2I> cvtF2I=make_shared<CvtF2I>();
    cvtF2I->src_f=Load(f2i->a_f);
    cvtF2I->des_i=vreg_manager.NewVRegR(4);
    AddInsturction(cvtF2I);

    Store(cvtF2I->des_i,f2i->b_i);
}