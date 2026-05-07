#include "../backend/RV64_I.h"

//Arithmetic
string addw(string reg_des_i,string reg_src1_i,string reg_src2_i)
{
    return "addw "+reg_des_i+","+reg_src1_i+","+reg_src2_i;
}

string addwi(string reg_des_i,string reg_src_i,int immediate)
{
    if(abs(immediate)<1000)
        return "addwi "+reg_des_i+","+reg_src_i+","+to_string(immediate);

    string addi_str;
    addi_str+=li("t6",to_string(immediate))+"\n\t";
    addi_str+=addw(reg_des_i,reg_src_i,"t6")+"\n\t";
    return addi_str;
}

string add(string reg_des_i,string reg_src1_i,string reg_src2_i)
{
    return "add "+reg_des_i+","+reg_src1_i+","+reg_src2_i;
}

string addi(string reg_des_i,string reg_src_i,int immediate)
{
    if(abs(immediate)<1000)
        return "addi "+reg_des_i+","+reg_src_i+","+to_string(immediate);
    
    string addi_str;
    addi_str+=li("t6",to_string(immediate))+"\n\t";
    addi_str+=add(reg_des_i,reg_src_i,"t6")+"\n\t";
    return addi_str;
}


string subw(string reg_des_i,string reg_src1_i,string reg_src2_i)
{
    return "subw "+reg_des_i+","+reg_src1_i+","+reg_src2_i;
}

string mul(string reg_des_i,string reg_src1_i,string reg_src2_i)
{
    return "mul "+reg_des_i+","+reg_src1_i+","+reg_src2_i;
}

string divw(string reg_des_i,string reg_src1_i,string reg_src2_i)
{
    return "divw "+reg_des_i+","+reg_src1_i+","+reg_src2_i;
}

string remw(string reg_des_i,string reg_src1_i,string reg_src2_i)
{
    return "remw "+reg_des_i+","+reg_src1_i+","+reg_src2_i;
}

string negw(string reg_des_i,string reg_src_i)//[Pseudo]
{
    return "negw "+reg_des_i+","+reg_src_i;
}



//Logic
string not_(string reg_des_i,string reg_src_i)
{
    return "not "+reg_des_i+","+reg_src_i;
}

string and_(string reg_des_i,string reg_src1_i,string reg_src2_i)
{
    return "and "+reg_des_i+","+reg_src1_i+","+reg_src2_i;
}

string andi(string reg_des_i,string reg_src_i,int immediate)
{
    return "andi "+reg_des_i+","+reg_src_i+","+to_string(immediate);
}

string or_(string reg_des_i,string reg_src1_i,string reg_src2_i)
{
    return "or "+reg_des_i+","+reg_src1_i+","+reg_src2_i;
}

string ori(string reg_des_i,string reg_src_i,int immediate)
{
    return "ori "+reg_des_i+","+reg_src_i+","+to_string(immediate);
}

string xor_(string reg_des_i,string reg_src1_i,string reg_src2_i)
{
    return "xor "+reg_des_i+","+reg_src1_i+","+reg_src2_i;
}

string xori(string reg_des_i,string reg_src_i,int immediate)
{
    return "xori "+reg_des_i+","+reg_src_i+","+to_string(immediate);
}



//Shift
string sllw(string reg_des_i,string reg_src1_i,string reg_src2_i)//<<(logic)
{
    return "sllw "+reg_des_i+","+reg_src1_i+","+reg_src2_i;
}

string slliw(string reg_des_i,string reg_src_i,string immediate)
{
    return "slliw "+reg_des_i+","+reg_src_i+","+immediate;
}

string srlw(string reg_des_i,string reg_src1_i,string reg_src2_i)//>>(logic)
{
    return "srlw "+reg_des_i+","+reg_src1_i+","+reg_src2_i;
}

string srliw(string reg_des_i,string reg_src_i,string immediate)
{
    return "srliw "+reg_des_i+","+reg_src_i+","+immediate;
}

string sraw(string reg_des_i,string reg_src1_i,string reg_src2_i)//>>(arithmetic)
{
    return "sraw "+reg_des_i+","+reg_src1_i+","+reg_src2_i;
}

string sraiw(string reg_des_i,string reg_src_i,string immediate)
{
    return "sraiw "+reg_des_i+","+reg_src_i+","+immediate;
}



//Compare arithmetic
string slt(string reg_des_i,string reg_src1_i,string reg_src2_i)//<
{
    return "slt "+reg_des_i+","+reg_src1_i+","+reg_src2_i;
}

string slti(string reg_des_i,string reg_src_i,int immediate)
{
    return "slti "+reg_des_i+","+reg_src_i+","+to_string(immediate);
}

string seqz(string reg_des_i,string reg_src_i)//==0 [Pseudo]
{
    return "seqz "+reg_des_i+","+reg_src_i;
}

string snez(string reg_des_i,string reg_src_i)//!=0 [Pseudo]
{
    return "snez "+reg_des_i+","+reg_src_i;
}

string sltz(string reg_des_i,string reg_src_i)//<0  [Pseudo]
{
    return "sltz "+reg_des_i+","+reg_src_i;
}

string sgtz(string reg_des_i,string reg_src_i)//>0  [Pseudo]
{
    return "sgtz "+reg_des_i+","+reg_src_i;
}



//Compare branch
string beq(string reg_src1_i,string reg_src2_i,string lable)//== 
{
    return "beq "+reg_src1_i+","+reg_src2_i+","+lable;
}

string bne(string reg_src1_i,string reg_src2_i,string lable)//!=
{
    return "bne "+reg_src1_i+","+reg_src2_i+","+lable;
}

string bgt(string reg_src1_i,string reg_src2_i,string lable)//>
{
    return "bgt "+reg_src1_i+","+reg_src2_i+","+lable;
}

string bge(string reg_src1_i,string reg_src2_i,string lable)//>=
{
    return "bge "+reg_src1_i+","+reg_src2_i+","+lable;
}

string blt(string reg_src1_i,string reg_src2_i,string lable)//<
{
    return "blt "+reg_src1_i+","+reg_src2_i+","+lable;
}

string ble(string reg_src1_i,string reg_src2_i,string lable)//<=
{
    return "ble "+reg_src1_i+","+reg_src2_i+","+lable;
}

string beqz(string reg_src_i,string lable)//==0 [Pseudo]
{
    return "beqz "+reg_src_i+","+lable;
}

string bnez(string reg_src_i,string lable)//!=0 [Pseudo]
{
    return "bnez "+reg_src_i+","+lable;
}

string bgtz(string reg_src_i,string lable)//>0  [Pseudo]
{
    return "bgtz "+reg_src_i+","+lable;
}

string bgez(string reg_src_i,string lable)//>=0 [Pseudo]
{
    return "bgez "+reg_src_i+","+lable;
}

string bltz(string reg_src_i,string lable)//<0  [Pseudo]
{
    return "bltz "+reg_src_i+","+lable;
}

string blez(string reg_src_i,string lable)//<=0 [Pseudo]
{
    return "blez "+reg_src_i+","+lable;
}



//Function
string call(string function)//[Pseudo]
{
    return "call "+function;
}

string ret()//[Pseudo]
{
    return "ret";
}

string tail()//[Pseudo]
{
    return "tail";
}



//Jump
string j(string lable)//[Pseudo]
{
    return "j "+lable;
}



//Memory
string la(string reg_des_i,string variable)//[Pseudo]
{
    return "la "+reg_des_i+","+variable;
}

string li(string reg_des_i,string immediate)//[Pseudo]
{
    return "li "+reg_des_i+","+immediate;
}

string lw(string reg_des_i,int offset,string reg_src_i)
{
    if(abs(offset)<1000)
        return "lw "+reg_des_i+","+to_string(offset)+"("+reg_src_i+")";
    
    string lw_str;
    lw_str+=li("t6",to_string(offset))+"\n\t";
    lw_str+=add("t6","t6",reg_src_i)+"\n\t";
    lw_str+="lw "+reg_des_i+",0(t6)";
    return lw_str;
}

string ld(string reg_des_i,int offset,string reg_src_i)
{    
    if(abs(offset)<1000)
        return "ld "+reg_des_i+","+to_string(offset)+"("+reg_src_i+")";
    
    string ld_str;
    ld_str+=li("t6",to_string(offset))+"\n\t";
    ld_str+=add("t6","t6",reg_src_i)+"\n\t";
    ld_str+="ld "+reg_des_i+",0(t6)";
    return ld_str;
}

string sw(string reg_src1_i,int offset,string reg_src2_i)
{
    if(abs(offset)<1000)
        return "sw "+reg_src1_i+","+to_string(offset)+"("+reg_src2_i+")";

    string sw_str;
    sw_str+=li("t6",to_string(offset))+"\n\t";
    sw_str+=add("t6","t6",reg_src2_i)+"\n\t";
    sw_str+="sw "+reg_src1_i+",0(t6)";
    return sw_str;
}

string sd(string reg_src1_i,int offset,string reg_src2_i)
{
    if(abs(offset)<1000)
        return "sd "+reg_src1_i+","+to_string(offset)+"("+reg_src2_i+")";
    
    string sd_str;
    sd_str+=li("t6",to_string(offset))+"\n\t";
    sd_str+=add("t6","t6",reg_src2_i)+"\n\t";
    sd_str+="sd "+reg_src1_i+",0(t6)";
    return sd_str;
}



//Move
string mv(string reg_des_i,string reg_src_i)//[Pseudo]
{
    return "mv "+reg_des_i+","+reg_src_i;
}
