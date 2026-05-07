#include "../backend/VirtualRegister.h"
#include "../utility/Diagnostor.h"

string VirtualRegister::GetStr()
{
    string vreg_str;

    //Mark
    switch(vreg_type)
    {
        case R_GENERAL_I:vreg_str+="r";break;
        case F_GENERAL_F:vreg_str+="f";break;
        case S_ARG_I:vreg_str+="s";break;
        case G_ARG_F:vreg_str+="g";break;
        case B_RET_I:vreg_str+="b";break;
        case W_RET_F:vreg_str+="w";break;
    }

    //Number
    vreg_str+=to_string(number);
    
    return vreg_str;
}

string VirtualRegister::GetRV64()
{
    string reg_rv64_str;

    //Mark
    switch(vreg_type)
    {
        case R_GENERAL_I:reg_rv64_str+="t";break;
        case F_GENERAL_F:reg_rv64_str+="ft";break;
        case S_ARG_I:reg_rv64_str+="a";break;
        case G_ARG_F:reg_rv64_str+="fa";break;
        case B_RET_I:reg_rv64_str+="a";break;
        case W_RET_F:reg_rv64_str+="fa";break;
    }

    //Number
    reg_rv64_str+=to_string(number);

    return reg_rv64_str;
}




VirtualRegister VRegManager::NewVRegR(int data_size)
{
    //Linear scan a free virtual register 
    for(int i=0;i<r_vregs.size();i++)
    {
        if(r_vregs[i])
        {
            r_vregs[i]=false;
            return VirtualRegister(R_GENERAL_I,i,data_size);
        }
    }
    
    //Scan failed, push new one
    r_vregs.push_back(false);
    return VirtualRegister(R_GENERAL_I,r_vregs.size()-1,data_size);
} 

VirtualRegister VRegManager::NewVRegF(int data_size)
{
    //Linear scan a free virtual register 
    for(int i=0;i<f_vregs.size();i++)
    {
        if(f_vregs[i])
        {
            f_vregs[i]=false;
            return VirtualRegister(F_GENERAL_F,i,data_size);
        }
    }
    
    //Scan failed, push new one
    f_vregs.push_back(false);
    return VirtualRegister(F_GENERAL_F,f_vregs.size()-1,data_size);
}    

void VRegManager::GeneralFree(VirtualRegister& vreg_used)
{
    if(vreg_used.vreg_type==R_GENERAL_I)
    {
        if(vreg_used.number<r_vregs.size())
        {
            if(!r_vregs[vreg_used.number])
                r_vregs[vreg_used.number]=true;
            else CompilerError("Free a freed R virtual register");
        }
        else CompilerError("R virtual register index invalid");
    }
    else if(vreg_used.vreg_type==F_GENERAL_F)
    {
        if(vreg_used.number<f_vregs.size())
        {
            if(!f_vregs[vreg_used.number])
                f_vregs[vreg_used.number]=true;
            else CompilerError("Free a freed F virtual register");
        }
        else CompilerError("F virtual register index invalid");       
    }
}



VirtualRegister VRegManager::NewVRegS(int data_size)
{
    return VirtualRegister(S_ARG_I,s_vreg_number++,data_size);
}

VirtualRegister VRegManager::NewVRegG(int data_size)
{
    return VirtualRegister(G_ARG_F,g_vreg_number++,data_size);
}

void VRegManager::ArgFree()
{
    s_vreg_number=0;
    g_vreg_number=0;
}



VirtualRegister VRegManager::GetVRegB(int data_size)
{
    return VirtualRegister(B_RET_I,0,data_size);
}

VirtualRegister VRegManager::GetVRegW(int data_size)
{
    return VirtualRegister(W_RET_F,0,data_size);
}



void VRegManager::Clear()
{
    r_vregs.clear();
    f_vregs.clear();
    
    ArgFree();
}
