#pragma once

#include "../utility/System.h"
#include "../midend/Value.h"
#include "../utility/Type.h"

enum MIRInstructionType
{
    //Copy
    COPY,

    //Control flow
    BR,JUMP,
    CALLL,RETT, //To be different from LIR instruction :)

    //Arithmetic
    OR,AND,NOT,NEG,
    EQU,NEQU,LES,LEQU,GRE,GEQU,
    ADD,SUB,MUL,DIV,MOD,

    //Convert
    I2F,F2I
};

class MIRInstruction
{
public:
    MIRInstructionType mir_instruction_type;

    MIRInstruction(MIRInstructionType mir_instruction_type):
        mir_instruction_type(mir_instruction_type){}
    virtual ~MIRInstruction()= default;

    virtual string GetStr()=0;
};



//Copy
class Copy:public MIRInstruction
{
public:
    shared_ptr<Value> value;
    shared_ptr<ValueVariable> variable;

    Copy():MIRInstruction(COPY){}

    string GetStr() override;
};



//Control flow
class Branch:public MIRInstruction
{
public:
    shared_ptr<Value> condition;
    int true_number;
    int false_number;

    Branch():MIRInstruction(BR){}

    string GetStr() override;
};

class Jump:public MIRInstruction
{
public:
    int number;

    Jump():MIRInstruction(JUMP){}

    string GetStr() override;
};

class Calll:public MIRInstruction
{
public:
    string function_name;
    vector<shared_ptr<Value>> arguments;
    shared_ptr<ValueVariable> get_retvalue;
    
    Calll():MIRInstruction(CALLL){}

    string GetStr() override;
};

class Return:public MIRInstruction
{
public:
    shared_ptr<Value> return_value;

    Return():MIRInstruction(RETT){}
    
    string GetStr() override;
};



//Arithmetic
class Or:public MIRInstruction //only integer
{
public:
    shared_ptr<Value> a;
    shared_ptr<Value> b;
    shared_ptr<ValueVariable> c;

    Or():MIRInstruction(OR){}

    string GetStr() override;
};

class And:public MIRInstruction //only integer
{
public:
    shared_ptr<Value> a;
    shared_ptr<Value> b;
    shared_ptr<ValueVariable> c;

    And():MIRInstruction(AND){}

    string GetStr() override;
};

class Not:public MIRInstruction //only integer
{
public:
    shared_ptr<Value> a;
    shared_ptr<ValueVariable> b;

    Not():MIRInstruction(NOT){}

    string GetStr() override;
};

class Negative:public MIRInstruction
{
public:
    DataType data_type;

    shared_ptr<Value> a;
    shared_ptr<ValueVariable> b;

    Negative():MIRInstruction(NEG){}

    string GetStr() override;
};

class Equal:public MIRInstruction
{
public:
    DataType data_type;

    shared_ptr<Value> a;
    shared_ptr<Value> b;
    shared_ptr<ValueVariable> c;

    Equal():MIRInstruction(EQU){}

    string GetStr() override;
};

class NotEqual:public MIRInstruction
{
public:
    DataType data_type;

    shared_ptr<Value> a;
    shared_ptr<Value> b;
    shared_ptr<ValueVariable> c;

    NotEqual():MIRInstruction(NEQU){}

    string GetStr() override;
};

class Less:public MIRInstruction
{
public:
    DataType data_type;

    shared_ptr<Value> a;
    shared_ptr<Value> b;
    shared_ptr<ValueVariable> c;

    Less():MIRInstruction(LES){}

    string GetStr() override;
};

class LessEqual:public MIRInstruction
{
public:
    DataType data_type;

    shared_ptr<Value> a;
    shared_ptr<Value> b;
    shared_ptr<ValueVariable> c;

    LessEqual():MIRInstruction(LEQU){}

    string GetStr() override;
};

class Greater:public MIRInstruction
{
public:
    DataType data_type;

    shared_ptr<Value> a;
    shared_ptr<Value> b;
    shared_ptr<ValueVariable> c;

    Greater():MIRInstruction(GRE){}

    string GetStr() override;
};

class GreaterEqual:public MIRInstruction
{
public:
    DataType data_type;

    shared_ptr<Value> a;
    shared_ptr<Value> b;
    shared_ptr<ValueVariable> c;

    GreaterEqual():MIRInstruction(GEQU){}

    string GetStr() override;
};

class Add:public MIRInstruction
{
public:
    DataType data_type;

    shared_ptr<Value> a;
    shared_ptr<Value> b;
    shared_ptr<ValueVariable> c;

    Add():MIRInstruction(ADD){}

    string GetStr() override;
};

class Sub:public MIRInstruction
{
public:
    DataType data_type;

    shared_ptr<Value> a;
    shared_ptr<Value> b;
    shared_ptr<ValueVariable> c;

    Sub():MIRInstruction(SUB){}

    string GetStr() override;
};

class Mul:public MIRInstruction
{
public:
    DataType data_type;

    shared_ptr<Value> a;
    shared_ptr<Value> b;
    shared_ptr<ValueVariable> c;

    Mul():MIRInstruction(MUL){}

    string GetStr() override;
};

class Div:public MIRInstruction
{
public:
    DataType data_type;

    shared_ptr<Value> a;
    shared_ptr<Value> b;
    shared_ptr<ValueVariable> c;
    
    Div():MIRInstruction(DIV){}

    string GetStr() override;
};

class Mod:public MIRInstruction //only integer
{
public:
    shared_ptr<Value> a;
    shared_ptr<Value> b;
    shared_ptr<ValueVariable> c;

    Mod():MIRInstruction(MOD){}

    string GetStr() override;
};



//Convert
class ItoF:public MIRInstruction
{
public:
    shared_ptr<Value> a_i;
    shared_ptr<ValueVariable> b_f;

    ItoF():MIRInstruction(I2F){}

    string GetStr() override;
};

class FtoI:public MIRInstruction
{
public:
    shared_ptr<Value> a_f;
    shared_ptr<ValueVariable> b_i;

    FtoI():MIRInstruction(F2I){}

    string GetStr() override;
};
