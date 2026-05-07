#include "code_gen.hpp"
#include "../ir/ir.hpp"
#include "inst.hpp"
#include "code_gen.hpp"
#include <cassert>
#include <memory>

LoongArch::Program::Program() {
    this->block_n = 0;
}

void LoongArch::Program::global_ini(ptr<int> pointer, ptr_list<ir::ir_value> init_val, ptr_list<ast::expr_syntax> dimensions, std::ostream& out) {
    // for(auto a : dimensions) {
    //     // if(dimensions.size() > 1)
    //         out << "[" << a->calc_res() << " x ";
    //     if(a == dimensions.back()) {
    //         // if(dimensions.size() == 1)
    //             out<<base_type[type];
    //         init_type = "[" + std::to_string(a->calc_res()) + " x " + base_type[type] + "]";
    //     }
    //     // out << "]";
    // }
    // for(auto a : dimensions) {
    //     // if(dimensions.size() > 1)
    //         out << "]";
    // }
    // out << " [";
    for(int i = 0; i < dimensions.front()->calc_res(); i++) {
        if(dimensions.size() > 1) {
            ptr_list<ast::expr_syntax> nxt_dim(dimensions.begin() + 1, dimensions.end());
            global_ini(pointer, init_val, nxt_dim, out);
            // if(i != dimensions.front()->calc_res() - 1) {
            //     out << ", ";
            // }
        }
        else {
            // init_val[*pointer]->accept(*this);
            out << "\t" << ".word" << "\t" << init_val[*pointer]->get_val() << std::endl;
            (*pointer)++;
            // if(i != dimensions.front()->calc_res() - 1) {
            //     out << ", ";
            // }
        }
    }
    // out << "]";
}

void LoongArch::Program::get_asm(std::ostream& out)
{
    //可以首先处理一些全局变量的问题
    out << ".align 8" << std::endl;
    out << ".data" << std::endl;
    for(auto var : global_var) {
        out << var->obj->get_addr()->get_name() << ":" << std::endl;
        // out << "\t";
        if(!var->init_val.empty()) {
            if(var->obj->get_dim()) {
                global_ini(std::make_shared<int>(0), var->init_val, var->obj->get_dim()->dimensions, out);
            }
            else {
                out << "\t" << ".word" << "\t" << var->init_val.front()->get_val() << std::endl;
            }
        }
        else {
            int total_cnt = 4;
            if(var->obj->get_dim()) {
                auto dimension = var->get_obj()->get_dim();
                for(auto dim : dimension->dimensions) {
                    total_cnt *= dim->calc_res();
                }
            }
            out << "\t" << ".space" << "\t" << total_cnt << std::endl;
        }
    }
    
    for(int i = 0; i < float_nums.size(); i++) {
        out << ".LC" << i << ":" << std::endl;
        out << "\t.word\t" << float_nums[i] << std::endl;
    }

    out << ".text" << '\n';      //标记代码段
    out << ".globl" << ' ' << "main" << '\n';    //标记main函数全局可见

    for(auto &func : lib_funcs) {
        out << ".extern " << func->get_name() << std::endl;
    }
    
    for(auto &func : functions){
        func->get_asm(out);
    }
}

LoongArch::Function::Function(std::string name): regn(0), stack_size(0) {
    this->name = name;
}

void LoongArch::Function::get_asm(std::ostream& out){
    out << this->name << " :" << '\n';
    for(auto &block : this->blocks)
        block->get_asm(out);
}

string LoongArch::Function::get_name() {
    return this->name;
}

int LoongArch::Function::bitalign(int value, int align) { 
    return (value + align - 1) & ~(align - 1);
}

LoongArch::Block::Block(std::string name) {
    this->name = name;
}

void LoongArch::Block::insert_before_jump(std::shared_ptr<Inst> inst) {
    auto i = std::end(instructions);
    while(i != instructions.begin()){
        auto prev_i = std::prev(i);
        if((*prev_i)->as<LoongArch::Jump>() || (*prev_i)->as<LoongArch::jr>()) {
            i = prev_i; 
        }else{
            break;
        }
    }
    this->instructions.insert(i,inst);
}

void LoongArch::Block::get_asm(std::ostream &out) {
    out << this->name << ':' << '\n';
    for(auto inst : this->instructions){
        out << '\t';
        inst->gen_asm(out);
    }
}

LoongArch::Reg LoongArch::IrMapping::new_reg() {
    // if(regn == 21)       // 这里在钻空子！
        // regn = 6;        // 这里在钻空子！
    return Reg{regn++};
}

LoongArch::Reg LoongArch::IrMapping::transfer_reg(ir::ir_reg irReg) {
    auto it = reg_mapping.find(irReg.id);
    if(it != reg_mapping.end()) return it->second;
    // std::shared_ptr<ir::ir_reg> reg(&irReg);
    auto spill_it = spill_mapping.find(irReg.id);
    if(spill_it != spill_mapping.end()) return spill_it->second;
    bool unreachable = false;
    assert(unreachable);  // 经过染色图分配后下面的分配新寄存器正常情况下不会执行
    Reg ret = new_reg();
    reg_mapping[irReg.id] = ret;
    return ret;
}


LoongArch::IrMapping::IrMapping() : regn(12) {}