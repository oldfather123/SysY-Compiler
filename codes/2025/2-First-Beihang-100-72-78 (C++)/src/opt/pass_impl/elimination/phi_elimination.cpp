#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <unordered_set>

#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {
static bool modified = false;

static void try_eliminate_phi(std::shared_ptr<ir::Phi> phi);

static std::unordered_set<std::shared_ptr<ir::Phi>> deleted_phis;

bool PhiElimination::run(ir::Module *module) {
    logger::INFO("Running PhiElimination");
    modified = false;
    module->for_each_func([](auto &func) {
        func->for_each_block([](auto &block) {
            auto instructions = block->get_instructions();
            for (auto inst : instructions) {
                if (auto phi = std::dynamic_pointer_cast<ir::Phi>(inst)) {
                    try_eliminate_phi(phi);
                }
            }
        });
    });
    deleted_phis.clear();
    return modified;
}

static inline bool value_equal(std::shared_ptr<ir::Value> v1, std::shared_ptr<ir::Value> v2) {
    if (v1 == v2) return true;
    if (std::dynamic_pointer_cast<ir::ConstantInt>(v1) && std::dynamic_pointer_cast<ir::ConstantInt>(v2)) {
        return std::dynamic_pointer_cast<ir::ConstantInt>(v1)->get_val() ==
               std::dynamic_pointer_cast<ir::ConstantInt>(v2)->get_val();
    }
    return false;
}

static void try_eliminate_phi(std::shared_ptr<ir::Phi> phi) {
    if (deleted_phis.count(phi)) return;

    auto &operands = phi->get_operands_ref();

    // 1. only one value
    // this will destroy LCSSA form
    if (operands.size() == 2) {
        modified = true;
        deleted_phis.insert(phi);
        util::substitute(phi, operands[0]);
        return;
    }

    // 2. all operands are the same
    auto first_val = operands[0];
    if (std::all_of(std::next(operands.begin(), 2), operands.end(), [&](const auto &val) {
            size_t index = &val - &operands[0];
            return (index % 2 != 0) || (val && value_equal(val, first_val));
        })) {
        modified = true;
        deleted_phis.insert(phi);
        util::substitute(phi, first_val);
        return;
    }
}

}  // namespace opt::pass
