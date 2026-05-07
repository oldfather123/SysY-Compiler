#include "rv_function.hpp"

#include <iostream>
#include <utility>

#include "log.hpp"
#include "rv_basic_block.hpp"
#include "rv_instruction.hpp"
#include "rv_operand.hpp"

namespace backend::riscv {

RVFunction::Ptr RVFunction::create(std::string name) { return new RVFunction(std::move(name)); }

RVFunction::RVFunction(std::string name) : name(std::move(name)) {}

RVBasicBlock *RVFunction::new_block(const std::string &name) {
    auto *block = new RVBasicBlock(name);
    block->set_function(this);
    blocks.push_back(block);
    return block;
}

std::list<RVBasicBlock *>::iterator RVFunction::remove_block(RVBasicBlock *block) {
    block->clear_preds();
    block->clear_succs();
    auto it = std::find(blocks.begin(), blocks.end(), block);
    if (it != blocks.end()) {
        // 在删除块之前，先删除每条指令
        auto &insts = block->get_insts();
        while (!insts.empty()) {
            auto *inst = insts.front();
            inst->delete_self();
        }
        return blocks.erase(it);
    }
    return blocks.end();
}

void RVFunction::remove_empty_blocks() {
    // 使用remove_if和lambda表达式删除所有空块
    blocks.remove_if([](RVBasicBlock *block) { return block->get_insts().empty(); });
}

std::string RVFunction::to_string() const {
    // 避免输出空的库函数导致错误
    if (blocks.empty()) return "";
    std::string ret;

    // Remove @ symbol from function name for assembly output
    std::string asm_name = name;
    if (!asm_name.empty() && asm_name[0] == '@') {
        asm_name = asm_name.substr(1);
    }

    ret += asm_name + ":\n";

    for (const auto &block : blocks) {
        ret += block->to_string();
    }

    return ret;
}

void RVFunction::alloc_stack(ir::Alloca *alloca) {
    if (has_alloca_inst_offset(alloca)) {
        return;
    }
    auto content_ty = alloca->get_content_type();
    int size = 0;

    // 根据内容类型计算大小
    if (content_ty->is_array_ty()) {
        auto arr_ty = std::dynamic_pointer_cast<ir::ArrayType>(content_ty);
        size = arr_ty->bits_num() / 8;
    } else if (content_ty->is_pointer_ty() || content_ty->is_integer_ty() && content_ty->bits_num() == 64) {
        size = 8;
    } else {
        size = 4;
    }

    size = size - 1 - (size - 1) % 8 + 8;  // 8字节对齐
    stack_size += size;
    alloca_inst_offset_map[alloca] = stack_size;
}

int RVFunction::get_alloca_inst_offset(ir::Alloca *alloca) {
    auto it = alloca_inst_offset_map.find(alloca);
    if (it == alloca_inst_offset_map.end()) {
        assert(false);
    }
    return it->second;
}

bool RVFunction::has_alloca_inst_offset(ir::Alloca *alloca) {
    return alloca_inst_offset_map.find(alloca) != alloca_inst_offset_map.end();
}

void RVFunction::add_virtual_register(RVVirReg *vir_reg) { all_vir_reg_used.insert(vir_reg); }

void RVFunction::liveness_analysis() {
    // logger::INFO(name + "::Liveness analysis");
    // 1. 初始化 live_use/live_def
    for (const auto &block : blocks) {
        auto &info = block->get_live_info();
        info.live_use.clear();
        info.live_def.clear();
        info.live_in.clear();
        info.live_out.clear();
        for (const auto &inst : block->get_insts()) {
            for (auto *use : inst->get_uses()) {
                if (info.live_def.find(use) == info.live_def.end()) {
                    info.live_use.insert(use);
                }
            }
            if (auto *def = inst->get_def()) {
                info.live_def.insert(def);
            }
        }
        for (auto *reg : info.live_use) {
            info.live_in.insert(reg);
        }
    }
    // 2. 迭代计算 live_in/live_out
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
            auto &block = *it;
            // logger::INFO(block->get_name() + "::++it");
            auto &old_info = block->get_live_info();
            std::set<RVReg *> new_live_out;
            for (const auto &succ : block->get_succs()) {
                // logger::INFO(succ->get_name() + "::block->get_succs()");
                for (const auto &reg : succ->get_live_info().live_in) {
                    new_live_out.insert(reg);
                }
            }

            std::set<RVReg *> new_live_in;
            for (auto *reg : new_live_out) {
                new_live_in.insert(reg);
            }
            for (const auto &reg : old_info.live_def) {
                new_live_in.erase(reg);
            }
            for (const auto &reg : old_info.live_use) {
                new_live_in.insert(reg);
            }
            if (old_info.live_out != new_live_out) {
                // // 打印 old_info.live_out
                // std::cout << "[DEBUG] " << block->get_name() << " old live_out: {";
                // for (auto *reg : old_info.live_out) {
                //     std::cout << reg->to_string() << ", "; // 或者 reg->id, 视你的实现而定
                // }
                // std::cout << "}" << std::endl;
                //
                // // 打印 new_live_out
                // std::cout << "[DEBUG] " << block->get_name() << " new live_out: {";
                // for (auto *reg : new_live_out) {
                //     std::cout << reg->to_string() << ", ";
                // }
                // std::cout << "}" << std::endl;

                old_info.live_out = new_live_out;
                changed = true;
            }
            old_info.live_in.clear();
            old_info.live_in = new_live_in;
        }
    }
}

void RVFunction::reorder_blocks(const std::vector<int> &new_order, const std::vector<RVBasicBlock *> &original_blocks) {
    // 清空当前的基本块列表
    blocks.clear();

    // 按照新的顺序重新添加基本块
    for (int index : new_order) {
        if (index >= 0 && index < static_cast<int>(original_blocks.size())) {
            auto *block = original_blocks[index];
            if (block) {
                blocks.push_back(block);
            }
        }
    }
}

void RVFunction::recalculate_cfg() {
    // 清空所有基本块的前驱和后继关系
    for (auto *block : blocks) {
        block->clear_preds();
        block->clear_succs();
    }

    // 遍历每个基本块，重新建立控制流图
    for (auto *block : blocks) {
        // 遍历基本块中的每条指令
        for (auto *inst : block->get_insts()) {
            // 如果是分支指令
            if (inst->is_branch_ins()) {
                // 获取分支指令的目标标签（通常是第三个操作数）
                const auto &operands = inst->get_operands();
                auto *label = dynamic_cast<backend::riscv::RVLabel *>(operands[2]);
                auto *target_block = find_block_by_label(label->name());
                if (target_block) {
                    // 建立前驱和后继关系
                    block->add_succ(target_block);
                    target_block->add_pred(block);
                }
            }
        }

        // 检查最后一条指令是否为跳转指令
        if (!block->get_insts().empty()) {
            auto *last_inst = block->get_insts().back();

            // 如果是返回指令，不需要添加后继
            if (last_inst->get_type() == backend::riscv::RVInstType::JR ||
                last_inst->get_type() == backend::riscv::RVInstType::RET) {
                continue;
            }

            // 如果是无条件跳转指令
            if (last_inst->get_type() == backend::riscv::RVInstType::J) {
                // 获取跳转指令的目标标签
                const auto &operands = last_inst->get_operands();
                auto *label = dynamic_cast<backend::riscv::RVLabel *>(operands[0]);
                auto *target_block = find_block_by_label(label->name());
                if (target_block) {
                    // 建立前驱和后继关系
                    block->add_succ(target_block);
                    target_block->add_pred(block);
                }
            }
        }
    }
}

// 辅助函数：根据标签名称找到对应的基本块
backend::riscv::RVBasicBlock *RVFunction::find_block_by_label(const std::string &label_name) {
    for (auto *block : blocks) {
        if (block->get_name() == label_name) {
            return block;
        }
    }
    return nullptr;
}

void RVFunction::allocate_stack_space(RVVirReg *vir_reg) {
    // 为每个虚拟寄存器分配唯一的栈空间
    stack_size += 8;  // 每个寄存器占用8字节
    vir_reg2stack_offset[vir_reg] = stack_size;
}

int RVFunction::get_stack_offset(RVVirReg *vir_reg) {
    auto it = vir_reg2stack_offset.find(vir_reg);
    if (it == vir_reg2stack_offset.end()) {
        std::cerr << "Error: stack offset not found for given virtual register!" << std::endl;
        assert(false);
    }
    return it->second;
}

void RVFunction::remap_virtual_register(RVVirReg *new_reg, RVVirReg *old_reg) {
    auto it = vir_reg2stack_offset.find(old_reg);
    if (it != vir_reg2stack_offset.end()) {
        vir_reg2stack_offset[new_reg] = it->second;
    } else {
        assert(false);
    }
    all_vir_reg_used.erase(old_reg);
    all_vir_reg_used.insert(new_reg);
}

void RVFunction::replace_virtual_register(RVVirReg *new_reg, RVVirReg *old_reg) {
    // 替换集合中的 old_reg 为 new_reg
    all_vir_reg_used.erase(old_reg);
    all_vir_reg_used.insert(new_reg);
}

// 参数偏移相关接口实现
// 注意：模板已在hpp实现为inline，这里无需重复实现

RVFunction::~RVFunction() {
    for (auto block : blocks) {
        if (block) delete block;
    }
    blocks.clear();
    for (auto vir_reg : all_vir_reg_used) {
        if (vir_reg) delete vir_reg;
    }
    all_vir_reg_used.clear();
    vir_reg2stack_offset.clear();
    stack_size = 0;
    next_vreg_index = 0;
}

}  // namespace backend::riscv
