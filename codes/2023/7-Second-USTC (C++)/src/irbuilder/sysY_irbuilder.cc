#include "sysY_irbuilder.hh"
#include "logging.hh"

#define ENABLE_MEMSET

#define __Array_Padding_IF__(bool_exp, action)                   \
                    while(bool_exp) {                            \
                        if(cur_type == FLOAT_T)                  \
                            init_val.push_back(CONST_FP(0));     \
                        else                                     \
                            init_val.push_back(CONST_INT(0));    \
                        action;                                  \
                    }    

#define __REMOVE_TERMINATOR_PROLOGUE__  cur_block_of_cur_fun = cur_basic_block_list.back();             \
                                        entry_block_of_cur_fun = cur_fun->get_entry_block();            \
                                        auto tmp_terminator = entry_block_of_cur_fun->get_terminator(); \
                                        if(tmp_terminator != nullptr)                                   \
                                            entry_block_of_cur_fun->get_instructions().pop_back();

#define __RESTORE_TERMINATOR_EPILOGUE__  if(tmp_terminator != nullptr)                                  \
                                            entry_block_of_cur_fun->add_instruction(tmp_terminator);     

#define __TO_CMP_INSTRUCTION__(instruction)                                                             \
                    Value *instruction;                                                                 \
                    if(tmp_val->get_type() == INT1_T) {                                                 \
                        instruction = tmp_val;                                                          \
                    } else if(tmp_val->get_type() == INT32_T) {                                         \
                        auto const_tmp_val = dynamic_cast<ConstantInt*>(tmp_val);                       \
                        if(const_tmp_val) {                                                             \
                            instruction = CONST_INT(const_tmp_val->get_value() != 0);                   \
                        } else {                                                                        \
                            instruction = builder->create_icmp_ne(tmp_val, CONST_INT(0));               \
                        }                                                                               \
                    } else if(tmp_val->get_type() == FLOAT_T) {                                         \
                        auto const_tmp_val = dynamic_cast<ConstantFP*>(tmp_val);                        \
                        if(const_tmp_val) {                                                             \
                            instruction = CONST_INT(const_tmp_val->get_value() != 0);                   \
                        } else {                                                                        \
                            instruction = builder->create_fcmp_ne(tmp_val, CONST_FP(0));                \
                        }                                                                               \
                    }
                     

#define __GEN_ALU_INSTRUCTION__(l_val, r_val, op, int_instruction, float_instruction)                          \
                    if(l_val->get_type() == INT1_T && r_val->get_type() == INT1_T) {                           \
                        auto l_val##_const = dynamic_cast<ConstantInt*>(l_val);                                \
                        auto r_val##_const = dynamic_cast<ConstantInt*>(r_val);                                \
                        if(l_val##_const != nullptr && r_val##_const != nullptr) {                             \
                            tmp_val = CONST_INT(l_val##_const->get_value() op r_val##_const->get_value());     \
                        } else {                                                                               \
                            l_val = builder->create_zext(l_val, INT32_T);                                      \
                            r_val = builder->create_zext(r_val, INT32_T);                                      \
                            tmp_val = builder->int_instruction(l_val, r_val);                                  \
                        }                                                                                      \
                    } else if(l_val->get_type() == INT32_T && r_val->get_type() == INT32_T) {                  \
                        auto l_val##_const = dynamic_cast<ConstantInt*>(l_val);                                \
                        auto r_val##_const = dynamic_cast<ConstantInt*>(r_val);                                \
                        if(l_val##_const != nullptr && r_val##_const != nullptr) {                             \
                            tmp_val = CONST_INT(l_val##_const->get_value() op r_val##_const->get_value());     \
                        } else {                                                                               \
                            tmp_val = builder->int_instruction(l_val, r_val);                                  \
                        }                                                                                      \
                    } else if(l_val->get_type() == FLOAT_T && r_val->get_type() == FLOAT_T) {                  \
                        auto l_val##_const = dynamic_cast<ConstantFP*>(l_val);                                 \
                        auto r_val##_const = dynamic_cast<ConstantFP*>(r_val);                                 \
                        if(l_val##_const != nullptr && r_val##_const != nullptr) {                             \
                            tmp_val = CONST_FP(l_val##_const->get_value() op r_val##_const->get_value());      \
                        } else {                                                                               \
                            tmp_val = builder->float_instruction(l_val, r_val);                                \
                        }                                                                                      \
                    } else if(l_val->get_type() == INT1_T) {                                                   \
                        if(r_val->get_type() == INT32_T) {                                                     \
                            auto l_val##_const = dynamic_cast<ConstantInt*>(l_val);                            \
                            auto r_val##_const = dynamic_cast<ConstantInt*>(r_val);                            \
                            if(l_val##_const != nullptr && r_val##_const != nullptr) {                         \ 
                                tmp_val = CONST_INT(l_val##_const->get_value() op r_val##_const->get_value()); \
                            } else {                                                                           \
                                l_val = builder->create_zext(l_val, INT32_T);                                  \
                                tmp_val = builder->int_instruction(l_val, r_val);                              \ 
                            }                                                                                  \
                        } else {                                                                               \
                            auto l_val##_const = dynamic_cast<ConstantInt*>(l_val);                            \
                            auto r_val##_const = dynamic_cast<ConstantFP*>(r_val);                             \
                            if(l_val##_const != nullptr && r_val##_const != nullptr) {                         \  
                                tmp_val = CONST_FP(l_val##_const->get_value() op r_val##_const->get_value());  \  
                            } else {                                                                           \
                                l_val = builder->create_zext(l_val, INT32_T);                                  \
                                l_val = builder->create_sitofp(l_val, FLOAT_T);                                \
                                tmp_val = builder->int_instruction(l_val, r_val);                              \ 
                            }                                                                                  \
                        }                                                                                      \
                    } else if(l_val->get_type() == INT32_T) {                                                  \
                        if(r_val->get_type() == INT1_T) {                                                      \                                               
                            auto l_val##_const = dynamic_cast<ConstantInt*>(l_val);                            \   
                            auto r_val##_const = dynamic_cast<ConstantInt*>(r_val);                            \
                            if(l_val##_const != nullptr && r_val##_const != nullptr) {                         \  
                                tmp_val = CONST_INT(l_val##_const->get_value() op r_val##_const->get_value()); \   
                            } else {                                                                           \
                                r_val = builder->create_zext(r_val, INT32_T);                                  \
                                tmp_val = builder->int_instruction(l_val, r_val);                              \ 
                            }                                                                                  \
                        } else {                                                                               \
                            auto l_val##_const = dynamic_cast<ConstantInt*>(l_val);                            \
                            auto r_val##_const = dynamic_cast<ConstantFP*>(r_val);                             \
                            if(l_val##_const != nullptr && r_val##_const != nullptr) {                         \
                                tmp_val = CONST_FP(l_val##_const->get_value() op r_val##_const->get_value());  \
                            } else {                                                                           \
                                l_val = builder->create_sitofp(l_val, FLOAT_T);                                \
                                tmp_val = builder->float_instruction(l_val, r_val);                            \
                            }                                                                                  \
                        }                                                                                      \
                    } else {                                                                                   \
                        if(r_val->get_type() == INT1_T) {                                                      \
                            auto l_val##_const = dynamic_cast<ConstantFP*>(l_val);                             \
                            auto r_val##_const = dynamic_cast<ConstantInt*>(r_val);                            \
                            if(l_val##_const != nullptr && r_val##_const != nullptr) {                         \
                                tmp_val = CONST_FP(l_val##_const->get_value() op r_val##_const->get_value());  \
                            } else {                                                                           \
                                r_val = builder->create_zext(r_val, INT32_T);                                  \
                                r_val = builder->create_sitofp(r_val, FLOAT_T);                                \
                                tmp_val = builder->float_instruction(l_val, r_val);                            \
                            }                                                                                  \
                        } else {                                                                               \
                            auto l_val##_const = dynamic_cast<ConstantFP*>(l_val);                             \
                            auto r_val##_const = dynamic_cast<ConstantInt*>(r_val);                            \
                            if(l_val##_const != nullptr && r_val##_const != nullptr) {                         \
                                tmp_val = CONST_FP(l_val##_const->get_value() op r_val##_const->get_value());  \
                            } else {                                                                           \
                                r_val = builder->create_sitofp(r_val, FLOAT_T);                                \
                                tmp_val = builder->float_instruction(l_val, r_val);                            \
                            }                                                                                  \
                        }                                                                                      \
                    }                                                          
 
#define __GEN_CMP_INSTRUCTION__(l_val, r_val, op, int_instruction, float_instruction)                          \
                    if(l_val->get_type() == INT1_T && r_val->get_type() == INT1_T) {                           \
                        auto l_val##_const = dynamic_cast<ConstantInt*>(l_val);                                \
                        auto r_val##_const = dynamic_cast<ConstantInt*>(r_val);                                \
                        if(l_val##_const != nullptr && r_val##_const != nullptr) {                             \
                            tmp_val = CONST_INT(l_val##_const->get_value() op r_val##_const->get_value());     \
                        } else {                                                                               \
                            l_val = builder->create_zext(l_val, INT32_T);                                      \
                            r_val = builder->create_zext(r_val, INT32_T);                                      \
                            tmp_val = builder->int_instruction(l_val, r_val);                                  \
                        }                                                                                      \
                    } else if(l_val->get_type() == INT32_T && r_val->get_type() == INT32_T) {                  \
                        auto l_val##_const = dynamic_cast<ConstantInt*>(l_val);                                \
                        auto r_val##_const = dynamic_cast<ConstantInt*>(r_val);                                \
                        if(l_val##_const != nullptr && r_val##_const != nullptr) {                             \
                            tmp_val = CONST_INT(l_val##_const->get_value() op r_val##_const->get_value());     \
                        } else {                                                                               \
                            tmp_val = builder->int_instruction(l_val, r_val);                                  \
                        }                                                                                      \
                    } else if(l_val->get_type() == FLOAT_T && r_val->get_type() == FLOAT_T) {                  \
                        auto l_val##_const = dynamic_cast<ConstantFP*>(l_val);                                 \
                        auto r_val##_const = dynamic_cast<ConstantFP*>(r_val);                                 \
                        if(l_val##_const != nullptr && r_val##_const != nullptr) {                             \
                            tmp_val = CONST_INT(l_val##_const->get_value() op r_val##_const->get_value());     \
                        } else {                                                                               \
                            tmp_val = builder->float_instruction(l_val, r_val);                                \
                        }                                                                                      \
                    } else if(l_val->get_type() == INT1_T) {                                                   \
                        if(r_val->get_type() == INT32_T) {                                                     \
                            auto l_val##_const = dynamic_cast<ConstantInt*>(l_val);                            \
                            auto r_val##_const = dynamic_cast<ConstantInt*>(r_val);                            \
                            if(l_val##_const != nullptr && r_val##_const != nullptr) {                         \ 
                                tmp_val = CONST_INT(l_val##_const->get_value() op r_val##_const->get_value()); \
                            } else {                                                                           \
                                l_val = builder->create_zext(l_val, INT32_T);                                  \
                                tmp_val = builder->int_instruction(l_val, r_val);                              \ 
                            }                                                                                  \
                        } else {                                                                               \
                            auto l_val##_const = dynamic_cast<ConstantInt*>(l_val);                            \
                            auto r_val##_const = dynamic_cast<ConstantFP*>(r_val);                             \
                            if(l_val##_const != nullptr && r_val##_const != nullptr) {                         \  
                                tmp_val = CONST_INT(l_val##_const->get_value() op r_val##_const->get_value()); \  
                            } else {                                                                           \
                                l_val = builder->create_zext(l_val, INT32_T);                                  \
                                l_val = builder->create_sitofp(l_val, FLOAT_T);                                \
                                tmp_val = builder->float_instruction(l_val, r_val);                            \ 
                            }                                                                                  \
                        }                                                                                      \
                    } else if(l_val->get_type() == INT32_T) {                                                  \
                        if(r_val->get_type() == INT1_T) {                                                      \                                               
                            auto l_val##_const = dynamic_cast<ConstantInt*>(l_val);                            \   
                            auto r_val##_const = dynamic_cast<ConstantInt*>(r_val);                            \
                            if(l_val##_const != nullptr && r_val##_const != nullptr) {                         \  
                                tmp_val = CONST_INT(l_val##_const->get_value() op r_val##_const->get_value()); \   
                            } else {                                                                           \
                                r_val = builder->create_zext(r_val, INT32_T);                                  \
                                tmp_val = builder->int_instruction(l_val, r_val);                              \ 
                            }                                                                                  \
                        } else {                                                                               \
                            auto l_val##_const = dynamic_cast<ConstantInt*>(l_val);                            \
                            auto r_val##_const = dynamic_cast<ConstantFP*>(r_val);                             \
                            if(l_val##_const != nullptr && r_val##_const != nullptr) {                         \
                                tmp_val = CONST_INT(l_val##_const->get_value() op r_val##_const->get_value()); \
                            } else {                                                                           \
                                l_val = builder->create_sitofp(l_val, FLOAT_T);                                \
                                tmp_val = builder->float_instruction(l_val, r_val);                            \
                            }                                                                                  \
                        }                                                                                      \
                    } else {                                                                                   \
                        if(r_val->get_type() == INT1_T) {                                                      \
                            auto l_val##_const = dynamic_cast<ConstantFP*>(l_val);                             \
                            auto r_val##_const = dynamic_cast<ConstantInt*>(r_val);                            \
                            if(l_val##_const != nullptr && r_val##_const != nullptr) {                         \
                                tmp_val = CONST_INT(l_val##_const->get_value() op r_val##_const->get_value()); \
                            } else {                                                                           \
                                r_val = builder->create_zext(r_val, INT32_T);                                  \
                                r_val = builder->create_sitofp(r_val, FLOAT_T);                                \
                                tmp_val = builder->float_instruction(l_val, r_val);                            \
                            }                                                                                  \
                        } else {                                                                               \
                            auto l_val##_const = dynamic_cast<ConstantFP*>(l_val);                             \
                            auto r_val##_const = dynamic_cast<ConstantInt*>(r_val);                            \
                            if(l_val##_const != nullptr && r_val##_const != nullptr) {                         \
                                tmp_val = CONST_INT(l_val##_const->get_value() op r_val##_const->get_value()); \
                            } else {                                                                           \
                                r_val = builder->create_sitofp(r_val, FLOAT_T);                                \
                                tmp_val = builder->float_instruction(l_val, r_val);                            \
                            }                                                                                  \
                        }                                                                                      \
                    }     
                     
                    

#define CONST_INT(num)  ConstantInt::get(num, module.get())
#define CONST_FP(num)   ConstantFP::get(num, module.get())

//& global variables

Value *tmp_val = nullptr;       //& store tmp value
Type  *cur_type = nullptr;      //& store current type
bool require_lvalue = false;    //& whether require lvalue
Function *cur_fun = nullptr;    //& function that is being built
bool pre_enter_scope = false;   //& whether pre-enter scope

Type *VOID_T;
Type *INT1_T;
Type *INT32_T;
Type *FLOAT_T;
Type *INT32PTR_T;
Type *FLOATPTR_T;               //& types used for IR builder

struct true_false_BB {
    BasicBlock *trueBB = nullptr;
    BasicBlock *falseBB = nullptr;
};                              //& used for backpatching

std::list<true_false_BB> IF_WHILE_Cond_Stack; //& used for Cond
std::list<true_false_BB> While_Stack;         //& used for break and continue



std::vector<BasicBlock*> cur_basic_block_list;

BasicBlock *entry_block_of_cur_fun;
BasicBlock *cur_block_of_cur_fun;   //& used for add instruction 

std::vector<int> array_bounds;
std::vector<int> array_sizes;
int cur_pos;
int cur_depth;     
std::map<int, Value*> init_val_map; 
std::vector<Constant*> init_val;    //& used for constant initializer

BasicBlock *ret_BB;
Value *ret_addr;   //& ret BB

bool is_inited_with_all_zero;




void sysY_irbuilder::visit(ASTCompUnit &node) {
    VOID_T = Type::get_void_type(module.get());
    INT1_T = Type::get_int1_type(module.get());
    INT32_T = Type::get_int32_type(module.get());
    INT32PTR_T = Type::get_int32_ptr_type(module.get());
    FLOAT_T = Type::get_float_type(module.get());
    FLOATPTR_T = Type::get_float_ptr_type(module.get());
    for (auto decl : node.Declaration_list) {
        decl->accept(*this);
    }
}

void sysY_irbuilder::visit(ASTConstDeclaration &node) {
    if (node.type == ASTType::TYPE_INT)
        cur_type = INT32_T;
    else
        cur_type = FLOAT_T;
    for (auto const_def : node.ConstDef_list) {
        const_def->accept(*this);        
    }
}
        
void sysY_irbuilder::visit(ASTConstDef &node) {
    if (node.ConstExp_list.empty()) {
        if(node.ConstInitVal == nullptr) {
            LOG(ERROR) << "Define const var without initializer";
        }
        node.ConstInitVal->accept(*this);
        if(cur_type == FLOAT_T) {
            auto initializer = dynamic_cast<ConstantFP*>(tmp_val);
            if(initializer == nullptr) {
                LOG(ERROR) << "You can initialize const var only with const var or literal";
            }
            auto var = CONST_FP(initializer->get_value());
            scope.push(node.id, var);
        } else {
            auto initializer = dynamic_cast<ConstantInt*>(tmp_val);
            if(initializer == nullptr) {
                LOG(ERROR) << "You can initialize const var only with const var or literal";
            }
            auto var = CONST_INT(initializer->get_value());
            scope.push(node.id, var);
        }
    } else {
        array_bounds.clear();
        array_sizes.clear();
        for(const auto& bound_expr : node.ConstExp_list) {
            bound_expr->accept(*this);
            auto bound = dynamic_cast<ConstantInt*>(tmp_val);
            if(bound == nullptr) {
                LOG(ERROR) << "Array bounds must be const int var or literal";
            }
            array_bounds.push_back(bound->get_value());
        }
        int total_size = 1;
        for(auto iter = array_bounds.rbegin(); iter != array_bounds.rend(); iter++) {
            array_sizes.insert(array_sizes.begin(), total_size);
            total_size *= (*iter);
        }
        array_sizes.insert(array_sizes.begin(), total_size);

        auto array_type = ArrayType::get(cur_type, total_size);
        init_val.clear();
        cur_depth = 0;
        cur_pos = 0;
        node.ConstInitVal->accept(*this);
        auto initializer = ConstantArray::get(array_type, init_val);
        if(scope.in_global()) {
            auto var = GlobalVariable::create(node.id, module.get(), array_type, true, initializer);
            scope.push(node.id, var);
            scope.push_size(node.id, array_sizes);
            scope.push_const(node.id, initializer);
        } else {
            __REMOVE_TERMINATOR_PROLOGUE__
            auto var = builder->create_alloca(array_type);
            cur_block_of_cur_fun->get_instructions().pop_back();
            entry_block_of_cur_fun->add_instruction(var);
            var->set_parent(entry_block_of_cur_fun);
            __RESTORE_TERMINATOR_EPILOGUE__
            for(int i = 0; i < array_sizes[0]; i++) {
                if(init_val_map[i]) {
                    builder->create_store(init_val_map[i], builder->create_gep(var, {CONST_INT(0), CONST_INT(i)}));
                } else {
                    if (cur_type == INT32_T) {
                        builder->create_store(CONST_INT(0), builder->create_gep(var, {CONST_INT(0), CONST_INT(i)}));
                    } else {
                        builder->create_store(CONST_FP(0), builder->create_gep(var, {CONST_INT(0), CONST_INT(i)}));
                    }
                } 
            }
            scope.push(node.id, var);
            scope.push_size(node.id, array_sizes);
            scope.push_const(node.id, initializer);
        }
    }
}

void sysY_irbuilder::visit(ASTConstInitVal &node) {
    if(node.ConstInitVal_list.empty() && node.ConstExp) {
        node.ConstExp->accept(*this);
        auto tmp_int32_val = dynamic_cast<ConstantInt*>(tmp_val);
        auto tmp_float_val = dynamic_cast<ConstantFP*>(tmp_val);
        if (cur_type == INT32_T && tmp_float_val != nullptr) {
            tmp_val = CONST_INT(int(tmp_float_val->get_value()));
        } else if(cur_type == FLOAT_T && tmp_int32_val != nullptr) {
            tmp_val = CONST_FP(float(tmp_int32_val->get_value()));
        }
        init_val_map[cur_pos] = tmp_val;
        init_val.push_back(static_cast<Constant*>(tmp_val));
        cur_pos++;
    } else {
        if(cur_depth != 0) {
            __Array_Padding_IF__(cur_pos % array_sizes[cur_depth] != 0, cur_pos++);
        }
        int cur_start_pos = cur_pos;
        for(const auto& const_init_val : node.ConstInitVal_list) {
            cur_depth++;
            const_init_val->accept(*this);
            cur_depth--;
        }
        if(cur_depth != 0) {
            __Array_Padding_IF__(cur_pos < cur_start_pos + array_sizes[cur_depth], cur_pos++);
        } else {
            __Array_Padding_IF__(cur_pos < array_sizes[0], cur_pos++);
        }
    }
}

void sysY_irbuilder::visit(ASTVarDeclaration &node) {
    if (node.type == ASTType::TYPE_INT)
        cur_type = INT32_T;
    else
        cur_type = FLOAT_T;
    for (auto def : node.VarDef_list)
        def->accept(*this);
}

void sysY_irbuilder::visit(ASTVarDef &node) {
    if(node.ConstExp_list.empty()) {
        if(scope.in_global()) {
            if(node.InitVal != nullptr) {
                node.InitVal->accept(*this);
                if(cur_type == FLOAT_T) {
                    auto initializer = dynamic_cast<ConstantFP*>(tmp_val);
                    if(initializer == nullptr) {
                        LOG(ERROR) << "You can initialize global var only with const var or literal";
                    }
                    auto var = GlobalVariable::create(node.id, module.get(), cur_type, false, initializer);
                    scope.push(node.id, var);
                } else {
                    auto initializer = dynamic_cast<ConstantInt*>(tmp_val);
                    if(initializer == nullptr) {
                        LOG(ERROR) << "You can initialize const var only with const var or literal";
                    }
                    auto var = GlobalVariable::create(node.id, module.get(), cur_type, false, initializer);
                    scope.push(node.id, var);
                }
            } else {
                auto initializer = ConstantZero::get(cur_type, module.get());
                auto var = GlobalVariable::create(node.id, module.get(), cur_type, false, initializer);
                scope.push(node.id, var);
            }
        } else {
            __REMOVE_TERMINATOR_PROLOGUE__
            auto var = builder->create_alloca(cur_type);
            cur_block_of_cur_fun->get_instructions().pop_back();
            entry_block_of_cur_fun->add_instruction(var);
            __RESTORE_TERMINATOR_EPILOGUE__
            if(node.InitVal != nullptr) {
                node.InitVal->accept(*this);
                builder->create_store(tmp_val, var);
            }
            scope.push(node.id, var);
        }
    } else {
        array_bounds.clear();
        array_sizes.clear();
        for(const auto& bound_expr : node.ConstExp_list) {
            bound_expr->accept(*this);
            auto bound = dynamic_cast<ConstantInt*>(tmp_val);
            if(bound == nullptr) {
                LOG(ERROR) << "Array bounds must be const int var or literal";
            }
            array_bounds.push_back(bound->get_value());
        }
        int total_size = 1;
        for(auto iter = array_bounds.rbegin(); iter != array_bounds.rend(); iter++) {
            array_sizes.insert(array_sizes.begin(), total_size);
            total_size *= (*iter);
        }
        array_sizes.insert(array_sizes.begin(), total_size);

        auto array_type = ArrayType::get(cur_type, total_size);
        init_val.clear();
        init_val_map.clear();
        cur_depth = 0;
        cur_pos = 0;
        if(scope.in_global()) {
            if(node.InitVal) {
                if(node.InitVal->InitVal_list.empty()) {
                    auto initializer = ConstantZero::get(array_type, module.get());
                    auto var = GlobalVariable::create(node.id, module.get(), array_type, false, initializer);
                    scope.push(node.id, var);
                    scope.push_size(node.id, array_sizes);
                } else if(is_all_zero(*(node.InitVal.get()))) {
                    auto initializer = ConstantZero::get(array_type, module.get());
                    auto var = GlobalVariable::create(node.id, module.get(), array_type, false, initializer);
                    scope.push(node.id, var);
                    scope.push_size(node.id, array_sizes);
                } else {
                    node.InitVal->accept(*this);
                    auto initializer = ConstantArray::get(array_type, init_val);
                    auto var = GlobalVariable::create(node.id, module.get(), array_type, false, initializer);
                    scope.push(node.id, var);
                    scope.push_size(node.id, array_sizes);
                }
            } else {
                auto initializer = ConstantZero::get(array_type, module.get());
                auto var = GlobalVariable::create(node.id, module.get(), array_type, false, initializer);
                scope.push(node.id, var);
                scope.push_size(node.id, array_sizes);
            }
        } else {
            __REMOVE_TERMINATOR_PROLOGUE__
            auto var = builder->create_alloca(array_type);
            cur_block_of_cur_fun->get_instructions().pop_back();
            entry_block_of_cur_fun->add_instruction(var);
            __RESTORE_TERMINATOR_EPILOGUE__
            if(node.InitVal) {
                #ifdef ENABLE_MEMSET
                    if(node.InitVal->InitVal_list.empty()) { 
                        builder->create_memset(var);
                    } else {
                        if(is_all_zero(*(node.InitVal.get()))) {
                            builder->create_memset(var);
                        } else {
                            node.InitVal->accept(*this);
                            for(int i = 0; i < array_sizes[0]; i++) {
                                if(init_val_map[i]) {
                                    builder->create_store(init_val_map[i], builder->create_gep(var, {CONST_INT(0), CONST_INT(i)}));
                                } else {
                                    if (cur_type == INT32_T) {
                                        builder->create_store(CONST_INT(0), builder->create_gep(var, {CONST_INT(0), CONST_INT(i)}));
                                    } else {
                                        builder->create_store(CONST_FP(0), builder->create_gep(var, {CONST_INT(0), CONST_INT(i)}));
                                    }
                                }
                            }
                        }
                        
                    }
                #else
                    node.InitVal->accept(*this);
                    for(int i = 0; i < array_sizes[0]; i++) {
                        if(init_val_map[i]) {
                            builder->create_store(init_val_map[i], builder->create_gep(var, {CONST_INT(0), CONST_INT(i)}));
                        } else {
                            if (cur_type == INT32_T) {
                                builder->create_store(CONST_INT(0), builder->create_gep(var, {CONST_INT(0), CONST_INT(i)}));
                            } else {
                                builder->create_store(CONST_FP(0), builder->create_gep(var, {CONST_INT(0), CONST_INT(i)}));
                            }
                        }
                    } 
                #endif
            }
            scope.push(node.id, var);
            scope.push_size(node.id, array_sizes);
        }
    }
}

void sysY_irbuilder::visit(ASTInitVal &node) {
    if(node.InitVal_list.empty() && node.Exp) {
        node.Exp->accept(*this);
        if (cur_type == INT32_T) {
            auto tmp_float_val = dynamic_cast<ConstantFP*>(tmp_val);
            if(tmp_float_val != nullptr) {
                tmp_val = CONST_INT(int(tmp_float_val->get_value()));
            } else if(tmp_val->get_type() == FLOAT_T) {
                tmp_val = builder->create_fptosi(tmp_val, INT32_T);
            } 
        } else if(cur_type == FLOAT_T) {
            auto tmp_int32_val = dynamic_cast<ConstantInt*>(tmp_val);
            if(tmp_int32_val != nullptr) {
                tmp_val = CONST_FP(float(tmp_int32_val->get_value()));
            } else if(tmp_val->get_type() == INT32_T) { 
                tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
            }
        } 
        init_val_map[cur_pos] = tmp_val;
        init_val.push_back(dynamic_cast<Constant*>(tmp_val));
        cur_pos++;
        
    } else {
        if(cur_depth != 0) {
            __Array_Padding_IF__(cur_pos % array_sizes[cur_depth] != 0, cur_pos++);
        }
        int cur_start_pos = cur_pos;
        for(const auto& const_init_val : node.InitVal_list) {
            cur_depth++;
            const_init_val->accept(*this);
            cur_depth--;
        }
        if(cur_depth != 0) {
            __Array_Padding_IF__(cur_pos < cur_start_pos + array_sizes[cur_depth], cur_pos++);
        } else {
            __Array_Padding_IF__(cur_pos < array_sizes[0], cur_pos++);
        }
    }
}

void sysY_irbuilder::visit(ASTFuncDef &node) {
    FunctionType *fun_type;
    Type *ret_type;
    if(node.type == ASTType::TYPE_FLOAT) 
        ret_type = FLOAT_T;
    else if(node.type == ASTType::TYPE_INT)
        ret_type = INT32_T;
    else
        ret_type = VOID_T;

    std::vector<Type*> param_types;
    for(auto func_param : node.FuncFParam_list) {
      func_param->accept(*this);
      param_types.push_back(cur_type);
    }
    fun_type = FunctionType::get(ret_type, param_types);
    auto fun = Function::create(fun_type, node.id, module.get());
    scope.push_func(node.id, fun);
    cur_fun = fun;
    auto entryBB = BasicBlock::create(module.get(), "entry", fun);
    builder->set_insert_point(entryBB);
    cur_basic_block_list.push_back(entryBB);
    scope.enter();
    pre_enter_scope = true;
    std::vector<Value*> args;
    for(auto arg = fun->arg_begin(); arg != fun->arg_end(); ++arg) {
      args.push_back(*arg);
    }
    int param_num = args.size();
    for(int i = 0; i < param_num; ++i) {
      if(! node.FuncFParam_list[i]->is_array) {
          Value* alloc = builder->create_alloca(param_types[i]);
          builder->create_store(args[i], alloc);
          scope.push(node.FuncFParam_list[i]->id, alloc);
      } else {
          Value* alloc_array;
          alloc_array = builder->create_alloca(param_types[i]);
          builder->create_store(args[i], alloc_array);
          scope.push(node.FuncFParam_list[i]->id, alloc_array);
          array_bounds.clear();
          array_sizes.clear();
          for(auto bound_expr : node.FuncFParam_list[i]->ParamArrayExp_list) {
              if(bound_expr == nullptr) {
                  array_bounds.push_back(1);
              } else {
                  bound_expr->accept(*this);
                  auto bound = dynamic_cast<ConstantInt*>(tmp_val);
                  if(bound == nullptr) {
                    LOG(ERROR) << "Array bounds must be const int var or literal";
                  }
                  array_bounds.push_back(bound->get_value());
              }
          }
          int total_size = 1;
          for(auto iter = array_bounds.rbegin(); iter != array_bounds.rend(); iter++) {
              array_sizes.insert(array_sizes.begin(), total_size);
              total_size *= (*iter);
          }
          array_sizes.insert(array_sizes.begin(), total_size);
          scope.push_size(node.FuncFParam_list[i]->id, array_sizes);
      }
    }
    //& ret
    if(ret_type == FLOAT_T) 
        ret_addr = builder->create_alloca(FLOAT_T);
    else if(ret_type == INT32_T)
        ret_addr = builder->create_alloca(INT32_T);
    ret_BB = BasicBlock::create(module.get(), "ret", fun);
    node.Block->accept(*this);
    if(builder->get_insert_block()->get_terminator() == nullptr) {
        if(cur_fun->get_return_type() == FLOAT_T) {
            builder->create_store(CONST_FP(0), ret_addr);
        } else if(cur_fun->get_return_type() == INT32_T) {
            builder->create_store(CONST_INT(0), ret_addr);
        }
        builder->create_br(ret_BB);
    }
    scope.exit();
    cur_basic_block_list.pop_back();
    builder->set_insert_point(ret_BB);
    if(fun->get_return_type() == VOID_T) {
        builder->create_void_ret();
    } else {
      auto ret_val = builder->create_load(ret_addr);
      builder->create_ret(ret_val);
    }
}

void sysY_irbuilder::visit(ASTFuncFParam &node) {
    if (node.type == ASTType::TYPE_INT)
        cur_type = INT32_T;
    else
        cur_type = FLOAT_T;
    if (node.is_array) {
        if(cur_type == INT32_T) 
            cur_type = INT32PTR_T;
        else
            cur_type = FLOATPTR_T;
    }
}

void sysY_irbuilder::visit(ASTBlock &node) {
    bool need_exit_scope = !pre_enter_scope;
    if(pre_enter_scope) {
      pre_enter_scope = false;
    } else {
      scope.enter();
    }
    for(auto block_item : node.BlockItem_list) {
      block_item->accept(*this);
      if(builder->get_insert_block()->get_terminator() != nullptr) {
        break;
      }
    }
    if(need_exit_scope) {
      scope.exit();
    }
}

void sysY_irbuilder::visit(ASTAssignStmt &node) {
    node.Exp->accept(*this);
    auto result = tmp_val;
    require_lvalue = true;
    node.LVal->accept(*this);
    auto addr = tmp_val;
    if (addr->get_type()->get_pointer_element_type()->is_integer_type() && result->get_type()->is_float_type()) {
        auto const_result = dynamic_cast<ConstantFP*>(result);
        if(const_result) {
            result = CONST_INT(int(const_result->get_value()));
        } else {
            result = builder->create_fptosi(result, INT32_T);
        }
    } else if (addr->get_type()->get_pointer_element_type()->is_float_type() && result->get_type()->is_integer_type()) {
        auto const_result = dynamic_cast<ConstantInt*>(result);
        if(const_result) {
            result = CONST_FP(float(const_result->get_value()));
        } else {
            result = builder->create_sitofp(result, FLOAT_T);
        }
    }
    builder->create_store(result, addr);
    tmp_val = result;
} 

void sysY_irbuilder::visit(ASTExpStmt &node) {
  if (node.Exp)
      node.Exp->accept(*this);
}

void sysY_irbuilder::visit(ASTBlockStmt &node) {
  if (node.Block)
    node.Block->accept(*this);
  else
    LOG(ERROR) << "unreachable!";
}

void sysY_irbuilder::visit(ASTSelectStmt &node) {
    auto true_BB = BasicBlock::create(module.get(), "", cur_fun);
    auto false_BB = BasicBlock::create(module.get(), "", cur_fun);
    auto next_BB = BasicBlock::create(module.get(), "", cur_fun);
    IF_WHILE_Cond_Stack.push_back({nullptr, nullptr});
    IF_WHILE_Cond_Stack.back().trueBB = true_BB;
    if(node.ElseStmt == nullptr) {
       IF_WHILE_Cond_Stack.back().falseBB = next_BB;
    } else {
      IF_WHILE_Cond_Stack.back().falseBB = false_BB;
    }
    node.Cond->accept(*this);
    IF_WHILE_Cond_Stack.pop_back();
    __TO_CMP_INSTRUCTION__(cond_val)
    if(node.ElseStmt == nullptr) {
        builder->create_cond_br(cond_val, true_BB, next_BB);
    } else {
        builder->create_cond_br(cond_val, true_BB, false_BB);
    }
    cur_basic_block_list.pop_back();
    builder->set_insert_point(true_BB);
    cur_basic_block_list.push_back(true_BB);
    if(dynamic_cast<ASTBlockStmt*>(node.IfStmt.get())) {
        node.IfStmt->accept(*this);
    } else {
        scope.enter();
        node.IfStmt->accept(*this);
        scope.exit();
    }
    if(builder->get_insert_block()->get_terminator() == nullptr) {
        builder->create_br(next_BB);
    } 
    cur_basic_block_list.pop_back();
    if(node.ElseStmt == nullptr) {
        false_BB->erase_from_parent();
    } else {
        builder->set_insert_point(false_BB);
        cur_basic_block_list.push_back(false_BB);
        if(dynamic_cast<ASTBlockStmt*>(node.ElseStmt.get())) {
          node.ElseStmt->accept(*this);
        } else {
          scope.enter();
          node.ElseStmt->accept(*this);
          scope.exit();
        }
      if(builder->get_insert_block()->get_terminator() == nullptr) {
          builder->create_br(next_BB);
      }
      cur_basic_block_list.pop_back();
    }
    builder->set_insert_point(next_BB);
    cur_basic_block_list.push_back(next_BB);
    if(next_BB->get_pre_basic_blocks().size() == 0) {
        builder->set_insert_point(true_BB);
        next_BB->erase_from_parent();
    }
}

void sysY_irbuilder::visit(ASTIterationStmt &node) {
    auto cond_BB = BasicBlock::create(module.get(), "", cur_fun);
    auto iter_BB = BasicBlock::create(module.get(), "", cur_fun);
    auto next_BB = BasicBlock::create(module.get(), "", cur_fun);
    While_Stack.push_back({cond_BB, next_BB});
    if(builder->get_insert_block()->get_terminator() == nullptr) {
      builder->create_br(cond_BB);
    }
    cur_basic_block_list.pop_back();
    builder->set_insert_point(cond_BB);
    IF_WHILE_Cond_Stack.push_back({iter_BB, next_BB});
    node.Cond->accept(*this);
    IF_WHILE_Cond_Stack.pop_back();
    __TO_CMP_INSTRUCTION__(cond_val)
    builder->create_cond_br(cond_val, iter_BB, next_BB);
    builder->set_insert_point(iter_BB);
    cur_basic_block_list.push_back(iter_BB);
    if(dynamic_cast<ASTBlockStmt*>(node.Stmt.get())) {
        node.Stmt->accept(*this);
    } else {
        scope.enter();
        node.Stmt->accept(*this);
        scope.exit();
    }
    if(builder->get_insert_block()->get_terminator() == nullptr) {
        builder->create_br(cond_BB);
    }
    cur_basic_block_list.pop_back();
    builder->set_insert_point(next_BB);
    cur_basic_block_list.push_back(next_BB);
    While_Stack.pop_back();
}

void sysY_irbuilder::visit(ASTBreakStmt &node) {
    builder->create_br(While_Stack.back().falseBB);
}

void sysY_irbuilder::visit(ASTContinueStmt &node) {
    builder->create_br(While_Stack.back().trueBB);
}

void sysY_irbuilder::visit(ASTReturnStmt &node) {
    if(node.Exp != nullptr) {
        node.Exp->accept(*this);
        if(cur_fun->get_return_type()->is_integer_type()) {
            auto tmp_float_val = dynamic_cast<ConstantFP*>(tmp_val);
            if(tmp_float_val != nullptr) {
                tmp_val = CONST_INT(int(tmp_float_val->get_value()));
            } else if(tmp_val->get_type() == FLOAT_T) {
                tmp_val = builder->create_fptosi(tmp_val, INT32_T);
            } 
            builder->create_store(tmp_val, ret_addr);
        } else {
            auto tmp_int32_val = dynamic_cast<ConstantInt*>(tmp_val);
            if(tmp_int32_val != nullptr) {
                tmp_val = CONST_FP(float(tmp_int32_val->get_value()));
            } else if(tmp_val->get_type() == INT32_T) { 
                tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
            }
            builder->create_store(tmp_val, ret_addr);
        }
    } 
    builder->create_br(ret_BB); 
}


void sysY_irbuilder::visit(ASTLVal &node) {
    auto var = scope.find(node.id);
    bool should_return_lvalue = require_lvalue;
    require_lvalue = false;
    if(node.ArrayExp_list.empty()) {
        if(should_return_lvalue) {
            if(var->get_type()->get_pointer_element_type()->is_array_type()) {
                tmp_val = builder->create_gep(var, {CONST_INT(0), CONST_INT(0)});
            } else if(var->get_type()->get_pointer_element_type()->is_pointer_type()) {
                tmp_val = builder->create_load(var);
            } else {
                tmp_val = var;
            }
        } else {
            if(var->get_type() == FLOAT_T) {
                auto val_const = dynamic_cast<ConstantFP*>(var);
                if(val_const != nullptr) 
                    tmp_val = val_const;
                else
                    tmp_val = builder->create_load(var);      
            } else {
                auto val_const = dynamic_cast<ConstantInt*>(var);
                if(val_const != nullptr) 
                    tmp_val = val_const;
                else
                    tmp_val = builder->create_load(var);
            }
        }
    } else {
        auto var_sizes = scope.find_size(node.id);
        std::vector<Value*> var_indexs;
        Value *var_index = nullptr;
        int index_const = 0;
        bool const_check = true;
        auto const_array = scope.find_const(node.id);
        if(const_array == nullptr) 
            const_check = false;
        for(int i = 0; i < node.ArrayExp_list.size(); ++i) {
            node.ArrayExp_list[i]->accept(*this);
            var_indexs.push_back(tmp_val);
            if(const_check == true) {
                auto tmp_const = dynamic_cast<ConstantInt*>(tmp_val);
                if(tmp_const == nullptr) {
                    const_check = false;
                } else {
                    index_const = var_sizes[i+1] * tmp_const->get_value() + index_const;
                }
            }
        }
        if(should_return_lvalue == false && const_check == true) {
            ConstantInt *tmp_const = dynamic_cast<ConstantInt*>(const_array->get_element_value(index_const));
            tmp_val = CONST_INT(tmp_const->get_value());
        } else {
            for(int i = 0; i < var_indexs.size(); i++) {
                auto index_val = var_indexs[i];
                Value* one_index;
                if(var_sizes[i+1] > 1) {
                    one_index = builder->create_imul(CONST_INT(var_sizes[i+1]), index_val);
                } else {
                    one_index = index_val;
                }
                if(var_index == nullptr) {
                    var_index = one_index;
                } else {
                    var_index = builder->create_iadd(var_index, one_index);
                }
            }
            if(var->get_type()->get_pointer_element_type()->is_pointer_type()) {
                auto tmp_load = builder->create_load(var);
                tmp_val = builder->create_gep(tmp_load, {var_index});
            } else {
                tmp_val = builder->create_gep(var, {CONST_INT(0), var_index});
            }
            if(!should_return_lvalue)
                tmp_val = builder->create_load(tmp_val);
        }
    }
}

void sysY_irbuilder::visit(ASTNumber &node) {
    switch(node.type) {
      case ASTType::TYPE_INT: 
          tmp_val = ConstantInt::get(node.i_val, module.get());
          break;
      case ASTType::TYPE_FLOAT:
          tmp_val = ConstantFP::get(node.f_val, module.get());
          break;
      default:
          LOG(ERROR) << "unreachable!";
          break;
    }
}

void sysY_irbuilder::visit(ASTUnaryExp &node) {
    if(node.PrimaryExp != nullptr) {
        node.PrimaryExp->accept(*this);
    } else if(node.UnaryExp != nullptr) {
        node.UnaryExp->accept(*this);
    } else {
        node.Callee->accept(*this);
    }
    switch(node.op) {
        case UnaryOp::OP_NEG:
            if(tmp_val->get_type()->is_float_type()) {
                Value* lhs = CONST_FP(0);
                Value* rhs = tmp_val;
                __GEN_ALU_INSTRUCTION__(lhs, rhs, -, create_isub, create_fsub)
            } else {
                Value* lhs = CONST_INT(0);
                Value* rhs = tmp_val;
                __GEN_ALU_INSTRUCTION__(lhs, rhs, -, create_isub, create_fsub)
            }
            break;
        case UnaryOp::OP_NOT: {
                auto fcmp_inst = dynamic_cast<FCmpInst*>(tmp_val);
                auto icmp_inst = dynamic_cast<CmpInst*>(tmp_val);
                if(fcmp_inst || icmp_inst) {
                    if(fcmp_inst) {
                        fcmp_inst->negation();
                    } else {
                        icmp_inst->negation();
                    }
                } else {
                    if(tmp_val->get_type()->is_float_type()) {
                        Value* lhs = tmp_val;
                        Value* rhs = CONST_FP(0);
                        __GEN_CMP_INSTRUCTION__(lhs, rhs, ==, create_icmp_eq, create_fcmp_eq)
                    } else {
                        Value* lhs = tmp_val;
                        Value* rhs = CONST_INT(0);
                        __GEN_CMP_INSTRUCTION__(lhs, rhs, ==, create_icmp_eq, create_fcmp_eq)
                    }
                }
            }
            break;
        default: 
            break;
    }
}

void sysY_irbuilder::visit(ASTCallee &node) {
    auto fun = static_cast<Function*>(scope.find_func(node.id));
    std::vector<Value*> params;
    int i = 0;
    if(node.id == "starttime" || node.id == "stoptime") {
        params.push_back(CONST_INT(node.lineno));
    } else {
        for(auto &param : node.ParamExp_list) {
            auto param_type = fun->get_function_type()->get_param_type(i++);
            if(param_type->is_integer_type() || param_type->is_float_type()) {
                require_lvalue = false;
            } else {
                require_lvalue = true;
            }
            param->accept(*this);
            require_lvalue = false;
            //! 指针类型
            if (param_type->is_float_type() && tmp_val->get_type()->is_integer_type()) {
                auto const_tmp_val_i = dynamic_cast<ConstantInt *>(tmp_val);
                if (const_tmp_val_i != nullptr) {
                    tmp_val = CONST_FP(float(const_tmp_val_i->get_value()));
                } else {
                    tmp_val = builder->create_sitofp(tmp_val, FLOAT_T);
                }
            } else if (param_type->is_integer_type() && tmp_val->get_type()->is_float_type()) {
                auto const_tmp_val_f = dynamic_cast<ConstantFP *>(tmp_val);
                if (const_tmp_val_f != nullptr) {
                    tmp_val = CONST_INT(int(const_tmp_val_f->get_value()));
                } else {
                    tmp_val = builder->create_fptosi(tmp_val, INT32_T);
                }
            }
            params.push_back(tmp_val);
        }
    }
    tmp_val = builder->create_call(static_cast<Function*>(fun), params);
}

void sysY_irbuilder::visit(ASTMulExp &node) {
  if (node.MulExp == nullptr) {
    node.UnaryExp->accept(*this);
  } else {
    node.MulExp->accept(*this);
    auto lhs = tmp_val;
    node.UnaryExp->accept(*this);
    auto rhs = tmp_val;

    switch(node.op) {
        case MulOp::OP_MUL: 
            __GEN_ALU_INSTRUCTION__(lhs, rhs, *, create_imul, create_fmul)
            break;
        case MulOp::OP_DIV:
            __GEN_ALU_INSTRUCTION__(lhs, rhs, /, create_isdiv, create_fdiv)
            break;
        case MulOp::OP_MOD:
            if(lhs->get_type() == INT32_T && rhs->get_type() == INT32_T) {                       
                auto lhs_const = dynamic_cast<ConstantInt*>(lhs);                              
                auto rhs_const = dynamic_cast<ConstantInt*>(rhs);                              
                if(lhs_const != nullptr && rhs_const != nullptr) {                            
                    tmp_val = CONST_INT(lhs_const->get_value() % rhs_const->get_value());    
                } else {                                                                              
                    tmp_val = builder->create_isrem(lhs, rhs);                                
                }                                                                                  
            } else {
                LOG(ERROR) << "只有整数能进行除余操作";
            }
            break;
        default:
            break;
    }
  }
}

void sysY_irbuilder::visit(ASTAddExp &node) {
    if(node.AddExp == nullptr) {
        node.MulExp->accept(*this);
    } else {
        node.AddExp->accept(*this);
        auto lhs = tmp_val;
        node.MulExp->accept(*this);
        auto rhs = tmp_val;
        switch(node.op) {
            case AddOp::OP_MINUS:
                __GEN_ALU_INSTRUCTION__(lhs, rhs, -, create_isub, create_fsub)
                break;
            case AddOp::OP_PLUS:
                __GEN_ALU_INSTRUCTION__(lhs, rhs, +, create_iadd, create_fadd)
                break;
            default:
                break;
        }
    }
}

void sysY_irbuilder::visit(ASTRelExp &node) {
    if(node.RelExp == nullptr) {
        node.AddExp->accept(*this);
    } else {
        node.RelExp->accept(*this);
        auto lhs = tmp_val;
        node.AddExp->accept(*this);
        auto rhs = tmp_val;
        switch(node.op) {
            case RelOp::OP_LTE:
                __GEN_CMP_INSTRUCTION__(lhs, rhs, <=, create_icmp_le, create_fcmp_le)
                break;
            case RelOp::OP_LT:
                __GEN_CMP_INSTRUCTION__(lhs, rhs, <, create_icmp_lt, create_fcmp_lt)
                break;
            case RelOp::OP_GT:
                __GEN_CMP_INSTRUCTION__(lhs, rhs, >, create_icmp_gt, create_fcmp_gt)
                break;
            case RelOp::OP_GTE:
                __GEN_CMP_INSTRUCTION__(lhs, rhs, >=, create_icmp_ge, create_fcmp_ge)
                break;
            default:
                break;    
        }
    }
}

void sysY_irbuilder::visit(ASTEqExp &node) {
    if(node.EqExp == nullptr) {
        node.RelExp->accept(*this);
    } else {
        node.EqExp->accept(*this);
        auto lhs = tmp_val;
        node.RelExp->accept(*this);
        auto rhs = tmp_val;
        switch(node.op) {
            case RelOp::OP_EQ:
                __GEN_CMP_INSTRUCTION__(lhs, rhs, ==, create_icmp_eq, create_fcmp_eq)
                break;
            case RelOp::OP_NEQ:
                __GEN_CMP_INSTRUCTION__(lhs, rhs, !=, create_icmp_ne, create_fcmp_ne)
                break;
            default:
                break;
        }
    }
}

void sysY_irbuilder::visit(ASTLAndExp &node) {
    if(node.LAndExp == nullptr) {
        node.EqExp->accept(*this);
    } else {
        auto true_BB = BasicBlock::create(module.get(), "", cur_fun);
        node.LAndExp->accept(*this);
        __TO_CMP_INSTRUCTION__(cond_val)
        builder->create_cond_br(cond_val, true_BB, IF_WHILE_Cond_Stack.back().falseBB);
        builder->set_insert_point(true_BB);
        node.EqExp->accept(*this);
    }
}

void sysY_irbuilder::visit(ASTLOrExp &node) {
    if(node.LOrExp == nullptr) {
        node.LAndExp->accept(*this);
    } else {
        auto false_BB = BasicBlock::create(module.get(), "", cur_fun);
        IF_WHILE_Cond_Stack.push_back({IF_WHILE_Cond_Stack.back().trueBB, false_BB});
        node.LOrExp->accept(*this);
        IF_WHILE_Cond_Stack.pop_back();
        __TO_CMP_INSTRUCTION__(cond_val)
        builder->create_cond_br(cond_val, IF_WHILE_Cond_Stack.back().trueBB, false_BB);
        builder->set_insert_point(false_BB);
        node.LAndExp->accept(*this);
    }
}


bool sysY_irbuilder::is_all_zero(ASTInitVal &node) {
    zero_judger.reset();
    zero_judger.visit(node);
    return zero_judger.is_all_zero();
}


