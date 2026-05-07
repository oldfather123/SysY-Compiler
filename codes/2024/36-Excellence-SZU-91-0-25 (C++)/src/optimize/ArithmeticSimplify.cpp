#include "../optimize/Optimizer.h"
#include "../midend/MIR.h"
#include "../utility/Product.h"
#include "../utility/Diagnostor.h"

void Optimizer::ArithmeticSimplify()
{
    for(MIRFunction& mir_function:mir.functions)
    {
        for(MIRBasicBlock& mir_block:mir_function.basic_blocks)
        {
            for(shared_ptr<MIRInstruction>& mir_instruction_ptr:mir_block.instructions)
            {
                switch(mir_instruction_ptr->mir_instruction_type)
                {
                    case OR:{
                        shared_ptr<Or> _or=static_pointer_cast<Or>(mir_instruction_ptr);
                        //or a a=a
                        if(IsEqualValue(_or->a,_or->b))
                        {
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            copy->value=_or->a;
                            copy->variable=_or->c;
                            mir_instruction_ptr=copy;
                        }
                        //or a 1=1
                        else if(Is1(_or->a)||Is1(_or->b))
                        {
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            copy->value=make_shared<ValueConstant>(1);
                            copy->variable=_or->c;
                            mir_instruction_ptr=copy;
                        }
                        break;
                    }

                    case AND:{
                        shared_ptr<And> _and=static_pointer_cast<And>(mir_instruction_ptr);
                        //a and a=a
                        if(IsEqualValue(_and->a,_and->b))
                        {
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            copy->value=_and->a;
                            copy->variable=_and->c;
                            mir_instruction_ptr=copy;
                        }
                        //a and 0=0
                        else if(Is0(_and->a)||Is0(_and->b))
                        {
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            copy->value=make_shared<ValueConstant>(0);
                            copy->variable=_and->c;
                            mir_instruction_ptr=copy;
                        }
                        break;
                    }

                    case ADD:{
                        shared_ptr<Add> add=static_pointer_cast<Add>(mir_instruction_ptr);
                        //a + 0=a
                        if(Is0(add->b))
                        {   
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            copy->value=add->a;
                            copy->variable=add->c;
                            mir_instruction_ptr=copy;
                        }
                        //0 + b=b
                        else if(Is0(add->a))
                        {
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            copy->value=add->b;
                            copy->variable=add->c;
                            mir_instruction_ptr=copy;
                        }
                        break;
                    }

                    case SUB:{
                        shared_ptr<Sub> sub=static_pointer_cast<Sub>(mir_instruction_ptr);
                        //a - 0=a
                        if(Is0(sub->b))
                        {
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            copy->value=sub->a;
                            copy->variable=sub->c;
                            mir_instruction_ptr=copy;
                        }
                        break;
                    }

                    case MUL:{
                        shared_ptr<Mul> mul=static_pointer_cast<Mul>(mir_instruction_ptr);
                        //a * 0=0
                        if(Is0(mul->a)||Is0(mul->b))
                        {
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            copy->value=make_shared<ValueConstant>(0);
                            copy->variable=mul->c;
                            mir_instruction_ptr=copy;
                        }
                        //a * 1=a
                        else if(Is1(mul->b))
                        {
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            copy->value=mul->a;
                            copy->variable=mul->c;
                            mir_instruction_ptr=copy;
                        }
                        //1 * b=b
                        else if(Is1(mul->a))
                        {
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            copy->value=mul->b;
                            copy->variable=mul->c;
                            mir_instruction_ptr=copy;
                        }
                        break;
                    }

                    case DIV:{
                        shared_ptr<Div> div=static_pointer_cast<Div>(mir_instruction_ptr);
                        //0 / a=a
                        if(Is0(div->a))
                        {
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            copy->value=make_shared<ValueConstant>(0);
                            copy->variable=div->c;
                            mir_instruction_ptr=copy;
                        }
                        //a / 1=a
                        else if(Is1(div->b))
                        {
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            copy->value=div->a;
                            copy->variable=div->c;
                            mir_instruction_ptr=copy;
                        }
                        //a / a=1
                        else if(IsEqualValue(div->a,div->b))
                        {
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            copy->value=make_shared<ValueConstant>(1);
                            copy->variable=div->c;
                            mir_instruction_ptr=copy;
                        }
                        break;
                    }
                }
            }
        }
    }
}
