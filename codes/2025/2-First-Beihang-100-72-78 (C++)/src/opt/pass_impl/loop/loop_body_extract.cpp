#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/mod.hpp"
#include "ir/support.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
#include "util.hpp"

namespace opt::pass {

// Safe-subset LoopBodyExtract: extract simple loop bodies into standalone functions
// Constraints (safe subset):
// - Loop must have header and latch
// - No function calls inside loop body
// - Values defined in body are not used outside (except induction maintenance)
// - Control flow inside body stays within body (except latch -> exit)
// - Do not attempt atomic reduction transformation

static bool is_simple_body(const std::unordered_set<std::shared_ptr<ir::BasicBlock>> &body) {
    for (auto &bb : body) {
        for (auto &inst : bb->get_instructions_ref()) {
            if (inst->is_type(ir::Instruction::InstructionType::CALL)) return false;
        }
    }
    return true;
}

static std::unordered_set<std::shared_ptr<ir::BasicBlock>> collect_body(const std::shared_ptr<ir::BasicBlock> &header,
                                                                        const std::shared_ptr<ir::BasicBlock> &latch) {
    std::unordered_set<std::shared_ptr<ir::BasicBlock>> body;
    // DFS from header until latch; include all reachable blocks until hitting non-dominated exits
    std::vector<std::shared_ptr<ir::BasicBlock>> stack{header};
    while (!stack.empty()) {
        auto cur = stack.back();
        stack.pop_back();
        if (body.count(cur)) continue;
        body.insert(cur);
        for (auto &succ_w : cur->opt_info.successors) {
            auto succ = succ_w.lock();
            if (!succ) continue;
            if (!body.count(succ)) stack.push_back(succ);
        }
    }
    // ensure connectivity: all successors (except exit from latch) are in body
    for (auto &bb : body) {
        for (auto &succ_w : bb->opt_info.successors) {
            auto succ = succ_w.lock();
            if (!succ) continue;
            if (!body.count(succ) && bb != latch) return {};
        }
    }
    return body;
}

static bool used_outside_body(const std::unordered_set<std::shared_ptr<ir::BasicBlock>> &body,
                              const std::shared_ptr<ir::Value> &v) {
    for (auto &wu : v->get_users_ref()) {
        auto u = wu.lock();
        if (!u) continue;
        auto ui = std::dynamic_pointer_cast<ir::Instruction>(u);
        if (!ui) continue;
        auto pb = ui->get_parent_block().lock();
        if (!pb) continue;
        if (!body.count(pb)) return true;
    }
    return false;
}

static bool extract_one(ir::Module *module,
                        const std::shared_ptr<ir::Function> &func,
                        const std::shared_ptr<ir::opt_support::LoopInfo> &loop) {
    auto header = loop->header;
    if (!header) return false;
    auto latch = loop->latch();
    if (!latch) return false;

    auto body = collect_body(header, latch);
    if (body.empty()) return false;
    if (!is_simple_body(body)) return false;

    // Values defined in body but used outside are not allowed (safe subset)
    for (auto &bb : body) {
        for (auto &inst : bb->get_instructions_ref()) {
            if (used_outside_body(body, inst)) return false;
        }
    }

    // Build extracted function type: void () for safe subset
    auto fty = ir::FunctionType::get(ir::VoidType::get(), {});
    auto extracted = std::make_shared<ir::Function>(fty, "@" + gen_block_name());
    module->add_function(extracted);

    // Clone blocks into new function (preserve order by simple topo from header)
    std::unordered_map<std::shared_ptr<ir::Value>, std::shared_ptr<ir::Value>> vmap;
    // Create a mapping for basic blocks
    std::unordered_map<std::shared_ptr<ir::BasicBlock>, std::shared_ptr<ir::BasicBlock>> bbmap;
    for (auto &bb : func->get_basic_blocks_ref()) {
        if (!body.count(bb)) continue;
        auto newbb = std::make_shared<ir::BasicBlock>(extracted, bb->get_name());
        extracted->add_basic_block(extracted->get_basic_blocks_ref().end(), newbb);
        bbmap[bb] = newbb;
    }
    // Clone instructions
    for (auto &[oldbb, newbb] : bbmap) {
        for (auto &inst : oldbb->get_instructions_ref()) {
            auto cloned = inst->clone();
            cloned->set_parent_block(newbb);
            newbb->add_instructions({cloned});
            vmap[inst] = cloned;
        }
    }
    // Fix operands inside cloned region
    for (auto &nbb : extracted->get_basic_blocks_ref()) {
        for (auto &inst : nbb->get_instructions_ref()) {
            auto &ops = inst->get_operands_ref();
            for (auto &op : ops) {
                if (vmap.count(op)) op = vmap[op];
                // operands outside body are constants/globals/args; safe subset assumes no such defs
            }
        }
    }

    // In original function, replace body blocks with a single call to extracted
    // Strategy: redirect predecessors of header to a trampoline block that calls extracted, then to original latch
    // successor
    auto tramp = std::make_shared<ir::BasicBlock>(func, gen_block_name());
    func->add_basic_block(header->node, tramp);

    // call extracted
    auto call = ir::Call::create(tramp, extracted, {}, gen_local_var_name());
    tramp->add_instructions({call});

    // branch to header's non-body successor (choose any exit from latch)
    std::shared_ptr<ir::BasicBlock> exit = nullptr;
    for (auto &succ_w : latch->opt_info.successors) {
        auto succ = succ_w.lock();
        if (!succ) continue;
        if (!body.count(succ)) {
            exit = succ;
            break;
        }
    }
    if (!exit) return false;
    tramp->add_instructions({ir::Br::create(tramp, exit, gen_local_var_name())});

    // redirect preds of header (outside body) to trampoline
    std::vector<std::shared_ptr<ir::BasicBlock>> preds;
    for (auto &pred_w : header->opt_info.predecessors) {
        auto pred = pred_w.lock();
        if (pred && !body.count(pred)) preds.push_back(pred);
    }
    for (auto &pred : preds) {
        opt::util::redirect(pred, header, tramp);
    }

    // Remove body blocks from original function (safe; we already redirected)
    for (auto &bb : func->get_basic_blocks_ref()) {
        if (body.count(bb)) {
            // disconnect by replacing terminators with direct branch to exit to keep CFG consistent
            auto term = bb->tail_instruction();
            if (!std::dynamic_pointer_cast<ir::Br>(term)) {
                bb->add_instructions({ir::Br::create(bb, exit, gen_local_var_name())});
            }
        }
    }
    // Physically erase body blocks
    for (auto it = func->get_basic_blocks_ref().begin(); it != func->get_basic_blocks_ref().end();) {
        if (body.count(*it))
            it = func->get_basic_blocks_ref().erase(it);
        else
            ++it;
    }

    return true;
}

bool LoopBodyExtract::run(ir::Module *module) {
    bool modified = false;
    module->for_each_func([&](auto f) {
        // require loop info built by AnalyzeLoop
        auto &forest = f->opt_info.loop_forest;
        for (auto it = forest.begin(); it != forest.end(); ++it) {
            auto loop = *it;
            if (!loop) continue;
            modified |= extract_one(module, f, loop);
        }
    });
    return modified;
}

}  // namespace opt::pass
