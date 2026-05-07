// referenced from https://github.com/dtcxzyw/cmmc/blob/main/cmmc/Analysis/AliasAnalysis.cpp

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ir/instruction.hpp"
#include "ir/mod.hpp"
#include "ir/type.hpp"
#include "ir/value.hpp"
#include "log.hpp"
#include "opt/opt_util.hpp"
#include "opt/pass.hpp"
namespace opt::pass {

struct InheritEdge {
    std::shared_ptr<ir::Value> dst;
    std::shared_ptr<ir::Value> src1;
    std::shared_ptr<ir::Value> src2;

    bool operator==(const InheritEdge &other) const {
        return dst == other.dst && src1 == other.src1 && src2 == other.src2;
    }

    InheritEdge(std::shared_ptr<ir::Value> dst,
                std::shared_ptr<ir::Value> src1,
                std::shared_ptr<ir::Value> src2 = nullptr)
        : dst(dst), src1(src1), src2(src2) {}
};

struct EdgeHasher final {
    size_t operator()(const InheritEdge &edge) const {
        std::hash<std::shared_ptr<ir::Value>> hasher;
        return (hasher(edge.dst) * 10007) ^ (hasher(edge.src1) * 131) ^ hasher(edge.src2);
    }
};

// a copy of module->opt_info.alias_info
static ir::opt_support::AliasInfo *alias_info = nullptr;
static uint32_t alloca_id = 0;
static uint32_t global_id = 1;
static std::unordered_set<uint32_t> global_group;
static std::unordered_set<uint32_t> stack_group;
static std::unordered_set<InheritEdge, EdgeHasher> inherit_graph;

static void analyze_func(std::shared_ptr<ir::Function> func);
static bool type_include(std::shared_ptr<ir::Type> t1, std::shared_ptr<ir::Type> t2);
static void divide_gep(const std::vector<std::shared_ptr<ir::Getelementptr>> &gep_list, uint32_t idx);

bool AliasAnalyzation::run(ir::Module *module) {
    logger::INFO("Running AliasAnalyzation...");
    alias_info = &module->opt_info.alias_info;
    alias_info->clear();
    global_group.clear();
    alloca_id = 0;
    global_id = alloca_id++;
    module->for_each_global_variable([&](auto &&global_var) {
        auto id = alloca_id++;
        alias_info->add_value(global_var, {global_id, id});
        global_group.insert(id);
    });
    module->for_each_func([](auto &func) { analyze_func(func); });
    return false;
}

static void analyze_func(std::shared_ptr<ir::Function> func) {
    stack_group.clear();
    inherit_graph.clear();
    auto stack_id = alloca_id++, arg_id = alloca_id++;
    alias_info->add_pair(global_id, stack_id);
    for (auto &arg : func->get_arguments_ref()) {
        if (!arg->get_type()->is_pointer_ty()) continue;
        alias_info->add_value(arg, {arg_id});
    }

    std::vector<std::shared_ptr<ir::Getelementptr>> geps;
    // layer traversal dom tree
    for (auto &block : util::layer_traversal_dom(func)) {
        for (auto &inst : block->get_instructions_ref()) {
            if (!inst->get_type()->is_pointer_ty()) continue;
            if (auto alloca = std::dynamic_pointer_cast<ir::Alloca>(inst)) {
                auto id = alloca_id++;
                stack_group.insert(id);
                alias_info->add_pair(id, arg_id);
                alias_info->add_value(alloca, {stack_id, id});
                continue;
            }
            if (auto gep = std::dynamic_pointer_cast<ir::Getelementptr>(inst)) {
                std::vector<uint32_t> attrs;
                auto cur = gep;
                while (true) {
                    auto base_ptr = cur->base_ptr();
                    if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(cur->get_indexes()[0]);
                        const_int && const_int->get_val() == 0) {
                        inherit_graph.emplace(gep, base_ptr);
                        break;
                    }
                    if (cur == gep) {
                        if (auto const_int = std::dynamic_pointer_cast<ir::ConstantInt>(cur->get_indexes()[0]);
                            !const_int || const_int->get_val() != 0) {
                            auto a1 = alloca_id++;
                            auto a2 = alloca_id++;
                            alias_info->add_pair(a1, a2);
                            attrs.push_back(a1);
                            alias_info->append_attr(cur->base_ptr(), a2);
                        }
                    }
                    if (auto base_gep = std::dynamic_pointer_cast<ir::Getelementptr>(base_ptr))
                        cur = base_gep;
                    else
                        break;
                }
                alias_info->add_value(gep, std::move(attrs));
                geps.push_back(gep);
                continue;
            }
            if (auto bitcast = std::dynamic_pointer_cast<ir::Bitcast>(inst)) {
                alias_info->add_value(bitcast, {});
                continue;
            }
            if (inst->is_type(ir::Instruction::InstructionType::LOAD) ||
                inst->is_type(ir::Instruction::InstructionType::CALL)) {
                alias_info->add_value(inst, {});
                continue;
            }
            if (auto phi = std::dynamic_pointer_cast<ir::Phi>(inst)) {
                alias_info->add_value(phi, {});
                std::unordered_set<std::shared_ptr<ir::Value>> inherit_set;
                for (auto &value : phi->values()) {
                    if (std::dynamic_pointer_cast<ir::Constant>(value)) continue;
                    inherit_set.insert(value);
                    // NOTE: up to support 2 inherit edges now
                    if (inherit_set.size() > 2) break;
                }
                if (inherit_set.size() == 1)
                    inherit_graph.emplace(phi, *inherit_set.begin());
                else if (inherit_set.size() == 2)
                    inherit_graph.emplace(phi, *inherit_set.begin(), *std::next(inherit_set.begin()));
            }
        }
    }

    // TBAA
    std::unordered_map<std::shared_ptr<ir::PointerType>, uint32_t> types;
    for (const auto &[val, attrs] : alias_info->get_pointer_attrs()) {
        auto type = std::dynamic_pointer_cast<ir::PointerType>(val->get_type());
        if (types.count(type)) continue;
        types.emplace(type, alloca_id++);
    }

    const auto byte_t = ir::IntegerType::get(8);
    for (auto i = types.cbegin(); i != types.cend(); i++) {
        const auto t1 = i->first->get_reference_type();
        if (t1 == byte_t) continue;
        for (auto j = std::next(i); j != types.cend(); j++) {
            const auto t2 = j->first->get_reference_type();
            if (t2 == byte_t) continue;
            if (!type_include(t1, t2) && !type_include(t2, t1)) alias_info->add_pair(i->second, j->second);
        }
    }
    for (const auto &[val, attrs] : alias_info->get_pointer_attrs()) {
        const auto attr = types[std::dynamic_pointer_cast<ir::PointerType>(val->get_type())];
        alias_info->append_attr(val, attr);
    }

    // dive into geps
    std::unordered_map<std::shared_ptr<ir::Value>, std::vector<std::shared_ptr<ir::Getelementptr>>> clusters;
    for (auto &gep : geps) clusters[gep->base_ptr()].push_back(gep);
    for (auto &[base, gep_list] : clusters) divide_gep(gep_list, 0);

    alias_info->add_distinct_group(global_group);
    alias_info->add_distinct_group(stack_group);

    // spread
    while (true) {
        bool modified = false;
        for (const auto &edge : inherit_graph) {
            if (edge.src2 == nullptr)
                modified |= alias_info->append_attr(edge.dst, alias_info->inherit_from(edge.src1));
            else {
                auto lhs = alias_info->inherit_from(edge.src1);
                auto rhs = alias_info->inherit_from(edge.src2);
                std::sort(lhs.begin(), lhs.end());
                std::sort(rhs.begin(), rhs.end());
                std::vector<uint32_t> intersection;
                std::set_intersection(
                    lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend(), std::back_inserter(intersection));
                modified |= alias_info->append_attr(edge.dst, intersection);
            }
        }
        if (!modified) break;
    }
}

static bool type_include(std::shared_ptr<ir::Type> t1, std::shared_ptr<ir::Type> t2) {
    if (t1 == t2) return true;
    if (t1->is_integer_ty() || t1->is_float_ty()) return false;
    if (t1->is_array_ty()) {
        const auto array_type = std::dynamic_pointer_cast<ir::ArrayType>(t1);
        const auto next = t2->is_array_ty() ? array_type->get_element_type() : array_type->scalar_type();
        return type_include(next, t2);
    }
    logger::WARN("[AliasAnalyzation::type_include] got type ", t1, ", maybe forget to run PtrAllocaReduction");
    return false;
}

static void divide_gep(const std::vector<std::shared_ptr<ir::Getelementptr>> &gep_list, uint32_t idx) {
    if (gep_list.size() <= 1) return;

    std::unordered_map<std::shared_ptr<ir::Constant>, std::vector<std::shared_ptr<ir::Getelementptr>>> clusters;
    for (const auto &gep : gep_list) {
        if (gep->get_indexes().size() <= idx) continue;
        auto index = gep->get_indexes()[idx];
        if (auto constant = std::dynamic_pointer_cast<ir::Constant>(index)) clusters[constant].push_back(gep);
    }
    if (clusters.empty()) return;

    if (clusters.size() >= 2) {
        std::unordered_set<uint32_t> distinct_group;
        for (auto &[base, geps] : clusters) {
            auto new_id = alloca_id++;
            distinct_group.insert(new_id);
            for (auto &gep : geps) alias_info->append_attr(gep, new_id);
            divide_gep(geps, idx + 1);
        }
    } else
        divide_gep(gep_list, idx + 1);
}
}  // namespace opt::pass
