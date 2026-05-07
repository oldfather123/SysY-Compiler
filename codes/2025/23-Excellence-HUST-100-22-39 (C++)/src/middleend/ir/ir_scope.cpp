#include "ir_scope.hpp"
#include "ir_value.hpp"

void Scope::enter_child_scope() {
    tables.emplace_back();
}

void Scope::exit_child_scope() {
    if(tables.empty()) {
        return; // 没有子作用域
    }
    tables.pop_back();
}

bool Scope::add_to_cur_scope(const string& name, Value* val) {
    return (tables[tables.size() - 1].insert({name, val})).second;
}

Value* Scope::find_val_with_name(const string& name) {
    for(auto it = tables.rbegin(); it != tables.rend(); ++it) {
        auto var_it = it->find(name);
        if(var_it != it->end()) {
            return var_it->second; // 找到变量
        }
    }
    return nullptr; // 未找到变量
}

bool Scope::is_global() {
    return tables.size() == 1;
}