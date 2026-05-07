#include <memory>
#include <vector>

#include "ir/mod.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {

static void trans_load_to_value(std::shared_ptr<ir::GlobalVariable> gv);

bool ConstIdx2Value::run(ir::Module *module) {
    // 遍历所有全局变量
    for (const auto &gv : module->get_global_variables()) {
        trans_load_to_value(gv);
    }
    return false;
}

static void trans_load_to_value(std::shared_ptr<ir::GlobalVariable> gv) {
    // 只处理数组类型的全局变量
    auto gv_type = gv->get_type();
    auto ptr_type = std::dynamic_pointer_cast<ir::PointerType>(gv_type);
    if (!ptr_type) return;

    auto array_type = std::dynamic_pointer_cast<ir::ArrayType>(ptr_type->get_reference_type());
    if (!array_type) return;

    // 收集所有使用指令并检查是否可以优化
    std::vector<std::shared_ptr<ir::Instruction>> load_list;
    std::vector<std::shared_ptr<ir::User>> use_inst;

    // 初始化使用指令列表
    for (auto &weak_user : gv->get_users_ref()) {
        if (auto user = weak_user.lock()) {
            use_inst.push_back(user);
        }
    }

    // 分析使用模式
    while (!use_inst.empty()) {
        auto user = use_inst.back();
        use_inst.pop_back();

        auto inst = std::dynamic_pointer_cast<ir::Instruction>(user);
        if (!inst) continue;

        if (auto store = std::dynamic_pointer_cast<ir::Store>(inst)) {
            // 有store操作，不能优化
            return;
        } else if (auto call = std::dynamic_pointer_cast<ir::Call>(inst)) {
            // 有call操作，不能优化
            return;
        } else if (auto load = std::dynamic_pointer_cast<ir::Load>(inst)) {
            // 收集load指令
            load_list.push_back(load);
        } else if (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(inst)) {
            // 继续追踪getelementptr的使用
            for (auto &weak_next_user : gep->get_users_ref()) {
                if (auto next_user = weak_next_user.lock()) {
                    use_inst.push_back(next_user);
                }
            }
        } else if (auto bitcast = std::dynamic_pointer_cast<ir::Bitcast>(inst)) {
            // 继续追踪bitcast的使用
            for (auto &weak_next_user : bitcast->get_users_ref()) {
                if (auto next_user = weak_next_user.lock()) {
                    use_inst.push_back(next_user);
                }
            }
        } else {
            // 其他未处理的指令类型，为安全起见不优化
            return;
        }
    }

    // 替换常量索引的load为常量值
    std::vector<std::shared_ptr<ir::Load>> del_list;

    for (auto &inst : load_list) {
        auto load_inst = std::dynamic_pointer_cast<ir::Load>(inst);
        if (!load_inst) continue;

        auto addr = load_inst->addr();
        auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(addr);
        if (!gep) continue;

        // 检查GEP是否指向我们的全局变量
        if (gep->base_ptr() != gv && !std::dynamic_pointer_cast<ir::Getelementptr>(gep->base_ptr())) {
            continue;
        }

        // 处理零初始化数组
        if (auto zero_init = std::dynamic_pointer_cast<ir::ZeroInitializer>(gv->get_init_value())) {
            // 获取基本元素类型
            auto basic_type = array_type;
            while (auto nested_array = std::dynamic_pointer_cast<ir::ArrayType>(basic_type->get_element_type())) {
                basic_type = nested_array;
            }
            auto element_type = basic_type->get_element_type();

            std::shared_ptr<ir::Constant> const_value;
            if (auto int_type = std::dynamic_pointer_cast<ir::IntegerType>(element_type)) {
                const_value = std::make_shared<ir::ConstantInt>(int_type, 0);
            } else if (auto float_type = std::dynamic_pointer_cast<ir::FloatType>(element_type)) {
                const_value = std::make_shared<ir::ConstantFloat>(float_type, 0.0F);
            } else {
                continue;  // 不支持的类型
            }

            // 替换所有使用
            util::replace_all_uses_with(load_inst, const_value);
            del_list.push_back(load_inst);
        }
        // 处理常量数组
        else if (auto const_array = std::dynamic_pointer_cast<ir::ConstantArray>(gv->get_init_value())) {
            auto indexes = gep->get_indexes();
            if (indexes.size() >= 2) {
                // 检查所有索引是否都是常量
                bool all_const = true;
                for (size_t i = 1; i < indexes.size(); i++) {  // 跳过第一个索引(通常是0)
                    if (!std::dynamic_pointer_cast<ir::Constant>(indexes[i])) {
                        all_const = false;
                        break;
                    }
                }

                if (all_const && indexes.size() == 2) {
                    // 对于一维数组访问
                    auto idx_const = std::dynamic_pointer_cast<ir::ConstantInt>(indexes[1]);
                    if (idx_const) {
                        int idx = idx_const->get_val();
                        auto elements = const_array->get_vals();
                        if (idx >= 0 && idx < static_cast<int>(elements.size())) {
                            auto element_value = elements[idx];
                            util::replace_all_uses_with(load_inst, element_value);
                            del_list.push_back(load_inst);
                        }
                    }
                }
                // 对于多维数组，需要更复杂的索引计算
                // 这里可以根据需要扩展
            }
        }
    }

    // 删除被替换的load指令
    for (auto &load : del_list) {
        util::remove_instruction(load);
    }
}

}  // namespace opt::pass
