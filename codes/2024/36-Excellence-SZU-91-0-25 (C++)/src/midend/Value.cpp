#include "../midend/Value.h"

bool Value::IsValueConstant()
{
    return value_type==CONSTANT;
}

bool Value::IsValueVariable()
{
    return value_type==VARIABLE;
}



bool ValueVariable::IsNormalVar()
{
    return (data_type==I32||data_type==F32)&&!IsArrayElem();
}

bool ValueVariable::IsArrayElem()
{
    if(index)return true;
    return false;
}

bool ValueVariable::IsPointer()
{
    return data_type==I32_PTR||data_type==F32_PTR;
}

string ValueVariable::GetStrNoType()
{
    string value_variable_str;
    
    value_variable_str+=StoreTypeMark[store_type];
    value_variable_str+=to_string(number);

    if(index)
        value_variable_str+="["+index->GetStrNoType()+"]";

    return value_variable_str;
}

string ValueVariable::GetStrWithType()
{
    return GetStrNoType()+":"+DataTypeText[data_type];
}



void ValueConstant::Convert(DataType convert_to)
{   
    switch(convert_to)
    {
        case I32:{
            if(data_type==F32)
            {
                data_type=I32;
                constant_value.i32=(int)constant_value.f32;
            }
            break;
        }

        case F32:{
            if(data_type==I32)
            {
                data_type=F32;
                constant_value.f32=(float)constant_value.i32;
            }
            break;
        }

        default:CompilerError("Invalid data type in ValueConstant::Convert()"); 
    }
}

string ValueConstant::GetStrNoType()
{
    string value_constant_str;

    switch(data_type)
    {
        case I32:value_constant_str=to_string(constant_value.i32);break;
        case F32:value_constant_str=to_string(constant_value.f32);break;

        default:CompilerError("Invalid data type in ValueConstant::GetStrNoType()"); 
    }

    return value_constant_str;
}

string ValueConstant::GetAsmStr()
{
    string asm_str;

    if(data_type==I32)asm_str=to_string(constant_value.i32);
    else if(data_type==F32)
    {
        uint32_t intRepresentation = *reinterpret_cast<uint32_t*>(&constant_value.f32);
        stringstream ss;
        ss << "0x" << hex << setfill('0') << setw(8) << intRepresentation;
        asm_str=ss.str();
    }
    else CompilerError("Invalid data type in ValueConstant::GetAsmStr()");
    
    return asm_str;
} 


string ValueConstant::GetStrWithType()
{
    return GetStrNoType()+":"+DataTypeText[data_type];
}



DataType ValueDataType(shared_ptr<Value>& value)
{
    DataType value_data_type;
    
    switch(value->value_type)
    {
        case VARIABLE:
            value_data_type=static_pointer_cast<ValueVariable>(value)->data_type;
            break;
        case CONSTANT:
            value_data_type=static_pointer_cast<ValueConstant>(value)->data_type;
            break;
    }

    return value_data_type;
}



shared_ptr<ValueConstant> GetValueConstantPtr(shared_ptr<Value>& value)
{
    return static_pointer_cast<ValueConstant>(value);
}

shared_ptr<ValueVariable> GetValueVariablePtr(shared_ptr<Value>& value)
{
    return static_pointer_cast<ValueVariable>(value);
}

bool IsEqualValue(shared_ptr<Value>& value1,shared_ptr<Value>& value2)
{
    if(value1==nullptr&&value2==nullptr)return true;
    else if(value1==nullptr&&value2!=nullptr)return false;
    else if(value1!=nullptr&&value2==nullptr)return false;
    else if(value1!=nullptr&&value2!=nullptr)
    {
        if(value1->IsValueConstant()&&value2->IsValueConstant())
        {
            shared_ptr<ValueConstant> value1_constant=GetValueConstantPtr(value1);
            shared_ptr<ValueConstant> value2_constant=GetValueConstantPtr(value2);
            return *value1_constant==*value2_constant;
        }
        else if(value1->IsValueVariable()&&value2->IsValueVariable())
        {
            shared_ptr<ValueVariable> value1_variable=GetValueVariablePtr(value1);
            shared_ptr<ValueVariable> value2_variable=GetValueVariablePtr(value2);
            return *value1_variable==*value2_variable; 
        }
        else return false;
    }

    return false;
}

bool Is1(shared_ptr<Value>& value)
{
    if(value->IsValueConstant())
    {
        shared_ptr<ValueConstant> value_constant=GetValueConstantPtr(value);
        return value_constant->data_type==I32&&value_constant->constant_value.i32==1;
    }
    return false;
}

bool Is0(shared_ptr<Value>& value)
{
    if(value->IsValueConstant())
    {
        shared_ptr<ValueConstant> value_constant=GetValueConstantPtr(value);
        return value_constant->data_type==I32&&value_constant->constant_value.i32==0;
    }
    return false;
}
