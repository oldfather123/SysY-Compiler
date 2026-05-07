#pragma once

#include "middleend/ir.hpp"
#include "middleend/cfg.hpp"
#include "middleend/dominator_tree.hpp"
#include "middleend/use_def_analysis.hpp"
#include "middleend/constant_propagation.hpp"

namespace middleend {

struct ValueRange {
    std::unordered_map<BinaryOp, std::unordered_set<std::pair<Temp *, ir::BasicBlock *>>> bounds;

    std::string to_str() {
        std::string result;
        if (bounds.count(BinaryOp::Lt)) {
            result += "  Lt: ";
            for (auto [val, bb]: bounds[BinaryOp::Lt]) {
                result += val->to_str() + " in " + bb->get_name() + ", ";
            }
            result += "\n";
        }

        if (bounds.count(BinaryOp::Gt)) {
            result += "  Gt: ";
            for (auto [val, bb]: bounds[BinaryOp::Gt]) {
                result += val->to_str() + " in " + bb->get_name() + ", ";
            }
            result += "\n";
        }

        if (bounds.count(BinaryOp::Leq)) {
            result += "  Leq: ";
            for (auto [val, bb]: bounds[BinaryOp::Leq]) {
                result += val->to_str() + " in " + bb->get_name() + ", ";
            }
            result += "\n";
        }

        if (bounds.count(BinaryOp::Geq)) {
            result += "  Geq: ";
            for (auto [val, bb]: bounds[BinaryOp::Geq]) {
                result += val->to_str() + " in " + bb->get_name() + ", ";
            }
            result += "\n";
        }

        if (bounds.count(BinaryOp::Neq)) {
            result += "  Neq: ";
            for (auto [val, bb]: bounds[BinaryOp::Neq]) {
                result += val->to_str() + " in " + bb->get_name() + ", ";
            }
            result += "\n";
        }
        return result;
    }
    
    bool empty() const {
        return bounds.empty();
    }

    bool update(BinaryOp op, Temp *val, ir::BasicBlock *bb) {
        if (bounds.count(op) == 0) {
            bounds[op] = std::unordered_set<std::pair<Temp *, ir::BasicBlock *>>();
        }
        auto &bound = bounds[op];
        auto pair = std::make_pair(val, bb);
        if (bound.count(pair) == 0) {
            bound.insert(pair);
            return true;
        }
        return false;
    }

    std::unordered_set<std::pair<Temp *, ir::BasicBlock *>> get_temps(BinaryOp op) {
        std::unordered_set<std::pair<Temp *, ir::BasicBlock *>> result;
        if (bounds.count(op)) {
            result.insert(bounds[op].begin(), bounds[op].end());
        }
        if (op == BinaryOp::Leq) {
            if (bounds.count(BinaryOp::Lt)) {
                result.insert(bounds[BinaryOp::Lt].begin(), bounds[BinaryOp::Lt].end());
            }
        }
        if (op == BinaryOp::Geq) {
            if (bounds.count(BinaryOp::Gt)) {
                result.insert(bounds[BinaryOp::Gt].begin(), bounds[BinaryOp::Gt].end());
            }
        }
        return result;
    }
};

class RangeAnalysis {
private:
    ir::Function *func_;
    CFG *cfg_;
    DominatorTree_ *dt_;
    UseDefAnalysis *ud_;
    std::unordered_map<Temp *, int> constant_map;
    std::map<std::tuple<Temp *, ir::BasicBlock *, ir::BasicBlock *>, ValueRange> from_bb_range;
    std::unordered_map<std::pair<Temp *, ir::BasicBlock *>, ValueRange> in_bb_range;
    std::unordered_map<std::pair<Temp *, ir::BasicBlock *>, bool> val_map;
    std::unordered_set<ir::BasicBlock *> should_split_bbs;
    bool changed;
    bool all_changed;
    
    void insert_from_bb_range(Temp *temp, ir::BasicBlock *bb, BinaryOp op, Temp *val, ir::BasicBlock *prev_bb);
    void update_bound(BinaryOp op, ir::BasicBlock *bb, ir::BasicBlock *pred, Temp *temp, Temp *val);
    void update_bound(ir::BasicBlock *bb, Temp *temp, ValueRange range);
    void unite(Temp *temp, ir::BasicBlock *bb, BinaryOp op, Temp *val, ir::BasicBlock *prev_bb);
    void unite(Temp *temp, ir::BasicBlock *bb, ValueRange range);
    void update_val(ir::BasicBlock *bb, Temp *temp, bool val);
    bool has_bound(ir::BasicBlock *bb, Temp *temp) const;
    ValueRange get_bound(Temp *temp, ir::BasicBlock *pred);
    ValueRange get_pred_bound(ir::BasicBlock *bb, Temp *temp, ir::BasicBlock *pred);
    int check_val(ir::BasicBlock *bb, Temp *temp);
    int cond_result(ValueRange range, BinaryOp op, Temp *val, ir::BasicBlock *bb);
    void run();

public:
    ~RangeAnalysis() {
        delete cfg_;
        delete dt_;
        delete ud_;
    }
    RangeAnalysis(ir::Function *func) : func_(func) {
        do {
            run();
        } while (all_changed);
    }
};

}