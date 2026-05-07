#include "../backend/LIRInstruction.h"

//Integer

//Arithmetic
string AddI::GetStr()
{
    string instruction_str;

    instruction_str+="addI ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i1.GetStr();
    instruction_str+=",";
    instruction_str+=src_i2.GetStr();
    
    return instruction_str;
}

string AddImm::GetStr()
{
    string instruction_str;

    instruction_str+="addImm ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i.GetStr();
    instruction_str+=",";
    instruction_str+=to_string(src_imm);
    
    return instruction_str;
}

string AddL::GetStr()
{
    string instruction_str;

    instruction_str+="addL ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i1.GetStr();
    instruction_str+=",";
    instruction_str+=src_i2.GetStr();
    
    return instruction_str;
}

string SubI::GetStr()
{
    string instruction_str;

    instruction_str+="subI ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i1.GetStr();
    instruction_str+=",";
    instruction_str+=src_i2.GetStr();
    
    return instruction_str;
}

string MulI::GetStr()
{
    string instruction_str;

    instruction_str+="mulI ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i1.GetStr();
    instruction_str+=",";
    instruction_str+=src_i2.GetStr();
    
    return instruction_str;
}

string DivI::GetStr()
{
    string instruction_str;

    instruction_str+="divI ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i1.GetStr();
    instruction_str+=",";
    instruction_str+=src_i2.GetStr();
    
    return instruction_str;
}

string RemI::GetStr()
{
    string instruction_str;

    instruction_str+="remI ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i1.GetStr();
    instruction_str+=",";
    instruction_str+=src_i2.GetStr();
    
    return instruction_str;
}

string NegI::GetStr()
{
    string instruction_str;

    instruction_str+="negI ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i.GetStr();
    
    return instruction_str;  
}



//Logic
string NotI::GetStr()
{
    string instruction_str;

    instruction_str+="notI ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i.GetStr();
    
    return instruction_str;  
}

string AndI::GetStr()
{
    string instruction_str;

    instruction_str+="andI ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i1.GetStr();
    instruction_str+=",";
    instruction_str+=src_i2.GetStr();
    
    return instruction_str;
}

string AndImm::GetStr()
{
    string instruction_str;

    instruction_str+="andImm ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i.GetStr();
    instruction_str+=",";
    instruction_str+=to_string(src_imm);
    
    return instruction_str;
}

string OrI::GetStr()
{
    string instruction_str;

    instruction_str+="orI ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i1.GetStr();
    instruction_str+=",";
    instruction_str+=src_i2.GetStr();
    
    return instruction_str;
}

string OrImm::GetStr()
{
    string instruction_str;

    instruction_str+="orImm ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i.GetStr();
    instruction_str+=",";
    instruction_str+=to_string(src_imm);
    
    return instruction_str;
}

string XorI::GetStr()
{
    string instruction_str;

    instruction_str+="xorI ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i1.GetStr();
    instruction_str+=",";
    instruction_str+=src_i2.GetStr();
    
    return instruction_str;
}

string XorImm::GetStr()
{
    string instruction_str;

    instruction_str+="xorImm ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i.GetStr();
    instruction_str+=",";
    instruction_str+=to_string(src_imm);
    
    return instruction_str;
}



//Compare
string LessI::GetStr()
{
    string instruction_str;

    instruction_str+="lessI ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i1.GetStr();
    instruction_str+=",";
    instruction_str+=src_i2.GetStr();
    
    return instruction_str;
}

string LessImm::GetStr()
{
    string instruction_str;

    instruction_str+="lessImm ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i.GetStr();
    instruction_str+=",";
    instruction_str+=to_string(src_imm);
    
    return instruction_str;
}

string EquzI::GetStr()
{
    string instruction_str;

    instruction_str+="equzI ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i.GetStr();
    
    return instruction_str;
}

string NequzI::GetStr()
{
    string instruction_str;

    instruction_str+="nequzI ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i.GetStr();
    
    return instruction_str;
}

string LesszI::GetStr()
{
    string instruction_str;

    instruction_str+="lesszI ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i.GetStr();
    
    return instruction_str;
}

string GrezI::GetStr()
{
    string instruction_str;

    instruction_str+="grezI ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i.GetStr();
    
    return instruction_str;
}



//Memory
string LoadA::GetStr()
{
    string instruction_str;

    instruction_str+="loadA ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_globalvar;
    
    return instruction_str;
}

string LoadImm::GetStr()
{
    string instruction_str;

    instruction_str+="loadImm ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_imm;
    
    return instruction_str;
}

string LoadI::GetStr()
{
    string instruction_str;

    instruction_str+="loadI ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i.GetStr();
    
    return instruction_str;
}

string LoadL::GetStr()
{
    string instruction_str;

    instruction_str+="loadL ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i.GetStr();
    
    return instruction_str;
}

string StoreI::GetStr()
{
    string instruction_str;

    instruction_str+="storeI ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i.GetStr();
    
    return instruction_str;
}

string StoreL::GetStr()
{
    string instruction_str;

    instruction_str+="storeL ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i.GetStr();
    
    return instruction_str;
}





//Float

//Arithmetic
string AddF::GetStr()
{
    string instruction_str;

    instruction_str+="addF ";
    instruction_str+=des_f.GetStr();
    instruction_str+=",";
    instruction_str+=src_f1.GetStr();
    instruction_str+=",";
    instruction_str+=src_f2.GetStr();
    
    return instruction_str;
}

string SubF::GetStr()
{
    string instruction_str;

    instruction_str+="subF ";
    instruction_str+=des_f.GetStr();
    instruction_str+=",";
    instruction_str+=src_f1.GetStr();
    instruction_str+=",";
    instruction_str+=src_f2.GetStr();
    
    return instruction_str;
}

string MulF::GetStr()
{
    string instruction_str;

    instruction_str+="mulF ";
    instruction_str+=des_f.GetStr();
    instruction_str+=",";
    instruction_str+=src_f1.GetStr();
    instruction_str+=",";
    instruction_str+=src_f2.GetStr();
    
    return instruction_str;
}

string DivF::GetStr()
{
    string instruction_str;

    instruction_str+="divF ";
    instruction_str+=des_f.GetStr();
    instruction_str+=",";
    instruction_str+=src_f1.GetStr();
    instruction_str+=",";
    instruction_str+=src_f2.GetStr();
    
    return instruction_str;
}

string NegF::GetStr()
{
    string instruction_str;

    instruction_str+="negF ";
    instruction_str+=des_f.GetStr();
    instruction_str+=",";
    instruction_str+=src_f.GetStr();
    
    return instruction_str;
}



//Compare
string EquF::GetStr()
{
    string instruction_str;

    instruction_str+="equF ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_f1.GetStr();
    instruction_str+=",";
    instruction_str+=src_f2.GetStr();
    
    return instruction_str;
}

string LequF::GetStr()
{
    string instruction_str;

    instruction_str+="lequF ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_f1.GetStr();
    instruction_str+=",";
    instruction_str+=src_f2.GetStr();
    
    return instruction_str;
}

string LessF::GetStr()
{
    string instruction_str;

    instruction_str+="lessF ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_f1.GetStr();
    instruction_str+=",";
    instruction_str+=src_f2.GetStr();
    
    return instruction_str;
}



//Memory
string LoadF::GetStr()
{
    string instruction_str;

    instruction_str+="loadF ";
    instruction_str+=des_f.GetStr();
    instruction_str+=",";
    instruction_str+=src_i.GetStr();
    
    return instruction_str;
}

string StoreF::GetStr()
{
    string instruction_str;

    instruction_str+="storeF ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_f.GetStr();
    
    return instruction_str;
}





//Share

//Move
string MoveI::GetStr()
{
    string instruction_str;

    instruction_str+="moveI ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_i.GetStr();
    
    return instruction_str;
}

string MoveF::GetStr()
{
    string instruction_str;

    instruction_str+="moveF ";
    instruction_str+=des_f.GetStr();
    instruction_str+=",";
    instruction_str+=src_f.GetStr();
    
    return instruction_str;
}

string MoveI2F::GetStr()
{
    string instruction_str;

    instruction_str+="moveI2F ";
    instruction_str+=des_f.GetStr();
    instruction_str+=",";
    instruction_str+=src_i.GetStr();
    
    return instruction_str;
}

string MoveF2I::GetStr()
{
    string instruction_str;

    instruction_str+="moveF2I ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_f.GetStr();
    
    return instruction_str;
}



//Control flow
string Call::GetStr()
{
    string instruction_str;

    instruction_str+="call ";
    instruction_str+=function;
    
    return instruction_str;
}

string Ret::GetStr()
{
    string instruction_str;

    instruction_str+="ret ";
    
    return instruction_str;
}

string Bnez::GetStr()
{
    string instruction_str;

    instruction_str+="bnez ";
    instruction_str+=to_string(number);
    instruction_str+=",";
    instruction_str+=src_i.GetStr();
    
    return instruction_str;
}

string Jmp::GetStr()
{
    string instruction_str;

    instruction_str+="jmp ";
    instruction_str+=to_string(number);
    
    return instruction_str;
}



//Convert
string CvtI2F::GetStr()
{
    string instruction_str;

    instruction_str+="cvtI2F ";
    instruction_str+=des_f.GetStr();
    instruction_str+=",";
    instruction_str+=src_i.GetStr();
    
    return instruction_str;
}

string CvtF2I::GetStr()
{
    string instruction_str;

    instruction_str+="cvtF2I ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",";
    instruction_str+=src_f.GetStr();
    
    return instruction_str;
}





//Custom
string GetI::GetStr()
{
    string instruction_str;
    instruction_str+="getI ";
    instruction_str+=des_i.GetStr();
    instruction_str+=",%";
    instruction_str+=to_string(src_localvar_number);

    return instruction_str;
}

string GetF::GetStr()
{
    string instruction_str;
    instruction_str+="getF ";
    instruction_str+=des_f.GetStr();
    instruction_str+=",%";
    instruction_str+=to_string(src_localvar_number);

    return instruction_str;
}

string PutI::GetStr()
{
    string instruction_str;
    instruction_str+="putI %";
    instruction_str+=to_string(des_localvar_number);
    instruction_str+=",";
    instruction_str+=src_i.GetStr();

    return instruction_str;
}

string PutF::GetStr()
{
    string instruction_str;
    instruction_str+="putF %";
    instruction_str+=to_string(des_localvar_number);
    instruction_str+=",";
    instruction_str+=src_f.GetStr();

    return instruction_str;
}
