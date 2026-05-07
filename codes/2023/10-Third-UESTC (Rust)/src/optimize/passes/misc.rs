use crate::{
    common::immediate::Immediate,
    frontend::llvm::{
        function::Function,
        instr::{AShr, Div, Instruction, Mod, Mul, Shl},
        llvm_module::LLVMModule,
        ssa::SSARightValue,
    },
};

pub fn strength_reduction(llvm_module: &mut LLVMModule) {
    llvm_module.for_each_user_func_mut(strength_reduction_for_func);
}

fn strength_reduction_for_func(func: &mut Function) {
    let func_addr = func as *mut Function as usize;
    for bb_id in func.layout().block_iter() {
        for inst_id in func.layout().inst_iter(bb_id) {
            let func_recover = unsafe { &mut *(func_addr as *mut Function) };
            let inst = func_recover.instructions_mut().get_mut(&inst_id).unwrap();
            if let Some(mul) = inst.instr().as_any().downcast_ref::<Mul>() {
                // if s2 is const or immediate, we can use shift
                let value = if mul.s2().is_immediate() {
                    Some(*mul.s2().inner().as_immediate().unwrap().as_int().unwrap())
                }
                // else if mul.s2().is_const() {
                //     Some(todo!())
                // }
                else {
                    None
                };
                if let Some(value) = value {
                    // value must be positive
                    if value <= 0 {
                        continue;
                    }
                    // if value is 2^n, we can use shift
                    if value != 0 && value & (value - 1) == 0 {
                        let power = value.trailing_zeros();
                        // println!("{} is 2 raised to the power of {}.", value, power);
                        *inst = Instruction::new(
                            Box::new(Shl::new(
                                mul.d1().clone(),
                                mul.s1().clone(),
                                SSARightValue::new_imme(Immediate::new_int(power as _)),
                            )),
                            bb_id,
                        );
                    } else {
                        // println!("{} is not a power of 2.", value);
                    }
                }
            } else if let Some(div) = inst.instr().as_any().downcast_ref::<Div>() {
                // if s2 is const or immediate, we can use shift and mul and add
                let value = if div.s2().is_immediate() {
                    Some(*div.s2().inner().as_immediate().unwrap().as_int().unwrap())
                }
                // else if div.s2().is_const() {
                //     Some(todo!())
                // }
                else {
                    None
                };
                if let Some(value) = value {
                    // if value is positive
                    if value > 0 {
                        // if value is 2^n, we can use shift
                        if value != 0 && value & (value - 1) == 0 {
                            let power = value.trailing_zeros();
                            // println!("{} is 2 raised to the power of {}.", value, power);
                            if power == 1 {
                                *inst = Instruction::new(
                                    Box::new(AShr::new(
                                        div.d1().clone(),
                                        div.s1().clone(),
                                        SSARightValue::new_imme(Immediate::new_int(power as _)),
                                    )),
                                    bb_id,
                                );
                            }
                        } else {
                        }
                    } else if value == 0 {
                        panic!("div by zero")
                    }
                    // if value is negative
                    else {
                    }
                }
            } else if let Some(_srem) = inst.instr().as_any().downcast_ref::<Mod>() {
            } else {
                // do nothing
            }
        }
    }
}
