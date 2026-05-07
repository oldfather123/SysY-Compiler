#pragma once

#include "../utility/System.h"
#include "../backend/VirtualRegister.h"

enum LIRInstructionType
{
    //Integer
    ADD_I, ADD_IMM, ADD_L, SUB_I, MUL_I, DIV_I, REM_I, NEG_I,      //Arithmetic
    NOT_I, AND_I, AND_IMM, OR_I, OR_IMM, XOR_I, XOR_IMM,    //Logic
    LESS_I, LESS_IMM, EQUZ_I, NEQUZ_I, LESSZ_I, GREZ_I,     //Compare
    LOAD_A, LOAD_IMM, LOAD_I, LOAD_L, STORE_I, STORE_L,     //Memory
    
    //Float
    ADD_F, SUB_F, MUL_F, DIV_F, NEG_F,  //Arithmetic
    EQU_F, LEQU_F, LESS_F,              //Compare
    LOAD_F, STORE_F,                    //Memory
    
    //Share
    MOVE_I, MOVE_F, MOVE_I2F, MOVE_F2I, //Move
    CALL, RET, BNEZ, JMP,                     //Control flow
    CVT_I2F, CVT_F2I,                   //Convert

    //Custom
    GETI,GETF,PUTI,PUTF
};

class LIRInstruction
{
public:
    LIRInstructionType lir_instruction_type;

    LIRInstruction(LIRInstructionType lir_instruction_type):
        lir_instruction_type(lir_instruction_type){}
    virtual ~LIRInstruction()= default;

    virtual string GetStr()=0;
};



//Integer

//Arithmetic
class AddI:public LIRInstruction
{
public:
    VirtualRegister src_i1;
    VirtualRegister src_i2;
    VirtualRegister des_i;
    
    AddI():LIRInstruction(ADD_I){}

    string GetStr() override;
};

class AddImm:public LIRInstruction
{
public:
    VirtualRegister src_i;
    int src_imm;
    VirtualRegister des_i;
    
    AddImm():LIRInstruction(ADD_IMM){}

    string GetStr() override;
};

class AddL:public LIRInstruction
{
public:
    VirtualRegister src_i1;
    VirtualRegister src_i2;
    VirtualRegister des_i;

    AddL():LIRInstruction(ADD_L){}

    string GetStr() override;
};

class SubI:public LIRInstruction
{
public:
    VirtualRegister src_i1;
    VirtualRegister src_i2;
    VirtualRegister des_i;

    SubI():LIRInstruction(SUB_I){}

    string GetStr() override;
};

class MulI:public LIRInstruction
{
public:
    VirtualRegister src_i1;
    VirtualRegister src_i2;
    VirtualRegister des_i;  

    MulI():LIRInstruction(MUL_I){}

    string GetStr() override;
};

class DivI:public LIRInstruction
{
public:
    VirtualRegister src_i1;
    VirtualRegister src_i2;
    VirtualRegister des_i; 

    DivI():LIRInstruction(DIV_I){}

    string GetStr() override;
};

class RemI:public LIRInstruction
{
public:
    VirtualRegister src_i1;
    VirtualRegister src_i2;
    VirtualRegister des_i; 

    RemI():LIRInstruction(REM_I){}

    string GetStr() override;
};

class NegI:public LIRInstruction
{
public:
    VirtualRegister src_i;
    VirtualRegister des_i;

    NegI():LIRInstruction(NEG_I){}

    string GetStr() override;
};



//Logic
class NotI:public LIRInstruction
{
public:
    VirtualRegister src_i;
    VirtualRegister des_i;   

    NotI():LIRInstruction(NOT_I){}

    string GetStr() override;
};

class AndI:public LIRInstruction
{
public:
    VirtualRegister src_i1;
    VirtualRegister src_i2;
    VirtualRegister des_i; 

    AndI():LIRInstruction(AND_I){}

    string GetStr() override;
};

class AndImm:public LIRInstruction
{
public:
    VirtualRegister src_i;
    int src_imm;
    VirtualRegister des_i;

    AndImm():LIRInstruction(AND_IMM){}

    string GetStr() override;
};

class OrI:public LIRInstruction
{
public:
    VirtualRegister src_i1;
    VirtualRegister src_i2;
    VirtualRegister des_i; 

    OrI():LIRInstruction(OR_I){}

    string GetStr() override;
};

class OrImm:public LIRInstruction
{
public:
    VirtualRegister src_i;
    int src_imm;
    VirtualRegister des_i;

    OrImm():LIRInstruction(OR_IMM){}

    string GetStr() override;
};

class XorI:public LIRInstruction
{
public:
    VirtualRegister src_i1;
    VirtualRegister src_i2;
    VirtualRegister des_i; 

    XorI():LIRInstruction(XOR_I){}

    string GetStr() override;
};

class XorImm:public LIRInstruction
{
public:
    VirtualRegister src_i;
    int src_imm;
    VirtualRegister des_i;

    XorImm():LIRInstruction(XOR_IMM){}

    string GetStr() override;
};



//Compare
class LessI:public LIRInstruction
{
public:
    VirtualRegister src_i1;
    VirtualRegister src_i2;
    VirtualRegister des_i; 

    LessI():LIRInstruction(LESS_I){}

    string GetStr() override;
};

class LessImm:public LIRInstruction
{
public:
    VirtualRegister src_i;
    int src_imm;
    VirtualRegister des_i;

    LessImm():LIRInstruction(LESS_IMM){}

    string GetStr() override;
};

class EquzI:public LIRInstruction
{
public:
    VirtualRegister src_i;
    VirtualRegister des_i;

    EquzI():LIRInstruction(EQUZ_I){}

    string GetStr() override;
};

class NequzI:public LIRInstruction
{
public:
    VirtualRegister src_i;
    VirtualRegister des_i;

    NequzI():LIRInstruction(NEQUZ_I){}

    string GetStr() override;
};

class LesszI:public LIRInstruction
{
public:
    VirtualRegister src_i;
    VirtualRegister des_i;

    LesszI():LIRInstruction(LESSZ_I){}

    string GetStr() override;
};

class GrezI:public LIRInstruction
{
public:
    VirtualRegister src_i;
    VirtualRegister des_i;

    GrezI():LIRInstruction(GREZ_I){}

    string GetStr() override;
};



//Memory
class LoadA:public LIRInstruction
{
public:
    string src_globalvar;
    VirtualRegister des_i;

    LoadA():LIRInstruction(LOAD_A){}

    string GetStr() override;
};

class LoadImm:public LIRInstruction
{
public:
    string src_imm;
    VirtualRegister des_i;

    LoadImm():LIRInstruction(LOAD_IMM){}

    string GetStr() override;
};

class LoadI:public LIRInstruction
{
public:
    VirtualRegister src_i; //address
    VirtualRegister des_i;

    LoadI():LIRInstruction(LOAD_I){}

    string GetStr() override;
};

class LoadL:public LIRInstruction
{
public:
    VirtualRegister src_i; //address
    VirtualRegister des_i;

    LoadL():LIRInstruction(LOAD_L){}

    string GetStr() override;
};

class StoreI:public LIRInstruction
{
public:
    VirtualRegister src_i;
    VirtualRegister des_i; //address

    StoreI():LIRInstruction(STORE_I){}

    string GetStr() override;
};

class StoreL:public LIRInstruction
{
public:
    VirtualRegister src_i;
    VirtualRegister des_i; //address

    StoreL():LIRInstruction(STORE_L){}

    string GetStr() override;
};





//Float

//Arithmetic
class AddF:public LIRInstruction
{
public:
    VirtualRegister src_f1;
    VirtualRegister src_f2;
    VirtualRegister des_f;

    AddF():LIRInstruction(ADD_F){}

    string GetStr() override;
};

class SubF:public LIRInstruction
{
public:
    VirtualRegister src_f1;
    VirtualRegister src_f2;
    VirtualRegister des_f;

    SubF():LIRInstruction(SUB_F){}

    string GetStr() override;
};

class MulF:public LIRInstruction
{
public:
    VirtualRegister src_f1;
    VirtualRegister src_f2;
    VirtualRegister des_f;

    MulF():LIRInstruction(MUL_F){}

    string GetStr() override;
};

class DivF:public LIRInstruction
{
public:
    VirtualRegister src_f1;
    VirtualRegister src_f2;
    VirtualRegister des_f;

    DivF():LIRInstruction(DIV_F){}

    string GetStr() override;
};

class NegF:public LIRInstruction
{
public:
    VirtualRegister src_f;
    VirtualRegister des_f;

    NegF():LIRInstruction(NEG_F){}

    string GetStr() override;
};



//Compare
class EquF:public LIRInstruction
{
public:
    VirtualRegister src_f1;
    VirtualRegister src_f2;
    VirtualRegister des_i;

    EquF():LIRInstruction(EQU_F){}

    string GetStr() override;
};

class LequF:public LIRInstruction
{
public:
    VirtualRegister src_f1;
    VirtualRegister src_f2;
    VirtualRegister des_i;

    LequF():LIRInstruction(LEQU_F){}
    
    string GetStr() override;
};

class LessF:public LIRInstruction
{
public:
    VirtualRegister src_f1;
    VirtualRegister src_f2;
    VirtualRegister des_i;

    LessF():LIRInstruction(LESS_F){}
    
    string GetStr() override;
};



//Memory
class LoadF:public LIRInstruction
{
public:
    VirtualRegister src_i; //address
    VirtualRegister des_f;

    LoadF():LIRInstruction(LOAD_F){}
    
    string GetStr() override;
};

class StoreF:public LIRInstruction
{
public:
    VirtualRegister src_f;
    VirtualRegister des_i; //address

    StoreF():LIRInstruction(STORE_F){}
    
    string GetStr() override;
};





//Share

//Move
class MoveI:public LIRInstruction
{
public:
    VirtualRegister src_i;
    VirtualRegister des_i;

    MoveI():LIRInstruction(MOVE_I){}
    
    string GetStr() override;
};

class MoveF:public LIRInstruction
{
public:
    VirtualRegister src_f;
    VirtualRegister des_f;

    MoveF():LIRInstruction(MOVE_F){}

    string GetStr() override;
};

class MoveI2F:public LIRInstruction
{
public:
    VirtualRegister src_i;
    VirtualRegister des_f;

    MoveI2F():LIRInstruction(MOVE_I2F){}
    
    string GetStr() override;
};

class MoveF2I:public LIRInstruction
{
public:
    VirtualRegister src_f;
    VirtualRegister des_i;

    MoveF2I():LIRInstruction(MOVE_F2I){}

    string GetStr() override;
};



//Control flow
class Call:public LIRInstruction
{
public:
    string function;

    Call():LIRInstruction(CALL){}
    
    string GetStr() override;
};

class Ret:public LIRInstruction
{
public:
    Ret():LIRInstruction(RET){}
    
    string GetStr() override;
};

class Bnez:public LIRInstruction
{
public:
    VirtualRegister src_i;
    int number;

    Bnez():LIRInstruction(BNEZ){}

    string GetStr() override;
};

class Jmp:public LIRInstruction
{
public:
    int number;

    Jmp():LIRInstruction(JMP){}
    
    string GetStr() override;
};



//Convert
class CvtI2F:public LIRInstruction
{
public:
    VirtualRegister src_i;
    VirtualRegister des_f;

    CvtI2F():LIRInstruction(CVT_I2F){}
    
    string GetStr() override;
};

class CvtF2I:public LIRInstruction
{
public:
    VirtualRegister src_f;
    VirtualRegister des_i;

    CvtF2I():LIRInstruction(CVT_F2I){}
    
    string GetStr() override;
};





//Custom
class GetI:public LIRInstruction
{
public:
    int src_localvar_number;
    VirtualRegister des_i;

    GetI():LIRInstruction(GETI){}

    string GetStr() override;
};

class GetF:public LIRInstruction
{
public:
    int src_localvar_number;
    VirtualRegister des_f;

    GetF():LIRInstruction(GETF){}

    string GetStr() override;
};

class PutI:public LIRInstruction
{
public:
    VirtualRegister src_i;
    int des_localvar_number;

    PutI():LIRInstruction(PUTI){}

    string GetStr() override;
};

class PutF:public LIRInstruction
{
public:
    VirtualRegister src_f;
    int des_localvar_number;

    PutF():LIRInstruction(PUTF){}

    string GetStr() override;
};

