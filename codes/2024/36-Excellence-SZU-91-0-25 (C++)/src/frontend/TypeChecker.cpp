#include "../frontend/TypeChecker.h"
#include "../frontend/MIREmitter.h"
#include "../utility/Diagnostor.h"

void Check(shared_ptr<Value>& value,DataType expected)
{
    DataType value_type=ValueDataType(value);
    
    if(value_type!=expected)
    {
        if(value_type==I32&&expected==F32)
        {
            //@Instruction: i2f
            shared_ptr<ItoF> i2f=make_shared<ItoF>();
            i2f->a_i=value;
            i2f->b_f=mir_emitter.InFunction().NewTempLocalVar(F32);

            value=i2f->b_f;
            mir_emitter.InBasicBlock().AddInsturction(i2f);
        }
        else if(value_type==F32&&expected==I32)
        {
            //@Instruction: f2i
            shared_ptr<FtoI> f2i=make_shared<FtoI>();
            f2i->a_f=value;
            f2i->b_i=mir_emitter.InFunction().NewTempLocalVar(I32);
        
            value=f2i->b_i;
            mir_emitter.InBasicBlock().AddInsturction(f2i);
        }
        else TypeError("Can't convert "+DataTypeText[value_type]+" to "+DataTypeText[expected]);
    }
}



void CheckBinary(shared_ptr<Value>&a,shared_ptr<Value>&b)
{
    DataType a_type=ValueDataType(a);
    DataType b_type=ValueDataType(b);

    if(a_type==I32&&b_type==F32)
    {
        //@Instruction: i2f
        shared_ptr<ItoF> i2f=make_shared<ItoF>();
        i2f->a_i=a;
        i2f->b_f=mir_emitter.InFunction().NewTempLocalVar(F32);

        a=i2f->b_f;
        mir_emitter.InBasicBlock().AddInsturction(i2f);
    }
    else if(a_type==F32&&b_type==I32)
    {
        //@Instruction: i2f
        shared_ptr<ItoF> i2f=make_shared<ItoF>();
        i2f->a_i=b;
        i2f->b_f=mir_emitter.InFunction().NewTempLocalVar(F32);

        b=i2f->b_f;
        mir_emitter.InBasicBlock().AddInsturction(i2f);
    }
}



void CheckCopy(shared_ptr<Copy>& copy) 
{
    Check(copy->value,copy->variable->data_type);
}



void CheckOr(shared_ptr<Or>& _or) 
{
    Check(_or->a,I32);
    Check(_or->b,I32);
}

void CheckAnd(shared_ptr<And>& _and) 
{
    Check(_and->a,I32);
    Check(_and->b,I32);
}

void CheckNot(shared_ptr<Not>& _not)
{
    Check(_not->a,I32);
}



void CheckEqual(shared_ptr<Equal>& equal) 
{
    CheckBinary(equal->a,equal->b);
    equal->data_type=ValueDataType(equal->a);
}

void CheckNotEqual(shared_ptr<NotEqual>& not_equal) 
{
    CheckBinary(not_equal->a,not_equal->b);
    not_equal->data_type=ValueDataType(not_equal->a);
}

void CheckLess(shared_ptr<Less>& less) 
{
    CheckBinary(less->a,less->b);
    less->data_type=ValueDataType(less->a);
}

void CheckLessEqual(shared_ptr<LessEqual>& less_equal) 
{
    CheckBinary(less_equal->a,less_equal->b);
    less_equal->data_type=ValueDataType(less_equal->a);
}

void CheckGreater(shared_ptr<Greater>& greater) 
{
    CheckBinary(greater->a,greater->b);
    greater->data_type=ValueDataType(greater->a);
}

void CheckGreaterEqual(shared_ptr<GreaterEqual>& greater_equal) 
{
    CheckBinary(greater_equal->a,greater_equal->b);
    greater_equal->data_type=ValueDataType(greater_equal->a);
}



void CheckAdd(shared_ptr<Add>& add) 
{
    CheckBinary(add->a,add->b);
    add->data_type=ValueDataType(add->a);
}

void CheckSub(shared_ptr<Sub>& sub) 
{
    CheckBinary(sub->a,sub->b);
    sub->data_type=ValueDataType(sub->a);
}

void CheckMul(shared_ptr<Mul>& mul) 
{
    CheckBinary(mul->a,mul->b);
    mul->data_type=ValueDataType(mul->a);
}

void CheckDiv(shared_ptr<Div>& div) 
{
    CheckBinary(div->a,div->b);
    div->data_type=ValueDataType(div->a);
}

void CheckMod(shared_ptr<Mod>& mod,Cursor location) 
{
    if(ValueDataType(mod->a)==F32||ValueDataType(mod->b)==F32)
        TypeError("Mod is only valid on integers",location);
}