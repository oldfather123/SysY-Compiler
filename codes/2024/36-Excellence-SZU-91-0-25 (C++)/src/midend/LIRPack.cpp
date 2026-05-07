#include "../midend/LIREmitter.h"
#include "../utility/Product.h"

void LIREmitter::AddInsturction(shared_ptr<LIRInstruction> lir_instruction_ptr)
{
    InBasicBlock().AddInsturction(lir_instruction_ptr);

    switch(lir_instruction_ptr->lir_instruction_type)
    {
        //Integer

        //Arithmetic
        case ADD_I:{
            shared_ptr<AddI> addI=static_pointer_cast<AddI>(lir_instruction_ptr);
            vreg_manager.GeneralFree(addI->src_i1);
            vreg_manager.GeneralFree(addI->src_i2);
            break;
        }
        case ADD_IMM:{
            shared_ptr<AddImm> addImm=static_pointer_cast<AddImm>(lir_instruction_ptr);
            vreg_manager.GeneralFree(addImm->src_i);
            break;
        }
        case ADD_L:{
            shared_ptr<AddL> addL=static_pointer_cast<AddL>(lir_instruction_ptr);
            vreg_manager.GeneralFree(addL->src_i1);
            vreg_manager.GeneralFree(addL->src_i2);
            break;
        }
        case SUB_I:{
            shared_ptr<SubI> subI=static_pointer_cast<SubI>(lir_instruction_ptr);
            vreg_manager.GeneralFree(subI->src_i1);
            vreg_manager.GeneralFree(subI->src_i2);
            break;
        }
        case MUL_I:{
            shared_ptr<MulI> mulI=static_pointer_cast<MulI>(lir_instruction_ptr);
            vreg_manager.GeneralFree(mulI->src_i1);
            vreg_manager.GeneralFree(mulI->src_i2);
            break;
        }
        case DIV_I:{
            shared_ptr<DivI> divI=static_pointer_cast<DivI>(lir_instruction_ptr);
            vreg_manager.GeneralFree(divI->src_i1);
            vreg_manager.GeneralFree(divI->src_i2);
            break;
        }
        case REM_I:{
            shared_ptr<RemI> remI=static_pointer_cast<RemI>(lir_instruction_ptr);
            vreg_manager.GeneralFree(remI->src_i1);
            vreg_manager.GeneralFree(remI->src_i2);
            break;
        }
        case NEG_I:{
            shared_ptr<NegI> negI=static_pointer_cast<NegI>(lir_instruction_ptr);
            vreg_manager.GeneralFree(negI->src_i);
            break;
        }
        //Logic
        case NOT_I:{
            shared_ptr<NotI> notI=static_pointer_cast<NotI>(lir_instruction_ptr);
            vreg_manager.GeneralFree(notI->src_i);
            break;
        }
        case AND_I:{
            shared_ptr<AndI> andI=static_pointer_cast<AndI>(lir_instruction_ptr);
            vreg_manager.GeneralFree(andI->src_i1);
            vreg_manager.GeneralFree(andI->src_i2);
            break;
        }
        case AND_IMM:{
            shared_ptr<AndImm> andImm=static_pointer_cast<AndImm>(lir_instruction_ptr);
            vreg_manager.GeneralFree(andImm->src_i);
            break;
        }
        case OR_I:{
            shared_ptr<OrI> orI=static_pointer_cast<OrI>(lir_instruction_ptr);
            vreg_manager.GeneralFree(orI->src_i1);
            vreg_manager.GeneralFree(orI->src_i2);
            break;
        }
        case OR_IMM:{
            shared_ptr<OrImm> orImm=static_pointer_cast<OrImm>(lir_instruction_ptr);
            vreg_manager.GeneralFree(orImm->src_i);
            break;
        }
        case XOR_I:{
            shared_ptr<XorI> xorI=static_pointer_cast<XorI>(lir_instruction_ptr);
            vreg_manager.GeneralFree(xorI->src_i1);
            vreg_manager.GeneralFree(xorI->src_i2);
            break;
        }
        case XOR_IMM:{
            shared_ptr<XorImm> xorImm=static_pointer_cast<XorImm>(lir_instruction_ptr);
            vreg_manager.GeneralFree(xorImm->src_i);
            break;
        }
        //Compare
        case LESS_I:{
            shared_ptr<LessI> lessI=static_pointer_cast<LessI>(lir_instruction_ptr);
            vreg_manager.GeneralFree(lessI->src_i1);
            vreg_manager.GeneralFree(lessI->src_i2);
            break;
        }
        case LESS_IMM:{
            shared_ptr<LessImm> lessImm=static_pointer_cast<LessImm>(lir_instruction_ptr);
            vreg_manager.GeneralFree(lessImm->src_i);
            break;
        }
        case EQUZ_I:{
            shared_ptr<EquzI> equzI=static_pointer_cast<EquzI>(lir_instruction_ptr);
            vreg_manager.GeneralFree(equzI->src_i);
            break;
        }
        case NEQUZ_I:{
            shared_ptr<NequzI> nequzI=static_pointer_cast<NequzI>(lir_instruction_ptr);
            vreg_manager.GeneralFree(nequzI->src_i);
            break;
        }
        case LESSZ_I:{
            shared_ptr<LesszI> lesszI=static_pointer_cast<LesszI>(lir_instruction_ptr);
            vreg_manager.GeneralFree(lesszI->src_i);
            break;
        }
        case GREZ_I:{
            shared_ptr<GrezI> grezI=static_pointer_cast<GrezI>(lir_instruction_ptr);
            vreg_manager.GeneralFree(grezI->src_i);
            break;
        }
        //Memory
        case LOAD_I:{
            shared_ptr<LoadI> loadI=static_pointer_cast<LoadI>(lir_instruction_ptr);
            vreg_manager.GeneralFree(loadI->src_i);
            break;
        }
        case LOAD_L:{
            shared_ptr<LoadL> loadL=static_pointer_cast<LoadL>(lir_instruction_ptr);
            vreg_manager.GeneralFree(loadL->src_i);
            break;
        }
        case STORE_I:{
            shared_ptr<StoreI> storeI=static_pointer_cast<StoreI>(lir_instruction_ptr);
            vreg_manager.GeneralFree(storeI->src_i);
            vreg_manager.GeneralFree(storeI->des_i);
            break;
        }
        case STORE_L:{
            shared_ptr<StoreL> storeL=static_pointer_cast<StoreL>(lir_instruction_ptr);
            vreg_manager.GeneralFree(storeL->src_i);
            vreg_manager.GeneralFree(storeL->des_i);
            break;
        }

        //Float
        //Arithmetic
        case ADD_F:{
            shared_ptr<AddF> addF=static_pointer_cast<AddF>(lir_instruction_ptr);
            vreg_manager.GeneralFree(addF->src_f1);
            vreg_manager.GeneralFree(addF->src_f2);
            break;
        }
        case SUB_F:{
            shared_ptr<SubF> subF=static_pointer_cast<SubF>(lir_instruction_ptr);
            vreg_manager.GeneralFree(subF->src_f1);
            vreg_manager.GeneralFree(subF->src_f2);
            break;
        }
        case MUL_F:{
            shared_ptr<MulF> mulF=static_pointer_cast<MulF>(lir_instruction_ptr);
            vreg_manager.GeneralFree(mulF->src_f1);
            vreg_manager.GeneralFree(mulF->src_f2);
            break;
        }
        case DIV_F:{
            shared_ptr<DivF> divF=static_pointer_cast<DivF>(lir_instruction_ptr);
            vreg_manager.GeneralFree(divF->src_f1);
            vreg_manager.GeneralFree(divF->src_f2);
            break;
        }
        case NEG_F:{
            shared_ptr<NegF> negF=static_pointer_cast<NegF>(lir_instruction_ptr);
            vreg_manager.GeneralFree(negF->src_f);
            break;
        }
        //Compare
        case EQU_F:{
            shared_ptr<EquF> equF=static_pointer_cast<EquF>(lir_instruction_ptr);
            vreg_manager.GeneralFree(equF->src_f1);
            vreg_manager.GeneralFree(equF->src_f2);
            break;
        }
        case LEQU_F:{
            shared_ptr<LequF> lequF=static_pointer_cast<LequF>(lir_instruction_ptr);
            vreg_manager.GeneralFree(lequF->src_f1);
            vreg_manager.GeneralFree(lequF->src_f2);
            break;
        }
        case LESS_F:{
            shared_ptr<LessF> lessF=static_pointer_cast<LessF>(lir_instruction_ptr);
            vreg_manager.GeneralFree(lessF->src_f1);
            vreg_manager.GeneralFree(lessF->src_f2);
            break;
        }
        //Memory
        case LOAD_F:{
            shared_ptr<LoadF> loadF=static_pointer_cast<LoadF>(lir_instruction_ptr);
            vreg_manager.GeneralFree(loadF->src_i);
            break;
        }
        case STORE_F:{
            shared_ptr<StoreF> storeF=static_pointer_cast<StoreF>(lir_instruction_ptr);
            vreg_manager.GeneralFree(storeF->src_f);
            vreg_manager.GeneralFree(storeF->des_i);
            break;
        }
        
        //Share
        //Move
        case MOVE_I:{
            shared_ptr<MoveI> moveI=static_pointer_cast<MoveI>(lir_instruction_ptr);
            vreg_manager.GeneralFree(moveI->src_i);
            break;
        }
        case MOVE_F:{
            shared_ptr<MoveF> moveF=static_pointer_cast<MoveF>(lir_instruction_ptr);
            vreg_manager.GeneralFree(moveF->src_f);
            break;
        }
        case MOVE_I2F:{
            shared_ptr<MoveI2F> moveI2F=static_pointer_cast<MoveI2F>(lir_instruction_ptr);
            vreg_manager.GeneralFree(moveI2F->src_i);
            break;
        }
        case MOVE_F2I:{
            shared_ptr<MoveF2I> moveF2I=static_pointer_cast<MoveF2I>(lir_instruction_ptr);
            vreg_manager.GeneralFree(moveF2I->src_f);
            break;
        }

        //Control flow
        case BNEZ:{
            shared_ptr<Bnez> bnez=static_pointer_cast<Bnez>(lir_instruction_ptr);
            vreg_manager.GeneralFree(bnez->src_i);
            break;
        }

        //Convert
        case CVT_I2F:{
            shared_ptr<CvtI2F> cvtI2F=static_pointer_cast<CvtI2F>(lir_instruction_ptr);
            vreg_manager.GeneralFree(cvtI2F->src_i);
            break;
        }
        case CVT_F2I:{
            shared_ptr<CvtF2I> cvtF2I=static_pointer_cast<CvtF2I>(lir_instruction_ptr);
            vreg_manager.GeneralFree(cvtF2I->src_f);
            break;
        }

        //Custom
        case PUTI:{
            shared_ptr<PutI> putI=static_pointer_cast<PutI>(lir_instruction_ptr);
            vreg_manager.GeneralFree(putI->src_i);
            break;
        }
        case PUTF:{
            shared_ptr<PutF> putF=static_pointer_cast<PutF>(lir_instruction_ptr);
            vreg_manager.GeneralFree(putF->src_f);
            break;
        }
    }
}

VirtualRegister LIREmitter::Load(shared_ptr<Value>& value)
{
    VirtualRegister result_vreg;

    //Variable
    if(value->IsValueVariable())
    {
        shared_ptr<ValueVariable> value_variable=GetValueVariablePtr(value);

        //Global
        if(value_variable->store_type==GLOBAL)
        {   
            //Load global variable address
            shared_ptr<LoadA> loadA=make_shared<LoadA>();
            loadA->src_globalvar=mir.global_variables[value_variable->number].name;
            loadA->des_i=vreg_manager.NewVRegR(8);
            AddInsturction(loadA);

            if(value_variable->IsNormalVar())
            {
                if(value_variable->data_type==I32)
                {
                    shared_ptr<LoadI> loadI=make_shared<LoadI>();
                    loadI->src_i=loadA->des_i;
                    loadI->des_i=vreg_manager.NewVRegR(4);
                    AddInsturction(loadI);

                    result_vreg=loadI->des_i;
                }
                else if(value_variable->data_type==F32)
                {
                    shared_ptr<LoadF> loadF=make_shared<LoadF>();
                    loadF->src_i=loadA->des_i;
                    loadF->des_f=vreg_manager.NewVRegF(4);
                    AddInsturction(loadF);

                    result_vreg=loadF->des_f;
                }
            }
            else if(value_variable->IsPointer())
            {
                result_vreg=loadA->des_i;
            }
            else if(value_variable->IsArrayElem())
            {
                shared_ptr<LoadImm> loadImm=make_shared<LoadImm>();
                loadImm->src_imm="4";
                loadImm->des_i=vreg_manager.NewVRegR(4);
                AddInsturction(loadImm);

                shared_ptr<MulI> mulI=make_shared<MulI>();
                mulI->src_i1=Load(value_variable->index);//Recursive
                mulI->src_i2=loadImm->des_i;
                mulI->des_i=vreg_manager.NewVRegR(8);
                AddInsturction(mulI);

                shared_ptr<AddL> addL=make_shared<AddL>();
                addL->src_i1=loadA->des_i;
                addL->src_i2=mulI->des_i;
                addL->des_i=vreg_manager.NewVRegR(8);
                AddInsturction(addL);

                if(value_variable->data_type==I32)
                {
                    shared_ptr<LoadI> loadI=make_shared<LoadI>();
                    loadI->src_i=addL->des_i;
                    loadI->des_i=vreg_manager.NewVRegR(4);
                    AddInsturction(loadI);

                    result_vreg=loadI->des_i;
                }
                else if(value_variable->data_type==F32)
                {
                    shared_ptr<LoadF> loadF=make_shared<LoadF>();
                    loadF->src_i=addL->des_i;
                    loadF->des_f=vreg_manager.NewVRegF(4);
                    AddInsturction(loadF);
                    
                    result_vreg=loadF->des_f;
                }
            }   
        }

        //Local
        else if(value_variable->store_type==LOCAL)
        {
            if(value_variable->IsNormalVar())
            {
                if(value_variable->data_type==I32)
                {
                    shared_ptr<GetI> getI=make_shared<GetI>();
                    getI->src_localvar_number=value_variable->number;
                    getI->des_i=vreg_manager.NewVRegR(4);
                    AddInsturction(getI);

                    result_vreg=getI->des_i;
                }   
                else if(value_variable->data_type==F32)
                {
                    shared_ptr<GetF> getF=make_shared<GetF>();
                    getF->src_localvar_number=value_variable->number;
                    getF->des_f=vreg_manager.NewVRegF(4);
                    AddInsturction(getF);

                    result_vreg=getF->des_f;
                }
            }
            else if(value_variable->IsPointer())
            {
                shared_ptr<GetI> getI=make_shared<GetI>();
                getI->src_localvar_number=value_variable->number;
                getI->des_i=vreg_manager.NewVRegR(8);
                AddInsturction(getI);

                result_vreg=getI->des_i;
            }
            else if(value_variable->IsArrayElem())
            {
                shared_ptr<LoadImm> loadImm=make_shared<LoadImm>();
                loadImm->src_imm="4";
                loadImm->des_i=vreg_manager.NewVRegR(4);
                AddInsturction(loadImm);

                shared_ptr<MulI> mulI=make_shared<MulI>();
                mulI->src_i1=Load(value_variable->index);//Recursive
                mulI->src_i2=loadImm->des_i;
                mulI->des_i=vreg_manager.NewVRegR(8);
                AddInsturction(mulI);

                shared_ptr<GetI> getI=make_shared<GetI>();
                getI->src_localvar_number=value_variable->number;
                getI->des_i=vreg_manager.NewVRegR(8);
                AddInsturction(getI);

                shared_ptr<AddL> addL=make_shared<AddL>();
                addL->src_i1=getI->des_i;
                addL->src_i2=mulI->des_i;
                addL->des_i=vreg_manager.NewVRegR(8);
                AddInsturction(addL);

                if(value_variable->data_type==I32)
                {
                    shared_ptr<LoadI> loadI=make_shared<LoadI>();
                    loadI->src_i=addL->des_i;
                    loadI->des_i=vreg_manager.NewVRegR(4);
                    AddInsturction(loadI);

                    result_vreg=loadI->des_i;
                }
                else if(value_variable->data_type==F32)
                {
                    shared_ptr<LoadF> loadF=make_shared<LoadF>();
                    loadF->src_i=addL->des_i;
                    loadF->des_f=vreg_manager.NewVRegF(4);
                    AddInsturction(loadF);
                    
                    result_vreg=loadF->des_f;
                }
            }
        }
    }

    //Constant
    else if(value->IsValueConstant())
    {
        shared_ptr<ValueConstant> value_constant=GetValueConstantPtr(value);

        shared_ptr<LoadImm> loadImm=make_shared<LoadImm>();
        loadImm->src_imm=value_constant->GetAsmStr();
        loadImm->des_i=vreg_manager.NewVRegR(4);
        AddInsturction(loadImm);

        result_vreg=loadImm->des_i;

        //Float immediate
        if(value_constant->data_type==F32)
        {
            shared_ptr<MoveI2F> moveI2F=make_shared<MoveI2F>();
            moveI2F->src_i=loadImm->des_i;
            moveI2F->des_f=vreg_manager.NewVRegF(4);
            AddInsturction(moveI2F);

            result_vreg=moveI2F->des_f;
        }
    }

    return result_vreg;
}

void LIREmitter::Store(VirtualRegister result_reg,shared_ptr<ValueVariable>& value_variable)
{
    //Global
    if(value_variable->store_type==GLOBAL)
    {
        //Load global variable address
        shared_ptr<LoadA> loadA=make_shared<LoadA>();
        loadA->src_globalvar=mir.global_variables[value_variable->number].name;
        loadA->des_i=vreg_manager.NewVRegR(8);
        AddInsturction(loadA);

        if(value_variable->IsNormalVar())
        {
            if(value_variable->data_type==I32)
            {
                shared_ptr<StoreI> storeI=make_shared<StoreI>();
                storeI->src_i=result_reg;
                storeI->des_i=loadA->des_i;
                AddInsturction(storeI);
            }
            else if(value_variable->data_type==F32)
            {
                shared_ptr<StoreF> storeF=make_shared<StoreF>();
                storeF->src_f=result_reg;
                storeF->des_i=loadA->des_i;
                AddInsturction(storeF);
            }
        }
        else if(value_variable->IsPointer())
        {
            CompilerError("No global pointer be stored");
        }
        else if(value_variable->IsArrayElem())
        {
            shared_ptr<LoadImm> loadImm=make_shared<LoadImm>();
            loadImm->src_imm="4";
            loadImm->des_i=vreg_manager.NewVRegR(4);
            AddInsturction(loadImm);

            shared_ptr<MulI> mulI=make_shared<MulI>();
            mulI->src_i1=Load(value_variable->index);//Recursive
            mulI->src_i2=loadImm->des_i;
            mulI->des_i=vreg_manager.NewVRegR(8);
            AddInsturction(mulI);

            shared_ptr<AddL> addL=make_shared<AddL>();
            addL->src_i1=loadA->des_i;
            addL->src_i2=mulI->des_i;
            addL->des_i=vreg_manager.NewVRegR(8);
            AddInsturction(addL);

            if(value_variable->data_type==I32)
            {
                shared_ptr<StoreI> storeI=make_shared<StoreI>();
                storeI->src_i=result_reg;
                storeI->des_i=addL->des_i;
                AddInsturction(storeI);
            }
            else if(value_variable->data_type==F32)
            {
                shared_ptr<StoreF> storeF=make_shared<StoreF>();
                storeF->src_f=result_reg;
                storeF->des_i=addL->des_i;
                AddInsturction(storeF);
            }
        }
    }

    //Local
    else if(value_variable->store_type==LOCAL)
    {
        if(value_variable->IsNormalVar())
        {
            if(value_variable->data_type==I32)
            {
                shared_ptr<PutI> putI=make_shared<PutI>();
                putI->src_i=result_reg;
                putI->des_localvar_number=value_variable->number;
                AddInsturction(putI);
            }
            else if(value_variable->data_type==F32)
            {
                shared_ptr<PutF> putF=make_shared<PutF>();
                putF->src_f=result_reg;
                putF->des_localvar_number=value_variable->number;
                AddInsturction(putF);
            }
        }
        else if(value_variable->IsPointer())
        {
            shared_ptr<PutI> putI=make_shared<PutI>();
            putI->src_i=result_reg;
            putI->des_localvar_number=value_variable->number;
            AddInsturction(putI);
        }
        else if(value_variable->IsArrayElem())
        {
            shared_ptr<LoadImm> loadImm=make_shared<LoadImm>();
            loadImm->src_imm="4";
            loadImm->des_i=vreg_manager.NewVRegR(4);
            AddInsturction(loadImm);

            shared_ptr<MulI> mulI=make_shared<MulI>();
            mulI->src_i1=Load(value_variable->index);//Recursive
            mulI->src_i2=loadImm->des_i;
            mulI->des_i=vreg_manager.NewVRegR(8);
            AddInsturction(mulI);

            shared_ptr<GetI> getI=make_shared<GetI>();
            getI->src_localvar_number=value_variable->number;
            getI->des_i=vreg_manager.NewVRegR(8);
            AddInsturction(getI);

            shared_ptr<AddL> addL=make_shared<AddL>();
            addL->src_i1=getI->des_i;
            addL->src_i2=mulI->des_i;
            addL->des_i=vreg_manager.NewVRegR(8);
            AddInsturction(addL);

            if(value_variable->data_type==I32)
            {
                shared_ptr<StoreI> storeI=make_shared<StoreI>();
                storeI->src_i=result_reg;
                storeI->des_i=addL->des_i;
                AddInsturction(storeI);
            }
            else if(value_variable->data_type==F32)
            {
                shared_ptr<StoreF> storeF=make_shared<StoreF>();
                storeF->src_f=result_reg;
                storeF->des_i=addL->des_i;
                AddInsturction(storeF);
            }
        }
    }
}
