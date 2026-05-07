#include "../backend/CodeGenerator.h"
#include "../utility/Product.h"
#include "../backend/RV64_I.h"
#include "../backend/RV64_F.h"

CodeGenerator code_generator;

LIRFunction& CodeGenerator::InFunction()
{
    return lir.functions[in_function_number];
}



void CodeGenerator::CodeGen()
{
    //.data
    CodeGenGlobalDef();

    //.text
    for(int i=0;i<lir.functions.size();i++)
    {
        in_function_number=i;
        CodeGenFunction(lir.functions[i]);
    }
}

void CodeGenerator::CodeGenGlobalDef()
{
    for(auto iter=lir.global_variables.begin();
        iter!=lir.global_variables.end();iter++)
        {
            Variable& global_var=iter->second;
            assembly.AddData(".globl "+global_var.name+"\n");
            assembly.AddData(" "+global_var.name+":\n");

            //Scalar
            if(global_var.data_type==I32)
            {
                assembly.AddData("  .word ");
                if(global_var.initial.empty())
                    assembly.AddData(" 0\n");
                else assembly.AddData(global_var.initial[0].GetStrNoType()+"\n");
            }
            else if(global_var.data_type==F32)
            {
                assembly.AddData("  .float ");
                if(global_var.initial.empty())
                    assembly.AddData(" 0\n");
                else assembly.AddData(global_var.initial[0].GetStrNoType()+"\n");
            }
            
            //Array
            else if(global_var.data_type==I32_PTR)
            {
                if(global_var.initial.empty())
                {
                    assembly.AddData("  .zero ");
                    assembly.AddData(to_string(4*global_var.length)+"\n");
                }
                else
                {
                    int zero_number=0;
                    for(ValueConstant& initial:global_var.initial)
                    {
                        if(initial.constant_value.i32)
                        {
                            if(zero_number)
                            {
                                assembly.AddData("  .zero "+to_string(4*zero_number)+"\n");
                                zero_number=0;
                            }
                            assembly.AddData("  .word "+to_string(initial.constant_value.i32)+"\n");
                        }
                        else zero_number++;
                    }
                    if(zero_number)
                        assembly.AddData("  .zero "+to_string(4*zero_number)+"\n");
                }
            }
            else if(global_var.data_type==F32_PTR)
            {
                if(global_var.initial.empty())
                {
                    assembly.AddData("  .zero ");
                    assembly.AddData(to_string(4*global_var.length)+"\n");
                }
                else
                {
                    int zero_number=0;
                    for(ValueConstant& initial:global_var.initial)
                    {
                        if(initial.constant_value.f32)
                        {
                            if(zero_number)
                            {
                                assembly.AddData("  .zero "+to_string(4*zero_number)+"\n");
                                zero_number=0;
                            }
                            assembly.AddData("  .float "+to_string(initial.constant_value.f32)+"\n");
                        }
                        else zero_number++;
                    }
                    if(zero_number)
                        assembly.AddData("  .zero "+to_string(4*zero_number)+"\n");
                }
            }

            assembly.AddData("\n");
        }
}

void CodeGenerator::CodeGenFunction(LIRFunction& lir_function)
{
    //Create new function
    assembly.NewFunction(lir_function.name);
    offset_table.clear();

    //Caculate stack
    int sp_offset=lir_function.stack_parameter_use;

    for(auto iter=lir_function.local_variables.begin();
        iter!=lir_function.local_variables.end();iter++)
        {
            Variable& local_var=iter->second;
            
            if(local_var.IsNormalVar())
            {
                offset_table[local_var.number]=sp_offset;
                sp_offset+=4;
            }
            else if(local_var.IsNormalPointer())
            {
                offset_table[local_var.number]=sp_offset;
                sp_offset+=8;
            }
            else if(local_var.IsArrayPointer())
            {
                offset_table[local_var.number]=sp_offset;
                sp_offset+=local_var.length*4;
            }
        }
        
    sp_offset+=8;//Reserve for ra
    if(sp_offset%16!=0)sp_offset=((sp_offset/16)+1)*16;

    //Create stack space
    assembly.AddPrologue(addi("sp","sp",-sp_offset));
    //Store ra
    assembly.AddPrologue(sd("ra",sp_offset-8,"sp"));

    InFunction().sp_offset=sp_offset;

    //Mark visited block
    vector<bool> visited(lir_function.basic_blocks.size(),false);
    visited[lir_function.exit_block_number]=true;

    //BFS
    queue<LIRBasicBlock*> Q;
    Q.push(&lir_function.basic_blocks[lir_function.entry_block_number]);
    visited[lir_function.entry_block_number]=true;

    //Must be marked before gen!!!
    while(!Q.empty())
    {
        CodeGenBasicBlock(*Q.front());

        for(const int& suc_number:Q.front()->succeeds)
            if(!visited[suc_number])
            {
                Q.push(&lir_function.basic_blocks[suc_number]);
                visited[suc_number]=true;
            }

        Q.pop();
    }

    //Gen exit block at last
    CodeGenBasicBlock(lir_function.basic_blocks[lir_function.exit_block_number]);

    //Restore ra
    assembly.AddEpilogue(ld("ra",sp_offset-8,"sp"));
    //Restore sp
    assembly.AddEpilogue(addi("sp","sp",sp_offset));
    assembly.AddEpilogue(ret());
}

void CodeGenerator::CodeGenBasicBlock(LIRBasicBlock& lir_block)
{
    //Lable
    assembly.AddLable(InFunction().name+"_L"+to_string(lir_block.number));

    for(shared_ptr<LIRInstruction>& lir_instruction_ptr:lir_block.instructions)
        CodeGenInstruction(lir_instruction_ptr);
}

void CodeGenerator::CodeGenInstruction(shared_ptr<LIRInstruction>& lir_instruction_ptr)
{
    switch(lir_instruction_ptr->lir_instruction_type)
    {
        //Integer

        //Arithmetic
        case ADD_I:{
            shared_ptr<AddI> addI=static_pointer_cast<AddI>(lir_instruction_ptr);
            assembly.AddBody(addw(addI->des_i.GetRV64(),addI->src_i1.GetRV64(),addI->src_i2.GetRV64()));
            break;
        }
        case ADD_IMM:{
            shared_ptr<AddImm> addImm=static_pointer_cast<AddImm>(lir_instruction_ptr);
            assembly.AddBody(addwi(addImm->des_i.GetRV64(),addImm->src_i.GetRV64(),addImm->src_imm));
            break;
        }
        case ADD_L:{
            shared_ptr<AddL> addL=static_pointer_cast<AddL>(lir_instruction_ptr);
            assembly.AddBody(add(addL->des_i.GetRV64(),addL->src_i1.GetRV64(),addL->src_i2.GetRV64()));
            break;
        }
        case SUB_I:{
            shared_ptr<SubI> subI=static_pointer_cast<SubI>(lir_instruction_ptr);
            assembly.AddBody(subw(subI->des_i.GetRV64(),subI->src_i1.GetRV64(),subI->src_i2.GetRV64()));
            break;
        }
        case MUL_I:{
            shared_ptr<MulI> mulI=static_pointer_cast<MulI>(lir_instruction_ptr);
            assembly.AddBody(mul(mulI->des_i.GetRV64(),mulI->src_i1.GetRV64(),mulI->src_i2.GetRV64()));
            break;
        }
        case DIV_I:{
            shared_ptr<DivI> divI=static_pointer_cast<DivI>(lir_instruction_ptr);
            assembly.AddBody(divw(divI->des_i.GetRV64(),divI->src_i1.GetRV64(),divI->src_i2.GetRV64()));
            break;
        }
        case REM_I:{
            shared_ptr<RemI> remI=static_pointer_cast<RemI>(lir_instruction_ptr);
            assembly.AddBody(remw(remI->des_i.GetRV64(),remI->src_i1.GetRV64(),remI->src_i2.GetRV64()));
            break;
        }
        case NEG_I:{
            shared_ptr<NegI> negI=static_pointer_cast<NegI>(lir_instruction_ptr);
            assembly.AddBody(negw(negI->des_i.GetRV64(),negI->src_i.GetRV64()));
            break;
        }
        //Logic
        case NOT_I:{
            shared_ptr<NotI> notI=static_pointer_cast<NotI>(lir_instruction_ptr);
            assembly.AddBody(not_(notI->des_i.GetRV64(),notI->src_i.GetRV64()));
            break;
        }
        case AND_I:{
            shared_ptr<AndI> andI=static_pointer_cast<AndI>(lir_instruction_ptr);
            assembly.AddBody(and_(andI->des_i.GetRV64(),andI->src_i1.GetRV64(),andI->src_i2.GetRV64()));
            break;
        }
        case AND_IMM:{
            shared_ptr<AndImm> andImm=static_pointer_cast<AndImm>(lir_instruction_ptr);
            assembly.AddBody(andi(andImm->des_i.GetRV64(),andImm->src_i.GetRV64(),andImm->src_imm));
            break;
        }
        case OR_I:{
            shared_ptr<OrI> orI=static_pointer_cast<OrI>(lir_instruction_ptr);
            assembly.AddBody(or_(orI->des_i.GetRV64(),orI->src_i1.GetRV64(),orI->src_i2.GetRV64()));
            break;
        }
        case OR_IMM:{
            shared_ptr<OrImm> orImm=static_pointer_cast<OrImm>(lir_instruction_ptr);
            assembly.AddBody(ori(orImm->des_i.GetRV64(),orImm->src_i.GetRV64(),orImm->src_imm));
            break;
        }
        case XOR_I:{
            shared_ptr<XorI> xorI=static_pointer_cast<XorI>(lir_instruction_ptr);
            assembly.AddBody(xor_(xorI->des_i.GetRV64(),xorI->src_i1.GetRV64(),xorI->src_i2.GetRV64()));
            break;
        }
        case XOR_IMM:{
            shared_ptr<XorImm> xorImm=static_pointer_cast<XorImm>(lir_instruction_ptr);
            assembly.AddBody(xori(xorImm->des_i.GetRV64(),xorImm->src_i.GetRV64(),xorImm->src_imm));
            break;
        }
        //Compare
        case LESS_I:{
            shared_ptr<LessI> lessI=static_pointer_cast<LessI>(lir_instruction_ptr);
            assembly.AddBody(slt(lessI->des_i.GetRV64(),lessI->src_i1.GetRV64(),lessI->src_i2.GetRV64()));
            break;
        }
        case LESS_IMM:{
            shared_ptr<LessImm> lessImm=static_pointer_cast<LessImm>(lir_instruction_ptr);
            assembly.AddBody(slti(lessImm->des_i.GetRV64(),lessImm->src_i.GetRV64(),lessImm->src_imm));
            break;
        }
        case EQUZ_I:{
            shared_ptr<EquzI> equzI=static_pointer_cast<EquzI>(lir_instruction_ptr);
            assembly.AddBody(seqz(equzI->des_i.GetRV64(),equzI->src_i.GetRV64()));
            break;
        }
        case NEQUZ_I:{
            shared_ptr<NequzI> nequzI=static_pointer_cast<NequzI>(lir_instruction_ptr);
            assembly.AddBody(snez(nequzI->des_i.GetRV64(),nequzI->src_i.GetRV64()));
            break;
        }
        case LESSZ_I:{
            shared_ptr<LesszI> lesszI=static_pointer_cast<LesszI>(lir_instruction_ptr);
            assembly.AddBody(sltz(lesszI->des_i.GetRV64(),lesszI->src_i.GetRV64()));
            break;
        }
        case GREZ_I:{
            shared_ptr<GrezI> grezI=static_pointer_cast<GrezI>(lir_instruction_ptr);
            assembly.AddBody(sgtz(grezI->des_i.GetRV64(),grezI->src_i.GetRV64()));
            break;
        }
        //Memory
        case LOAD_A:{
            shared_ptr<LoadA> loadA=static_pointer_cast<LoadA>(lir_instruction_ptr);
            assembly.AddBody(la(loadA->des_i.GetRV64(),loadA->src_globalvar));
            break;
        }
        case LOAD_IMM:{
            shared_ptr<LoadImm> loadImm=static_pointer_cast<LoadImm>(lir_instruction_ptr);
            assembly.AddBody(li(loadImm->des_i.GetRV64(),loadImm->src_imm));
            break;
        }
        case LOAD_I:{
            shared_ptr<LoadI> loadI=static_pointer_cast<LoadI>(lir_instruction_ptr);
            assembly.AddBody(lw(loadI->des_i.GetRV64(),0,loadI->src_i.GetRV64()));
            break;
        }
        case LOAD_L:{
            shared_ptr<LoadL> loadL=static_pointer_cast<LoadL>(lir_instruction_ptr);
            assembly.AddBody(ld(loadL->des_i.GetRV64(),0,loadL->src_i.GetRV64()));
            break;
        }
        case STORE_I:{
            shared_ptr<StoreI> storeI=static_pointer_cast<StoreI>(lir_instruction_ptr);
            // if(storeI->src_i.vreg_type==S_ARG_I&&storeI->src_i.number>=8)
            // {
            //     int stack_offset=(storeI->des_i.number-8)*8;
            //     assembly.AddBody(ld("t6",stack_offset,"sp"));
            //     assembly.AddBody(sw("t6",0,storeI->des_i.GetRV64()));
            // }
            // else 
            assembly.AddBody(sw(storeI->src_i.GetRV64(),0,storeI->des_i.GetRV64()));
            break;
        }
        case STORE_L:{
            shared_ptr<StoreL> storeL=static_pointer_cast<StoreL>(lir_instruction_ptr);
            // if(storeL->src_i.vreg_type==S_ARG_I&&storeL->src_i.number>=8)
            // {
            //     int stack_offset=(storeL->des_i.number-8)*8;
            //     assembly.AddBody(ld("t6",stack_offset,"sp"));
            //     assembly.AddBody(sd("t6",0,storeL->des_i.GetRV64()));
            // }
            // else 
            assembly.AddBody(sd(storeL->src_i.GetRV64(),0,storeL->des_i.GetRV64()));
            break;
        }

        //Float
        //Arithmetic
        case ADD_F:{
            shared_ptr<AddF> addF=static_pointer_cast<AddF>(lir_instruction_ptr);
            assembly.AddBody(fadd_s(addF->des_f.GetRV64(),addF->src_f1.GetRV64(),addF->src_f2.GetRV64()));
            break;
        }
        case SUB_F:{
            shared_ptr<SubF> subF=static_pointer_cast<SubF>(lir_instruction_ptr);
            assembly.AddBody(fsub_s(subF->des_f.GetRV64(),subF->src_f1.GetRV64(),subF->src_f2.GetRV64()));
            break;
        }
        case MUL_F:{
            shared_ptr<MulF> mulF=static_pointer_cast<MulF>(lir_instruction_ptr);
            assembly.AddBody(fmul_s(mulF->des_f.GetRV64(),mulF->src_f1.GetRV64(),mulF->src_f2.GetRV64()));
            break;
        }
        case DIV_F:{
            shared_ptr<DivF> divF=static_pointer_cast<DivF>(lir_instruction_ptr);
            assembly.AddBody(fdiv_s(divF->des_f.GetRV64(),divF->src_f1.GetRV64(),divF->src_f2.GetRV64()));
            break;
        }
        case NEG_F:{
            shared_ptr<NegF> negF=static_pointer_cast<NegF>(lir_instruction_ptr);
            assembly.AddBody(fneg_s(negF->des_f.GetRV64(),negF->src_f.GetRV64()));
            break;
        }
        //Compare
        case EQU_F:{
            shared_ptr<EquF> equF=static_pointer_cast<EquF>(lir_instruction_ptr);
            assembly.AddBody(feq_s(equF->des_i.GetRV64(),equF->src_f1.GetRV64(),equF->src_f2.GetRV64()));
            break;
        }
        case LEQU_F:{
            shared_ptr<LequF> lequF=static_pointer_cast<LequF>(lir_instruction_ptr);
            assembly.AddBody(fle_s(lequF->des_i.GetRV64(),lequF->src_f1.GetRV64(),lequF->src_f2.GetRV64()));
            break;
        }
        case LESS_F:{
            shared_ptr<LessF> lessF=static_pointer_cast<LessF>(lir_instruction_ptr);
            assembly.AddBody(flt_s(lessF->des_i.GetRV64(),lessF->src_f1.GetRV64(),lessF->src_f2.GetRV64()));
            break;
        }
        //Memory
        case LOAD_F:{
            shared_ptr<LoadF> loadF=static_pointer_cast<LoadF>(lir_instruction_ptr);
            assembly.AddBody(flw(loadF->des_f.GetRV64(),0,loadF->src_i.GetRV64()));
            break;
        }
        case STORE_F:{
            shared_ptr<StoreF> storeF=static_pointer_cast<StoreF>(lir_instruction_ptr);
            // if(storeF->src_f.vreg_type==G_ARG_F&&storeF->src_f.number>=8)
            // {
            //     int stack_offset=(storeF->des_i.number-8)*8;
            //     assembly.AddBody(flw("ft6",stack_offset,"sp"));
            //     assembly.AddBody(fsw("ft6",0,storeF->des_i.GetRV64()));
            // } 
            // else 
            assembly.AddBody(fsw(storeF->src_f.GetRV64(),0,storeF->des_i.GetRV64()));
            break;
        }
        
        //Share
        //Move
        case MOVE_I:{
            shared_ptr<MoveI> moveI=static_pointer_cast<MoveI>(lir_instruction_ptr);
            if(moveI->des_i.vreg_type==S_ARG_I&&moveI->des_i.number>=8)
            {
                int stack_offset=(moveI->des_i.number-8)*8;
                assembly.AddBody(sd(moveI->src_i.GetRV64(),stack_offset,"sp"));
            }
            else assembly.AddBody(mv(moveI->des_i.GetRV64(),moveI->src_i.GetRV64()));
            break;
        }
        case MOVE_F:{
            shared_ptr<MoveF> moveF=static_pointer_cast<MoveF>(lir_instruction_ptr);
            if(moveF->des_f.vreg_type==G_ARG_F&&moveF->des_f.number>=8)
            {
                int stack_offset=(moveF->des_f.number-8)*8;
                assembly.AddBody(fsw(moveF->src_f.GetRV64(),stack_offset,"sp"));
            }
            else assembly.AddBody(fmv_s(moveF->des_f.GetRV64(),moveF->src_f.GetRV64()));
            break;
        }
        case MOVE_I2F:{
            shared_ptr<MoveI2F> moveI2F=static_pointer_cast<MoveI2F>(lir_instruction_ptr);
            assembly.AddBody(fmv_w_x(moveI2F->des_f.GetRV64(),moveI2F->src_i.GetRV64()));
            break;
        }
        case MOVE_F2I:{
            shared_ptr<MoveF2I> moveF2I=static_pointer_cast<MoveF2I>(lir_instruction_ptr);
            assembly.AddBody(fmv_x_w(moveF2I->des_i.GetRV64(),moveF2I->src_f.GetRV64()));
            break;
        }

        //Control flow
        case CALL:{
            shared_ptr<Call> _call=static_pointer_cast<Call>(lir_instruction_ptr);
            assembly.AddBody(call(_call->function));
            break;
        }
        case RET:{
            shared_ptr<Ret> _ret=static_pointer_cast<Ret>(lir_instruction_ptr);
            assembly.AddBody(ret());
            break;
        }
        case BNEZ:{
            shared_ptr<Bnez> _bnez=static_pointer_cast<Bnez>(lir_instruction_ptr);
            assembly.AddBody(bnez(_bnez->src_i.GetRV64(),InFunction().name+"_L"+to_string(_bnez->number)));
            break;
        }
        case JMP:{
            shared_ptr<Jmp> jmp=static_pointer_cast<Jmp>(lir_instruction_ptr);
            assembly.AddBody(j(InFunction().name+"_L"+to_string(jmp->number)));
            break;
        }

        //Convert
        case CVT_I2F:{
            shared_ptr<CvtI2F> cvtI2F=static_pointer_cast<CvtI2F>(lir_instruction_ptr);
            assembly.AddBody(fcvt_s_w(cvtI2F->des_f.GetRV64(),cvtI2F->src_i.GetRV64()));
            break;
        }
        case CVT_F2I:{
            shared_ptr<CvtF2I> cvtF2I=static_pointer_cast<CvtF2I>(lir_instruction_ptr);
            assembly.AddBody(fcvt_w_s(cvtF2I->des_i.GetRV64(),cvtF2I->src_f.GetRV64()));
            break;
        }

        //Custom
        case GETI:{
            shared_ptr<GetI> getI=static_pointer_cast<GetI>(lir_instruction_ptr);
            Variable& local_var=InFunction().local_variables[getI->src_localvar_number];

            if(local_var.IsNormalVar())
                assembly.AddBody(lw(getI->des_i.GetRV64(),offset_table[local_var.number],"sp"));
            else if(local_var.IsNormalPointer())
                assembly.AddBody(ld(getI->des_i.GetRV64(),offset_table[local_var.number],"sp"));
            else if(local_var.IsArrayPointer())
                assembly.AddBody(addi(getI->des_i.GetRV64(),"sp",offset_table[local_var.number]));
            
            break;
        }
        case GETF:{
            shared_ptr<GetF> getF=static_pointer_cast<GetF>(lir_instruction_ptr);
            Variable& local_var=InFunction().local_variables[getF->src_localvar_number];

            if(local_var.IsNormalVar())
                assembly.AddBody(flw(getF->des_f.GetRV64(),offset_table[local_var.number],"sp"));
            else CompilerError("GetF should only can get F32");

            break;
        }
        case PUTI:{
            shared_ptr<PutI> putI=static_pointer_cast<PutI>(lir_instruction_ptr);
            Variable& local_var=InFunction().local_variables[putI->des_localvar_number];
            
            if(local_var.IsNormalVar())
            {
                if(putI->src_i.vreg_type==S_ARG_I&&putI->src_i.number>=8)
                {
                    int stack_offset=(putI->src_i.number-8)*8+InFunction().sp_offset;
                    assembly.AddBody(ld("t5",stack_offset,"sp"));
                    assembly.AddBody(sw("t5",offset_table[local_var.number],"sp"));
                }
                else assembly.AddBody(sw(putI->src_i.GetRV64(),offset_table[local_var.number],"sp"));
            }
            else if(local_var.IsNormalPointer())
            {
                if(putI->src_i.vreg_type==S_ARG_I&&putI->src_i.number>=8)
                {
                    int stack_offset=(putI->src_i.number-8)*8+InFunction().sp_offset;
                    assembly.AddBody(ld("t5",stack_offset,"sp"));
                    assembly.AddBody(sd("t5",offset_table[local_var.number],"sp"));
                }
                else assembly.AddBody(sd(putI->src_i.GetRV64(),offset_table[local_var.number],"sp"));
            }
            else CompilerError("PutI should only can put I32 and NomalPTR");

            break;
        }
        case PUTF:{
            shared_ptr<PutF> putF=static_pointer_cast<PutF>(lir_instruction_ptr);
            Variable& local_var=InFunction().local_variables[putF->des_localvar_number];
            
            if(local_var.IsNormalVar())
            {
                if(putF->src_f.vreg_type==G_ARG_F&&putF->src_f.number>=8)
                {
                    int stack_offset=(putF->src_f.number-8)*8+InFunction().sp_offset;
                    assembly.AddBody(flw("ft5",stack_offset,"sp"));
                    assembly.AddBody(fsw("ft5",offset_table[local_var.number],"sp"));
                }
                else assembly.AddBody(fsw(putF->src_f.GetRV64(),offset_table[local_var.number],"sp"));
            }
                
            else CompilerError("PutF should only can put F32");
            
            break;
        }
    }
}
