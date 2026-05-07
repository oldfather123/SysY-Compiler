#ifndef __LINEAR_SCAN_H_
#define __LINEAR_SCAN_H_

#include "codegen/MachineCodePass.hpp"
#include "codegen/MachineOperand.hpp"

#include <set>
#include <string>
#include <string_view>
#include <map>
#include <unordered_map>
#include <vector>
#include <stack>

class MachineUnit;
class MachineFunction;

class LinearScan : public MachineCodePass {
public:
    struct Interval {
        std::string vreg_name;
        int begin, end;
        bool fpu{false};
        bool spilled{false};
        int phy_reg{};
        int offset{};
    };

    // struct IntervalBeginCmper {
    //     bool operator() (const Interval *a, const Interval *b) const { return a->begin < b->begin; }
    // };
    struct IntervalEndCmper {
        bool operator() (const Interval *a, const Interval *b) const {
            return a->end < b->end || a->end == b->end && a->vreg_name < b->vreg_name;
        }
    };

    static std::vector<IREGISTER> init_free_regs;
    std::vector<IREGISTER> free_regs;

    static std::vector<Fregister> init_free_f_regs;
    std::vector<Fregister> free_f_regs;

    std::vector< std::unique_ptr<Interval> > intervals;
    std::vector< std::unique_ptr<Interval> > f_intervals;

    std::multiset<Interval *, IntervalEndCmper> active;
    std::multiset<Interval *, IntervalEndCmper> f_active;
    
    std::unordered_map<std::string, std::unique_ptr<Interval> > reg_allocation;

    std::unordered_map<std::string, std::set<IREGISTER>> caller_saved_regs;
    std::unordered_map<std::string, std::set<Fregister>> caller_saved_f_regs;

    std::unordered_map<std::string, std::set<IREGISTER>> callee_saved_regs;
    std::unordered_map<std::string, std::set<Fregister>> callee_saved_f_regs;

    int spilledReg{0};
        
    void pass(MachineUnit *mUnit) override;
    void computeLiveIntervals(MachineFunction *mFunc);
    void linearScanRegisterAllocation(MachineFunction *mFunc);
    void expireOldIntervals(Interval *i);
    void spillInterval(Interval *i, int framesize);
    void expireOldIntervals_f(Interval *i);
    void spillInterval_f(Interval *i, int framesize);
    void updateUsedPhysicalRegs(const std::string &func_name, IREGISTER reg);
    void updateUsedPhysicalRegs_f(const std::string &func_name, Fregister reg);
    void rewrite(MachineFunction *mFunc);
};

#endif