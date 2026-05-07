#include "../optimize/Optimizer.h"
#include "../midend/MIR.h"
#include "../utility/Product.h"
#include "../utility/Diagnostor.h"

void Optimizer::ConstantFold()
{
    for(MIRFunction& mir_function:mir.functions)
    {
        for(MIRBasicBlock& mir_block:mir_function.basic_blocks)
        {
            for(shared_ptr<MIRInstruction>& mir_instruction_ptr:mir_block.instructions)
            {
                switch(mir_instruction_ptr->mir_instruction_type)
                {
                    //Branch
                    case BR:{
                        shared_ptr<Branch> branch=static_pointer_cast<Branch>(mir_instruction_ptr);
                        if(branch->condition->IsValueConstant())
                        {
                            shared_ptr<ValueConstant> condition=GetValueConstantPtr(branch->condition);
                            int delete_number;
                            //@Instruction: jump
                            shared_ptr<Jump> jump=make_shared<Jump>();
                            if(condition->constant_value.i32)
                            {
                                jump->number=branch->true_number;
                                delete_number=branch->false_number;
                            }
                            else
                            {
                                jump->number=branch->false_number;
                                delete_number=branch->true_number;
                            }
                            mir_instruction_ptr=jump;
                            //Update basic block edge
                            mir_block.DeleteSucceed(delete_number);
                            mir_function.GetBasicBlock(delete_number).DeletePrecursor(mir_block.number);
                        }
                        break;
                    }

                    //Arithmetic
                    case OR:{
                        shared_ptr<Or> _or=static_pointer_cast<Or>(mir_instruction_ptr);
                        if(_or->a->IsValueConstant()&&_or->b->IsValueConstant())
                        {
                            shared_ptr<ValueConstant> a=GetValueConstantPtr(_or->a);
                            shared_ptr<ValueConstant> b=GetValueConstantPtr(_or->b);
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            copy->value=make_shared<ValueConstant>(a->constant_value.i32||b->constant_value.i32);
                            copy->variable=_or->c;
                            mir_instruction_ptr=copy;
                        }
                        break;
                    }   

                    case AND:{
                        shared_ptr<And> _and=static_pointer_cast<And>(mir_instruction_ptr);
                        if(_and->a->IsValueConstant()&&_and->b->IsValueConstant())
                        {
                            shared_ptr<ValueConstant> a=GetValueConstantPtr(_and->a);
                            shared_ptr<ValueConstant> b=GetValueConstantPtr(_and->b);
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            copy->value=make_shared<ValueConstant>(a->constant_value.i32&&b->constant_value.i32);
                            copy->variable=_and->c;
                            mir_instruction_ptr=copy;
                        }
                        break;
                    }
                    
                    case NOT:{
                        shared_ptr<Not> _not=static_pointer_cast<Not>(mir_instruction_ptr);
                        if(_not->a->IsValueConstant())
                        {
                            shared_ptr<ValueConstant> a=static_pointer_cast<ValueConstant>(_not->a);
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            copy->value=make_shared<ValueConstant>(!a->constant_value.i32);
                            copy->variable=_not->b;
                            mir_instruction_ptr=copy;
                        }
                        break;
                    }

                    case NEG:{
                        shared_ptr<Negative> neg=static_pointer_cast<Negative>(mir_instruction_ptr);
                        if(neg->a->IsValueConstant())
                        {
                            shared_ptr<ValueConstant> a=static_pointer_cast<ValueConstant>(neg->a);
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            switch(neg->data_type){
                                case I32:copy->value=make_shared<ValueConstant>(-a->constant_value.i32);break;
                                case F32:copy->value=make_shared<ValueConstant>(-a->constant_value.f32);break;
                                default:CompilerError("Invalid data type in ConstantFold() case NEG");
                            }
                            copy->variable=neg->b;
                            mir_instruction_ptr=copy;
                        }
                        break;
                    }

                    case EQU:{
                        shared_ptr<Equal> equal=static_pointer_cast<Equal>(mir_instruction_ptr);
                        if(equal->a->IsValueConstant()&&equal->b->IsValueConstant())
                        {
                            shared_ptr<ValueConstant> a=static_pointer_cast<ValueConstant>(equal->a);
                            shared_ptr<ValueConstant> b=static_pointer_cast<ValueConstant>(equal->b);
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            switch(equal->data_type){
                                case I32:copy->value=make_shared<ValueConstant>(a->constant_value.i32==b->constant_value.i32);break;
                                case F32:copy->value=make_shared<ValueConstant>(a->constant_value.f32==b->constant_value.f32);break;
                                default:CompilerError("Invalid data type in ConstantFold() case EQU");
                            }
                            copy->variable=equal->c;
                            mir_instruction_ptr=copy;
                        }   
                        break;
                    }

                    case NEQU:{
                        shared_ptr<NotEqual> not_equal=static_pointer_cast<NotEqual>(mir_instruction_ptr);
                        if(not_equal->a->IsValueConstant()&&not_equal->b->IsValueConstant())
                        {
                            shared_ptr<ValueConstant> a=static_pointer_cast<ValueConstant>(not_equal->a);
                            shared_ptr<ValueConstant> b=static_pointer_cast<ValueConstant>(not_equal->b);
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            switch(not_equal->data_type){
                                case I32:copy->value=make_shared<ValueConstant>(a->constant_value.i32!=b->constant_value.i32);break;
                                case F32:copy->value=make_shared<ValueConstant>(a->constant_value.f32!=b->constant_value.f32);break;
                                default:CompilerError("Invalid data type in ConstantFold() case NEQU");
                            }
                            copy->variable=not_equal->c;
                            mir_instruction_ptr=copy;
                        }
                        break;                        
                    }

                    case LES:{
                        shared_ptr<Less> less=static_pointer_cast<Less>(mir_instruction_ptr);
                        if(less->a->IsValueConstant()&&less->b->IsValueConstant())
                        {
                            shared_ptr<ValueConstant> a=static_pointer_cast<ValueConstant>(less->a);
                            shared_ptr<ValueConstant> b=static_pointer_cast<ValueConstant>(less->b);
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            switch(less->data_type){
                                case I32:copy->value=make_shared<ValueConstant>(a->constant_value.i32<b->constant_value.i32);break;
                                case F32:copy->value=make_shared<ValueConstant>(a->constant_value.f32<b->constant_value.f32);break;
                                default:CompilerError("Invalid data type in ConstantFold() case LES");
                            }
                            copy->variable=less->c;
                            mir_instruction_ptr=copy;
                        }
                        break;
                    }

                    case LEQU:{
                        shared_ptr<LessEqual> less_equal=static_pointer_cast<LessEqual>(mir_instruction_ptr);
                        if(less_equal->a->IsValueConstant()&&less_equal->b->IsValueConstant())
                        {
                            shared_ptr<ValueConstant> a=static_pointer_cast<ValueConstant>(less_equal->a);
                            shared_ptr<ValueConstant> b=static_pointer_cast<ValueConstant>(less_equal->b);
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            switch(less_equal->data_type){
                                case I32:copy->value=make_shared<ValueConstant>(a->constant_value.i32<=b->constant_value.i32);break;
                                case F32:copy->value=make_shared<ValueConstant>(a->constant_value.f32<=b->constant_value.f32);break;
                                default:CompilerError("Invalid data type in ConstantFold() case LEQU");
                            }
                            copy->variable=less_equal->c;
                            mir_instruction_ptr=copy;
                        }
                        break;
                    }

                    case GRE:{
                        shared_ptr<Greater> greater=static_pointer_cast<Greater>(mir_instruction_ptr);
                        if(greater->a->IsValueConstant()&&greater->b->IsValueConstant())
                        {
                            shared_ptr<ValueConstant> a=static_pointer_cast<ValueConstant>(greater->a);
                            shared_ptr<ValueConstant> b=static_pointer_cast<ValueConstant>(greater->b);
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            switch(greater->data_type){
                                case I32:copy->value=make_shared<ValueConstant>(a->constant_value.i32>b->constant_value.i32);break;
                                case F32:copy->value=make_shared<ValueConstant>(a->constant_value.f32>b->constant_value.f32);break;
                                default:CompilerError("Invalid data type in ConstantFold() case GRE");
                            }
                            copy->variable=greater->c;
                            mir_instruction_ptr=copy;
                        }            
                        break;
                    }

                    case GEQU:{
                        shared_ptr<GreaterEqual> greater_equal=static_pointer_cast<GreaterEqual>(mir_instruction_ptr);
                        if(greater_equal->a->IsValueConstant()&&greater_equal->b->IsValueConstant())
                        {
                            shared_ptr<ValueConstant> a=static_pointer_cast<ValueConstant>(greater_equal->a);
                            shared_ptr<ValueConstant> b=static_pointer_cast<ValueConstant>(greater_equal->b);
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            switch(greater_equal->data_type){
                                case I32:copy->value=make_shared<ValueConstant>(a->constant_value.i32>=b->constant_value.i32);break;
                                case F32:copy->value=make_shared<ValueConstant>(a->constant_value.f32>=b->constant_value.f32);break;
                                default:CompilerError("Invalid data type in ConstantFold() case GEQU");
                            }
                            copy->variable=greater_equal->c;
                            mir_instruction_ptr=copy;
                        }                
                        break;
                    }

                    case ADD:{
                        shared_ptr<Add> add=static_pointer_cast<Add>(mir_instruction_ptr);
                        if(add->a->IsValueConstant()&&add->b->IsValueConstant())
                        {
                            shared_ptr<ValueConstant> a=static_pointer_cast<ValueConstant>(add->a);
                            shared_ptr<ValueConstant> b=static_pointer_cast<ValueConstant>(add->b);
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            switch(add->data_type){
                                case I32:copy->value=make_shared<ValueConstant>(a->constant_value.i32+b->constant_value.i32);break;
                                case F32:copy->value=make_shared<ValueConstant>(a->constant_value.f32+b->constant_value.f32);break;
                                default:CompilerError("Invalid data type in ConstantFold() case ADD");
                            }
                            copy->variable=add->c;
                            mir_instruction_ptr=copy;
                        }
                        break;
                    }

                    case SUB:{
                        shared_ptr<Sub> sub=static_pointer_cast<Sub>(mir_instruction_ptr);
                        if(sub->a->IsValueConstant()&&sub->b->IsValueConstant())
                        {
                            shared_ptr<ValueConstant> a=static_pointer_cast<ValueConstant>(sub->a);
                            shared_ptr<ValueConstant> b=static_pointer_cast<ValueConstant>(sub->b);
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            switch(sub->data_type){
                                case I32:copy->value=make_shared<ValueConstant>(a->constant_value.i32-b->constant_value.i32);break;
                                case F32:copy->value=make_shared<ValueConstant>(a->constant_value.f32-b->constant_value.f32);break;
                                default:CompilerError("Invalid data type in ConstantFold() case SUB");
                            }
                            copy->variable=sub->c;
                            mir_instruction_ptr=copy;
                        }
                        break;
                    }

                    case MUL:{
                        shared_ptr<Mul> mul=static_pointer_cast<Mul>(mir_instruction_ptr);
                        if(mul->a->IsValueConstant()&&mul->b->IsValueConstant())
                        {
                            shared_ptr<ValueConstant> a=static_pointer_cast<ValueConstant>(mul->a);
                            shared_ptr<ValueConstant> b=static_pointer_cast<ValueConstant>(mul->b);
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            switch(mul->data_type){
                                case I32:copy->value=make_shared<ValueConstant>(a->constant_value.i32*b->constant_value.i32);break;
                                case F32:copy->value=make_shared<ValueConstant>(a->constant_value.f32*b->constant_value.f32);break;
                                default:CompilerError("Invalid data type in ConstantFold() case MUL");
                            }
                            copy->variable=mul->c;
                            mir_instruction_ptr=copy;
                        }
                        break;
                    }

                    case DIV:{
                        shared_ptr<Div> div=static_pointer_cast<Div>(mir_instruction_ptr);
                        if(div->a->IsValueConstant()&&div->b->IsValueConstant())
                        {
                            shared_ptr<ValueConstant> a=static_pointer_cast<ValueConstant>(div->a);
                            shared_ptr<ValueConstant> b=static_pointer_cast<ValueConstant>(div->b);
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            switch(div->data_type){
                                case I32:{
                                    if(b->constant_value.i32!=0)
                                        copy->value=make_shared<ValueConstant>(a->constant_value.i32/b->constant_value.i32);
                                    else Error("Can't div zero");//div 0
                                    break;
                                } 
                                case F32:{
                                    if(b->constant_value.f32!=0.0)
                                        copy->value=make_shared<ValueConstant>(a->constant_value.f32/b->constant_value.f32);
                                    else Error("Can't div zero");//div 0
                                    break;
                                }
                                default:CompilerError("Invalid data type in ConstantFold() case DIV");
                            }
                            copy->variable=div->c;
                            mir_instruction_ptr=copy;
                        }
                        break;
                    }

                    case MOD:{
                        shared_ptr<Mod> mod=static_pointer_cast<Mod>(mir_instruction_ptr);
                        if(mod->a->IsValueConstant()&&mod->b->IsValueConstant())
                        {
                            shared_ptr<ValueConstant> a=static_pointer_cast<ValueConstant>(mod->a);
                            shared_ptr<ValueConstant> b=static_pointer_cast<ValueConstant>(mod->b);
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            if(b->constant_value.i32!=0)
                                copy->value=make_shared<ValueConstant>(a->constant_value.i32%b->constant_value.i32);
                            else Error("Can't mod zero");//mod 0
                            copy->variable=mod->c;
                            mir_instruction_ptr=copy;
                        }
                        break;
                    }

                    //Convert
                    case I2F:{
                        shared_ptr<ItoF> i2f=static_pointer_cast<ItoF>(mir_instruction_ptr);
                        if(i2f->a_i->IsValueConstant())
                        {
                            shared_ptr<ValueConstant> a=static_pointer_cast<ValueConstant>(i2f->a_i);
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            copy->value=make_shared<ValueConstant>(float(a->constant_value.i32));
                            copy->variable=i2f->b_f;
                            mir_instruction_ptr=copy;
                        }
                        break;
                    }

                    case F2I:{
                        shared_ptr<FtoI> f2i=static_pointer_cast<FtoI>(mir_instruction_ptr);
                        if(f2i->a_f->IsValueConstant())
                        {
                            shared_ptr<ValueConstant> a=static_pointer_cast<ValueConstant>(f2i->a_f);
                            //@Instruction: copy
                            shared_ptr<Copy> copy=make_shared<Copy>();
                            copy->value=make_shared<ValueConstant>(int(a->constant_value.f32));
                            copy->variable=f2i->b_i;
                            mir_instruction_ptr=copy;
                        }
                        break;
                    }
                }
            }
        }
    }
}
