#include "middleend/llvm.hpp"

namespace middleend {

void LLVM::print_llvm(std::ostream &out) {
    for (auto lib_func_pair : module_->lib_funcs) {
        auto lib_func = lib_func_pair.second;
        if (lib_func_pair.first == "__memset_zero__") { // 内置函数存在定义
            out << "define void @__memset_zero__(i32* %TT0, i32 %TT1) {\n";
            out << "B0:\n";
            out << "  %TTend = getelementptr i32, i32* %TT0, i32 %TT1\n";
            out << "  br label %B1\n";
            out << "B1:\n";
            out << "  %TT2 = phi i32* [ %TT0, %B0 ], [ %TT3, %B1 ]\n";
            out << "  store i32 0, i32* %TT2\n";
            out << "  %TT3 = getelementptr i32, i32* %TT2, i32 1\n";
            out << "  %TT4 = icmp eq i32* %TT3, %TTend\n";
            out << "  br i1 %TT4, label %B2, label %B1\n";
            out << "B2:\n";
            out << "  ret void\n";
            out << "}\n";
        } else if (lib_func_pair.first == "starttime") {
            out << "declare void @_sysy_starttime(i32)\n";
        } else if (lib_func_pair.first == "stoptime") {
            out << "declare void @_sysy_stoptime(i32)\n";
        } else {
            out << "declare " << type_llvm_string(lib_func->get_ret_type()) << " @" << lib_func->get_name() << "(";
            for (int i = 0; i < lib_func->get_arg_num(); i++) {
                out << type_llvm_string(lib_func->get_arguments()->at(i)->get_data_type());
                if (i != lib_func->get_arg_num() - 1) {
                    out << ", ";
                }
            }
            out << ")\n";
        }
    }

    for (auto global : *module_->get_global_variables()) {
        out << "@" << global->get_name() << " = global ";
        auto init = global->get_init_value();
        // if (global->get_data_type().is_array()) {
            out << "[" << global->get_data_type().nr_elems() << " x " << type_llvm_string(global->get_data_type()) << "] ";
            if (global->get_not_init()) {
                out << "[";
                for (int i = 0; i < global->get_data_type().nr_elems(); i++) {
                    if (global->get_data_type().base_type == Int) {
                        out << "i32 0";
                    } else {
                        out << "float 0.0";
                    }
                    if (i != global->get_data_type().nr_elems() - 1) {
                        out << ", ";
                    }
                }
                out << "]";
            } else {
                if (global->get_data_type().is_array()) {
                    if (std::get_if<std::vector<ConstValue>>(&init) == nullptr) {
                        out << "[";
                        for (int i = 0; i < global->get_data_type().nr_elems(); i++) {
                            if (global->get_data_type().base_type == Int) {
                                out << "i32 0";
                            } else {
                                out << "float 0.0";
                            }
                            if (i != global->get_data_type().nr_elems() - 1) {
                                out << ", ";
                            }
                        }
                        out << "]";
                    } else {
                        auto init_value = std::get<1>(init);
                        out << "[";
                        for (int i = 0; i < init_value.size(); i++) {
                            if (init_value[i].type == Int) {
                                out << "i32 " << init_value[i].iv;
                            } else {
                                out << "float " << init_value[i].fv;
                            }
                            if (i != init_value.size() - 1) {
                                out << ", ";
                            }
                        }
                        out << "]";
                    }
                } else {
                    auto init_value = std::get<0>(init);
                    out << "[";
                    if (init_value.type == Int) {
                        out << "i32 " << init_value.iv;
                    } else {
                        out << "float " << init_value.fv;
                    }
                    out << "]";
                }
            }
        // } else {
        //     out << type_llvm_string(global->get_data_type()) << " ";
        //     if (global->get_not_init()) {
        //         out << "0";
        //     } else {
        //         auto init_value = std::get<0>(init);
        //         if (init_value.type == Int) {
        //             out << init_value.iv;
        //         } else {
        //             out << init_value.fv;
        //         }
        //     }
        // }
        out << "\n";
    }

    for (auto func : *module_->get_functions()) {
        out << "define " << type_llvm_string(func->get_ret_type()) << " @" << func->get_name() << "(";
        for (int i = 0; i < func->get_arg_num(); i++) {
            out << type_llvm_string(func->get_arguments()->at(i)->get_data_type()) << " " << func->get_arg_temp()->at(i)->to_llvm_str();
            if (i != func->get_arg_num() - 1) {
                out << ", ";
            }
        }
        out << ") {\n";
        for (auto &bb : *func->get_basic_blocks()) {
            out << "B" << bb->get_index() << ":\n";
            for (auto &inst : *bb->get_instructions()) {
                out << "  " << inst->to_llvm_str(llvm_temp_cnt) << "\n";
            }
        }
        out << "}\n";
    }
}

} // namespace middleend