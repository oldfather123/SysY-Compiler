#include "../optimize/Optimizer.h"
#include "../midend/MIR.h"
#include "../utility/Product.h"

void ConstReplaceValue(MIRFunction& in_function,shared_ptr<Value>& value)
{
    if(value->IsValueVariable())
    {
        shared_ptr<ValueVariable> value_variable=GetValueVariablePtr(value);
        Variable& variable=value_variable->store_type==GLOBAL?
            mir.global_variables[value_variable->number]:
            in_function.local_variables[value_variable->number];

        if(variable.is_const)
        {
            //Scalar
            if(variable.data_type==I32||variable.data_type==F32)
            {
                value=make_shared<ValueConstant>(variable.initial[0]);
            }
            //Array elem
            else if(variable.data_type==I32_PTR||variable.data_type==F32_PTR)
            {
                if(value_variable->IsArrayElem())
                {
                    //Maybe index is a const variable
                    ConstReplaceValue(in_function,value_variable->index);

                    if(value_variable->index->IsValueConstant())
                    {
                        shared_ptr<ValueConstant> value_index_constant=
                            GetValueConstantPtr(value_variable->index);
                        value_index_constant->Convert(I32);

                        value=make_shared<ValueConstant>(
                            variable.initial[value_index_constant->constant_value.i32]
                        );
                    }
                }
            }
        }
    }
}

void Optimizer::ConstReplace()
{
    for(MIRFunction& mir_function:mir.functions)
    {
        for(MIRBasicBlock& mir_block:mir_function.basic_blocks)
        {
            for(shared_ptr<MIRInstruction>& mir_instruction_ptr:mir_block.instructions)
            {
                switch(mir_instruction_ptr->mir_instruction_type)
                {
                    //Copy
                    case COPY:{
                        shared_ptr<Copy> copy=static_pointer_cast<Copy>(mir_instruction_ptr);
                        ConstReplaceValue(mir_function,copy->value);
                        break;
                    }

                    //Control flow
                    case BR:{
                        shared_ptr<Branch> branch=static_pointer_cast<Branch>(mir_instruction_ptr);
                        ConstReplaceValue(mir_function,branch->condition);
                        break;
                    }
                    case CALLL:{
                        shared_ptr<Calll> call=static_pointer_cast<Calll>(mir_instruction_ptr);
                        for(shared_ptr<Value>& argument:call->arguments)
                            ConstReplaceValue(mir_function,argument);
                        break;
                    }
                    case RETT:{
                        shared_ptr<Return> ret=static_pointer_cast<Return>(mir_instruction_ptr);
                        if(ret->return_value)
                            ConstReplaceValue(mir_function,ret->return_value);
                        break;
                    }

                    //Arithmetic
                    case OR:{
                        shared_ptr<Or> _or=static_pointer_cast<Or>(mir_instruction_ptr);
                        ConstReplaceValue(mir_function,_or->a);
                        ConstReplaceValue(mir_function,_or->b);
                        break;
                    }
                    case AND:{
                        shared_ptr<And> _and=static_pointer_cast<And>(mir_instruction_ptr);
                        ConstReplaceValue(mir_function,_and->a);
                        ConstReplaceValue(mir_function,_and->b);
                        break;
                    }
                    case NOT:{
                        shared_ptr<Not> _not=static_pointer_cast<Not>(mir_instruction_ptr);
                        ConstReplaceValue(mir_function,_not->a);
                        break;
                    }
                    case NEG:{
                        shared_ptr<Negative> neg=static_pointer_cast<Negative>(mir_instruction_ptr);
                        ConstReplaceValue(mir_function,neg->a);
                        break;
                    }
                    case EQU:{
                        shared_ptr<Equal> equal=static_pointer_cast<Equal>(mir_instruction_ptr);
                        ConstReplaceValue(mir_function,equal->a);
                        ConstReplaceValue(mir_function,equal->b);
                        break;
                    }
                    case NEQU:{
                        shared_ptr<NotEqual> not_equal=static_pointer_cast<NotEqual>(mir_instruction_ptr);
                        ConstReplaceValue(mir_function,not_equal->a);
                        ConstReplaceValue(mir_function,not_equal->b);
                        break;
                    }
                    case LES:{
                        shared_ptr<Less> less=static_pointer_cast<Less>(mir_instruction_ptr);
                        ConstReplaceValue(mir_function,less->a);
                        ConstReplaceValue(mir_function,less->b);
                        break;
                    }
                    case LEQU:{
                        shared_ptr<LessEqual> less_equal=static_pointer_cast<LessEqual>(mir_instruction_ptr);
                        ConstReplaceValue(mir_function,less_equal->a);
                        ConstReplaceValue(mir_function,less_equal->b);
                        break;
                    }
                    case GRE:{
                        shared_ptr<Greater> greater=static_pointer_cast<Greater>(mir_instruction_ptr);
                        ConstReplaceValue(mir_function,greater->a);
                        ConstReplaceValue(mir_function,greater->b);
                        break;
                    }
                    case GEQU:{
                        shared_ptr<GreaterEqual> greater_equal=static_pointer_cast<GreaterEqual>(mir_instruction_ptr);
                        ConstReplaceValue(mir_function,greater_equal->a);
                        ConstReplaceValue(mir_function,greater_equal->b);
                        break;
                    }
                    case ADD:{
                        shared_ptr<Add> add=static_pointer_cast<Add>(mir_instruction_ptr);
                        ConstReplaceValue(mir_function,add->a);
                        ConstReplaceValue(mir_function,add->b);
                        break;
                    }
                    case SUB:{
                        shared_ptr<Sub> sub=static_pointer_cast<Sub>(mir_instruction_ptr);
                        ConstReplaceValue(mir_function,sub->a);
                        ConstReplaceValue(mir_function,sub->b);
                        break;
                    }
                    case MUL:{
                        shared_ptr<Mul> mul=static_pointer_cast<Mul>(mir_instruction_ptr);
                        ConstReplaceValue(mir_function,mul->a);
                        ConstReplaceValue(mir_function,mul->b);
                        break;
                    }
                    case DIV:{
                        shared_ptr<Div> div=static_pointer_cast<Div>(mir_instruction_ptr);
                        ConstReplaceValue(mir_function,div->a);
                        ConstReplaceValue(mir_function,div->b);
                        break;
                    }
                    case MOD:{
                        shared_ptr<Mod> mod=static_pointer_cast<Mod>(mir_instruction_ptr);
                        ConstReplaceValue(mir_function,mod->a);
                        ConstReplaceValue(mir_function,mod->b);
                        break;
                    }

                    //Convert
                    case I2F:{
                        shared_ptr<ItoF> i2f=static_pointer_cast<ItoF>(mir_instruction_ptr);
                        ConstReplaceValue(mir_function,i2f->a_i);
                        break;
                    }
                    case F2I:{
                        shared_ptr<FtoI> f2i=static_pointer_cast<FtoI>(mir_instruction_ptr);
                        ConstReplaceValue(mir_function,f2i->a_f);
                        break;
                    }
                }
            }
        }
    }
}