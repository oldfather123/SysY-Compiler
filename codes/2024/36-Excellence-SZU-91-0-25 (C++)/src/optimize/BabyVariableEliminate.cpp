#include "../optimize/Optimizer.h"
#include "../midend/MIR.h"
#include "../utility/Product.h"

class LocalVarUseTable
{
private:
    unordered_set<int> table;

public:
    void AddUseRight(shared_ptr<Value> value);
    void AddUseLeft(shared_ptr<ValueVariable> value_variable);
    bool IsUsed(shared_ptr<ValueVariable>& value_variable);
};

void LocalVarUseTable::AddUseRight(shared_ptr<Value> value)
{
    if(value->IsValueVariable())
    {
        shared_ptr<ValueVariable> value_variable=GetValueVariablePtr(value);

        if(value_variable->store_type==LOCAL)
            table.insert(value_variable->number);
        
        //Recursive
        if(value_variable->index)
            AddUseRight(value_variable->index);
    }
}

void LocalVarUseTable::AddUseLeft(shared_ptr<ValueVariable> value_variable)
{
    if(value_variable->index)
        AddUseRight(value_variable->index);
}

bool LocalVarUseTable::IsUsed(shared_ptr<ValueVariable>& value_variable)
{
    if(value_variable->store_type==LOCAL
      &&table.find(value_variable->number)==table.end())
        return false;

    return true;
}



void Optimizer::BabyVariableEliminate()
{
    for(MIRFunction& mir_function:mir.functions)
    {
        LocalVarUseTable table;

        //Gather information
        for(MIRBasicBlock& mir_block:mir_function.basic_blocks)
        {
            for(shared_ptr<MIRInstruction>& mir_instruction_ptr:mir_block.instructions)
            {
                switch(mir_instruction_ptr->mir_instruction_type)
                {
                    //Copy
                    case COPY:{
                        shared_ptr<Copy> copy=static_pointer_cast<Copy>(mir_instruction_ptr);
                        table.AddUseRight(copy->value);
                        table.AddUseLeft(copy->variable);
                        break;
                    }

                    //Control flow
                    case BR:{
                        shared_ptr<Branch> branch=static_pointer_cast<Branch>(mir_instruction_ptr);
                        table.AddUseRight(branch->condition);
                        break;
                    }
                    case CALLL:{
                        shared_ptr<Calll> call=static_pointer_cast<Calll>(mir_instruction_ptr);
                        for(shared_ptr<Value>& argument:call->arguments)
                            table.AddUseRight(argument);
                        if(call->get_retvalue)
                            table.AddUseLeft(call->get_retvalue);
                        break;
                    }
                    case RETT:{
                        shared_ptr<Return> ret=static_pointer_cast<Return>(mir_instruction_ptr);
                        if(ret->return_value)
                            table.AddUseRight(ret->return_value);
                        break;
                    }

                    //Arithmetic
                    case OR:{
                        shared_ptr<Or> _or=static_pointer_cast<Or>(mir_instruction_ptr);
                        table.AddUseRight(_or->a);
                        table.AddUseRight(_or->b);
                        table.AddUseLeft(_or->c);
                        break;
                    }
                    case AND:{
                        shared_ptr<And> _and=static_pointer_cast<And>(mir_instruction_ptr);
                        table.AddUseRight(_and->a);
                        table.AddUseRight(_and->b);
                        table.AddUseLeft(_and->c);
                        break;
                    }
                    case NOT:{
                        shared_ptr<Not> _not=static_pointer_cast<Not>(mir_instruction_ptr);
                        table.AddUseRight(_not->a);
                        table.AddUseLeft(_not->b);
                        break;
                    }
                    case NEG:{
                        shared_ptr<Negative> neg=static_pointer_cast<Negative>(mir_instruction_ptr);
                        table.AddUseRight(neg->a);
                        table.AddUseLeft(neg->b);
                        break;
                    }
                    case EQU:{
                        shared_ptr<Equal> equal=static_pointer_cast<Equal>(mir_instruction_ptr);
                        table.AddUseRight(equal->a);
                        table.AddUseRight(equal->b);
                        table.AddUseLeft(equal->c);
                        break;
                    }
                    case NEQU:{
                        shared_ptr<NotEqual> not_equal=static_pointer_cast<NotEqual>(mir_instruction_ptr);
                        table.AddUseRight(not_equal->a);
                        table.AddUseRight(not_equal->b);
                        table.AddUseLeft(not_equal->c);
                        break;
                    }
                    case LES:{
                        shared_ptr<Less> less=static_pointer_cast<Less>(mir_instruction_ptr);
                        table.AddUseRight(less->a);
                        table.AddUseRight(less->b);
                        table.AddUseLeft(less->c);
                        break;
                    }
                    case LEQU:{
                        shared_ptr<LessEqual> less_equal=static_pointer_cast<LessEqual>(mir_instruction_ptr);
                        table.AddUseRight(less_equal->a);
                        table.AddUseRight(less_equal->b);
                        table.AddUseLeft(less_equal->c);
                        break;
                    }
                    case GRE:{
                        shared_ptr<Greater> greater=static_pointer_cast<Greater>(mir_instruction_ptr);
                        table.AddUseRight(greater->a);
                        table.AddUseRight(greater->b);
                        table.AddUseLeft(greater->c);
                        break;
                    }
                    case GEQU:{
                        shared_ptr<GreaterEqual> greater_equal=static_pointer_cast<GreaterEqual>(mir_instruction_ptr);
                        table.AddUseRight(greater_equal->a);
                        table.AddUseRight(greater_equal->b);
                        table.AddUseLeft(greater_equal->c);
                        break;
                    }
                    case ADD:{
                        shared_ptr<Add> add=static_pointer_cast<Add>(mir_instruction_ptr);
                        table.AddUseRight(add->a);
                        table.AddUseRight(add->b);
                        table.AddUseLeft(add->c);
                        break;
                    }
                    case SUB:{
                        shared_ptr<Sub> sub=static_pointer_cast<Sub>(mir_instruction_ptr);
                        table.AddUseRight(sub->a);
                        table.AddUseRight(sub->b);
                        table.AddUseLeft(sub->c);
                        break;
                    }
                    case MUL:{
                        shared_ptr<Mul> mul=static_pointer_cast<Mul>(mir_instruction_ptr);
                        table.AddUseRight(mul->a);
                        table.AddUseRight(mul->b);
                        table.AddUseLeft(mul->c);
                        break;
                    }
                    case DIV:{
                        shared_ptr<Div> div=static_pointer_cast<Div>(mir_instruction_ptr);
                        table.AddUseRight(div->a);
                        table.AddUseRight(div->b);
                        table.AddUseLeft(div->c);
                        break;
                    }
                    case MOD:{
                        shared_ptr<Mod> mod=static_pointer_cast<Mod>(mir_instruction_ptr);
                        table.AddUseRight(mod->a);
                        table.AddUseRight(mod->b);
                        table.AddUseLeft(mod->c);
                        break;
                    }

                    //Convert
                    case I2F:{
                        shared_ptr<ItoF> i2f=static_pointer_cast<ItoF>(mir_instruction_ptr);
                        table.AddUseRight(i2f->a_i);
                        table.AddUseLeft(i2f->b_f);
                        break;
                    }
                    case F2I:{
                        shared_ptr<FtoI> f2i=static_pointer_cast<FtoI>(mir_instruction_ptr);
                        table.AddUseRight(f2i->a_f);
                        table.AddUseLeft(f2i->b_i);
                        break;
                    }
                }
            }
        }

        //Eliminate
        for(MIRBasicBlock& mir_block:mir_function.basic_blocks)
        {
            for(auto iter=mir_block.instructions.begin();
                iter!=mir_block.instructions.end();)
                {
                    if((*iter)->mir_instruction_type==COPY)
                    {
                        shared_ptr<Copy> copy=static_pointer_cast<Copy>((*iter));

                        if(!table.IsUsed(copy->variable))
                            iter=mir_block.instructions.erase(iter);
                        else iter++;
                    }
                    else iter++;
                }
        }
    }
}
