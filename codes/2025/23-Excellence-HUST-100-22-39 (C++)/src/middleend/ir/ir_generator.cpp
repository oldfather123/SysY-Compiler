#include <iostream>
#include "ir_generator.hpp"
#include "ir_module.hpp"
#include "ir_scope.hpp"
#include "ir_builder.hpp"
#include "ir_function.hpp"
#include "ir_value.hpp"
#include "ir_type.hpp"
#include "ir_instruction.hpp"
#include "ir_basic_block.hpp"

void IRGenerator::implicit_cast() {
    if(cur_type == module->int32_type) {
        if(auto const_float = dynamic_cast<ConstFloat*>(last_val)) {
            last_val = new ConstInt(module->int32_type, static_cast<int>(const_float->val));
        } else if(last_val->type == module->float_type) {
            last_val = builder->create_fptosi(last_val, module->int32_type);
        }
    } else if(cur_type == module->float_type) {
        if(auto const_int = dynamic_cast<ConstInt*>(last_val)) {
            last_val = new ConstFloat(module->float_type, static_cast<float>(const_int->val));
        } else if(last_val->type == module->int32_type) {
            last_val = builder->create_sitofp(last_val, module->float_type);
        }
    }
}

ConstArray* IRGenerator::init_global_array(vector<int>& dimensions, vector<ArrayType*>& array_types, int up, vector<unique_ptr<InitValAST>>& list) {
    vector<int> element_count(dimensions.size());    // 对应各个维度的子数组的元素个数
    vector<Const*> elements;                     // 各个元素
    int dim_add;
    for(auto& val: list) {
        if(val->exp) {
            dim_add = dimensions.size() - 1;
            val->exp->accept(*this);
            implicit_cast();
            elements.push_back((ConstInt*)last_val);
        } else {
            auto next_up = get_next_dim(element_count, up); // 该嵌套数组的维度
            dim_add = next_up - 1; // 比他高一维度的数组需要添加一个元素
            if(next_up == dimensions.size()) {
                cerr << "[ERROR] Initial invalid: no continuous 0, not aligned!" << endl; // 没有连续0，没对齐，不合法
                exit(1);
            }
            if(val->init_val_list.empty()) {
                elements.push_back(new ConstZero(array_types[next_up]));
            } else {
                elements.push_back(init_global_array(dimensions, array_types, next_up, val->init_val_list));
            }
        }
        element_count[dim_add]++;
        merge_elements(dimensions, array_types, up, dim_add, elements, element_count);
    }
    final_merge(dimensions, array_types, up, elements, element_count);
    return static_cast<ConstArray*>(elements[0]);
}

int IRGenerator::get_next_dim(vector<int>& element_count, int up) {
    for(int i = element_count.size() - 1; i > up; i--) {
        if(element_count[i] != 0) {
            return i + 1;
        }
    }
    return up + 1;
}

int IRGenerator::get_next_dim(vector<int>& dim_count, int up, int cnt) {
    for(int i = up; i < dim_count.size(); i++) {
        if(cnt % dim_count[i] == 0) {
            return i;
        }
    }
    return 0;
}

void IRGenerator::init_local_array(Value* ptr, vector<unique_ptr<InitValAST>>& list, vector<int>& dim_count, int up) {
    int cnt = 0;
    Value* gep_inst = ptr;
    for(auto& init_val: list) {
        if(init_val->exp) {
            if(cnt == 0)
                cnt++; // 第一次赋值时可以少一次create_gep
            else
                gep_inst = builder->create_gep(ptr, {new ConstInt(module->int32_type, cnt++)});
            init_val->exp->accept(*this);
            implicit_cast();
            builder->create_store(last_val, gep_inst);
        } else {
            auto next_up = get_next_dim(dim_count, up, cnt);
            if(next_up == 0) {
                cerr << "[ERROR] Initial invalid: no continuous 0, not aligned!" << endl; // 没有连续0，没对齐，不合法
                exit(1);
            }
            if(!init_val->init_val_list.empty()) {
                if(cnt != 0) {
                    gep_inst = builder->create_gep(ptr, {new ConstInt(module->int32_type, cnt)}); // 没赋值过，那gep_inst实际就是ptr
                }
                init_local_array(gep_inst, init_val->init_val_list, dim_count, next_up);
            }
            cnt += dim_count[next_up]; // 数组初始化量一定增加这么多
        }
    }
}

void IRGenerator::merge_elements(vector<int>& dimensions, vector<ArrayType*>& array_types, int up, int dim_add, vector<Const*>& elements, vector<int>& element_count) {
    for(int i = dim_add; i > up; i--) {
        if(element_count[i] % dimensions[i] == 0) {
            vector<Const*> temp;
            temp.assign(elements.end() - dimensions[i], elements.end());
            elements.erase(elements.end() - dimensions[i], elements.end());
            elements.push_back(new ConstArray(array_types[i], temp));
            element_count[i] = 0;
            element_count[i - 1]++;
        } else {
            break;
        }
    }
}

void IRGenerator::final_merge(vector<int>& dimensions, vector<ArrayType*>& array_types, int up, vector<Const*>& elements, vector<int>& element_count) {
    for(int i = dimensions.size() - 1; i >= up; i--) {
        while(element_count[i] % dimensions[i] != 0) { // 补充当前数组类型所需0元素
            if(i == dimensions.size() - 1) {
                if(cur_type == module->int32_type) {
                    elements.push_back(new ConstInt(module->int32_type, 0));
                } else {
                    elements.push_back(new ConstFloat(module->float_type, 0));
                }
            } else {
                elements.push_back(new ConstZero(array_types[i + 1]));
            }
            element_count[i]++;
        }
        if(element_count[i] != 0) {
            vector<Const*> temp;
            temp.assign(elements.end() - dimensions[i], elements.end());
            elements.erase(elements.end() - dimensions[i], elements.end());
            elements.push_back(new ConstArray(array_types[i], temp));
            element_count[i] = 0;
            if(i != up) {
                element_count[i - 1]++;
            }
        }
    }
}

bool IRGenerator::check_const_cal_type(Value* val[], int int_vals[], float float_vals[]) {
    bool is_int = false;
    if(dynamic_cast<ConstInt*>(val[0]) && dynamic_cast<ConstInt*>(val[1])) {
        is_int = true;
        int_vals[0] = static_cast<ConstInt*>(val[0])->val;
        int_vals[1] = static_cast<ConstInt*>(val[1])->val;
    } else { // 操作结果一定是float
        if(dynamic_cast<ConstInt*>(val[0])) {
            float_vals[0] = static_cast<ConstInt*>(val[0])->val;
        } else {
            float_vals[0] = static_cast<ConstFloat*>(val[0])->val;
        }
        if(dynamic_cast<ConstInt*>(val[1])) {
            float_vals[1] = static_cast<ConstInt*>(val[1])->val;
        } else {
            float_vals[1] = static_cast<ConstFloat*>(val[1])->val;
        }
    }
    return is_int;
}

void IRGenerator::check_not_const_cal_type(Value* val[]) {
    if(val[0]->type == module->int1_type) {
        val[0] = builder->create_zext(val[0], module->int32_type);
    }
    if(val[1]->type == module->int1_type) {
        val[1] = builder->create_zext(val[1], module->int32_type);
    }
    if(val[0]->type == module->int32_type && val[1]->type == module->float_type) {
        val[0] = builder->create_sitofp(val[0], module->float_type);
    }
    if(val[1]->type == module->int32_type && val[0]->type == module->float_type) {
        val[1] = builder->create_sitofp(val[1], module->float_type);
    }
}

IRGenerator::IRGenerator()
    : cur_type(nullptr), cur_func(nullptr), func_bb(nullptr), ret_bb(nullptr), while_cond_bb(nullptr), while_next_bb(nullptr), true_bb(nullptr), false_bb(nullptr),
        has_br(false), is_single_exp(false), require_l_val(false), last_val(nullptr), id(1), is_const(false), use_const(false), is_new_func(false) {
    module = new Module();
    scope = new Scope();
    builder = new IRBuilder(module, nullptr);
    scope->enter_child_scope();
    scope->add_to_cur_scope("getint", new Function(new FunctionType(module->int32_type, {}), "getint", module));
    scope->add_to_cur_scope("getch", new Function(new FunctionType(module->int32_type, {}), "getch", module));
    scope->add_to_cur_scope("getfloat", new Function(new FunctionType(module->float_type, {}), "getfloat", module));
    scope->add_to_cur_scope("getarray", new Function(new FunctionType(module->int32_type, {module->get_pointer_type(module->int32_type)}), "getarray", module));
    scope->add_to_cur_scope("getfarray", new Function(new FunctionType(module->int32_type, {module->get_pointer_type(module->float_type)}), "getfarray", module));
    scope->add_to_cur_scope("putint", new Function(new FunctionType(module->void_type, {module->int32_type}), "putint", module));
    scope->add_to_cur_scope("putfloat", new Function(new FunctionType(module->void_type, {module->float_type}), "putfloat", module));
    scope->add_to_cur_scope("putch", new Function(new FunctionType(module->void_type, {module->int32_type}), "putch", module));
    scope->add_to_cur_scope("putarray", new Function(new FunctionType(module->void_type, {module->int32_type, module->get_pointer_type(module->int32_type)}), "putarray", module));
    scope->add_to_cur_scope("putfarray", new Function(new FunctionType(module->void_type, {module->int32_type, module->get_pointer_type(module->float_type)}), "putfarray", module));
    scope->add_to_cur_scope("starttime", new Function(new FunctionType(module->void_type, {}), "_sysy_starttime", module));
    scope->add_to_cur_scope("stoptime", new Function(new FunctionType(module->void_type, {}), "_sysy_stoptime", module));
    scope->add_to_cur_scope("memclr", new Function(new FunctionType(module->void_type, {module->get_pointer_type(module->int32_type), module->int32_type}), "__aeabi_memclr4", module));
    scope->add_to_cur_scope("llvm.memset.p0.i32", new Function(new FunctionType(module->void_type, {}), "llvm.memset.p0.i32", module));    
}

IRGenerator::~IRGenerator() {
    delete module;
    delete scope;
    delete builder;
}

Module* IRGenerator::get_module() {
    for(auto& func: module->func_list) {
        if(!func->bb_list.empty()) {
            func->set_ssa_id(); // 设置 SSA 编号
        }
    }
    return module;
}

void IRGenerator::visit(CompUnitAST& ast) {
    for(const auto& decl_def: ast.decl_def_list) {
        decl_def->accept(*this);
    }
}

void IRGenerator::visit(DeclDefAST& ast) {
    if(ast.decl) {
        ast.decl->accept(*this);
    } else {
        ast.func_def->accept(*this);
    }
}

void IRGenerator::visit(DeclAST& ast) {
    // 记录当前声明是否为常量以及其类型
    is_const = ast.is_const;
    cur_type = ast.type == TYPE_INT ? module->int32_type : module->float_type; // i32 or float
    for(auto& def: ast.def_list) {
        def->accept(*this);
    }
}

void IRGenerator::visit(DefAST& ast) {
    string var_name = *ast.id;
    if(scope->is_global()) { // 全局变量或常量
        if(ast.arrays.empty()) { // 不是数组，即全局量
            if(ast.init_val == nullptr) { // 无初始化
                if(is_const) {
                    cerr << "[ERROR] Constant variable must be initialized!" << endl;
                    exit(1);
                }
                GlobalVariable* global_var; // 此时一定为全局变量
                if(cur_type == module->int32_type) {
                    global_var = new GlobalVariable(var_name, module, cur_type, false, new ConstInt(module->int32_type, 0));
                } else {
                    global_var = new GlobalVariable(var_name, module, cur_type, false, new ConstFloat(module->float_type, 0));
                }
                scope->add_to_cur_scope(var_name, global_var);
            } else { // 有初始化
                use_const = true;
                ast.init_val->accept(*this); // 计算初始值
                use_const = false;
                implicit_cast();
                if(is_const) { // 全局常量，直接插入符号表，无需GlobalVariable
                    scope->add_to_cur_scope(var_name, last_val);
                } else { // 全局变量
                    scope->add_to_cur_scope(var_name, new GlobalVariable(var_name, module, cur_type, false, static_cast<Const*>(last_val)));
                }
            }
        } else { // 是数组，即全局数组量
            vector<int> dimensions; // 数组各维度; [2][3][4] 对应下方的数组类型
            use_const = true; // 维度计算为常量
            for(auto& exp: ast.arrays) { // 获取数组各维度
                exp->accept(*this);
                dimensions.push_back(static_cast<ConstInt*>(last_val)->val);
            }
            use_const = false;
            vector<ArrayType*> array_types(dimensions.size()); // 数组类型：{[2 * [3 * [4 * i32]]], [3 * [4 * i32]], [4 * i32]}
            for(int i = dimensions.size() - 1; i >= 0; i--) {
                if(i == dimensions.size() - 1) {
                    array_types[i] = module->get_array_type(cur_type, dimensions[i]);
                } else {
                    array_types[i] = module->get_array_type(array_types[i + 1], dimensions[i]);
                }
            }
            // 无初始化或者初始化为空
            if(ast.init_val == nullptr || ast.init_val->init_val_list.empty()) {
                auto init = new ConstZero(array_types[0]);
                auto var = new GlobalVariable(var_name, module, array_types[0], is_const, init);
                scope->add_to_cur_scope(var_name, var);
            } else {
                use_const = true; // 全局数组量的初始值必为常量
                auto global_array_init_val = init_global_array(dimensions, array_types, 0, ast.init_val->init_val_list);
                use_const = false;
                scope->add_to_cur_scope(var_name, new GlobalVariable(var_name, module, array_types[0], is_const, global_array_init_val));
            }
        }
    } else { // 局部变量或常量
        if(ast.arrays.empty()) { // 不是数组，即普通局部量
            if(ast.init_val == nullptr) { // 无初始化
                if(is_const) {
                    cerr << "[ERROR] Constant variable must be initialized!" << endl;
                    exit(1);
                } else { // 无初始化变量
                    auto bb_bak = builder->cur_bb;
                    builder->cur_bb = func_bb; // 将局部变量移动至当前函数头部的 Basic block
                    AllocaInst* alloca_inst = builder->create_alloca(cur_type);
                    builder->cur_bb = bb_bak; // 还原插入点
                    scope->add_to_cur_scope(var_name, alloca_inst);
                }
            } else { // 有初始化
                ast.init_val->accept(*this);
                implicit_cast();
                if(is_const) {
                    scope->add_to_cur_scope(var_name, last_val); // 单个常量定义不用create_alloca
                } else {
                    auto bb_bak = builder->cur_bb;
                    builder->cur_bb = func_bb; 
                    AllocaInst* alloca_inst = builder->create_alloca(cur_type);
                    builder->cur_bb = bb_bak;
                    scope->add_to_cur_scope(var_name, alloca_inst);
                    builder->create_store(last_val, alloca_inst);
                }
            }
        } else { // 局部数组量
            vector<int> dimensions(ast.arrays.size()), dim_count((ast.arrays.size())); // 数组各维度, [2][3][4]对应; 次维度数组元素个数, [24][12][4]
            int total_bytes = 1; // 存储总共的字节数
            use_const = true;
            // 获取数组各维度
            for(int i = dimensions.size() - 1; i >= 0; i--) {
                ast.arrays[i]->accept(*this);
                int dimension = static_cast<ConstInt*>(last_val)->val;
                total_bytes *= dimension;
                dimensions[i] = dimension;
                dim_count[i] = total_bytes;
            }
            total_bytes *= 4; // 计算字节数
            use_const = false;
            ArrayType* array_type = nullptr; // 数组类型
            for(int i = dimensions.size() - 1; i >= 0; i--) {
                if(i == dimensions.size() - 1) {
                    array_type = module->get_array_type(cur_type, dimensions[i]);
                } else {
                    array_type = module->get_array_type(array_type, dimensions[i]);
                }
            }
            AllocaInst* alloca_inst = builder->create_alloca(array_type);
            scope->add_to_cur_scope(var_name, alloca_inst);
            if(ast.init_val == nullptr) { // 无初始化
                if(is_const) {
                    cerr << "[ERROR] Constant variable must be initialized!" << endl;
                    exit(1);
                }
                return; // 无初始化变量数组无需再做处理
            }
            // 调用 __aeabi_memclr4 函数清零
            Function* memclr = static_cast<Function*>(scope->find_val_with_name("memclr"));
            UnaryInst* bitcast_inst = builder->create_bitcast(alloca_inst, module->get_pointer_type(module->int32_type));
            builder->create_call(memclr, {bitcast_inst, new ConstInt(module->int32_type, total_bytes)});
            // 数组初始化时，成员 exp 一定是空，若 init_val_list 也是空，即大括号，已经置零了直接返回
            if(ast.init_val->init_val_list.empty()) {
                return;
            }
            vector<Value*> idxs(dimensions.size() + 1);
            for(int i = 0; i < dimensions.size() + 1; i++) {
                idxs[i] = new ConstInt(module->int32_type, 0);
            }
            GetElementPtrInst* gep_inst = builder->create_gep(alloca_inst, idxs); // 获取数组开头地址
            init_local_array(gep_inst, ast.init_val->init_val_list, dim_count, 1);
        }
    }
}

void IRGenerator::visit(InitValAST& ast) {
    if(ast.exp) {
        ast.exp->accept(*this);
    }
}

void IRGenerator::visit(FuncDefAST& ast) {
    // 检测到一个新函数
    is_new_func = true;
    param_types.clear();
    param_names.clear();
    // 获取函数返回值类型
    Type* ret_type;
    if(ast.func_type == TYPE_INT) {
        ret_type = module->int32_type;
    } else if(ast.func_type == TYPE_FLOAT) {
        ret_type = module->float_type;
    } else {
        ret_type = module->void_type;
    }
    // 获取参数列表
    for(auto& func_param: ast.func_param_list) {
        func_param->accept(*this);
    }
    // 获取函数类型
    auto func_type = new FunctionType(ret_type, param_types);
    // 添加函数
    auto func = new Function(func_type, *ast.id, module);
    cur_func = func;
    scope->add_to_cur_scope(*ast.id, func); // 在进入新的作用域之前添加到当前级符号表中
    // 进入函数(进入新的作用域)
    scope->enter_child_scope();
    // 获取函数的形参
    vector<Value*> args;
    for(auto arg = func->args.begin(); arg != func->args.end(); arg++) {
        args.push_back(*arg);
    }
    auto bb = new BasicBlock(module, "label_entry", func); // 创建函数入口的BasicBlock
    builder->cur_bb = bb;
    func_bb = bb;
    for(int i = 0; i < param_types.size(); i++) {
        auto alloca_inst = builder->create_alloca(param_types[i]);  // 分配形参空间
        builder->create_store(args[i], alloca_inst);                // store 形参
        scope->add_to_cur_scope(param_names[i], alloca_inst);                       // 加入作用域
    }
    // 统一创建返回块
    ret_bb = new BasicBlock(module, "label_ret", func);
    if(ret_type == module->void_type) { // void类型无需返回值
        builder->cur_bb = ret_bb;
        builder->create_ret();
    } else {
        ret_val = builder->create_alloca(ret_type); // 在内存中分配返回值的位置
        builder->cur_bb = ret_bb;
        auto load_inst = builder->create_load(ret_val);
        builder->create_ret(load_inst);
    }
    // 重新回到函数开始
    builder->cur_bb = bb;
    has_br = false;
    ast.block->accept(*this);
    // 处理没有return的空块
    if(!builder->cur_bb->get_terminator())
        builder->create_uncond_br(ret_bb);
}

void IRGenerator::visit(FuncParamAST& ast) {
    // 获取参数类型
    Type* param_type;
    if(ast.type == TYPE_INT) {
        param_type = module->int32_type;
    } else {
        param_type = module->float_type;
    }
    // 是否为数组
    if(ast.is_array) {
        use_const = true; // 数组维度是整型常量
        for(int i = ast.arrays.size() - 1; i >= 0; i--) {
            ast.arrays[i]->accept(*this);
            param_type = module->get_array_type(param_type, ((ConstInt*)last_val)->val);
        }
        use_const = false;
        // 如int a[][2]，则参数为[2 * i32]* ;  int a[],参数为i32* 
        param_type = module->get_pointer_type(param_type);
    }
    param_types.push_back(param_type);
    param_names.push_back(*ast.id);
}

void IRGenerator::visit(BlockAST& ast) {
    if(is_new_func) { // 如果是一个新的函数，则不用再进入一个新的作用域，因为在FuncDef中已经进入了一个新的作用域
        is_new_func = false;
    } else { // 其它情况，需要进入一个新的作用域
        scope->enter_child_scope();
    }
    // 遍历每一个语句块
    for(auto& item: ast.block_item_list) {
        if(has_br) {
            break;
        }
        item->accept(*this);
    }
    scope->exit_child_scope();
}

void IRGenerator::visit(BlockItemAST& ast) {
    if(ast.decl) {
        ast.decl->accept(*this);
    } else {
        ast.stmt->accept(*this);
    }
}

void IRGenerator::visit(StmtAST& ast) {
    switch(ast.type) {
        case SEMI:
            break;
        case ASS: {
            require_l_val = true;       // Init l_val state
            ast.l_val->accept(*this);   // Visit l_val
            auto l_val = last_val;      // Get l_val
            auto l_val_type = static_cast<PointerType*>(l_val->type)->pointed_type;
            ast.exp->accept(*this);     // Visit expression
            auto r_val = last_val;
            if(l_val_type != last_val->type) { // if l_val.type != r_val.type Forge a cast
                if(l_val_type == module->float_type) {
                    r_val = builder->create_sitofp(last_val, module->float_type);
                } else {
                    r_val = builder->create_fptosi(last_val, module->int32_type);
                }
            }
            builder->create_store(r_val, l_val); // Create a store primitive
            break;
        }
        case EXP:
            is_single_exp = true;
            ast.exp->accept(*this);
            is_single_exp = false;
            break;
        case CONT:
            builder->create_uncond_br(while_cond_bb);
            has_br = true;
            break;
        case BRK:
            builder->create_uncond_br(while_next_bb);
            has_br = true;
            break;
        case RET:
            ast.ret_stmt->accept(*this);
            break;
        case BLK:
            ast.block->accept(*this);
            break;
        case COND:
            ast.cond_stmt->accept(*this);
            break;
        case LOOP:
            ast.loop_stmt->accept(*this);
            break;
    }
}

void IRGenerator::visit(ReturnStmtAST& ast) {
    if(ast.exp == nullptr) {
        last_val = builder->create_uncond_br(ret_bb);
    } else {
        // 先把返回值store在ret_val中，再跳转到统一的返回入口
        ast.exp->accept(*this);
        // 类型转换
        if(last_val->type == module->float_type && cur_func->get_ret_type() == module->int32_type) {
            builder->create_store(builder->create_fptosi(last_val, module->int32_type), ret_val);
        } else if(last_val->type == module->int32_type && cur_func->get_ret_type() == module->float_type) {
            builder->create_store(builder->create_sitofp(last_val, module->float_type), ret_val);
        } else {
            builder->create_store(last_val, ret_val);
        }
        last_val = builder->create_uncond_br(ret_bb);
    }
    has_br = true;
}

void IRGenerator::visit(CondStmtAST& ast) {
    auto true_bb_bak = true_bb;
    auto false_bb_bak = false_bb;
    true_bb = new BasicBlock(module, to_string(id++), cur_func);
    false_bb = new BasicBlock(module, to_string(id++), cur_func);
    BasicBlock* if_next_bb; // if语句后的基本块
    if(ast.else_stmt == nullptr) {
        if_next_bb = false_bb;
    } else {
        if_next_bb = new BasicBlock(module, to_string(id++), cur_func);
    }
    ast.cond->accept(*this);
    // 检查是否是i1，不是则进行比较
    if(last_val->type == module->int32_type) {
        last_val = builder->create_icmp_ne(last_val, new ConstInt(module->int32_type, 0));
    } else if(last_val->type == module->float_type) {
        last_val = builder->create_fcmp_ne(last_val, new ConstFloat(module->float_type, 0));
    }
    builder->create_cond_br(last_val, true_bb, false_bb);
    // 构建 true_bb
    builder->cur_bb = true_bb;
    has_br = false;
    ast.if_stmt->accept(*this);
    if(!builder->cur_bb->get_terminator()) {
        builder->create_uncond_br(if_next_bb);
    }
    // 构建 false_bb
    if(ast.else_stmt) {
        builder->cur_bb = false_bb;
        has_br = false;
        ast.else_stmt->accept(*this);
        if(!builder->cur_bb->get_terminator()) {
            builder->create_uncond_br(if_next_bb);
        }
    }

    builder->cur_bb = if_next_bb;
    has_br = false;
    // 还原 true_bb 和 false_bb
    true_bb = true_bb_bak;
    false_bb = false_bb_bak;
}

void IRGenerator::visit(LoopStmtAST& ast) {
    // 先保存true_bb和false_bb，防止嵌套导致返回上一层后丢失块的地址
    auto true_bb_bak = true_bb;
    auto false_bb_bak = false_bb; // 即 while 的 next block
    auto while_cond_bb_bak = while_cond_bb;
    auto while_next_bb_bak = while_next_bb; // break 只跳 while 的 false，而不跳全局 false

    while_cond_bb = new BasicBlock(module, to_string(id++), cur_func);
    true_bb = new BasicBlock(module, to_string(id++), cur_func);
    false_bb = new BasicBlock(module, to_string(id++), cur_func);
    while_next_bb = false_bb;
    // 先创建一个 while 的条件基本块
    builder->create_uncond_br(while_cond_bb);
    builder->cur_bb = while_cond_bb;
    has_br = false;
    ast.cond->accept(*this);
    if(last_val->type == module->int32_type) {
        last_val = builder->create_icmp_ne(last_val, new ConstInt(module->int32_type, 0));
    } else if(last_val->type == module->float_type) {
        last_val = builder->create_fcmp_ne(last_val, new ConstFloat(module->float_type, 0));
    }
    builder->create_cond_br(last_val, true_bb, false_bb);
    builder->cur_bb = true_bb;
    has_br = false;
    ast.stmt->accept(*this);
    // while 语句体一定是跳回 cond
    if(!builder->cur_bb->get_terminator()) {
        builder->create_uncond_br(while_cond_bb);
    }
    builder->cur_bb = false_bb;
    has_br = false;
    // 还原true_bb，false_bb，while_cond_bb_bak
    true_bb = true_bb_bak;
    false_bb = false_bb_bak;
    while_cond_bb = while_cond_bb_bak;
    while_next_bb = while_next_bb_bak;
}

void IRGenerator::visit(AddExpAST& ast) {
    if(ast.add_exp == nullptr) {
        ast.mul_exp->accept(*this);
        return;
    }
    Value* val[2]; // l_val, r_val
    ast.add_exp->accept(*this);
    val[0] = last_val;
    ast.mul_exp->accept(*this);
    val[1] = last_val;

    // 若都是常量
    if(use_const) {
        int int_vals[3];     // l_int, r_int, re_int;
        float float_vals[3]; // l_float, r_float, re_float;
        bool is_int = check_const_cal_type(val, int_vals, float_vals);
        switch(ast.op) {
            case AOP_ADD:
                int_vals[2] = int_vals[0] + int_vals[1];
                float_vals[2] = float_vals[0] + float_vals[1];
                break;
            case AOP_MINUS:
                int_vals[2] = int_vals[0] - int_vals[1];
                float_vals[2] = float_vals[0] - float_vals[1];
                break;
        }
        if(is_int) {
            last_val = new ConstInt(module->int32_type, int_vals[2]);
        } else {
            last_val = new ConstFloat(module->float_type, float_vals[2]);
        }
        return;
    }
    // 若不是常量，进行计算，输出指令
    check_not_const_cal_type(val);
    if(val[0]->type == module->int32_type) {
        switch(ast.op) {
            case AOP_ADD:
                last_val = builder->create_add(val[0], val[1]);
                break;
            case AOP_MINUS:
                last_val = builder->create_sub(val[0], val[1]);
                break;
        }
    } else {
        switch(ast.op) {
            case AOP_ADD:
                last_val = builder->create_fadd(val[0], val[1]);
                break;
            case AOP_MINUS:
                last_val = builder->create_fsub(val[0], val[1]);
                break;
        }
    }
}

void IRGenerator::visit(MulExpAST& ast) {
    if(ast.mul_exp == nullptr) {
        ast.unary_exp->accept(*this);
        return;
    }
    Value* val[2]; // l_val, r_val
    ast.mul_exp->accept(*this);
    val[0] = last_val;
    ast.unary_exp->accept(*this);
    val[1] = last_val;
    // 若都是常量
    if(use_const) {
        int int_vals[3];     // l_int, r_int, re_int;
        float float_vals[3]; // l_float, r_float, re_float;
        bool is_int = check_const_cal_type(val, int_vals, float_vals);
        switch(ast.op) {
            case MOP_MUL:
                int_vals[2] = int_vals[0]*  int_vals[1];
                float_vals[2] = float_vals[0]*  float_vals[1];
                break;
            case MOP_DIV:
                int_vals[2] = int_vals[0] / int_vals[1];
                float_vals[2] = float_vals[0] / float_vals[1];
                break;
            case MOP_MOD:
                int_vals[2] = int_vals[0] % int_vals[1];
                break;
        }
        if(is_int) {
            last_val = new ConstInt(module->int32_type, int_vals[2]);
        } else {
            last_val = new ConstFloat(module->float_type, float_vals[2]);
        }
        return;
    }
    // 若不是常量，进行计算，输出指令
    check_not_const_cal_type(val);
    if(val[0]->type == module->int32_type) {
        switch(ast.op) {
            case MOP_MUL:
                last_val = builder->create_mul(val[0], val[1]);
                break;
            case MOP_DIV:
                last_val = builder->create_div(val[0], val[1]);
                break;
            case MOP_MOD:
                last_val = builder->create_rem(val[0], val[1]);
                break;
        }
    } else {
        switch(ast.op) { // 浮点只有乘除，没有取模
            case MOP_MUL:
                last_val = builder->create_fmul(val[0], val[1]);
                break;
            case MOP_DIV:
                last_val = builder->create_fdiv(val[0], val[1]);
                break;
            default:
                break;
        }
    }
}

void IRGenerator::visit(UnaryExpAST& ast) {
    // 为常量算式
    if(use_const) {
        if(ast.pri_exp) {
            ast.pri_exp->accept(*this);
        } else if(ast.unary_exp) {
            ast.unary_exp->accept(*this);
            if(ast.op == UOP_MINUS) {
                // 是整型常量
                if(dynamic_cast<ConstInt*>(last_val)) {
                    auto temp = (ConstInt*)last_val;
                    temp->val = -temp->val;
                    last_val = temp;
                } else {
                    auto temp = (ConstFloat*)last_val;
                    temp->val = -temp->val;
                    last_val = temp;
                }
            }
        } else {
            cerr << "[ERROR] UnaryExpAST must have pri_exp or unary_exp!" << endl;
            exit(1);
        }
        return;
    }
    // 不是常量算式
    if(ast.pri_exp) {
        ast.pri_exp->accept(*this);
    } else if(ast.call) {
        ast.call->accept(*this);
    } else {
        ast.unary_exp->accept(*this);
        if(last_val->type == module->void_type) {
            return;
        } else if(last_val->type == module->int1_type) { // INT1-->INT32
            last_val = builder->create_zext(last_val, module->int32_type);
        }
        if(last_val->type == module->int32_type) {
            switch(ast.op) {
                case UOP_MINUS: // 取负，使用 sub 0 v1
                    last_val = builder->create_sub(new ConstInt(module->int32_type, 0), last_val);
                    break;
                case UOP_NOT: // 取反?，使用 icmp eq v1, 0
                    last_val = builder->create_icmp_eq(last_val, new ConstInt(module->int32_type, 0));
                    break;
                case UOP_ADD: // 取正，什么都不做
                    break;
            }
        } else {
        switch(ast.op) { // 同上
            case UOP_MINUS:
                last_val = builder->create_fsub(new ConstFloat(module->float_type, 0), last_val);
                break;
            case UOP_NOT:
                last_val = builder->create_fcmp_eq(last_val, new ConstFloat(module->float_type, 0));
                break;
            case UOP_ADD:
                break;
        }
        }
    }
}

void IRGenerator::visit(PrimaryExpAST& ast) {
    if(ast.exp) {
        ast.exp->accept(*this);
    } else if(ast.l_val) {
        ast.l_val->accept(*this);
    } else if(ast.number) {
        ast.number->accept(*this);
    }
}

void IRGenerator::visit(LValAST& ast) {
    bool is_true_l_val = require_l_val; // 是否真是作为左值
    require_l_val = false;
    auto var = scope->find_val_with_name(*ast.id);
    // 全局作用域内，一定使用常量，全局作用域下访问到LValAST，那么use_const一定被置为了true
    if(scope->is_global()) {
        // 不是数组，直接返回该常量
        if(ast.arrays.empty()) {
            last_val = var;
            return;
        }
        // 若是数组，则var一定是全局常量数组
        vector<int> index;
        for(auto& exp: ast.arrays) {
            exp->accept(*this);
            index.push_back(dynamic_cast<ConstInt*>(last_val)->val);
        }
        last_val = ((GlobalVariable*)var)->init_val; // 使用var的初始化数组查找常量元素
        for(int i: index) {
            // 某数组元素为ConstZero，则该数一定是0
            if(dynamic_cast<ConstZero*>(last_val)) {
                Type* array_type = last_val->type;
                // 找数组元素标签
                while(array_type->tid == TypeID::ArrayTy) {
                    array_type = static_cast<ArrayType*>(array_type)->element_type;
                }
                if(array_type == module->int32_type) {
                    last_val = new ConstInt(module->int32_type, 0);
                } else {
                    last_val = new ConstFloat(module->float_type, 0);
                }
                return;
            }
            if(dynamic_cast<ConstArray*>(last_val)) {
                last_val = ((ConstArray*)last_val)->const_array[i];
            }
        }
        return;
    }
    // 局部作用域
    if(var->type->tid == TypeID::IntegerTy || var->type->tid == TypeID::FloatTy) { // 说明为局部常量
        last_val = var;
        return;
    }
    // 不是常量那么var一定是指针类型
    Type* var_type = static_cast<PointerType*>(var->type)->pointed_type; // 所指的类型
    if(!ast.arrays.empty()) { // 说明是数组
        vector<Value*> idxs;
        for(auto& exp: ast.arrays) {
            exp->accept(*this);
            idxs.push_back(last_val);
        }
        // 当函数传入参数 i32*，会生成类型为 i32** 的局部变量，即此情况
        if(var_type->tid == TypeID::PointerTy) {
            var = builder->create_load(var);
        } else if(var_type->tid == TypeID::ArrayTy) {
            idxs.insert(idxs.begin(), new ConstInt(module->int32_type, 0));
        }
        var = builder->create_gep(var, idxs); // 获取的一定是指针类型
        var_type = ((PointerType* )var->type)->pointed_type;
    }
    // 指向的还是数组,那么一定是传数组参,数组若为x[2], 参数为int a[]，需要传i32* 
    if(var_type->tid == TypeID::ArrayTy) {
        last_val = builder->create_gep(var, {new ConstInt(module->int32_type, 0), new ConstInt(module->int32_type, 0)});
    } else if(!is_true_l_val) { // 如果不是取左值，那么load
        last_val = builder->create_load(var);
    } else { // 否则返回地址值
        last_val = var;
    }
}

void IRGenerator::visit(NumberAST& ast) {
    if(ast.is_int) {
        last_val = new ConstInt(module->int32_type, ast.int_val);
    } else {
        last_val = new ConstFloat(module->float_type, ast.float_val);
    }
}

void IRGenerator::visit(CallAST& ast) {
    auto fun = (Function*)scope->find_val_with_name(*ast.id);
    // 引用函数返回值
    if(fun->bb_list.size() && !is_single_exp) {
        is_single_exp = false;
    }
    vector<Value*> args;
    for(int i = 0; i < ast.func_arg_list.size(); i++) {
        ast.func_arg_list[i]->accept(*this);
        // 检查函数形参与实参类型是否匹配
        if(last_val->type == module->int32_type && fun->args[i]->type == module->float_type) {
            last_val = builder->create_sitofp(last_val, module->float_type);
        } else if(last_val->type == module->float_type && fun->args[i]->type == module->int32_type) {
            last_val = builder->create_fptosi(last_val, module->int32_type);
        }
        args.push_back(last_val);
    }
    last_val = builder->create_call(fun, args);
}

void IRGenerator::visit(RelExpAST& ast) {
    if(ast.rel_exp == nullptr) {
        ast.add_exp->accept(*this);
        return;
    }
    Value* val[2];
    ast.rel_exp->accept(*this);
    val[0] = last_val;
    ast.add_exp->accept(*this);
    val[1] = last_val;
    check_not_const_cal_type(val);
    if(val[0]->type == module->int32_type) {
        switch(ast.op) {
            case ROP_LTE:
                last_val = builder->create_icmp_le(val[0], val[1]);
                break;
            case ROP_LT:
                last_val = builder->create_icmp_lt(val[0], val[1]);
                break;
            case ROP_GT:
                last_val = builder->create_icmp_gt(val[0], val[1]);
                break;
            case ROP_GTE:
                last_val = builder->create_icmp_ge(val[0], val[1]);
                break;
        }
    } else {
        switch(ast.op) {
            case ROP_LTE:
                last_val = builder->create_fcmp_le(val[0], val[1]);
                break;
            case ROP_LT:
                last_val = builder->create_fcmp_lt(val[0], val[1]);
                break;
            case ROP_GT:
                last_val = builder->create_fcmp_gt(val[0], val[1]);
                break;
            case ROP_GTE:
                last_val = builder->create_fcmp_ge(val[0], val[1]);
                break;
        }
    }
}

void IRGenerator::visit(EqExpAST& ast) {
    if(ast.eq_exp == nullptr) {
        ast.rel_exp->accept(*this);
        return;
    }
    Value* val[2];
    ast.eq_exp->accept(*this);
    val[0] = last_val;
    ast.rel_exp->accept(*this);
    val[1] = last_val;
    check_not_const_cal_type(val);
    if(val[0]->type == module->int32_type) {
        switch(ast.op) {
            case EOP_EQ:
                last_val = builder->create_icmp_eq(val[0], val[1]);
                break;
            case EOP_NEQ:
                last_val = builder->create_icmp_ne(val[0], val[1]);
                break;
        }
    } else {
        switch(ast.op) {
            case EOP_EQ:
                last_val = builder->create_fcmp_eq(val[0], val[1]);
                break;
            case EOP_NEQ:
                last_val = builder->create_fcmp_ne(val[0], val[1]);
                break;
        }
    }
}

void IRGenerator::visit(LAndExpAST& ast) {
    if(ast.l_and_exp == nullptr) {
        ast.eq_exp->accept(*this);
        return;
    }
    auto true_bb_bak = true_bb; // 防止嵌套and导致原true_bb丢失。用于生成短路模块
    true_bb = new BasicBlock(module, to_string(id++), cur_func);
    ast.l_and_exp->accept(*this);

    if(last_val->type == module->int32_type) {
        last_val = builder->create_icmp_ne(last_val, new ConstInt(module->int32_type, 0));
    } else if(last_val->type == module->float_type) {
        last_val = builder->create_fcmp_ne(last_val, new ConstFloat(module->float_type, 0));
    }
    builder->create_cond_br(last_val, true_bb, false_bb);
    builder->cur_bb = true_bb;
    has_br = false;
    true_bb = true_bb_bak; // 还原原来的true模块

    ast.eq_exp->accept(*this);
}

void IRGenerator::visit(LOrExpAST& ast) {
    if(ast.l_or_exp == nullptr) {
        ast.l_and_exp->accept(*this);
        return;
    }
    auto false_bb_bak = false_bb; // 防止嵌套and导致原true_bb丢失。用于生成短路模块
    false_bb = new BasicBlock(module, to_string(id++), cur_func);
    ast.l_or_exp->accept(*this);
    if(last_val->type == module->int32_type) {
        last_val = builder->create_icmp_ne(last_val, new ConstInt(module->int32_type, 0));
    } else if(last_val->type == module->float_type) {
        last_val = builder->create_fcmp_ne(last_val, new ConstFloat(module->float_type, 0));
    }
    builder->create_cond_br(last_val, true_bb, false_bb);
    builder->cur_bb = false_bb;
    has_br = false;
    false_bb = false_bb_bak;

    ast.l_and_exp->accept(*this);
}