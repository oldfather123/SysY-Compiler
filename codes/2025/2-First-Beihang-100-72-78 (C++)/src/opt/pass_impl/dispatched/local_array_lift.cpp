#include <algorithm>
#include <memory>
#include <unordered_map>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/type.hpp"
#include "ir/value.hpp"
#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {

struct ArrayInitInfo {
    std::unordered_map<int, std::shared_ptr<ir::Constant>> init_values;
    std::vector<std::shared_ptr<ir::Instruction>> stores_to_delete;
    std::shared_ptr<ir::Alloca> alloca;
};

static std::shared_ptr<ir::Value> get_base_addr(std::shared_ptr<ir::Value> value);
static bool collect_array_info(std::shared_ptr<ir::Alloca> alloca, ArrayInitInfo &info);
static bool collect_once_array_info(std::shared_ptr<ir::Alloca> alloca, ArrayInitInfo &info);
static void lift_arrays(ir::Module *module, std::vector<ArrayInitInfo> &arrays_to_lift);
static bool is_dominated_by_all_stores(const std::vector<std::shared_ptr<ir::Instruction>> &stores,
                                       const std::vector<std::shared_ptr<ir::Instruction>> &loads);
static int calculate_linear_index(const std::vector<std::shared_ptr<ir::Value>> &indexes,
                                  std::shared_ptr<ir::ArrayType> array_type);
static std::shared_ptr<ir::Constant> create_nested_constant_array(
    std::shared_ptr<ir::ArrayType> array_type, const std::unordered_map<int, std::shared_ptr<ir::Constant>> &values);

static int lift_counter = 0;

bool LocalArrayLift::run(ir::Module *module) {
    logger::INFO("Running LocalArrayLift...");

    bool modified = false;

    for (auto &func : module->get_functions_ref()) {
        if (ir::Function::is_lib(func)) continue;

        std::vector<ArrayInitInfo> arrays_to_lift;

        // 遍历所有基本块，寻找可提升的数组
        for (auto &block : func->get_basic_blocks_ref()) {
            for (const auto &inst : block->get_instructions()) {
                auto alloca = std::dynamic_pointer_cast<ir::Alloca>(inst);
                if (!alloca) continue;

                // 检查是否为数组类型
                auto ptr_type = std::dynamic_pointer_cast<ir::PointerType>(alloca->get_type());
                if (!ptr_type) continue;
                auto array_type = std::dynamic_pointer_cast<ir::ArrayType>(ptr_type->get_reference_type());
                if (!array_type) continue;

                ArrayInitInfo info;
                info.alloca = alloca;

                // 尝试收集数组信息（全局分析）

                if (collect_array_info(alloca, info)) {
                    arrays_to_lift.push_back(info);
                    modified = true;
                }
            }
        }

        // 对于 main 函数，进行第二轮一次性数组提升（类似 Java 的 FindOnceLiftArray）
        if (func->is_main()) {
            for (auto &block : func->get_basic_blocks_ref()) {
                for (const auto &inst : block->get_instructions()) {
                    auto alloca = std::dynamic_pointer_cast<ir::Alloca>(inst);
                    if (!alloca) continue;

                    // 检查是否为数组类型
                    auto ptr_type = std::dynamic_pointer_cast<ir::PointerType>(alloca->get_type());
                    if (!ptr_type) continue;
                    auto array_type = std::dynamic_pointer_cast<ir::ArrayType>(ptr_type->get_reference_type());
                    if (!array_type) continue;

                    // 检查是否已经被处理过
                    bool already_processed = false;
                    for (const auto &processed_info : arrays_to_lift) {
                        if (processed_info.alloca == alloca) {
                            already_processed = true;
                            break;
                        }
                    }
                    if (already_processed) continue;

                    ArrayInitInfo info;
                    info.alloca = alloca;

                    // 尝试一次性数组提升

                    if (collect_once_array_info(alloca, info)) {
                        arrays_to_lift.push_back(info);
                        modified = true;
                    }
                }
            }
        }

        // 执行数组提升
        if (!arrays_to_lift.empty()) {
            lift_arrays(module, arrays_to_lift);
        }
    }

    logger::INFO("endding LocalArrayLift...");

    return modified;
}

// 获取指针的基址
static std::shared_ptr<ir::Value> get_base_addr(std::shared_ptr<ir::Value> value) {
    auto current = value;
    while (true) {
        if (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(current)) {
            current = gep->base_ptr();
        } else if (auto bitcast = std::dynamic_pointer_cast<ir::Bitcast>(current)) {
            current = bitcast->val();
        } else {
            break;
        }
    }
    return current;
}

// 收集数组信息（全局分析版本）
static bool collect_array_info(std::shared_ptr<ir::Alloca> alloca, ArrayInitInfo &info) {
    std::vector<std::shared_ptr<ir::Instruction>> stores;
    std::vector<std::shared_ptr<ir::Instruction>> loads;
    std::vector<std::shared_ptr<ir::User>> to_visit;

    // 收集所有使用
    for (auto &weak_user : alloca->get_users_ref()) {
        if (auto user = weak_user.lock()) {
            to_visit.push_back(user);
        }
    }

    while (!to_visit.empty()) {
        auto user = to_visit.back();
        to_visit.pop_back();

        auto inst = std::dynamic_pointer_cast<ir::Instruction>(user);
        if (!inst) continue;

        if (auto load = std::dynamic_pointer_cast<ir::Load>(inst)) {
            loads.push_back(load);
        } else if (auto store = std::dynamic_pointer_cast<ir::Store>(inst)) {
            // 检查是否通过 GEP 访问
            auto addr = store->addr();
            if (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(addr)) {
                auto indexes = gep->get_indexes();
                if (indexes.size() >= 2) {
                    // 检查所有索引是否都是常量
                    bool all_const = true;
                    for (size_t i = 1; i < indexes.size(); i++) {
                        if (!std::dynamic_pointer_cast<ir::Constant>(indexes[i])) {
                            all_const = false;
                            break;
                        }
                    }
                    auto val = store->val();
                    // 必须是常量索引和常量值
                    if (all_const && std::dynamic_pointer_cast<ir::Constant>(val)) {
                        stores.push_back(store);
                    } else {
                        return false;  // 非常量访问，无法提升
                    }
                } else {
                    return false;
                }
            } else {
                return false;  // 直接存储到数组基址，无法处理
            }
        } else if (auto memset = std::dynamic_pointer_cast<ir::Memset>(inst)) {
            // 处理 Memset 指令
            auto operands = memset->get_operands();
            if (!operands.empty()) {
                auto base = get_base_addr(operands[0]);

                if (base == alloca) {
                    stores.push_back(memset);
                    // 同时需要将中间的指令链也标记为删除
                    // 追踪从 alloca 到 memset 参数的指令链
                    auto current = operands[0];
                    while (current != alloca) {
                        if (auto current_inst = std::dynamic_pointer_cast<ir::Instruction>(current)) {
                            stores.push_back(current_inst);
                            auto current_operands = current_inst->get_operands();
                            if (!current_operands.empty()) {
                                current = current_operands[0];
                            } else {
                                break;
                            }
                        } else {
                            break;
                        }
                    }
                }
            }
        } else if (auto call = std::dynamic_pointer_cast<ir::Call>(inst)) {
            auto func = call->get_function();
            if (func->get_name() == "@llvm.memset.p0.i32") {
                auto args = call->args();
                if (!args.empty()) {
                    auto base = get_base_addr(args[0]);

                    if (base == alloca) {
                        stores.push_back(call);
                    }
                }
            } else if (!ir::Function::is_lib(func)) {
                // 非库函数调用，可能有副作用
                if (func->opt_info.has_side_effect) {
                    return false;
                }
            }
        } else if (std::dynamic_pointer_cast<ir::Getelementptr>(inst) || std::dynamic_pointer_cast<ir::Bitcast>(inst)) {
            // 继续追踪 GEP 和 BitCast 的使用
            for (auto &weak_next_user : inst->get_users_ref()) {
                if (auto next_user = weak_next_user.lock()) {
                    to_visit.push_back(next_user);
                }
            }
        } else {
            return false;  // 其他类型的使用，无法处理
        }
    }

    // 如果没有 store 操作，但有 memset，也可以提升（全零数组）

    if (stores.empty()) {
        return false;
    }

    // 检查所有 store 是否在支配树的一条根向路径上
    std::shared_ptr<ir::Instruction> last_store = nullptr;
    for (const auto &store : stores) {
        if (!last_store) {
            last_store = store;
            continue;
        }

        auto store_bb = store->get_parent_block().lock();
        auto last_store_bb = last_store->get_parent_block().lock();

        if (store_bb->opt_info.dominates(last_store_bb)) {
            last_store = store;
        } else if (!last_store_bb->opt_info.dominates(store_bb)) {
            return false;  // 不在同一条支配路径上
        }
    }

    // 检查所有 load 是否被最后一个 store 支配
    if (!is_dominated_by_all_stores(stores, loads)) {
        return false;
    }

    // 收集初始化值
    for (const auto &store : stores) {
        if (auto store_inst = std::dynamic_pointer_cast<ir::Store>(store)) {
            auto addr = store_inst->addr();
            if (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(addr)) {
                auto indexes = gep->get_indexes();
                if (indexes.size() >= 2) {
                    auto ptr_type = std::dynamic_pointer_cast<ir::PointerType>(alloca->get_type());
                    auto array_type = std::dynamic_pointer_cast<ir::ArrayType>(ptr_type->get_reference_type());
                    int linear_idx = calculate_linear_index(indexes, array_type);
                    if (linear_idx >= 0) {
                        auto val = std::dynamic_pointer_cast<ir::Constant>(store_inst->val());
                        if (val && info.init_values.find(linear_idx) == info.init_values.end()) {
                            info.init_values[linear_idx] = val;
                        }
                    }
                }
            }
        }
        // 注意：memset 调用不需要在这里处理，因为它表示全零初始化
        // 对于只有 memset 的数组，info.init_values 为空，这是正确的（将生成全零数组）
        info.stores_to_delete.push_back(store);
    }

    return true;
}

// 收集一次性数组信息（仅在当前基本块内）
static bool collect_once_array_info(std::shared_ptr<ir::Alloca> alloca, ArrayInitInfo &info) {
    auto block = alloca->get_parent_block().lock();
    if (!block) return false;

    // 检查是否在循环中
    if (auto loop = block->opt_info.loop.lock()) {
        if (loop->depth != 0) return false;
    }

    auto instructions = block->get_instructions();
    auto alloca_it = std::find(instructions.begin(), instructions.end(), alloca);
    if (alloca_it == instructions.end()) return false;

    // 从 alloca 之后开始扫描
    for (auto it = std::next(alloca_it); it != instructions.end(); ++it) {
        auto inst = *it;

        if (auto load = std::dynamic_pointer_cast<ir::Load>(inst)) {
            if (get_base_addr(load->addr()) == alloca) {
                break;  // 遇到读取，停止收集
            }
        } else if (auto store = std::dynamic_pointer_cast<ir::Store>(inst)) {
            auto addr = store->addr();
            if (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(addr)) {
                if (get_base_addr(gep) == alloca) {
                    auto indexes = gep->get_indexes();
                    if (indexes.size() >= 2) {
                        // 检查所有索引是否都是常量
                        bool all_const = true;
                        for (size_t i = 1; i < indexes.size(); i++) {
                            if (!std::dynamic_pointer_cast<ir::Constant>(indexes[i])) {
                                all_const = false;
                                break;
                            }
                        }
                        auto val_const = std::dynamic_pointer_cast<ir::Constant>(store->val());
                        if (all_const && val_const) {
                            auto ptr_type = std::dynamic_pointer_cast<ir::PointerType>(alloca->get_type());
                            auto array_type = std::dynamic_pointer_cast<ir::ArrayType>(ptr_type->get_reference_type());
                            int linear_idx = calculate_linear_index(indexes, array_type);
                            if (linear_idx >= 0) {
                                info.init_values[linear_idx] = val_const;
                                info.stores_to_delete.push_back(store);
                            }
                        } else {
                            break;  // 非常量，停止收集
                        }
                    }
                }
            }
        } else if (auto memset = std::dynamic_pointer_cast<ir::Memset>(inst)) {
            // 处理 Memset 指令
            auto operands = memset->get_operands();
            if (!operands.empty()) {
                auto base = get_base_addr(operands[0]);
                if (base == alloca) {
                    info.stores_to_delete.push_back(memset);
                    // 同时需要将中间的指令链也标记为删除
                    // 追踪从 alloca 到 memset 参数的指令链
                    auto current = operands[0];
                    while (current != alloca) {
                        if (auto current_inst = std::dynamic_pointer_cast<ir::Instruction>(current)) {
                            info.stores_to_delete.push_back(current_inst);
                            auto current_operands = current_inst->get_operands();
                            if (!current_operands.empty()) {
                                current = current_operands[0];
                            } else {
                                break;
                            }
                        } else {
                            break;
                        }
                    }
                }
            }
        } else if (auto call = std::dynamic_pointer_cast<ir::Call>(inst)) {
            auto func = call->get_function();
            if (func->get_name() == "@llvm.memset.p0.i32") {
                auto args = call->args();
                if (!args.empty() && get_base_addr(args[0]) == alloca) {
                    info.stores_to_delete.push_back(call);
                    // 同时需要将中间的指令链也标记为删除
                    // 追踪从 alloca 到 memset 参数的指令链
                    auto current = args[0];
                    while (current != alloca) {
                        if (auto current_inst = std::dynamic_pointer_cast<ir::Instruction>(current)) {
                            info.stores_to_delete.push_back(current_inst);
                            auto current_operands = current_inst->get_operands();
                            if (!current_operands.empty()) {
                                current = current_operands[0];
                            } else {
                                break;
                            }
                        } else {
                            break;
                        }
                    }
                }
            } else {
                break;  // 其他函数调用，停止收集
            }
        }
    }

    return !info.init_values.empty() || !info.stores_to_delete.empty();
}

// 检查所有 load 是否被 store 支配
static bool is_dominated_by_all_stores(const std::vector<std::shared_ptr<ir::Instruction>> &stores,
                                       const std::vector<std::shared_ptr<ir::Instruction>> &loads) {
    if (stores.empty()) return true;

    // 找到最后一个 store（在支配树中最深的）
    std::shared_ptr<ir::Instruction> last_store = stores[0];
    for (size_t i = 1; i < stores.size(); i++) {
        auto store_bb = stores[i]->get_parent_block().lock();
        auto last_store_bb = last_store->get_parent_block().lock();
        if (store_bb->opt_info.dominates(last_store_bb)) {
            last_store = stores[i];
        }
    }

    // 检查所有 load 是否被最后一个 store 支配
    auto last_store_bb = last_store->get_parent_block().lock();
    for (const auto &load : loads) {
        auto load_bb = load->get_parent_block().lock();
        if (!last_store_bb->opt_info.dominates(load_bb)) {
            return false;
        }
    }

    return true;
}

// 执行数组提升
static void lift_arrays(ir::Module *module, std::vector<ArrayInitInfo> &arrays_to_lift) {
    for (auto &info : arrays_to_lift) {
        auto alloca = info.alloca;
        auto ptr_type = std::dynamic_pointer_cast<ir::PointerType>(alloca->get_type());
        auto array_type = std::dynamic_pointer_cast<ir::ArrayType>(ptr_type->get_reference_type());

        // 创建常量数组（支持多维）
        std::shared_ptr<ir::Constant> init_value = create_nested_constant_array(array_type, info.init_values);

        // 创建全局变量
        std::string gv_name = "@_lift_array_" + std::to_string(lift_counter++);
        auto global_var =
            std::make_shared<ir::GlobalVariable>(ir::PointerType::get(array_type), init_value, false, gv_name);

        module->get_global_variables_ref().push_back(global_var);

        // 替换所有使用
        opt::util::replace_all_uses_with(alloca, global_var);

        // 删除相关指令
        for (const auto &inst : info.stores_to_delete) {
            opt::util::remove_instruction(inst);
        }
        opt::util::remove_instruction(alloca);
    }
}

// 计算多维数组的线性索引
static int calculate_linear_index(const std::vector<std::shared_ptr<ir::Value>> &indexes,
                                  std::shared_ptr<ir::ArrayType> array_type) {
    if (indexes.size() < 2) return -1;

    // 第一个索引 (indexes[0]) 通常是 0，表示数组基址
    // 从 indexes[1] 开始是实际的数组索引
    std::vector<int> dims;
    auto current_type = array_type;

    // 收集所有维度大小
    while (current_type) {
        dims.push_back(current_type->get_size());
        auto element_type = current_type->get_element_type();
        current_type = std::dynamic_pointer_cast<ir::ArrayType>(element_type);
    }

    // 检查索引数量是否匹配
    if (indexes.size() - 1 != dims.size()) return -1;

    // 计算线性索引
    int linear_idx = 0;
    int multiplier = 1;

    // 从最后一个维度开始计算
    for (int i = static_cast<int>(dims.size()) - 1; i >= 0; i--) {
        auto idx_const = std::dynamic_pointer_cast<ir::ConstantInt>(indexes[i + 1]);
        if (!idx_const) return -1;

        int idx_val = idx_const->get_val();
        if (idx_val < 0 || idx_val >= dims[i]) return -1;

        linear_idx += idx_val * multiplier;
        multiplier *= dims[i];
    }

    return linear_idx;
}

// 创建嵌套的常量数组
static std::shared_ptr<ir::Constant> create_nested_constant_array(
    std::shared_ptr<ir::ArrayType> array_type, const std::unordered_map<int, std::shared_ptr<ir::Constant>> &values) {
    auto element_type = array_type->get_element_type();
    int array_size = array_type->get_size();

    // 如果元素类型也是数组，递归处理
    if (auto sub_array_type = std::dynamic_pointer_cast<ir::ArrayType>(element_type)) {
        std::vector<std::shared_ptr<ir::Constant>> elements;
        elements.reserve(array_size);

        // auto scalar_type = sub_array_type->scalar_type();

        for (int i = 0; i < array_size; i++) {
            // 为每个子数组收集对应的值
            std::unordered_map<int, std::shared_ptr<ir::Constant>> sub_values;

            // 计算子数组的线性大小
            int sub_linear_size = 1;
            auto temp_type = sub_array_type;
            while (temp_type) {
                sub_linear_size *= temp_type->get_size();
                temp_type = std::dynamic_pointer_cast<ir::ArrayType>(temp_type->get_element_type());
            }

            for (int j = 0; j < sub_linear_size; j++) {
                int global_idx = i * sub_linear_size + j;
                if (values.find(global_idx) != values.end()) {
                    sub_values[j] = values.at(global_idx);
                }
            }

            elements.push_back(create_nested_constant_array(sub_array_type, sub_values));
        }

        // 检查是否全为零
        bool all_zero = true;
        for (const auto &elem : elements) {
            if (!elem->is_zero()) {
                all_zero = false;
                break;
            }
        }

        if (all_zero) {
            return std::make_shared<ir::ZeroInitializer>(array_type);
        } else {
            return std::make_shared<ir::ConstantArray>(array_type, elements);
        }
    } else {
        // 基本类型数组
        std::vector<std::shared_ptr<ir::Constant>> elements;
        elements.reserve(array_size);

        for (int i = 0; i < array_size; i++) {
            if (values.find(i) != values.end()) {
                elements.push_back(values.at(i));
            } else {
                elements.push_back(ir::get_zero(element_type));
            }
        }

        // 检查是否全为零
        bool all_zero = true;
        for (const auto &elem : elements) {
            if (!elem->is_zero()) {
                all_zero = false;
                break;
            }
        }

        if (all_zero) {
            return std::make_shared<ir::ZeroInitializer>(array_type);
        } else {
            return std::make_shared<ir::ConstantArray>(array_type, elements);
        }
    }
}

}  // namespace opt::pass
