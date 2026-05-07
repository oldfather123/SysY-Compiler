#include <cstddef>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/module.hpp"
#include "ir/type.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
#include "util.hpp"

namespace opt::pass {
static bool modified = false;

// ( Alloca | GlobalVariable ) -> ( GEP -> (indexes) )
static std::unordered_map<std::variant<std::shared_ptr<ir::Alloca>, std::shared_ptr<ir::GlobalVariable>>,
                          std::unordered_map<std::shared_ptr<ir::Getelementptr>, std::vector<int>>>
    gep_index_map;

// GEP -> Bitcast (Bitcast is only used by Memset, so it could infer the memsets)
static std::unordered_map<std::shared_ptr<ir::Getelementptr>, std::shared_ptr<ir::Bitcast>> gep_bitcast_map;
static std::unordered_set<std::shared_ptr<ir::Alloca>> alloca_worklist;

static void local_sroa(std::shared_ptr<ir::Function> func);
static void global_sroa(std::shared_ptr<ir::GlobalVariable> global_var, ir::Module *module);

bool ScalarReplacementOfAggregates::run(ir::Module *module) {
    logger::INFO("Running ScalarReplacementOfAggregates...");
    modified = false;
    module->for_each_func([](auto &func) { local_sroa(func); });
    module->for_each_global_variable([&](auto &global_var) { global_sroa(global_var, module); });
    return modified;
}

static std::shared_ptr<ir::ArrayType> get_arr_content_type(
    std::variant<std::shared_ptr<ir::Alloca>, std::shared_ptr<ir::GlobalVariable>> root_ptr) {
    std::shared_ptr<ir::ArrayType> arr_type;
    std::visit(
        overloaded{[&arr_type](std::shared_ptr<ir::Alloca> alloca) {
                       arr_type = std::dynamic_pointer_cast<ir::ArrayType>(alloca->get_content_type());
                   },
                   [&arr_type](std::shared_ptr<ir::GlobalVariable> global_var) {
                       arr_type = std::dynamic_pointer_cast<ir::ArrayType>(
                           std::dynamic_pointer_cast<ir::PointerType>(global_var->get_type())->get_reference_type());
                   }},
        root_ptr);
    return arr_type;
}

static std::shared_ptr<ir::Value> get_root_ptr_value(
    std::variant<std::shared_ptr<ir::Alloca>, std::shared_ptr<ir::GlobalVariable>> root_ptr) {
    std::shared_ptr<ir::Value> root_ptr_value;
    std::visit(
        overloaded{[&root_ptr_value](std::shared_ptr<ir::Alloca> alloca) { root_ptr_value = alloca; },
                   [&root_ptr_value](std::shared_ptr<ir::GlobalVariable> global_var) { root_ptr_value = global_var; }},
        root_ptr);
    return root_ptr_value;
}

static void add_to_gep_map(std::variant<std::shared_ptr<ir::Alloca>, std::shared_ptr<ir::GlobalVariable>> root_ptr,
                           std::shared_ptr<ir::Getelementptr> gep,
                           const std::vector<int> &cur_indexes) {
    auto arr_type = get_arr_content_type(root_ptr);
    auto dims = arr_type->dims();
    if (dims.size() == cur_indexes.size()) {
        gep_index_map[root_ptr][gep] = cur_indexes;
    } else {
        auto new_indexes = cur_indexes;
        while (new_indexes.size() < dims.size()) {
            new_indexes.push_back(0);
        }
        gep_index_map[root_ptr][gep] = new_indexes;
    }
}

static int index2offset(std::shared_ptr<ir::ArrayType> arr_type, const std::vector<int> &indexes) {
    auto dims = arr_type->dims();
    assert(indexes.size() == dims.size());
    int offset = 0;
    int stride = 1;
    for (int i = static_cast<int>(dims.size()) - 1; i >= 0; i--) {
        offset += indexes[i] * stride;
        stride *= dims[i];
    }
    assert(offset < arr_type->get_element_nums());
    return offset;
}

static std::vector<int> offset2index(std::shared_ptr<ir::ArrayType> arr_type, int offset) {
    auto dims = arr_type->dims();
    std::vector<int> indexes(dims.size());
    for (int i = static_cast<int>(dims.size()) - 1; i >= 0; i--) {
        indexes[i] = offset % dims[i];
        offset /= dims[i];
    }
    return indexes;
}

static std::shared_ptr<ir::Constant> get_certain_init(std::shared_ptr<ir::GlobalVariable> global_var, int offset) {
    auto arr_type = get_arr_content_type(global_var);
    auto certain_dims = offset2index(arr_type, offset);
    auto cur_init_val = global_var->get_init_value();
    for (int certain_dim : certain_dims) {
        if (cur_init_val->is_zero()) return ir::get_zero(arr_type->scalar_type());
        if (auto arr_val = std::dynamic_pointer_cast<ir::ConstantArray>(cur_init_val))
            cur_init_val = arr_val->get_vals()[certain_dim];
        return cur_init_val;
    }
    __builtin_unreachable();
}

static bool is_const_alloca_indexes(
    std::variant<std::shared_ptr<ir::Alloca>, std::shared_ptr<ir::GlobalVariable>> root_ptr,
    std::shared_ptr<ir::Getelementptr> gep,
    std::vector<int> cur_indexes) {
    auto base_ptr = gep->base_ptr();
    auto indexes = gep->get_indexes();
    if (base_ptr == get_root_ptr_value(root_ptr)) {
        assert(cur_indexes.empty());
        auto first_index_const_int = std::dynamic_pointer_cast<ir::ConstantInt>(indexes[0]);
        if (!first_index_const_int || first_index_const_int->get_val() != 0) return false;
    } else if (auto base_gep = std::dynamic_pointer_cast<ir::Getelementptr>(base_ptr)) {
        // assert(indexes.size() == 1);
        auto arr_type = get_arr_content_type(root_ptr);
        auto index_const_int = std::dynamic_pointer_cast<ir::ConstantInt>(indexes.front());
        if (!index_const_int) return false;
        cur_indexes[cur_indexes.size() - 1] += index_const_int->get_val();
        assert(cur_indexes.back() >= 0);
        assert(cur_indexes.back() < arr_type->dims()[cur_indexes.size() - 1]);
    } else {
        __builtin_unreachable();
    }
    for (size_t i = 1; i < indexes.size(); i++) {
        auto const_index = std::dynamic_pointer_cast<ir::ConstantInt>(indexes[i]);
        if (!const_index) return false;
        cur_indexes.push_back(const_index->get_val());
    }
    add_to_gep_map(root_ptr, gep, cur_indexes);
    for (auto &weak_user : gep->get_users_ref()) {
        auto user = weak_user.lock();
        if (auto user_gep = std::dynamic_pointer_cast<ir::Getelementptr>(user)) {
            if (!is_const_alloca_indexes(root_ptr, user_gep, cur_indexes)) return false;
        } else if (auto user_bitcast = std::dynamic_pointer_cast<ir::Bitcast>(user)) {
            assert(!gep_bitcast_map.count(gep));
            gep_bitcast_map[gep] = user_bitcast;
        } else if (std::dynamic_pointer_cast<ir::Call>(user)) {
            return false;
        }
    }
    return true;
}

static void local_sroa(std::shared_ptr<ir::Function> func) {
    for (const auto &block : func->get_basic_blocks_ref()) {
        for (const auto &inst : block->get_instructions_ref()) {
            if (auto alloca = std::dynamic_pointer_cast<ir::Alloca>(inst)) {
                auto arr_type = std::dynamic_pointer_cast<ir::ArrayType>(alloca->get_content_type());
                if (!arr_type) continue;
                bool flag = true;
                for (auto &weak_user : alloca->get_users_ref()) {
                    auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(weak_user.lock());
                    if (!gep) {
                        // user is not a gep, but rather phi, skip
                        flag = false;
                        break;
                    }
                    if (!is_const_alloca_indexes(alloca, gep, {})) {
                        flag = false;
                        break;
                    }
                }
                if (flag) alloca_worklist.insert(alloca);
            }
        }
    }

    modified |= !alloca_worklist.empty();

    for (const auto &alloca : alloca_worklist) {
        auto arr_type = std::dynamic_pointer_cast<ir::ArrayType>(alloca->get_content_type());
        auto block = alloca->get_parent_block().lock();
        std::vector<std::shared_ptr<ir::Alloca>> alloca_map(arr_type->get_element_nums());
        for (int i = 0; i < arr_type->get_element_nums(); i++) {
            auto new_alloca = ir::Alloca::create(block, arr_type->scalar_type(), gen_local_var_name());
            alloca_map[i] = new_alloca;
            block->add_instruction(alloca->node, new_alloca);
        }
        for (const auto &[gep, indexes] : gep_index_map[alloca]) {
            if (gep_bitcast_map.count(gep)) {
                auto bitcast = gep_bitcast_map[gep];
                assert(bitcast->get_users_ref().size() == 1);
                auto memset = std::dynamic_pointer_cast<ir::Memset>(bitcast->get_users_ref().front().lock());
                auto base_offset = index2offset(arr_type, gep_index_map[alloca][gep]);
                auto scalar_type = arr_type->scalar_type();
                auto element_len = memset->get_size() * 8 / scalar_type->bits_num();
                auto memset_block = memset->get_parent_block().lock();
                for (int i = base_offset; i < base_offset + element_len; i++) {
                    auto store =
                        ir::Store::create(memset_block, ir::get_zero(scalar_type), alloca_map[i], gen_local_var_name());
                    memset_block->add_instruction(memset->node, store);
                }
                util::remove_instruction(memset);
                util::remove_instruction(bitcast);
            }

            auto offset = index2offset(arr_type, indexes);
            util::substitute(gep, alloca_map[offset]);
        }

        assert(alloca->get_users_ref().empty());
        util::remove_instruction(alloca);
    }

    gep_index_map.clear();
    gep_bitcast_map.clear();
    alloca_worklist.clear();
}

static void global_sroa(std::shared_ptr<ir::GlobalVariable> global_var, ir::Module *module) {
    auto arr_type = std::dynamic_pointer_cast<ir::ArrayType>(global_var->get_type());
    bool valid = true;
    if (!arr_type) valid = false;
    for (auto &weak_user : global_var->get_users_ref()) {
        auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(weak_user.lock());
        if (!gep) {
            valid = false;
            break;
        }
        if (!is_const_alloca_indexes(global_var, gep, {})) {
            valid = false;
            break;
        }
    }
    if (!valid) {
        gep_index_map.clear();
        return;
    }
    assert(gep_bitcast_map.empty());
    modified = true;

    std::vector<std::shared_ptr<ir::GlobalVariable>> global_var_map(arr_type->get_element_nums());
    for (int i = 0; i < arr_type->get_element_nums(); i++) {
        auto new_global_var = std::make_shared<ir::GlobalVariable>(
            arr_type->scalar_type(), get_certain_init(global_var, i), false, gen_global_var_name());
        global_var_map[i] = new_global_var;
        module->add_global_variables({new_global_var});
    }

    for (const auto &[gep, indexes] : gep_index_map[global_var]) {
        auto offset = index2offset(arr_type, indexes);
        util::substitute(gep, global_var_map[offset]);
    }

    assert(global_var->get_users_ref().empty());
    util::remove_global_variable(module, global_var);
    gep_index_map.clear();
}
}  // namespace opt::pass
