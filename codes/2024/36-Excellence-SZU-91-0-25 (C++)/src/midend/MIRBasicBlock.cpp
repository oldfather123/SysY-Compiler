#include "../midend/MIRBasicBlock.h"
#include "../frontend/MIREmitter.h"

void MIRBasicBlock::AddInsturction(shared_ptr<MIRInstruction> mir_instruction_ptr)
{
    instructions.push_back(mir_instruction_ptr);

    switch(mir_instruction_ptr->mir_instruction_type)
    {
        //Copy
        case COPY:{
            shared_ptr<Copy> copy=static_pointer_cast<Copy>(mir_instruction_ptr);
            mir_emitter.InFunction().FreeTempLocalVar(copy->value);
            break;
        }

        //Control flow
        case BR:{
            shared_ptr<Branch> branch=static_pointer_cast<Branch>(mir_instruction_ptr);
            mir_emitter.InFunction().FreeTempLocalVar(branch->condition);
            break;
        }
        case CALLL:{
            shared_ptr<Calll> call=static_pointer_cast<Calll>(mir_instruction_ptr);
            for(shared_ptr<Value>& argument:call->arguments)
                mir_emitter.InFunction().FreeTempLocalVar(argument);
            break;
        }
        case RETT:{
            shared_ptr<Return> ret=static_pointer_cast<Return>(mir_instruction_ptr);
            if(ret->return_value)
                mir_emitter.InFunction().FreeTempLocalVar(ret->return_value);
            break;
        }

        //Arithmetic
        case OR:{
            shared_ptr<Or> _or=static_pointer_cast<Or>(mir_instruction_ptr);
            mir_emitter.InFunction().FreeTempLocalVar(_or->a);
            mir_emitter.InFunction().FreeTempLocalVar(_or->b);
            break;
        }
        case AND:{
            shared_ptr<And> _and=static_pointer_cast<And>(mir_instruction_ptr);
            mir_emitter.InFunction().FreeTempLocalVar(_and->a);
            mir_emitter.InFunction().FreeTempLocalVar(_and->b);
            break;
        }
        case NOT:{
            shared_ptr<Not> _not=static_pointer_cast<Not>(mir_instruction_ptr);
            mir_emitter.InFunction().FreeTempLocalVar(_not->a);
            break;
        }
        case NEG:{
            shared_ptr<Negative> neg=static_pointer_cast<Negative>(mir_instruction_ptr);
            mir_emitter.InFunction().FreeTempLocalVar(neg->a);
            break;
        }
        case EQU:{
            shared_ptr<Equal> equal=static_pointer_cast<Equal>(mir_instruction_ptr);
            mir_emitter.InFunction().FreeTempLocalVar(equal->a);
            mir_emitter.InFunction().FreeTempLocalVar(equal->b);
            break;
        }
        case NEQU:{
            shared_ptr<NotEqual> not_equal=static_pointer_cast<NotEqual>(mir_instruction_ptr);
            mir_emitter.InFunction().FreeTempLocalVar(not_equal->a);
            mir_emitter.InFunction().FreeTempLocalVar(not_equal->b);
            break;
        }
        case LES:{
            shared_ptr<Less> less=static_pointer_cast<Less>(mir_instruction_ptr);
            mir_emitter.InFunction().FreeTempLocalVar(less->a);
            mir_emitter.InFunction().FreeTempLocalVar(less->b);
            break;
        }
        case LEQU:{
            shared_ptr<LessEqual> less_equal=static_pointer_cast<LessEqual>(mir_instruction_ptr);
            mir_emitter.InFunction().FreeTempLocalVar(less_equal->a);
            mir_emitter.InFunction().FreeTempLocalVar(less_equal->b);
            break;
        }
        case GRE:{
            shared_ptr<Greater> greater=static_pointer_cast<Greater>(mir_instruction_ptr);
            mir_emitter.InFunction().FreeTempLocalVar(greater->a);
            mir_emitter.InFunction().FreeTempLocalVar(greater->b);
            break;
        }
        case GEQU:{
            shared_ptr<GreaterEqual> greater_equal=static_pointer_cast<GreaterEqual>(mir_instruction_ptr);
            mir_emitter.InFunction().FreeTempLocalVar(greater_equal->a);
            mir_emitter.InFunction().FreeTempLocalVar(greater_equal->b);
            break;
        }
        case ADD:{
            shared_ptr<Add> add=static_pointer_cast<Add>(mir_instruction_ptr);
            mir_emitter.InFunction().FreeTempLocalVar(add->a);
            mir_emitter.InFunction().FreeTempLocalVar(add->b);
            break;
        }
        case SUB:{
            shared_ptr<Sub> sub=static_pointer_cast<Sub>(mir_instruction_ptr);
            mir_emitter.InFunction().FreeTempLocalVar(sub->a);
            mir_emitter.InFunction().FreeTempLocalVar(sub->b);
            break;
        }
        case MUL:{
            shared_ptr<Mul> mul=static_pointer_cast<Mul>(mir_instruction_ptr);
            mir_emitter.InFunction().FreeTempLocalVar(mul->a);
            mir_emitter.InFunction().FreeTempLocalVar(mul->b);
            break;
        }
        case DIV:{
            shared_ptr<Div> div=static_pointer_cast<Div>(mir_instruction_ptr);
            mir_emitter.InFunction().FreeTempLocalVar(div->a);
            mir_emitter.InFunction().FreeTempLocalVar(div->b);
            break;
        }
        case MOD:{
            shared_ptr<Mod> mod=static_pointer_cast<Mod>(mir_instruction_ptr);
            mir_emitter.InFunction().FreeTempLocalVar(mod->a);
            mir_emitter.InFunction().FreeTempLocalVar(mod->b);
            break;
        }

        //Convert
        case I2F:{
            shared_ptr<ItoF> i2f=static_pointer_cast<ItoF>(mir_instruction_ptr);
            mir_emitter.InFunction().FreeTempLocalVar(i2f->a_i);
            break;
        }
        case F2I:{
            shared_ptr<FtoI> f2i=static_pointer_cast<FtoI>(mir_instruction_ptr);
            mir_emitter.InFunction().FreeTempLocalVar(f2i->a_f);
            break;
        }
    }
}



void MIRBasicBlock::AddPrecursor(int pre_number)
{
    precursors.insert(pre_number);
}

void MIRBasicBlock::DeletePrecursor(int pre_number)
{
    precursors.erase(pre_number);
}

void MIRBasicBlock::AddSucceed(int suc_number)
{
    succeeds.insert(suc_number);
}

void MIRBasicBlock::DeleteSucceed(int suc_number)
{
    succeeds.erase(suc_number);
}



string MIRBasicBlock::GetStr()
{   
    string basic_block_str;
    
    //Block number
    basic_block_str+=" <"+to_string(number)+">\n";
    
    //Precursor and succeed
    basic_block_str+="  [Precursor] ";for(const int& pre:precursors)basic_block_str+=to_string(pre)+" ";
    basic_block_str+="[Succeed] ";for(const int& suc:succeeds)basic_block_str+=to_string(suc)+" ";
    basic_block_str+="\n";
    
    //Instructions
    for(shared_ptr<MIRInstruction>& instruction_ptr:instructions)
    {
        basic_block_str+="    ";
        basic_block_str+=instruction_ptr->GetStr();
        basic_block_str+="\n";
    }
    basic_block_str+="\n";

    return basic_block_str;
}