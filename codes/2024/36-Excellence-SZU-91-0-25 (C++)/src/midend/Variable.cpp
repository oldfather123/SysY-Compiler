#include "../midend/Variable.h"

void Variable::Initialize()
{
    DataType elem_data_type=data_type;
    if(elem_data_type==I32_PTR) elem_data_type=I32;
    else if(elem_data_type==F32_PTR) elem_data_type=F32;

    initial.resize(length,ValueConstant(elem_data_type));
}



bool Variable::IsNormalVar()
{
    return data_type==I32||data_type==F32;
}

bool Variable::IsArrayPointer()
{
    return (data_type==I32_PTR||data_type==F32_PTR)&&length!=0;
}

bool Variable::IsNormalPointer()
{
    return (data_type==I32_PTR||data_type==F32_PTR)&&length==0;
}



string Variable::GetStr()
{
    string variable_str;

    variable_str+=to_string(number);
    variable_str+="("+name+") ";
    variable_str+=DataTypeText[data_type]+" ";
    variable_str+="const:"+to_string(is_const)+" ";
    variable_str+="length:"+to_string(length)+" ";

    if(!initial.empty())
    {
        variable_str+="initial:";
        for(ValueConstant& value_constant:initial)
            variable_str+=value_constant.GetStrNoType()+" ";
    }
    
    return variable_str;
}
