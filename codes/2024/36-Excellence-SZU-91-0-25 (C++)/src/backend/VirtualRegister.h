#pragma once

#include "../utility/System.h"

enum VRegType
{
    R_GENERAL_I,    //Integer general register
    F_GENERAL_F,    //Float general register
    
    S_ARG_I,        //Integer argument register
    G_ARG_F,        //Float argument register
    
    B_RET_I,        //Integer return register
    W_RET_F         //Float argument register
};

class VirtualRegister
{
public:
    VRegType vreg_type;
    int number;
    int data_size;

    VirtualRegister(){}
    VirtualRegister(VRegType vreg_type,int number,int data_size):
        vreg_type(vreg_type),number(number),data_size(data_size){}

    string GetStr();
    string GetRV64();
};



class VRegManager
{
private:
    vector<bool> r_vregs; //Ture is free
    vector<bool> f_vregs;
    int s_vreg_number;
    int g_vreg_number;

public:
    VirtualRegister NewVRegR(int data_size);    
    VirtualRegister NewVRegF(int data_size);    
    void GeneralFree(VirtualRegister& vreg_used);

    VirtualRegister NewVRegS(int data_size);    
    VirtualRegister NewVRegG(int data_size);
    void ArgFree();
    
    VirtualRegister GetVRegB(int data_size);
    VirtualRegister GetVRegW(int data_size);

    void Clear();
};
