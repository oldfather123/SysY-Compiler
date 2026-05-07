#include "FuncInfo.hpp"

FuncInfo::FuncInfo(Function* func): func(func), is_inlineable(false) {
    this->param_count = func->args.size();
    this->bb_count = func->bb_list.size();
    this->inst_count = 0;
    this->has_return = func->get_ret_type() != func->parent->void_type;
    this->is_recursive = false;
    this->has_global_access = false;
    for(auto& bb: func->bb_list) {
        this->inst_count += bb->inst_list.size();
        for(auto& inst: bb->inst_list) {
            if(auto call_inst = dynamic_cast<CallInst*>(inst)) {
                this->call_insts.push_back(call_inst);
                if(auto callee_func = dynamic_cast<Function*>(call_inst->get_op(call_inst->operands.size() - 1))) {
                    this->callee_funcs.insert(callee_func);
                    if(callee_func == func) {
                        this->is_recursive = true;
                    }
                }
            } else if(auto store_inst = dynamic_cast<StoreInst*>(inst)) {
                auto ptr = store_inst->get_op(1);
                if(dynamic_cast<GlobalVariable*>(ptr)) {
                    this->has_global_access = true;
                }
            }
        }
    }
    this->is_pure = is_pure_func(func);
}

bool is_pure_func(Function* func) {
    if(func->bb_list.empty()) { // 声明函数为 SysY2022 语言的库函数，基本是 IO 函数，设定为不纯
        // cerr << "[Info] Declaration: NOT pure\n";
        return false;
    }
    // 遍历函数的所有指令
    for(auto& bb : func->bb_list) {
        for(auto& inst : bb->inst_list) {
            // 检查是否有写全局变量的 Store 指令
            if(auto store_inst = dynamic_cast<StoreInst*>(inst)) {
                auto ptr = store_inst->get_op(1); // 第二个操作数是指针
                if(dynamic_cast<GlobalVariable*>(ptr)) {
                    // cerr << "[Info] Global variable access: NOT pure\n";
                    // TODO: 个人认为可以将全局变量的写操作视为纯函数，因为程序执行顺序和内容确定的情况下，完全不会因为全局变量的写操作而影响函数的输出
                    return false; // 写全局变量，非纯函数
                }
            } else if(auto call_inst = dynamic_cast<CallInst*>(inst)) { // 检查调用的函数是否是纯函数
                if(auto callee_func = dynamic_cast<Function*>(call_inst->get_op(call_inst->operands.size() - 1))) {
                    if(callee_func == func) {
                        // cerr << "[Info] Recursive call: NOT pure\n";
                        return false; // 递归调用，非纯函数
                    } else if(!is_pure_func(callee_func)) {
                        // cerr << "[Info] Call to non-pure function: " << callee_func->name << " NOT pure\n";
                        return false; // 调用非纯函数，非纯函数
                    }
                }
            }
        }
    }
    // cerr << "[Info] Function " << func->name << " is pure.\n";
    return true;
}

void show_func_info(const FuncInfo& info) {
    cerr << "Function: " << info.func->name << "\n"
              << "  Parameters: " << info.param_count << "\n"
              << "  Basic Blocks: " << info.bb_count << "\n"
              << "  Instructions: " << info.inst_count << "\n"
              << "  Has Return: " << (info.has_return ? "Yes" : "No") << "\n"
              << "  Is Pure: " << (info.is_pure ? "Yes" : "No") << "\n"
              << "  Is Recursive: " << (info.is_recursive ? "Yes" : "No") << "\n"
              << "  Has Global Access: " << (info.has_global_access ? "Yes" : "No") << "\n"
              << "  Is Inlineable: " << (info.is_inlineable ? "Yes" : "No") << "\n"
              << "  Callee Functions: ";
    for(const auto& callee : info.callee_funcs) {
        cerr << callee->name << " ";
    }
    cerr << "\n  Caller Functions: ";
    for(const auto& caller : info.caller_funcs) {
        cerr << caller->name << " ";
    }
    cerr << "\n";
}

void show_func_info_map(const unordered_map<Function*, FuncInfo>& func_info_map) {
    for (const auto& [func, info]: func_info_map) {
        if(func->bb_list.empty()) {
            continue;
        }
        show_func_info(info);
    }
}