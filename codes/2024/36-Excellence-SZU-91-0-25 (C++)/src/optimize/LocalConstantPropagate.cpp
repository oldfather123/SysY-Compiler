#include "../optimize/Optimizer.h"
#include "../midend/MIR.h"
#include "../utility/Product.h"

//@Discover:XXX that's wrong now (>_<)
//1.Source variable only be difined value by instruction--copy,
//and will be difined many times;
//2.Intermediate variable will be difined by other instructions,
//and will be difined only once
//3.There is no variable will be difined by copy and other

class LocalVarToConstantTable
{
private: 
    unordered_map<int,shared_ptr<Value>> table;

public:
    void AddConstant(shared_ptr<Copy>& copy);
    void Redefine(shared_ptr<ValueVariable>& value_variable);
    void Propagate(shared_ptr<Value>& value);
};

void LocalVarToConstantTable::AddConstant(shared_ptr<Copy>& copy)
{
    if(copy->variable->IsNormalVar()&&copy->variable->store_type==LOCAL)
    {
        //Define by constant,cover front define
        if(copy->value->IsValueConstant())
            table[copy->variable->number]=copy->value;
        //Define by variable,erase front define
        else table.erase(copy->variable->number);
    }
}

void LocalVarToConstantTable::Redefine(shared_ptr<ValueVariable>& value_variable)
{    
    if(value_variable->IsNormalVar()&&value_variable->store_type==LOCAL)
        table.erase(value_variable->number);
}

void LocalVarToConstantTable::Propagate(shared_ptr<Value>& value)
{
    if(value->IsValueVariable())
    {
        shared_ptr<ValueVariable> value_variable=GetValueVariablePtr(value);
        
        if(value_variable->IsNormalVar()&&value_variable->store_type==LOCAL)
        {
            if(table.find(value_variable->number)!=table.end())
                value=table[value_variable->number];
        }

        //Recursive propagate array index
        if(value_variable->index)
            Propagate(value_variable->index);
    }
}



void Optimizer::LocalConstantPropagate()
{
    for(MIRFunction& mir_function:mir.functions)
    {
        for(MIRBasicBlock& mir_block:mir_function.basic_blocks)
        {
            LocalVarToConstantTable table;

            for(shared_ptr<MIRInstruction>& mir_instruction_ptr:mir_block.instructions)
            {
                switch(mir_instruction_ptr->mir_instruction_type)
                {
                    //Copy
                    case COPY:{
                        shared_ptr<Copy> copy=static_pointer_cast<Copy>(mir_instruction_ptr);
                        table.Propagate(copy->value);
                        table.AddConstant(copy);
                        break;
                    }

                    //Control flow
                    case BR:{
                        shared_ptr<Branch> branch=static_pointer_cast<Branch>(mir_instruction_ptr);
                        table.Propagate(branch->condition);
                        break;
                    }
                    case CALLL:{
                        shared_ptr<Calll> call=static_pointer_cast<Calll>(mir_instruction_ptr);
                        for(shared_ptr<Value>& argument:call->arguments)
                            table.Propagate(argument);
                        if(call->get_retvalue)
                            table.Redefine(call->get_retvalue);
                        break;
                    }
                    case RETT:{
                        shared_ptr<Return> ret=static_pointer_cast<Return>(mir_instruction_ptr);
                        if(ret->return_value)
                            table.Propagate(ret->return_value);
                        break;
                    }

                    //Arithmetic
                    case OR:{
                        shared_ptr<Or> _or=static_pointer_cast<Or>(mir_instruction_ptr);
                        table.Propagate(_or->a);
                        table.Propagate(_or->b);
                        table.Redefine(_or->c);
                        break;
                    }
                    case AND:{
                        shared_ptr<And> _and=static_pointer_cast<And>(mir_instruction_ptr);
                        table.Propagate(_and->a);
                        table.Propagate(_and->b);
                        table.Redefine(_and->c);
                        break;
                    }
                    case NOT:{
                        shared_ptr<Not> _not=static_pointer_cast<Not>(mir_instruction_ptr);
                        table.Propagate(_not->a);
                        table.Redefine(_not->b);
                        break;
                    }
                    case NEG:{
                        shared_ptr<Negative> neg=static_pointer_cast<Negative>(mir_instruction_ptr);
                        table.Propagate(neg->a);
                        table.Redefine(neg->b);
                        break;
                    }
                    case EQU:{
                        shared_ptr<Equal> equal=static_pointer_cast<Equal>(mir_instruction_ptr);
                        table.Propagate(equal->a);
                        table.Propagate(equal->b);
                        table.Redefine(equal->c);
                        break;
                    }
                    case NEQU:{
                        shared_ptr<NotEqual> not_equal=static_pointer_cast<NotEqual>(mir_instruction_ptr);
                        table.Propagate(not_equal->a);
                        table.Propagate(not_equal->b);
                        table.Redefine(not_equal->c);
                        break;
                    }
                    case LES:{
                        shared_ptr<Less> less=static_pointer_cast<Less>(mir_instruction_ptr);
                        table.Propagate(less->a);
                        table.Propagate(less->b);
                        table.Redefine(less->c);
                        break;
                    }
                    case LEQU:{
                        shared_ptr<LessEqual> less_equal=static_pointer_cast<LessEqual>(mir_instruction_ptr);
                        table.Propagate(less_equal->a);
                        table.Propagate(less_equal->b);
                        table.Redefine(less_equal->c);
                        break;
                    }
                    case GRE:{
                        shared_ptr<Greater> greater=static_pointer_cast<Greater>(mir_instruction_ptr);
                        table.Propagate(greater->a);
                        table.Propagate(greater->b);
                        table.Redefine(greater->c);
                        break;
                    }
                    case GEQU:{
                        shared_ptr<GreaterEqual> greater_equal=static_pointer_cast<GreaterEqual>(mir_instruction_ptr);
                        table.Propagate(greater_equal->a);
                        table.Propagate(greater_equal->b);
                        table.Redefine(greater_equal->c);
                        break;
                    }
                    case ADD:{
                        shared_ptr<Add> add=static_pointer_cast<Add>(mir_instruction_ptr);
                        table.Propagate(add->a);
                        table.Propagate(add->b);
                        table.Redefine(add->c);
                        break;
                    }
                    case SUB:{
                        shared_ptr<Sub> sub=static_pointer_cast<Sub>(mir_instruction_ptr);
                        table.Propagate(sub->a);
                        table.Propagate(sub->b);
                        table.Redefine(sub->c);
                        break;
                    }
                    case MUL:{
                        shared_ptr<Mul> mul=static_pointer_cast<Mul>(mir_instruction_ptr);
                        table.Propagate(mul->a);
                        table.Propagate(mul->b);
                        table.Redefine(mul->c);
                        break;
                    }
                    case DIV:{
                        shared_ptr<Div> div=static_pointer_cast<Div>(mir_instruction_ptr);
                        table.Propagate(div->a);
                        table.Propagate(div->b);
                        table.Redefine(div->c);
                        break;
                    }
                    case MOD:{
                        shared_ptr<Mod> mod=static_pointer_cast<Mod>(mir_instruction_ptr);
                        table.Propagate(mod->a);
                        table.Propagate(mod->b);
                        table.Redefine(mod->c);
                        break;
                    }

                    //Convert
                    case I2F:{
                        shared_ptr<ItoF> i2f=static_pointer_cast<ItoF>(mir_instruction_ptr);
                        table.Propagate(i2f->a_i);
                        table.Redefine(i2f->b_f);
                        break;
                    }
                    case F2I:{
                        shared_ptr<FtoI> f2i=static_pointer_cast<FtoI>(mir_instruction_ptr);
                        table.Propagate(f2i->a_f);
                        table.Redefine(f2i->b_i);
                        break;
                    }
                }
            }
        }
    }
}
