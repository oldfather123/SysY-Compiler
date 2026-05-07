#include "program_builder.hpp"
#include "../ir/ir.hpp"
#include "arch.hpp"
#include "code_gen.hpp"
#include "inst.hpp"
#include "../parser/SyntaxTree.hpp"
#include <algorithm>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
bool flag = true;

void LoongArch::ProgramBuilder::check_write_back(LoongArch::Reg tar) {
    auto it = std::find(cur_mapping->spill_vec.begin(), cur_mapping->spill_vec.end(), tar.ir_id);
    if(it != cur_mapping->spill_vec.end()) {
        // int idx = std::distance(cur_mapping->spill_vec.begin(), it);
        // int offset = cur_func->stack_size - (4 * idx);
        int offset = cur_func->stack_size/* - cur_mapping->call_mem*/ - cur_mapping->spill_offset.find(tar.ir_id)->second;
        if(tar.is_arr) {
            // if((uint32_t)offset >> 12) {
            //     cur_block->instructions.push_back(std::make_shared<LoadImm>(const_reg_l, offset));
            //     cur_block->instructions.push_back(std::make_shared<RegRegInst>(RegRegInst::sub_d, const_reg_l, Reg{fp}, const_reg_l));
            //     cur_block->instructions.push_back(std::make_shared<LoongArch::st>(tar, const_reg_l, 0, tar.is_float() ? st::fst_f : st::st_d));
            // }
            // else
                cur_block->instructions.push_back(std::make_shared<LoongArch::st>(tar, Reg{fp}, -offset, tar.is_float() ? st::fst_f : st::st_d));
        }
        else {
            // if((uint32_t)offset >> 12) {
            //     cur_block->instructions.push_back(std::make_shared<LoadImm>(const_reg_l, offset));
            //     cur_block->instructions.push_back(std::make_shared<RegRegInst>(RegRegInst::sub_d, const_reg_l, Reg{fp}, const_reg_l));
            //     cur_block->instructions.push_back(std::make_shared<LoongArch::st>(tar, const_reg_l, 0, tar.is_float() ? st::fst_f : st::st_w));
            // }
            // else
                cur_block->instructions.push_back(std::make_shared<LoongArch::st>(tar, Reg{fp}, -offset, tar.is_float() ? st::fst_f : st::st_w));
        }
    }
}

void LoongArch::ProgramBuilder::visit(ir::ir_reg &node) {
    auto it = std::find(cur_mapping->spill_vec.begin(), cur_mapping->spill_vec.end(), node.id);
    if(it != cur_mapping->spill_vec.end()) {
        Reg tar;
        if(node.get_type() == vartype::FLOAT/* || node.get_type() == vartype::FLOATADDR*/) {
            tar.type = FLOAT;
        }
        else {
            tar.type = INT;
        }
        tar.ir_id = node.id;
        tar.is_arr = node.is_arr;
        if(is_dst) {
            is_dst = false;
            tar.id = spill_dst.id;
            pass_reg = tar;
            return;
        }
        else {
            if(flag) {
                tar.id = spill_use_1.id;
            }
            else {
                tar.id = spill_use_2.id;
            }
            flag = !flag;
        }
        // int idx = std::distance(cur_mapping->spill_vec.begin(), it);
        // int offset = cur_func->stack_size - (4 * idx);
        int offset = cur_func->stack_size/* - cur_mapping->call_mem*/ - cur_mapping->spill_offset.find(node.id)->second;
        if(node.is_arr) {
            cur_block->instructions.push_back(std::make_shared<LoongArch::ld>(tar, Reg{fp}, -offset, tar.is_float() ? ld::fld_f : ld::ld_d));
        }
        else {
            cur_block->instructions.push_back(std::make_shared<LoongArch::ld>(tar, Reg{fp}, -offset, tar.is_float() ? ld::fld_f : ld::ld_w));
        }
        pass_reg = tar;
    }
    else {
        is_dst = false;
        pass_reg = cur_mapping->transfer_reg(node);
        pass_reg.ir_id = node.id;
    }
}

void LoongArch::ProgramBuilder::visit(ir::ir_constant &node) {
    if(is_dst) {
        abort();
    }
    auto optional_val = node.init_val;
    if(node.get_type() == vartype::FLOAT || node.get_type() == vartype::FLOATADDR) {
        using_reg.type = FLOAT;
    }
    else {
        using_reg.type = INT;
    }
    if(optional_val.has_value()) {
        auto variant_val = optional_val.value();
        if(std::holds_alternative<int>(variant_val)) {
            int val = std::get<int>(variant_val);
            cur_block->instructions.push_back(std::make_shared<LoongArch::LoadImm>(using_reg, val));
        } else if(std::holds_alternative<float>(variant_val)) {
            float val = std::get<float>(variant_val);
            // Reg middle_reg{using_reg.id};
            // cur_block->instructions.push_back(std::make_shared<LoongArch::LoadFloat>(middle_reg, prog->float_nums.size()));
            // cur_block->instructions.push_back(std::make_shared<LoongArch::ld>(using_reg, middle_reg, 0, ld::fld_f));
            cur_block->instructions.push_back(std::make_shared<LoongArch::LoadFloat>(using_reg, prog->float_nums.size()));
            // double double_num = static_cast<double>(val);
            std::bitset<32> bits(*reinterpret_cast<unsigned int*>(&val));
            std::stringstream ss;
            ss << std::hex << std::setw(8) << std::setfill('0') << bits.to_ulong();
            prog->float_nums.push_back("0x" + ss.str());
        }
    }
    pass_reg = using_reg;
}

void LoongArch::ProgramBuilder::visit(ir::ir_basicblock &node) 
{
    for(auto & i : node.instructions) {
        // auto is_phi = std::dynamic_pointer_cast<ir::phi>(i);
        // if(is_phi != nullptr) {
        //     continue;
        // }
        // std::vector<std::shared_ptr<ir::ir_reg>> regs;
        // for(auto def : i->def_reg()) {
        //     auto reg = std::dynamic_pointer_cast<ir::ir_reg>(def);
        //     auto it = std::find(cur_mapping->spill_vec.begin(), cur_mapping->spill_vec.end(), reg);
        //     if(it != cur_mapping->spill_vec.end())
        //         regs.push_back(reg);
        // }
        // for(auto use : i->use_reg()) {
        //     auto reg = std::dynamic_pointer_cast<ir::ir_reg>(use);
        //     auto it = std::find(cur_mapping->spill_vec.begin(), cur_mapping->spill_vec.end(), reg);
        //     if(it != cur_mapping->spill_vec.end())
        //         regs.push_back(reg);
        // }
        // assert(regs.size() <= 3);       // 理论上一条LA指令涉及寄存器不会超过3个
        // int spill_idx = spill_base;
        // for(auto reg : regs) {
        //     auto tar = Reg{spill_idx++};
        //     auto it = std::find(cur_mapping->spill_vec.begin(), cur_mapping->spill_vec.end(), reg);
        //     int idx = std::distance(cur_mapping->spill_vec.begin(), it);
        //     int offset = cur_func->stack_size - (4 * idx);
        //     cur_block->instructions.push_back(std::make_shared<LoongArch::ld>(tar, Reg{fp}, -offset, ld::ld_w));
        //     cur_mapping->spill_mapping[reg->id] = tar;
        // }

        i->accept(*this); // ！！由于phi指令已消解，所以phi指令对应的visit方法不用实现，也说明为什么上面判断是phi指令可以使用continue！！

        // spill_idx = spill_base;
        // for(auto def : i->def_reg()) {
        //     auto reg = std::dynamic_pointer_cast<ir::ir_reg>(def);
        //     auto it = std::find(cur_mapping->spill_vec.begin(), cur_mapping->spill_vec.end(), reg);
        //     if(it != cur_mapping->spill_vec.end())
        //         regs.push_back(reg);
        // }
        // for(auto reg : regs) {
        //     auto tar = spill_dst;
        //     auto it = std::find(cur_mapping->spill_vec.begin(), cur_mapping->spill_vec.end(), reg);
        //     int idx = std::distance(cur_mapping->spill_vec.begin(), it);
        //     int offset = cur_func->stack_size - (4 * idx);
        //     cur_block->instructions.push_back(std::make_shared<LoongArch::st>(tar, Reg{fp}, -offset, st::st_w));
        //     // auto del_it = cur_mapping->spill_mapping.find(reg->id);
        //     // if(del_it != cur_mapping->spill_mapping.end())
        //         // cur_mapping->spill_mapping.erase(del_it);
        // }
        // for(auto reg :regs) {
        //     auto tar = Reg{spill_idx++};
        //     auto it = std::find(cur_mapping->spill_vec.begin(), cur_mapping->spill_vec.end(), reg);
        //     int idx = std::distance(cur_mapping->spill_vec.begin(), it);
        //     int offset = cur_func->stack_size - (4 * idx);
        //     cur_block->instructions.push_back(std::make_shared<LoongArch::st>(tar, Reg{fp}, -offset, st::st_w));
        //     auto del_it = cur_mapping->spill_mapping.find(reg->id);
        //     if(del_it != cur_mapping->spill_mapping.end())
        //         cur_mapping->spill_mapping.erase(del_it);
        // }
    }
}

void LoongArch::ProgramBuilder::visit(ir::ir_module &node) { 

    this->prog = std::make_shared<LoongArch::Program>();
    this->cur_mapping = std::make_shared<IrMapping>();

    for(auto & [name, global] : node.global_var) {
        prog->global_var.push_back(global);
    }

    //可以调用寄存器分配函数进行寄存器分配
    node.reg_allocate(cur_mapping->regn, prog->global_var);       // 新增了传入分配起始地址的信息，方便分配

    for(auto & [name, func] : node.libfuncs) {
        prog->lib_funcs.push_back(std::make_shared<LoongArch::Function>(name));
    }

    if(node.init_block) {
        this->func_name = node.global_init_func->name;
        prog->functions.emplace_back(std::make_shared<LoongArch::Function>(node.global_init_func->name));
        this->cur_mapping = std::make_shared<IrMapping>();
        node.global_init_func->accept(*this);
    }

    if(node.enable_mem_set) {
        auto fun = std::make_shared<LoongArch::Function>("Looouiiis_self_memset");
        prog->functions.emplace_back(fun);
        auto loop_block = std::make_shared<LoongArch::Block>("Looouiiis_self_memset_loop");
        auto ret_block = std::make_shared<LoongArch::Block>("Looouiiis_self_memset_ret");
        loop_block->instructions.push_back(std::make_shared<LoongArch::Br>(Br::beqz, spill_use_1, ret_block.get()));
        loop_block->instructions.push_back(std::make_shared<LoongArch::st>(Reg{0}, spill_dst, 0, st::st_w));
        loop_block->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::add_d, spill_dst, spill_dst, spill_use_2));
        loop_block->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::sub_d, spill_use_1, spill_use_1, spill_use_2));
        loop_block->instructions.push_back(std::make_shared<LoongArch::Jump>(loop_block.get()));
        ret_block->instructions.push_back(std::make_shared<LoongArch::jr>(true));
        fun->blocks.push_back(loop_block);
        fun->blocks.push_back(ret_block);
    }

    for(auto & [name,func] : node.usrfuncs){
        this->func_name = name;
        prog->functions.emplace_back(std::make_shared<LoongArch::Function>(name));
        this->cur_mapping = std::make_shared<IrMapping>();
        func->accept(*this);
    }
    //后面可以用来处理全局变量，不过这里没有
}

void LoongArch::ProgramBuilder::visit(ir::ir_userfunc &node) {
    
    this->cur_func = prog->functions.back();
    
    this->cur_func->stack_size = 16; //默认为16字节，分别存储$ra和父函数的fp值

    for(auto [ir_reg,backend_reg] : node.regAllocateOut)
    {
        this->cur_mapping->reg_mapping[ir_reg->id] = backend_reg;
    }

    for(auto reg : node.regSpill) {
        auto it = std::find(this->cur_mapping->spill_vec.begin(), this->cur_mapping->spill_vec.end(), reg->id);
        if(it != this->cur_mapping->spill_vec.end()) {
            abort();
        }
        this->cur_mapping->spill_vec.push_back(reg->id);
        this->cur_mapping->spill_offset.insert({reg->id, cur_mapping->used_mem});
        this->cur_mapping->used_mem += reg->size;
    }
    
    for(auto obj : node.arrobj) {
        this->cur_mapping->mem_var.insert({obj->addr->id, this->cur_mapping->used_mem});
        for(auto spilled_reg : obj->get_spilled_reg()) {
            this->cur_mapping->mem_var.insert({spilled_reg->id, this->cur_mapping->used_mem});
        }
        int total_cnt = obj->addr->size;
        if(obj->dim) {
            for(auto a : obj->dim->dimensions) {
                total_cnt *= a->calc_res();
            }
        }
        this->cur_mapping->used_mem += total_cnt; 
    }

    // int spill_cnt = node.regSpill.size();
    // cur_func->stack_size += spill_cnt * 4;

    // 查找函数调用指令，获取调用时放入内存的参数数量
    int max_call = 0;
    for(auto bb : node.bbs) {
        for(auto ins : bb->instructions) {
            auto is_call = std::dynamic_pointer_cast<ir::func_call>(ins);
            if(is_call) {
                int f_par_num = 0;
                int i_par_num = 0;
                for(auto par : is_call->params) {
                    if(par->get_type() == vartype::FLOAT/* || par->get_type() == vartype::FLOATADDR*/) {
                        f_par_num++;
                    }
                    else {
                        i_par_num++;
                    }
                }
                if(f_par_num > 8) {
                    i_par_num += f_par_num - 8;
                }
                int total = i_par_num <= 8 ? 0 : i_par_num - 8;
                // int total = (i_par_num <= 8 ? 0 : i_par_num - 8) + (f_par_num <= 8 ? 0 : f_par_num - 8);
                max_call = std::max(total, max_call);
            }
        }
    }
    this->cur_mapping->call_mem += max_call * 8;

    cur_func->stack_size += this->cur_mapping->used_mem;
    if(cur_func->stack_size % 8 != 0) {
        cur_func->stack_size += (8 - cur_func->stack_size % 8);
    }

    // if((cur_func->stack_size % 16) != 0) {
        // cur_func->stack_size += 16 - (cur_func->stack_size % 16);
    // }
    // cur_func->stack_size += this->cur_mapping->call_mem;

    // for(auto obj : node.arrobj) {
    //     // this->cur_mapping->mem_var.insert({obj->addr->id, this->cur_mapping->used_mem});
    //     auto offset = this->cur_mapping->mem_var.find(obj->addr->id)->second;
    //     std::cout << "# " << obj->name << " -> " << -(cur_func->stack_size - offset) << std::endl; 
    // }
    
    //处理内存变量

    //build a entry
    cur_func->entry = std::make_shared<LoongArch::Block>(".entry_" + func_name);
    cur_func->blocks.push_back(cur_func->entry);
    prog->block_n++;
    
    //构造函数的进入部分
    // if((uint32_t)cur_func->stack_size >> 12) {
    //     this->cur_func->entry->instructions.push_back(std::make_shared<LoongArch::LoadImm>(const_reg_l, cur_func->stack_size));
    //     this->cur_func->entry->instructions.push_back(std::make_shared<LoongArch::LoadImm>(const_reg_r, -1));
    //     this->cur_func->entry->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::mul_d, const_reg_l, const_reg_l, const_reg_r));
    //     this->cur_func->entry->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::add_d,Reg{sp},Reg{sp},const_reg_l));  //sp:栈顶指针
    // }
    // else
    this->cur_func->entry->instructions.push_back(std::make_shared<LoongArch::RegImmInst>(RegImmInst::addi_d,Reg{sp},Reg{sp},-cur_func->stack_size));  //sp:栈顶指针
    this->cur_func->entry->instructions.push_back(std::make_shared<LoongArch::st>(Reg{ra},Reg{sp},cur_func->stack_size-8,st::st_d));//保存ra//返回地址
    this->cur_func->entry->instructions.push_back(std::make_shared<LoongArch::st>(Reg{fp},Reg{sp},cur_func->stack_size - 16,st::st_d));//保存ra//返回地址
    // if((uint32_t)cur_func->stack_size >> 12) {
    //     this->cur_func->entry->instructions.push_back(std::make_shared<LoongArch::LoadImm>(const_reg_l, cur_func->stack_size));
    //     this->cur_func->entry->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::add_d,Reg{sp},Reg{sp},const_reg_l));
    // }
    // else
    this->cur_func->entry->instructions.push_back(std::make_shared<LoongArch::RegImmInst>(RegImmInst::addi_d,Reg{fp},Reg{sp},cur_func->stack_size)); //确定
    
    int i_pointer = 10;
    int f_pointer = 10;
    int cur_stk = 0;
    auto bak = cur_block;
    cur_block = cur_func->entry;
    for(auto arg : node.func_args) {
        auto is_spilled = node.spilled_args.find(arg->addr);
        if(is_spilled != node.spilled_args.end()) {
            auto param = is_spilled->first;
            auto spill_obj = is_spilled->second;
            auto middle = const_reg_l;
            int offset = cur_mapping->mem_var.find(spill_obj->addr->id)->second;
            offset = cur_func->stack_size - offset;
            if(param->type == vartype::FLOAT) {
                middle.type = FLOAT;
                if(f_pointer < 18) {
                    cur_func->entry->instructions.push_back(std::make_shared<LoongArch::st>(Reg{f_pointer++, Rtype::FLOAT}, Reg{fp}, -offset, param->is_arr ? st::fst_d : st::fst_f));
                }
                else {
                    if(i_pointer < 18) {
                        cur_func->entry->instructions.push_back(std::make_shared<LoongArch::mov>(middle, Reg{i_pointer++}, mov::gtf));
                        cur_func->entry->instructions.push_back(std::make_shared<LoongArch::st>(middle, Reg{fp}, -offset, param->is_arr ? st::fst_d : st::fst_f));
                    }
                    else {
                        if(arg->addr->is_arr) {
                            cur_func->entry->instructions.push_back(std::make_shared<ld>(middle, Reg{fp}, cur_stk, ld::fld_d));
                            cur_func->entry->instructions.push_back(std::make_shared<LoongArch::st>(middle, Reg{fp}, -offset, st::fst_d));
                        }
                        else {
                            cur_func->entry->instructions.push_back(std::make_shared<ldptr>(middle, Reg{fp}, cur_stk, ldptr::fld_d));
                            cur_func->entry->instructions.push_back(std::make_shared<LoongArch::st>(middle, Reg{fp}, -offset, st::fst_f));
                        }
                        cur_stk += 8;
                    }
                }
            }
            else {
                if(i_pointer < 18) {
                    cur_func->entry->instructions.push_back(std::make_shared<st>(Reg{i_pointer++}, Reg{fp}, -offset, param->is_arr ? st::st_d : st::st_w));
                }
                else {
                    if(arg->addr->is_arr) {
                        cur_func->entry->instructions.push_back(std::make_shared<ld>(middle, Reg{fp}, cur_stk, ld::ld_d));
                        cur_func->entry->instructions.push_back(std::make_shared<LoongArch::st>(middle, Reg{fp}, -offset, st::st_d));
                    }
                    else {
                        cur_func->entry->instructions.push_back(std::make_shared<ldptr>(middle, Reg{fp}, cur_stk, ldptr::ld_d));
                        cur_func->entry->instructions.push_back(std::make_shared<LoongArch::st>(middle, Reg{fp}, -offset, st::st_w));
                    }
                    cur_stk += 8;
                }
            }
        }
        else {
            is_dst = true;
            arg->addr->accept(*this);
            auto reg = pass_reg;
            if(reg.is_float()) {
                if(f_pointer < 18) {
                    cur_func->entry->instructions.push_back(std::make_shared<LoongArch::mov>(reg, Reg{f_pointer++, Rtype::FLOAT}, mov::ftf_f));
                }
                else {
                    if(i_pointer < 18) {
                        cur_func->entry->instructions.push_back(std::make_shared<LoongArch::mov>(reg, Reg{i_pointer++}, mov::gtf));
                    }
                    else {
                        if(arg->addr->is_arr) {
                            cur_func->entry->instructions.push_back(std::make_shared<ld>(reg, Reg{fp}, cur_stk, ld::fld_d));
                        }
                        else {
                            cur_func->entry->instructions.push_back(std::make_shared<ldptr>(reg, Reg{fp}, cur_stk, ldptr::fld_d));
                        }
                        cur_stk += 8;
                    }
                }
            }
            else {
                if(i_pointer < 18) {
                    cur_func->entry->instructions.push_back(std::make_shared<RegRegInst>(RegRegInst::add_d, reg, Reg{i_pointer++}, Reg{0}));
                }
                else {
                    if(arg->addr->is_arr) {
                        cur_func->entry->instructions.push_back(std::make_shared<ld>(reg, Reg{fp}, cur_stk, ld::ld_d));
                    }
                    else {
                        cur_func->entry->instructions.push_back(std::make_shared<ldptr>(reg, Reg{fp}, cur_stk, ldptr::ld_d));
                    }
                    cur_stk += 8;
                }
            }
            check_write_back(reg);
        }
    }
    cur_block = bak;
    // int pointer = 4 + node.func_args.size();
    // ptr_list<ir::ir_reg> f_par;
    // ptr_list<ir::ir_reg> i_par;
    // for(auto arg : node.func_args) {
    //     // is_dst = true;
    //     // arg->addr->accept(*this);
    //     // auto reg = pass_reg;
    //     if(arg->addr->type == vartype::FLOAT || arg->addr->type == vartype::FLOATADDR) {
    //         f_par.push_back(arg->addr);
    //     }
    //     else {
    //         i_par.push_back(arg->addr);
    //     }
    //     // cur_func->entry->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::add_d, reg, Reg{--pointer}, Reg{0}));
    //     // check_write_back(reg);
    // }
    // int i_pointer = 4;
    // int f_pointer = 0;
    // int cur_stk;
    // for(auto par : i_par) {
    //     is_dst = true;
    //     par->accept(*this);
    //     auto reg = pass_reg;
    //     if(i_pointer < 12) {
    //         cur_func->entry->instructions.push_back(std::make_shared<RegRegInst>(RegRegInst::add_d, reg, Reg{i_pointer++}, Reg{0}));
    //     }
    //     else {
    //         if(par->is_arr) {
    //             cur_func->entry->instructions.push_back(std::make_shared<ld>(reg, Reg{fp}, cur_stk, ld::ld_d));
    //         }
    //         else {
    //             cur_func->entry->instructions.push_back(std::make_shared<ldptr>(reg, Reg{fp}, cur_stk, ldptr::ld_d));
    //         }
    //         cur_stk += 8;
    //     }
    //     check_write_back(reg);
    // }
    // for(auto par : f_par) {
    //     is_dst = true;
    //     par->accept(*this);
    //     auto reg = pass_reg;
    //     cur_func->entry->instructions.push_back(std::make_shared<LoongArch::mov>(reg, Reg{f_pointer++, Rtype::FLOAT}, mov::ftf_f));
    //     check_write_back(reg);
    // }

    
    for(auto & block : node.bbs){
        auto cur_block_name = ".L" + std::to_string(prog->block_n);
        auto backend_block = std::make_shared<LoongArch::Block>(cur_block_name);
        //把entry的终点设定成L0
        if(cur_block_name == ".L1"){
            this->cur_func->entry->instructions.push_back(std::make_shared<LoongArch::Jump>(backend_block.get()));       
        }
        prog->block_n++;
        //确定映射关系
        this->cur_mapping->blockmapping[block] = backend_block;
        this->cur_mapping->rev_blockmapping[backend_block] = block;

        cur_func->blocks.push_back(backend_block);
    }

    //在遍历block的过程中，可能会牵扯到下一个block
    for(int i = 0; i < cur_func->blocks.size(); ++i){
        if(cur_func->blocks[i] != cur_func->entry){
            this->cur_block = cur_func->blocks[i];
            if(i < cur_func->blocks.size() - 1) this->next_block = cur_func->blocks[i+1];
            else this->next_block = nullptr;
            this->cur_mapping->rev_blockmapping[cur_func->blocks[i]]->accept(*this);
        }
    }

    for(auto block : cur_func->waiting_blocks) {
        cur_func->blocks.push_back(block);
    }

    //消解phi指令
    struct PendingMove {
        std::shared_ptr<LoongArch::Block> block;
        std::shared_ptr<ir::ir_reg> to;
        LoongArch::Reg from;
    };

    std::vector<PendingMove> Pending_moves;
    for(auto &bb : node.bbs){
        for(auto &inst : bb->instructions){
            if(auto *cur = dynamic_cast<ir::phi*>(inst.get())){
                for(auto &prev : cur->uses){
                    auto block_from = prev.second.lock();
                    assert(block_from);
                    auto b = cur_mapping->blockmapping[block_from];
                    std::shared_ptr<ir::ir_reg> use_reg = std::dynamic_pointer_cast<ir::ir_reg>(prev.first);
                    if(use_reg) {

                        // int spill_idx = spill_base;
                        // std::vector<std::shared_ptr<ir::ir_reg>> regs;
                        // regs.push_back(use_reg);
                        // regs.push_back(cur->dst);
                        // for(auto reg : regs) {
                        //     auto tar = Reg{spill_idx++};
                        //     auto it = std::find(cur_mapping->spill_vec.begin(), cur_mapping->spill_vec.end(), reg);
                        //     if(it != cur_mapping->spill_vec.end()) {
                        //         int idx = std::distance(cur_mapping->spill_vec.begin(), it);
                        //         int offset = cur_func->stack_size - (4 * idx);
                        //         cur_block->instructions.push_back(std::make_shared<LoongArch::ld>(tar, Reg{fp}, -offset, ld::ld_w));
                        //         cur_mapping->spill_mapping[reg->id] = tar;
                        //     }
                        // }

                        // is_dst = true;
                        // use_reg->accept(*this);
                        auto it = std::find(cur_mapping->spill_vec.begin(), cur_mapping->spill_vec.end(), use_reg->id);
                        if(it != cur_mapping->spill_vec.end()) {
                            Reg tar;
                            if(use_reg->get_type() == vartype::FLOAT/* || use_reg->get_type() == vartype::FLOATADDR*/) {
                                tar.type = FLOAT;
                            }
                            else {
                                tar.type = INT;
                            }
                            tar.ir_id = use_reg->id;
                            tar.is_arr = use_reg->is_arr;
                            if(is_dst) {
                                is_dst = false;
                                tar.id = spill_dst.id;
                                pass_reg = tar;
                            }
                            else {
                                if(flag) {
                                    tar.id = spill_use_1.id;
                                }
                                else {
                                    tar.id = spill_use_2.id;
                                }
                                flag = !flag;
                                // int idx = std::distance(cur_mapping->spill_vec.begin(), it);
                                // int offset = cur_func->stack_size - (4 * idx);
                                int offset = cur_func->stack_size/* - cur_mapping->call_mem*/ - cur_mapping->spill_offset.find(use_reg->id)->second;
                                b->insert_before_jump(std::make_shared<LoongArch::ld>(tar, Reg{fp}, -offset, tar.is_float() ? ld::fld_f : ld::ld_w));
                                pass_reg = tar;
                            }
                        }
                        else {
                            is_dst = false;
                            pass_reg = cur_mapping->transfer_reg(*use_reg);
                            pass_reg.ir_id = use_reg->id;
                        }
                        Reg temp = pass_reg;

                        // Reg temp = cur_mapping->transfer_reg(*use_reg.get());
                        // Reg mid = cur_mapping->new_reg();
                        // auto mid = const_reg_l;
                        // mid.type = temp.type;

                        is_dst = true;
                        it = std::find(cur_mapping->spill_vec.begin(), cur_mapping->spill_vec.end(), cur->dst->id);
                        if(it != cur_mapping->spill_vec.end()) {
                            Reg tar;
                            if(cur->dst->get_type() == vartype::FLOAT/* || cur->dst->get_type() == vartype::FLOATADDR*/) {
                                tar.type = FLOAT;
                            }
                            else {
                                tar.type = INT;
                            }
                            tar.ir_id = cur->dst->id;
                            tar.is_arr = cur->dst->is_arr;
                            if(is_dst) {
                                is_dst = false;
                                tar.id = spill_dst.id;
                                pass_reg = tar;
                            }
                            else {
                                if(flag) {
                                    tar.id = spill_use_1.id;
                                }
                                else {
                                    tar.id = spill_use_2.id;
                                }
                                flag = !flag;
                                // int idx = std::distance(cur_mapping->spill_vec.begin(), it);
                                // int offset = cur_func->stack_size - (4 * idx);
                                int offset = cur_func->stack_size/* - cur_mapping->call_mem*/ - cur_mapping->spill_offset.find(use_reg->id)->second;
                                b->insert_before_jump(std::make_shared<LoongArch::ld>(tar, Reg{fp}, -offset, tar.is_float() ? ld::fld_f : ld::ld_w));
                                pass_reg = tar;
                            }
                        }
                        else {
                            is_dst = false;
                            pass_reg = cur_mapping->transfer_reg(*cur->dst);
                            pass_reg.ir_id = cur->dst->id;
                        }
                        auto mid = pass_reg;
                        if(temp.is_float()) {
                            b->insert_before_jump(std::make_shared<LoongArch::mov>(mid, temp, mov::ftf_f));
                        }
                        else {
                            b->insert_before_jump(
                                std::make_shared<RegImmInst>(RegImmInst::addi_w,mid,temp,0)
                            );
                        }
                        Pending_moves.push_back({b,cur->dst,mid});

    it = std::find(cur_mapping->spill_vec.begin(), cur_mapping->spill_vec.end(), mid.ir_id);
    if(it != cur_mapping->spill_vec.end()) {
        int offset = cur_func->stack_size/* - cur_mapping->call_mem*/ - cur_mapping->spill_offset.find(mid.ir_id)->second;
        if(mid.is_arr) {
            b->insert_before_jump(std::make_shared<LoongArch::st>(mid, Reg{fp}, -offset, mid.is_float() ? st::fst_f : st::st_d));
        }
        else {
            b->insert_before_jump(std::make_shared<LoongArch::st>(mid, Reg{fp}, -offset, mid.is_float() ? st::fst_f : st::st_w));
        }
    }

                        // spill_idx = spill_base;
                        // for(auto reg :regs) {
                        //     auto tar = Reg{spill_idx++};
                        //     auto it = std::find(cur_mapping->spill_vec.begin(), cur_mapping->spill_vec.end(), reg);
                        //     if(it != cur_mapping->spill_vec.end()) {
                        //         int idx = std::distance(cur_mapping->spill_vec.begin(), it);
                        //         int offset = cur_func->stack_size - (4 * idx);
                        //         cur_block->instructions.push_back(std::make_shared<LoongArch::st>(tar, Reg{fp}, -offset, st::st_w));
                        //         auto del_it = cur_mapping->spill_mapping.find(reg->id);
                        //         if(del_it != cur_mapping->spill_mapping.end())
                        //             cur_mapping->spill_mapping.erase(del_it);
                        //     }
                        // }

                    }else{
                        std::shared_ptr<ir::ir_constant> use_constant = std::dynamic_pointer_cast<ir::ir_constant>(prev.first);
                        //直接把那个立即数放到相应的phi的目的寄存器里面就行了，在uses block的的jump的前一句
                        //这里直接使用LoongArch的加载立即数指令
                        using_reg = const_reg_r;
                        // auto value = use_constant->init_val.value();
                        // use_constant->accept(*this);
                        auto optional_val = use_constant->init_val;
                        if(use_constant->get_type() == vartype::FLOAT || use_constant->get_type() == vartype::FLOATADDR) {
                            using_reg.type = FLOAT;
                        }
                        else {
                            using_reg.type = INT;
                        }
                        if(optional_val.has_value()) {
                            auto variant_val = optional_val.value();
                            if(std::holds_alternative<int>(variant_val)) {
                                int val = std::get<int>(variant_val);
                                b->insert_before_jump(std::make_shared<LoongArch::LoadImm>(using_reg, val));
                            } else if(std::holds_alternative<float>(variant_val)) {
                                float val = std::get<float>(variant_val);
                                // Reg middle_reg{using_reg.id, FLOAT};
                                b->insert_before_jump(std::make_shared<LoongArch::LoadFloat>(using_reg, prog->float_nums.size()));
                                // b->insert_before_jump(std::make_shared<LoongArch::ld>(using_reg, middle_reg, 0, ld::fld_f));
                                // double double_num = static_cast<double>(val);
                                std::bitset<32> bits(*reinterpret_cast<unsigned int*>(&val));
                                std::stringstream ss;
                                ss << std::hex << std::setw(8) << std::setfill('0') << bits.to_ulong();
                                prog->float_nums.push_back("0x" + ss.str());
                            }
                        }
                        pass_reg = using_reg;
                        auto value = pass_reg;
                        // auto mid = cur_mapping->new_reg();
                        // auto mid = const_reg_l;

                        is_dst = true;
                        auto it = std::find(cur_mapping->spill_vec.begin(), cur_mapping->spill_vec.end(), cur->dst->id);
                        if(it != cur_mapping->spill_vec.end()) {
                            Reg tar;
                            if(cur->dst->get_type() == vartype::FLOAT/* || cur->dst->get_type() == vartype::FLOATADDR*/) {
                                tar.type = FLOAT;
                            }
                            else {
                                tar.type = INT;
                            }
                            tar.ir_id = cur->dst->id;
                            tar.is_arr = cur->dst->is_arr;
                            if(is_dst) {
                                is_dst = false;
                                tar.id = spill_dst.id;
                                pass_reg = tar;
                            }
                            else {
                                if(flag) {
                                    tar.id = spill_use_1.id;
                                }
                                else {
                                    tar.id = spill_use_2.id;
                                }
                                flag = !flag;
                                // int idx = std::distance(cur_mapping->spill_vec.begin(), it);
                                // int offset = cur_func->stack_size - (4 * idx);
                                int offset = cur_func->stack_size/* - cur_mapping->call_mem*/ - cur_mapping->spill_offset.find(use_reg->id)->second;
                                b->insert_before_jump(std::make_shared<LoongArch::ld>(tar, Reg{fp}, -offset, tar.is_float() ? ld::fld_f : ld::ld_w));
                                pass_reg = tar;
                            }
                        }
                        else {
                            is_dst = false;
                            pass_reg = cur_mapping->transfer_reg(*cur->dst);
                            pass_reg.ir_id = cur->dst->id;
                        }
                        auto mid = pass_reg;

                        mid.type = value.type;
                        // if(std::holds_alternative<int>(value)){
                        //     int value_num = std::get<int>(value);
                        //     b->insert_before_jump(
                        //         std::make_shared<LoongArch::LoadImm>(mid,value_num)
                        //     );
                        // }
                        if(value.is_float()) {
                            b->insert_before_jump(std::make_shared<LoongArch::mov>(mid, value, mov::ftf_f));
                        }
                        else {
                            b->insert_before_jump(std::make_shared<LoongArch::RegRegInst>(RegRegInst::add_w, mid, value, Reg{0}));
                        }

                        // int spill_idx = spill_base;
                        // std::vector<std::shared_ptr<ir::ir_reg>> regs;
                        // regs.push_back(cur->dst);
                        // for(auto reg : regs) {
                        //     auto tar = Reg{spill_idx++};
                        //     auto it = std::find(cur_mapping->spill_vec.begin(), cur_mapping->spill_vec.end(), reg);
                        //     if(it != cur_mapping->spill_vec.end()) {
                        //         int idx = std::distance(cur_mapping->spill_vec.begin(), it);
                        //         int offset = cur_func->stack_size - (4 * idx);
                        //         cur_block->instructions.push_back(std::make_shared<LoongArch::ld>(tar, Reg{fp}, -offset, ld::ld_w));
                        //         cur_mapping->spill_mapping[reg->id] = tar;
                        //     }
                        // }

                        Pending_moves.push_back({b,cur->dst,mid});


    it = std::find(cur_mapping->spill_vec.begin(), cur_mapping->spill_vec.end(), mid.ir_id);
    if(it != cur_mapping->spill_vec.end()) {
        int offset = cur_func->stack_size/* - cur_mapping->call_mem*/ - cur_mapping->spill_offset.find(mid.ir_id)->second;
        if(mid.is_arr) {
            b->insert_before_jump(std::make_shared<LoongArch::st>(mid, Reg{fp}, -offset, mid.is_float() ? st::fst_f : st::st_d));
        }
        else {
            b->insert_before_jump(std::make_shared<LoongArch::st>(mid, Reg{fp}, -offset, mid.is_float() ? st::fst_f : st::st_w));
        }
    }


                        // spill_idx = spill_base;
                        // for(auto reg :regs) {
                        //     auto tar = Reg{spill_idx++};
                        //     auto it = std::find(cur_mapping->spill_vec.begin(), cur_mapping->spill_vec.end(), reg);
                        //     if(it != cur_mapping->spill_vec.end()) {
                        //         int idx = std::distance(cur_mapping->spill_vec.begin(), it);
                        //         int offset = cur_func->stack_size - (4 * idx);
                        //         cur_block->instructions.push_back(std::make_shared<LoongArch::st>(tar, Reg{fp}, -offset, st::st_w));
                        //         auto del_it = cur_mapping->spill_mapping.find(reg->id);
                        //         if(del_it != cur_mapping->spill_mapping.end())
                        //             cur_mapping->spill_mapping.erase(del_it);
                        //     }
                        // }
                    }

                }
            }
        }
    }
    for(auto &i : Pending_moves){
        // // int spill_idx = spill_base;
        // // std::vector<std::shared_ptr<ir::ir_reg>> regs;
        // // regs.push_back(i.to);
        // // for(auto reg : regs) {
        // //     auto tar = Reg{spill_idx++};
        // //     auto it = std::find(cur_mapping->spill_vec.begin(), cur_mapping->spill_vec.end(), reg);
        // //     if(it != cur_mapping->spill_vec.end()) {
        // //         int idx = std::distance(cur_mapping->spill_vec.begin(), it);
        // //         int offset = cur_func->stack_size - (4 * idx);
        // //         i.block->insert_before_jump(std::make_shared<LoongArch::ld>(tar, Reg{fp}, -offset, ld::ld_w));
        // //         cur_mapping->spill_mapping[reg->id] = tar;
        // //     }
        // // }
        // is_dst = true;
        // // i.to->accept(*this);
        // auto it = std::find(cur_mapping->spill_vec.begin(), cur_mapping->spill_vec.end(), i.to->id);
        // if(it != cur_mapping->spill_vec.end()) {
        //     Reg tar;
        //     if(i.to->get_type() == vartype::FLOAT/* || i.to->get_type() == vartype::FLOATADDR*/) {
        //         tar.type = FLOAT;
        //     }
        //     else {
        //         tar.type = INT;
        //     }
        //     tar.ir_id = i.to->id;
        //     tar.is_arr = i.to->is_arr;
        //     if(is_dst) {
        //         is_dst = false;
        //         tar.id = spill_dst.id;
        //         pass_reg = tar;
        //         // return;
        //     }
        //     else {
        //         if(flag) {
        //             tar.id = spill_use_1.id;
        //         }
        //         else {
        //             tar.id = spill_use_2.id;
        //         }
        //         flag = !flag;
        //         // int idx = std::distance(cur_mapping->spill_vec.begin(), it);
        //         // int offset = cur_func->stack_size - (4 * idx);
        //         int offset = cur_func->stack_size/* - cur_mapping->call_mem*/ - cur_mapping->spill_offset.find(i.to->id)->second;
        //         i.block->insert_before_jump(std::make_shared<LoongArch::ld>(tar, Reg{fp}, -offset, tar.is_float() ? ld::fld_f : ld::ld_w));
        //         pass_reg = tar;
        //     }
        // }
        // else {
        //     is_dst = false;
        //     pass_reg = cur_mapping->transfer_reg(*i.to);
        // }
        // auto to = pass_reg;
        // // i.block->insert_before_jump(std::make_shared<RegImmInst>(RegImmInst::addi_w, cur_mapping->transfer_reg(*i.to.get()), i.from, 0)); //插入一条move指令
        // if(to.is_float()) {
        //     i.block->insert_before_jump(std::make_shared<LoongArch::mov>(to, i.from, mov::ftf_f));
        // }
        // else {
        //     i.block->insert_before_jump(std::make_shared<RegImmInst>(RegImmInst::addi_w, to, i.from, 0)); //插入一条move指令
        // }
        // // check_write_back(to);
        // it = std::find(cur_mapping->spill_vec.begin(), cur_mapping->spill_vec.end(), to.ir_id);
        // if(it != cur_mapping->spill_vec.end()) {
        //     // int idx = std::distance(cur_mapping->spill_vec.begin(), it);
        //     // int offset = cur_func->stack_size - (4 * idx);
        //     int offset = cur_func->stack_size/* - cur_mapping->call_mem*/ - cur_mapping->spill_offset.find(to.ir_id)->second;
        //     if(to.is_arr) {
        //         i.block->insert_before_jump(std::make_shared<LoongArch::st>(to, Reg{fp}, -offset, to.is_float() ? st::fst_f : st::st_d));
        //     }
        //     else {
        //         i.block->insert_before_jump(std::make_shared<LoongArch::st>(to, Reg{fp}, -offset, to.is_float() ? st::fst_f : st::st_w));
        //     }
        // }
        // cur_func->regn = cur_mapping->regn;

        // // spill_idx = spill_base;
        // // for(auto reg :regs) {
        // //     reg->accept(*this);
        // //     auto tar = Reg{spill_idx++};
        // //     auto it = std::find(cur_mapping->spill_vec.begin(), cur_mapping->spill_vec.end(), reg);
        // //     if(it != cur_mapping->spill_vec.end()) {
        // //         int idx = std::distance(cur_mapping->spill_vec.begin(), it);
        // //         int offset = cur_func->stack_size - (4 * idx);
        // //         i.block->insert_before_jump(std::make_shared<LoongArch::st>(tar, Reg{fp}, -offset, st::st_w));
        // //         auto del_it = cur_mapping->spill_mapping.find(reg->id);
        // //         if(del_it != cur_mapping->spill_mapping.end())
        // //             cur_mapping->spill_mapping.erase(del_it);
        // //     }
        // // }
    }
}

void LoongArch::ProgramBuilder::visit(ir::store &node) {
    // is_dst = true;
    // node.addr->accept(*this);
    // Reg reg = pass_reg;
    using_reg = const_reg_l;
    node.value->accept(*this);
    Reg dst = pass_reg;
    st::Type type = st::st_w;
    stptr::Type sttype = stptr::st_w;
    if(dst.is_float()) {
        type = st::fst_f;
        sttype = stptr::fst_f;
    }
    auto val_is_reg = std::dynamic_pointer_cast<ir::ir_reg>(node.value);
    if(val_is_reg && val_is_reg->check_is_arr()) {
        type = st::st_d;
        sttype = stptr::st_d;
    }
    // cur_block->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::add_w, reg, Reg{0}, dst));
    if(node.addr->is_arr) {
        node.addr->accept(*this);
        Reg reg = pass_reg;
        cur_block->instructions.push_back(std::make_shared<st>(dst, reg, 0, type));
    }
    else if(node.addr->is_global) {
        // node.addr->accept(*this);
        // Reg reg = const_reg_r;
        // cur_block->instructions.push_back(std::make_shared<la>(node.addr, reg));
        // cur_block->instructions.push_back(std::make_shared<stptr>(dst, reg, 0, sttype));
        cur_block->instructions.push_back(std::make_shared<stglob>(node.addr, dst));
    }
    else {
        int offset = cur_mapping->mem_var.find(node.addr->id)->second;
        offset = cur_func->stack_size/* - cur_mapping->call_mem*/ - offset;
        cur_block->instructions.push_back(std::make_shared<st>(dst, Reg{fp}, -offset, type));
    }
    // check_write_back(reg);
    // // auto reg_id = std::dynamic_pointer_cast<ir::ir_reg>(node.addr)->id;
    // auto ir_r = std::dynamic_pointer_cast<ir::ir_reg>(node.addr);
    // // auto reg = this->cur_mapping->reg_mapping[reg_id];
    // auto reg = this->cur_mapping->transfer_reg(*ir_r.get());
    // // auto offset = node.value;
    // auto constant = std::dynamic_pointer_cast<ir::ir_constant>(node.value);
    // if(constant != nullptr) {
    //     auto optional_val = constant->init_val;
    //     if(optional_val.has_value()) {
    //         auto variant_val = optional_val.value();
    //         if(std::holds_alternative<int>(variant_val)) {
    //             int val = std::get<int>(variant_val);
    //             cur_block->instructions.push_back(std::make_shared<LoongArch::RegImmInst>(RegImmInst::addi_w, reg, Reg{0}, val));
    //         } else if(std::holds_alternative<float>(variant_val)) {
    //             float val = std::get<float>(variant_val);
    //             cur_block->instructions.push_back(std::make_shared<LoongArch::RegImmInst>(RegImmInst::addi_w, reg, Reg{0}, val));
    //         }
    //         // cur_block->instructions.push_back(std::make_shared<LoongArch::st>(Reg{reg}, Reg{fp}, this->cur_mapping->objoffset_mapping, st::st_w));
    //     }
    //     // cur_block->instructions.push_back(std::make_shared<LoongArch::RegImmInst>(RegImmInst::addi_w, Reg{reg}, Reg{0}, value));
    // }
    // else {
    //     auto val_r = std::dynamic_pointer_cast<ir::ir_reg>(node.value);
    //     auto val = cur_mapping->transfer_reg(*val_r.get());
    //     cur_block->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::add_w, reg, Reg{0}, val));
    // }
}

void LoongArch::ProgramBuilder::visit(ir::jump &node) {
    auto tar_b = node.get_target();
    auto target = cur_mapping->blockmapping[tar_b];
    // target->insert_before_jump(std::make_shared<ir::load>(Args &&args...))
    cur_block->instructions.push_back(std::make_shared<LoongArch::Jump>(target.get()));
}

void LoongArch::ProgramBuilder::visit(ir::br &node) {
    node.get_cond()->accept(*this);
    Reg cond = pass_reg;
    // if(cond.type == FBOOL) {
    //     auto target_true = cur_mapping->blockmapping[node.get_target_true()];
    //     auto target_false = cur_mapping->blockmapping[node.get_target_false()];
    //     cur_block->instructions.push_back(std::make_shared<LoongArch::Br>(Br::bcnez, cond, target_true.get()));
    //     cur_block->instructions.push_back(std::make_shared<LoongArch::Jump>(target_false.get()));
    // }
    // else {
        auto target_true = cur_mapping->blockmapping[node.get_target_true()];
        auto target_false = cur_mapping->blockmapping[node.get_target_false()];
        cur_block->instructions.push_back(std::make_shared<LoongArch::Br>(Br::bnez, cond, target_true.get()));
        cur_block->instructions.push_back(std::make_shared<LoongArch::Jump>(target_false.get()));
    // }
}

void LoongArch::ProgramBuilder::visit(ir::ret &node) {
    node.value->accept(*this);
    Reg backend_reg = pass_reg;
    if(backend_reg.is_float()) {
        cur_block->instructions.push_back(std::make_shared<LoongArch::mov>(Reg{10, FLOAT}, backend_reg, mov::ftf_f));
    }
    else {
        cur_block->instructions.push_back(
            std::make_shared<LoongArch::RegRegInst>(RegRegInst::orw,Reg{10},backend_reg,Reg{0})
        );
    }
    cur_block->instructions.push_back(
        std::make_shared<LoongArch::ld>(Reg{fp},Reg{sp},this->cur_func->stack_size - 16,ld::ld_d)
    );
    cur_block->instructions.push_back(std::make_shared<LoongArch::ld>(Reg{ra}, Reg{sp}, cur_func->stack_size - 8, ld::ld_d));
    // if((uint32_t)cur_func->stack_size >> 12) {
    //     cur_block->instructions.push_back(std::make_shared<LoadImm>(const_reg_l, cur_func->stack_size));
    //     cur_block->instructions.push_back(std::make_shared<RegRegInst>(RegRegInst::add_d, Reg{sp}, Reg{sp}, const_reg_l));
    // }
    // else
    cur_block->instructions.push_back(
        std::make_shared<LoongArch::RegImmInst>(RegImmInst::addi_d,Reg{sp},Reg{sp},cur_func->stack_size)
    );
    cur_block->instructions.push_back(std::make_shared<LoongArch::jr>(true));
}

void LoongArch::ProgramBuilder::visit(ir::load &node) {
    // node.addr->accept(*this);
    // Reg src = pass_reg;
    is_dst = true;
    node.dst->accept(*this);
    Reg dst = pass_reg;
    ld::Type type = ld::ld_w;
    ldptr::Type ldtype = ldptr::ld_w;
    if(dst.is_float()) {
        // type = ld::fld_f;
        ldtype = ldptr::fld_f;
    }
    if(node.dst->check_is_arr()) {
        ldtype = ldptr::ld_d;
    }
    // cur_block->instructions.push_back(std::make_shared<LoongArch::RegImmInst>(RegImmInst::addi_w, dst, src, 0));
    if(node.addr->is_arr) {
        node.addr->accept(*this);
        Reg reg = pass_reg;
        cur_block->instructions.push_back(std::make_shared<ldptr>(dst, reg, 0, ldtype));
    }
    else if(node.addr->is_global) {
        // node.addr->accept(*this);
        // Reg reg = const_reg_r;
        // cur_block->instructions.push_back(std::make_shared<la>(node.addr, reg));
        // cur_block->instructions.push_back(std::make_shared<ldptr>(dst, reg, 0, ldtype));
        cur_block->instructions.push_back(std::make_shared<ldglob>(node.addr, dst));
    }
    else {
        int offset = cur_mapping->mem_var.find(node.addr->id)->second;
        offset = cur_func->stack_size/* - cur_mapping->call_mem*/ - offset;
        cur_block->instructions.push_back(std::make_shared<ldptr>(dst, Reg{fp}, -offset, ldtype));
    }
    check_write_back(dst);
}

void LoongArch::ProgramBuilder::visit(ir::alloc &node) {
    // is_dst = true;
    // node.var->addr->accept(*this);
    // auto dst = pass_reg;
    // int offset = cur_mapping->mem_var.find(node.var->addr->id)->second;
    // cur_block->instructions.push_back(std::make_shared<RegImmInst>(RegImmInst::addi_w, dst, Reg{0}, offset));
    // check_write_back(dst);
    
// auto regs = node.def_reg();
// for(auto reg : regs) {
        // auto id = std::dynamic_pointer_cast<ir::ir_reg>(reg)->id;
        // cur_mapping->reg_mapping[id] = cur_mapping->new_reg();
// auto ir_r = std::dynamic_pointer_cast<ir::ir_reg>(reg);
        // cur_mapping->transfer_reg(*ir_r.get());              /*有映射就不用啦*/
// }
    // auto reg = this->cur_mapping->regn;
    // auto value = this->cur_mapping.
    // cur_block->instructions.push_back(std::make_shared<LoongArch::RegImmInst>(RegImmInst::addi_w, Reg{reg}, Reg{0},));
}

void LoongArch::ProgramBuilder::visit(ir::phi &node) {                  // 上方已经消解了phi指令
    // auto src = node.use_reg();
    // auto dst_r = node.dst;
    // auto dst = cur_mapping->transfer_reg(*dst_r.get());
    // for(auto item_v : src) {
    //     auto item = cur_mapping->transfer_reg(*(std::dynamic_pointer_cast<ir::ir_reg>(item_v)).get());
    //     // cur_block->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::sub_w, dst, item, dst));
    // }
}

void LoongArch::ProgramBuilder::fb2gr(Reg src, Reg dst) {
    auto set_true = std::make_shared<LoongArch::Block>("set_true" + std::to_string(set_cnt));
    auto set_false = std::make_shared<LoongArch::Block>("set_false" + std::to_string(set_cnt));
    auto nxt = std::make_shared<LoongArch::Block>("after_set" + std::to_string(set_cnt));
    set_cnt++;
    cur_block->instructions.push_back(std::make_shared<LoongArch::Br>(Br::bceqz, src, set_true.get()));
    cur_block->instructions.push_back(std::make_shared<LoongArch::Jump>(set_false.get()));
    set_true->instructions.push_back(std::make_shared<LoongArch::LoadImm>(dst, 1));
    set_true->instructions.push_back(std::make_shared<LoongArch::Jump>(nxt.get()));
    set_false->instructions.push_back(std::make_shared<LoongArch::LoadImm>(dst, 0));
    set_false->instructions.push_back(std::make_shared<LoongArch::Jump>(nxt.get()));
    cur_block = nxt;
    cur_func->waiting_blocks.push_back(set_true);
    cur_func->waiting_blocks.push_back(set_false);
    cur_func->waiting_blocks.push_back(nxt);
}

void LoongArch::ProgramBuilder::visit(ir::unary_op_ins &node) {
    is_dst = true;
    node.get_dst()->accept(*this);
    Reg dst = pass_reg;
    using_reg = const_reg_l;
    node.get_src()->accept(*this);
    Reg src = pass_reg;                                                                                            // Reg：物理寄存器
    if(node.op == unaryop::minus) {                                                                     // 当op为“-”（取反指令）
        if(src.is_float()) {
            cur_block->instructions.push_back(std::make_shared<LoongArch::funary>(dst, src, funary::neg_f));
        }
        else {
            auto negative = const_reg_r;
            cur_block->instructions.push_back(std::make_shared<LoongArch::LoadImm>(negative, -1));
            cur_block->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::mul_w, dst, src, negative));
        }
    }
    else if(node.op == unaryop::plus) {                                                                 // 当op为“+”（符号不变）
        if(src.is_float()) {
            cur_block->instructions.push_back(std::make_shared<LoongArch::mov>(dst, src, mov::ftf_f));
        }
        else {
            cur_block->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::add_w, dst, Reg{0}, src));
        }
    }
    else if(node.op == unaryop::op_not) {                                                               // 当op为布尔非
        // if(src.type == FBOOL) {
        //     fb2gr(src, dst);
        // }
        // else {
            cur_block->instructions.push_back(std::make_shared<LoongArch::RegImmInst>(RegImmInst::xori, dst, src, true)); // （xori是我自己加的）
        // }
    }
    check_write_back(dst);
}

void LoongArch::ProgramBuilder::visit(ir::binary_op_ins &node) {
    std::unordered_map<binop, RegRegInst::Type> map = {
        {binop::plus, RegRegInst::add_w}, 
        {binop::minus, RegRegInst::sub_w}, 
        {binop::multiply, RegRegInst::mul_w},
        {binop::divide, RegRegInst::div_w},
        {binop::modulo, RegRegInst::mod_w}
    };
    std::unordered_map<binop, RegRegInst::Type> fmap = {
        {binop::plus, RegRegInst::fadd_f}, 
        {binop::minus, RegRegInst::fsub_f}, 
        {binop::multiply, RegRegInst::fmul_f},
        {binop::divide, RegRegInst::fdiv_f}
    };
    // auto type = map[node.op];
    auto tar_r = std::dynamic_pointer_cast<ir::ir_reg>(node.dst);
    is_dst = true;
    node.dst->accept(*this);
    Reg tar = pass_reg;
    RegRegInst::Type type;
    if(tar.is_float()) {
        type = fmap[node.op];
    }
    else {
        type = map[node.op];
    }
    using_reg = const_reg_l;
    node.src1->accept(*this);
    Reg exp1 = pass_reg;
    using_reg = const_reg_r;
    node.src2->accept(*this);
    Reg exp2 = pass_reg;
    cur_block->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(type, tar, exp1, exp2));
    check_write_back(tar);
}

void LoongArch::ProgramBuilder::visit(ir::cmp_ins &node) {
    is_dst = true;
    node.get_dst()->accept(*this);
    Reg dst = pass_reg;
    auto exps = node.get_src();
    using_reg = const_reg_l;
    exps[0]->accept(*this);
    Reg exp1 = pass_reg;
    using_reg = const_reg_r;
    exps[1]->accept(*this);
    Reg exp2 = pass_reg;
    // if(node.dst->get_type() == vartype::FBOOL) {
    //     // if(node.op == relop::equal || node.op == relop::non_equal)
    //     std::unordered_map<relop, FCMP::Type> map_to_f = {
    //         {relop::equal, FCMP::eq},
    //         {relop::non_equal, FCMP::ne},
    //         {relop::greater, FCMP::gt},
    //         {relop::greater_equal, FCMP::ge},
    //         {relop::less, FCMP::lt},
    //         {relop::less_equal, FCMP::le},
    //     };
    //     cur_block->instructions.push_back(std::make_shared<LoongArch::FCMP>(dst, exp1, exp2, map_to_f[node.op]));
    // }
    // else {
        // if(node.op == relop::equal || node.op == relop::non_equal) {            // TODO: 改成riscv中的现有指令(eq, ne, gt, ge, lt, le)
        //     cur_block->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::sub_w, dst, exp1, exp2));
        //     cur_block->instructions.push_back(std::make_shared<LoongArch::RegImmInst>(RegImmInst::sltui, dst, dst, 1));
        //     if(node.op == relop::non_equal) {
        //         cur_block->instructions.push_back(std::make_shared<LoongArch::RegImmInst>(RegImmInst::xori, dst, dst, 1));
        //     }
        // }
        // else if (node.op == relop::greater) {
        //     cur_block->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::slt, dst, exp2, exp1));
        // }
        // else if (node.op == relop::greater_equal) {
        //     cur_block->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::slt, dst, exp1, exp2));
        //     cur_block->instructions.push_back(std::make_shared<LoongArch::RegImmInst>(RegImmInst::xori, dst, dst, 1));
        // }
        // else if (node.op == relop::less) {
        //     cur_block->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::slt, dst, exp1, exp2));
        // }
        // else if (node.op == relop::less_equal) {
        //     cur_block->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::slt, dst, exp2, exp1));
        //     cur_block->instructions.push_back(std::make_shared<LoongArch::RegImmInst>(RegImmInst::xori, dst, dst, 1));
        // }
        if(node.src1->get_type() == vartype::FLOAT) {
            std::unordered_map<relop, FCMP::Type> map_to_f = {
                {relop::equal, FCMP::eq},
                {relop::non_equal, FCMP::ne},
                {relop::greater, FCMP::gt},
                {relop::greater_equal, FCMP::ge},
                {relop::less, FCMP::lt},
                {relop::less_equal, FCMP::le},
            };
            cur_block->instructions.push_back(std::make_shared<LoongArch::FCMP>(dst, exp1, exp2, map_to_f[node.op]));
        }
        else {
            std::unordered_map<relop, CMP::Type> map = {
                {relop::equal, CMP::eq},
                {relop::non_equal, CMP::ne},
                {relop::greater, CMP::gt},
                {relop::greater_equal, CMP::ge},
                {relop::less, CMP::lt},
                {relop::less_equal, CMP::le},
            };
            cur_block->instructions.push_back(std::make_shared<LoongArch::CMP>(dst, exp1, exp2, map[node.op]));
        }
    // }
    check_write_back(dst);
}

void LoongArch::ProgramBuilder::visit(ir::logic_ins &node) {
    // auto dst = cur_mapping->transfer_reg(*std::dynamic_pointer_cast<ir::ir_reg>(node.def_reg()[0]).get());
    is_dst = true;
    node.get_dst()->accept(*this);
    auto dst = pass_reg;
    auto srcs = node.get_src();
    using_reg = const_reg_l;
    srcs[0]->accept(*this);
    Reg src1 = pass_reg;
    using_reg = const_reg_r;
    srcs[1]->accept(*this);
    Reg src2 = pass_reg;
    if(node.op == relop::op_and) {
        cur_block->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::andw, dst, src1, src2));
    } else if(node.op == relop::op_or) {
        cur_block->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::orw, dst, src1, src2));
    }
    check_write_back(dst);
}

void LoongArch::ProgramBuilder::visit(ir::get_element_ptr& node) {
    auto dimensions = node.obj_offset;
    is_dst = true;
    int base_dim_cnt = node.base_dimension->dimensions.size() + (node.base_dimension->has_first_dim ? 0 : 1);
    node.dst->accept(*this);
    auto dst = pass_reg;
    if(node.base_reg->is_global) {
        cur_block->instructions.push_back(std::make_shared<la>(node.base_reg, dst, la::local));
    }
    else if(node.base_dimension->has_first_dim) {
        int offset = cur_mapping->mem_var.find(node.base_reg->id)->second/* + cur_mapping->call_mem*/ - cur_func->stack_size;
        // if((uint32_t)offset >> 12) {
        //     cur_block->instructions.push_back(std::make_shared<LoadImm>(const_reg_l, offset));
        //     cur_block->instructions.push_back(std::make_shared<RegRegInst>(RegRegInst::add_d, dst, Reg{fp}, const_reg_l));
        // }
        // else
        cur_block->instructions.push_back(std::make_shared<RegImmInst>(RegImmInst::addi_d, dst, Reg{fp}, offset));
    }
    else {
        assert(!node.base_reg->check_local());
        node.base_reg->accept(*this);
        auto base_add = pass_reg;
        cur_block->instructions.push_back(std::make_shared<RegRegInst>(RegRegInst::add_d, dst, base_add, Reg{0}));
    }
    for(int i = 0; (i < dimensions.size() - 1) && !dimensions.empty(); i++) {
        using_reg = const_reg_l;
        // auto ini_r = std::make_shared<ir::ir_constant>(1);
        // ini_r->accept(*this);
        auto is_spilled = node.spilled_obj.find(dimensions[i]);
        Reg ini;
        if(is_spilled != node.spilled_obj.end()) {
            ini = using_reg;
            auto ir_reg = is_spilled->second->addr;
            if(ir_reg->type == vartype::FLOAT) {
                ini.type = FLOAT;
            }
            else {
                ini.type = INT;
            }
            int offset = cur_mapping->mem_var.find(ir_reg->id)->second;
            offset = cur_func->stack_size - offset;
            cur_block->instructions.push_back(std::make_shared<LoongArch::ld>(ini, Reg{fp}, -offset, ld::ld_w));
        }
        else {
            dimensions[i]->accept(*this);
            ini = pass_reg;
        }
        if(ini.id != using_reg.id) {
            cur_block->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::add_d, using_reg, ini, Reg{0}));
            ini = using_reg;
        }
        int total_cnt = 1;
        for(int j = i + 1; j < base_dim_cnt; j++) {
            total_cnt *= node.base_dimension->dimensions[node.base_dimension->has_first_dim ? j : j - 1]->calc_res();
        }
        auto load_cnt = std::make_shared<ir::ir_constant>(total_cnt);
        load_cnt->type = vartype::INT;
        using_reg = const_reg_r;
        load_cnt->accept(*this);
        Reg cnt = pass_reg;
        cur_block->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::mul_d, ini, ini, cnt));
        cur_block->instructions.push_back(std::make_shared<LoongArch::RegImmInst>(RegImmInst::slli_d, ini, ini, 2));
        cur_block->instructions.push_back(std::make_shared<RegRegInst>(RegRegInst::add_d, dst, dst, ini));
    }
    if(!dimensions.empty()) {
        using_reg = const_reg_l;
        auto is_spilled = node.spilled_obj.find(dimensions.back());
        Reg ini;
        if(is_spilled != node.spilled_obj.end()) {
            ini = using_reg;
            auto ir_reg = is_spilled->second->addr;
            if(ir_reg->type == vartype::FLOAT) {
                ini.type = FLOAT;
            }
            else {
                ini.type = INT;
            }
            int offset = cur_mapping->mem_var.find(ir_reg->id)->second;
            offset = cur_func->stack_size - offset;
            cur_block->instructions.push_back(std::make_shared<LoongArch::ld>(ini, Reg{fp}, -offset, ld::ld_w));
        }
        else {
            dimensions.back()->accept(*this);
            ini = pass_reg;
        }
        if(ini.id != using_reg.id) {
            cur_block->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::add_d, using_reg, ini, Reg{0}));
            ini = using_reg;
        }
        if(base_dim_cnt > dimensions.size()) {
            int total_cnt = 1;
            for(int j = dimensions.size(); j < base_dim_cnt; j++) {
                total_cnt *= node.base_dimension->dimensions[node.base_dimension->has_first_dim ? j : j - 1]->calc_res();
            }
            auto load_cnt = std::make_shared<ir::ir_constant>(total_cnt);
            load_cnt->type = vartype::INT;
            using_reg = const_reg_r;
            load_cnt->accept(*this);
            Reg cnt = pass_reg;
            cur_block->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::mul_d, ini, ini, cnt));
        }
        cur_block->instructions.push_back(std::make_shared<LoongArch::RegImmInst>(RegImmInst::slli_d, ini, ini, 2));
        cur_block->instructions.push_back(std::make_shared<RegRegInst>(RegRegInst::add_d, dst, dst, ini));
    }
    check_write_back(dst);
    node.dst->is_arr = true;
    node.dst->size = 8;
    // pass_reg = dst;
}

void LoongArch::ProgramBuilder::visit(ir::while_loop& node) {
    auto tar_b = node.get_cond_from();
    auto target = cur_mapping->blockmapping[tar_b];
    cur_block->instructions.push_back(std::make_shared<LoongArch::Jump>(target.get()));
}

void LoongArch::ProgramBuilder::visit(ir::break_or_continue& node) {
    auto tar_b = node.get_target();
    auto target = cur_mapping->blockmapping[tar_b];
    cur_block->instructions.push_back(std::make_shared<LoongArch::Jump>(target.get()));
}

void LoongArch::ProgramBuilder::visit(ir::func_call& node) {
    // 保存自己的寄存器
    auto raw = node.get_live_in();
    std::set<Reg> live_in;
    for(auto ir_reg : raw) {
        auto reg = cur_mapping->transfer_reg(*ir_reg.get());
        live_in.insert(reg);
    }
    if(live_in.size()) {
        cur_block->instructions.push_back(std::make_shared<LoongArch::RegImmInst>(RegImmInst::addi_d, Reg{sp}, Reg{sp}, -(caller_save_regs.size() * 8)));
        for(int i = 0; i < caller_save_regs.size(); i++) {
            if(live_in.find(caller_save_regs[i]) != live_in.end()) {
                cur_block->instructions.push_back(std::make_shared<LoongArch::st>(caller_save_regs[i], Reg{sp}, 8 * i, caller_save_regs[i].is_float()? st::fst_f : st::st_d));
            }
        }
    }

    int i_pointer = 10;
    int f_pointer = 10;
    int cur_stk = 0;
    if(cur_mapping->call_mem)
        cur_block->instructions.push_back(std::make_shared<LoongArch::RegImmInst>(RegImmInst::addi_d, Reg{sp}, Reg{sp}, -cur_mapping->call_mem));
    for(auto par : node.params) {
        auto is_spilled = node.spilled_obj.find(par);
        Reg reg;
        if(is_spilled != node.spilled_obj.end()) {
            auto ir_reg = is_spilled->second->addr;
            auto par_reg = std::dynamic_pointer_cast<ir::ir_reg>(par);
            assert(par_reg);
            reg = const_reg_l;
            if(par_reg->type == vartype::FLOAT) {
                reg.type = FLOAT;
            }
            else {
                reg.type = INT;
            }
            int offset = cur_mapping->mem_var.find(ir_reg->id)->second;
            offset = cur_func->stack_size - offset;
            if(par_reg->is_arr) {
                cur_block->instructions.push_back(std::make_shared<LoongArch::ld>(reg, Reg{fp}, -offset, ld::ld_d));
            }
            else {
                cur_block->instructions.push_back(std::make_shared<LoongArch::ld>(reg, Reg{fp}, -offset, reg.is_float() ? ld::fld_f : ld::ld_w));
            }
        }
        else {
            par->accept(*this);
            reg = pass_reg;   
        }
        if(reg.is_float()) {
            if(f_pointer < 18) {
                cur_block->instructions.push_back(std::make_shared<LoongArch::mov>(Reg{f_pointer++, FLOAT}, reg, mov::ftf_f));
            }
            else {
                if(i_pointer < 18) {
                    cur_block->instructions.push_back(std::make_shared<LoongArch::mov>(Reg{i_pointer++}, reg, mov::ftg));
                }
                else {
                    auto par_reg = std::dynamic_pointer_cast<ir::ir_reg>(par);
                    if(par_reg && par_reg->is_arr) {
                        cur_block->instructions.push_back(std::make_shared<st>(reg, Reg{sp}, cur_stk, st::fst_d));
                    }
                    else {
                        cur_block->instructions.push_back(std::make_shared<stptr>(reg, Reg{sp}, cur_stk, stptr::fst_d));
                    }
                    cur_stk += 8;
                }
            }
        }
        else {
            if(i_pointer < 18) {
                if(reg.id >= 10 && reg.id <= 17/* && reg.type != FBOOL*/) {
                    auto it = std::find(caller_save_regs.begin(), caller_save_regs.end(), reg);
                    int offset = std::distance(caller_save_regs.begin(), it) * 8 + cur_mapping->call_mem;
                    cur_block->instructions.push_back(std::make_shared<ld>(Reg{i_pointer++}, Reg{sp}, offset, ld::ld_d));
                }
                else
                    cur_block->instructions.push_back(std::make_shared<RegRegInst>(RegRegInst::add_d, Reg{i_pointer++}, reg, Reg{0}));
            }
            else {
                // cur_block->instructions.push_back(std::make_shared<stptr>(reg, Reg{sp}, 8 * (i_pointer++) - 12, stptr::st_d));
                if(reg.id >= 10 && reg.id <= 17/* && reg.type != FBOOL*/) {
                    auto it = std::find(caller_save_regs.begin(), caller_save_regs.end(), reg);
                    int offset = std::distance(caller_save_regs.begin(), it) * 8 + cur_mapping->call_mem;
                    cur_block->instructions.push_back(std::make_shared<ld>(const_reg_r, Reg{sp}, offset, ld::ld_d));
                    const_reg_r.type = reg.type;
                    reg = const_reg_r;
                }
                auto par_reg = std::dynamic_pointer_cast<ir::ir_reg>(par);
                if(par_reg && par_reg->is_arr) {
                    cur_block->instructions.push_back(std::make_shared<st>(reg, Reg{sp}, cur_stk, st::st_d));
                }
                else {
                    cur_block->instructions.push_back(std::make_shared<stptr>(reg, Reg{sp}, cur_stk, stptr::st_d));
                }
                cur_stk += 8;
            }
        }
    }
    cur_block->instructions.push_back(std::make_shared<LoongArch::Bl>(node.func_name));

    if(cur_mapping->call_mem)
        cur_block->instructions.push_back(std::make_shared<LoongArch::RegImmInst>(RegImmInst::addi_d, Reg{sp}, Reg{sp}, cur_mapping->call_mem));
    if(live_in.size()) {
        for(int i = 0; i < caller_save_regs.size(); i++) {
            if(live_in.find(caller_save_regs[i]) != live_in.end()) {
                cur_block->instructions.push_back(std::make_shared<LoongArch::ld>(caller_save_regs[i], Reg{sp}, 8 * i, caller_save_regs[i].is_float()? ld::fld_f : ld::ld_d));
            }
        }
        cur_block->instructions.push_back(std::make_shared<LoongArch::RegImmInst>(RegImmInst::addi_d, Reg{sp}, Reg{sp}, caller_save_regs.size() * 8));
    }

    if(node.ret_reg) {
        is_dst = true;
        node.ret_reg->accept(*this);
        auto dst = pass_reg;
        if(dst.is_float()) {
            cur_block->instructions.push_back(std::make_shared<LoongArch::mov>(dst, Reg{10, Rtype::FLOAT}, mov::ftf_f));
        }
        else {
            cur_block->instructions.push_back(std::make_shared<RegRegInst>(RegRegInst::add_d, dst, Reg{10}, Reg{0}));
        }
        check_write_back(dst);
    }
}

void LoongArch::ProgramBuilder::visit(ir::global_def& node) {

}

void LoongArch::ProgramBuilder::visit(ir::trans& node) {
    node.src->accept(*this);
    Reg src = pass_reg;
    is_dst = true;
    node.dst->accept(*this);
    Reg dst = pass_reg;
    if((node.src->get_type() == vartype::BOOL/* || node.src->get_type() == vartype::FBOOL*/) && node.target == vartype::INT) {
        // if(node.src->get_type() == vartype::FBOOL) {
        //     fb2gr(src, dst);
        // }
        // else {
            cur_block->instructions.push_back(std::make_shared<LoongArch::RegRegInst>(RegRegInst::add_w, dst, Reg{0}, src));
        // }
    }
    else if(node.target == vartype::FLOAT) {
        // cur_block->instructions.push_back(std::make_shared<LoongArch::mov>(dst, src, mov::gtf));
        cur_block->instructions.push_back(std::make_shared<LoongArch::trans>(dst, src, trans::itf));
    }
    else {
        // auto bak = const_reg_l;
        // const_reg_l.type = FLOAT;
        // cur_block->instructions.push_back(std::make_shared<LoongArch::trans>(const_reg_l, src, trans::fti));
        cur_block->instructions.push_back(std::make_shared<LoongArch::trans>(dst, src, trans::fti));
        // cur_block->instructions.push_back(std::make_shared<LoongArch::mov>(dst, const_reg_l, mov::ftg));
        // const_reg_l = bak;
    }
    check_write_back(dst);
}

void LoongArch::ProgramBuilder::visit(ir::ir_libfunc& node) {
    
}

void LoongArch::ProgramBuilder::visit(ir::memset& node) {
    cur_block->instructions.push_back(std::make_shared<RegImmInst>(RegImmInst::addi_d, Reg{sp}, Reg{sp}, -24));
    cur_block->instructions.push_back(std::make_shared<st>(spill_dst, Reg{sp}, 0, st::st_d));
    cur_block->instructions.push_back(std::make_shared<st>(spill_use_1, Reg{sp}, 8, st::st_d));
    cur_block->instructions.push_back(std::make_shared<st>(spill_use_2, Reg{sp}, 16, st::st_d));


    auto var_it = cur_mapping->mem_var.find(node.base->id);
    int cnt = node.cnt * node.base->size;
    if(var_it != cur_mapping->mem_var.end()) {
        int offset = var_it->second - cur_func->stack_size;
        cur_block->instructions.push_back(std::make_shared<RegImmInst>(RegImmInst::addi_d, spill_dst, Reg{fp}, offset));
    }
    else if(node.base->is_global) {
        cur_block->instructions.push_back(std::make_shared<la>(node.base, spill_dst, la::local));
    }
    else {
        abort();
    }
    cur_block->instructions.push_back(std::make_shared<LoadImm>(spill_use_1, cnt));
    // cur_block->instructions.push_back(std::make_shared<RegRegInst>(RegRegInst::add_d, spill_use_1, spill_dst, spill_use_1));
    cur_block->instructions.push_back(std::make_shared<LoadImm>(spill_use_2, node.base->size));
    cur_block->instructions.push_back(std::make_shared<Bl>("Looouiiis_self_memset"));


    
    cur_block->instructions.push_back(std::make_shared<ld>(spill_dst, Reg{sp}, 0, ld::ld_d));
    cur_block->instructions.push_back(std::make_shared<ld>(spill_use_1, Reg{sp}, 8, ld::ld_d));
    cur_block->instructions.push_back(std::make_shared<ld>(spill_use_2, Reg{sp}, 16, ld::ld_d));
    cur_block->instructions.push_back(std::make_shared<RegImmInst>(RegImmInst::addi_d, Reg{sp}, Reg{sp}, 24));
}

void LoongArch::ProgramBuilder::visit(ir::tail_call& node) {
    auto call_ins = node.get_call_ins();

    auto raw = call_ins->get_live_in();
    std::set<Reg> live_in;
    for(auto ir_reg : raw) {
        auto reg = cur_mapping->transfer_reg(*ir_reg.get());
        live_in.insert(reg);
    }
    if(live_in.size()) {
        cur_block->instructions.push_back(std::make_shared<LoongArch::RegImmInst>(RegImmInst::addi_d, Reg{sp}, Reg{sp}, -(caller_save_regs.size() * 8)));
        for(int i = 0; i < global_arg_regs.size(); i++) {
            if(live_in.find(global_arg_regs[i]) != live_in.end()) {
                cur_block->instructions.push_back(std::make_shared<LoongArch::st>(global_arg_regs[i], Reg{sp}, 8 * i, global_arg_regs[i].is_float()? st::fst_f : st::st_d));
            }
        }        
    }

    int i_pointer = 10;
    int f_pointer = 10;
    int cur_stk = 0;
    for(auto par : call_ins->params) {
        auto is_spilled = call_ins->spilled_obj.find(par);
        Reg reg;
        if(is_spilled != call_ins->spilled_obj.end()) {
            auto ir_reg = is_spilled->second->addr;
            auto par_reg = std::dynamic_pointer_cast<ir::ir_reg>(par);
            assert(par_reg);
            reg = const_reg_l;
            if(par_reg->type == vartype::FLOAT) {
                reg.type = FLOAT;
            }
            else {
                reg.type = INT;
            }
            int offset = cur_mapping->mem_var.find(ir_reg->id)->second;
            offset = cur_func->stack_size - offset;
            if(par_reg->is_arr) {
                cur_block->instructions.push_back(std::make_shared<LoongArch::ld>(reg, Reg{fp}, -offset, ld::ld_d));
            }
            else {
                cur_block->instructions.push_back(std::make_shared<LoongArch::ld>(reg, Reg{fp}, -offset, reg.is_float() ? ld::fld_f : ld::ld_w));
            }
        }
        else {
            par->accept(*this);
            reg = pass_reg;   
        }
        if(reg.is_float()) {
            if(f_pointer < 18) {
                cur_block->instructions.push_back(std::make_shared<LoongArch::mov>(Reg{f_pointer++, FLOAT}, reg, mov::ftf_f));
            }
            else {
                if(i_pointer < 18) {
                    cur_block->instructions.push_back(std::make_shared<LoongArch::mov>(Reg{i_pointer++}, reg, mov::ftg));
                }
                else {
                    auto par_reg = std::dynamic_pointer_cast<ir::ir_reg>(par);
                    if(par_reg && par_reg->is_arr) {
                        cur_block->instructions.push_back(std::make_shared<st>(reg, Reg{sp}, cur_stk, st::fst_d));
                    }
                    else {
                        cur_block->instructions.push_back(std::make_shared<stptr>(reg, Reg{sp}, cur_stk, stptr::fst_d));
                    }
                    cur_stk += 8;
                }
            }
        }
        else {
            if(i_pointer < 18) {
                if(reg.id >= 10 && reg.id <= 17/* && reg.type != FBOOL*/) {
                    auto it = std::find(global_arg_regs.begin(), global_arg_regs.end(), reg);
                    int offset = std::distance(global_arg_regs.begin(), it) * 8 + cur_mapping->call_mem;
                    cur_block->instructions.push_back(std::make_shared<ld>(Reg{i_pointer++}, Reg{sp}, offset, ld::ld_d));
                }
                else
                    cur_block->instructions.push_back(std::make_shared<RegRegInst>(RegRegInst::add_d, Reg{i_pointer++}, reg, Reg{0}));
            }
            else {
                // cur_block->instructions.push_back(std::make_shared<stptr>(reg, Reg{sp}, 8 * (i_pointer++) - 12, stptr::st_d));
                if(reg.id >= 10 && reg.id <= 17/* && reg.type != FBOOL*/) {
                    auto it = std::find(global_arg_regs.begin(), global_arg_regs.end(), reg);
                    int offset = std::distance(global_arg_regs.begin(), it) * 8 + cur_mapping->call_mem;
                    cur_block->instructions.push_back(std::make_shared<ld>(const_reg_r, Reg{sp}, offset, ld::ld_d));
                    const_reg_r.type = reg.type;
                    reg = const_reg_r;
                }
                auto par_reg = std::dynamic_pointer_cast<ir::ir_reg>(par);
                if(par_reg && par_reg->is_arr) {
                    cur_block->instructions.push_back(std::make_shared<st>(reg, Reg{sp}, cur_stk, st::st_d));
                }
                else {
                    cur_block->instructions.push_back(std::make_shared<stptr>(reg, Reg{sp}, cur_stk, stptr::st_d));
                }
                cur_stk += 8;
            }
        }
    }
    assert(cur_stk == 0);

    // 恢复栈
    // cur_block->instructions.push_back(
    //     std::make_shared<LoongArch::ld>(Reg{22},Reg{3},this->cur_func->stack_size - 16,ld::ld_d)
    // );
    // cur_block->instructions.push_back(std::make_shared<LoongArch::ld>(Reg{ra}, Reg{sp}, cur_func->stack_size - 8, ld::ld_d));
    if(live_in.size()) {
        cur_block->instructions.push_back(std::make_shared<LoongArch::RegImmInst>(RegImmInst::addi_d, Reg{sp}, Reg{sp}, caller_save_regs.size() * 8));
    }
    cur_block->instructions.push_back(
        std::make_shared<LoongArch::RegImmInst>(RegImmInst::addi_d,Reg{sp},Reg{sp},cur_func->stack_size - 16)   // 很可惜，还是需要保存返回地址以及fp，所以tail_call也不是可以一直递归调用
    );

    // cur_block->instructions.push_back(std::make_shared<LoongArch::st>(Reg{ra}, Reg{sp}, 0, st::st_d));  
    auto func_name = node.get_call_ins()->func_name;
    cur_block->instructions.push_back(std::make_shared<LoongArch::Bl>(func_name));

    cur_block->instructions.push_back(std::make_shared<LoongArch::ld>(Reg{fp},Reg{sp}, 0,ld::ld_d));
    cur_block->instructions.push_back(std::make_shared<LoongArch::ld>(Reg{ra}, Reg{sp}, 8, ld::ld_d));
    cur_block->instructions.push_back(std::make_shared<LoongArch::RegImmInst>(RegImmInst::addi_d, Reg{sp}, Reg{sp}, 16));
    cur_block->instructions.push_back(std::make_shared<LoongArch::jr>(true));
}
