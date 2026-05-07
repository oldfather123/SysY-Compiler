/**
 * @file reg_alloc.h
 * 该头文件定义了寄存器分配相关的数据结构和算法
 */
#ifndef AAAC_REG_ALLOC_H
#define AAAC_REG_ALLOC_H

#pragma once

#include "backend.h"
#include "common.h"
#include "riscv.h"
#include "sym.h"
#include "ADT/CFG.h"

#include <list>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace aaac {
namespace backend {

void LivenessPass(frontend::ControlFlowGraph *cfg);

/**
 * @brief LiveInterval class
 *
 * Represents the live range of a variable in the program
 */
class LiveInterval {
private:
    std::shared_ptr<sym::Var> var;
    int start;  // Start point of the interval
    int end;    // End point of the interval
    RISCV_Reg assigned_reg = RISCV_Reg::INVALID;
    bool Float = false;
    bool spilled = false;
    int spill_offset = -1;  // Stack offset if spilled

public:
    LiveInterval(std::shared_ptr<sym::Var> v, int s, int e)
        : var(std::move(v)), start(s), end(e) {}

    const std::shared_ptr<sym::Var>& getVar() const { return var; }
    int getStart() const { return start; }
    int getEnd() const { return end; }
    void setStart(int s) { start = s; }
    void setEnd(int e) { end = e; }

    void assignReg(RISCV_Reg reg) { assigned_reg = reg; }
    RISCV_Reg getAssignedReg() const { return assigned_reg; }

    void setSpilled(bool value) { spilled = value; }
    bool isSpilled() const { return spilled; }

    void setSpillOffset(int offset) { spill_offset = offset; }
    int getSpillOffset() const { return spill_offset; }

    // Comparison operator for sorting intervals
    bool operator<(const LiveInterval& other) const {
        return start < other.start;
    }
    void setFloat(bool f) { Float = f; }
    bool isFloat() const    { return Float; }
};


// 由于需要拆为不同 Pass，需要把 LiveInterval 当作 CFG 的 Property
struct LiveIntervalProperty : public utils::BaseProperty, public DebugDumpImpl {
  std::vector<LiveInterval> intervals;
  LiveIntervalProperty() = default;
  LiveIntervalProperty(const std::vector<LiveInterval>& ivs)
    : intervals(ivs) {}
  std::ostream& dump(std::ostream& os) const override {
    os << "LiveIntervals(" << intervals.size() << ")\n";
    return os;
  }
};


/**
 * @brief Build live intervals for all variables in a function
 *
 * @param func The function to analyze
 * @return std::vector<LiveInterval> Vector of live intervals
 */
std::vector<LiveInterval> buildLiveIntervals(Function* func);

/**
 * @brief Perform linear scan register allocation
 *
 * @param func The function to allocate registers for
 * @param intervals Vector of live intervals
 * @param available_regs Vector of available registers
 * @return true if allocation was successful
 */
bool linearScanAllocation(Function* func,
                          std::vector<LiveInterval>& intervals,
                          const std::vector<RISCV_Reg>& available_regs,
                          const std::vector<RISCV_Reg>& float_regs);

/**
 * @brief Insert spill code for spilled variables
 *
 * @param func The function containing spilled variables
 * @param intervals Vector of live intervals
 */
 void insertSpillCode(Function* func, const std::vector<LiveInterval>& intervals);

 /**
 * @brief Convenience helper to run the full linear scan algorithm
 *
 */
 bool performLinearScan(Function* func, const std::vector<RISCV_Reg>& available_regs,
                       const std::vector<RISCV_Reg>& float_regs,
                       std::vector<LiveInterval>& out_intervals);
} // namespace backend
} // namespace aaac

#endif
