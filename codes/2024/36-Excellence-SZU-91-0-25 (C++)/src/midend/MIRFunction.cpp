#include "../midend/MIRFunction.h"

Variable& MIRFunction::NewLocalVariable(bool is_const,DataType data_type,string name,vector<int> dimensions)
{
    if(local_var_number>=VAR_SIZE) CompilerError("Too many local varibales!");

    local_variables[local_var_number]=
        Variable(is_const,LOCAL,data_type,name,local_var_number,dimensions,false);
        
    return local_variables[local_var_number++];
}

shared_ptr<ValueVariable> MIRFunction::NewTempLocalVar(DataType data_type)
{
    //Find a free temp var
    for(int i=0;i<local_var_number;i++)
    {
        if(local_variables[i].is_temp&&local_variables[i].is_free&&local_variables[i].data_type==data_type)
        {
            local_variables[i].is_free=false;
            return make_shared<ValueVariable>(LOCAL,data_type,i);
        }
    }

    //If find failed,new one
    if(local_var_number>=VAR_SIZE) CompilerError("Too many local varibales!");

    local_variables[local_var_number]=
        Variable(false,LOCAL,data_type,"",local_var_number,vector<int>(),true);

    return make_shared<ValueVariable>(LOCAL,data_type,local_var_number++);
}

void MIRFunction::FreeTempLocalVar(shared_ptr<Value>& value)
{
    if(value->IsValueVariable())
    {
        shared_ptr<ValueVariable> value_variable=static_pointer_cast<ValueVariable>(value);

        if(value_variable->store_type==LOCAL)
        {
            Variable& local_var=local_variables[value_variable->number];
            if(local_var.is_temp)
            {
                //@Bug
                //if(!local_var.is_free)
                    local_var.is_free=true;
                //else CompilerError("Free a freed temp variable");
            }

            //Recursive
            if(value_variable->index)
                FreeTempLocalVar(value_variable->index);
        }
    }
}

Variable& MIRFunction::NewParameter(DataType data_type,string name,vector<int> dimensions)
{
    Variable& parameter=NewLocalVariable(false,data_type,name,dimensions);
    parameter_number++;
 
    return parameter;
}

int MIRFunction::NewBasicBlock()
{
    int block_number=basic_blocks.size();
    basic_blocks.push_back(MIRBasicBlock(block_number));

    return block_number;
}



MIRBasicBlock& MIRFunction::GetBasicBlock(int block_number)
{
    return basic_blocks[block_number];
}   

MIRBasicBlock& MIRFunction::GetEntryBasicBlock()
{
    return basic_blocks[entry_block_number];
}

MIRBasicBlock& MIRFunction::GetExitBasicBlock()
{
    return basic_blocks[exit_block_number];
}



string MIRFunction::GetStrLocalVariables()
{
    string local_variables_str;

    for(int i=0;i<local_var_number;i++)
    {
        local_variables_str+="%";
        local_variables_str+=local_variables[i].GetStr();
        local_variables_str+="\n";
    }
    local_variables_str+="\n";

    return local_variables_str;
}

string MIRFunction::GetStr()
{
    string function_str;

    //Local variables
    if(local_var_number)
        function_str+=GetStrLocalVariables();

    //Name
    function_str+="@"+name+"(";

    //Type
    for(int i=0;i<parameter_number;i++)
    {
        function_str+=DataTypeText[local_variables[i].data_type];
        if(i<parameter_number-1)function_str+=",";
    }
    function_str+=") => "+DataTypeText[return_type]+"\n";

    //Basic blocks
    function_str+="{\n";
    function_str+="entry:";function_str+=basic_blocks[entry_block_number].GetStr();
    for(int i=0;i<basic_blocks.size();i++)
    {
        if(i!=entry_block_number&&i!=exit_block_number&&basic_blocks[i].is_valid)
            function_str+=basic_blocks[i].GetStr();
    }
    function_str+="exit:";function_str+=basic_blocks[exit_block_number].GetStr();
    function_str+="}\n";

    return function_str;
}
