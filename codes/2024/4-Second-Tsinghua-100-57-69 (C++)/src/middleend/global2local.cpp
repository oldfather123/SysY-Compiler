#include "middleend/global2local.hpp"
#include "middleend/cfg.hpp"
#include "middleend/use_def_analysis.hpp"
#include "middleend/copy_propagation.hpp"
#include <iostream>
#include <queue>

namespace middleend {

void Global2Local::mark_global_temp() {
    for (auto& func : *module_->get_functions()) {
        func->global_temps.clear();
        for (auto& bb : *func->get_basic_blocks()) {
            for (auto& inst : *bb->get_instructions()) {
                TypeCase(la, ir::instruction::LoadAddr*, inst) {
                    func->global_temps[la->get_addr()] = la->get_name();
                }
                TypeCase(elementptr, ir::instruction::ElementPtr*, inst) {
                    if (func->global_temps.count(elementptr->get_base())) {
                        func->global_temps[elementptr->get_dst()] = func->global_temps[elementptr->get_base()];
                    }
                }
            }
        }
    }
}

void Global2Local::run() {
    mark_global_temp();
    std::unordered_map<std::string, ConstValue> global_const;
    for (auto &data: *module_->get_global_variables()) {
        if (!data->get_data_type().is_array()) {
            global_const[data->get_name()] = std::get<0>(data->get_init_value());
        }
    }
    std::unordered_set<std::string> main_used;
    std::unordered_set<std::string> other_used;
    // 这里保证接下来要替换的全局标量永远都不改变
    for (auto& func : *module_->get_functions()) {
        if (func->get_name() == "main") {
            for (auto& bb : *func->get_basic_blocks()) {
                for (auto& inst : *bb->get_instructions()) {
                    TypeCase(la, ir::instruction::LoadAddr*, inst) {
                        auto global_data = module_->global_var_map[la->get_name()];
                        if (global_data->get_data_type().is_array()) continue;
                        func->global_temps[la->get_addr()] = la->get_name();
                        main_used.insert(la->get_name());
                    }
                    TypeCase(store, ir::instruction::Store*, inst) {
                        if (func->global_temps.count(store->getaddr())) {
                            global_const.erase(func->global_temps[store->getaddr()]);
                        }
                    }
                }
            }
        }
        else {
            for (auto& bb : *func->get_basic_blocks()) {
                for (auto& inst : *bb->get_instructions()) {
                    TypeCase(la, ir::instruction::LoadAddr*, inst) {
                        auto global_data = module_->global_var_map[la->get_name()];
                        if (global_data->get_data_type().is_array()) continue;
                        func->global_temps[la->get_addr()] = la->get_name();
                        other_used.insert(la->get_name());
                    }
                    TypeCase(store, ir::instruction::Store*, inst) {
                        if (func->global_temps.count(store->getaddr())) {
                            global_const.erase(func->global_temps[store->getaddr()]);
                        }
                    }
                }
            }
        }
    }
    // 把main函数里且不在其它函数里的全局标量转成局部标量（方便之后进行mem2reg）
    std::unordered_map<std::string, Temp*> global2local_map;
    auto main_func = module_->get_func_map()["main"];
    int temp_used_idx = main_func->get_temp_used();
    auto main_entry = main_func->get_entry();
    for (auto data_name: main_used) {
        if (other_used.count(data_name)) continue;
        has_new_alloca = true;
        auto global_data = module_->global_var_map[data_name];
        Temp *local_temp = new Temp(temp_used_idx++, global_data->get_data_type().get_pointer_type());
        Temp *init_temp = new Temp(temp_used_idx++, global_data->get_data_type());
        main_entry->add_instruction_front(new ir::instruction::Store(local_temp, init_temp, false));
        main_entry->add_instruction_front(new ir::instruction::LoadImm4(init_temp, std::get<0>(global_data->get_init_value())));
        main_entry->add_instruction_front(new ir::instruction::Alloca(local_temp, global_data->get_data_type()));

        global2local_map[data_name] = local_temp;
        module_->global_var_map.erase(data_name);
        module_->get_global_variables()->erase(std::remove(module_->get_global_variables()->begin(), module_->get_global_variables()->end(), global_data), module_->get_global_variables()->end());
    }
    main_func->set_temp_used(temp_used_idx);
    CFG *cfg = new CFG(main_func);
    UseDefAnalysis *ud = new UseDefAnalysis(cfg);
    for (auto &bb: *main_func->get_basic_blocks()) {
        for (auto iter = bb->get_instructions()->begin(); iter != bb->get_instructions()->end(); iter++) {
            TypeCase(la, ir::instruction::LoadAddr*, *iter) {
                if (!module_->global_var_map.count(la->get_name())) {
                    auto dst = la->get_addr();
                    auto src = global2local_map[la->get_name()];
                    while(ud->get_usesets()[dst].size() > 0) {
                        auto inst = *(ud->get_usesets()[dst].begin());
                        TypeCase(load, ir::instruction::Load*, inst) {
                            load->set_not_delete(false); 
                        }
                        else TypeCase(store, ir::instruction::Store*, inst) {
                            store->set_not_delete(false);
                        }
                        ud->change_use(dst, src, inst);
                        inst->change_use(dst, src);
                    }
                    bb->get_instructions()->erase(iter);
                    iter--;
                }
            }
        }
    }
    // 把所有函数里的没有store的全局标量转成局部标量
    for (auto& func : *module_->get_functions()) {
        std::unordered_map<std::string, Temp*> name2temp;
        std::unordered_map<Temp*, Temp*> temp2li;
        int temp_used_idx = func->get_temp_used();
        cfg = new CFG(func);
        ud = new UseDefAnalysis(cfg);
        for (auto& bb : *func->get_basic_blocks()) {
            auto insts = *bb->get_instructions();
            for (auto& inst : insts) {
                // inst->print(std::cout);
                TypeCase(la, ir::instruction::LoadAddr*, inst) {
                    if (global_const.count(la->get_name())) {
                        if (!name2temp.count(la->get_name())) {
                            Temp *temp = new Temp(temp_used_idx++, global_const[la->get_name()].type);
                            name2temp[la->get_name()] = temp;
                            cfg->get_bb(0)->add_instruction_front(new ir::instruction::LoadImm4(temp, global_const[la->get_name()]));
                        }
                        // std::cout << la->to_str() << " -> " << name2temp[la->get_name()]->to_str() << std::endl;
                        temp2li[la->get_addr()] = name2temp[la->get_name()];
                    }
                }
            }
        }
        func->set_temp_used(temp_used_idx);
        for (auto& bb : *func->get_basic_blocks()) {
            for (auto& inst : *bb->get_instructions()) {
                TypeCase(load, ir::instruction::Load*, inst) {
                    // load->print(std::cout);
                    if (temp2li.count(load->getaddr())) {
                        // std::cout << "load " << load->getdst()->to_str() << " <- " << temp2li[load->getaddr()]->to_str() << std::endl;
                        copy_propagation(ud, load->getdst(), temp2li[load->getaddr()]);
                    }
                }
            }
        }
    }
    for (auto& [global_data_name, init_value] : global_const) {
        auto global_data = module_->global_var_map[global_data_name];
        module_->get_global_variables()->erase(std::remove(module_->get_global_variables()->begin(), module_->get_global_variables()->end(), global_data), module_->get_global_variables()->end());
    }
}

}