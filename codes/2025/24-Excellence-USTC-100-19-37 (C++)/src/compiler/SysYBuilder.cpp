#include <iostream>
#include <cassert>
#include <cstddef>
#include <vector>

#include "SysYBuilder.hpp"
#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "Function.hpp"
#include "GlobalVariable.hpp"
#include "Instruction.hpp"
#include "Type.hpp"
#include "Value.hpp"
#include "ast.hpp"

#define CONST_FP(num)   ConstantFP::get((float)num, module.get())
#define CONST_INT(num)  ConstantInt::get(num, module.get())

#define COND_BB()   (BasicBlock::create(module.get(), context.func->get_name() + "_condBB_" + std::to_string(context.cond_count++), context.func))
#define TRUE_BB()   (BasicBlock::create(module.get(), context.func->get_name() + "_trueBB_" + std::to_string(context.true_count++), context.func))
#define FALSE_BB()  (BasicBlock::create(module.get(), context.func->get_name() + "_falseBB_" + std::to_string(context.false_count++), context.func))
#define NEXT_BB()   (BasicBlock::create(module.get(), context.func->get_name() + "_nextBB_" + std::to_string(context.next_count++), context.func))
#define ELSE_BB()   (BasicBlock::create(module.get(), context.func->get_name() + "_elseBB_" + std::to_string(context.else_count++), context.func))

// #define DEBUG
#ifdef DEBUG
    #define _DEBUG_(string)                                                         \
        std::cout << "[SysYBuilder] DEBUG: " << string << std::endl << std::flush;
#else
    #define _DEBUG_(string)
#endif

#define _ERROR_(string)                                                             \
    std::cerr << "[SysYBuilder] ERROR: " << string << std::endl;                    \
    std::abort();

// types
Type * VOID_T;
Type * INT1_T;
Type * INT32_T;
Type * INT32PTR_T;
Type * FLOAT_T;
Type * FLOATPTR_T;

/*
 * use CMinusfBuilder::Scope to construct scopes
 * scope.enter: enter a new scope
 * scope.exit: exit current scope
 * scope.push: add a new binding to current scope
 * scope.find: find and return the value bound to the name
 */

Value *SysYBuilder::visit(ASTProgram &node) {
    _DEBUG_("Visiting ASTProgram");
    VOID_T = module->get_void_type();
    INT1_T = module->get_int1_type();
    INT32_T = module->get_int32_type();
    INT32PTR_T = module->get_int32_ptr_type();
    FLOAT_T = module->get_float_type();
    FLOATPTR_T = module->get_float_ptr_type();

    for (auto & comp_unit : node.comp_units) {
        comp_unit->accept(*this);
    }
    return nullptr;
}

Value *SysYBuilder::visit(ASTAssignStmt &node) {
    if (builder->get_insert_block()->is_terminated()) {
        return nullptr;
    }

    node.exp->accept(*this);
    auto exp = context.val;
    node.lval->accept(*this);
    auto lval = context.val;

    auto lval_type = lval->get_type()->get_pointer_element_type();
    auto exp_type = exp->get_type();
    if (lval_type->is_float_type() && exp_type->is_integer_type()) {
        exp = builder->create_sitofp(exp, FLOAT_T);
    } else if (lval_type->is_integer_type() && exp_type->is_float_type()) {
        exp = builder->create_fptosi(exp, INT32_T);
    }

    builder->create_store(exp, lval);
    return nullptr;
}

Value *SysYBuilder::visit(ASTBlockItem &node) {
    if (builder->get_insert_block()->is_terminated()) {
        context.val = nullptr;
        return context.val;
    }
    if (node.is_decl) {
        auto decl = std::get<std::shared_ptr<ASTDecl>>(node.item);
        decl->accept(*this);
    } else {
        auto stmt = std::get<std::shared_ptr<ASTStmt>>(node.item);
        stmt->accept(*this);
    }
    return context.val;
}

Value *SysYBuilder::visit(ASTBlockStmt &node) {
    if (builder->get_insert_block()->is_terminated()) {
        return nullptr;
    }

    scope.enter();
    for (auto block_item : node.block_items) {
        block_item->accept(*this);
        if (builder->get_insert_block()->is_terminated()) {
            break;
        }
    }
    scope.exit();
    
    return nullptr;
}

// Value *SysYBuilder::visit(ASTBreakStmt &node) {
//     if (builder->get_insert_block()->is_terminated()) {
//         return nullptr;
//     }

//     builder->create_br(context.while_info.back().second);
//     return nullptr;
// }

Value *SysYBuilder::visit(ASTBreakStmt &node) {
    if (builder->get_insert_block()->is_terminated()) {
        return nullptr;
    }

    builder->create_br(context.while_next_bbs.top());
    return nullptr;
}

// Value *SysYBuilder::visit(ASTContinueStmt &node) {
//     if (builder->get_insert_block()->is_terminated()) {
//         return nullptr;
//     }

//     builder->create_br(context.while_info.back().first);
//     return nullptr;
// }

Value *SysYBuilder::visit(ASTContinueStmt &node) {
    if (builder->get_insert_block()->is_terminated()) {
        return nullptr;
    }

    builder->create_br(context.beginwhile.top());
    return nullptr;
}

Value *SysYBuilder::visit(ASTDecl &node) {
    context.is_const = node.is_const;
    context.type = node.type;
    for (auto &def : node.defs) {
        def->accept(*this);
    }

    return nullptr;
}

Value *SysYBuilder::visit(ASTDef &node) {
    // 常量必须初始化
    if (node.is_const && node.init_val == nullptr) {
        assert(false && "Constant variable must be initialized");
    }
    // 确定类型
    Type *type = nullptr;
    if (node.type == SysYType::TYPE_INT) {
        type = INT32_T;
    } else if (node.type == SysYType::TYPE_FLOAT) {
        type = FLOAT_T;
    } else {
        assert(false && "Invalid type for SysY2022 variable define");
    }
    // 声明信息收集
    std::vector<int> dim_sizes;
    if (node.dims.empty()) {
        context.init_val_counts = {1};
    } else {
        for (auto &dim : node.dims) {
            auto exp = dim->accept(*this);
            assert(dynamic_cast<ConstantInt *>(exp) &&
                "Dim size must be const int");
            auto exp_int = dynamic_cast<ConstantInt *>(exp);
            assert(exp_int->get_value() > 0 &&
                "Dim size must be positive const int");
            dim_sizes.push_back(exp_int->get_value());
        }
        context.init_val_counts = dim_sizes;
        for (int i = dim_sizes.size()-2; i >= 0; i--) {
            context.init_val_counts[i] *= context.init_val_counts[i+1];
        }
    }
    // 处理初始化数组
    context.is_all_zero = true;
    if (node.init_val != nullptr) {
        node.init_val->type = node.type;
        node.init_val->accept(*this);
    }
    // 处理声明
    if (node.dims.empty()) { // 单值
        if (scope.in_global()) { // 全局
            if (context.is_all_zero) { // 零初始化
                if (node.is_const) { // 常量
                    if (type->is_int32_type()) {
                        scope.push(node.id, CONST_INT(0));
                    } else {
                        scope.push(node.id, CONST_FP(0.0));
                    }
                } else { // 变量
                    auto zero = ConstantZero::get(type, module.get());
                    auto global_variable =
                        GlobalVariable::create(node.id, module.get(), type, node.is_const, zero);
                    scope.push(node.id, global_variable);
                }
            } else { // 非零初始化
                if (node.is_const) { // 常量
                    scope.push(node.id, context.init_vals[0]);
                } else { // 变量
                    auto init_val = static_cast<Constant *>(context.init_vals[0]);
                    auto global_variable =
                        GlobalVariable::create(node.id, module.get(), type, node.is_const, init_val);
                    scope.push(node.id, global_variable);
                }
            }
        } else { // 局部
            if (node.init_val == nullptr) { // 无初始化(变量)
                auto cur_bb = builder->get_insert_block();
                builder->set_insert_point(context.func->get_entry_block());
                auto variable = builder->create_alloca(type);
                builder->set_insert_point(cur_bb);
                scope.push(node.id, variable);
            } else if (context.is_all_zero) { // 零初始化
                if (node.is_const) { // 常量
                    if (type->is_int32_type()) {
                        scope.push(node.id, CONST_INT(0));
                    } else {
                        scope.push(node.id, CONST_FP(0.0));
                    }
                } else { // 变量
                    auto cur_bb = builder->get_insert_block();
                    builder->set_insert_point(context.func->get_entry_block());
                    auto variable = builder->create_alloca(type);
                    builder->set_insert_point(cur_bb);
                    if (type->is_int32_type()) {
                        builder->create_store(CONST_INT(0), variable);
                    } else {
                        builder->create_store(CONST_FP(0.0), variable);
                    }
                    scope.push(node.id, variable);
                }
            } else { // 非零初始化
                if (node.is_const) { // 常量
                    scope.push(node.id, context.init_vals[0]);
                } else { // 变量
                    auto cur_bb = builder->get_insert_block();
                    builder->set_insert_point(context.func->get_entry_block());
                    auto variable = builder->create_alloca(type);
                    builder->set_insert_point(cur_bb);
                    builder->create_store(context.init_vals[0], variable);
                    scope.push(node.id, variable);
                }
            }
        }
    } else { // 数组
        std::vector<ArrayType *> array_types;
        array_types.push_back(ArrayType::get(type, dim_sizes.back()));
        for (int i = dim_sizes.size()-2; i >= 0; i--) {
            array_types.push_back(ArrayType::get(array_types.back(), dim_sizes[i]));
        }
        if (scope.in_global()) { // 全局
            if (context.is_all_zero) { // 零初始化
                auto zero = ConstantZero::get(array_types.back(), module.get());
                auto global_variable =
                    GlobalVariable::create(node.id, module.get(), array_types.back(), node.is_const, zero);
                scope.push(node.id, global_variable);
            } else { // 非零初始化
                std::vector<Constant *> init_vals;
                std::vector<int> indices(dim_sizes.size(), 0);
                for (auto &init_val : context.init_vals) {
                    init_vals.push_back(static_cast<Constant *>(init_val));
                    // 压入常量
                    if (node.is_const) {
                        scope.push(node.id, indices, init_vals.back());
                    }
                    // 更新索引
                    indices.back()++;
                    for (int i = dim_sizes.size()-1; i > 0; i--) {
                        if (indices[i] < dim_sizes[i]) {
                            break;
                        }
                        if (indices[i] == dim_sizes[i]) {
                            indices[i] = 0;
                            indices[i-1]++;
                        }
                    }
                }
                for (unsigned i = 0; i < dim_sizes.size(); i++) {
                    int group = init_vals.size() / dim_sizes[dim_sizes.size()-1-i];
                    for (int j = 0; j < group; j++) {
                        std::vector<Constant *> array;
                        for (int k = 0; k < dim_sizes[dim_sizes.size()-1-i]; k++) {
                            array.push_back(init_vals.front());
                            init_vals.erase(init_vals.begin());
                        }
                        init_vals.push_back(ConstantArray::get(array_types[i], array));
                    }
                }
                auto global_variable =
                    GlobalVariable::create(node.id, module.get(), array_types.back(), node.is_const, init_vals[0]);
                scope.push(node.id, global_variable);
            }
        } else { // 局部
            auto variable = builder->create_alloca(array_types.back());
            if (node.init_val != nullptr && context.is_all_zero) { // 零初始化
                auto memset = scope.find("memset").second;
                auto ptr = builder->create_bitcast(variable, module->get_int8_ptr_type());
                long long size = variable->get_alloca_type()->get_size();
                builder->create_call(memset, {ptr, CONST_INT(0), CONST_INT(size)});
                if (node.is_const) { // 常量
                    for (unsigned i = 0; i < context.init_val_counts[0]; i++) {
                        std::vector<int> indices(dim_sizes.size(), 0);
                        if (type->is_int32_type()) {
                            scope.push(node.id, indices, CONST_INT(0));
                        } else {
                            scope.push(node.id, indices, CONST_FP(0.0));
                        }
                        indices.back()++;
                        for (int i = dim_sizes.size()-1; i > 0; i--) {
                            if (indices[i] == dim_sizes[i]) {
                                indices[i] = 0;
                                indices[i-1]++;
                            }
                        }
                    }
                }
            } else { // 非零初始化
                std::vector<Value *> array_ptrs;
                std::vector<int> indices(dim_sizes.size(), 0);
                array_ptrs.push_back(variable);
                for (unsigned i = 0; i < dim_sizes.size(); i++) {
                    array_ptrs.push_back(builder->create_gep(array_ptrs.back(), {CONST_INT(0), CONST_INT(0)}));
                }
                for (auto init_val : context.init_vals) {
                    builder->create_store(init_val, array_ptrs.back());
                    if (node.is_const) {
                        scope.push(node.id, indices, static_cast<Constant *>(init_val));
                    }
                    indices.back()++;
                    int update_depth;
                    for (update_depth = dim_sizes.size()-1; update_depth >= 0;) {
                        if (indices[update_depth] < dim_sizes[update_depth]) {
                            break;
                        }
                        if (indices[update_depth] == dim_sizes[update_depth]) {
                            if (update_depth == 0) {
                                update_depth--;
                            } else {
                                indices[update_depth] = 0;
                                indices[update_depth-1]++;
                                update_depth--;
                            }
                        }
                    }
                    if (update_depth >= 0) {
                        for (int i = update_depth; i < dim_sizes.size(); i++) {
                            array_ptrs[i+1] =
                                builder->create_gep(array_ptrs[i], {CONST_INT(0), CONST_INT(indices[i])});
                        }
                    }
                }
            }
            scope.push(node.id, variable);
        }
    }

    context.init_vals.clear();
    return nullptr;
}

Value *SysYBuilder::visit(ASTFuncDef &node) {
    // 获取返回类型
    Type *ret_type = VOID_T;
    switch (node.type) {
        case TYPE_INT: ret_type = INT32_T; break;
        case TYPE_FLOAT: ret_type = FLOAT_T; break;
    }
    // 获取参数类型
    std::vector<Type *> param_types;
    for (auto &param : node.params) {
        auto type = VOID_T;
        switch (param->type) {
            case TYPE_INT: type = INT32_T; break;
            case TYPE_FLOAT: type = FLOAT_T; break;
        }
        if (param->is_array) { // 数组
            std::vector<int> array_sizes;
            for (auto &dim : param->dims) {
                auto dim_int = dynamic_cast<ConstantInt *>(dim->accept(*this))->get_value();
                array_sizes.push_back(dim_int);
            }
            for (auto size = array_sizes.rbegin(); size != array_sizes.rend(); size++) {
                type = ArrayType::get(type, *size);
            }
            type = PointerType::get(type);
        }
        param_types.push_back(type);
    }
    // 构造函数类型
    auto func_type = FunctionType::get(ret_type, param_types);
    auto func = Function::create(func_type, node.id, module.get());
    scope.push(node.id, func);
    context.func = func;
    // 创建 Entry 入口
    scope.enter();
    context.isFirstFuncBB = true;
    auto entry = BasicBlock::create(module.get(), node.id+"_entry", func);
    builder->set_insert_point(entry);
    // 参数入栈
    int location = 0;
    for (auto &arg : func->get_args()) { // std::list 不支持下标访问
        auto arg_addr = builder->create_alloca(arg.get_type());
        builder->create_store(&arg, arg_addr);
        scope.push(node.params[location]->id, arg_addr);
        location++;
    }
    for (auto &block_item : node.block_items) {
        block_item->accept(*this);
    }
    if (!builder->get_insert_block()->is_terminated()) {
        if (ret_type->is_int32_type()) {
            builder->create_ret(CONST_INT(0));
        } else if (ret_type->is_float_type()) {
            builder->create_ret(CONST_FP(0.0));
        } else {
            builder->create_void_ret();
        }
    }
    scope.exit();
    context.isFirstFuncBB = false;
    if (context.firstBrBB) {
        builder->set_insert_point(entry);
        builder->create_br(context.firstBrBB);
        context.firstBrBB = nullptr; // reset firstBrBB after use
    }

    return nullptr;
}

Value *SysYBuilder::visit(ASTFuncParam &node) {
    return nullptr;
}

Value *SysYBuilder::visit(ASTInitVal &node) {
    if (node.is_default) { // 括号深度不能超过上限
        assert(node.depth < context.init_val_counts.size() &&
            "Default initialization depth exceeds maximum depth");
    } else { // 最深括号的值的深度为上限 + 1
        assert(node.depth <= context.init_val_counts.size() &&
            "Initialization depth exceeds maximum depth");
    }

    if (node.is_default) { // 默认初始化
        if (node.depth == 0) {
            return nullptr;
        }
        auto size = context.init_val_counts[node.depth];
        if (node.type == TYPE_INT) {
            for (int i = 0; i < size; i++) {
                context.init_vals.push_back(CONST_INT(0));
            }
        } else if (node.type == TYPE_FLOAT) {
            for (int i = 0; i < size; i++) {
                context.init_vals.push_back(CONST_FP(0.0));
            }
        } else {
            assert(false && "Invalid type for initial value");
        }
    } else if (node.exp != nullptr) { // 初始化值
        auto exp = node.exp->accept(*this);
        if (node.is_const || scope.in_global()) {
            assert(dynamic_cast<Constant *>(exp) &&
                "Const or global initial value must be constant");
        }
        // 更新全零标记
        if (node.type == TYPE_INT) { // 整数类型
            if (dynamic_cast<ConstantInt *>(exp)) {
                if (dynamic_cast<ConstantInt *>(exp)->get_value() != 0) {
                    context.is_all_zero = false;
                }
            } else if (dynamic_cast<ConstantFP *>(exp)) {
                exp = CONST_INT(int(dynamic_cast<ConstantFP *>(exp)->get_value()));
                if (dynamic_cast<ConstantInt *>(exp)->get_value() != 0) {
                    context.is_all_zero = false;
                }
            } else if (exp->get_type()->is_float_type()) {
                exp = builder->create_fptosi(exp, INT32_T);
                context.is_all_zero = false;
            } else {
                context.is_all_zero = false;
            }
        } else { // 浮点数类型
            if (dynamic_cast<ConstantFP *>(exp)) {
                if (dynamic_cast<ConstantFP *>(exp)->get_value() != 0.0) {
                    context.is_all_zero = false;
                }
            } else if (dynamic_cast<ConstantInt *>(exp)) {
                exp = CONST_FP(dynamic_cast<ConstantInt *>(exp)->get_value());
                if (dynamic_cast<ConstantFP *>(exp)->get_value() != 0.0) {
                    context.is_all_zero = false;
                }
            } else if (exp->get_type()->is_integer_type()) {
                exp = builder->create_sitofp(exp, FLOAT_T);
                context.is_all_zero = false;
            } else {
                context.is_all_zero = false;
            }
        }
        // 压入初始化值
        context.init_vals.push_back(exp);
    } else { // 大括号
        assert(node.init_vals.size() > 0 && "bug in ASTInitVal");
        for (auto &init_val : node.init_vals) {
            init_val->is_const = node.is_const;
            init_val->type = node.type;
            init_val->depth = node.depth + 1;
            init_val->accept(*this);
        }
        auto init_vals_size = context.init_vals.size();
        auto end = context.init_val_counts.size();
        auto count = init_vals_size % context.init_val_counts[end-node.level];
        if (count != 0) {
            for (auto i = count; i < context.init_val_counts[end-node.level]; i++) {
                if (node.type == TYPE_INT) {
                    context.init_vals.push_back(CONST_INT(0));
                } else {
                    context.init_vals.push_back(CONST_FP(0.0));
                }
            }
        }
        if (node.depth == 0) {
            for (auto i = context.init_vals.size(); i < context.init_val_counts.front(); i++) {
                if (node.type == TYPE_INT) {
                    context.init_vals.push_back(CONST_INT(0));
                } else {
                    context.init_vals.push_back(CONST_FP(0.0));
                }
            }
        }
    }

    return nullptr;
}

// Value *SysYBuilder::visit(ASTIterationStmt &node) {
//     if (builder->get_insert_block()->is_terminated()) {
//         return nullptr;
//     }

//     auto cond = node.cond->accept(*this);
//     BasicBlock *true_bb = nullptr;
//     BasicBlock *false_bb = nullptr;
//     if (cond->get_type()->is_int32_type()) { // 整形
//         if (dynamic_cast<ConstantInt *>(cond)) { // 常量
//             if (static_cast<ConstantInt *>(cond)->get_value() != 0) {
//                 true_bb = TRUE_BB();
//                 false_bb = FALSE_BB();
//                 builder->create_br(true_bb);
//             } else {
//                 return nullptr;
//             }
//         } else { // 未知
//             true_bb = TRUE_BB();
//             false_bb = FALSE_BB();
//             cond = builder->create_icmp_ne(cond, CONST_INT(0));
//             builder->create_cond_br(cond, true_bb, false_bb);
//         }
//     } else { // 浮点型
//         if (dynamic_cast<ConstantFP *>(cond)) { // 常量
//             if (static_cast<ConstantFP *>(cond)->get_value() != 0.0) {
//                 true_bb = TRUE_BB();
//                 false_bb = FALSE_BB();
//                 builder->create_br(true_bb);
//             } else {
//                 return nullptr;
//             }
//         } else { // 未知
//             true_bb = TRUE_BB();
//             false_bb = FALSE_BB();
//             cond = builder->create_fcmp_ne(cond, CONST_FP(0.0));
//             builder->create_cond_br(cond, true_bb, false_bb);
//         }
//     }
//     context.while_info.push_back({true_bb, false_bb});
//     builder->set_insert_point(true_bb);
//     node.stmt->accept(*this);
//     std::cerr << "get here" << std::endl;
//     builder->create_br(true_bb);
//     auto next_bb = NEXT_BB();
//     builder->set_insert_point(false_bb);
//     builder->create_br(next_bb);
//     builder->set_insert_point(next_bb);
//     context.while_info.pop_back();

//     return nullptr;
// }

Value *SysYBuilder::visit(ASTIterationStmt &node) {
    if (builder->get_insert_block()->is_terminated()) {
        return nullptr;
    }

    context.islval = false;
    node.cond->accept(*this); // expression的值保存到了context.val
    std::vector<condList> list = context.list;
    context.list.clear();
    auto startwhile = list[0].myBB;
    context.beginwhile.push(startwhile);
    auto trueBB = TRUE_BB();
    auto next = NEXT_BB();
    context.while_next_bbs.push(next);
    
    builder->set_insert_point(trueBB);
    node.stmt->accept(*this);
    if(not builder->get_insert_block()->is_terminated()) {
        builder->create_br(startwhile);
    }

    for(auto i:list)
    {
        builder->set_insert_point(i.myBB);
        BasicBlock * tb;
        BasicBlock * fb;
        if(i.trueBB==nullptr) {
            tb=trueBB;
        }
        else {
            tb=i.trueBB;
        }
        if(i.falseBB==nullptr) {
            fb=next;
        }
        else{
            fb=i.falseBB;
        }
        auto condtype=i.val->get_type();
        Value *cond1;
        if(condtype->is_float_type())
        {
            cond1=builder->create_fcmp_ne(i.val, CONST_FP(0.0));
        }
        else
        {
            cond1=builder->create_icmp_ne(i.val, CONST_INT(0));
        }
        builder->create_cond_br(cond1, tb, fb);
    }
    builder->set_insert_point(next);
    context.beginwhile.pop();
    context.while_next_bbs.pop();
    return nullptr;
}

Value *SysYBuilder::visit(ASTReturnStmt &node) {
    if (builder->get_insert_block()->is_terminated()) {
        return nullptr;
    }
    if (node.exp == nullptr) {
        builder->create_void_ret();
    } else {
        auto exp = node.exp->accept(*this);
        if (context.func->get_return_type()->is_int32_type() && exp->get_type()->is_float_type()) {
            exp = builder->create_fptosi(exp, INT32_T);
        } else if (context.func->get_return_type()->is_float_type() && exp->get_type()->is_int32_type()) {
            exp = builder->create_sitofp(exp, FLOAT_T);
        } else if (context.func->get_return_type() != exp->get_type()) {
            assert(false && "Return value cannot convert to function type");
        }
        builder->create_ret(exp);
    }

    return nullptr;
}

// Value *SysYBuilder::visit(ASTSelectionStmt &node) {
//     if (builder->get_insert_block()->is_terminated()) {
//         return nullptr;
//     }

//     BasicBlock *true_bb = nullptr;
//     BasicBlock *false_bb = nullptr;
//     BasicBlock *else_bb = nullptr;
//     BasicBlock *next_bb = nullptr;
//     auto cond = node.cond->accept(*this);
//     if (cond->get_type()->is_int32_type()) { // 整型
//         if (dynamic_cast<ConstantInt *>(cond)) { // 常数
//             if (static_cast<ConstantInt *>(cond)->get_value() != 0) {
//                 node.if_stmt->accept(*this);
//             } else {
//                 node.else_stmt->accept(*this);
//             }
//         } else { // 未知
//             // 创建条件判断分支
//             true_bb = TRUE_BB();
//             false_bb = FALSE_BB();
//             cond = builder->create_icmp_ne(cond, CONST_INT(0));
//             builder->create_cond_br(cond, true_bb, false_bb);
//             // 翻译 if 块
//             builder->set_insert_point(true_bb);
//             node.if_stmt->accept(*this);
//             // 翻译 else 块
//             if (node.else_stmt != nullptr) {
//                 else_bb = ELSE_BB();
//                 builder->set_insert_point(false_bb);
//                 builder->create_br(else_bb);
//                 builder->set_insert_point(else_bb);
//                 node.else_stmt->accept(*this);
//             }
//             // 补全分支
//             next_bb = NEXT_BB();
//             if (!true_bb->is_terminated()) {
//                 builder->set_insert_point(true_bb);
//                 builder->create_br(next_bb);
//             }
//             if (node.else_stmt != nullptr && !false_bb->is_terminated()) {
//                 builder->set_insert_point(false_bb);
//                 builder->create_br(next_bb);
//             } else if (node.else_stmt == nullptr) {
//                 builder->set_insert_point(false_bb);
//                 builder->create_br(next_bb);
//             }
//             builder->set_insert_point(next_bb);
//         }
//     } else { // 浮点型
//         if (dynamic_cast<ConstantFP *>(cond)) { // 常数
//             if (static_cast<ConstantFP *>(cond)->get_value() != 0.0) {
//                 node.if_stmt->accept(*this);
//             } else {
//                 node.else_stmt->accept(*this);
//             }
//         } else { // 未知
//             // 创建条件判断分支
//             true_bb = TRUE_BB();
//             false_bb = FALSE_BB();
//             cond = builder->create_fcmp_ne(cond, CONST_FP(0.0));
//             builder->create_cond_br(cond, true_bb, false_bb);
//             // 翻译 if 块
//             builder->set_insert_point(true_bb);
//             node.if_stmt->accept(*this);
//             // 翻译 else 块
//             if (node.else_stmt != nullptr) {
//                 else_bb = ELSE_BB();
//                 builder->set_insert_point(false_bb);
//                 builder->create_br(else_bb);
//                 builder->set_insert_point(else_bb);
//                 node.else_stmt->accept(*this);
//             }
//             // 补全分支
//             next_bb = NEXT_BB();
//             if (!true_bb->is_terminated()) {
//                 builder->set_insert_point(true_bb);
//                 builder->create_br(next_bb);
//             }
//             if (node.else_stmt != nullptr && !false_bb->is_terminated()) {
//                 builder->set_insert_point(false_bb);
//                 builder->create_br(next_bb);
//             } else if (node.else_stmt != nullptr) {
//                 builder->set_insert_point(false_bb);
//                 builder->create_br(next_bb);
//             }
//             builder->set_insert_point(next_bb);
//         }
//     }

//     return nullptr;
// }

Value *SysYBuilder::visit(ASTSelectionStmt &node) {
    if (builder->get_insert_block()->is_terminated()) {
        return nullptr;
    }
    node.cond->accept(*this); // cond的值存入了context.val
    
    if (node.else_stmt==nullptr) { // 只有if没有else
        std::vector<condList> list=context.list;
        context.list.clear();
        auto trueBB = TRUE_BB();

        builder->set_insert_point(trueBB);
        if (node.if_stmt) {
            node.if_stmt->accept(*this);
        }
        auto trueBBExit = builder->get_insert_block();
        auto nextBB = NEXT_BB();
        for(auto cond:list)
        {
            builder->set_insert_point(cond.myBB);
            BasicBlock * tb;
            BasicBlock * fb;
            if(cond.trueBB==nullptr) {
                _DEBUG_("using trueBB");
                tb=trueBB;
            } else {
                _DEBUG_("using cond.trueBB");
                tb=cond.trueBB;
            }
            if(cond.falseBB==nullptr) {
                _DEBUG_("using nextBB");
                fb=nextBB;
            } else {
                _DEBUG_("using cond.falseBB");
                fb=cond.falseBB;
            }
            auto condType=cond.val->get_type();
            Value *cond1;
            if(condType->is_float_type()) {
                cond1=builder->create_fcmp_ne(cond.val, CONST_FP(0.0));
            } else {
                cond1=builder->create_icmp_ne(cond.val, CONST_INT(0));
            }
            _DEBUG_("Creating conditional branch: " + cond1->print() + " to " + tb->get_name() + " or " + fb->get_name());
            builder->create_cond_br(cond1, tb, fb);
        }

        builder->set_insert_point(trueBBExit);
        if (not builder->get_insert_block()->is_terminated()) {
            builder->create_br(nextBB);
        }

        builder->set_insert_point(nextBB);
    } else { //有 if 有 else
        std::vector<condList> list = context.list;
        context.list.clear();

        auto trueBB = TRUE_BB();
        builder->set_insert_point(trueBB);
        node.if_stmt->accept(*this);
        auto trueBBExit = builder->get_insert_block();

        auto falseBB = FALSE_BB();
        builder->set_insert_point(falseBB);
        node.else_stmt->accept(*this);
        auto falseBBExit = builder->get_insert_block();
        
        for (auto cond:list) {
            builder->set_insert_point(cond.myBB);
            BasicBlock * tb;
            BasicBlock * fb;
            if (cond.trueBB==nullptr) {
                tb=trueBB;
            } else {
                tb=cond.trueBB;
            }
            
            if(cond.falseBB==nullptr) {
                fb=falseBB;
            } else {
                fb=cond.falseBB;
            }
            auto condType=cond.val->get_type();
            Value *cond1;
            if(condType->is_float_type()) {
                cond1=builder->create_fcmp_ne(cond.val, CONST_FP(0.0));
            } else {
                cond1=builder->create_icmp_ne(cond.val, CONST_INT(0));
            }

            _DEBUG_("Creating conditional branch: " + cond1->print() + " to " + tb->get_name() + " or " + fb->get_name());
            builder->create_cond_br(cond1, tb, fb);
        }
        //context.list.clear();//清空以备下次使用
        
        if (trueBBExit->is_terminated() && falseBBExit->is_terminated()) {
            return nullptr; // 如果两个分支都已终止，则不需要创建新的分支
        }
        
        auto nextBB = NEXT_BB();
        builder->set_insert_point(trueBBExit);
        if(not builder->get_insert_block()->is_terminated()) {
            builder->create_br(nextBB);
        }
        builder->set_insert_point(falseBBExit);
        if(not builder->get_insert_block()->is_terminated()) {
            builder->create_br(nextBB);
        }
        builder->set_insert_point(nextBB);
    }
    return nullptr;
}

Value *SysYBuilder::visit(ASTExpStmt &node) {
    if (builder->get_insert_block()->is_terminated()) {
        return nullptr;
    }

    return node.exp->accept(*this);
}

/*******************************************************************************************/
Value *SysYBuilder::visit(ASTLOrExp &node) {
    _DEBUG_("Visiting ASTLOrExp");
    context.isfirstLandexp=true;
    if (node.and_exps.size() == 1) {
        node.and_exps[0]->accept(*this);
        
        return context.val;
    } else {
        int startBBIndex = 0;
        int nextBBIndex = 0;
        for (int i = 0; i < node.and_exps.size(); ++i) {
            node.and_exps[i]->accept(*this);
            if (i == 0) {
                
            } else {
                
                for (int j = startBBIndex; j < nextBBIndex; ++j) {
                    context.list[j].falseBB = context.list[nextBBIndex].myBB;
                }
            }
            startBBIndex = nextBBIndex;
            nextBBIndex = context.list.size();
        }
    }
    return context.val;
}
Value *SysYBuilder::visit(ASTLAndExp &node) {
    _DEBUG_("Visiting ASTLAndExp");
    auto eqBB = COND_BB();
    if(context.isfirstLandexp)//只有第一个landexp要跳进来
    {
        if(not builder->get_insert_block()->is_terminated()) {
            if (context.isFirstFuncBB) {
                context.firstBrBB = eqBB;
                context.isFirstFuncBB = false;
            } else {
                builder->create_br(eqBB);
            }
        }
            
        context.isfirstLandexp=false;
    }
    for (int i = 0; i < node.eq_exps.size(); ++i) {
        builder->set_insert_point(eqBB);
        node.eq_exps[i]->accept(*this);
        auto val = context.val;
        if (i == node.eq_exps.size() - 1) {
            context.list.push_back({val, eqBB, nullptr, nullptr});
        }
        else {
            auto next_eqBB = COND_BB();
            context.list.push_back({val, eqBB, next_eqBB, nullptr});
            eqBB = next_eqBB;
        } 
    }   
    return nullptr;
}
Value *SysYBuilder::visit(ASTEqExp &node) {
    _DEBUG_("Visiting ASTEqExp");
    // OP_EQ, OP_NEQ
    node.left->accept(*this);
    auto leftType = context.val->get_type();
    auto leftVal = context.val;
    if (node.right.empty()) {
        context.val = leftVal; // 如果没有右操作数，直接返回左操作数
        // TODO:
        if (dynamic_cast<ConstantFP *>(leftVal)) {
            context.val = CONST_INT(static_cast<int>(dynamic_cast<ConstantFP *>(leftVal)->get_value()!= 0.0));
        } else if (leftVal->get_type()->is_float_type()) {
            context.val = builder->create_fptosi(leftVal, INT32_T);
        }
        return nullptr;
    }
    for (const auto &pair : node.right) {
        pair.second->accept(*this);
        auto rightType = context.val->get_type();
        auto rightVal = context.val;
        if (pair.first == OP_EQ) {
            if (leftType->is_float_type() && rightType->is_float_type()) {
                leftVal = builder->create_fcmp_eq(leftVal, rightVal);
            } else if (leftType->is_float_type() && rightType->is_integer_type()) {
                leftVal = builder->create_fcmp_eq(leftVal, builder->create_sitofp(rightVal, FLOAT_T));
            } else if (leftType->is_integer_type() && rightType->is_float_type()) {
                leftVal = builder->create_fcmp_eq(builder->create_sitofp(leftVal, FLOAT_T), rightVal);
            } else {
                leftVal = builder->create_icmp_eq(leftVal, rightVal);
                _DEBUG_("Comparing integers: " + leftVal->print() + " == " + rightVal->print());
            }
        } else if (pair.first == OP_NEQ) {
            if (leftType->is_float_type() && rightType->is_float_type()) {
                leftVal = builder->create_fcmp_ne(leftVal, rightVal);
            } else if (leftType->is_float_type() && rightType->is_integer_type()) {
                leftVal = builder->create_fcmp_ne(leftVal, builder->create_sitofp(rightVal, FLOAT_T));
            } else if (leftType->is_integer_type() && rightType->is_float_type()) {
                leftVal = builder->create_fcmp_ne(builder->create_sitofp(leftVal, FLOAT_T), rightVal);
            } else {
                _DEBUG_("Comparing integers: " + leftVal->print() + " != " + rightVal->print());
                leftVal = builder->create_icmp_ne(leftVal, rightVal);
            }
        }
        if (leftVal->get_type()->is_int1_type()) {
            // 如果是布尔类型，转换为整数类型
            leftVal = builder->create_zext(leftVal, INT32_T);
        }
        leftType = leftVal->get_type();
    }
    context.val=leftVal; // 更新上下文中的值
    return nullptr;
}
Value *SysYBuilder::visit(ASTRelExp &node) {
    _DEBUG_("Visiting ASTRelExp");
    node.left->accept(*this);
    auto leftType = context.val->get_type();
    auto leftVal = context.val;
    if (node.right.empty()) {
        context.val = leftVal; // 如果没有右操作数，直接返回左操作数
        return nullptr;
    }
    for (const auto &pair : node.right) {
        pair.second->accept(*this);
        auto rightType = context.val->get_type();
        auto rightVal = context.val;
        if (pair.first == OP_LE) {
            if (leftType->is_float_type() && rightType->is_float_type()) {
                leftVal = builder->create_fcmp_le(leftVal, rightVal);
            } else if (leftType->is_float_type() && rightType->is_integer_type()) {
                leftVal = builder->create_fcmp_le(leftVal, builder->create_sitofp(rightVal, FLOAT_T));
            } else if (leftType->is_integer_type() && rightType->is_float_type()) {
                leftVal = builder->create_fcmp_le(builder->create_sitofp(leftVal, FLOAT_T), rightVal);
            } else {
                leftVal = builder->create_icmp_le(leftVal, rightVal);
            }
        } else if (pair.first == OP_LT) {
            if (leftType->is_float_type() && rightType->is_float_type()) {
                leftVal = builder->create_fcmp_lt(leftVal, rightVal);
            } else if (leftType->is_float_type() && rightType->is_integer_type()) {
                leftVal = builder->create_fcmp_lt(leftVal, builder->create_sitofp(rightVal, FLOAT_T));
            } else if (leftType->is_integer_type() && rightType->is_float_type()) {
                leftVal = builder->create_fcmp_lt(builder->create_sitofp(leftVal, FLOAT_T), rightVal);
            } else {
                leftVal = builder->create_icmp_lt(leftVal, rightVal);
            }
        } else if (pair.first == OP_GT) {
            if (leftType->is_float_type() && rightType->is_float_type()) {
                leftVal = builder->create_fcmp_gt(leftVal, rightVal);
            } else if (leftType->is_float_type() && rightType->is_integer_type()) {
                leftVal = builder->create_fcmp_gt(leftVal, builder->create_sitofp(rightVal, FLOAT_T));
            } else if (leftType->is_integer_type() && rightType->is_float_type()) {
                leftVal = builder->create_fcmp_gt(builder->create_sitofp(leftVal, FLOAT_T), rightVal);
            } else {
                leftVal = builder->create_icmp_gt(leftVal, rightVal);
            }
        } else if (pair.first == OP_GE) {
            if (leftType->is_float_type() && rightType->is_float_type()) {
                leftVal = builder->create_fcmp_ge(leftVal, rightVal);
            } else if (leftType->is_float_type() && rightType->is_integer_type()) {
                leftVal = builder->create_fcmp_ge(leftVal, builder->create_sitofp(rightVal, FLOAT_T));
            } else if (leftType->is_integer_type() && rightType->is_float_type()) {
                leftVal = builder->create_fcmp_ge(builder->create_sitofp(leftVal, FLOAT_T), rightVal);
            } else {
                leftVal = builder->create_icmp_ge(leftVal, rightVal);
            }
        } 
        leftType = leftVal->get_type();
        leftVal = leftVal;
    }
    context.val = builder->create_zext(leftVal, INT32_T); // 扩展成32位
    return nullptr;
}

Value* SysYBuilder::visit(ASTAddExp &node) {
    _DEBUG_("Visiting ASTAddExp");
    node.left->accept(*this);
    auto leftType = context.val->get_type();
    auto leftVal = context.val;
    auto leftValIntConst = dynamic_cast<ConstantInt*>(leftVal);
    auto leftValFPConst = dynamic_cast<ConstantFP*>(leftVal);
    for (const auto &pair : node.right) {
        pair.second->accept(*this);
        auto rightType = context.val->get_type();
        auto rightVal = context.val;
        auto rightValIntConst = dynamic_cast<ConstantInt*>(rightVal);
        auto rightValFPConst = dynamic_cast<ConstantFP*>(rightVal);

        if (leftValIntConst && rightValIntConst) {
            // Int op Int
            if (pair.first == OP_PLUS) {
                context.val = CONST_INT(leftValIntConst->get_value() + rightValIntConst->get_value());
            } else if (pair.first == OP_MINUS) {
                context.val = CONST_INT(leftValIntConst->get_value() - rightValIntConst->get_value());
            } else {
                assert(false && "Unsupported operation for integer constants");
            }
        } else if (leftValFPConst && rightValFPConst) {
            // Float op Float
            if (pair.first == OP_PLUS) {
                context.val = CONST_FP(leftValFPConst->get_value() + rightValFPConst->get_value());
            } else if (pair.first == OP_MINUS) {
                context.val = CONST_FP(leftValFPConst->get_value() - rightValFPConst->get_value());
            } else {
                assert(false && "Unsupported operation for float constants");
            }
        } else if (leftValIntConst && rightValFPConst) {
            // Int op Float
            if (pair.first == OP_PLUS) {
                context.val = CONST_FP(leftValIntConst->get_value() + rightValFPConst->get_value());
            } else if (pair.first == OP_MINUS) {
                context.val = CONST_FP(leftValIntConst->get_value() - rightValFPConst->get_value());
            } else {
                assert(false && "Unsupported operation for mixed integer and float constants");
            }
        } else if (leftValFPConst && rightValIntConst) {
            // Float op Int
            if (pair.first == OP_PLUS) {
                context.val = CONST_FP(leftValFPConst->get_value() + rightValIntConst->get_value());
            } else if (pair.first == OP_MINUS) {
                context.val = CONST_FP(leftValFPConst->get_value() - rightValIntConst->get_value());
            } else {
                assert(false && "Unsupported operation for mixed float and integer constants");
            }
        } else {
            // 非全常数的计算
            if (pair.first == OP_PLUS) {
                if (leftType->is_float_type() && rightType->is_float_type()) {
                    context.val = builder->create_fadd(leftVal, rightVal);
                } else if (leftType->is_float_type() && rightType->is_integer_type()) {
                    context.val = builder->create_fadd(leftVal, builder->create_sitofp(rightVal, FLOAT_T));
                } else if (leftType->is_integer_type() && rightType->is_float_type()) {
                    context.val = builder->create_fadd(builder->create_sitofp(leftVal, FLOAT_T), rightVal);
                } else {
                    context.val = builder->create_iadd(leftVal, rightVal);
                }
            } else if (pair.first == OP_MINUS) {
                if (leftType->is_float_type() && rightType->is_float_type()) {
                    context.val = builder->create_fsub(leftVal, rightVal);
                } else if (leftType->is_float_type() && rightType->is_integer_type()) {
                    context.val = builder->create_fsub(leftVal, builder->create_sitofp(rightVal, FLOAT_T));
                } else if (leftType->is_integer_type() && rightType->is_float_type()) {
                    context.val = builder->create_fsub(builder->create_sitofp(leftVal, FLOAT_T), rightVal);
                } else {
                    context.val = builder->create_isub(leftVal, rightVal);
                }
            }
        }
        leftType = context.val->get_type();
        leftVal = context.val;
        leftValIntConst = dynamic_cast<ConstantInt*>(leftVal);
        leftValFPConst = dynamic_cast<ConstantFP*>(leftVal);
    }
    context.val = leftVal;
    return context.val;
}

Value* SysYBuilder::visit(ASTMulExp &node) {
    _DEBUG_("Visiting ASTMulExp");
    node.left->accept(*this);
    auto leftType = context.val->get_type();
    auto leftVal = context.val;
    auto leftValIntConst = dynamic_cast<ConstantInt*>(leftVal);
    auto leftValFPConst = dynamic_cast<ConstantFP*>(leftVal);

    for (const auto &pair : node.right) {
        pair.second->accept(*this);
        auto rightType = context.val->get_type();
        auto rightVal = context.val;
        auto rightValIntConst = dynamic_cast<ConstantInt*>(rightVal);
        auto rightValFPConst = dynamic_cast<ConstantFP*>(rightVal);

        if (leftValIntConst && rightValIntConst) {
            // Int op Int
            if (pair.first == OP_MUL) {
                context.val = CONST_INT(leftValIntConst->get_value() * rightValIntConst->get_value());
            } else if (pair.first == OP_DIV) {
                if (rightValIntConst->get_value() == 0) {
                    _ERROR_("Division by zero in integer division");
                }
                context.val = CONST_INT(leftValIntConst->get_value() / rightValIntConst->get_value());
            } else if (pair.first == OP_MOD) {
                if (rightValIntConst->get_value() == 0) {
                    _ERROR_("Division by zero in integer modulo");
                }
                context.val = CONST_INT(leftValIntConst->get_value() % rightValIntConst->get_value());
            } else {
                assert(false && "Unsupported operation for integer constants");
            }
        } else if (leftValFPConst && rightValFPConst) {
            // Float op Float
            if (pair.first == OP_MUL) {
                context.val = CONST_FP(leftValFPConst->get_value() * rightValFPConst->get_value());
            } else if (pair.first == OP_DIV) {
                if (rightValFPConst->get_value() == 0.0) {
                    _ERROR_("Division by zero in float division");
                }
                context.val = CONST_FP(leftValFPConst->get_value() / rightValFPConst->get_value());
            } else if (pair.first == OP_MOD) {
                _ERROR_("Modulo operation is not supported for float types");
            } else {
                assert(false && "Unsupported operation for float constants");
            }
        } else if (leftValIntConst && rightValFPConst) {
            // Int op Float
            if (pair.first == OP_MUL) {
                context.val = CONST_FP(leftValIntConst->get_value() * rightValFPConst->get_value());
            } else if (pair.first == OP_DIV) {
                if (rightValFPConst->get_value() == 0.0) {
                    _ERROR_("Division by zero in mixed integer and float division");
                }
                context.val = CONST_FP(leftValIntConst->get_value() / rightValFPConst->get_value());
            } else if (pair.first == OP_MOD) {
                _ERROR_("Modulo operation is not supported for mixed integer and float types");
            } else {
                assert(false && "Unsupported operation for mixed integer and float constants");
            }
        } else if (leftValFPConst && rightValIntConst) {
            // Float op Int
            if (pair.first == OP_MUL) {
                context.val = CONST_FP(leftValFPConst->get_value() * rightValIntConst->get_value());
            } else if (pair.first == OP_DIV) {
                if (rightValIntConst->get_value() == 0) {
                    _ERROR_("Division by zero in mixed float and integer division");
                }
                context.val = CONST_FP(leftValFPConst->get_value() / rightValIntConst->get_value());
            } else if (pair.first == OP_MOD) {
                _ERROR_("Modulo operation is not supported for mixed float and integer types");
            } else {
                assert(false && "Unsupported operation for mixed float and integer constants");
            }
        }
        else {
            // 非全常数的计算
            if (pair.first == OP_MUL) {
                if (leftType->is_float_type() && rightType->is_float_type()) {
                    context.val = builder->create_fmul(leftVal, rightVal);
                } else if (leftType->is_float_type() && rightType->is_integer_type()) {
                    context.val = builder->create_fmul(leftVal, builder->create_sitofp(rightVal, FLOAT_T));
                } else if (leftType->is_integer_type() && rightType->is_float_type()) {
                    context.val = builder->create_fmul(builder->create_sitofp(leftVal, FLOAT_T), rightVal);
                } else {
                    context.val = builder->create_imul(leftVal, rightVal);
                }
            } else if (pair.first == OP_DIV) {
                if (leftType->is_float_type() && rightType->is_float_type()) {
                    context.val = builder->create_fdiv(leftVal, rightVal);
                } else if (leftType->is_float_type() && rightType->is_integer_type()) {
                    context.val = builder->create_fdiv(leftVal, builder->create_sitofp(rightVal, FLOAT_T));
                } else if (leftType->is_integer_type() && rightType->is_float_type()) {
                    context.val = builder->create_fdiv(builder->create_sitofp(leftVal, FLOAT_T), rightVal);
                } else {
                    context.val = builder->create_isdiv(leftVal, rightVal);
                }
            } else if (pair.first == OP_MOD) {
                // TODO float
                if (leftType->is_integer_type() && rightType->is_integer_type()) {
                    context.val = builder->create_srem(leftVal, rightVal);
                } else {
                    assert(false && "Mod operation is only supported for integer types");
                }
            }
        }
        leftType = context.val->get_type();
        leftVal = context.val;
        leftValIntConst = dynamic_cast<ConstantInt*>(leftVal);
        leftValFPConst = dynamic_cast<ConstantFP*>(leftVal);
    }
    context.val = leftVal;
    return nullptr;
}

Value *SysYBuilder::visit(ASTUnaryExp &node) {
    // 处理符号
    auto final_op = OP_POS;
    auto is_cond = false;
    for (auto &op : node.ops) {
        if (op == OP_NEG) {
            if (final_op == OP_POS) {
                final_op = OP_NEG;
            } else if (final_op == OP_NEG) {
                final_op = OP_POS;
            } else { // -!
                final_op = OP_NEG;
            }
        } else if (op == OP_NOT) {
            if (final_op == OP_NOT) {
                final_op = OP_POS; // 双重否定
            } else {
                final_op = OP_NOT;
                is_cond = true;
            }
        }
    }
    // 处理值
    auto exp = node.primary_exp->accept(*this);
    if (is_cond) { // 是条件语句
        if (final_op == OP_POS) { // 双重取反
            if (exp->get_type()->is_int32_type()) { // 整数类型
                if (dynamic_cast<ConstantInt *>(exp)) { // 常数
                    context.val = CONST_INT(static_cast<ConstantInt *>(exp)->get_value() != 0 ? 1 : 0);
                } else {
                    auto cond = builder->create_icmp_ne(exp, CONST_INT(0));
                    context.val = builder->create_zext(cond, INT32_T); // 扩展成32位
                }
            } else { // 浮点数类型
                if (dynamic_cast<ConstantFP *>(exp)) { // 常数
                    context.val = CONST_INT(static_cast<ConstantFP *>(exp)->get_value() != 0.0 ? 1 : 0);
                } else { // 变量
                    auto cond = builder->create_fcmp_ne(exp, CONST_FP(0.0));
                    context.val = builder->create_zext(cond, INT32_T); // 扩展成32位
                }
            }
        } else if (final_op == OP_NOT) { // 取反加负
            if (exp->get_type()->is_int32_type()) { // 整数类型
                if (dynamic_cast<ConstantInt *>(exp)) { // 常数
                    context.val = CONST_INT(static_cast<ConstantInt *>(exp)->get_value() == 0 ? -1 : 0);
                } else { // 变量
                    auto cond = builder->create_icmp_eq(exp, CONST_INT(0));
                    auto zext = builder->create_zext(cond, INT32_T); // 扩展成32位
                    context.val = builder->create_isub(CONST_INT(0), zext);
                }
            } else { // 浮点数类型
                if (dynamic_cast<ConstantFP *>(exp)) { // 常数
                    context.val = CONST_INT(static_cast<ConstantFP *>(exp)->get_value() == 0.0 ? -1 : 0);
                } else { // 变量
                    auto cond = builder->create_fcmp_eq(exp, CONST_FP(0.0));
                    auto zext = builder->create_zext(cond, INT32_T); // 扩展成32位
                    context.val = builder->create_fsub(CONST_FP(0.0), zext);
                }
            }
        } else { // 取反
            if (exp->get_type()->is_int32_type()) { // 整数类型
                if (dynamic_cast<ConstantInt *>(exp)) { // 常数
                    context.val = CONST_INT(static_cast<ConstantInt *>(exp)->get_value() == 0 ? 1 : 0);
                } else { // 变量
                    auto cond = builder->create_icmp_eq(exp, CONST_INT(0));
                    context.val = builder->create_zext(cond, INT32_T); // 扩展成32位
                }
            } else { // 浮点数类型
                if (dynamic_cast<ConstantFP *>(exp)) { // 常数
                    context.val = CONST_INT(static_cast<ConstantFP *>(exp)->get_value() == 0.0 ? 1 : 0);
                } else { // 变量
                    auto cond = builder->create_fcmp_eq(exp, CONST_FP(0.0));
                    context.val = builder->create_zext(cond, INT32_T); // 扩展成32位
                }
            }
        }
    } else { // 简单语句
        if (final_op == OP_POS) { // 正号
            context.val = exp;
        } else if (final_op == OP_NEG) { // 负号
            if (exp->get_type()->is_int32_type()) { // 整数类型
                if (dynamic_cast<ConstantInt *>(exp)) { // 常数
                    context.val = CONST_INT(-static_cast<ConstantInt *>(exp)->get_value());
                } else { // 变量
                    context.val = builder->create_isub(CONST_INT(0), exp);
                }
            } else if (exp->get_type()->is_float_type()) { // 浮点数类型
                if (dynamic_cast<ConstantFP *>(exp)) { // 常数
                    context.val = CONST_FP(-static_cast<ConstantFP *>(exp)->get_value());
                } else { // 变量
                    context.val = builder->create_fsub(CONST_FP(0.0), exp);
                }
            }
        } else {
            assert(false && "Not support other types for op '!'");
        }
    }

    return context.val;
}

Value* SysYBuilder::visit(ASTParenthesizedExp &node) {
    context.val = node.exp->accept(*this);
    return context.val;
}

Value *SysYBuilder::visit(ASTCall &node) {
    auto func = static_cast<Function *>(scope.find(node.id).second);
    std::vector<Value *> params;
    for (unsigned i = 0; i < node.params.size(); i++) {
        auto exp = node.params[i]->accept(*this);
        auto param_type = static_cast<FunctionType *>(func->get_type())->get_param_type(i);
        auto exp_type = exp->get_type();
        if (param_type->is_int32_type() && exp_type->is_float_type()) {
            exp = builder->create_fptosi(exp, INT32_T);
        } else if (param_type->is_float_type() && exp_type->is_int32_type()) {
            exp = builder->create_sitofp(exp, FLOAT_T);
        } else if (param_type != exp_type && param_type->is_pointer_type() && exp_type->is_pointer_type()) {
            exp = builder->create_bitcast(exp, param_type);
        } else {
            assert(param_type == exp_type && "Parameter type does not match expression type");
        }
        params.push_back(exp);
    }
    auto call = builder->create_call(func, params);

    context.val = call;
    return context.val;
}

Value *SysYBuilder::visit(ASTLVal &node) {
    std::vector<Value *> indices;
    for (auto &index : node.indices) {
        auto index_int = index->accept(*this);
        assert(index_int->get_type()->is_int32_type() && "index must be int");
        indices.push_back(index_int);
    }
    if (node.is_lval) { // 左值
        auto lval = scope.find(node.id).second;
        if (!indices.empty()) { // 数组
            if (lval->get_type()->get_pointer_element_type()->is_pointer_type()) {
                lval = builder->create_load(lval);
                lval = builder->create_gep(lval, {indices.front()});
                indices.erase(indices.begin());
            }
            for (auto &index : indices) {
                lval = builder->create_gep(lval, {CONST_INT(0), index});
            }
        }
        context.val = lval;
        return context.val;
    } else { // 右值
        auto rval = scope.find(node.id).second;
        if (indices.empty()) { // 没有维度
            if (rval->get_type()->is_pointer_type() &&
                rval->get_type()->get_pointer_element_type()->is_array_type()) { // 数组名
                rval = builder->create_gep(rval, {CONST_INT(0), CONST_INT(0)});
            } else { // 单值
                if (!dynamic_cast<Constant *>(rval)) { // 不是常量
                    rval = builder->create_load(rval);
                }
            }
        } else { // 需要处理维度
            auto constant = scope.find(node.id, indices).second;
            if (constant) { // 是常量
                rval = constant;
            } else { // 是变量
                if (rval->get_type()->get_pointer_element_type()->is_pointer_type()) {
                    rval = builder->create_load(rval);
                    rval = builder->create_gep(rval, {indices.front()});
                    indices.erase(indices.begin());
                }
                for (auto &index : indices) {
                    rval = builder->create_gep(rval, {CONST_INT(0), index});
                }
                if (!rval->get_type()->get_pointer_element_type()->is_array_type()) {
                    rval = builder->create_load(rval);
                }
            }
        }
        context.val = rval;
        return context.val;
    }
}

Value* SysYBuilder::visit(ASTNum &node) {
    if(node.type == TYPE_INT)
        context.val = CONST_INT(node.i_val);
    else if(node.type == TYPE_FLOAT)
        context.val = CONST_FP(node.f_val);
    return context.val;
}
/*******************************************************************************************/
