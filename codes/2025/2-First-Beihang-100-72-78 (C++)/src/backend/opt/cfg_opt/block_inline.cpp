#include "backend/opt/cfg_opt/block_inline.hpp"

#include <algorithm>
#include <list>
#include <queue>
#include <unordered_set>
#include <vector>

#include "backend/opt/cfg_opt/simplify_cfg.hpp"
#include "backend/riscv/rv_instruction.hpp"
#include "log.hpp"

namespace backend::opt::cfg_opt {

void BlockInline::run(backend::riscv::RVModule *module) {
    // 执行完这个之后，指令格式的状态应该是,中间可能有B，但是最后一定是一个J
    for (const auto &[name, function] : module->get_functions()) {
        if (function->is_external()) continue;
        bool con = true;
        while (con) {
            con = pre_simplify(function);
        }
        con = true;
        while (con) {
            con = block_inline(function);
        }
    }
}

bool BlockInline::pre_simplify(backend::riscv::RVFunction *func) {
    bool modify = false;
    modify |= SimplifyCFG::redirect_goto(func);
    modify |= SimplifyCFG::remove_unused_labels(func);
    return modify;
}

bool BlockInline::block_inline(backend::riscv::RVFunction *function) {
    // 只需要一个一个复制即可，删除可以交给广搜解决
    bool modify = false;
    auto &blocks = function->get_blocks();

    for (auto *block : blocks) {
        if (block->get_insts().empty()) continue;

        // 如果最后一个是j指令并且长度比阈值小，那么就直接复制过来
        auto *last_inst = block->get_insts().back();
        if (last_inst->get_type() == backend::riscv::RVInstType::J) {
            auto operands = last_inst->get_operands();
            auto *label = dynamic_cast<backend::riscv::RVLabel *>(operands[0]);

            // 查找目标块
            backend::riscv::RVBasicBlock *to = nullptr;
            for (auto *target_block : function->get_blocks()) {
                if (target_block->get_name() == label->name()) {
                    to = target_block;
                    break;
                }
            }

            if (to && to->get_insts().size() <= MAX_LEN) {
                // 判断是否这个块是个自指块
                if (to == block) {
                    std::vector<backend::riscv::RVInstruction *> tmp;
                    for (auto *ri : block->get_insts()) {
                        tmp.push_back(ri->clone());
                    }
                    last_inst->delete_self();
                    for (auto *ri : tmp) {
                        block->add_inst(ri);
                    }
                } else {
                    // 不是自指块，那么指向了一个块
                    last_inst->delete_self();
                    for (auto *ri : to->get_insts()) {
                        block->add_inst(ri->clone());
                    }
                }
                modify = true;
            }
        }
    }

    return SimplifyCFG::remove_unused_labels(function) || modify;
}

}  // namespace backend::opt::cfg_opt
