#ifndef __IR_SCOPE_HPP__
#define __IR_SCOPE_HPP__

#include <string>
#include <vector>
#include <unordered_map>
#include "ir.hpp"

class Scope {
private:
    vector<unordered_map<string, Value*>> tables; // 作用域变量表，记录从变量名到变量值的映射
public:
    /// @brief 进入子作用域
    /// @note 在当前作用域的基础上创建一个新的作用域
    void enter_child_scope();

    /// @brief 退出子作用域
    /// @note 删除当前作用域，返回到上一级作用域
    void exit_child_scope();

    /// @brief 添加变量到当前作用域
    /// @param name 变量名
    /// @param val 变量值
    /// @return 是否成功
    bool add_to_cur_scope(const string& name,Value* val);

    /// @brief 在当前作用域及其父作用域中查找变量
    /// @param name 变量名称
    /// @return 找到的变量，如果未找到则返回 nullptr
    /// @note 从最内层作用域开始查找，直到找到为止
    Value* find_val_with_name(const string& name);

    /// @brief 判断当前作用域是否为全局作用域
    /// @note 全局作用域是最外层的作用域，通常只有一个
    /// @return 如果是全局作用域则返回 true，否则返回 false
    bool is_global();
};

#endif // __IR_SCOPE_HPP__