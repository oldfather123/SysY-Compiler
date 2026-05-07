#include "../midend/MIRInstruction.h"

//Copy
string Copy::GetStr()
{
    string instruction_str;

    instruction_str+="copy ";
    instruction_str+=value->GetStrWithType();
    instruction_str+=" => ";
    instruction_str+=variable->GetStrWithType();

    return instruction_str;
}



//Control flow
string Branch::GetStr()
{
    string instruction_str;

    instruction_str+="br ";
    instruction_str+=condition->GetStrWithType();
    instruction_str+=" <"+to_string(true_number)+">";
    instruction_str+=" <"+to_string(false_number)+">";

    return instruction_str;
}

string Jump::GetStr()
{
    string instruction_str;

    instruction_str+="jump ";
    instruction_str+=" <"+to_string(number)+">";

    return instruction_str;
}

string Calll::GetStr()
{
    string instruction_str;

    instruction_str+="call @"+function_name;
    instruction_str+="(";
    for(int i=0;i<arguments.size();i++)
    {
        instruction_str+=arguments[i]->GetStrWithType();
        if(i<arguments.size()-1)instruction_str+=",";
    }
    instruction_str+=")";
    if(get_retvalue)
    {
        instruction_str+=" => ";
        instruction_str+=get_retvalue->GetStrWithType();
    }

    return instruction_str;
}

string Return::GetStr()
{
    string instruction_str;

    instruction_str+="ret ";
    if(return_value)
        instruction_str+=return_value->GetStrWithType();

    return instruction_str;  
}



//Arithmetic
string Or::GetStr()
{
    string instruction_str;

    instruction_str+="or ";
    instruction_str+=a->GetStrWithType();
    instruction_str+=" ";
    instruction_str+=b->GetStrWithType();
    instruction_str+=" => ";
    instruction_str+=c->GetStrWithType();

    return instruction_str;
}

string And::GetStr()
{
    string instruction_str;

    instruction_str+="and ";
    instruction_str+=a->GetStrWithType();
    instruction_str+=" ";
    instruction_str+=b->GetStrWithType();
    instruction_str+=" => ";
    instruction_str+=c->GetStrWithType();

    return instruction_str;
}

string Not::GetStr()
{
    string instruction_str;

    instruction_str+="not ";
    instruction_str+=a->GetStrWithType();
    instruction_str+=" => ";
    instruction_str+=b->GetStrWithType();

    return instruction_str;
}

string Negative::GetStr()
{
    string instruction_str;

    instruction_str+="neg ";
    instruction_str+=a->GetStrWithType();
    instruction_str+=" => ";
    instruction_str+=b->GetStrWithType();

    return instruction_str;
}

string Equal::GetStr()
{
    string instruction_str;

    instruction_str+="equ ";
    instruction_str+=a->GetStrWithType();
    instruction_str+=" ";
    instruction_str+=b->GetStrWithType();
    instruction_str+=" => ";
    instruction_str+=c->GetStrWithType();

    return instruction_str;
}

string NotEqual::GetStr()
{
    string instruction_str;

    instruction_str+="nequ ";
    instruction_str+=a->GetStrWithType();
    instruction_str+=" ";
    instruction_str+=b->GetStrWithType();
    instruction_str+=" => ";
    instruction_str+=c->GetStrWithType();

    return instruction_str;
}

string Less::GetStr()
{
    string instruction_str;

    instruction_str+="les ";
    instruction_str+=a->GetStrWithType();
    instruction_str+=" ";
    instruction_str+=b->GetStrWithType();
    instruction_str+=" => ";
    instruction_str+=c->GetStrWithType(); 

    return instruction_str;
}

string LessEqual::GetStr()
{
    string instruction_str;

    instruction_str+="lequ ";
    instruction_str+=a->GetStrWithType();
    instruction_str+=" ";
    instruction_str+=b->GetStrWithType();
    instruction_str+=" => ";
    instruction_str+=c->GetStrWithType(); 

    return instruction_str;
}

string Greater::GetStr()
{
    string instruction_str;

    instruction_str+="gre ";
    instruction_str+=a->GetStrWithType();
    instruction_str+=" ";
    instruction_str+=b->GetStrWithType();
    instruction_str+=" => ";
    instruction_str+=c->GetStrWithType();  

    return instruction_str;
}

string GreaterEqual::GetStr()
{
    string instruction_str;

    instruction_str+="gequ ";
    instruction_str+=a->GetStrWithType();
    instruction_str+=" ";
    instruction_str+=b->GetStrWithType();
    instruction_str+=" => ";
    instruction_str+=c->GetStrWithType();   

    return instruction_str;
}

string Add::GetStr()
{
    string instruction_str;

    instruction_str+="add ";
    instruction_str+=a->GetStrWithType();
    instruction_str+=" ";
    instruction_str+=b->GetStrWithType();
    instruction_str+=" => ";
    instruction_str+=c->GetStrWithType();

    return instruction_str;
}

string Sub::GetStr()
{
    string instruction_str;

    instruction_str+="sub ";
    instruction_str+=a->GetStrWithType();
    instruction_str+=" ";
    instruction_str+=b->GetStrWithType();
    instruction_str+=" => ";
    instruction_str+=c->GetStrWithType();   

    return instruction_str;
}

string Mul::GetStr()
{
    string instruction_str;

    instruction_str+="mul ";
    instruction_str+=a->GetStrWithType();
    instruction_str+=" ";
    instruction_str+=b->GetStrWithType();
    instruction_str+=" => ";
    instruction_str+=c->GetStrWithType();    

    return instruction_str;
}

string Div::GetStr()
{
    string instruction_str;

    instruction_str+="div ";
    instruction_str+=a->GetStrWithType();
    instruction_str+=" ";
    instruction_str+=b->GetStrWithType();
    instruction_str+=" => ";
    instruction_str+=c->GetStrWithType();  

    return instruction_str;
}

string Mod::GetStr()
{
    string instruction_str;

    instruction_str+="mod ";
    instruction_str+=a->GetStrWithType();
    instruction_str+=" ";
    instruction_str+=b->GetStrWithType();
    instruction_str+=" => ";
    instruction_str+=c->GetStrWithType();

    return instruction_str;
}

string ItoF::GetStr()
{
    string instruction_str;

    instruction_str+="i2f ";
    instruction_str+=a_i->GetStrWithType();
    instruction_str+=" => ";
    instruction_str+=b_f->GetStrWithType();  

    return instruction_str;
}

string FtoI::GetStr()
{
    string instruction_str;
    
    instruction_str+="f2i ";
    instruction_str+=a_f->GetStrWithType();
    instruction_str+=" => ";
    instruction_str+=b_i->GetStrWithType();
    
    return instruction_str;
}
