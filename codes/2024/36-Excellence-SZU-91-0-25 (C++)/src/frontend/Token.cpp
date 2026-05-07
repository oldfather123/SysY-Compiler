#include "../frontend/Token.h"
#include "../utility/Diagnostor.h"

DataType Token::ToDataType()
{
    DataType data_type;

    switch(token_type)
    {
        case T_VOID:  data_type=VOID;break;
        case T_INT:   data_type=I32; break;
        case T_FLOAT: data_type=F32; break;
        default:CompilerError("Not exist corresponding data type");
    }

    return data_type;
}