#include "sysy-builder.hpp"
#include <iostream>

#define CONST_FP(num) ConstantFP::get((float)num, module.get())
#define CONST_INT(num) ConstantInt::get(num, module.get())

// 全局类型指针
Type *VOID_T;
Type *INT1_T;
Type *INT32_T;
Type *INT32PTR_T;
Type *FLOAT_T;
Type *FLOATPTR_T;

// 检查是否可以安全地插入指令
bool CminusfBuilder::can_insert_instruction() {
    auto *bb = builder->get_insert_block();
    return bb && !bb->is_terminated();
}

// 确保有有效的插入点
void CminusfBuilder::ensure_valid_insert_point() {
    if (!can_insert_instruction()) {
        // 创建一个新的不可达基本块
        auto *unreachableBB = BasicBlock::create(module.get(), "unreachable_cont", context.func);
        builder->set_insert_point(unreachableBB);
    }
}

// 类型提升函数
bool CminusfBuilder::promote(Value **l_val_p, Value **r_val_p, const_val *l_num, const_val *r_num) {
    bool is_int = true;  // 默认为整数类型
    auto &l_val = *l_val_p;
    auto &r_val = *r_val_p;
    
    // 检查空指针
    if (!l_val || !r_val) {
        return true;  // 默认返回整数类型
    }
    
    // 获取类型，处理 i1 类型
    Type *l_type = l_val->get_type();
    Type *r_type = r_val->get_type();
    
    // 如果有 i1 类型，先转换为 i32
    if (l_type->is_int1_type()) {
        if (can_insert_instruction() && !context.in_const_exp) {
            l_val = builder->create_zext(l_val, INT32_T);
            l_type = INT32_T;
        }
    }
    
    if (r_type->is_int1_type()) {
        if (can_insert_instruction() && !context.in_const_exp) {
            r_val = builder->create_zext(r_val, INT32_T);
            r_type = INT32_T;
        }
    }
    
    // 判断是否需要类型提升
    if (l_type->is_float_type() || r_type->is_float_type()) {
        // 至少有一个是浮点数，需要都转换为浮点数
        is_int = false;
        
        if (l_type->is_integer_type()) {
            if (context.in_const_exp) {
                l_num->f_val = (float)l_num->i_val;
            } else if (can_insert_instruction()) {
                ensure_valid_insert_point();
                l_val = builder->create_sitofp(l_val, FLOAT_T);
            }
        }
        
        if (r_type->is_integer_type()) {
            if (context.in_const_exp) {
                r_num->f_val = (float)r_num->i_val;
            } else if (can_insert_instruction()) {
                ensure_valid_insert_point();
                r_val = builder->create_sitofp(r_val, FLOAT_T);
            }
        }
    }
    
    return is_int;
}

// 值类型转换
Value* CminusfBuilder::cast_value(Value *val, Type *target_type) {
    // 检查空指针
    if (!val || !target_type) {
        return val;
    }
    
    Type *val_type = val->get_type();
    
    if (val_type == target_type) {
        return val;
    }
    
    ensure_valid_insert_point();
    
    // 处理 i1 类型
    if (val_type->is_int1_type()) {
        if (target_type->is_int32_type()) {
            return builder->create_zext(val, INT32_T);
        } else if (target_type->is_float_type()) {
            // i1 -> float: 先转换为 i32，再转换为 float
            auto *i32_val = builder->create_zext(val, INT32_T);
            return builder->create_sitofp(i32_val, FLOAT_T);
        }
    }
    
    if (target_type->is_int32_type() && val_type->is_float_type()) {
        return builder->create_fptosi(val, INT32_T);
    } else if (target_type->is_float_type() && val_type->is_int32_type()) {
        return builder->create_sitofp(val, FLOAT_T);
    }
    
    return val;
}

// 获取常量数组
Constant* CminusfBuilder::get_const_array(Type *array_type_t, std::vector<const_val> val) {
    if (!array_type_t) {
        return nullptr;
    }
    
    if (array_type_t->is_array_type()) {
        auto array_type = static_cast<ArrayType *>(array_type_t);
        auto dim = array_type->get_num_of_elements();
        auto elem_type = array_type->get_element_type();
        
        if (!elem_type) {
            return nullptr;
        }
        
        // 对于大数组，检查是否全为零
        if (dim > 1000) {
            bool all_zero = true;
            size_t check_size = std::min(val.size(), static_cast<size_t>(dim));
            for (size_t i = 0; i < check_size; i++) {
                if (val[i].i_val != 0 || val[i].f_val != 0.0f) {
                    all_zero = false;
                    break;
                }
            }
            // 如果提供的值全为零，且剩余的值也应该是零（未提供的值默认为零）
            if (all_zero) {
                return ConstantZero::get(array_type_t, module.get());
            }
        }
        
        std::vector<Constant *> temp;
        temp.reserve(dim);  // 预分配空间以提高性能
        
        if (elem_type->is_array_type()) {
            // 多维数组 - 计算子数组大小
            int sub_array_size = 1;
            Type *current_type = elem_type;
            while (current_type->is_array_type()) {
                auto *arr_type = static_cast<ArrayType *>(current_type);
                sub_array_size *= arr_type->get_num_of_elements();
                current_type = arr_type->get_element_type();
            }
            
            // 为每个子数组创建常量
            for (unsigned int i = 0; i < dim; i++) {
                size_t start_idx = i * sub_array_size;
                
                std::vector<const_val> sub_vals;
                sub_vals.reserve(sub_array_size);  // 预分配空间
                
                for (size_t j = start_idx; j < start_idx + sub_array_size; j++) {
                    if (j < val.size()) {
                        sub_vals.push_back(val[j]);
                    } else {
                        sub_vals.push_back({0, 0.0f});
                    }
                }
                
                auto sub_const = get_const_array(elem_type, sub_vals);
                if (!sub_const) {
                    sub_const = ConstantZero::get(elem_type, module.get());
                }
                temp.push_back(sub_const);
            }
        } else {
            // 一维数组 - 基本类型元素
            for (unsigned int i = 0; i < dim; i++) {
                if (i < val.size()) {
                    if (elem_type->is_int32_type()) {
                        temp.push_back(ConstantInt::get(int(val[i].i_val), module.get()));
                    } else if (elem_type->is_float_type()) {
                        temp.push_back(ConstantFP::get(val[i].f_val, module.get()));
                    } else {
                        temp.push_back(ConstantZero::get(elem_type, module.get()));
                    }
                } else {
                    // 用0填充
                    if (elem_type->is_int32_type()) {
                        temp.push_back(ConstantInt::get(0, module.get()));
                    } else if (elem_type->is_float_type()) {
                        temp.push_back(ConstantFP::get(0.0f, module.get()));
                    } else {
                        temp.push_back(ConstantZero::get(elem_type, module.get()));
                    }
                }
            }
        }
        
        return ConstantArray::get(array_type, temp);
    } else if (array_type_t->is_int32_type()) {
        return ConstantInt::get(int(val[0].i_val), module.get());
    } else if (array_type_t->is_float_type()) {
        return ConstantFP::get(val[0].f_val, module.get());
    }
    return nullptr;
}

void CminusfBuilder::process_array_init_val(ASTInitVal &node, ArrayInitHelper &helper,
                                          std::vector<Value*> &init_vals,
                                          std::vector<const_val> &const_init_vals,
                                          bool is_const) {
    
              
    if (node.exp) {
        // 单个表达式
       
        
        bool saved_in_const = context.in_const_exp;
        if (is_const) {
            context.in_const_exp = true;
        }
        
        node.exp->accept(*this);
        
        if (is_const) {
            context.in_const_exp = saved_in_const;
            
            const_init_vals.push_back(context.val);
        } else {
            if (context.cur_val) {
                if (auto *const_int = dynamic_cast<ConstantInt*>(context.cur_val)) {
                    
                } else if (auto *const_fp = dynamic_cast<ConstantFP*>(context.cur_val)) {
                    
                } else {
                    
                }
            } else {
                
            }
            init_vals.push_back(context.cur_val);
        }
        
        helper.advance();
    } else {
        // 嵌套初始化列表处理
       
        int start_pos = helper.getCurrentPosition();
        std::vector<int> dims = context.array_dims;
 
        bool is_top_level = false;
        
        if (start_pos == 0 && dims.size() > 1) {
            // 对于多维数组，检查初始化列表的模式
            bool has_list = false;
            bool has_single = false;
            
            for (auto &item : node.initVals) {
                if (item->exp) {
                    has_single = true;
                } else {
                    has_list = true;
                }
            }
            
            if (has_list || node.initVals.size() <= static_cast<size_t>(dims[0])) {
                is_top_level = true;
            }
            
           
        }
        
        if (is_top_level) {
            // 计算子数组大小
            int sub_array_size = 1;
            for (size_t i = 1; i < dims.size(); i++) {
                sub_array_size *= dims[i];
            }
            
           
            
            // 保存原始维度信息，防止递归调用时被修改
            std::vector<int> saved_dims = context.array_dims;
            
            // 处理每个顶层元素
            for (size_t i = 0; i < node.initVals.size() && !helper.isComplete(); i++) {
                
                if (node.initVals[i]->exp) {
                    // 单值 - 作为新子数组的开始
                    process_array_init_val(*node.initVals[i], helper, init_vals, const_init_vals, is_const);
                    
                    // 查找后续的单值来填充当前子数组
                    int filled = 1;
                    size_t j = i + 1;
                    while (j < node.initVals.size() && filled < sub_array_size && node.initVals[j]->exp) {
                        process_array_init_val(*node.initVals[j], helper, init_vals, const_init_vals, is_const);
                        filled++;
                        j++;
                    }
                    i = j - 1;
                    
                    // 填充剩余位置到子数组边界
                    while (helper.getCurrentPosition() % sub_array_size != 0 && !helper.isComplete()) {
                       
                        if (is_const) {
                            const_init_vals.push_back({0, 0.0f});
                        } else {
                            init_vals.push_back(nullptr);
                        }
                        helper.advance();
                    }
                } else {
                    // 嵌套列表 - 递归处理
                    int pos_before = helper.getCurrentPosition();
                    
                    
                    std::vector<int> nested_dims;
                    for (size_t d = 1; d < saved_dims.size(); d++) {
                        nested_dims.push_back(saved_dims[d]);
                    }
                    context.array_dims = nested_dims;
                    
                    process_array_init_val(*node.initVals[i], helper, init_vals, const_init_vals, is_const);
                    
                    // 恢复原始维度
                    context.array_dims = saved_dims;
                    
                    int pos_after = helper.getCurrentPosition();
                   
                    
                    // 如果是空列表，需要填充整个子数组
                    if (pos_after == pos_before) {
                        // 空列表 - 填充整个子数组为0
                       
                        for (int j = 0; j < sub_array_size && !helper.isComplete(); j++) {
                            if (is_const) {
                                const_init_vals.push_back({0, 0.0f});
                            } else {
                                init_vals.push_back(nullptr);
                            }
                            helper.advance();
                        }
                    } else {
                        // 非空列表 - 填充到子数组边界
                        while (helper.getCurrentPosition() % sub_array_size != 0 && !helper.isComplete()) {
                            
                            if (is_const) {
                                const_init_vals.push_back({0, 0.0f});
                            } else {
                                init_vals.push_back(nullptr);
                            }
                            helper.advance();
                        }
                    }
                }
                
                
            }
        } else {
            // 普通列表处理 - 扁平化的初始化
          
            for (auto &val : node.initVals) {
                if (helper.isComplete()) break;
                process_array_init_val(*val, helper, init_vals, const_init_vals, is_const);
            }
        }
    }
    
   
}

void CminusfBuilder::process_const_array_init_val(ASTConstInitVal &node, ArrayInitHelper &helper) {
    
    
    if (node.constExp) {
        // 单个表达式
        node.constExp->accept(*this);
        
        // 保存常量值
        if (context.cur_val) {
            if (auto *const_int = dynamic_cast<ConstantInt*>(context.cur_val)) {
                context.val.i_val = const_int->get_value();
                
            } else if (auto *const_fp = dynamic_cast<ConstantFP*>(context.cur_val)) {
                context.val.f_val = const_fp->get_value();
                
            }
        }
        context.const_array_init_vals.push_back(context.val);
        helper.advance();
    } else {
        // 嵌套初始化列表
       
        
        int start_pos = helper.getCurrentPosition();
        std::vector<int> dims = context.array_dims;
        
        // 判断是否是顶层列表
        bool is_top_level = false;
        
        if (start_pos == 0 && dims.size() > 1) {
            // 检查初始化列表的模式
            bool has_list = false;
            bool has_single = false;
            
            for (auto &item : node.constInitVals) {
                if (item->constExp) {
                    has_single = true;
                } else {
                    has_list = true;
                }
            }
            
            // 如果有列表元素，或者元素数量较少（暗示每个元素对应一个子数组）
            if (has_list || node.constInitVals.size() <= static_cast<size_t>(dims[0])) {
                is_top_level = true;
            }
            
            
        }
        
        if (is_top_level) {
            // 计算子数组大小
            int sub_array_size = 1;
            for (size_t i = 1; i < dims.size(); i++) {
                sub_array_size *= dims[i];
            }
            
           
            
            // 保存原始维度信息
            std::vector<int> saved_dims = context.array_dims;
            
            // 处理每个顶层元素
            for (size_t i = 0; i < node.constInitVals.size() && !helper.isComplete(); i++) {
                
                
                if (node.constInitVals[i]->constExp) {
                    // 单值 - 作为新子数组的开始
                    
                    process_const_array_init_val(*node.constInitVals[i], helper);
                    
                    // 查找后续的单值来填充当前子数组
                    int filled = 1;
                    size_t j = i + 1;
                    while (j < node.constInitVals.size() && filled < sub_array_size && node.constInitVals[j]->constExp) {
                        
                        process_const_array_init_val(*node.constInitVals[j], helper);
                        filled++;
                        j++;
                    }
                    i = j - 1;
                    
                    // 填充剩余位置到子数组边界
                    while (helper.getCurrentPosition() % sub_array_size != 0 && !helper.isComplete()) {
                        
                        context.const_array_init_vals.push_back({0, 0.0f});
                        helper.advance();
                    }
                } else {
                    // 嵌套列表 - 递归处理
                    
                    int pos_before = helper.getCurrentPosition();
                    
                    // 为嵌套列表设置正确的维度上下文
                    std::vector<int> nested_dims;
                    for (size_t d = 1; d < saved_dims.size(); d++) {
                        nested_dims.push_back(saved_dims[d]);
                    }
                    context.array_dims = nested_dims;
                    
                    process_const_array_init_val(*node.constInitVals[i], helper);
                    
                    // 恢复原始维度
                    context.array_dims = saved_dims;
                    
                    // 如果是空列表，需要填充整个子数组
                    if (helper.getCurrentPosition() == pos_before) {
                        // 空列表 - 填充整个子数组为0
                        for (int j = 0; j < sub_array_size && !helper.isComplete(); j++) {
                           
                            context.const_array_init_vals.push_back({0, 0.0f});
                            helper.advance();
                        }
                    } else {
                        // 非空列表 - 填充到子数组边界
                        while (helper.getCurrentPosition() % sub_array_size != 0 && !helper.isComplete()) {
                           
                            context.const_array_init_vals.push_back({0, 0.0f});
                            helper.advance();
                        }
                    }
                }
            }
        } else {
            // 普通列表处理
           
            for (auto &val : node.constInitVals) {
                if (helper.isComplete()) break;
                process_const_array_init_val(*val, helper);
            }
        }
    }
}

Value* CminusfBuilder::visit(ASTProgram &node) {
    VOID_T = module->get_void_type();
    INT1_T = module->get_int1_type();
    INT32_T = module->get_int32_type();
    INT32PTR_T = module->get_int32_ptr_type();
    FLOAT_T = module->get_float_type();
    FLOATPTR_T = module->get_float_ptr_type();

    for (auto &unit : node.compUnits) {
        unit->accept(*this);
    }
    
    return nullptr;
}

Value* CminusfBuilder::visit(ASTConstDecl &node) {
    auto saved_type = context.cur_type;
    context.cur_type = (node.bType == TYPE_INT) ? INT32_T : FLOAT_T;
    
    for (auto &def : node.constDefs) {
        def->accept(*this);
    }
    
    context.cur_type = saved_type;
    return nullptr;
}

Value* CminusfBuilder::visit(ASTConstDef &node) {
    Type *var_type = context.cur_type;
    
    if (node.dims.empty()) {
        // 简单常量
        if (node.constInitVal) {
            context.in_const_exp = true;
            
            // 重置 context.val 以确保每个常量都有独立的值
            context.val = {0, 0.0f};
            
            node.constInitVal->accept(*this);
            context.in_const_exp = false;
            
            // 创建全局常量
            Constant *initializer = nullptr;
            if (var_type == FLOAT_T) {
                // 对于浮点常量，需要检查初始值是否是整数
                if (context.cur_val && context.cur_val->get_type()->is_int32_type()) {
                    // 整数到浮点的隐式转换
                    if (auto *const_int = dynamic_cast<ConstantInt*>(context.cur_val)) {
                        context.val.f_val = static_cast<float>(const_int->get_value());
                        initializer = ConstantFP::get(context.val.f_val, module.get());
                    } else {
                        initializer = ConstantFP::get(context.val.f_val, module.get());
                    }
                } else {
                    initializer = ConstantFP::get(context.val.f_val, module.get());
                }
            } else {
                // 对于整数常量，需要检查初始值是否是浮点数
                if (context.cur_val && context.cur_val->get_type()->is_float_type()) {
                    // 浮点到整数的隐式转换
                    if (auto *const_fp = dynamic_cast<ConstantFP*>(context.cur_val)) {
                        context.val.i_val = static_cast<int>(const_fp->get_value());
                        initializer = ConstantInt::get(context.val.i_val, module.get());
                    } else {
                        initializer = ConstantInt::get(context.val.i_val, module.get());
                    }
                } else {
                    initializer = ConstantInt::get(context.val.i_val, module.get());
                }
            }
            
            auto *gv = GlobalVariable::create(node.ident, module.get(), var_type, true, initializer);
            scope.push(node.ident, gv);
            
            // 存储常量值供后续查找
            scope.push({node.ident, {}}, context.val);
        } else {
            // 没有初始值的常量，使用默认值0
            Constant *initializer = ConstantZero::get(var_type, module.get());
            auto *gv = GlobalVariable::create(node.ident, module.get(), var_type, true, initializer);
            scope.push(node.ident, gv);
            scope.push({node.ident, {}}, {0, 0.0f});
        }
    } else {
        // 数组常量处理部分保持不变
        Type *array_type = var_type;
        std::vector<int> dimensions;
        
        // 解析维度
        for (auto &dim : node.dims) {
            context.in_const_exp = true;
            dim->accept(*this);
            context.in_const_exp = false;
            
            if (auto *const_val = dynamic_cast<ConstantInt *>(context.cur_val)) {
                dimensions.push_back(const_val->get_value());
            } else {
                dimensions.push_back(1);
            }
        }

        // 反转维度
        std::reverse(dimensions.begin(), dimensions.end());
        
        // 构建数组类型
        for (int i = dimensions.size() - 1; i >= 0; i--) {
            array_type = module->get_array_type(array_type, dimensions[i]);
        }
        
        // 初始化数组
        if (node.constInitVal) {
            context.array_dims = dimensions;
            context.const_array_init_vals.clear();
            context.in_const_exp = true;
            
            // 重置常量值
            context.val = {0, 0.0f};
            
            ArrayInitHelper helper(dimensions, var_type);
            context.array_init_helper = &helper;
            
            process_const_array_init_val(*node.constInitVal, helper);
            
            // 填充剩余元素
            while (context.const_array_init_vals.size() < static_cast<size_t>(helper.getTotalSize())) {
                context.const_array_init_vals.push_back({0, 0.0f});
            }
            
            context.in_const_exp = false;
            context.array_init_helper = nullptr;
            
            auto initializer = get_const_array(array_type, context.const_array_init_vals);
            auto *gv = GlobalVariable::create(node.ident, module.get(), array_type, true, initializer);
            scope.push(node.ident, gv);
            
            // 存储常量值供后续查找
            for (size_t i = 0; i < context.const_array_init_vals.size(); i++) {
                std::vector<unsigned int> indices;
                size_t temp = i;
                for (int j = dimensions.size() - 1; j >= 0; j--) {
                    indices.insert(indices.begin(), static_cast<unsigned int>(temp % dimensions[j]));
                    temp /= dimensions[j];
                }
                scope.push({node.ident, indices}, context.const_array_init_vals[i]);
            }
        } else {
            Constant *initializer = ConstantZero::get(array_type, module.get());
            auto *gv = GlobalVariable::create(node.ident, module.get(), array_type, true, initializer);
            scope.push(node.ident, gv);
        }
    }
    return nullptr;
}
    


Value* CminusfBuilder::visit(ASTConstInitVal &node) {
    //实际处理在 process_const_array_init_val 中
    if (context.array_init_helper) {
        process_const_array_init_val(node, *context.array_init_helper);
    } else {
        // 非数组情况
        if (node.constExp) {
            node.constExp->accept(*this);
        }
    }
    return context.cur_val;
}

Value* CminusfBuilder::visit(ASTVarDecl &node) {
    auto saved_type = context.cur_type;
    context.cur_type = (node.bType == TYPE_INT) ? INT32_T : FLOAT_T;
    
    for (auto &def : node.varDefs) {
        def->accept(*this);
    }
    
    context.cur_type = saved_type;
    return nullptr;
}

Value* CminusfBuilder::visit(ASTVarDef &node) {
    Type *var_type = context.cur_type;
    
    if (node.dims.empty()) {
        // 简单变量处理（保持不变）
        if (scope.in_global()) {
            Constant *initializer = nullptr;
            if (node.initVal) {
                context.in_const_exp = true;
                node.initVal->accept(*this);
                context.in_const_exp = false;
                
                if (var_type == FLOAT_T) {
                    initializer = ConstantFP::get(context.val.f_val, module.get());
                } else {
                    initializer = ConstantInt::get(context.val.i_val, module.get());
                }
            } else {
                initializer = ConstantZero::get(var_type, module.get());
            }
            auto *gv = GlobalVariable::create(node.ident, module.get(), var_type, false, initializer);
            scope.push(node.ident, gv);
        } else {
            ensure_valid_insert_point();
            auto *alloca = builder->create_alloca(var_type);
            scope.push(node.ident, alloca);
            
            if (node.initVal) {
                bool old_in_const = context.in_const_exp;
                context.in_const_exp = false;
                node.initVal->accept(*this);
                context.in_const_exp = old_in_const;
                
                if (context.cur_val && can_insert_instruction()) {
                    context.cur_val = cast_value(context.cur_val, var_type);
                    if (context.cur_val) {
                        builder->create_store(context.cur_val, alloca);
                    }
                }
            }
        }
    } else {
        // 数组变量
        std::vector<int> dimensions;
        Type *array_type = var_type;
        
        // 解析维度
        for (auto &dim : node.dims) {
            context.in_const_exp = true;
            dim->accept(*this);
            context.in_const_exp = false;
            
            if (auto *const_val = dynamic_cast<ConstantInt *>(context.cur_val)) {
                dimensions.push_back(const_val->get_value());
            } else {
                dimensions.push_back(1);
            }
        }
        
        // 反转维度
        std::reverse(dimensions.begin(), dimensions.end());
        
        // 计算总大小
        int total_size = 1;
        for (int dim : dimensions) {
            total_size *= dim;
        }
        
        // 构建数组类型
        for (int i = dimensions.size() - 1; i >= 0; i--) {
            array_type = module->get_array_type(array_type, dimensions[i]);
        }
        
        if (scope.in_global()) {
            // 全局数组
            Constant *initializer = nullptr;
            
            if (node.initVal) {
                // 检查是否是简单的零初始化 {0}
                bool is_zero_init = false;
                if (node.initVal->initVals.size() == 1 && 
                    node.initVal->initVals[0]->exp != nullptr) {
                    // 检查是否是单个0
                    context.in_const_exp = true;
                    node.initVal->initVals[0]->exp->accept(*this);
                    context.in_const_exp = false;
                    
                    if (auto *const_int = dynamic_cast<ConstantInt*>(context.cur_val)) {
                        is_zero_init = (const_int->get_value() == 0);
                    } else if (auto *const_fp = dynamic_cast<ConstantFP*>(context.cur_val)) {
                        is_zero_init = (const_fp->get_value() == 0.0f);
                    }
                }
                
                if (is_zero_init || node.initVal->initVals.empty()) {
                    // 使用零初始化
                    initializer = ConstantZero::get(array_type, module.get());
                } else {
                    // 完整初始化
                    context.array_dims = dimensions;
                    context.const_array_init_vals.clear();
                    context.const_array_init_vals.reserve(total_size);  // 预分配空间
                    context.in_const_exp = true;
                    
                    ArrayInitHelper helper(dimensions, var_type);
                    context.array_init_helper = &helper;
                    
                    std::vector<Value*> dummy_vals;
                    process_array_init_val(*node.initVal, helper, dummy_vals, context.const_array_init_vals, true);
                    
                    while (context.const_array_init_vals.size() < static_cast<size_t>(total_size)) {
                        context.const_array_init_vals.push_back({0, 0.0f});
                    }
                    
                    context.in_const_exp = false;
                    context.array_init_helper = nullptr;
                    
                    initializer = get_const_array(array_type, context.const_array_init_vals);
                }
            } else {
                initializer = ConstantZero::get(array_type, module.get());
            }
            
            auto *gv = GlobalVariable::create(node.ident, module.get(), array_type, false, initializer);
            scope.push(node.ident, gv);
        } else {
            // 局部数组
            ensure_valid_insert_point();
            auto *alloca = builder->create_alloca(array_type);
            scope.push(node.ident, alloca);
            
            if (node.initVal) {
                // 检查是否是 {0} 初始化
                bool is_zero_init = false;
                bool is_empty_init = node.initVal->initVals.empty();
                
                if (!is_empty_init && node.initVal->initVals.size() == 1 && 
                    node.initVal->initVals[0]->exp != nullptr) {
                    context.in_const_exp = true;
                    node.initVal->initVals[0]->exp->accept(*this);
                    context.in_const_exp = false;
                    
                    if (auto *const_int = dynamic_cast<ConstantInt*>(context.cur_val)) {
                        is_zero_init = (const_int->get_value() == 0);
                    } else if (auto *const_fp = dynamic_cast<ConstantFP*>(context.cur_val)) {
                        is_zero_init = (const_fp->get_value() == 0.0f);
                    }
                }
                
                if (is_zero_init || is_empty_init) {
                    // 对于 {0} 或 {} 初始化，批量生成store指令
                    const int BATCH_SIZE = 16;  
                    
                    // 预先创建零值常量
                    Value *zero_val = (var_type == INT32_T) ? 
                        static_cast<Value*>(CONST_INT(0)) : 
                        static_cast<Value*>(CONST_FP(0.0f));
                    
                    // 批量处理，减少循环开销
                    for (int batch_start = 0; batch_start < total_size; batch_start += BATCH_SIZE) {
                        int batch_end = std::min(batch_start + BATCH_SIZE, total_size);
                        
                        for (int i = batch_start; i < batch_end; i++) {
                            if (can_insert_instruction()) {
                                std::vector<Value*> indices;
                                indices.reserve(dimensions.size() + 1);  // 预分配空间
                                indices.push_back(CONST_INT(0));
                                
                                size_t temp = i;
                                for (int j = dimensions.size() - 1; j >= 0; j--) {
                                    indices.insert(indices.begin() + 1, CONST_INT(static_cast<int>(temp % dimensions[j])));
                                    temp /= dimensions[j];
                                }
                                
                                auto *gep = builder->create_gep(alloca, indices);
                                builder->create_store(zero_val, gep);
                            }
                        }
                    }
                } else {
                    // 非零初始化
                    context.array_dims = dimensions;
                    context.array_init_vals.clear();
                    context.array_init_vals.reserve(total_size);  // 预分配空间
                    
                    bool old_in_const = context.in_const_exp;
                    context.in_const_exp = false;
                    
                    ArrayInitHelper helper(dimensions, var_type);
                    context.array_init_helper = &helper;
                    
                    std::vector<const_val> dummy_const_vals;
                    process_array_init_val(*node.initVal, helper, context.array_init_vals, dummy_const_vals, false);
                    
                    context.in_const_exp = old_in_const;
                    context.array_init_helper = nullptr;
                    
                    // 只初始化提供的值
                    size_t i = 0;
                    for (auto val : context.array_init_vals) {
                        if (can_insert_instruction() && i < static_cast<size_t>(total_size)) {
                            std::vector<Value*> indices;
                            indices.reserve(dimensions.size() + 1);  // 预分配空间
                            indices.push_back(CONST_INT(0));
                            
                            size_t temp = i;
                            for (int j = dimensions.size() - 1; j >= 0; j--) {
                                indices.insert(indices.begin() + 1, CONST_INT(static_cast<int>(temp % dimensions[j])));
                                temp /= dimensions[j];
                            }
                            
                            auto *gep = builder->create_gep(alloca, indices);
                            
                            if (!val) {
                                val = (var_type == INT32_T) ? 
                                    static_cast<Value*>(CONST_INT(0)) : 
                                    static_cast<Value*>(CONST_FP(0.0f));
                            } else {
                                val = cast_value(val, var_type);
                            }
                            
                            if (val && can_insert_instruction()) {
                                builder->create_store(val, gep);
                            }
                        }
                        i++;
                    }
                }
            }
            // 没有初始化器的局部数组保持未定义
        }
    }
    return nullptr;
}

Value* CminusfBuilder::visit(ASTInitVal &node) {
    // std::cerr << "[DEBUG InitVal] has_exp: " << (node.exp != nullptr)
    //           << ", num_initVals: " << node.initVals.size() << std::endl;
    
    // 实际处理在 process_array_init_val
    if (context.array_init_helper) {
        std::vector<Value*> dummy_vals;
        std::vector<const_val> dummy_const_vals;
        process_array_init_val(node, *context.array_init_helper, 
                             context.array_init_vals, context.const_array_init_vals, 
                             context.in_const_exp);
    } else {
        // 非数组情况
        if (node.exp) {
            node.exp->accept(*this);
        }
    }
    return context.cur_val;
}

Value* CminusfBuilder::visit(ASTFuncDef &node) {
    FunctionType *fun_type;
    Type *ret_type;
    std::vector<Type *> param_types;
    
    // 清空之前的数组参数维度信息
    array_param_dims.clear();
    
    // 返回类型
    if (node.funcType == TYPE_INT)
        ret_type = INT32_T;
    else if (node.funcType == TYPE_FLOAT)
        ret_type = FLOAT_T;
    else
        ret_type = VOID_T;

    // 参数类型
    for (size_t i = 0; i < node.params.size(); i++) {
        auto &param = node.params[i];
        
        if (param->bType == TYPE_INT) {
            if (param->isArray) {
                param_types.push_back(INT32PTR_T);
            } else {
                param_types.push_back(INT32_T);
            }
        } else {
            if (param->isArray) {
                param_types.push_back(FLOATPTR_T);
            } else {
                param_types.push_back(FLOAT_T);
            }
        }
    }

    fun_type = FunctionType::get(ret_type, param_types);
    auto func = Function::create(fun_type, node.ident, module.get());
    scope.push(node.ident, func);
    context.func = func;
    
    auto funBB = BasicBlock::create(module.get(), "entry", func);
    builder->set_insert_point(funBB);
    scope.enter();
    
    // 处理参数
    auto &args = func->get_args();
    auto arg_it = args.begin();
    for (unsigned i = 0; i < node.params.size(); ++i, ++arg_it) {
        auto param = node.params[i];
        Value *arg = &(*arg_it);
        
        // 添加调试输出
        // std::cerr << "[DEBUG FuncDef] Param " << param->ident 
        //           << ", isArray: " << param->isArray
        //           << ", dims.size: " << param->dims.size() << std::endl;
        
        if (param->isArray) {
            // 数组参数作为指针处理
            auto *alloca = builder->create_alloca(arg->get_type());
            builder->create_store(arg, alloca);
            scope.push(param->ident, alloca);
            
            // 保存数组参数的维度信息
            std::vector<int> dims;
            
            // 对于 int b[][59] 这样的参数，dims.size() == 1
            // 但它实际上是二维数组，第一维是空的
            if (param->dims.size() > 0) {
                // 添加一个标记表示第一维是空的（使用 -1）
                dims.push_back(-1);  // 表示第一维是空的
                
                // 处理其他维度
                for (auto& dim_exp : param->dims) {
                    if (dim_exp) {
                        // 计算维度表达式的值
                        context.in_const_exp = true;
                        dim_exp->accept(*this);
                        context.in_const_exp = false;
                        
                        if (auto *const_val = dynamic_cast<ConstantInt *>(context.cur_val)) {
                            dims.push_back(const_val->get_value());
                            // std::cerr << "[DEBUG FuncDef] Param " << param->ident 
                            //           << " dim value: " << const_val->get_value() << std::endl;
                        }
                    }
                }
            } else {
                // 一维数组参数，如 int d[]
                dims.push_back(-1);  // 表示维度未知
            }
            
            // 存储维度信息
            array_param_dims[param->ident] = dims;
            // std::cerr << "[DEBUG FuncDef] Stored dims for " << param->ident 
            //           << ", num_dims: " << dims.size() 
            //           << " (is_2d: " << (dims.size() >= 2) << ")" << std::endl;
        } else {
            // 非数组参数
            auto *alloca = builder->create_alloca(arg->get_type());
            builder->create_store(arg, alloca);
            scope.push(param->ident, alloca);
        }
    }
    
    // 处理函数体
    node.block->accept(*this);
    
    // 添加默认返回 - 只有在当前块未终止时才添加
    if (can_insert_instruction()) {
        if (context.func->get_return_type()->is_void_type())
            builder->create_void_ret();
        else if (context.func->get_return_type()->is_float_type())
            builder->create_ret(CONST_FP(0.));
        else
            builder->create_ret(CONST_INT(0));
    }
    
    scope.exit();
    return nullptr;
}

Value* CminusfBuilder::visit(ASTFuncFParam &node) {
    // 参数在FuncDef中处理
    return nullptr;
}



Value* CminusfBuilder::visit(ASTAssignStmt &node) {
    if (!can_insert_instruction()) return nullptr;
    
    // 计算右值
    node.exp->accept(*this);
    Value *rval = context.cur_val;
    
    // 获取左值地址
    context.require_lvalue = true;
    node.lVal->accept(*this);
    Value *lval_addr = context.cur_val;
    context.require_lvalue = false;
    
    // 检查空指针
    if (!rval || !lval_addr) {
        return nullptr;
    }
    
    if (!can_insert_instruction()) return nullptr;
    
    // 类型转换
    Type *lval_type = lval_addr->get_type()->get_pointer_element_type();
    rval = cast_value(rval, lval_type);
    
    // 存储
    if (rval && lval_addr && can_insert_instruction()) {
        builder->create_store(rval, lval_addr);
        
    //     // std::cerr << "[DEBUG AssignStmt] Stored value ";
    //     if (auto *const_val = dynamic_cast<ConstantInt*>(rval)) {
    //         std::cerr << const_val->get_value();
    //     } else if (auto *const_fp = dynamic_cast<ConstantFP*>(rval)) {
    //         std::cerr << const_fp->get_value();
    //     } else {
    //         std::cerr << "non-constant";
    //     }
    //     std::cerr << " to address" << std::endl;
     }
    return nullptr;
}

Value* CminusfBuilder::visit(ASTExpStmt &node) {
    if (!can_insert_instruction()) {
        return nullptr;
    }
    
    if (node.exp) {
        node.exp->accept(*this);
        
    }
    return nullptr;
}

Value* CminusfBuilder::visit(ASTBlockStmt &node) {
    if (!can_insert_instruction()) return nullptr;
    
    // BlockStmt 不需要创建新的作用域，因为其内部的 Block 会处理
    node.block->accept(*this);
    
    return nullptr;
}

Value* CminusfBuilder::visit(ASTBlock &node) {
    // Block 总是创建新的作用域
    scope.enter();
    
    for (auto &item : node.blockItems) {
        item->accept(*this);
    }
    
    scope.exit();
    return nullptr;
}

Value* CminusfBuilder::visit(ASTIfStmt &node) {
    if (!can_insert_instruction()) return nullptr;
    
    // 创建唯一的基本块标签
    static int if_counter = 0;
    int if_id = if_counter++;
    std::string suffix = "_" + std::to_string(if_id);
    
    // 创建基本块
    auto *trueBB = BasicBlock::create(module.get(), "if_true" + suffix, context.func);
    auto *falseBB = node.elseStmt ? 
        BasicBlock::create(module.get(), "if_false" + suffix, context.func) : nullptr;
    auto *endBB = BasicBlock::create(module.get(), "if_end" + suffix, context.func);
    
    // 计算条件 - 设置 in_cond 标志
    bool saved_in_cond = context.in_cond;
    context.in_cond = true;
    node.cond->accept(*this);
    context.in_cond = saved_in_cond;
    
    Value *cond_val = context.cur_val;
    
    if (!can_insert_instruction()) return nullptr;
    
    // 确保条件是 i1 类型
    if (!cond_val) {
        cond_val = ConstantInt::get(false, module.get());
    } else if (cond_val->get_type()->is_int32_type()) {
        cond_val = builder->create_icmp_ne(cond_val, CONST_INT(0));
    } else if (cond_val->get_type()->is_float_type()) {
        cond_val = builder->create_fcmp_ne(cond_val, CONST_FP(0.));
    } else if (!cond_val->get_type()->is_int1_type()) {
        cond_val = ConstantInt::get(false, module.get());
    }
    
    // 生成条件跳转
    if (node.elseStmt) {
        builder->create_cond_br(cond_val, trueBB, falseBB);
    } else {
        builder->create_cond_br(cond_val, trueBB, endBB);
    }
    
    // then分支
    builder->set_insert_point(trueBB);
    
    node.thenStmt->accept(*this);
    
    if (can_insert_instruction()) {
        builder->create_br(endBB);
    }
    
    // else分支
    if (node.elseStmt) {
        builder->set_insert_point(falseBB);
        node.elseStmt->accept(*this);
        if (can_insert_instruction()) {
            builder->create_br(endBB);
        }
    }
    
    // 继续后续代码
    builder->set_insert_point(endBB);
    
    return nullptr;
}

Value* CminusfBuilder::visit(ASTWhileStmt &node) {
    if (!can_insert_instruction()) return nullptr;
    
    // 创建唯一的基本块标签
    static int while_counter = 0;
    int while_id = while_counter++;
    std::string suffix = "_" + std::to_string(while_id);
    
    // 创建基本块
    auto *condBB = BasicBlock::create(module.get(), "while_cond" + suffix, context.func);
    auto *bodyBB = BasicBlock::create(module.get(), "while_body" + suffix, context.func);
    auto *endBB = BasicBlock::create(module.get(), "while_end" + suffix, context.func);
    
    // 保存循环上下文
    auto saved_cond = context.loop_cond;
    auto saved_body = context.loop_body;
    auto saved_end = context.loop_end;
    
    context.loop_cond = condBB;
    context.loop_body = bodyBB;
    context.loop_end = endBB;
    
    // 跳转到条件块
    builder->create_br(condBB);
    
    // 条件块
    builder->set_insert_point(condBB);
    
    // 计算条件表达式
    bool saved_in_cond = context.in_cond;
    context.in_cond = true;
    node.cond->accept(*this);
    context.in_cond = saved_in_cond;
    
    Value *cond_val = context.cur_val;
    
    // 转换条件为i1类型
    if (cond_val->get_type()->is_int32_type()) {
        cond_val = builder->create_icmp_ne(cond_val, CONST_INT(0));
    } else if (cond_val->get_type()->is_float_type()) {
        cond_val = builder->create_fcmp_ne(cond_val, CONST_FP(0.));
    }
    
    builder->create_cond_br(cond_val, bodyBB, endBB);
    
    // 循环体 - 关键：确保在bodyBB中执行
    builder->set_insert_point(bodyBB);
    
    // 访问循环体语句
    node.stmt->accept(*this);
    
    if (can_insert_instruction()) {
        builder->create_br(condBB);
    }
    
    // 恢复上下文
    context.loop_cond = saved_cond;
    context.loop_body = saved_body;
    context.loop_end = saved_end;
    
    // 继续后续代码
    builder->set_insert_point(endBB);
    return nullptr;
}

Value* CminusfBuilder::visit(ASTBreakStmt &node) {
    
    
    if (!can_insert_instruction()) {
        
        return nullptr;
    }
    
    // 确保我们在循环内部
    if (context.loop_end) {
        // 创建跳转到循环结束的指令
        
        builder->create_br(context.loop_end);
    } else {
        // 如果不在循环内，break 语句是无效的
       
    }
    return nullptr;
}

Value* CminusfBuilder::visit(ASTContinueStmt &node) {
    if (!can_insert_instruction()) return nullptr;
    
    if (context.loop_cond) {
        builder->create_br(context.loop_cond);
    } else {
        std::cerr << "[WARNING] continue statement outside of loop\n";
    }
    return nullptr;
}

Value* CminusfBuilder::visit(ASTReturnStmt &node) {
    if (!can_insert_instruction()) return nullptr;
    
    if (node.exp) {
        node.exp->accept(*this);
        Value *ret_val = context.cur_val;
        
        if (!ret_val) {
            // 如果表达式求值失败，返回默认值
            if (context.func->get_return_type()->is_int32_type()) {
                ret_val = CONST_INT(0);
            } else if (context.func->get_return_type()->is_float_type()) {
                ret_val = CONST_FP(0.0);
            } else {
                builder->create_void_ret();
                return nullptr;
            }
        }
        
        // 类型转换
        ret_val = cast_value(ret_val, context.func->get_return_type());
        
        if (ret_val && can_insert_instruction()) {
            builder->create_ret(ret_val);
        }
    } else {
        builder->create_void_ret();
    }
    
    return nullptr;
}

Value* CminusfBuilder::visit(ASTExp &node) {
    node.addExp->accept(*this);
    return context.cur_val;
}

Value* CminusfBuilder::visit(ASTCond &node) {
    node.lOrExp->accept(*this);
    return context.cur_val;
}

Value* CminusfBuilder::visit(ASTLVal &node) {
    // 查找变量
    Value *var = scope.find(node.ident);
    if (!var) {
        context.cur_val = nullptr;
        return nullptr;
    }
    
    // std::cerr << "[DEBUG LVal] Variable: " << node.ident 
    //           << ", indices.size: " << node.indices.size()
    //           << ", require_lvalue: " << context.require_lvalue << std::endl;
    
    // 检查变量是否是全局常量
    if (auto *gv = dynamic_cast<GlobalVariable *>(var)) {
        if (gv->is_const() && node.indices.empty() && !context.require_lvalue) {
            // 对于简单的全局常量，直接返回其初始值
            if (auto *init = gv->get_init()) {
                context.cur_val = init;
                if (auto *const_int = dynamic_cast<ConstantInt *>(init)) {
                    context.val.i_val = const_int->get_value();
                } else if (auto *const_fp = dynamic_cast<ConstantFP *>(init)) {
                    context.val.f_val = const_fp->get_value();
                }
                return context.cur_val;
            }
        }
    }
    
    if (node.indices.empty()) {
        // 简单变量或数组名
        if (context.require_lvalue) {
            // 返回地址
            context.cur_val = var;
        } else {
            // 返回值
            Type *var_type = var->get_type();
            
            if (var_type->is_pointer_type()) {
                Type *elem_type = var_type->get_pointer_element_type();
                
                if (elem_type->is_array_type()) {
                    // 指向数组的指针，展平
                    if (can_insert_instruction()) {
                        Type *current_type = elem_type;
                        std::vector<Value*> indices = {CONST_INT(0)};
                        
                        // 对所有数组（包括二维数组），展平到基本元素类型
                        while (current_type->is_array_type()) {
                            indices.push_back(CONST_INT(0));
                            current_type = static_cast<ArrayType*>(current_type)->get_element_type();
                        }
                        
                        auto *gep = builder->create_gep(var, indices);
                        context.cur_val = gep;
                    } else {
                        context.cur_val = nullptr;
                    }
                } else if (elem_type->is_pointer_type()) {
                    // 指向指针的指针（数组参数）
                    if (can_insert_instruction()) {
                        context.cur_val = builder->create_load(var);
                    } else {
                        context.cur_val = nullptr;
                    }
                } else {
                    // 普通变量，需要加载
                    if (can_insert_instruction()) {
                        context.cur_val = builder->create_load(var);
                    } else {
                        context.cur_val = nullptr;
                    }
                }
            } else if (var_type->is_int32_type() || var_type->is_float_type()) {
                // 常量或立即数
                context.cur_val = var;
            } else {
                // 未知类型
                context.cur_val = nullptr;
            }
        }
    } else {
        // 数组元素访问
        // std::cerr << "[DEBUG LVal] Array access for " << node.ident << std::endl;
        
        std::vector<Value *> indices;
        std::vector<int> index_values;  // 用于常量表达式
        
        // 判断是否是数组参数
        bool is_array_param = false;
        Type *var_type = var->get_type();
        
        // 检查是否是指向指针的指针（数组参数的特征）
        if (var_type->is_pointer_type()) {
            Type *pointed_type = var_type->get_pointer_element_type();
            if (pointed_type->is_pointer_type()) {
                // 这是一个数组参数（存储在 alloca 中的指针）
                is_array_param = true;
                // std::cerr << "[DEBUG LVal] This is an array parameter" << std::endl;
            }
        }
        
        // 首先收集所有索引值
        std::vector<Value*> collected_indices;
        std::vector<int> collected_values;
        
        for (size_t i = 0; i < node.indices.size(); i++) {
            auto &index = node.indices[i];
            // 处理索引时，require_lvalue 必须为 false
            bool saved_require_lvalue = context.require_lvalue;
            context.require_lvalue = false;
            index->accept(*this);
            context.require_lvalue = saved_require_lvalue;
            
            Value *idx = context.cur_val;
            
            // 收集常量值
            if (auto *const_idx = dynamic_cast<ConstantInt*>(idx)) {
                collected_values.push_back(const_idx->get_value());
            } else {
                collected_values.push_back(-1);
            }
            
            // 确保索引是i32整数类型
            if (!idx) {
                idx = CONST_INT(0);
            } else if (idx->get_type()->is_float_type()) {
                if (can_insert_instruction()) {
                    idx = builder->create_fptosi(idx, INT32_T);
                }
            } else if (idx->get_type()->is_int1_type()) {
                if (can_insert_instruction()) {
                    idx = builder->create_zext(idx, INT32_T);
                }
            } else if (!idx->get_type()->is_int32_type()) {
                idx = CONST_INT(0);
            }
            
            collected_indices.push_back(idx);
        }
        
        // 保存原始索引值用于常量查找
        std::vector<int> original_values = collected_values;
        
        if (is_array_param) {
            // std::cerr << "[DEBUG LVal] Processing array parameter access" << std::endl;
            
            // 检查是否有保存的维度信息
            auto dim_iter = array_param_dims.find(node.ident);
            if (dim_iter != array_param_dims.end()) {
                auto& saved_dims = dim_iter->second;
                // std::cerr << "[DEBUG LVal] Found saved dims for " << node.ident 
                //           << ", size: " << saved_dims.size() << std::endl;
                
                if (saved_dims.size() >= 2 && node.indices.size() == 2 && saved_dims[1] > 0) {
                    // 这是二维数组参数访问
                    int second_dim = saved_dims[1];
                    // std::cerr << "[DEBUG LVal] Processing 2D array param " << node.ident 
                    //           << " with second dim: " << second_dim << std::endl;
                    
                    if (can_insert_instruction()) {
                        // 先加载指针
                        var = builder->create_load(var);
                        
                        // 对于 b[a][index]，collected_indices[0] 是 a，collected_indices[1] 是 index
                        // 但从 IR 看，实际生成的是 index * 59 + a，说明顺序是反的
                        auto *idx0 = collected_indices[0];  
                        auto *idx1 = collected_indices[1];  
                        
                        // 交换索引顺序来修正问题
                        auto *mul = builder->create_imul(idx1, CONST_INT(second_dim));  // 第二个索引 * 59
                        auto *add = builder->create_iadd(mul, idx0);  // 结果 + 第一个索引
                        
                        // std::cerr << "[DEBUG LVal] Computing linear index: idx1 * " 
                        //           << second_dim << " + idx0" << std::endl;
                        
                        // 使用线性索引访问
                        auto *gep = builder->create_gep(var, {add});
                        
                        if (context.require_lvalue) {
                            context.cur_val = gep;
                        } else {
                            context.cur_val = builder->create_load(gep);
                        }
                    }
                } else {
                    // 一维数组参数或其他情况
                    if (can_insert_instruction()) {
                        var = builder->create_load(var);
                        indices = collected_indices;
                        auto *gep = builder->create_gep(var, indices);
                        
                        if (context.require_lvalue) {
                            context.cur_val = gep;
                        } else {
                            context.cur_val = builder->create_load(gep);
                        }
                    }
                }
            } else {
                // 没有保存的维度信息，按一维数组处理
                if (can_insert_instruction()) {
                    var = builder->create_load(var);
                    indices = collected_indices;
                    auto *gep = builder->create_gep(var, indices);
                    
                    if (context.require_lvalue) {
                        context.cur_val = gep;
                    } else {
                        context.cur_val = builder->create_load(gep);
                    }
                }
            }
        } else {
            // 局部或全局数组 - 需要先加0
            indices.push_back(CONST_INT(0));
            
            // 反转索引
            std::reverse(collected_indices.begin(), collected_indices.end());
            
            // 使用反转后的索引
            for (auto idx : collected_indices) {
                indices.push_back(idx);
            }
            
            // 创建GEP指令
            if (can_insert_instruction()) {
                auto *gep = builder->create_gep(var, indices);
                
                if (context.require_lvalue) {
                    context.cur_val = gep;
                } else {
                    Type *elem_type = gep->get_type()->get_pointer_element_type();
                    if (elem_type->is_array_type()) {
                        // 多维数组 - 返回数组地址，需要继续展平
                        Type *current_type = elem_type;
                        std::vector<Value*> extra_indices = {CONST_INT(0)};
                        
                        // 继续添加索引直到到达基本类型
                        while (current_type->is_array_type()) {
                            extra_indices.push_back(CONST_INT(0));
                            current_type = static_cast<ArrayType*>(current_type)->get_element_type();
                        }
                        
                        context.cur_val = builder->create_gep(gep, extra_indices);
                    } else {
                        // 加载值
                        context.cur_val = builder->create_load(gep);
                    }
                }
            } else {
                context.cur_val = nullptr;
            }
        }
        
        // 处理常量表达式的特殊情况
        if (context.in_const_exp) {
            // 特别处理常量数组元素的访问
            if (auto *gv = dynamic_cast<GlobalVariable *>(scope.find(node.ident))) {
                if (gv->is_const()) {
                    // 使用原始索引值（未反转的）来查找常量
                    std::vector<unsigned int> const_indices;
                    for (int val : original_values) {
                        if (val >= 0) {
                            const_indices.push_back(static_cast<unsigned int>(val));
                        } else {
                            const_indices.push_back(0);
                        }
                    }
                    
                    context.val = scope.find_const({node.ident, const_indices});
                    
                    // 设置当前值为常量
                    if (context.cur_type == INT32_T || var_type->get_pointer_element_type()->is_int32_type()) {
                        context.cur_val = CONST_INT(context.val.i_val);
                    } else {
                        context.cur_val = CONST_FP(context.val.f_val);
                    }
                }
            }
        }
    }
    
    return context.cur_val;
}

Value* CminusfBuilder::visit(ASTParenExp &node) {
    node.exp->accept(*this);
    return context.cur_val;
}

Value* CminusfBuilder::visit(ASTLValExp &node) {
    node.lVal->accept(*this);
    return context.cur_val;
}

Value* CminusfBuilder::visit(ASTNumber &node) {
    if (node.isInt) {
        context.val.i_val = node.intVal;
        context.cur_val = CONST_INT(node.intVal);
    } else {
        context.val.f_val = node.floatVal;
        context.cur_val = CONST_FP(node.floatVal);
        
        // 如果在常量表达式中且目标类型是整数，进行隐式转换
        if (context.in_const_exp && context.cur_type == INT32_T) {
            context.val.i_val = static_cast<int>(node.floatVal);
            context.cur_val = CONST_INT(context.val.i_val);
        }
    }
    return context.cur_val;
}

Value* CminusfBuilder::visit(ASTPrimaryUnaryExp &node) {
    node.primaryExp->accept(*this);
    return context.cur_val;
}

Value* CminusfBuilder::visit(ASTCallExp &node) {
    if (!can_insert_instruction()) {
        context.cur_val = nullptr;
        return nullptr;
    }
    
    Function *func = static_cast<Function *>(scope.find(node.ident));
    if (!func) {
        context.cur_val = nullptr;
        return nullptr;
    }
    
    std::vector<Value *> args;
    auto func_type = func->get_function_type();
    auto param_types = func_type->param_begin();
    
    if (node.funcRParams) {
        int arg_idx = 0;
        for (auto &exp : node.funcRParams->exps) {
            // 添加调试输出
            // std::cerr << "[DEBUG CallExp] Processing arg " << arg_idx 
            //           << " for function " << node.ident << std::endl;
            
            // 确保在处理参数时 require_lvalue 为 false
            bool saved_require_lvalue = context.require_lvalue;
            context.require_lvalue = false;
            
            exp->accept(*this);
            Value *arg = context.cur_val;
            
            // 调试输出
            // if (arg) {
            //     std::cerr << "[DEBUG CallExp] Arg " << arg_idx 
            //               << " type: " << arg->get_type()->print() << std::endl;
            // } else {
            //     std::cerr << "[DEBUG CallExp] Arg " << arg_idx << " is null!" << std::endl;
            // }
            
            // 恢复上下文
            context.require_lvalue = saved_require_lvalue;
            
            // 如果参数个数超过函数定义，跳过
            if (param_types == func_type->param_end()) {
                break;
            }
            
            Type *expected_type = *param_types;
            
            if (!arg) {
                // 如果参数求值失败，尝试特殊处理
                if (expected_type->is_pointer_type()) {
                    // 对于指针类型，尝试再次解析
                    if (auto *lval_exp = dynamic_cast<ASTLValExp*>(exp.get())) {
                        if (auto *lval = lval_exp->lVal.get()) {
                            // 直接查找并创建 GEP
                            if (auto *var = scope.find(lval->ident)) {
                                if (var->get_type()->is_pointer_type() && 
                                    var->get_type()->get_pointer_element_type()->is_array_type()) {
                                    // 创建 GEP 获取数组首地址
                                    Type *array_type = var->get_type()->get_pointer_element_type();
                                    std::vector<Value*> indices = {CONST_INT(0)};
                                    
                                    while (array_type->is_array_type()) {
                                        indices.push_back(CONST_INT(0));
                                        array_type = static_cast<ArrayType*>(array_type)->get_element_type();
                                    }
                                    
                                    arg = builder->create_gep(var, indices);
                                    // std::cerr << "[DEBUG CallExp] Created GEP for array parameter" << std::endl;
                                } else {
                                    arg = ConstantZero::get(expected_type, module.get());
                                }
                            } else {
                                arg = ConstantZero::get(expected_type, module.get());
                            }
                        } else {
                            arg = ConstantZero::get(expected_type, module.get());
                        }
                    } else {
                        arg = ConstantZero::get(expected_type, module.get());
                    }
                } else if (expected_type->is_int32_type()) {
                    arg = CONST_INT(0);
                } else if (expected_type->is_float_type()) {
                    arg = CONST_FP(0.0);
                } else {
                    arg = ConstantZero::get(expected_type, module.get());
                }
            } else {
                // 参数类型转换
                if (!expected_type->is_pointer_type() && !arg->get_type()->is_pointer_type()) {
                    // 非指针类型需要类型转换
                    if (arg->get_type() != expected_type) {
                        arg = cast_value(arg, expected_type);
                        if (!arg) {
                            // 转换失败，使用默认值
                            if (expected_type->is_int32_type()) {
                                arg = CONST_INT(0);
                            } else if (expected_type->is_float_type()) {
                                arg = CONST_FP(0.0);
                            }
                        }
                    }
                }
                // 对于指针类型，保持原样（已经在 LVal 中处理过了）
            }
            
            args.push_back(arg);
            ++param_types;
            arg_idx++;
        }
    }
    
    // 如果参数个数不足，补充默认值
    while (args.size() < func->get_num_of_args()) {
        if (param_types != func_type->param_end()) {
            Type *expected_type = *param_types;
            if (expected_type->is_int32_type()) {
                args.push_back(CONST_INT(0));
            } else if (expected_type->is_float_type()) {
                args.push_back(CONST_FP(0.0));
            } else if (expected_type->is_pointer_type()) {
                args.push_back(ConstantZero::get(expected_type, module.get()));
            }
            ++param_types;
        }
    }
    
    if (can_insert_instruction()) {
        context.cur_val = builder->create_call(func, args);
        // std::cerr << "[DEBUG CallExp] Function " << node.ident << " called successfully" << std::endl;
    } else {
        context.cur_val = nullptr;
    }
    return context.cur_val;
}

Value* CminusfBuilder::visit(ASTUnaryOpExp &node) {
    node.unaryExp->accept(*this);
    Value *val = context.cur_val;
    
    if (!val) {
        context.cur_val = CONST_INT(0);
        return context.cur_val;
    }
    
    if (!can_insert_instruction() && !context.in_const_exp) {
        context.cur_val = nullptr;
        return nullptr;
    }
    
    switch (node.unaryOp) {
        case UOP_PLUS:
            // +expr，不做任何操作
            break;
        case UOP_MINUS:
            // -expr
            if (context.in_const_exp) {
                if (val->get_type()->is_int32_type()) {
                    context.val.i_val = -context.val.i_val;
                    context.cur_val = CONST_INT(context.val.i_val);
                } else {
                    context.val.f_val = -context.val.f_val;
                    context.cur_val = CONST_FP(context.val.f_val);
                }
            } else {
                ensure_valid_insert_point();
                if (val->get_type()->is_int32_type()) {
                    context.cur_val = builder->create_isub(CONST_INT(0), val);
                } else {
                    context.cur_val = builder->create_fsub(CONST_FP(0.), val);
                }
            }
            break;
        case UOP_NOT:
            // !expr
            if (!context.in_const_exp) {
                ensure_valid_insert_point();
                if (val->get_type()->is_int32_type()) {
                    context.cur_val = builder->create_icmp_eq(val, CONST_INT(0));
                } else {
                    context.cur_val = builder->create_fcmp_eq(val, CONST_FP(0.));
                }
                if (can_insert_instruction()) {
                    context.cur_val = builder->create_zext(context.cur_val, INT32_T);
                }
            } else {
                // 常量表达式处理
                if (val->get_type()->is_int32_type()) {
                    context.val.i_val = (context.val.i_val == 0) ? 1 : 0;
                } else {
                    context.val.i_val = (context.val.f_val == 0.0f) ? 1 : 0;
                }
                context.cur_val = CONST_INT(context.val.i_val);
            }
            break;
    }
    return context.cur_val;
}

Value* CminusfBuilder::visit(ASTFuncRParams &node) {
    // 在CallExp中处理
    return nullptr;
}

Value* CminusfBuilder::visit(ASTMulExp &node) {
    if (!node.mulExp) {
        node.unaryExp->accept(*this);
        return context.cur_val;
    }
    
    const_val l_num, r_num;
    node.mulExp->accept(*this);
    Value *lhs = context.cur_val;
    l_num = context.val;
    
    node.unaryExp->accept(*this);
    Value *rhs = context.cur_val;
    r_num = context.val;
    
    // 处理空值情况
    if (!lhs) lhs = CONST_INT(0);
    if (!rhs) rhs = CONST_INT(0);
    
    // 在常量表达式中不需要检查插入点
    if (!context.in_const_exp && !can_insert_instruction()) {
        context.cur_val = nullptr;
        return nullptr;
    }
    
    bool is_int = promote(&lhs, &rhs, &l_num, &r_num);
    
    switch (node.mulOp) {
        case OP_MUL:
            if (context.in_const_exp) {
                if (is_int) {
                    context.val.i_val = l_num.i_val * r_num.i_val;
                    context.cur_val = CONST_INT(context.val.i_val);
                } else {
                    context.val.f_val = l_num.f_val * r_num.f_val;
                    context.cur_val = CONST_FP(context.val.f_val);
                }
            } else {
                ensure_valid_insert_point();
                if (is_int) {
                    context.cur_val = builder->create_imul(lhs, rhs);
                } else {
                    context.cur_val = builder->create_fmul(lhs, rhs);
                }
            }
            break;
        case OP_DIV:
            if (context.in_const_exp) {
                if (is_int) {
                    context.val.i_val = (r_num.i_val != 0) ? l_num.i_val / r_num.i_val : 0;
                    context.cur_val = CONST_INT(context.val.i_val);
                } else {
                    context.val.f_val = (r_num.f_val != 0.0f) ? l_num.f_val / r_num.f_val : 0.0f;
                    context.cur_val = CONST_FP(context.val.f_val);
                }
            } else {
                ensure_valid_insert_point();
                if (is_int) {
                    context.cur_val = builder->create_isdiv(lhs, rhs);
                } else {
                    context.cur_val = builder->create_fdiv(lhs, rhs);
                }
            }
            break;
        case OP_MOD:
            if (is_int) {
                if (context.in_const_exp) {
                    context.val.i_val = (r_num.i_val != 0) ? l_num.i_val % r_num.i_val : 0;
                    context.cur_val = CONST_INT(context.val.i_val);
                } else {
                    ensure_valid_insert_point();
                    // a % b = a - (a / b) * b
                    auto div = builder->create_isdiv(lhs, rhs);
                    auto mul = builder->create_imul(div, rhs);
                    context.cur_val = builder->create_isub(lhs, mul);
                }
            } else {
                // 浮点数取模，使用默认值
                if (context.in_const_exp) {
                    context.val.f_val = 0.0f;
                    context.cur_val = CONST_FP(0.0f);
                } else {
                    context.cur_val = CONST_FP(0.0f);
                }
            }
            break;
    }
    return context.cur_val;
}

Value* CminusfBuilder::visit(ASTAddExp &node) {
    if (!node.addExp) {
        node.mulExp->accept(*this);
        return context.cur_val;
    }
    
    const_val l_num, r_num;
    node.addExp->accept(*this);
    Value *lhs = context.cur_val;
    l_num = context.val;
    
    node.mulExp->accept(*this);
    Value *rhs = context.cur_val;
    r_num = context.val;
    
    // 处理空值情况
    if (!lhs) lhs = CONST_INT(0);
    if (!rhs) rhs = CONST_INT(0);
    
    if (!can_insert_instruction() && !context.in_const_exp) {
        context.cur_val = nullptr;
        return nullptr;
    }
    
    bool is_int = promote(&lhs, &rhs, &l_num, &r_num);
    
    if (node.addOp == OP_PLUS) {
        if (context.in_const_exp) {
            if (is_int) {
                context.val.i_val = l_num.i_val + r_num.i_val;
                context.cur_val = CONST_INT(context.val.i_val);
            } else {
                context.val.f_val = l_num.f_val + r_num.f_val;
                context.cur_val = CONST_FP(context.val.f_val);
            }
        } else {
            ensure_valid_insert_point();
            if (is_int) {
                context.cur_val = builder->create_iadd(lhs, rhs);
            } else {
                context.cur_val = builder->create_fadd(lhs, rhs);
            }
        }
    } else {
        if (context.in_const_exp) {
            if (is_int) {
                context.val.i_val = l_num.i_val - r_num.i_val;
                context.cur_val = CONST_INT(context.val.i_val);
            } else {
                context.val.f_val = l_num.f_val - r_num.f_val;
                context.cur_val = CONST_FP(context.val.f_val);
            }
        } else {
            ensure_valid_insert_point();
            if (is_int) {
                context.cur_val = builder->create_isub(lhs, rhs);
            } else {
                context.cur_val = builder->create_fsub(lhs, rhs);
            }
        }
    }
    return context.cur_val;
}

Value* CminusfBuilder::visit(ASTRelExp &node) {
    if (!node.relExp) {
        node.addExp->accept(*this);
        return context.cur_val;
    }
    
    const_val l_num, r_num;
    node.relExp->accept(*this);
    Value *lhs = context.cur_val;
    l_num = context.val;
    
    node.addExp->accept(*this);
    Value *rhs = context.cur_val;
    r_num = context.val;
    
    // 处理空值情况
    if (!lhs) lhs = CONST_INT(0);
    if (!rhs) rhs = CONST_INT(0);
    
    if (!can_insert_instruction() && !context.in_const_exp) {
        context.cur_val = nullptr;
        return nullptr;
    }
    
    bool is_int = promote(&lhs, &rhs, &l_num, &r_num);
    Value *cmp = nullptr;
    
    switch (node.relOp) {
        case OP_LT:
            if (context.in_const_exp) {
                context.val.i_val = is_int ? (l_num.i_val < r_num.i_val) : (l_num.f_val < r_num.f_val);
                cmp = CONST_INT(context.val.i_val);
            } else {
                ensure_valid_insert_point();
                cmp = is_int ? static_cast<Value*>(builder->create_icmp_lt(lhs, rhs)) : static_cast<Value*>(builder->create_fcmp_lt(lhs, rhs));
            }
            break;
        case OP_LE:
            if (context.in_const_exp) {
                context.val.i_val = is_int ? (l_num.i_val <= r_num.i_val) : (l_num.f_val <= r_num.f_val);
                cmp = CONST_INT(context.val.i_val);
            } else {
                ensure_valid_insert_point();
                cmp = is_int ? static_cast<Value*>(builder->create_icmp_le(lhs, rhs)) : static_cast<Value*>(builder->create_fcmp_le(lhs, rhs));
            }
            break;
        case OP_GT:
            if (context.in_const_exp) {
                context.val.i_val = is_int ? (l_num.i_val > r_num.i_val) : (l_num.f_val > r_num.f_val);
                cmp = CONST_INT(context.val.i_val);
            } else {
                ensure_valid_insert_point();
                cmp = is_int ? static_cast<Value*>(builder->create_icmp_gt(lhs, rhs)) : static_cast<Value*>(builder->create_fcmp_gt(lhs, rhs));
            }
            break;
        case OP_GE:
            if (context.in_const_exp) {
                context.val.i_val = is_int ? (l_num.i_val >= r_num.i_val) : (l_num.f_val >= r_num.f_val);
                cmp = CONST_INT(context.val.i_val);
            } else {
                ensure_valid_insert_point();
                cmp = is_int ? static_cast<Value*>(builder->create_icmp_ge(lhs, rhs)) : static_cast<Value*>(builder->create_fcmp_ge(lhs, rhs));
            }
            break;
        case OP_EQ:
        case OP_NE:
            //？
            cmp = CONST_INT(0);
            break;
    }
    
    if (!context.in_const_exp && cmp && cmp->get_type()->is_int1_type() && 
        can_insert_instruction() && !context.in_cond) {
        cmp = builder->create_zext(cmp, INT32_T);
    }
    
    context.cur_val = cmp;
    return context.cur_val;
}

Value* CminusfBuilder::visit(ASTEqExp &node) {
    if (!node.eqExp) {
        node.relExp->accept(*this);
        return context.cur_val;
    }
    
    const_val l_num, r_num;
    node.eqExp->accept(*this);
    Value *lhs = context.cur_val;
    l_num = context.val;
    
    node.relExp->accept(*this);
    Value *rhs = context.cur_val;
    r_num = context.val;
    
    // 处理空值情况
    if (!lhs) lhs = CONST_INT(0);
    if (!rhs) rhs = CONST_INT(0);
    
    if (!can_insert_instruction() && !context.in_const_exp) {
        context.cur_val = nullptr;
        return nullptr;
    }
    
    bool is_int = promote(&lhs, &rhs, &l_num, &r_num);
    Value *cmp = nullptr;
    
    if (node.eqOp == OP_EQ) {
        if (context.in_const_exp) {
            context.val.i_val = is_int ? (l_num.i_val == r_num.i_val) : (l_num.f_val == r_num.f_val);
            cmp = CONST_INT(context.val.i_val);
        } else {
            ensure_valid_insert_point();
            cmp = is_int ? static_cast<Value*>(builder->create_icmp_eq(lhs, rhs)) : static_cast<Value*>(builder->create_fcmp_eq(lhs, rhs));
        }
    } else {
        if (context.in_const_exp) {
            context.val.i_val = is_int ? (l_num.i_val != r_num.i_val) : (l_num.f_val != r_num.f_val);
            cmp = CONST_INT(context.val.i_val);
        } else {
            ensure_valid_insert_point();
            cmp = is_int ? static_cast<Value*>(builder->create_icmp_ne(lhs, rhs)) : static_cast<Value*>(builder->create_fcmp_ne(lhs, rhs));
        }
    }
    
    // 只在非条件上下文中才转换为 i32
    if (!context.in_const_exp && cmp && cmp->get_type()->is_int1_type() && 
        can_insert_instruction() && !context.in_cond) {
        cmp = builder->create_zext(cmp, INT32_T);
    }
    
    context.cur_val = cmp;
    return context.cur_val;
}

Value* CminusfBuilder::visit(ASTLOrExp &node) {
    if (!node.lOrExp) {
        node.lAndExp->accept(*this);
        return context.cur_val;
    }
    
    if (context.in_const_exp) {
        // 常量表达式求值（保持不变）
        node.lOrExp->accept(*this);
        int lhs_val = context.val.i_val;
        
        node.lAndExp->accept(*this);
        int rhs_val = context.val.i_val;
        
        context.val.i_val = (lhs_val != 0 || rhs_val != 0) ? 1 : 0;
        context.cur_val = CONST_INT(context.val.i_val);
    } else {
        if (!can_insert_instruction()) {
            context.cur_val = nullptr;
            return nullptr;
        }
        
        // 短路求值
        static int lor_counter = 0;
        int lor_id = lor_counter++;
        std::string suffix = "_" + std::to_string(lor_id);
        
        auto *rhsBB = BasicBlock::create(module.get(), "lor_rhs" + suffix, context.func);
        auto *endBB = BasicBlock::create(module.get(), "lor_end" + suffix, context.func);
        
        // 计算左操作数
        node.lOrExp->accept(*this);
        Value *lhs = context.cur_val;
        
        if (!lhs) lhs = CONST_INT(0);
        
        ensure_valid_insert_point();
        
        // 保存左操作数计算后的基本块
        auto *lhsEndBB = builder->get_insert_block();
        
        // 将左操作数转换为布尔值
        Value *lhs_bool = lhs;
        if (lhs->get_type()->is_int32_type()) {
            lhs_bool = builder->create_icmp_ne(lhs, CONST_INT(0));
        } else if (lhs->get_type()->is_float_type()) {
            lhs_bool = builder->create_fcmp_ne(lhs, CONST_FP(0.));
        }
        
        // 创建条件跳转
        builder->create_cond_br(lhs_bool, endBB, rhsBB);
        
        // 计算右操作数
        builder->set_insert_point(rhsBB);
        node.lAndExp->accept(*this);
        Value *rhs = context.cur_val;
        
        if (!rhs) rhs = CONST_INT(0);
        
        ensure_valid_insert_point();
        
        // 将右操作数转换为布尔值
        Value *rhs_bool = rhs;
        if (rhs->get_type()->is_int32_type()) {
            rhs_bool = builder->create_icmp_ne(rhs, CONST_INT(0));
        } else if (rhs->get_type()->is_float_type()) {
            rhs_bool = builder->create_fcmp_ne(rhs, CONST_FP(0.));
        }
        
        auto *rhsEndBB = builder->get_insert_block();
        builder->create_br(endBB);
        
        // 合并结果
        builder->set_insert_point(endBB);
        
        // 创建 phi 指令
        std::vector<Value*> phi_vals;
        std::vector<BasicBlock*> phi_bbs;
        
        phi_vals.push_back(ConstantInt::get(true, module.get()));
        phi_bbs.push_back(lhsEndBB);
        
        phi_vals.push_back(rhs_bool);
        phi_bbs.push_back(rhsEndBB);
        
        auto *phi = PhiInst::create_phi(INT1_T, endBB, phi_vals, phi_bbs);
        
        // 手动将phi指令添加到基本块
        endBB->add_instruction(phi);
        
        // 返回phi指令作为结果
        context.cur_val = phi;
    }
    
    return context.cur_val;
}

Value* CminusfBuilder::visit(ASTLAndExp &node) {
   if (!node.lAndExp) {
       node.eqExp->accept(*this);
       return context.cur_val;
   }
   
   if (context.in_const_exp) {
       // 常量表达式求值
       node.lAndExp->accept(*this);
       int lhs_val = context.val.i_val;
       
       node.eqExp->accept(*this);
       int rhs_val = context.val.i_val;
       
       context.val.i_val = (lhs_val != 0 && rhs_val != 0) ? 1 : 0;
       context.cur_val = CONST_INT(context.val.i_val);
   } else {
       if (!can_insert_instruction()) {
           context.cur_val = nullptr;
           return nullptr;
       }
       
       // 短路求值
       static int land_counter = 0;
       int land_id = land_counter++;
       std::string suffix = "_" + std::to_string(land_id);
       
       auto *rhsBB = BasicBlock::create(module.get(), "land_rhs" + suffix, context.func);
       auto *endBB = BasicBlock::create(module.get(), "land_end" + suffix, context.func);
       
       // 计算左操作数
       node.lAndExp->accept(*this);
       Value *lhs = context.cur_val;
       
       if (!lhs) lhs = CONST_INT(0);
       
       ensure_valid_insert_point();
       
       // 保存左操作数计算后的基本块
       auto *lhsEndBB = builder->get_insert_block();
       
       Value *lhs_bool = lhs;
       if (lhs->get_type()->is_int32_type()) {
           lhs_bool = builder->create_icmp_ne(lhs, CONST_INT(0));
       } else if (lhs->get_type()->is_float_type()) {
           lhs_bool = builder->create_fcmp_ne(lhs, CONST_FP(0.));
       }
       
       // 如果左边为假，直接跳到结束
       builder->create_cond_br(lhs_bool, rhsBB, endBB);
       
       // 计算右操作数
       builder->set_insert_point(rhsBB);
       node.eqExp->accept(*this);
       Value *rhs = context.cur_val;
       
       if (!rhs) rhs = CONST_INT(0);
       
       ensure_valid_insert_point();
       
       Value *rhs_bool = rhs;
       if (rhs->get_type()->is_int32_type()) {
           rhs_bool = builder->create_icmp_ne(rhs, CONST_INT(0));
       } else if (rhs->get_type()->is_float_type()) {
           rhs_bool = builder->create_fcmp_ne(rhs, CONST_FP(0.));
       }
       
       auto *rhsEndBB = builder->get_insert_block();
       builder->create_br(endBB);
       
       // 合并结果
       builder->set_insert_point(endBB);
       
       // 创建 phi 指令
       std::vector<Value*> phi_vals;
       std::vector<BasicBlock*> phi_bbs;
       
       phi_vals.push_back(ConstantInt::get(false, module.get()));
       phi_bbs.push_back(lhsEndBB);
       
       phi_vals.push_back(rhs_bool);
       phi_bbs.push_back(rhsEndBB);
       
       auto *phi = PhiInst::create_phi(INT1_T, endBB, phi_vals, phi_bbs);
       
       // 手动将phi指令添加到基本块
       endBB->add_instruction(phi);
       
       // 返回phi指令作为结果
       context.cur_val = phi;
   }
   
   return context.cur_val;
}


Value* CminusfBuilder::visit(ASTConstExp &node) {
   context.in_const_exp = true;
   node.addExp->accept(*this);
   context.in_const_exp = false;
   return context.cur_val;
}