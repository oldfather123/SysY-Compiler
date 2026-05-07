#include <climits>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ir/mod.hpp"
#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {

// 偏移对：基准GEP + 常量偏移
struct OffsetPair {
    std::shared_ptr<ir::Getelementptr> gep_base;
    int offset;

    OffsetPair(std::shared_ptr<ir::Getelementptr> base, int off) : gep_base(base), offset(off) {}
};

// 索引计算：展开的加法操作数
struct IndexCal {
    std::vector<std::shared_ptr<ir::Value>> sum;          // 所有加法操作数
    std::vector<std::shared_ptr<ir::Instruction>> store;  // 可能删除的中间计算指令

    // 标准化：将常量移到末尾
    void standard() {
        std::vector<std::shared_ptr<ir::ConstantInt>> ints;
        auto it = sum.begin();
        while (it != sum.end()) {
            if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(*it)) {
                ints.push_back(const_int);
                it = sum.erase(it);
            } else {
                ++it;
            }
        }
        // 将常量添加到末尾
        for (const auto &int_val : ints) {
            sum.push_back(int_val);
        }
    }
};

// 检查value是否是当前基本块中的ADD指令
static bool contains_and_is_add(const std::shared_ptr<ir::BasicBlock> &block, const std::shared_ptr<ir::Value> &value) {
    auto inst = std::dynamic_pointer_cast<ir::Instruction>(value);
    if (!inst) return false;

    if (inst->get_ins_type() != ir::Instruction::InstructionType::ADD) return false;

    // 检查指令是否在当前基本块中
    auto inst_block = inst->get_parent_block().lock();
    return inst_block && inst_block == block;
}

// 计算两个索引计算之间的常量偏移
static int calculate_offset(const IndexCal &c1, const IndexCal &c2) {
    // 大小差异检查：最多相差1
    auto size1 = static_cast<int>(c1.sum.size());
    auto size2 = static_cast<int>(c2.sum.size());
    int size_diff = size2 - size1;
    if (std::abs(size_diff) > 1) {
        return INT_MAX;  // 表示无法计算偏移
    }

    // 确定比较范围
    int range = 0;
    if (size_diff == 0) {
        // 相同大小：检查最后一个是否都是常量
        if (!c1.sum.empty() && !c2.sum.empty()) {
            auto c1_last = std::dynamic_pointer_cast<ir::ConstantInt>(c1.sum.back());
            auto c2_last = std::dynamic_pointer_cast<ir::ConstantInt>(c2.sum.back());
            if (c1_last && c2_last) {
                range = size1 - 1;
            } else {
                range = size1;
            }
        } else {
            range = size1;
        }
    } else {
        range = std::min(size1, size2);
    }

    // 检查前面的操作数是否相同
    for (int i = 0; i < range; i++) {
        if (c1.sum[i] != c2.sum[i]) {
            return INT_MAX;
        }
    }

    // 计算偏移
    if (size_diff > 0) {
        // c2比c1多一个元素
        if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(c2.sum.back())) {
            return const_int->get_val();
        }
        return INT_MAX;
    } else if (size_diff < 0) {
        // c1比c2多一个元素
        if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(c1.sum.back())) {
            return -const_int->get_val();
        }
        return INT_MAX;
    } else {
        // 相同大小，检查最后的常量差异
        if (!c1.sum.empty() && !c2.sum.empty()) {
            auto c1_last = std::dynamic_pointer_cast<ir::ConstantInt>(c1.sum.back());
            auto c2_last = std::dynamic_pointer_cast<ir::ConstantInt>(c2.sum.back());
            if (c1_last && c2_last) {
                return c2_last->get_val() - c1_last->get_val();
            }
        }
        return INT_MAX;
    }
}

// 在基本块中运行GEP优化
static bool run_on_block(const std::shared_ptr<ir::BasicBlock> &block) {
    bool modified = false;

    // 清理映射表
    std::unordered_map<std::shared_ptr<ir::Getelementptr>, OffsetPair> base_offset_map;
    std::unordered_map<std::shared_ptr<ir::Getelementptr>, IndexCal> index_cal_map;
    // 按“逻辑基 + 前缀索引（去掉最后一维）”分组，确保仅最后一维不同
    std::unordered_map<std::string, std::vector<std::shared_ptr<ir::Getelementptr>>> base_to_geps;

    // 第一遍：收集所有GEP并分析索引计算（与Java实现一致，不做名称过滤）
    for (const auto &inst : block->get_instructions_ref()) {
        if (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(inst)) {
            // 构造“逻辑基 + 前缀索引”的签名
            auto base = gep->base_ptr();
            auto indexes = gep->get_indexes();
            if (indexes.empty()) continue;  // 无最后一维，不处理
            std::stringstream ss;
            ss << "B:" << reinterpret_cast<uintptr_t>(base.get());
            // 仅用前缀索引参与分组（去掉最后一维）
            for (size_t i = 0; i + 1 < indexes.size(); ++i) {
                ss << ":I" << reinterpret_cast<uintptr_t>(indexes[i].get());
            }
            auto key = ss.str();
            base_to_geps[key].push_back(gep);

            // 分析索引计算：仅对最后一个索引展开
            if (!indexes.empty()) {
                auto offset = indexes.back();  // 获取最后一个索引（偏移）

                IndexCal index_cal;
                std::queue<std::shared_ptr<ir::Value>> q;
                q.push(offset);

                // BFS展开加法操作
                while (!q.empty()) {
                    auto value = q.front();
                    q.pop();

                    if (contains_and_is_add(block, value)) {
                        // 是ADD指令，继续展开
                        auto add_inst = std::dynamic_pointer_cast<ir::Instruction>(value);
                        for (const auto &operand : add_inst->get_operands_ref()) {
                            q.push(operand);
                        }

                        // 如果这个ADD只被一个用户使用，可以删除
                        if (add_inst->get_users_ref().size() == 1) {
                            index_cal.store.push_back(add_inst);
                        }
                    } else {
                        // 不是ADD，直接加入sum
                        index_cal.sum.push_back(value);
                    }
                }

                // 标准化
                index_cal.standard();
                index_cal_map[gep] = std::move(index_cal);
            }
        }
    }

    // 第二遍：对相同base的GEP计算偏移
    for (auto &[key, gep_list] : base_to_geps) {
        while (gep_list.size() > 1) {
            auto head = gep_list[0];
            auto next = gep_list[1];

            // 计算偏移
            int offset = calculate_offset(index_cal_map[head], index_cal_map[next]);

            if (offset != INT_MAX) {
                // 可以约减
                base_offset_map.emplace(next, OffsetPair(head, offset));
                gep_list.erase(gep_list.begin() + 1);
            } else {
                gep_list.erase(gep_list.begin());
            }
        }
    }

    // 第三遍：应用优化
    std::unordered_set<std::shared_ptr<ir::Instruction>> to_remove;

    for (const auto &[gep, offset_pair] : base_offset_map) {
        // 检查类型是否匹配
        if (gep->get_type() != offset_pair.gep_base->get_type()) continue;

        // 创建新的GEP：base_gep + offset
        auto offset_const = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), offset_pair.offset);
        // 直接复用原GEP的名字（先替换uses再删除旧GEP，避免重名冲突）
        auto new_gep = ir::Getelementptr::create(nullptr, offset_pair.gep_base, {offset_const}, gep->get_name());

        // 在原GEP前插入新GEP
        new_gep->set_parent_block(block);
        block->add_instruction(gep->node, new_gep);

        // 替换所有使用
        util::replace_all_uses_with(gep, new_gep);

        // 标记删除原GEP
        util::remove_instruction(gep);

        // 标记删除中间计算指令
        const auto &index_cal = index_cal_map[gep];
        for (const auto &inst : index_cal.store) {
            to_remove.insert(inst);
        }

        modified = true;
    }

    // 删除不再需要的指令
    for (const auto &inst : to_remove) {
        util::remove_instruction(inst);
    }

    return modified;
}

bool GepLift::run(ir::Module *module) {
    logger::INFO("Running GepLift...");
    bool modified = false;

    module->for_each_func([&](const auto &func) {
        for (const auto &block : func->get_basic_blocks_ref()) {
            if (run_on_block(block)) {
                modified = true;
            }
        }
    });

    return modified;
}

}  // namespace opt::pass
