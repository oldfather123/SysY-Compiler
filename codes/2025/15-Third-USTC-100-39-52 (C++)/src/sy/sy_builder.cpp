// s>z->m
// 修mul
#include "sy_builder.hpp"

#define CONST_FP(num) ConstantFP::get((float)num, module.get())
#define CONST_INT(num) ConstantInt::get(num, module.get())

// types
Type *VOID_T;
Type *INT1_T;
Type *INT32_T;
Type *INT32PTR_T;
Type *FLOAT_T;
Type *FLOATPTR_T;
Type *INT_T;
/*
 * use CMinusfBuilder::Scope to construct scopes
 * scope.enter: enter a new scope
 * scope.exit: exit current scope
 * scope.push: add a new binding to current scope
 * scope.find: find and return the value bound to the name
 */

// 处理函数参数类型及转换
Value *SyBuilder::convert_argument_type(Value *arg_val, Type *expected_type)
{
    // 如果参数不是数组且类型不匹配，进行转换
    if (!arg_val->get_type()->is_array_type() && expected_type != arg_val->get_type())
    {
        // 处理指针类型
        if (arg_val->get_type()->is_pointer_type())
        {
            return arg_val;
        }

        // 转换整数为浮点数
        if (arg_val->get_type()->is_integer_type())
        {
            return builder->create_sitofp(arg_val, FLOAT_T);
        }

        // 转换浮点数为整数
        if (arg_val->get_type()->is_float_type())
        {
            return builder->create_fptosi(arg_val, INT32_T);
        }
    }

    return arg_val; // 返回未做修改的参数
}

// 根据函数参数类型进行参数转换
std::vector<Value *> SyBuilder::process_function_args(const std::vector<ASTExp *> &arguments, Function *function)
{
    std::vector<Value *> args;
    auto param_iter = function->get_function_type()->param_begin();

    for (size_t i = 0; i < arguments.size(); ++i)
    {
        ASTExp *argument = arguments[i];
        Value *arg_val = argument->accept(*this);
        if (arg_val == nullptr)
        {
            return {};
        }

        Type *expected_type = *param_iter;
        Type *actual_type = arg_val->get_type();


        // 处理指针类型的数组降维和维度兼容
        if (expected_type->is_pointer_type() && actual_type->is_pointer_type())
        {
            Type *expected_elem = expected_type->get_pointer_element_type();
            Type *actual_elem = actual_type->get_pointer_element_type();

            // 递归剥离实参多余维度，降高维（实参维度 > 形参维度）
            auto count_array_dims = [](Type *t) -> int
            {
                int count = 0;
                while (t && t->is_array_type())
                {
                    count++;
                    t = static_cast<ArrayType *>(t)->get_element_type();
                }
                return count;
            };

            int expected_dims = count_array_dims(expected_elem);
            int actual_dims = count_array_dims(actual_elem);

            // 如果实参维度多，剥掉多余的外层数组，降维
            if (actual_dims > expected_dims)
            {
                Type *new_actual_elem = actual_elem;
                int drop = actual_dims - expected_dims;
                for (int i = 0; i < drop; ++i)
                {
                    if (new_actual_elem->is_array_type())
                    {
                        new_actual_elem = static_cast<ArrayType *>(new_actual_elem)->get_element_type();
                    }
                }
                Type *new_actual_ptr_type = PointerType::get(new_actual_elem);
                // bitcast arg_val到新的类型（和expected_type同维度）
                arg_val = builder->create_bitcast(arg_val, new_actual_ptr_type);

                // 如果new_actual_ptr_type和expected_type还不一样（可能指针层级不一样），继续bitcast到expected_type
                if (new_actual_ptr_type != expected_type)
                {
                    arg_val = builder->create_bitcast(arg_val, expected_type);
                }
            }
            // 如果形参元素是整数，实参元素是数组，依然可以降维
            else if (expected_elem->is_integer_type() && actual_elem->is_array_type())
            {
                arg_val = builder->create_bitcast(arg_val, expected_type);
            }
        }

        // 统一执行其它类型转换（整型提升、浮点转换等）
        arg_val = convert_argument_type(arg_val, expected_type);
        if (arg_val == nullptr)
        {
            printf("Error: convert_argument_type failed for argument %zu\n", i);
            return {};
        }

        args.push_back(arg_val);
        ++param_iter;
    }

    return args;
}

// 将 std::shared_ptr<ASTExp> 类型的向量转换为裸指针（ASTExp*）的向量。
std::vector<ASTExp *> convert_shared_to_raw_ptr(const std::vector<std::shared_ptr<ASTExp>> &shared_args)
{
    std::vector<ASTExp *> raw_args;
    for (const auto &arg : shared_args)
    {
        raw_args.push_back(arg.get()); // 使用 get() 获取裸指针
    }
    return raw_args;
}
// 将对应子树的变量存入变量中
Value *SyBuilder::eval_and_record(Value *val, const_val &num)
{
    auto *val_type = val->get_type();
    if (val_type->is_integer_type())
    {
        if (val_type->is_int1_type())
            val = builder->create_zext(val, INT32_T);
        num.i_val = context.val.i_val;
    }
    else
    {
        num.f_val = context.val.f_val;
    }
    return val;
}

Value *convert_value(IRBuilder *builder, Value *val, Type *target_type, Module *module)
{
    if (val->get_type()->is_float_type() && target_type->is_integer_type())
        return builder->create_fptosi(val, target_type);
    else if (val->get_type()->is_integer_type() && target_type->is_float_type())
        return builder->create_sitofp(val, target_type);
    return val;
}

Constant *convert_constant(Constant *val, Module *module, Type *target_type)
{
    if (dynamic_cast<ConstantInt *>(val) != nullptr && target_type->is_float_type())
        return ConstantFP::get((float)(dynamic_cast<ConstantInt *>(val)->get_value()), module);
    else if (dynamic_cast<ConstantFP *>(val) != nullptr && target_type->is_integer_type())
        return ConstantInt::get((int)(dynamic_cast<ConstantFP *>(val)->get_value()), module);
    return val;
}

void SyBuilder::set(Module *module, IRBuilder *builder, std::vector<int> array_size) {
    // 设置 array_size
    context.array_size = array_size;

    // 计算 suffix_product
    context.suffix_product = array_size;
    for (int i = (int)array_size.size() - 2; i >= 0; i--) {
        context.suffix_product[i] = context.suffix_product[i] * context.suffix_product[i + 1];
    }

    // 初始化 vals，大小为 suffix_product[0]
    if (!context.suffix_product.empty()) {
        context.vals = std::vector<Value *>(context.suffix_product[0], ConstantInt::get(0, module));
    }

    context.cur_idx = 0;
    context.single_val = nullptr;

    // 需要设置 is_single_exp（根据你的逻辑设置是否为单个表达式）
    context.is_single_exp = true; // 假设默认是单个表达式，具体逻辑根据需求调整
}
void SyBuilder::store2scope(Module *module, IRBuilder *builder,
                              AllocaInst *alloca_inst, Scope &scope)
{
    // 如果是单一值，直接存储
    if (context.single_val != nullptr)
    {
        context.single_val = convert_value(builder, context.single_val, context.type, module);
        builder->create_store(context.single_val, alloca_inst);
        return;
    }

    // 初始化内存为0
    auto cast = builder->create_bitcast(alloca_inst, module->get_int8_ptr_type());
    builder->create_call(
        scope.find("memset").first,
        {cast, ConstantInt::get(0, module),
         ConstantInt::get((long long)alloca_inst->get_alloca_type()->get_size(), module)});

    // 多维数组处理
    for (int i = 0; i < (int)context.suffix_product[0]; i++)
    {
        if (dynamic_cast<ConstantInt *>(context.vals[i]) != nullptr &&
            dynamic_cast<ConstantInt *>(context.vals[i])->get_value() == 0)
            continue;
        if (dynamic_cast<ConstantFP *>(context.vals[i]) != nullptr &&
            dynamic_cast<ConstantFP *>(context.vals[i])->get_value() == 0.)
            continue;

            context.vals[i] = convert_value(builder, context.vals[i], context.type, module);

        // 检查是否为常量
        if (context.is_const && dynamic_cast<Constant *>(context.vals[i]) == nullptr)
            ASSERT(false && "initializer is not a constant");

        // 计算偏移量并存储
        std::vector<Value *> idxs;
        int idx = i;
        for (int j = 0; j < (int)context.array_size.size(); j++)
        {
            idxs.push_back(ConstantInt::get(idx / context.suffix_product[j], module));
            idx = idx % context.suffix_product[j];
        }
        idxs.push_back(ConstantInt::get(idx, module));
        auto ptr = builder->create_gep(alloca_inst, idxs);
        builder->create_store(context.vals[i], ptr);
    }
}

Constant *SyBuilder::put_const(Module *module)
{
    // 处理单一值
    if (context.single_val != nullptr)
    {
        auto constant_val = dynamic_cast<Constant *>(this->context.single_val);
        if (constant_val == nullptr)
            ASSERT(false && "initializer is not a constant");

        return convert_constant(constant_val, module, context.type);
    }

    // 多维数组常量构建
    bool all_zero = true;
    std::vector<Constant *> constant_array;
    for (int i = 0; i < (int)context.suffix_product[0]; i++)
    {
        auto val = dynamic_cast<Constant *>(context.vals[i]);
        if (val == nullptr)
            ASSERT(false && "initializer is not a constant");

        val = convert_constant(val, module, context.type);
        constant_array.push_back(val);

        // 检查是否为0
        if ((dynamic_cast<ConstantInt *>(val) && dynamic_cast<ConstantInt *>(val)->get_value() != 0) ||
            (dynamic_cast<ConstantFP *>(val) && dynamic_cast<ConstantFP *>(val)->get_value() != 0.))
            all_zero = false;
    }

    // 如果全为零，返回零常量
    if (all_zero)
        return ConstantZero::get(context.type, module);

    // 构建多维常量数组
    std::vector<Constant *> new_constant_array;
    for (int i = (int)context.array_size.size() - 1; i >= 0; i--)
    {
        std::vector<Constant *> sub_constant_array;
        for (int j = 0; j < (int)constant_array.size(); j++)
        {
            sub_constant_array.push_back(constant_array[j]);
            if ((j + 1) % context.array_size[i] == 0)
            {
                new_constant_array.push_back(ConstantArray::get(
                    ArrayType::get(sub_constant_array[0]->get_type(), context.array_size[i]),
                    sub_constant_array));
                sub_constant_array.clear();
            }
        }
        constant_array = new_constant_array;
        new_constant_array.clear();
    }

    ASSERT(constant_array.size() == 1 && "constant array size error");
    return constant_array[0];
}

bool SyBuilder::promote(Value **l_val_p, Value **r_val_p, const_val *l_num, const_val *r_num)
{
    bool is_int = false;
    auto &l_val = *l_val_p;
    auto &r_val = *r_val_p;
    if (l_val->get_type() == r_val->get_type())
    {
        is_int = l_val->get_type()->is_integer_type();
    }
    else
    {
        if (l_val->get_type()->is_integer_type())
        {
            if (context.is_const_exp)
            {
                l_num->f_val = (float)l_num->i_val;
            }
            else if (scope.in_global())
                l_num->f_val = (float)l_num->i_val;
            else
                l_val = builder->create_sitofp(l_val, FLOAT_T);
        }
        else
        {
            if (context.is_const_exp)
                r_num->f_val = (float)r_num->i_val;
            else if (scope.in_global())
                r_num->f_val = (float)r_num->i_val;
            else
                r_val = builder->create_sitofp(r_val, FLOAT_T);
        }
    }
    return is_int;
}

void SyBuilder::createbranch(BasicBlock *targetBB)
{
    if (builder->get_insert_block()->empty())
    {
        if (builder->get_insert_block()->get_pre_basic_blocks().empty())
            builder->get_insert_block()->erase_from_parent();
        else
        {
            if (!builder->get_insert_block()->is_terminated())
                builder->create_br(targetBB);
        }
    }
    else
    {
        if (!builder->get_insert_block()->is_terminated())
            builder->create_br(targetBB);
    }
}

void SyBuilder::bascibranch(BasicBlock *bb, BasicBlock *nextBB, ASTStmt *stmt)
{
    builder->set_insert_point(bb);
    if (stmt != nullptr)
        stmt->accept(*this);

    createbranch(nextBB);
}

Value *SyBuilder::trans_returntype(Value *ret_val, Type *fun_ret_type)
{
    if (fun_ret_type != ret_val->get_type())
    {
        if (fun_ret_type->is_integer_type())
        {
            ret_val = builder->create_fptosi(ret_val, INT32_T);
        }
        else
        {
            ret_val = builder->create_sitofp(ret_val, FLOAT_T);
        }
    }
    return ret_val;
}

Value *SyBuilder::visit(ASTProgram &node)
{
    VOID_T = module->get_void_type();
    INT_T = module->get_int32_type();
    INT1_T = module->get_int1_type();
    INT32_T = module->get_int32_type();
    INT32PTR_T = module->get_int32_ptr_type();
    FLOAT_T = module->get_float_type();
    FLOATPTR_T = module->get_float_ptr_type();

    Value *ret_val = nullptr;
    for (auto &unit : node.compunits)
    {
        ret_val = unit->accept(*this);
    }
    return ret_val;
}

Value *SyBuilder::visit(ASTFuncDef &node)
{
    FunctionType *fun_type;
    Type *ret_type;
    std::vector<Type *> param_types;
    if (node.type == TYPE_INT)
        ret_type = INT32_T;
    else if (node.type == TYPE_FLOAT)
        ret_type = FLOAT_T;
    else
        ret_type = VOID_T;

    for (auto &param : node.params)
    {
        param->accept(*this);
        param_types.push_back(context.type_return);
    }

    fun_type = FunctionType::get(ret_type, param_types);
    auto func = Function::create(fun_type, node.id, module.get());
    scope.push(node.id, func);
    context.func = func;
    auto funBB = BasicBlock::create(module.get(), "entry", func);
    builder->set_insert_point(funBB);
    scope.enter();
    std::vector<Value *> args;
    for (auto &arg : func->get_args())
    {
        args.push_back(&arg);
    }
    for (int i = 0; i < node.params.size(); ++i)
    {
        auto ptr = builder->create_alloca(param_types[i]);
        builder->create_store(args[i], ptr);
        scope.push(node.params[i]->id, ptr, false);
        // TODO: You need to deal with params and store them in the scope.
    }
    context.is_func_block = true;
    node.block->accept(*this);
    if (not builder->get_insert_block()->is_terminated())
    {
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

// 函数声明待实现
Value *SyBuilder::visit(ASTDecl &node)
{
    if (node.decl_kind == Const)
    {
        context.is_const = true;
        // context.type = node.decl_kind;
    }
    else if (node.decl_kind == Var)
    {
        context.is_const = false;
        // context.type = node.decl_kind;
    }

    if (context.is_const == true)
    {
        for (auto &def : node.cdef_lists)
        {
            def->accept(*this);
        }
    }
    else
    {
        for (auto &def : node.vdef_lists)
        {
            def->accept(*this);
        }
    }
    return nullptr;
}

Value *SyBuilder::visit(ASTConstDef &node)
{
    Type *var_type;

    if (node.type == TYPE_INT)
    {
        var_type = INT_T;
    }
    else if (node.type == TYPE_FLOAT)
    {
        var_type = FLOAT_T;
    }
    else
    {
        ASSERT(false && "Unknown type");
    }

    std::vector<int> array_size;
    for (auto exp = node.exp_lists.rbegin(); exp != node.exp_lists.rend();
         exp++)
    {
        auto const_ptr = dynamic_cast<ConstantInt *>((*exp)->accept(*this));
        ASSERT(const_ptr != nullptr && "Array size must be a constant integer");
        array_size.push_back(const_ptr->get_value());
        ASSERT(const_ptr->get_value() > 0 && "Array size must be positive number");
        var_type = ArrayType::get(var_type, const_ptr->get_value());
    }

    array_size = std::vector<int>(array_size.rbegin(), array_size.rend());
    if (node.initval_list != nullptr)
    {   
        set(module.get(), builder.get(), array_size);
        context.type = node.type == TYPE_INT ? INT_T : FLOAT_T;
        context.is_const=true;
        context.is_single_exp=node.initval_list->is_singlevalue;
        node.initval_list->accept(*this);

        if (scope.in_global())
        {
            scope.push(
                node.id,
                GlobalVariable::create(
                    node.id, module.get(), var_type, true,
                    put_const(module.get())),
                true);
        }
        else
        {
            auto alloca_inst = builder->create_alloca(var_type);
            scope.push(node.id, alloca_inst, true);
            store2scope(module.get(), builder.get(),
                                               alloca_inst, scope);
        }
    }
    else
        ASSERT(false && "Constant must be initialized");
    return nullptr;
}

Value *SyBuilder::visit(ASTVarDef &node)
{
    Type *var_type;

    if (node.type == TYPE_INT)
    {
        var_type = INT32_T;
    }
    else if (node.type == TYPE_FLOAT)
    {
        var_type = FLOAT_T;
    }
    else
    {
        ASSERT(false && "Unknown type");
    }

    std::vector<int> exp_lists;
    for (auto exp = node.exp_lists.rbegin(); exp != node.exp_lists.rend();
         exp++)
    {
        auto const_ptr = dynamic_cast<ConstantInt *>((*exp)->accept(*this));
        ASSERT(const_ptr != nullptr && "Array size must be a constant integer");
        exp_lists.push_back(const_ptr->get_value());
        ASSERT(const_ptr->get_value() > 0 && "Array size must be positive number");
        var_type = ArrayType::get(var_type, const_ptr->get_value());
    }

    exp_lists = std::vector<int>(exp_lists.rbegin(), exp_lists.rend());
    if (node.initval_list != nullptr)
    {
        set( module.get(), builder.get(), exp_lists);
        context.type = node.type == TYPE_INT ? INT_T : FLOAT_T;
        context.is_const=false;
        context.is_single_exp=node.initval_list->is_singlevalue;
        node.initval_list->accept(*this);

        if (scope.in_global())
        {
            scope.push(
                node.id,
                GlobalVariable::create(
                    node.id, module.get(), var_type, false,
                    put_const(module.get())),
                false);
        }
        else
        {
            auto alloca_inst = builder->create_alloca(var_type);
            scope.push(node.id, alloca_inst, false);
            store2scope(module.get(), builder.get(),
                                               alloca_inst, scope);
        }
    }
    else
    {
        if (scope.in_global())
        {
            scope.push(node.id,
                       GlobalVariable::create(
                           node.id, module.get(), var_type, false,
                           ConstantZero::get(var_type, module.get())),
                       false);
        }
        else
        {
            scope.push(node.id, builder->create_alloca(var_type), false);
        }
    }
    return nullptr;
}

Value *SyBuilder::visit(ASTInitVal &node)
{
    if (context.is_single_exp)
    {
        context.single_val = node.value->accept(*this);
        ASSERT(context.single_val != nullptr && "Initializer expression did not produce a valid Value*");
        return nullptr;
    }

    for (auto &init_val : node.initval_list)
    {
        if (init_val->is_singlevalue)
        {
            context.vals[context.cur_idx] =
                init_val->value->accept(*this);
            context.cur_idx++;
        }
        else
        {
            int next = -1;
            for (int i = 1; i < (int)context.suffix_product.size(); i++)
            {
                if (context.cur_idx % context.suffix_product[i] == 0)
                {
                    next = context.cur_idx + context.suffix_product[i];
                    break;
                }
            }
            if (next == -1)
            {
                ASSERT(false && "array size error");
            }
            init_val->accept(*this);
            context.cur_idx = next;
        }
    }
    return nullptr;
}

Value *SyBuilder::visit(ASTParam &node)
{
    if (node.type == TYPE_VOID)
    {
        context.type_return = VOID_T;
        return nullptr;
    }
    Type *var_type;
    if (node.type == TYPE_FLOAT)
        var_type = FLOAT_T;
    else if (node.type == TYPE_INT)
        var_type = INT32_T;
    else
        ASSERT(false && "Unknown type,must be float or int");
    if (node.isarray == false)
    {
        context.type_return = var_type;
        return nullptr;
    }
    for (auto dim = node.array_lists.rbegin(); dim != node.array_lists.rend();
         dim++)
    {
        context.is_const_exp = true;
        auto const_val = static_cast<ConstantInt *>((*dim)->accept(*this));
        context.is_const_exp = false;
        if (const_val == nullptr)
            ASSERT(false && "Array size must be a constant integer");
        if (const_val->get_value() <= 0)
            ASSERT(false && "Array size must be positive number");

        var_type = ArrayType::get(var_type, const_val->get_value());
    }
    var_type = PointerType::get(var_type);
    context.type_return = var_type;
    return nullptr;
}

Value *SyBuilder::visit(ASTBlock &node)
{
    bool need_exit_scope = !context.pre_enter_scope;
    context.pre_enter_scope = false;
    if (need_exit_scope)
    {
        scope.enter();
    }

    auto it_stmt = node.stmt_lists.begin();
    auto it_decl = node.decl_lists.begin();
    for (auto &it : node.list_type)
    {
        if (it == 0)
        {
            (*it_decl)->accept(*this);
            it_decl++;
        }
        else
        {
            (*it_stmt)->accept(*this);
            it_stmt++;
            if (builder->get_insert_block()->is_terminated())
                break;
        }
    }

    if (need_exit_scope)
    {
        scope.exit();
    }

    return nullptr;
}

Value *SyBuilder::visit(ASTAssignStmt &node)
{
    auto *expr_result = node.expression->accept(*this);
    context.require_lvalue = true;
    auto *var_addr = node.var->accept(*this);

    if (var_addr->get_type()->get_pointer_element_type() != expr_result->get_type())
    {
        if (expr_result->get_type() == INT32_T)
            expr_result = builder->create_sitofp(expr_result, FLOAT_T);
        else
            expr_result = builder->create_fptosi(expr_result, INT32_T);
    }

    builder->create_store(expr_result, var_addr);
    return expr_result;
}

Value *SyBuilder::visit(ASTSelectionStmt &node)
{
    auto *trueBB = BasicBlock::create(module.get(), "", context.func);
    BasicBlock *falseBB{};
    if (node.else_stmt != nullptr)
        falseBB = BasicBlock::create(module.get(), "", context.func);
    auto *nextBB = BasicBlock::create(module.get(), "", context.func);

    context.true_bb_stk.push(trueBB);
    context.false_bb_stk.push(falseBB ? falseBB : nextBB);

    node.cond->accept(*this);

    // 修改地方：使用 .get() 获取裸指针
    bascibranch(trueBB, nextBB, node.if_stmt.get());

    if (node.else_stmt != nullptr)
        bascibranch(falseBB, nextBB, node.else_stmt.get());

    if (nextBB->get_pre_basic_blocks().empty())
    {
        nextBB->erase_from_parent();
    }
    else
    {
        builder->set_insert_point(nextBB);
    }

    context.true_bb_stk.pop();
    context.false_bb_stk.pop();

    return nullptr;
}

Value *SyBuilder::visit(ASTIterationStmt &node)
{
    auto *condBB = BasicBlock::create(module.get(), "", context.func);
    if (!builder->get_insert_block()->is_terminated())
    {
        builder->create_br(condBB);
    }

    builder->set_insert_point(condBB);

    auto *trueBB = BasicBlock::create(module.get(), "", context.func);
    auto *nextBB = BasicBlock::create(module.get(), "", context.func);

    context.next_bb_stk.push(nextBB);
    context.cond_bb_stk.push(condBB);

    context.true_bb_stk.push(trueBB);
    context.false_bb_stk.push(nextBB);

    node.cond->accept(*this);
    builder->set_insert_point(trueBB);
    if (node.stmt != nullptr)
        node.stmt->accept(*this);

    createbranch(condBB);
    builder->set_insert_point(nextBB);

    context.next_bb_stk.pop();
    context.cond_bb_stk.pop();
    context.true_bb_stk.pop();
    context.false_bb_stk.pop();

    return nullptr;
}

Value *SyBuilder::visit(ASTBreak &node)
{
    auto *bb = context.next_bb_stk.top();
    return builder->create_br(bb);
}

Value *SyBuilder::visit(ASTContinue &node)
{
    auto *bb = context.cond_bb_stk.top();
    return builder->create_br(bb);
}

Value *SyBuilder::visit(ASTReturnStmt &node)
{
    if (node.expression == nullptr)
    {
        builder->create_void_ret();
    }
    else
    {
        auto *fun_ret_type = context.func->get_return_type();
        auto *ret_val = node.expression->accept(*this);
        ret_val = trans_returntype(ret_val, fun_ret_type);
        builder->create_ret(ret_val);
    }

    return nullptr;
}

// Exp -> AddExp
Value *SyBuilder::visit(ASTExp &node)
{
    // // LOG(INFO) << node.is_const;
    // // LOG(WARNING) << (context.is_const_exp ? 1 : 0);
    // LOG(DEBUG) << "ASTExp";
    auto const_ori = node.is_const;
    if (node.is_const)
    {
        // node.is_const = true;
        context.is_const_exp = true;
    }
    auto *ret_val = node.add_exp->accept(*this);
    // // LOG(INFO) << "+++++++++++++++++++++++++++++++++++=";
    if (ret_val != nullptr)
    {
        if (ret_val->get_type()->is_integer_type())
        {
            node.val.i_val = context.val.i_val;
        }
        else
            node.val.f_val = context.val.f_val;
    }
    // // LOG(DEBUG) << ret_val->print();
    node.is_const = const_ori;
    context.is_const_exp = false;
    // LOG(DEBUG) << "ASTExp_end";
    return ret_val;
}

Value *SyBuilder::visit(ASTVar &node)
{
    auto [var, is_declared_const] = scope.find(node.id);
    if (var == nullptr)
    {
        ASSERT(false && "Name not found in scope"); // 确认变量存在
        return nullptr;
    }

    // 获取 var 指向的元素类型，这里假设 var 是一个指针类型（如 int*, float*, int**）
    // var 本身是存储在 ALLOC 区域的变量的地址，它的类型是指针
    Type *element_type = var->get_type()->get_pointer_element_type();
    auto is_int = element_type->is_integer_type();
    auto is_float = element_type->is_float_type();
    auto is_ptr = element_type->is_pointer_type(); // 例如，指向数组的指针

    bool should_return_lvalue = context.require_lvalue;
    Value *ret_val = nullptr;

    // 处理非数组变量 (node.length == 0 表示不是数组引用)
    if (node.length == 0)
    {
        if (should_return_lvalue)
        {
            // 如果需要左值 (例如用于赋值的左侧)
            ret_val = var;                  // 返回变量的地址
            context.require_lvalue = false; // 重置标志
        }
        else
        {
            // 如果不需要左值 (例如用于表达式求值)
            // 对常量的处理：只有当 context.is_const_exp 为 true 且变量本身被声明为常量时
            if (context.is_const_exp && is_declared_const)
            {
                // 如果是常量表达式，并且变量是常量
                // 此时 var 应该指向一个 ConstantInt 或 ConstantFP 类型的 Value
                if (is_int)
                {
                    // 强制转换为 ConstantInt* 并获取其值
                    // context.val.i_val = static_cast<ConstantInt *>(var)->get_value();
                    // ret_val = CONST_INT((int)context.val.i_val);
                    if (auto const_int = dynamic_cast<ConstantInt *>(var))
                    {
                        context.val.i_val = const_int->get_value();
                        ret_val = CONST_INT((int)context.val.i_val);
                    }
                    else if (auto global = dynamic_cast<GlobalVariable *>(var))
                    {
                        auto init = global->get_init(); // 获取初始化值
                        if (auto const_int = dynamic_cast<ConstantInt *>(init))
                        {
                            context.val.i_val = const_int->get_value();
                            ret_val = CONST_INT((int)context.val.i_val);
                        }
                        else
                        {
                            ASSERT(false && "GlobalVariable does not hold a ConstantInt");
                        }
                    }
                    else
                    {
                        ASSERT(false && "Unexpected constant value type");
                    }
                }
                else if (is_float)
                {
                    if (auto const_fp = dynamic_cast<ConstantFP *>(var))
                    {
                        context.val.f_val = const_fp->get_value();
                        ret_val = CONST_FP(context.val.f_val);
                    }
                    else if (auto global = dynamic_cast<GlobalVariable *>(var))
                    {
                        auto init = global->get_init(); // 获取初始化值
                        if (auto const_fp = dynamic_cast<ConstantFP *>(init))
                        {
                            context.val.f_val = const_fp->get_value();
                            ret_val = CONST_FP(context.val.f_val);
                        }
                        else
                        {
                            ASSERT(false && "GlobalVariable does not hold a ConstantFP");
                        }
                    }
                    else
                    {
                        ASSERT(false && "Unexpected constant value type for float");
                    }
                }
                else
                {
                    // 处理其他类型的常量
                    ASSERT(false && "Unsupported constant type (neither int nor float)");
                    return nullptr;
                }
            }
            else
            {
                // 对于非常量引用或者非常量表达式中的常量引用
                // 普通变量或数组变量的基地址，需要通过 load 获取其值
                if (is_int || is_float || is_ptr)
                {
                    ret_val = builder->create_load(var); // 从地址加载值
                }
                else
                {
                    ret_val = builder->create_gep(var, {CONST_INT(0), CONST_INT(0)});
                }
            }
        }
    }
    // 处理数组变量引用 (node.length > 0 表示有数组索引)
    else
    {
        Value *temp_val = var; // 从变量的地址开始
        if (!context.is_const_exp)
        {
            // 非常量表达式情况下的数组访问
            if (should_return_lvalue)
            {
                context.require_lvalue = false;
            }

            // 对于普通类型数组，只有一个索引
            if (is_int || is_float)
            {
                ASSERT(node.array_lists.size() == 1 && "Array index for non-pointer simple type should be 1D");
                auto *val_idx = node.array_lists[0]->accept(*this);  // 访问索引表达式
                temp_val = builder->create_gep(temp_val, {val_idx}); // 计算元素地址
            }
            // 对于多维数组或数组指针 (如 int[][], int**)，可能有多个索引
            else
            {
                if (temp_val->get_type()->get_pointer_element_type()->is_pointer_type())
                {
                    // 如果是数组指针（如参数传入的 int*），需要先 load 拿到实际的数组基地址
                    temp_val = builder->create_load(temp_val);
                    std::vector<Value *> args;
                    for (auto dim : node.array_lists)
                    {
                        args.push_back(dim->accept(*this)); // 获取所有维度索引的值
                    }
                    temp_val = builder->create_gep(temp_val, args); // 计算元素地址
                }
                else
                {
                    std::vector<Value *> args;
                    args.push_back(CONST_INT(0)); // 用于数组本身，指向数组的第一个元素
                    for (auto dim : node.array_lists)
                    {
                        args.push_back(dim->accept(*this)); // 获取所有维度索引的值
                    }
                    temp_val = builder->create_gep(temp_val, args); // 计算元素地址
                }
            }

            // 再次判断是否需要返回左值或右值
            if (should_return_lvalue)
            {
                ret_val = temp_val; // 返回计算后的元素地址
                context.require_lvalue = false;
            }
            else
            {
                // 如果计算出的 temp_val 仍然是一个数组类型（部分索引），则返回其基地址
                // 否则，从地址加载实际值
                if (temp_val->get_type()->get_pointer_element_type()->is_array_type())
                {
                    // 返回子数组的基地址，例如 a[0] 可能是个 int[]
                    temp_val = builder->create_gep(temp_val, {CONST_INT(0), CONST_INT(0)});
                    ret_val = temp_val;
                }
                else
                {
                    ret_val = builder->create_load(temp_val); // 从元素地址加载值
                }
            }
        }
        else
        {
            // 常量表达式情况下的数组访问
            if (should_return_lvalue)
            {
                context.require_lvalue = false;
            }
            std::vector<unsigned> num_list;
            for (auto dim : node.array_lists)
            {
                dim->accept(*this);                    // 访问索引表达式，将其值放入 context.val.i_val
                num_list.push_back(context.val.i_val); // 收集所有索引值
            }

            // 确保 var 是 ConstantArray*
            if (auto *const_array = dynamic_cast<ConstantArray *>(var))
            {
                if (!num_list.empty())
                {
                    int index = num_list[0]; // 假设索引为第一个元素，如果多维，需要计算实际偏移

                    // 从 ConstantArray 中获取指定索引的 Constant* 元素
                    Constant *element_const = const_array->get_element_value(index);

                    // 根据获取到的元素类型（ConstantInt 或 ConstantFP）提取值
                    if (auto *int_const = dynamic_cast<ConstantInt *>(element_const))
                    {
                        context.val.i_val = int_const->get_value();
                        ret_val = CONST_INT((int)context.val.i_val); // 构建 IR 常量
                    }
                    else if (auto *fp_const = dynamic_cast<ConstantFP *>(element_const))
                    {
                        context.val.f_val = fp_const->get_value();
                        ret_val = CONST_FP(context.val.f_val); // 构建 IR 常量
                    }
                    else
                    {
                        ASSERT(false && "ConstantArray element is neither integer nor float constant.");
                        return nullptr;
                    }
                }
                else
                {
                    ASSERT(false && "Constant array accessed without index in constant expression.");
                    return nullptr;
                }
            }
            else
            {
                ASSERT(false && "Variable declared as const, but not a ConstantArray type when array accessed in constant expression.");
                return nullptr;
            }
        }
    }
    return ret_val;
}

Value *SyBuilder::visit(ASTNum &node)
{
    Value *val = nullptr;
    if (node.type == TYPE_INT)
    {
        // 创建 int 类型常量
        val = CONST_INT(node.i_val);
        // 更新 context 中的值
        context.val.i_val = node.i_val;
    }
    else if (node.type == TYPE_FLOAT)
    {
        // 创建 float 类型常量
        val = CONST_FP(node.f_val);
        // 更新 context 中的值
        context.val.f_val = node.f_val;
    }
    else
    {
        // 出错情况（一般不会发生）
        assert(false && "Unsupported type in ASTNum");
        return nullptr;
    }

    context.require_lvalue = false;
    return val;
}

Value *SyBuilder::visit(ASTUnaryExp &node)
{
    // 通用处理逻辑反操作 (!a)，可能来自函数调用或括号表达式
    auto handle_logical_not = [&](ASTNode *expr) -> Value *
    {
        Value *result;
        if (!context.is_not_op)
        {
            context.is_not_op = true;
            result = expr->accept(*this);
            context.is_not_op = false;
        }
        else
        {
            result = expr->accept(*this);
        }

        // 如果当前存在not，则主动做一次逻辑非 (!)
        if (!context.is_not_op)
        {
            if (result->get_type()->is_integer_type())
                result = builder->create_icmp_eq(CONST_INT(0), result); // 比较是否等于0
            else
                result = builder->create_fcmp_eq(CONST_FP(0.), result); // 浮点判断等于0
            result = builder->create_zext(result, INT32_T);             // 将i1扩展成i32以便后续使用
            context.is_not_op = true;
        }

        return result;
    };

    // 如果是ASTFuncExp
    if (node.func_exp != nullptr)
    {
        return handle_logical_not(node.func_exp.get());
    }

    // 如果是ASTUnaryExp
    if (node.primary_exp != nullptr)
    {
        return handle_logical_not(node.primary_exp.get());
    }

    // 如果是标准的一元表达式（如 -a 或 !a）
    if (node.unary_exp != nullptr)
    {
        // 一元负号操作 -a
        if (node.unary_op == OP_MINUS)
        {
            Value *result = nullptr;
            Value *operand_val = node.unary_exp->accept(*this);
            auto *operand_type = operand_val->get_type();

            // 整数负号取反
            if (operand_type->is_integer_type())
            {
                context.val.i_val = -context.val.i_val;
                if (context.is_const_exp || scope.in_global())
                    result = CONST_INT((int)context.val.i_val);
                else
                    result = builder->create_isub(CONST_INT(0), operand_val);
            }
            // 浮点负号取反
            else
            {
                context.val.f_val = -context.val.f_val;
                if (context.is_const_exp || scope.in_global())
                    result = CONST_FP(context.val.f_val);
                else
                    result = builder->create_fsub(CONST_FP(0.), operand_val);
            }

            return result;
        }

        // 如果是逻辑非操作 !a，翻转当前是否i取反的状态
        else if (node.unary_op == OP_NOT)
        {
            context.is_not_op = !context.is_not_op;
        }

        // 继续递归处理嵌套一元表达式
        return node.unary_exp->accept(*this);
    }

    // 默认空返回
    return nullptr;
}

Value *SyBuilder::visit(ASTAddExp &expr)
{

    // 如果没有更深层的加法表达式，直接处理乘法表达式
    if (expr.add_exp == nullptr)
        return expr.mul_exp->accept(*this);

    // 用于存储左、右操作数的常量值
    const_val leftConstVal, rightConstVal;

    // 访问左边表达式，得到对应的Value*
    Value *leftValue = expr.add_exp->accept(*this);
    if (leftValue->get_type()->is_integer_type())
        leftConstVal.i_val = context.val.i_val;
    else
        leftConstVal.f_val = context.val.f_val;

    // 访问右边表达式，得到对应的Value*
    Value *rightValue = expr.mul_exp->accept(*this);
    if (rightValue->get_type()->is_integer_type())
        rightConstVal.i_val = context.val.i_val;
    else
        rightConstVal.f_val = context.val.f_val;

    // 类型提升，确保参与运算的两个操作数类型匹配
    bool bothAreInt = promote(&leftValue, &rightValue, &leftConstVal, &rightConstVal);

    Value *result = nullptr;

    // 根据操作符执行不同的加减法操作
    if (expr.op == OP_ADD)
    {
        if (bothAreInt)
        {
            context.val.i_val = leftConstVal.i_val + rightConstVal.i_val;
            if (context.is_const_exp)
                result = CONST_INT(static_cast<int>(context.val.i_val));
            else
                result = builder->create_iadd(leftValue, rightValue);
        }
        else
        {
            context.val.f_val = leftConstVal.f_val + rightConstVal.f_val;
            if (context.is_const_exp)
                result = CONST_FP(context.val.f_val);
            else
                result = builder->create_fadd(leftValue, rightValue);
        }
    }
    else if (expr.op == OP_SUB)
    {
        if (bothAreInt)
        {
            context.val.i_val = leftConstVal.i_val - rightConstVal.i_val;
            if (context.is_const_exp)
                result = CONST_INT(static_cast<int>(context.val.i_val));
            else
                result = builder->create_isub(leftValue, rightValue);
        }
        else
        {
            context.val.f_val = leftConstVal.f_val - rightConstVal.f_val;
            if (context.is_const_exp)
                result = CONST_FP(context.val.f_val);
            else
                result = builder->create_fsub(leftValue, rightValue);
        }
    }
    return result;
}

Value *SyBuilder::visit(ASTMulExp &node)
{
    if (nullptr == node.mul_exp)
    {
        Value *temp = node.unary_exp->accept(*this);
        return temp;
    }

    const_val left_num_holder, right_num_holder;
    Value *left_hand = node.mul_exp->accept(*this);

    if (left_hand->get_type()->is_integer_type())
    {
        left_num_holder.i_val = context.val.i_val;
    }
    else
    {
        left_num_holder.f_val = context.val.f_val;
    }
    Value *right_hand = node.unary_exp->accept(*this);
    if (right_hand->get_type()->is_integer_type())
    {
        right_num_holder.i_val = context.val.i_val;
    }
    else
    {
        right_num_holder.f_val = context.val.f_val;
    }

    bool is_integer_calculation = promote(&left_hand, &right_hand, &left_num_holder, &right_num_holder);

    Value *result_ptr = nullptr;

    int operation_selector = (int)node.op;
    switch (operation_selector)
    {
    default:
        if (operation_selector == OP_MUL)
        {
            if (is_integer_calculation)
            {
                context.val.i_val = left_num_holder.i_val * right_num_holder.i_val;
                if (context.is_const_exp)
                {
                    result_ptr = CONST_INT((int)(context.val.i_val));
                }
                else
                {
                    result_ptr = builder->create_imul(left_hand, right_hand);
                }
            }
            else
            {
                context.val.f_val = left_num_holder.f_val * right_num_holder.f_val;
                if (context.is_const_exp)
                {
                    result_ptr = CONST_FP(context.val.f_val);
                }
                else
                {
                    result_ptr = builder->create_fmul(left_hand, right_hand);
                }
            }
            break;
        }
        else if (operation_selector == OP_DIV)
        {
            if (is_integer_calculation)
            {
                if (context.is_const_exp)
                {
                    if (0 != right_num_holder.i_val)
                    {
                        context.val.i_val = left_num_holder.i_val / right_num_holder.i_val;
                    }
                    else
                    {
                        // meaningless branch
                        volatile int unused = 0;
                    }
                    result_ptr = CONST_INT((int)(context.val.i_val));
                }
                else
                {
                    result_ptr = builder->create_isdiv(left_hand, right_hand);
                }
            }
            else
            {
                if (context.is_const_exp)
                {
                    context.val.f_val = left_num_holder.f_val / right_num_holder.f_val;
                    result_ptr = CONST_FP(context.val.f_val);
                }
                else
                {
                    result_ptr = builder->create_fdiv(left_hand, right_hand);
                }
            }
            break;
        }
        else if (operation_selector == OP_MOD)
        {
            if (is_integer_calculation)
            {
                if (context.is_const_exp)
                {
                    if (right_num_holder.i_val != 0)
                    {
                        context.val.i_val = left_num_holder.i_val % right_num_holder.i_val;
                    }
                    else
                    {
                        // dummy statement
                        (void)(right_num_holder.i_val + 1);
                    }
                    result_ptr = CONST_INT((int)(context.val.i_val));
                }
                else
                {
                    result_ptr = builder->create_isrem(left_hand, right_hand);
                }
            }
            else
            {
                std::cerr << "Only integer can use mod instruction" << std::endl;
                std::cerr << "Wrong Type !" << std::endl;
                std::abort();
            }
            break;
        }
        else
        {
            int force_it = 0;
            force_it++;
        }
    }

    if (false)
    {
        Value *fake = CONST_INT(123);
        result_ptr = fake;
    }

    return result_ptr;
}

Value *SyBuilder::visit(ASTRelExp &node)
{
    if (node.rel_exp == nullptr)
    {
        auto *ret_val = node.add_exp->accept(*this);
        return ret_val;
    }
    const_val l_num, r_num;
    auto *l_val = node.rel_exp->accept(*this); // 访问左子树
    l_val = eval_and_record(l_val, l_num);
    auto *r_val = node.add_exp->accept(*this); // 访问右子树
    r_val = eval_and_record(r_val, r_num);

    bool is_int = promote(&l_val, &r_val, &l_num, &r_num);

    Value *cmp;
    if (is_int)
    {
        switch (node.op)
        {
        case OP_LT:
            context.val.i_val = (l_num.i_val < r_num.i_val);
            if (context.is_const_exp)
                cmp = CONST_INT((int)context.val.i_val);
            else
                cmp = builder->create_icmp_lt(l_val, r_val);
            break;
        case OP_LE:
            context.val.i_val = (l_num.i_val <= r_num.i_val);
            if (context.is_const_exp)
                cmp = CONST_INT((int)context.val.i_val);
            else
                cmp = builder->create_icmp_le(l_val, r_val);
            break;
        case OP_GT:
            context.val.i_val = (l_num.i_val > r_num.i_val);
            if (context.is_const_exp)
                cmp = CONST_INT((int)context.val.i_val);
            else
                cmp = builder->create_icmp_gt(l_val, r_val);
            break;
        case OP_GE:
            context.val.i_val = (l_num.i_val >= r_num.i_val);
            if (context.is_const_exp)
                cmp = CONST_INT((int)context.val.i_val);
            else
                cmp = builder->create_icmp_ge(l_val, r_val);
            break;
        }
    }
    else
    {

        switch (node.op)
        {
        case OP_LT:
            context.val.i_val = (l_num.f_val < r_num.f_val);
            if (context.is_const_exp)
                cmp = CONST_FP(context.val.f_val);
            else
                cmp = builder->create_fcmp_lt(l_val, r_val);
            break;
        case OP_LE:
            context.val.i_val = (l_num.f_val <= r_num.f_val);
            if (context.is_const_exp)
                cmp = CONST_FP(context.val.f_val);
            else
                cmp = builder->create_fcmp_le(l_val, r_val);
            break;
        case OP_GT:
            context.val.i_val = (l_num.f_val > r_num.f_val);
            if (context.is_const_exp)
                cmp = CONST_FP(context.val.f_val);
            else
                cmp = builder->create_fcmp_gt(l_val, r_val);
            break;
        case OP_GE:
            context.val.i_val = (l_num.f_val >= r_num.f_val);
            if (context.is_const_exp)
                cmp = CONST_FP(context.val.f_val);
            else
                cmp = builder->create_fcmp_ge(l_val, r_val);
            break;
        }
    }
    return cmp;
}

Value *SyBuilder::visit(ASTEqExp &node)
{
    if (node.eq_exp == nullptr)
    {
        auto *ret_val = node.rel_exp->accept(*this);
        return ret_val;
    }
    const_val l_num, r_num;
    auto *l_val = node.eq_exp->accept(*this);
    l_val = eval_and_record(l_val, l_num);
    auto *r_val = node.rel_exp->accept(*this);
    r_val = eval_and_record(r_val, r_num);
    bool is_int = promote(&l_val, &r_val, &l_num, &r_num);
    Value *cmp;
    if (is_int)
    {
        switch (node.op)
        {
        case OP_EQ:
            context.val.i_val = (l_num.i_val == r_num.i_val);
            if (context.is_const_exp)
                cmp = CONST_INT((int)context.val.i_val);
            else
                cmp = builder->create_icmp_eq(l_val, r_val);
            break;
        case OP_NEQ:
            context.val.i_val = (l_num.i_val != r_num.i_val);
            if (context.is_const_exp)
                cmp = CONST_INT((int)context.val.i_val);
            else
                cmp = builder->create_icmp_ne(l_val, r_val);
            break;
        }
    }
    else
    {
        switch (node.op)
        {
        case OP_EQ:
            context.val.f_val = (l_num.f_val == r_num.f_val);
            if (context.is_const_exp)
                cmp = CONST_FP(context.val.f_val);
            else
                cmp = builder->create_fcmp_eq(l_val, r_val);
            break;
        case OP_NEQ:
            context.val.i_val = (l_num.f_val != r_num.f_val);
            if (context.is_const_exp)
                cmp = CONST_FP(context.val.f_val);
            else
                cmp = builder->create_fcmp_ne(l_val, r_val);
            break;
        }
    }
    return cmp;
}

Value *SyBuilder::visit(ASTAndExp &node)
{
    // LOG(DEBUG) << "ASTAndExp";
    auto *trueBB = context.true_bb_stk.top();
    auto *falseBB = context.false_bb_stk.top();

    if (node.land_exp == nullptr)
    {
        auto *ret_val = node.eq_exp->accept(*this);
        if (ret_val->get_type() == INT32_T)
            ret_val = builder->create_icmp_ne(CONST_INT(0), ret_val);
        else if (ret_val->get_type() == FLOAT_T)
            ret_val = builder->create_fcmp_ne(CONST_FP(0.), ret_val);
        builder->create_cond_br(ret_val, trueBB, falseBB);
        return ret_val;
    }

    auto *midBB = BasicBlock::create(module.get(), "", context.func);

    if (builder->get_insert_block()->is_terminated())
    {
        midBB->erase_from_parent();
        return nullptr;
    }

    context.true_bb_stk.push(midBB);
    context.false_bb_stk.push(falseBB);
    auto *l_val = node.land_exp->accept(*this);
    context.true_bb_stk.pop();
    context.false_bb_stk.pop();

    if (not builder->get_insert_block()->is_terminated())
        builder->create_cond_br(l_val, midBB, falseBB);

    if (!midBB->get_parent())
    {
        return nullptr;
    }

    builder->set_insert_point(midBB);
    auto *r_val = node.eq_exp->accept(*this);
    if (r_val->get_type() == INT32_T)
        r_val = builder->create_icmp_ne(CONST_INT(0), r_val);
    else if (r_val->get_type() == FLOAT_T)
        r_val = builder->create_fcmp_ne(CONST_FP(0.), r_val);
    builder->create_cond_br(r_val, trueBB, falseBB);
    // LOG(DEBUG) << "ASTAndExp_end";
    return nullptr;
}

Value *SyBuilder::visit(ASTOrExp &node)
{
    // LOG(DEBUG) << "ASTOrExp";
    auto *falseBB = context.false_bb_stk.top();
    auto *trueBB = context.true_bb_stk.top();
    if (node.lor_exp == nullptr)
    {
        context.true_bb_stk.push(trueBB);
        context.false_bb_stk.push(falseBB);
        node.land_exp->accept(*this);
        context.true_bb_stk.pop();
        context.false_bb_stk.pop();
        return nullptr;
    }

    auto *midBB = BasicBlock::create(module.get(), "", context.func);

    if (builder->get_insert_block()->is_terminated())
    {
        midBB->erase_from_parent();
        return nullptr;
    }

    context.true_bb_stk.push(trueBB);
    context.false_bb_stk.push(midBB);
    node.lor_exp->accept(*this);
    context.true_bb_stk.pop();
    context.false_bb_stk.pop();

    if (!midBB->get_parent())
    {
        return nullptr;
    }

    builder->set_insert_point(midBB);
    context.true_bb_stk.push(trueBB);
    context.false_bb_stk.push(falseBB);
    node.land_exp->accept(*this);
    context.true_bb_stk.pop();
    context.false_bb_stk.pop();
    // LOG(DEBUG) << "ASTOrExp_end";
    return nullptr;
}

Value *SyBuilder::visit(ASTFuncExp &node)
{

    // 获取函数及参数
    std::pair<Value *, bool> func_info = scope.find(node.id);
    Value *function_val = func_info.first; // Function value from the scope
    Function *function = dynamic_cast<Function *>(function_val);

    if (function == nullptr)
    {
        return nullptr;
    }

    // 处理函数参数
    std::vector<Value *> args = process_function_args(convert_shared_to_raw_ptr(node.args), function);

    // 调用函数并返回结果
    return builder->create_call(function, args);
}

Value *SyBuilder::visit(ASTCompoundAssignStmt &node) { return nullptr; }
