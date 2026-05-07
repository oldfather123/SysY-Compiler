#include <algorithm>
#include <climits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/mod.hpp"
#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {

// Map: instruction -> vector of (count, value)
using CountedValue = std::pair<int, std::shared_ptr<ir::Value>>;
static std::unordered_map<std::shared_ptr<ir::Value>, std::vector<CountedValue>> factor_map;
static std::unordered_map<std::shared_ptr<ir::Value>, int> number_map;

static int get_number(const std::shared_ptr<ir::Value> &v) {
    if (std::dynamic_pointer_cast<ir::Constant>(v)) return INT_MAX;
    auto it = number_map.find(v);
    if (it != number_map.end()) return it->second;
    return 0;
}

static bool is_associative(const std::shared_ptr<ir::Instruction> &inst) {
    using IT = ir::Instruction::InstructionType;
    const auto ty = inst->get_ins_type();
    return ty == IT::ADD || ty == IT::MUL;  // restrict to integer add/mul
}

static void run_on_inst(const std::shared_ptr<ir::Instruction> &inst) {
    using IT = ir::Instruction::InstructionType;
    if (!inst) return;
    if (!is_associative(inst)) return;
    const auto ty = inst->get_ins_type();
    auto block = inst->get_parent_block().lock();
    if (!block) return;

    std::vector<CountedValue> args;
    auto op1 = inst->get_operands_ref()[0];
    auto op2 = inst->get_operands_ref()[1];

    auto take = [&](const std::shared_ptr<ir::Value> &v) {
        if (auto sub = std::dynamic_pointer_cast<ir::Instruction>(v); sub && sub->get_ins_type() == ty) {
            auto it = factor_map.find(v);
            if (it != factor_map.end()) {
                args.insert(args.end(), it->second.begin(), it->second.end());
                return;
            }
        }
        args.emplace_back(1, v);
    };
    take(op1);
    take(op2);

    // sort: constants first; then by number_map ascending
    std::sort(args.begin(), args.end(), [&](const CountedValue &a, const CountedValue &b) {
        const auto &lv = a.second;
        const auto &rv = b.second;
        const bool lc = (bool)std::dynamic_pointer_cast<ir::ConstantInt>(lv);
        const bool rc = (bool)std::dynamic_pointer_cast<ir::ConstantInt>(rv);
        if (lc && rc) {
            return std::dynamic_pointer_cast<ir::ConstantInt>(lv)->get_val() <
                   std::dynamic_pointer_cast<ir::ConstantInt>(rv)->get_val();
        }
        if (lc) return true;
        if (rc) return false;
        return get_number(lv) < get_number(rv);
    });

    // merge same values: accumulate counts
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i].first == 0) continue;
        for (size_t j = i + 1; j < args.size(); ++j) {
            if (args[j].second != args[i].second) break;
            args[i].first += args[j].first;
            args[j].first = 0;
        }
    }
    args.erase(std::remove_if(args.begin(), args.end(), [](const CountedValue &p) { return p.first == 0; }),
               args.end());

    // fast-path: if single user and same op type, memoize and defer rebuild
    if (inst->get_users_ref().size() == 1) {
        if (auto only_user = inst->get_users_ref().front().lock()) {
            if (auto iu = std::dynamic_pointer_cast<ir::Instruction>(only_user)) {
                if (iu->get_ins_type() == ty) {
                    factor_map[inst] = args;
                    return;
                }
            }
        }
    }

    factor_map[inst] = {CountedValue{1, inst}};

    bool need_rebuild = false;
    int num_consts = 0;
    for (const auto &pr : args) {
        if (std::dynamic_pointer_cast<ir::ConstantInt>(pr.second)) num_consts++;
        if (pr.first != 1) need_rebuild = true;
    }
    if (!need_rebuild && num_consts < 1) return;

    // rebuild
    std::vector<std::shared_ptr<ir::Value>> storage;
    auto insert_before = inst;  // position anchor

    if (ty == IT::ADD) {
        for (const auto &pr : args) {
            if (pr.first == 1) {
                storage.push_back(pr.second);
            } else {
                // k * v
                auto k = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), pr.first);
                auto mul = ir::Mul::create(block, pr.second, k, gen_local_var_name());
                block->add_instruction(insert_before->node, mul);
                storage.push_back(mul);
            }
        }
    } else if (ty == IT::MUL) {
        for (const auto &pr : args) {
            if (pr.first == 1) {
                storage.push_back(pr.second);
            } else {
                // power by squaring like v^k as product of v and v*v...
                int c = pr.first;
                auto v = pr.second;
                while (c != 0) {
                    if (c & 1) storage.push_back(v);
                    c >>= 1;
                    if (c != 0) {
                        auto mul = ir::Mul::create(block, v, v, gen_local_var_name());
                        block->add_instruction(insert_before->node, mul);
                        v = mul;
                    }
                }
            }
        }
    }

    std::shared_ptr<ir::Value> reduced = nullptr;
    for (const auto &v : storage) {
        if (!reduced) {
            reduced = v;
            continue;
        }
        if (ty == IT::ADD) {
            auto add = ir::Add::create(block, reduced, v, gen_local_var_name());
            block->add_instruction(insert_before->node, add);
            reduced = add;
        } else {
            auto mul = ir::Mul::create(block, reduced, v, gen_local_var_name());
            block->add_instruction(insert_before->node, mul);
            reduced = mul;
        }
    }

    if (auto reduced_inst = std::dynamic_pointer_cast<ir::Instruction>(reduced)) {
        if (!factor_map.count(reduced)) {
            auto arr = factor_map[inst];
            if (arr.size() == 1 && arr[0].second == inst) arr[0].second = reduced;
            factor_map[reduced] = arr;
        }
        util::substitute(inst, reduced);
    }
}

bool Reassociate::run(ir::Module *module) {
    logger::INFO("Running Reassociate...");
    bool modified = false;
    module->for_each_func([&](const auto &func) {
        // numbering per block in dom order is non-trivial here; we use order in block
        func->for_each_block([&](const auto &block) {
            int num = 1;
            for (auto &inst : block->get_instructions_ref()) {
                if (inst) number_map[inst] = num++;
            }
            // take a snapshot to avoid iterator invalidation during substitution
            std::vector<std::shared_ptr<ir::Instruction>> snapshot;
            snapshot.reserve(block->get_instructions_ref().size());
            for (auto &inst : block->get_instructions_ref()) snapshot.push_back(inst);
            for (auto &inst : snapshot) {
                if (!inst) continue;
                run_on_inst(inst);
                modified = true;  // conservatively mark modified
            }
        });
    });
    factor_map.clear();
    number_map.clear();
    return modified;
}

}  // namespace opt::pass
