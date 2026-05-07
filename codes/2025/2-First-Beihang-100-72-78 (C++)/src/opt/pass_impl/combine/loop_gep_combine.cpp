#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/value.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"

namespace opt::pass {

static std::optional<int> as_const_int(const std::shared_ptr<ir::Value> &v) {
    if (auto ci = std::dynamic_pointer_cast<ir::ConstantInt>(v)) return ci->get_val();
    return std::nullopt;
}

// Create hash string for GEP (equivalent to Java's setGEPHash)
// Only include base and prefix indices (excluding the last index)
static std::string create_gep_hash(const std::shared_ptr<ir::Getelementptr> &gep) {
    std::string hash = "getelementptr inbounds " + gep->base_ptr()->get_type()->to_string() + ", ";
    auto indices = gep->get_indexes();

    // Add base pointer info
    hash += gep->base_ptr()->get_type()->to_string() + " " + gep->base_ptr()->get_name();

    // Add all indices except the last one
    for (size_t i = 0; i + 1 < indices.size(); ++i) {
        hash += ", " + indices[i]->get_type()->to_string() + " " + indices[i]->get_name();
    }
    return hash;
}

static bool run_on_block(const std::shared_ptr<ir::BasicBlock> &block) {
    bool modified = false;

    // Phase 1: Collect - equivalent to Java's collection phase
    std::map<std::shared_ptr<ir::Getelementptr>, std::pair<std::shared_ptr<ir::Value>, int>> gep_combine_map;
    std::unordered_map<std::string, std::pair<std::shared_ptr<ir::Getelementptr>, int>> prev_gep;

    for (const auto &inst : block->get_instructions_ref()) {
        auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(inst);
        if (!gep) continue;

        auto indices = gep->get_indexes();
        if (indices.empty()) continue;

        auto last_idx = indices.back();
        auto last_const = as_const_int(last_idx);
        if (!last_const.has_value()) continue;

        std::string hash = create_gep_hash(gep);

        if (prev_gep.find(hash) == prev_gep.end()) {
            prev_gep[hash] = std::make_pair(gep, last_const.value());
        } else {
            auto prev_gep_ptr = prev_gep[hash].first;
            int prev_offset = prev_gep[hash].second;
            prev_gep[hash] = std::make_pair(gep, last_const.value());

            int offset = last_const.value() - prev_offset;
            gep_combine_map[gep] = std::make_pair(prev_gep_ptr, offset);
        }
    }

    // Phase 2: Apply - process in reverse order like Java
    if (gep_combine_map.empty()) return false;

    std::vector<std::shared_ptr<ir::Getelementptr>> keys_list;
    keys_list.reserve(gep_combine_map.size());
    for (const auto &pair : gep_combine_map) {
        keys_list.push_back(pair.first);
    }

    std::unordered_map<std::shared_ptr<ir::Getelementptr>, std::shared_ptr<ir::Getelementptr>> substitute_map;

    // Process in reverse order (like Java's for loop from size-1 to 0)
    for (int i = static_cast<int>(keys_list.size()) - 1; i >= 0; --i) {
        auto gep = keys_list[static_cast<size_t>(i)];
        auto base = gep_combine_map[gep].first;
        int offset = gep_combine_map[gep].second;

        auto offset_const = std::make_shared<ir::ConstantInt>(ir::IntegerType::get(32), offset);
        std::vector<std::shared_ptr<ir::Value>> new_indexes{offset_const};
        auto new_gep = ir::Getelementptr::create(nullptr, base, new_indexes, gen_local_var_name());

        // Set parent block and insert before current gep (like gep_lift.cpp pattern)
        new_gep->set_parent_block(block);
        block->add_instruction(gep->node, new_gep);
        // util::substitute(gep, new_gep);

        substitute_map[gep] = new_gep;
        modified = true;
    }

    for (auto &[gep, new_gep] : substitute_map) {
        util::substitute(gep, new_gep);
    }

    return modified;
}

bool LoopGEPCombine::run(ir::Module *module) {
    bool modified = false;
    for (auto &func : module->get_functions_ref()) {
        for (auto &block : func->get_basic_blocks_ref()) {
            modified |= run_on_block(block);
        }
    }
    return modified;
}

}  // namespace opt::pass
