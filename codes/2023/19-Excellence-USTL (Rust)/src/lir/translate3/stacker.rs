#![allow(unused)]

use crate::lir::lir::HRegister;
use crate::lir::translate3::allocator::RegisterHelper;

struct StackHelper {

}

impl StackHelper {
    pub fn init_16size() -> Self{
        StackHelper {

        }
    }

    pub fn new_load(&mut self,register_helper:&mut RegisterHelper,base_reg:HRegister,offset:i32){

    }
    pub fn new_store(&mut self,register_helper:&mut RegisterHelper,base_reg:HRegister,offset:i32){

    }
    pub fn new_word(&mut self){

    }
    pub fn new_array(&mut self){

    }

}