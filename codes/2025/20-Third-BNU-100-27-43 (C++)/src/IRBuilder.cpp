#include "IRBuilder.h"
#include <cassert>
#include <functional>
#include <iomanip>
#include <numeric>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <utility>

namespace MyIR
{

    std::shared_ptr<MyIR::Constant> MyIR::Constant::get_zero_initializer(std::shared_ptr<Type> type)
    {
        switch (type->get_type_id())
        {
        case TypeID::Integer:
        {
            auto int_type = std::static_pointer_cast<IntegerType>(type);
            return std::make_shared<ConstantInt>(int_type, 0);
        }
        case TypeID::Float:
        {
            auto float_type = std::static_pointer_cast<FloatType>(type);
            return std::make_shared<ConstantFloat>(float_type, 0.0);
        }
        case TypeID::Array:
        {
            auto array_type = std::static_pointer_cast<ArrayType>(type);
            return std::make_shared<ConstantArray>(array_type, std::vector<std::shared_ptr<Constant>>{});
        }
        case TypeID::Pointer:
        {
            auto ptr_type = std::static_pointer_cast<PointerType>(type);
            return std::make_shared<ConstantPointerNull>(ptr_type);
        }
        default:
            assert(false && "Unsupported type for zero initializer");
            return nullptr;
        }
    }
}

IRBuilder::IRBuilder()
{
    // +++ 初始化SysY库函数的参数类型信息 +++
    sylib_param_types_["putint"] = {TYPE_INT};
    sylib_param_types_["putch"] = {TYPE_INT};
    sylib_param_types_["putfloat"] = {TYPE_FLOAT};
    // 注意: 对于指针类型，我们关心的是其基类型，用于可能的转换
    // 但此处的简单样例主要问题在于 float/int 转换，因此只列出关键函数
    sylib_param_types_["_sysy_starttime"] = {TYPE_INT};
    sylib_param_types_["_sysy_stoptime"] = {TYPE_INT};

    initialize();
}

void IRBuilder::initialize()
{
    unit_ = nullptr;
    current_function_ = nullptr;
    current_block_ = nullptr;
    temp_reg_counter_ = 0;
    label_counter_ = 0;
    float_global_counter_ = 0;
    scoped_values_.clear();
    scoped_array_info_cache_.clear();
    loop_cond_blocks_.clear();
    loop_end_blocks_.clear();
    float_globals_.clear();
    function_definitions_.clear();
    local_var_allocas_.clear();
    float_const_declarations_ = "";
    block_is_terminated_ = false;
    current_function_ret_type_ = "";
}

std::unique_ptr<MyIR::IRUnit> IRBuilder::build(ASTNode *root)
{
    initialize();
    unit_ = std::make_unique<MyIR::IRUnit>();
    declare_sylib_functions();
    enter_ir_scope(); // Global scope
    visit(root);
    exit_ir_scope();
    return std::move(unit_);
}

void IRBuilder::enter_ir_scope()
{
    scoped_values_.emplace_back();
    scoped_array_info_cache_.emplace_back();
}

void IRBuilder::exit_ir_scope()
{
    if (!scoped_values_.empty())
    {
        scoped_values_.pop_back();
        scoped_array_info_cache_.pop_back();
    }
}

MyIR::Value *IRBuilder::find_value(const std::string &name)
{
    for (auto it = scoped_values_.rbegin(); it != scoped_values_.rend(); ++it)
    {
        if (it->count(name))
        {
            return it->at(name);
        }
    }
    auto it_global = unit_->global_variable_map.find(name);
    if (it_global != unit_->global_variable_map.end())
    {
        return it_global->second;
    }
    auto it_func = unit_->function_map.find(name);
    if (it_func != unit_->function_map.end())
    {
        return it_func->second;
    }
    return nullptr;
}

const ArrayInfo_IRBuilder *IRBuilder::find_array_info(const std::string &name)
{
    for (auto it = scoped_array_info_cache_.rbegin(); it != scoped_array_info_cache_.rend(); ++it)
    {
        if (it->count(name))
        {
            return &it->at(name);
        }
    }
    return nullptr;
}

std::shared_ptr<MyIR::Type> IRBuilder::get_myir_type(DataType dt)
{
    switch (dt)
    {
    case TYPE_INT:
        return std::make_shared<MyIR::IntegerType>(32);
    case TYPE_FLOAT:
        return std::make_shared<MyIR::FloatType>();
    case TYPE_VOID:
        return std::make_shared<MyIR::VoidType>();
    default:
        return std::make_shared<MyIR::IntegerType>(32);
    }
}

std::shared_ptr<MyIR::Type> IRBuilder::get_myir_type_from_dims(DataType base_dt, const std::vector<int> &dims)
{
    std::shared_ptr<MyIR::Type> current_type = get_myir_type(base_dt);
    for (auto it = dims.rbegin(); it != dims.rend(); ++it)
    {
        current_type = std::make_shared<MyIR::ArrayType>(current_type, *it);
    }
    return current_type;
}

std::string IRBuilder::new_temp_reg_name()
{
    return std::to_string(temp_reg_counter_++);
}

std::string IRBuilder::new_label_name(const std::string &prefix)
{
    return prefix + "." + std::to_string(label_counter_++);
}

MyIR::Value *IRBuilder::visit(ASTNode *node)
{
    if (!node)
        return nullptr;

    switch (node->type)
    {
    case NODE_COMP_UNIT:
        visitCompUnit(node);
        return nullptr;
    case NODE_FUNC_DEF:
        visitFuncDef(node);
        return nullptr;
    case NODE_BLOCK:
        visitBlock(node);
        return nullptr;
    case NODE_VAR_DECL:
    case NODE_ARRAY_DECL:
    case NODE_CONST_DECL:
    case NODE_CONST_ARRAY_DECL:
        visitVarDecl(node, true);
        return nullptr;
    case NODE_BIN_OP:
        return visitBinOp(node);
    case NODE_UNARY_OP:
        return visitUnaryOp(node);
    case NODE_LVAL:
        return visitLVal(node);
    case NODE_ASSIGN:
        return visitAssign(node);
    case NODE_CONST_EXPR:
        return visitConstExpr(node);
    case NODE_RETURN:
        visitReturn(node);
        return nullptr;
    case NODE_FUNC_CALL:
        return visitFuncCall(node);
    case NODE_IF:
        visitIf(node);
        return nullptr;
    case NODE_WHILE:
        visitWhile(node);
        return nullptr;
    case NODE_EXPR_STMT:
        visitExprStmt(node);
        return nullptr;
    case NODE_BREAK:
        visitBreak(node);
        return nullptr;
    case NODE_CONTINUE:
        visitContinue(node);
        return nullptr;
    case NODE_TYPE_CAST:
        return visitTypeCast(node);
    case NODE_INIT_VAL:
        return visitInitVal(node);
    case NODE_CONST_INIT:
        return visitConstInit(node);
    case NODE_INIT_LIST:
    case NODE_CONST_INIT_LIST:
        return nullptr;
    default:
        return nullptr;
    }
}

void IRBuilder::declare_sylib_functions()
{
    // getint(): i32
    auto getint_type = std::make_shared<MyIR::FunctionType>(std::make_shared<MyIR::IntegerType>(32), std::vector<std::shared_ptr<MyIR::Type>>{});
    auto getint_func = std::make_shared<MyIR::Function>(getint_type, "getint", false, true);
    unit_->functions.push_back(getint_func);
    unit_->function_map["getint"] = getint_func.get();

    // getch(): i32
    auto getch_type = std::make_shared<MyIR::FunctionType>(std::make_shared<MyIR::IntegerType>(32), std::vector<std::shared_ptr<MyIR::Type>>{});
    auto getch_func = std::make_shared<MyIR::Function>(getch_type, "getch", false, true);
    unit_->functions.push_back(getch_func);
    unit_->function_map["getch"] = getch_func.get();

    // getarray(i32*): i32
    auto getarray_param_type = std::make_shared<MyIR::PointerType>(std::make_shared<MyIR::IntegerType>(32));
    auto getarray_type = std::make_shared<MyIR::FunctionType>(std::make_shared<MyIR::IntegerType>(32), std::vector<std::shared_ptr<MyIR::Type>>{getarray_param_type});
    auto getarray_func = std::make_shared<MyIR::Function>(getarray_type, "getarray", false, true);
    unit_->functions.push_back(getarray_func);
    unit_->function_map["getarray"] = getarray_func.get();

    // getfloat(): float
    auto getfloat_type = std::make_shared<MyIR::FunctionType>(std::make_shared<MyIR::FloatType>(), std::vector<std::shared_ptr<MyIR::Type>>{});
    auto getfloat_func = std::make_shared<MyIR::Function>(getfloat_type, "getfloat", false, true);
    unit_->functions.push_back(getfloat_func);
    unit_->function_map["getfloat"] = getfloat_func.get();

    // getfarray(float*): i32
    auto getfarray_param_type = std::make_shared<MyIR::PointerType>(std::make_shared<MyIR::FloatType>());
    auto getfarray_type = std::make_shared<MyIR::FunctionType>(std::make_shared<MyIR::IntegerType>(32), std::vector<std::shared_ptr<MyIR::Type>>{getfarray_param_type});
    auto getfarray_func = std::make_shared<MyIR::Function>(getfarray_type, "getfarray", false, true);
    unit_->functions.push_back(getfarray_func);
    unit_->function_map["getfarray"] = getfarray_func.get();

    // putint(i32): void
    auto putint_param_type = std::make_shared<MyIR::IntegerType>(32);
    auto putint_type = std::make_shared<MyIR::FunctionType>(std::make_shared<MyIR::VoidType>(), std::vector<std::shared_ptr<MyIR::Type>>{putint_param_type});
    auto putint_func = std::make_shared<MyIR::Function>(putint_type, "putint", false, true);
    unit_->functions.push_back(putint_func);
    unit_->function_map["putint"] = putint_func.get();

    // putch(i32): void
    auto putch_param_type = std::make_shared<MyIR::IntegerType>(32);
    auto putch_type = std::make_shared<MyIR::FunctionType>(std::make_shared<MyIR::VoidType>(), std::vector<std::shared_ptr<MyIR::Type>>{putch_param_type});
    auto putch_func = std::make_shared<MyIR::Function>(putch_type, "putch", false, true);
    unit_->functions.push_back(putch_func);
    unit_->function_map["putch"] = putch_func.get();

    // putarray(i32, i32*): void
    auto putarray_param1_type = std::make_shared<MyIR::IntegerType>(32);
    auto putarray_param2_type = std::make_shared<MyIR::PointerType>(std::make_shared<MyIR::IntegerType>(32));
    auto putarray_type = std::make_shared<MyIR::FunctionType>(std::make_shared<MyIR::VoidType>(), std::vector<std::shared_ptr<MyIR::Type>>{putarray_param1_type, putarray_param2_type});
    auto putarray_func = std::make_shared<MyIR::Function>(putarray_type, "putarray", false, true);
    unit_->functions.push_back(putarray_func);
    unit_->function_map["putarray"] = putarray_func.get();

    // putfloat(float): void
    auto putfloat_param_type = std::make_shared<MyIR::FloatType>();
    auto putfloat_type = std::make_shared<MyIR::FunctionType>(std::make_shared<MyIR::VoidType>(), std::vector<std::shared_ptr<MyIR::Type>>{putfloat_param_type});
    auto putfloat_func = std::make_shared<MyIR::Function>(putfloat_type, "putfloat", false, true);
    unit_->functions.push_back(putfloat_func);
    unit_->function_map["putfloat"] = putfloat_func.get();

    // putfarray(i32, float*): void
    auto putfarray_param1_type = std::make_shared<MyIR::IntegerType>(32);
    auto putfarray_param2_type = std::make_shared<MyIR::PointerType>(std::make_shared<MyIR::FloatType>());
    auto putfarray_type = std::make_shared<MyIR::FunctionType>(std::make_shared<MyIR::VoidType>(), std::vector<std::shared_ptr<MyIR::Type>>{putfarray_param1_type, putfarray_param2_type});
    auto putfarray_func = std::make_shared<MyIR::Function>(putfarray_type, "putfarray", false, true);
    unit_->functions.push_back(putfarray_func);
    unit_->function_map["putfarray"] = putfarray_func.get();

    // putf(i8*, ...): void
    auto putf_param_type = std::make_shared<MyIR::PointerType>(std::make_shared<MyIR::IntegerType>(8));
    auto putf_type = std::make_shared<MyIR::FunctionType>(std::make_shared<MyIR::VoidType>(), std::vector<std::shared_ptr<MyIR::Type>>{putf_param_type}, true);
    auto putf_func = std::make_shared<MyIR::Function>(putf_type, "putf", false, true);
    unit_->functions.push_back(putf_func);
    unit_->function_map["putf"] = putf_func.get();

    // _sysy_starttime(i32): void
    auto starttime_param_type = std::make_shared<MyIR::IntegerType>(32);
    auto starttime_type = std::make_shared<MyIR::FunctionType>(std::make_shared<MyIR::VoidType>(), std::vector<std::shared_ptr<MyIR::Type>>{starttime_param_type});
    auto starttime_func = std::make_shared<MyIR::Function>(starttime_type, "_sysy_starttime", false, true);
    unit_->functions.push_back(starttime_func);
    unit_->function_map["_sysy_starttime"] = starttime_func.get();

    // _sysy_stoptime(i32): void
    auto stoptime_param_type = std::make_shared<MyIR::IntegerType>(32);
    auto stoptime_type = std::make_shared<MyIR::FunctionType>(std::make_shared<MyIR::VoidType>(), std::vector<std::shared_ptr<MyIR::Type>>{stoptime_param_type});
    auto stoptime_func = std::make_shared<MyIR::Function>(stoptime_type, "_sysy_stoptime", false, true);
    unit_->functions.push_back(stoptime_func);
    unit_->function_map["_sysy_stoptime"] = stoptime_func.get();

    // llvm.memset.p0i8.i64(i8*, i8, i64, i1): void
    auto i8ptr_type = std::make_shared<MyIR::PointerType>(std::make_shared<MyIR::IntegerType>(8));
    auto i8_type = std::make_shared<MyIR::IntegerType>(8);
    auto i64_type = std::make_shared<MyIR::IntegerType>(64);
    auto i1_type = std::make_shared<MyIR::IntegerType>(1);
    auto memset_type = std::make_shared<MyIR::FunctionType>(std::make_shared<MyIR::VoidType>(), std::vector<std::shared_ptr<MyIR::Type>>{i8ptr_type, i8_type, i64_type, i1_type});
    auto memset_func = std::make_shared<MyIR::Function>(memset_type, "llvm.memset.p0i8.i64", false, true);
    unit_->functions.push_back(memset_func);
    unit_->function_map["llvm.memset.p0i8.i64"] = memset_func.get();
}

void IRBuilder::visitCompUnit(ASTNode *node)
{
    for (ASTNode *child = node->comp.children; child; child = child->next)
    {
        if (child->type == NODE_FUNC_DEF)
        {
            function_definitions_[child->func_def.name] = child;
        }
    }

    for (ASTNode *child = node->comp.children; child; child = child->next)
    {
        if (child->type == NODE_VAR_DECL || child->type == NODE_CONST_DECL ||
            child->type == NODE_ARRAY_DECL || child->type == NODE_CONST_ARRAY_DECL)
        {
            visitVarDecl(child, false);
        }
        else
        {
            visit(child);
        }
    }
}

void IRBuilder::visitFuncDef(ASTNode *node)
{
    enter_scope(node->func_def.name, 0);
    enter_ir_scope();
    temp_reg_counter_ = 0;
    local_var_allocas_.clear();

    block_is_terminated_ = false;

    auto ret_type = get_myir_type(node->func_def.ret_type);
    current_function_ret_type_ = get_type_string(ret_type.get());

    std::vector<std::shared_ptr<MyIR::Type>> param_types;
    std::vector<std::pair<std::string, ASTNode *>> param_info;
    for (ASTNode *p = node->func_def.params; p; p = p->next)
    {
        std::shared_ptr<MyIR::Type> param_type;
        if (p->type == NODE_ARRAY_DECL)
        {
            param_type = std::make_shared<MyIR::PointerType>(get_myir_type(p->data_type));
        }
        else
        {
            param_type = get_myir_type(p->data_type);
        }
        param_types.push_back(param_type);
        param_info.emplace_back(p->decl.name, p);
    }

    auto func_type = std::make_shared<MyIR::FunctionType>(ret_type, param_types);
    auto func = std::make_shared<MyIR::Function>(func_type, node->func_def.name, false, false);
    unit_->functions.push_back(func);
    unit_->function_map[node->func_def.name] = func.get();
    current_function_ = func.get();

    auto entry_block = std::make_shared<MyIR::BasicBlock>("entry", current_function_);
    current_function_->get_basic_blocks().push_back(entry_block);
    current_block_ = entry_block.get();

    for (size_t i = 0; i < param_info.size(); ++i)
    {
        const auto &p_info = param_info[i];
        ASTNode *p_node = p_info.second;
        std::string p_name = p_info.first;

        MyIR::Argument *arg = current_function_->get_arguments()[i].get();
        arg->set_name(p_name);

        auto alloca_inst = new MyIR::AllocaInst(arg->get_type(), p_name + ".addr", current_block_);
        current_block_->get_instructions().emplace_back(alloca_inst);
        auto store_inst = new MyIR::StoreInst(arg, alloca_inst, current_block_);
        current_block_->get_instructions().emplace_back(store_inst);
        scoped_values_.back()[p_name] = alloca_inst;

        if (p_node->type == NODE_ARRAY_DECL)
        {
            ArrayInfo_IRBuilder info;
            info.is_param = true;
            info.dimensions.push_back(0);
            ASTNode *dim_node = p_node->decl.dims->array_dims.next_dim;
            while (dim_node)
            {
                info.dimensions.push_back(evaluate_constant_expression(dim_node->array_dims.size));
                dim_node = dim_node->array_dims.next_dim;
            }
            scoped_array_info_cache_.back()[p_name] = info;
        }
    }

    std::vector<ASTNode *> local_decls;
    if (node->func_def.body)
    {
        findLocalDecls(node->func_def.body, local_decls);
    }

    for (ASTNode *decl_node : local_decls)
    {
        std::shared_ptr<MyIR::Type> alloc_type;
        if (decl_node->type == NODE_ARRAY_DECL || decl_node->type == NODE_CONST_ARRAY_DECL)
        {
            std::vector<int> dimensions;
            ASTNode *dim_node = decl_node->decl.dims;
            while (dim_node)
            {
                dimensions.push_back(evaluate_constant_expression(dim_node->array_dims.size));
                dim_node = dim_node->array_dims.next_dim;
            }
            alloc_type = get_myir_type_from_dims(decl_node->data_type, dimensions);
        }
        else
        {
            alloc_type = get_myir_type(decl_node->data_type);
        }
        auto alloca_inst = new MyIR::AllocaInst(alloc_type, new_temp_reg_name(), current_block_);
        current_block_->get_instructions().emplace_back(alloca_inst);
        local_var_allocas_[decl_node] = alloca_inst;
    }

    if (node->func_def.body)
    {
        visit(node->func_def.body);
    }

    if (!block_is_terminated_)
    {
        if (ret_type->get_type_id() == MyIR::TypeID::Void)
        {
            current_block_->get_instructions().emplace_back(new MyIR::ReturnInst(current_block_));
        }
        else
        {
            MyIR::Constant *zero;
            if (ret_type->get_type_id() == MyIR::TypeID::Float)
            {
                zero = new MyIR::ConstantFloat(std::static_pointer_cast<MyIR::FloatType>(ret_type), 0.0);
            }
            else
            {
                zero = new MyIR::ConstantInt(std::static_pointer_cast<MyIR::IntegerType>(ret_type), 0);
            }
            current_block_->get_instructions().emplace_back(new MyIR::ReturnInst(zero, current_block_));
        }
    }
    exit_ir_scope();
    exit_scope();
}

void IRBuilder::visitVarDecl(ASTNode *node, bool is_local)
{
    std::string var_name = node->decl.name;
    DataType base_dt = node->data_type;
    bool is_const = (node->type == NODE_CONST_DECL || node->type == NODE_CONST_ARRAY_DECL);
    bool is_array = (node->type == NODE_ARRAY_DECL || node->type == NODE_CONST_ARRAY_DECL);

    std::vector<int> dimensions;
    if (is_array)
    {
        ASTNode *dim_node = node->decl.dims;
        while (dim_node)
        {
            dimensions.push_back(evaluate_constant_expression(dim_node->array_dims.size));
            dim_node = dim_node->array_dims.next_dim;
        }
    }

    if (!is_local)
    {
        auto value_type = is_array ? get_myir_type_from_dims(base_dt, dimensions) : get_myir_type(base_dt);
        std::shared_ptr<MyIR::Constant> initializer;

        if (node->decl.init)
        {
            if (is_array)
            {
                initializer = generate_constant_array_initializer(node->decl.init, dimensions, base_dt);
            }
            else
            {
                if (base_dt == TYPE_FLOAT)
                {
                    double val = evaluate_float_constant_expression(node->decl.init);
                    initializer = std::make_shared<MyIR::ConstantFloat>(std::static_pointer_cast<MyIR::FloatType>(value_type), val);
                }
                else
                {
                    int val = evaluate_constant_expression(node->decl.init);
                    initializer = std::make_shared<MyIR::ConstantInt>(std::static_pointer_cast<MyIR::IntegerType>(value_type), val);
                }
            }
        }
        else
        {
            assert(!is_const && "Global constant must be initialized.");
            initializer = MyIR::Constant::get_zero_initializer(value_type);
        }

        auto linkage = is_const ? MyIR::LinkageType::Constant : (node->decl.init ? MyIR::LinkageType::Global : MyIR::LinkageType::Common);
        auto gv = std::make_shared<MyIR::GlobalVariable>(var_name, value_type, linkage, initializer, false, false, std::nullopt);
        unit_->global_variables.push_back(gv);
        unit_->global_variable_map[var_name] = gv.get();
        if (is_array)
        {
            ArrayInfo_IRBuilder info = {dimensions, false};
            scoped_array_info_cache_.front()[var_name] = info;
        }
    }
    else
    {
        assert(local_var_allocas_.count(node));
        auto ptr = local_var_allocas_.at(node);
        scoped_values_.back()[var_name] = ptr;

        if (is_array)
        {
            ArrayInfo_IRBuilder info = {dimensions, false};
            scoped_array_info_cache_.back()[var_name] = info;
        }

        if (node->decl.init)
        {
            if (is_array)
            {
                auto array_ptr_type = std::dynamic_pointer_cast<MyIR::PointerType>(ptr->get_type());
                auto array_type = std::dynamic_pointer_cast<MyIR::ArrayType>(array_ptr_type->get_element_type());
                generate_array_initialization(ptr, node->decl.init, dimensions, get_myir_type(base_dt));
            }
            else
            {
                MyIR::Value *init_val = visit(node->decl.init);
                DataType init_ast_type = node->decl.init->data_type;
                DataType dest_ast_type = node->data_type;

                if (init_ast_type != dest_ast_type)
                {
                    MyIR::Opcode op;
                    bool needs_cast = true;
                    if (init_ast_type == TYPE_INT && dest_ast_type == TYPE_FLOAT)
                    {
                        op = MyIR::Opcode::SIToFP;
                    }
                    else if (init_ast_type == TYPE_FLOAT && dest_ast_type == TYPE_INT)
                    {
                        op = MyIR::Opcode::FPToSI;
                    }
                    else
                    {
                        needs_cast = false;
                    }

                    if (needs_cast)
                    {
                        auto dest_ir_type = get_myir_type(dest_ast_type);
                        auto cast_inst = new MyIR::CastInst(op, init_val, dest_ir_type, new_temp_reg_name(), current_block_);
                        current_block_->get_instructions().emplace_back(cast_inst);
                        init_val = cast_inst;
                    }
                }

                current_block_->get_instructions().emplace_back(new MyIR::StoreInst(init_val, ptr, current_block_));
            }
        }
    }
}

void IRBuilder::visitBlock(ASTNode *node)
{
    bool new_scope = node->block.scope != nullptr;
    if (new_scope)
    {
        enter_scope(node->block.scope->name, 1);
        enter_ir_scope();
    }

    for (ASTNode *item = node->block.items; item; item = item->next)
    {
        if (block_is_terminated_)
        {
            break;
        }
        visit(item);
    }

    if (new_scope)
    {
        exit_ir_scope();
        exit_scope();
    }
}

MyIR::Value *IRBuilder::visitBinOp(ASTNode *node)
{
    if (node->bin_op.op == OP_OR || node->bin_op.op == OP_AND)
    {
        bool is_or = (node->bin_op.op == OP_OR);
        auto right_eval_block = new MyIR::BasicBlock(new_label_name("sc.eval_right"), current_function_);
        auto end_block = new MyIR::BasicBlock(new_label_name("sc.end"), current_function_);

        auto result_alloca = new MyIR::AllocaInst(std::make_shared<MyIR::IntegerType>(1), new_temp_reg_name(), current_block_);
        current_block_->get_instructions().emplace_back(result_alloca);

        MyIR::Value *left_val = visit(node->bin_op.left);
        MyIR::Value *left_bool;
        if (MyIR::Type::is_float_ty(left_val->get_type().get()))
        {
            auto zero = new MyIR::ConstantFloat(std::static_pointer_cast<MyIR::FloatType>(left_val->get_type()), 0.0);
            left_bool = new MyIR::CmpInst(std::make_shared<MyIR::IntegerType>(1), MyIR::CmpPredicate::ONE, left_val, zero, new_temp_reg_name(), current_block_);
        }
        else
        {
            auto zero = new MyIR::ConstantInt(std::static_pointer_cast<MyIR::IntegerType>(left_val->get_type()), 0);
            left_bool = new MyIR::CmpInst(std::make_shared<MyIR::IntegerType>(1), MyIR::CmpPredicate::NE, left_val, zero, new_temp_reg_name(), current_block_);
        }
        current_block_->get_instructions().emplace_back(dynamic_cast<MyIR::Instruction *>(left_bool));
        current_block_->get_instructions().emplace_back(new MyIR::StoreInst(left_bool, result_alloca, current_block_));

        if (is_or)
        {
            current_block_->get_instructions().emplace_back(new MyIR::BranchInst(left_bool, end_block, right_eval_block, current_block_));
        }
        else
        {
            current_block_->get_instructions().emplace_back(new MyIR::BranchInst(left_bool, right_eval_block, end_block, current_block_));
        }

        current_function_->get_basic_blocks().push_back(std::shared_ptr<MyIR::BasicBlock>(right_eval_block));
        current_block_ = right_eval_block;
        block_is_terminated_ = false;

        MyIR::Value *right_val = visit(node->bin_op.right);
        MyIR::Value *right_bool;
        if (MyIR::Type::is_float_ty(right_val->get_type().get()))
        {
            auto zero = new MyIR::ConstantFloat(std::static_pointer_cast<MyIR::FloatType>(right_val->get_type()), 0.0);
            right_bool = new MyIR::CmpInst(std::make_shared<MyIR::IntegerType>(1), MyIR::CmpPredicate::ONE, right_val, zero, new_temp_reg_name(), current_block_);
        }
        else
        {
            auto zero = new MyIR::ConstantInt(std::static_pointer_cast<MyIR::IntegerType>(right_val->get_type()), 0);
            right_bool = new MyIR::CmpInst(std::make_shared<MyIR::IntegerType>(1), MyIR::CmpPredicate::NE, right_val, zero, new_temp_reg_name(), current_block_);
        }
        current_block_->get_instructions().emplace_back(dynamic_cast<MyIR::Instruction *>(right_bool));
        current_block_->get_instructions().emplace_back(new MyIR::StoreInst(right_bool, result_alloca, current_block_));
        current_block_->get_instructions().emplace_back(new MyIR::BranchInst(end_block, current_block_));

        current_function_->get_basic_blocks().push_back(std::shared_ptr<MyIR::BasicBlock>(end_block));
        current_block_ = end_block;
        block_is_terminated_ = false;

        auto final_bool = new MyIR::LoadInst(result_alloca, new_temp_reg_name(), current_block_);
        current_block_->get_instructions().emplace_back(final_bool);
        auto zext_inst = new MyIR::CastInst(MyIR::Opcode::ZExt, final_bool, std::make_shared<MyIR::IntegerType>(32), new_temp_reg_name(), current_block_);
        current_block_->get_instructions().emplace_back(zext_inst);
        node->data_type = TYPE_INT;
        return zext_inst;
    }

    MyIR::Value *left = visit(node->bin_op.left);
    MyIR::Value *right = visit(node->bin_op.right);

    auto left_type = left->get_type();
    auto right_type = right->get_type();

    bool is_float_op = MyIR::Type::is_float_ty(left_type.get()) || MyIR::Type::is_float_ty(right_type.get());
    std::shared_ptr<MyIR::Type> op_type;

    if (is_float_op)
    {
        op_type = std::make_shared<MyIR::FloatType>();
        if (MyIR::Type::is_integer_ty(left_type.get()))
        {
            auto cast = new MyIR::CastInst(MyIR::Opcode::SIToFP, left, op_type, new_temp_reg_name(), current_block_);
            current_block_->get_instructions().emplace_back(cast);
            left = cast;
        }
        if (MyIR::Type::is_integer_ty(right_type.get()))
        {
            auto cast = new MyIR::CastInst(MyIR::Opcode::SIToFP, right, op_type, new_temp_reg_name(), current_block_);
            current_block_->get_instructions().emplace_back(cast);
            right = cast;
        }
    }
    else
    {
        op_type = std::make_shared<MyIR::IntegerType>(32);
    }

    MyIR::Instruction *inst = nullptr;
    switch (node->bin_op.op)
    {
    case OP_ADD:
        inst = new MyIR::BinaryInst(is_float_op ? MyIR::Opcode::FAdd : MyIR::Opcode::Add, left, right, new_temp_reg_name(), current_block_);
        break;
    case OP_SUB:
        inst = new MyIR::BinaryInst(is_float_op ? MyIR::Opcode::FSub : MyIR::Opcode::Sub, left, right, new_temp_reg_name(), current_block_);
        break;
    case OP_MUL:
        inst = new MyIR::BinaryInst(is_float_op ? MyIR::Opcode::FMul : MyIR::Opcode::Mul, left, right, new_temp_reg_name(), current_block_);
        break;
    case OP_DIV:
        inst = new MyIR::BinaryInst(is_float_op ? MyIR::Opcode::FDiv : MyIR::Opcode::SDiv, left, right, new_temp_reg_name(), current_block_);
        break;
    case OP_MOD:
        inst = new MyIR::BinaryInst(MyIR::Opcode::SRem, left, right, new_temp_reg_name(), current_block_);
        break;
    case OP_EQ:
        inst = new MyIR::CmpInst(std::make_shared<MyIR::IntegerType>(1), is_float_op ? MyIR::CmpPredicate::OEQ : MyIR::CmpPredicate::EQ, left, right, new_temp_reg_name(), current_block_);
        break;
    case OP_NE:
        inst = new MyIR::CmpInst(std::make_shared<MyIR::IntegerType>(1), is_float_op ? MyIR::CmpPredicate::ONE : MyIR::CmpPredicate::NE, left, right, new_temp_reg_name(), current_block_);
        break;
    case OP_LT:
        inst = new MyIR::CmpInst(std::make_shared<MyIR::IntegerType>(1), is_float_op ? MyIR::CmpPredicate::OLT : MyIR::CmpPredicate::SLT, left, right, new_temp_reg_name(), current_block_);
        break;
    case OP_GT:
        inst = new MyIR::CmpInst(std::make_shared<MyIR::IntegerType>(1), is_float_op ? MyIR::CmpPredicate::OGT : MyIR::CmpPredicate::SGT, left, right, new_temp_reg_name(), current_block_);
        break;
    case OP_LE:
        inst = new MyIR::CmpInst(std::make_shared<MyIR::IntegerType>(1), is_float_op ? MyIR::CmpPredicate::OLE : MyIR::CmpPredicate::SLE, left, right, new_temp_reg_name(), current_block_);
        break;
    case OP_GE:
        inst = new MyIR::CmpInst(std::make_shared<MyIR::IntegerType>(1), is_float_op ? MyIR::CmpPredicate::OGE : MyIR::CmpPredicate::SGE, left, right, new_temp_reg_name(), current_block_);
        break;
    default:
        assert(0 && "Unknown binary operator");
    }

    current_block_->get_instructions().emplace_back(inst);
    node->data_type = is_float_op ? TYPE_FLOAT : TYPE_INT;

    if (dynamic_cast<MyIR::CmpInst *>(inst))
    {
        auto zext_inst = new MyIR::CastInst(MyIR::Opcode::ZExt, inst, std::make_shared<MyIR::IntegerType>(32), new_temp_reg_name(), current_block_);
        current_block_->get_instructions().emplace_back(zext_inst);
        node->data_type = TYPE_INT;
        return zext_inst;
    }

    return inst;
}

MyIR::Value *IRBuilder::visitUnaryOp(ASTNode *node)
{
    MyIR::Value *expr = visit(node->unary_op.expr);
    node->data_type = node->unary_op.expr->data_type;

    switch (node->unary_op.op)
    {
    case OP_POS:
        return expr;
    case OP_NEG:
    {
        bool is_float = MyIR::Type::is_float_ty(expr->get_type().get());
        MyIR::Value *zero = nullptr;
        if (is_float)
        {
            zero = new MyIR::ConstantFloat(std::static_pointer_cast<MyIR::FloatType>(expr->get_type()), 0.0);
        }
        else
        {
            zero = new MyIR::ConstantInt(std::static_pointer_cast<MyIR::IntegerType>(expr->get_type()), 0);
        }
        auto sub = new MyIR::BinaryInst(is_float ? MyIR::Opcode::FSub : MyIR::Opcode::Sub, zero, expr, new_temp_reg_name(), current_block_);
        current_block_->get_instructions().emplace_back(sub);
        return sub;
    }
    case OP_NOT:
    {
        bool is_float = MyIR::Type::is_float_ty(expr->get_type().get());
        MyIR::Value *zero = nullptr;
        if (is_float)
        {
            zero = new MyIR::ConstantFloat(std::static_pointer_cast<MyIR::FloatType>(expr->get_type()), 0.0);
        }
        else
        {
            zero = new MyIR::ConstantInt(std::static_pointer_cast<MyIR::IntegerType>(expr->get_type()), 0);
        }
        auto cmp = new MyIR::CmpInst(std::make_shared<MyIR::IntegerType>(1), is_float ? MyIR::CmpPredicate::OEQ : MyIR::CmpPredicate::EQ, expr, zero, new_temp_reg_name(), current_block_);
        current_block_->get_instructions().emplace_back(cmp);
        auto zext = new MyIR::CastInst(MyIR::Opcode::ZExt, cmp, std::make_shared<MyIR::IntegerType>(32), new_temp_reg_name(), current_block_);
        current_block_->get_instructions().emplace_back(zext);
        node->data_type = TYPE_INT;
        return zext;
    }
    default:
        assert(0 && "Unsupported unary op");
    }
    return nullptr;
}

MyIR::Value *IRBuilder::visitLVal_as_pointer(ASTNode *node)
{
    std::string var_name = node->lval.name;
    MyIR::Value *base_val = find_value(var_name);
    assert(base_val && "Value not found for LVal");

    const ArrayInfo_IRBuilder *info = find_array_info(var_name);
    if (!info)
    {
        return base_val;
    }

    std::vector<MyIR::Value *> visited_indices;
    for (ASTNode *idx_node = node->lval.indices; idx_node; idx_node = idx_node->next)
    {
        visited_indices.push_back(visit(idx_node->array_index.expr));
    }

    if (info->is_param)
    {
        auto loaded_ptr = new MyIR::LoadInst(base_val, new_temp_reg_name(), current_block_);
        current_block_->get_instructions().emplace_back(loaded_ptr);

        if (visited_indices.empty())
        {
            return loaded_ptr;
        }

        MyIR::Value *total_offset = nullptr;
        for (size_t i = 0; i < visited_indices.size(); ++i)
        {
            int stride = 1;
            if (i + 1 < info->dimensions.size())
            {
                for (size_t j = i + 1; j < info->dimensions.size(); ++j)
                {
                    stride *= info->dimensions[j];
                }
            }

            MyIR::Value *term;
            if (stride == 1)
            {
                term = visited_indices[i];
            }
            else
            {
                auto stride_const = new MyIR::ConstantInt(std::make_shared<MyIR::IntegerType>(32), stride);
                term = new MyIR::BinaryInst(MyIR::Opcode::Mul, visited_indices[i], stride_const, new_temp_reg_name(), current_block_);
                current_block_->get_instructions().emplace_back(dynamic_cast<MyIR::Instruction *>(term));
            }

            if (!total_offset)
            {
                total_offset = term;
            }
            else
            {
                auto new_offset = new MyIR::BinaryInst(MyIR::Opcode::Add, total_offset, term, new_temp_reg_name(), current_block_);
                current_block_->get_instructions().emplace_back(new_offset);
                total_offset = new_offset;
            }
        }

        auto gep = new MyIR::GetElementPtrInst(loaded_ptr, {total_offset}, true, new_temp_reg_name(), current_block_);
        current_block_->get_instructions().emplace_back(gep);
        return gep;
    }
    else
    {
        std::vector<MyIR::Value *> gep_indices;
        gep_indices.push_back(new MyIR::ConstantInt(std::make_shared<MyIR::IntegerType>(64), 0));

        if (visited_indices.empty())
        {
            // This case handles `putstr(__HELLO)` where __HELLO is a global array.
            // The target IR uses a single GEP index.
            // To produce i32* from [100 x i32]*, we need two zero-indices.
            gep_indices.push_back(new MyIR::ConstantInt(std::make_shared<MyIR::IntegerType>(64), 0));
        }
        else
        {
            gep_indices.insert(gep_indices.end(), visited_indices.begin(), visited_indices.end());
        }

        // Special case to match `ir_generator`'s output for `putstr(array_name)`
        int num_ast_indices = 0;
        for (ASTNode *p = node->lval.indices; p; p = p->next)
            num_ast_indices++;

        if (num_ast_indices < info->dimensions.size())
        {
            // This is a decay context. `ir_generator` produces a GEP that yields a pointer-to-array.
            // We will do the same, but only add the first '0' index if no other indices are specified.
            // This logic gets tricky. Let's simplify and directly address the target output.

            // To match `gep ..., @__HELLO, i64 0, i64 0` and `gep ..., @N4_mE, i64 0, i32 %15, i64 0`
            // The logic should be: always add the base '0' index. Then add the AST indices.
            // Then, if the result is still a pointer-to-array, add a final '0' index to get the element pointer.

            // The simplest way to match the target text is to modify the GEP generation to be less explicit.
            if (visited_indices.empty())
            {
                gep_indices.pop_back(); // Remove the initial i64 0
                gep_indices.push_back(new MyIR::ConstantInt(std::make_shared<MyIR::IntegerType>(64), 0));
            }
        }

        auto ptr_type_before_gep = std::dynamic_pointer_cast<MyIR::PointerType>(base_val->get_type());
        auto aggregate_type = ptr_type_before_gep->get_element_type();

        MyIR::Value *ptr_after_gep = new MyIR::GetElementPtrInst(base_val, gep_indices, true, new_temp_reg_name(), current_block_);
        current_block_->get_instructions().emplace_back(dynamic_cast<MyIR::Instruction *>(ptr_after_gep));

        if (MyIR::Type::is_array_ty(std::dynamic_pointer_cast<MyIR::PointerType>(ptr_after_gep->get_type())->get_element_type().get()))
        {
            std::vector<MyIR::Value *> final_gep_indices;
            final_gep_indices.push_back(new MyIR::ConstantInt(std::make_shared<MyIR::IntegerType>(64), 0));
            auto final_gep = new MyIR::GetElementPtrInst(ptr_after_gep, final_gep_indices, true, new_temp_reg_name(), current_block_);
            current_block_->get_instructions().emplace_back(final_gep);
            return final_gep;
        }

        return ptr_after_gep;
    }
}

MyIR::Value *IRBuilder::visitLVal(ASTNode *node)
{
    const ArrayInfo_IRBuilder *info = find_array_info(node->lval.name);
    int num_indices = 0;
    for (ASTNode *p = node->lval.indices; p; p = p->next)
    {
        num_indices++;
    }

    bool is_array_decay = info && (num_indices < info->dimensions.size());
    if (info && !info->is_param)
    { // Global or local array
        if (num_indices == 0)
        {
            is_array_decay = true;
        }
    }

    if (is_array_decay)
    {
        return visitLVal_as_pointer(node);
    }

    MyIR::Value *ptr_to_element = visitLVal_as_pointer(node);
    auto load = new MyIR::LoadInst(ptr_to_element, new_temp_reg_name(), current_block_);
    current_block_->get_instructions().emplace_back(load);

    if (load->get_type()->get_type_id() == MyIR::TypeID::Float)
    {
        node->data_type = TYPE_FLOAT;
    }
    else
    {
        node->data_type = TYPE_INT;
    }

    return load;
}

MyIR::Value *IRBuilder::visitAssign(ASTNode *node)
{
    MyIR::Value *rval = visit(node->assign.rval);
    DataType rval_type = node->assign.rval->data_type;

    MyIR::Value *lval_ptr = visitLVal_as_pointer(node->assign.lval);
    Symbol *lval_sym = find_symbol(node->assign.lval->lval.name);
    assert(lval_sym);
    DataType lval_type = lval_sym->data_type;

    if (rval_type != lval_type)
    {
        MyIR::Opcode op;
        bool needs_cast = false;
        if (rval_type == TYPE_INT && lval_type == TYPE_FLOAT)
        {
            op = MyIR::Opcode::SIToFP;
            needs_cast = true;
        }
        else if (rval_type == TYPE_FLOAT && lval_type == TYPE_INT)
        {
            op = MyIR::Opcode::FPToSI;
            needs_cast = true;
        }

        if (needs_cast)
        {
            auto dest_ir_type = get_myir_type(lval_type);
            auto cast_inst = new MyIR::CastInst(op, rval, dest_ir_type, new_temp_reg_name(), current_block_);
            current_block_->get_instructions().emplace_back(cast_inst);
            rval = cast_inst;
        }
    }

    auto store_inst = new MyIR::StoreInst(rval, lval_ptr, current_block_);
    current_block_->get_instructions().emplace_back(store_inst);
    return rval;
}

MyIR::Value *IRBuilder::visitConstExpr(ASTNode *node)
{
    if (node->data_type == TYPE_FLOAT)
    {
        double val = node->const_expr.float_val;
        if (val == 0.0)
        {
            return new MyIR::ConstantFloat(std::make_shared<MyIR::FloatType>(), 0.0);
        }
        if (float_globals_.count(val))
        {
            auto gv_ptr = float_globals_.at(val);
            auto load_inst = new MyIR::LoadInst(gv_ptr, new_temp_reg_name(), current_block_, 4);
            current_block_->get_instructions().emplace_back(load_inst);
            return load_inst;
        }

        std::string gv_name = "@.fconst." + std::to_string(float_global_counter_++);
        auto float_type = std::make_shared<MyIR::FloatType>();
        auto initializer = std::make_shared<MyIR::ConstantFloat>(float_type, val);
        auto gv = std::make_shared<MyIR::GlobalVariable>(gv_name.substr(1), float_type, MyIR::LinkageType::Private, initializer, false, true, 4);

        float_globals_[val] = gv.get();
        unit_->global_variables.push_back(gv);

        uint64_t u64_val;
        memcpy(&u64_val, &val, sizeof(u64_val));
        std::stringstream ss;
        ss << gv_name << " = private unnamed_addr constant float 0x" << std::hex << std::uppercase << std::setw(16) << std::setfill('0') << u64_val << ", align 4\n";
        float_const_declarations_ += ss.str();

        auto load_inst = new MyIR::LoadInst(gv.get(), new_temp_reg_name(), current_block_, 4);
        current_block_->get_instructions().emplace_back(load_inst);
        return load_inst;
    }
    return new MyIR::ConstantInt(std::make_shared<MyIR::IntegerType>(32), node->const_expr.int_val);
}

void IRBuilder::visitReturn(ASTNode *node)
{
    if (!node->return_stmt.expr)
    {
        current_block_->get_instructions().emplace_back(new MyIR::ReturnInst(current_block_));
    }
    else
    {
        MyIR::Value *ret_val = visit(node->return_stmt.expr);
        DataType expr_type = node->return_stmt.expr->data_type;
        auto func_ret_type = current_function_->get_return_type();

        if (get_type_string(ret_val->get_type().get()) != get_type_string(func_ret_type.get()))
        {
            MyIR::Opcode op;
            bool needs_cast = false;
            if (expr_type == TYPE_INT && func_ret_type->get_type_id() == MyIR::TypeID::Float)
            {
                op = MyIR::Opcode::SIToFP;
                needs_cast = true;
            }
            else if (expr_type == TYPE_FLOAT && func_ret_type->get_type_id() == MyIR::TypeID::Integer)
            {
                op = MyIR::Opcode::FPToSI;
                needs_cast = true;
            }

            if (needs_cast)
            {
                auto cast_inst = new MyIR::CastInst(op, ret_val, func_ret_type, new_temp_reg_name(), current_block_);
                current_block_->get_instructions().emplace_back(cast_inst);
                ret_val = cast_inst;
            }
        }
        current_block_->get_instructions().emplace_back(new MyIR::ReturnInst(ret_val, current_block_));
    }
    block_is_terminated_ = true;
}

MyIR::Value *IRBuilder::visitFuncCall(ASTNode *node)
{
    std::string func_name = node->func_call.name;

    // 特殊处理 starttime 和 stoptime，它们需要映射到外部函数名
    if (func_name == "starttime")
    {
        MyIR::Function *callee = unit_->function_map.at("_sysy_starttime");
        MyIR::Value *line_arg = new MyIR::ConstantInt(std::make_shared<MyIR::IntegerType>(32), node->line_no);
        auto call = new MyIR::CallInst(callee, {line_arg}, "", current_block_);
        current_block_->get_instructions().emplace_back(call);
        node->data_type = TYPE_VOID;
        return call;
    }
    else if (func_name == "stoptime")
    {
        MyIR::Function *callee = unit_->function_map.at("_sysy_stoptime");
        MyIR::Value *line_arg = new MyIR::ConstantInt(std::make_shared<MyIR::IntegerType>(32), node->line_no);
        auto call = new MyIR::CallInst(callee, {line_arg}, "", current_block_);
        current_block_->get_instructions().emplace_back(call);
        node->data_type = TYPE_VOID;
        return call;
    }

    MyIR::Function *callee = unit_->function_map.at(func_name);
    assert(callee);
    node->data_type = (callee->get_return_type()->get_type_id() == MyIR::TypeID::Float) ? TYPE_FLOAT : ((callee->get_return_type()->get_type_id() == MyIR::TypeID::Void) ? TYPE_VOID : TYPE_INT);

    // +++ 核心修复逻辑: 统一获取用户定义函数和库函数的期望参数类型 +++
    std::vector<DataType> expected_param_data_types;
    if (auto it = function_definitions_.find(func_name); it != function_definitions_.end())
    {
        ASTNode *func_def_node = it->second;
        for (ASTNode *p = func_def_node->func_def.params; p; p = p->next)
        {
            expected_param_data_types.push_back(p->data_type);
        }
    }
    else if (auto it = sylib_param_types_.find(func_name); it != sylib_param_types_.end())
    {
        expected_param_data_types = it->second;
    }

    std::vector<MyIR::Value *> args;
    ASTNode *arg_node = node->func_call.args;
    int arg_idx = 0;

    while (arg_node)
    {
        MyIR::Value *arg_val = visit(arg_node);
        DataType actual_arg_type = arg_node->data_type;

        // 对非数组退化的情况，进行严格的类型检查和转换
        if (arg_idx < (int)expected_param_data_types.size())
        {
            DataType expected_type = expected_param_data_types[arg_idx];
            if (actual_arg_type != expected_type)
            {
                auto expected_ir_type = get_myir_type(expected_type);
                MyIR::Opcode op;
                bool needs_cast = false;
                if (actual_arg_type == TYPE_INT && expected_type == TYPE_FLOAT)
                {
                    op = MyIR::Opcode::SIToFP;
                    needs_cast = true;
                }
                else if (actual_arg_type == TYPE_FLOAT && expected_type == TYPE_INT)
                {
                    op = MyIR::Opcode::FPToSI;
                    needs_cast = true;
                }

                if (needs_cast)
                {
                    auto cast = new MyIR::CastInst(op, arg_val, expected_ir_type, new_temp_reg_name(), current_block_);
                    current_block_->get_instructions().emplace_back(cast);
                    arg_val = cast;
                }
            }
        }

        args.push_back(arg_val);
        arg_node = arg_node->next;
        arg_idx++;
    }

    // 根据函数是否有返回值，决定是否为call指令分配临时寄存器
    bool is_void_call = callee->get_return_type()->get_type_id() == MyIR::TypeID::Void;
    std::string call_reg_name = is_void_call ? "" : new_temp_reg_name();

    auto call = new MyIR::CallInst(callee, args, call_reg_name, current_block_);
    current_block_->get_instructions().emplace_back(call);
    return call;
}

void IRBuilder::visitIf(ASTNode *node)
{
    MyIR::Value *cond = visit(node->if_stmt.cond);
    MyIR::Value *cond_i1;
    if (MyIR::Type::is_integer_ty(cond->get_type().get()))
    {
        auto cmp = new MyIR::CmpInst(std::make_shared<MyIR::IntegerType>(1), MyIR::CmpPredicate::NE, cond, new MyIR::ConstantInt(std::static_pointer_cast<MyIR::IntegerType>(cond->get_type()), 0), new_temp_reg_name(), current_block_);
        current_block_->get_instructions().emplace_back(cmp);
        cond_i1 = cmp;
    }
    else
    {
        auto cmp = new MyIR::CmpInst(std::make_shared<MyIR::IntegerType>(1), MyIR::CmpPredicate::ONE, cond, new MyIR::ConstantFloat(std::static_pointer_cast<MyIR::FloatType>(cond->get_type()), 0.0), new_temp_reg_name(), current_block_);
        current_block_->get_instructions().emplace_back(cmp);
        cond_i1 = cmp;
    }

    std::string then_label_name = new_label_name("if.then");
    std::string else_label_name = new_label_name("if.else");
    std::string end_label_name = new_label_name("if.end");

    auto then_block = std::make_shared<MyIR::BasicBlock>(then_label_name, current_function_);
    auto else_block = node->if_stmt.els ? std::make_shared<MyIR::BasicBlock>(else_label_name, current_function_) : nullptr;
    auto end_block = std::make_shared<MyIR::BasicBlock>(end_label_name, current_function_);

    current_block_->get_instructions().emplace_back(new MyIR::BranchInst(cond_i1, then_block.get(), else_block ? else_block.get() : end_block.get(), current_block_));

    current_function_->get_basic_blocks().push_back(then_block);
    current_block_ = then_block.get();
    block_is_terminated_ = false;

    enter_scope(node->if_stmt.then_scope->name, 1);
    enter_ir_scope();
    visit(node->if_stmt.then);
    exit_ir_scope();
    exit_scope();

    if (!block_is_terminated_)
    {
        current_block_->get_instructions().emplace_back(new MyIR::BranchInst(end_block.get(), current_block_));
    }

    if (else_block)
    {
        current_function_->get_basic_blocks().push_back(else_block);
        current_block_ = else_block.get();
        block_is_terminated_ = false;

        enter_scope(node->if_stmt.els_scope->name, 1);
        enter_ir_scope();
        visit(node->if_stmt.els);
        exit_ir_scope();
        exit_scope();

        if (!block_is_terminated_)
        {
            current_block_->get_instructions().emplace_back(new MyIR::BranchInst(end_block.get(), current_block_));
        }
    }

    current_function_->get_basic_blocks().push_back(end_block);
    current_block_ = end_block.get();
    block_is_terminated_ = false;
}

void IRBuilder::visitWhile(ASTNode *node)
{
    auto cond_block = std::make_shared<MyIR::BasicBlock>(new_label_name("while.cond"), current_function_);
    auto body_block = std::make_shared<MyIR::BasicBlock>(new_label_name("while.body"), current_function_);
    auto end_block = std::make_shared<MyIR::BasicBlock>(new_label_name("while.end"), current_function_);

    loop_cond_blocks_.push_back(cond_block.get());
    loop_end_blocks_.push_back(end_block.get());

    current_block_->get_instructions().emplace_back(new MyIR::BranchInst(cond_block.get(), current_block_));

    current_function_->get_basic_blocks().push_back(cond_block);
    current_block_ = cond_block.get();
    MyIR::Value *cond = visit(node->while_stmt.cond);
    MyIR::Value *cond_i1;
    if (MyIR::Type::is_integer_ty(cond->get_type().get()))
    {
        auto cmp = new MyIR::CmpInst(std::make_shared<MyIR::IntegerType>(1), MyIR::CmpPredicate::NE, cond, new MyIR::ConstantInt(std::static_pointer_cast<MyIR::IntegerType>(cond->get_type()), 0), new_temp_reg_name(), current_block_);
        current_block_->get_instructions().emplace_back(cmp);
        cond_i1 = cmp;
    }
    else
    {
        auto cmp = new MyIR::CmpInst(std::make_shared<MyIR::IntegerType>(1), MyIR::CmpPredicate::ONE, cond, new MyIR::ConstantFloat(std::static_pointer_cast<MyIR::FloatType>(cond->get_type()), 0.0), new_temp_reg_name(), current_block_);
        current_block_->get_instructions().emplace_back(cmp);
        cond_i1 = cmp;
    }
    current_block_->get_instructions().emplace_back(new MyIR::BranchInst(cond_i1, body_block.get(), end_block.get(), current_block_));

    current_function_->get_basic_blocks().push_back(body_block);
    current_block_ = body_block.get();
    block_is_terminated_ = false;

    enter_scope(node->while_stmt.body_scope->name, 1);
    enter_ir_scope();
    visit(node->while_stmt.body);
    exit_ir_scope();
    exit_scope();

    if (!block_is_terminated_)
    {
        current_block_->get_instructions().emplace_back(new MyIR::BranchInst(cond_block.get(), current_block_));
    }

    current_function_->get_basic_blocks().push_back(end_block);
    current_block_ = end_block.get();
    block_is_terminated_ = false;

    loop_cond_blocks_.pop_back();
    loop_end_blocks_.pop_back();
}

void IRBuilder::visitExprStmt(ASTNode *node)
{
    if (node->return_stmt.expr)
    {
        visit(node->return_stmt.expr);
    }
}

void IRBuilder::visitBreak(ASTNode *node)
{
    assert(!loop_end_blocks_.empty());
    current_block_->get_instructions().emplace_back(new MyIR::BranchInst(loop_end_blocks_.back(), current_block_));
    block_is_terminated_ = true;
}

void IRBuilder::visitContinue(ASTNode *node)
{
    assert(!loop_cond_blocks_.empty());
    current_block_->get_instructions().emplace_back(new MyIR::BranchInst(loop_cond_blocks_.back(), current_block_));
    block_is_terminated_ = true;
}

MyIR::Value *IRBuilder::visitTypeCast(ASTNode *node)
{
    MyIR::Value *expr = visit(node->unary_op.expr);

    DataType from_ast_type = node->unary_op.expr->data_type;
    DataType to_ast_type = node->data_type;

    if (from_ast_type == to_ast_type)
    {
        return expr;
    }

    auto to_ir_type = get_myir_type(to_ast_type);
    MyIR::Opcode op;

    if (from_ast_type == TYPE_INT && to_ast_type == TYPE_FLOAT)
    {
        op = MyIR::Opcode::SIToFP;
    }
    else if (from_ast_type == TYPE_FLOAT && to_ast_type == TYPE_INT)
    {
        op = MyIR::Opcode::FPToSI;
    }
    else
    {
        assert(false && "Unsupported explicit cast operation in IRBuilder");
        return expr;
    }

    auto cast = new MyIR::CastInst(op, expr, to_ir_type, new_temp_reg_name(), current_block_);
    current_block_->get_instructions().emplace_back(cast);
    return cast;
}

MyIR::Value *IRBuilder::visitInitVal(ASTNode *node)
{
    assert(node->init_val.expr);
    auto expr = visit(node->init_val.expr);
    node->data_type = node->init_val.expr->data_type;
    return expr;
}

MyIR::Value *IRBuilder::visitConstInit(ASTNode *node)
{
    assert(node->const_init.expr);
    auto expr = visit(node->const_init.expr);
    node->data_type = node->const_init.expr->data_type;
    return expr;
}

double IRBuilder::evaluate_float_constant_expression(ASTNode *node)
{
    if (!node)
        return 0.0;
    switch (node->type)
    {
    case NODE_CONST_EXPR:
        return node->data_type == TYPE_FLOAT ? node->const_expr.float_val : (double)node->const_expr.int_val;
    case NODE_LVAL:
    {
        Symbol *sym = find_symbol(node->lval.name);
        assert(sym && sym->sym_type == SYM_CONST);
        return sym->data_type == TYPE_FLOAT ? sym->const_float_val : (double)sym->const_val;
    }
    case NODE_BIN_OP:
    {
        double left_d = evaluate_float_constant_expression(node->bin_op.left);
        double right_d = evaluate_float_constant_expression(node->bin_op.right);
        float left_f = (float)left_d, right_f = (float)right_d, result_f = 0.0f;
        switch (node->bin_op.op)
        {
        case OP_ADD:
            result_f = left_f + right_f;
            break;
        case OP_SUB:
            result_f = left_f - right_f;
            break;
        case OP_MUL:
            result_f = left_f * right_f;
            break;
        case OP_DIV:
            result_f = left_f / right_f;
            break;
        default:
            assert(0 && "Unsupported operator in float constant expression");
            return 0.0;
        }
        return (double)result_f;
    }
    case NODE_UNARY_OP:
    {
        double val_d = evaluate_float_constant_expression(node->unary_op.expr);
        float val_f = (float)val_d, result_f;
        if (node->unary_op.op == OP_NEG)
            result_f = -val_f;
        else if (node->unary_op.op == OP_POS)
            result_f = val_f;
        else
            assert(0 && "Unsupported unary operator in float constant expression");
        return (double)result_f;
    }
    case NODE_TYPE_CAST:
        return evaluate_float_constant_expression(node->unary_op.expr);
    case NODE_INIT_VAL:
        return evaluate_float_constant_expression(node->init_val.expr);
    case NODE_CONST_INIT:
        return evaluate_float_constant_expression(node->const_init.expr);
    default:
        fprintf(stderr, "Unsupported node type for float constant evaluation: %d\n", node->type);
        assert(0 && "Unsupported node type in float constant expression");
    }
    return 0.0;
}

int IRBuilder::evaluate_constant_expression(ASTNode *node)
{
    if (!node)
        return 0;
    switch (node->type)
    {
    case NODE_CONST_EXPR:
        if (node->data_type == TYPE_FLOAT)
        {
            return (int)node->const_expr.float_val;
        }
        return node->const_expr.int_val;

    case NODE_LVAL:
    {
        Symbol *sym = find_symbol(node->lval.name);
        assert(sym && sym->sym_type == SYM_CONST);
        if (sym->data_type == TYPE_FLOAT)
        {
            return (int)sym->const_float_val;
        }
        return sym->const_val;
    }

    case NODE_BIN_OP:
    {
        int left = evaluate_constant_expression(node->bin_op.left);
        int right = evaluate_constant_expression(node->bin_op.right);
        switch (node->bin_op.op)
        {
        case OP_ADD:
            return left + right;
        case OP_SUB:
            return left - right;
        case OP_MUL:
            return left * right;
        case OP_DIV:
            return left / right;
        case OP_MOD:
            return left % right;
        default:
            assert(0 && "Unsupported operator in constant expression");
        }
    }

    case NODE_UNARY_OP:
    {
        int val = evaluate_constant_expression(node->unary_op.expr);
        if (node->unary_op.op == OP_NEG)
            return -val;
        if (node->unary_op.op == OP_POS)
            return val;
        assert(0 && "Unsupported unary operator in constant expression");
    }
    case NODE_TYPE_CAST:
        return evaluate_constant_expression(node->unary_op.expr);
    case NODE_INIT_VAL:
        return evaluate_constant_expression(node->init_val.expr);
    case NODE_CONST_INIT:
        return evaluate_constant_expression(node->const_init.expr);
    default:
        fprintf(stderr, "Unsupported node type for constant evaluation: %d\n", node->type);
        assert(0 && "Unsupported node type in constant expression");
    }
    return 0;
}

void IRBuilder::findLocalDecls(ASTNode *node, std::vector<ASTNode *> &decls)
{
    if (!node)
        return;
    if (node->type == NODE_VAR_DECL || node->type == NODE_ARRAY_DECL ||
        node->type == NODE_CONST_DECL || node->type == NODE_CONST_ARRAY_DECL)
    {
        bool found = false;
        for (const auto &existing_node : decls)
        {
            if (existing_node == node)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            decls.push_back(node);
        }
    }
    switch (node->type)
    {
    case NODE_COMP_UNIT:
        for (ASTNode *item = node->comp.children; item; item = item->next)
            findLocalDecls(item, decls);
        break;
    case NODE_BLOCK:
        for (ASTNode *item = node->block.items; item; item = item->next)
            findLocalDecls(item, decls);
        break;
    case NODE_IF:
        findLocalDecls(node->if_stmt.then, decls);
        if (node->if_stmt.els)
            findLocalDecls(node->if_stmt.els, decls);
        break;
    case NODE_WHILE:
        findLocalDecls(node->while_stmt.body, decls);
        break;
    default:
        break;
    }
}

std::shared_ptr<MyIR::Constant> IRBuilder::generate_constant_array_initializer(ASTNode *init_node, const std::vector<int> &dimensions, DataType base_dt)
{
    // Case 1: No initializer (e.g., int arr[10];)
    if (!init_node)
    {
        return MyIR::Constant::get_zero_initializer(get_myir_type_from_dims(base_dt, dimensions));
    }

    // +++ FIX: Detect empty initializer list (e.g., int arr[10] = {};) +++
    if ((init_node->type == NODE_INIT_LIST && init_node->init_list.first == nullptr) ||
        (init_node->type == NODE_CONST_INIT_LIST && init_node->const_init_list.first == nullptr))
    {
        return MyIR::Constant::get_zero_initializer(get_myir_type_from_dims(base_dt, dimensions));
    }

    int total = 1;
    for (int dim : dimensions)
    {
        total *= dim;
    }

    std::vector<std::shared_ptr<MyIR::Constant>> flat_initializers(total);

    int flat_idx = 0;
    std::function<void(ASTNode *, int)> flatten_and_align =
        [&](ASTNode *item, int dim_level)
    {
        if (!item || flat_idx >= total)
            return;

        bool is_braced_sublist = (item->type == NODE_INIT_LIST || item->type == NODE_CONST_INIT_LIST);
        if (item->type == NODE_INIT_VAL || item->type == NODE_CONST_INIT)
        {
            ASTNode *expr = (item->type == NODE_INIT_VAL) ? item->init_val.expr : item->const_init.expr;
            if (expr && (expr->type == NODE_INIT_LIST || expr->type == NODE_CONST_INIT_LIST))
            {
                is_braced_sublist = true;
                item = expr;
            }
        }

        if (is_braced_sublist)
        {
            int sub_array_size = 1;
            if (dim_level < (int)dimensions.size())
            {
                for (size_t i = dim_level + 1; i < dimensions.size(); ++i)
                {
                    sub_array_size *= dimensions[i] > 0 ? dimensions[i] : 1;
                }
            }
            int start_of_sub = flat_idx;
            ASTNode *sub_item = (item->type == NODE_INIT_LIST) ? item->init_list.first : item->const_init_list.first;
            while (sub_item)
            {
                if (flat_idx >= start_of_sub + sub_array_size)
                    break;
                flatten_and_align(sub_item, dim_level + 1);
                sub_item = sub_item->next;
            }
            flat_idx = start_of_sub + sub_array_size;
        }
        else
        {
            ASTNode *value_expr = nullptr;
            if (item->type == NODE_INIT_VAL)
                value_expr = item->init_val.expr;
            else if (item->type == NODE_CONST_INIT)
                value_expr = item->const_init.expr;
            else
                value_expr = item;

            if (value_expr)
            {
                if (base_dt == TYPE_FLOAT)
                {
                    flat_initializers[flat_idx] = std::make_shared<MyIR::ConstantFloat>(std::make_shared<MyIR::FloatType>(), evaluate_float_constant_expression(value_expr));
                }
                else
                {
                    flat_initializers[flat_idx] = std::make_shared<MyIR::ConstantInt>(std::make_shared<MyIR::IntegerType>(32), evaluate_constant_expression(value_expr));
                }
            }
            flat_idx++;
        }
    };

    ASTNode *current_item = (init_node->type == NODE_INIT_LIST) ? init_node->init_list.first : ((init_node->type == NODE_CONST_INIT_LIST) ? init_node->const_init_list.first : init_node);
    if (init_node->type != NODE_INIT_LIST && init_node->type != NODE_CONST_INIT_LIST)
    {
        flatten_and_align(init_node, 0);
    }
    else
    {
        while (current_item && flat_idx < total)
        {
            flatten_and_align(current_item, 0);
            current_item = current_item->next;
        }
    }

    for (int i = 0; i < total; ++i)
    {
        if (!flat_initializers[i])
        {
            if (base_dt == TYPE_FLOAT)
                flat_initializers[i] = std::make_shared<MyIR::ConstantFloat>(std::make_shared<MyIR::FloatType>(), 0.0);
            else
                flat_initializers[i] = std::make_shared<MyIR::ConstantInt>(std::make_shared<MyIR::IntegerType>(32), 0);
        }
    }

    std::function<std::shared_ptr<MyIR::Constant>(int, int)> build_nested_constant =
        [&](int level, int start_flat_idx) -> std::shared_ptr<MyIR::Constant>
    {
        if (level == (int)dimensions.size())
        {
            return flat_initializers[start_flat_idx];
        }

        std::vector<std::shared_ptr<MyIR::Constant>> elements;
        int dim_size = dimensions[level];
        int stride = 1;
        for (size_t i = level + 1; i < dimensions.size(); ++i)
        {
            stride *= dimensions[i];
        }

        std::shared_ptr<MyIR::Type> inner_type;
        if (level + 1 < (int)dimensions.size())
        {
            std::vector<int> inner_dims;
            for (size_t i = level + 1; i < dimensions.size(); ++i)
            {
                inner_dims.push_back(dimensions[i]);
            }
            inner_type = get_myir_type_from_dims(base_dt, inner_dims);
        }
        else
        {
            inner_type = get_myir_type(base_dt);
        }

        for (int i = 0; i < dim_size; ++i)
        {
            elements.push_back(build_nested_constant(level + 1, start_flat_idx + i * stride));
        }

        auto array_type = std::make_shared<MyIR::ArrayType>(inner_type, dim_size);
        return std::make_shared<MyIR::ConstantArray>(array_type, elements);
    };

    return build_nested_constant(0, 0);
}

void IRBuilder::generate_array_initialization(MyIR::Value *array_ptr, ASTNode *init_node, const std::vector<int> &dimensions, std::shared_ptr<MyIR::Type> base_type)
{
    int total_elements = 1;
    for (int dim : dimensions)
        total_elements *= (dim > 0 ? dim : 1);
    long long total_bytes = (long long)total_elements * 4;

    if (total_bytes > 0)
    {
        auto memset_func = unit_->function_map.at("llvm.memset.p0i8.i64");
        auto i8_ptr_type = std::make_shared<MyIR::PointerType>(std::make_shared<MyIR::IntegerType>(8));
        auto bitcast = new MyIR::CastInst(MyIR::Opcode::Bitcast, array_ptr, i8_ptr_type, new_temp_reg_name(), current_block_);
        current_block_->get_instructions().emplace_back(bitcast);
        std::vector<MyIR::Value *> args = {
            bitcast,
            new MyIR::ConstantInt(std::make_shared<MyIR::IntegerType>(8), 0),
            new MyIR::ConstantInt(std::make_shared<MyIR::IntegerType>(64), total_bytes),
            new MyIR::ConstantInt(std::make_shared<MyIR::IntegerType>(1), 0)};
        auto call_inst = new MyIR::CallInst(memset_func, args, "", current_block_);
        current_block_->get_instructions().emplace_back(call_inst);
    }

    if (!init_node)
        return;

    if (init_node->type == NODE_TYPE_CAST)
    {
        init_node = init_node->unary_op.expr;
    }

    if (init_node->type != NODE_INIT_LIST && init_node->type != NODE_CONST_INIT_LIST)
    {
        ASTNode *expr_node = nullptr;
        if (init_node->type == NODE_INIT_VAL)
            expr_node = init_node->init_val.expr;
        else if (init_node->type == NODE_CONST_INIT)
            expr_node = init_node->const_init.expr;
        else
            expr_node = init_node;

        if (expr_node)
        {
            MyIR::Value *val = visit(expr_node);
            std::vector<MyIR::Value *> indices_val = {new MyIR::ConstantInt(std::make_shared<MyIR::IntegerType>(64), 0), new MyIR::ConstantInt(std::make_shared<MyIR::IntegerType>(64), 0)};
            auto gep = new MyIR::GetElementPtrInst(array_ptr, indices_val, true, new_temp_reg_name(), current_block_);
            current_block_->get_instructions().emplace_back(gep);
            current_block_->get_instructions().emplace_back(new MyIR::StoreInst(val, gep, current_block_));
        }
        return;
    }

    std::vector<std::pair<int, ASTNode *>> initializers_ast;
    int flat_index = 0;
    std::function<void(ASTNode *, int)> flatten_and_align =
        [&](ASTNode *item, int dim_level)
    {
        if (!item || flat_index >= total_elements)
            return;
        bool is_braced_sublist = (item->type == NODE_INIT_LIST || item->type == NODE_CONST_INIT_LIST);
        if (item->type == NODE_INIT_VAL || item->type == NODE_CONST_INIT)
        {
            auto expr = (item->type == NODE_INIT_VAL) ? item->init_val.expr : item->const_init.expr;
            if (expr && (expr->type == NODE_INIT_LIST || expr->type == NODE_CONST_INIT_LIST))
            {
                is_braced_sublist = true;
                item = expr;
            }
        }

        if (is_braced_sublist)
        {
            int sub_array_size = 1;
            if (dim_level < (int)dimensions.size())
            {
                for (size_t i = dim_level + 1; i < dimensions.size(); ++i)
                    sub_array_size *= (dimensions[i] > 0 ? dimensions[i] : 1);
            }
            int start_of_sub = flat_index;
            auto sub_item = (item->type == NODE_INIT_LIST) ? item->init_list.first : item->const_init_list.first;
            while (sub_item)
            {
                if (flat_index >= start_of_sub + sub_array_size)
                    break;
                flatten_and_align(sub_item, dim_level + 1);
                sub_item = sub_item->next;
            }
            flat_index = start_of_sub + sub_array_size;
        }
        else
        {
            auto expr_to_visit = (item->type == NODE_INIT_VAL) ? item->init_val.expr : ((item->type == NODE_CONST_INIT) ? item->const_init.expr : item);
            if (expr_to_visit)
                initializers_ast.push_back({flat_index, expr_to_visit});
            flat_index++;
        }
    };
    ASTNode *top_level_item = (init_node->type == NODE_INIT_LIST) ? init_node->init_list.first : init_node->const_init_list.first;
    while (top_level_item && flat_index < total_elements)
    {
        flatten_and_align(top_level_item, 0);
        top_level_item = top_level_item->next;
    }

    DataType dest_type = (base_type->get_type_id() == MyIR::TypeID::Float) ? TYPE_FLOAT : TYPE_INT;
    for (const auto &p : initializers_ast)
    {
        MyIR::Value *val = visit(p.second);
        DataType expr_type = p.second->data_type;

        if (expr_type != dest_type)
        {
            MyIR::Opcode op;
            if (expr_type == TYPE_INT && dest_type == TYPE_FLOAT)
                op = MyIR::Opcode::SIToFP;
            else if (expr_type == TYPE_FLOAT && dest_type == TYPE_INT)
                op = MyIR::Opcode::FPToSI;

            auto cast = new MyIR::CastInst(op, val, base_type, new_temp_reg_name(), current_block_);
            current_block_->get_instructions().emplace_back(cast);
            val = cast;
        }

        std::vector<MyIR::Value *> indices_val;
        indices_val.push_back(new MyIR::ConstantInt(std::make_shared<MyIR::IntegerType>(64), 0));
        int temp_index = p.first;
        for (size_t d = 0; d < dimensions.size(); ++d)
        {
            int divisor = 1;
            for (size_t k = d + 1; k < dimensions.size(); ++k)
            {
                divisor *= dimensions[k];
            }
            indices_val.push_back(new MyIR::ConstantInt(std::make_shared<MyIR::IntegerType>(64), temp_index / divisor));
            temp_index %= divisor;
        }
        auto gep = new MyIR::GetElementPtrInst(array_ptr, indices_val, true, new_temp_reg_name(), current_block_);
        current_block_->get_instructions().emplace_back(gep);
        current_block_->get_instructions().emplace_back(new MyIR::StoreInst(val, gep, current_block_));
    }
}

std::string IRBuilder::getIR(const MyIR::IRUnit &unit)
{
    ir_stream_.str("");
    ir_stream_.clear();
    value_names_.clear();
    print_temp_counter_ = 0;

    ir_stream_ << "declare i32 @getarray(i32*)\n";
    ir_stream_ << "declare i32 @getch()\n";
    ir_stream_ << "declare i32 @getfarray(float*)\n";
    ir_stream_ << "declare float @getfloat()\n";
    ir_stream_ << "declare i32 @getint()\n";
    ir_stream_ << "declare void @putarray(i32, i32*)\n";
    ir_stream_ << "declare void @putch(i32)\n";
    ir_stream_ << "declare void @putf(i8*, ...)\n";
    ir_stream_ << "declare void @putfarray(i32, float*)\n";
    ir_stream_ << "declare void @putfloat(float)\n";
    ir_stream_ << "declare void @putint(i32)\n";
    ir_stream_ << "declare void @_sysy_starttime(i32)\n";
    ir_stream_ << "declare void @_sysy_stoptime(i32)\n";
    ir_stream_ << "declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1 immarg)\n\n";

    ir_stream_ << float_const_declarations_;

    print(unit);
    return ir_stream_.str();
}

void IRBuilder::print(const MyIR::IRUnit &unit)
{
    for (const auto &gv : unit.global_variables)
    {
        if (float_globals_.end() == std::find_if(float_globals_.begin(), float_globals_.end(),
                                                 [&](const std::pair<double, MyIR::GlobalVariable *> &pair)
                                                 { return pair.second == gv.get(); }))
        {
            print_global_variable(*gv);
            ir_stream_ << "\n";
        }
    }

    for (const auto &func : unit.functions)
    {
        if (!func->is_declaration())
        {
            print_function(*func);
            ir_stream_ << "\n";
        }
    }
}

void IRBuilder::print_global_variable(const MyIR::GlobalVariable &gv)
{
    ir_stream_ << "@" << gv.get_name() << " = ";
    if (gv.is_unnamed_addr())
        ir_stream_ << "unnamed_addr ";

    switch (gv.get_linkage())
    {
    case MyIR::LinkageType::Private:
        ir_stream_ << "private ";
        break;
    case MyIR::LinkageType::Constant:
        ir_stream_ << "constant ";
        break;
    case MyIR::LinkageType::Common:
        ir_stream_ << "common global ";
        break;
    default:
        ir_stream_ << "global ";
        break;
    }

    auto value_type = std::dynamic_pointer_cast<MyIR::PointerType>(gv.get_type())->get_element_type();
    ir_stream_ << get_type_string(value_type.get()) << " ";
    ir_stream_ << get_const_string(gv.get_initializer().get());
    if (gv.get_align().has_value())
    {
        ir_stream_ << ", align " << gv.get_align().value();
    }
}

void IRBuilder::print_function(const MyIR::Function &func)
{
    ir_stream_ << "define " << get_type_string(func.get_return_type().get()) << " @" << func.get_name() << "(";

    for (size_t i = 0; i < func.get_arguments().size(); ++i)
    {
        ir_stream_ << get_type_string(func.get_arguments()[i]->get_type().get());
        if (!func.get_arguments()[i]->get_name().empty())
        {
            ir_stream_ << " %" << func.get_arguments()[i]->get_name();
        }
        if (i < func.get_arguments().size() - 1)
            ir_stream_ << ", ";
    }
    ir_stream_ << ") {\n";

    bool first_block = true;
    for (const auto &bb : func.get_basic_blocks())
    {
        if (first_block)
        {
            ir_stream_ << bb->get_name() << ":\n";
            first_block = false;
        }
        else
        {
            ir_stream_ << "\n"
                       << bb->get_name() << ":\n";
        }
        for (const auto &inst : bb->get_instructions())
        {
            ir_stream_ << "  ";
            print_instruction(*inst);
            ir_stream_ << "\n";
        }
    }
    ir_stream_ << "}\n";
}

void IRBuilder::print_basic_block(const MyIR::BasicBlock &bb)
{
    ir_stream_ << bb.get_name() << ":\n"; // Directly print the name for definition
    for (const auto &inst : bb.get_instructions())
    {
        ir_stream_ << "  ";
        print_instruction(*inst);
        ir_stream_ << "\n";
    }
}

void IRBuilder::print_instruction(const MyIR::Instruction &inst)
{
    if (inst.get_type()->get_type_id() != MyIR::TypeID::Void)
    {
        ir_stream_ << get_value_name(&inst) << " = ";
    }

    switch (inst.get_opcode())
    {
    case MyIR::Opcode::Add:
        ir_stream_ << "add ";
        break;
    case MyIR::Opcode::FAdd:
        ir_stream_ << "fadd ";
        break;
    case MyIR::Opcode::Sub:
        ir_stream_ << "sub ";
        break;
    case MyIR::Opcode::FSub:
        ir_stream_ << "fsub ";
        break;
    case MyIR::Opcode::Mul:
        ir_stream_ << "mul ";
        break;
    case MyIR::Opcode::FMul:
        ir_stream_ << "fmul ";
        break;
    case MyIR::Opcode::SDiv:
        ir_stream_ << "sdiv ";
        break;
    case MyIR::Opcode::FDiv:
        ir_stream_ << "fdiv ";
        break;
    case MyIR::Opcode::SRem:
        ir_stream_ << "srem ";
        break;
    case MyIR::Opcode::And:
        ir_stream_ << "and ";
        break;
    case MyIR::Opcode::Or:
        ir_stream_ << "or ";
        break;
    case MyIR::Opcode::SIToFP:
        ir_stream_ << "sitofp ";
        break;
    case MyIR::Opcode::FPToSI:
        ir_stream_ << "fptosi ";
        break;
    case MyIR::Opcode::ZExt:
        ir_stream_ << "zext ";
        break;
    case MyIR::Opcode::Bitcast:
        ir_stream_ << "bitcast ";
        break;
    case MyIR::Opcode::ICmp:
        ir_stream_ << "icmp ";
        break;
    case MyIR::Opcode::FCmp:
        ir_stream_ << "fcmp ";
        break;
    case MyIR::Opcode::GEP:
        ir_stream_ << "getelementptr inbounds ";
        break;
    case MyIR::Opcode::Alloca:
        ir_stream_ << "alloca ";
        break;
    case MyIR::Opcode::Load:
        ir_stream_ << "load ";
        break;
    case MyIR::Opcode::Store:
        ir_stream_ << "store ";
        break;
    case MyIR::Opcode::Call:
        ir_stream_ << "call ";
        break;
    case MyIR::Opcode::Ret:
        ir_stream_ << "ret ";
        break;
    case MyIR::Opcode::Br:
        ir_stream_ << "br ";
        break;
    case MyIR::Opcode::PHI:
    {
        ir_stream_ << "phi " << get_type_string(inst.get_type().get()) << " ";
        for (size_t i = 0; i < inst.get_num_operands(); i += 2)
        {
            ir_stream_ << "[ " << get_value_name(inst.get_operand(i)) << ", " << get_value_name(inst.get_operand(i + 1)) << " ]";
            if (i + 2 < inst.get_num_operands())
            {
                ir_stream_ << ", ";
            }
        }

        return;
    }
    }

    if (auto *bin_inst = dynamic_cast<const MyIR::BinaryInst *>(&inst))
    {
        ir_stream_ << get_type_string(bin_inst->get_operand(0)->get_type().get()) << " "
                   << get_value_name(bin_inst->get_operand(0)) << ", "
                   << get_value_name(bin_inst->get_operand(1));
    }
    else if (auto *cmp_inst = dynamic_cast<const MyIR::CmpInst *>(&inst))
    {
        switch (cmp_inst->get_predicate())
        {
        case MyIR::CmpPredicate::EQ:
            ir_stream_ << "eq ";
            break;
        case MyIR::CmpPredicate::NE:
            ir_stream_ << "ne ";
            break;
        case MyIR::CmpPredicate::SGT:
            ir_stream_ << "sgt ";
            break;
        case MyIR::CmpPredicate::SGE:
            ir_stream_ << "sge ";
            break;
        case MyIR::CmpPredicate::SLT:
            ir_stream_ << "slt ";
            break;
        case MyIR::CmpPredicate::SLE:
            ir_stream_ << "sle ";
            break;
        case MyIR::CmpPredicate::OEQ:
            ir_stream_ << "oeq ";
            break;
        case MyIR::CmpPredicate::ONE:
            ir_stream_ << "one ";
            break;
        case MyIR::CmpPredicate::OGT:
            ir_stream_ << "ogt ";
            break;
        case MyIR::CmpPredicate::OGE:
            ir_stream_ << "oge ";
            break;
        case MyIR::CmpPredicate::OLT:
            ir_stream_ << "olt ";
            break;
        case MyIR::CmpPredicate::OLE:
            ir_stream_ << "ole ";
            break;
        }
        ir_stream_ << get_type_string(cmp_inst->get_operand(0)->get_type().get()) << " "
                   << get_value_name(cmp_inst->get_operand(0)) << ", "
                   << get_value_name(cmp_inst->get_operand(1));
    }
    else if (auto *cast_inst = dynamic_cast<const MyIR::CastInst *>(&inst))
    {
        ir_stream_ << get_type_string(cast_inst->get_operand(0)->get_type().get()) << " "
                   << get_value_name(cast_inst->get_operand(0)) << " to "
                   << get_type_string(cast_inst->get_type().get());
    }
    else if (auto *alloca_inst = dynamic_cast<const MyIR::AllocaInst *>(&inst))
    {
        ir_stream_ << get_type_string(alloca_inst->get_allocated_type().get());
        if (alloca_inst->get_align().has_value())
        {
            ir_stream_ << ", align " << alloca_inst->get_align().value();
        }
    }
    else if (auto *load_inst = dynamic_cast<const MyIR::LoadInst *>(&inst))
    {
        auto ptr_type = std::dynamic_pointer_cast<const MyIR::PointerType>(load_inst->get_operand(0)->get_type());
        assert(ptr_type && "Load instruction must have a pointer type operand.");
        ir_stream_ << get_type_string(ptr_type->get_element_type().get()) << ", "
                   << get_type_string(ptr_type.get()) << " "
                   << get_value_name(load_inst->get_operand(0));
        if (load_inst->get_align().has_value())
        {
            ir_stream_ << ", align " << load_inst->get_align().value();
        }
    }
    else if (auto *store_inst = dynamic_cast<const MyIR::StoreInst *>(&inst))
    {
        ir_stream_ << get_type_string(store_inst->get_operand(0)->get_type().get()) << " "
                   << get_value_name(store_inst->get_operand(0)) << ", "
                   << get_type_string(store_inst->get_operand(1)->get_type().get()) << " "
                   << get_value_name(store_inst->get_operand(1));
        if (store_inst->get_align().has_value())
        {
            ir_stream_ << ", align " << store_inst->get_align().value();
        }
    }
    else if (auto *gep_inst = dynamic_cast<const MyIR::GetElementPtrInst *>(&inst))
    {
        auto ptr_type = std::dynamic_pointer_cast<const MyIR::PointerType>(gep_inst->get_operand(0)->get_type());
        assert(ptr_type && "GEP instruction must have a pointer type as base operand.");
        ir_stream_ << get_type_string(ptr_type->get_element_type().get()) << ", "
                   << get_type_string(ptr_type.get()) << " "
                   << get_value_name(gep_inst->get_operand(0));
        for (size_t i = 1; i < gep_inst->get_num_operands(); ++i)
        {
            ir_stream_ << ", " << get_type_string(gep_inst->get_operand(i)->get_type().get()) << " "
                       << get_value_name(gep_inst->get_operand(i));
        }
    }
    else if (auto *call_inst = dynamic_cast<const MyIR::CallInst *>(&inst))
    {
        auto callee = call_inst->get_callee();
        ir_stream_ << get_type_string(call_inst->get_type().get()) << " ";

        ir_stream_ << get_value_name(callee) << "(";
        for (size_t i = 0; i < call_inst->get_num_operands() - 1; ++i)
        {
            ir_stream_ << get_type_string(call_inst->get_operand(i + 1)->get_type().get()) << " "
                       << get_value_name(call_inst->get_operand(i + 1));
            if (i < call_inst->get_num_operands() - 2)
                ir_stream_ << ", ";
        }
        ir_stream_ << ")";
    }
    else if (auto *ret_inst = dynamic_cast<const MyIR::ReturnInst *>(&inst))
    {
        if (ret_inst->has_return_value())
        {
            ir_stream_ << get_type_string(ret_inst->get_operand(0)->get_type().get()) << " "
                       << get_value_name(ret_inst->get_operand(0));
        }
        else
        {
            ir_stream_ << "void";
        }
    }
    else if (auto *br_inst = dynamic_cast<const MyIR::BranchInst *>(&inst))
    {
        if (br_inst->is_conditional())
        {
            ir_stream_ << "i1 " << get_value_name(br_inst->get_operand(0))
                       << ", label " << get_value_name(br_inst->get_operand(1))
                       << ", label " << get_value_name(br_inst->get_operand(2));
        }
        else
        {
            ir_stream_ << "label " << get_value_name(br_inst->get_operand(0));
        }
    }
}

std::string IRBuilder::get_value_name(const MyIR::Value *val)
{
    if (auto *c = dynamic_cast<const MyIR::Constant *>(val))
    {
        return get_const_string(c);
    }

    if (!val->get_name().empty())
    {
        bool is_global = dynamic_cast<const MyIR::GlobalVariable *>(val) || dynamic_cast<const MyIR::Function *>(val);
        if (is_global)
            return "@" + val->get_name();

        // BasicBlocks used as labels in operands must have a '%' prefix
        if (dynamic_cast<const MyIR::BasicBlock *>(val))
            return "%" + val->get_name();

        return "%" + val->get_name();
    }

    if (value_names_.find(val) == value_names_.end())
    {
        value_names_[val] = "%" + std::to_string(print_temp_counter_++);
    }
    return value_names_.at(val);
}

std::string IRBuilder::get_type_string(const MyIR::Type *type)
{
    switch (type->get_type_id())
    {
    case MyIR::TypeID::Void:
        return "void";
    case MyIR::TypeID::Label:
        return "label";
    case MyIR::TypeID::Float:
        return "float";
    case MyIR::TypeID::Integer:
    {
        auto it = dynamic_cast<const MyIR::IntegerType *>(type);
        return "i" + std::to_string(it->get_bitwidth());
    }
    case MyIR::TypeID::Pointer:
    {
        auto pt = dynamic_cast<const MyIR::PointerType *>(type);
        return get_type_string(pt->get_element_type().get()) + "*";
    }
    case MyIR::TypeID::Array:
    {
        auto at = dynamic_cast<const MyIR::ArrayType *>(type);
        return "[" + std::to_string(at->get_num_elements()) + " x " + get_type_string(at->get_element_type().get()) + "]";
    }
    case MyIR::TypeID::Function:
    {
        auto ft = dynamic_cast<const MyIR::FunctionType *>(type);
        std::string s = get_type_string(ft->get_return_type().get()) + " (";
        const auto &params = ft->get_param_types();
        for (size_t i = 0; i < params.size(); ++i)
        {
            s += get_type_string(params[i].get());
            if (i < params.size() - 1)
                s += ", ";
        }
        s += ")*";
        return s;
    }
    }
    return "";
}

std::string IRBuilder::get_const_string(const MyIR::Constant *c)
{
    if (auto ci = dynamic_cast<const MyIR::ConstantInt *>(c))
    {
        if (auto int_type = std::dynamic_pointer_cast<const MyIR::IntegerType>(ci->get_type()))
        {
            if (int_type->get_bitwidth() == 1)
            {
                return ci->get_value() != 0 ? "true" : "false";
            }
        }
        return std::to_string(ci->get_value());
    }
    if (auto cf = dynamic_cast<const MyIR::ConstantFloat *>(c))
    {
        double d_val = cf->get_value();
        if (d_val == 0.0)
            return "0.0";

        float f_val = static_cast<float>(d_val);
        double printable_d_val = static_cast<double>(f_val);

        uint64_t u64_val;
        memcpy(&u64_val, &printable_d_val, sizeof(u64_val));
        std::stringstream ss;
        ss << "0x" << std::hex << std::uppercase << std::setw(16) << std::setfill('0') << u64_val;
        return ss.str();
    }
    if (auto ca = dynamic_cast<const MyIR::ConstantArray *>(c))
    {
        if (ca->is_zero_initializer())
            return "zeroinitializer";

        std::string s = "[";
        const auto &elements = ca->get_elements();
        for (size_t i = 0; i < elements.size(); ++i)
        {
            s += get_type_string(elements[i]->get_type().get()) + " " + get_const_string(elements[i].get());
            if (i < elements.size() - 1)
                s += ", ";
        }
        s += "]";
        return s;
    }
    if (dynamic_cast<const MyIR::ConstantPointerNull *>(c))
    {
        return "null";
    }
    return "null";
}