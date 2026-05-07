#include "HAsmFunc.hh"
#include "HAsmVal.hh"
#include "HAsm2Asm.hh"

#include "logging.hh"

HAsmBlock *HAsmFunc::create_bb(BasicBlock *bb, Label *label) { 
    auto new_bb = new HAsmBlock(this, bb, label);
    blocks_list_.push_back(new_bb);
    return new_bb;
}

std::string HAsmFunc::get_asm_code() {
    std::string asm_code;
    asm_code += func_header_;
    asm_code += f_->get_name() + ":" + HAsm2Asm::newline;
    for(auto bb: blocks_list_) {
        asm_code += bb->get_asm_code();
    }
    asm_code += HAsm2Asm::space + ".size" + HAsm2Asm::space + f_->get_name() + ", " + ".-" + f_->get_name() + HAsm2Asm::newline;
    return asm_code;
}


std::string HAsmFunc::print() {
    std::string hasm_code;
    hasm_code += func_header_;
    hasm_code += f_->get_name() + ":" + HAsm2Asm::newline;
    for(auto bb: blocks_list_) {
        hasm_code += bb->print();
    }
    hasm_code += HAsm2Asm::space + ".size" + HAsm2Asm::space + f_->get_name() + ", " + ".-" + f_->get_name() + HAsm2Asm::newline;
    return hasm_code;
}