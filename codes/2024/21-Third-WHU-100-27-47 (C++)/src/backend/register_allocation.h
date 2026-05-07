#ifndef BACKEND_REGISTER_ALLOCATION_H_
#define BACKEND_REGISTER_ALLOCATION_H_

#include "Common.h"
#include "instruction_selection.h"
#include "register.h"
#include "DFA.h"

namespace backend {
    void findReadWrittenRegs(Ptr<RiscFunction>);

    void fillVirtualRegs(Ptr<RiscFunction>);

    struct LiveInterval {
        Ptr<VirtualRegister> vreg;
        int start;
        int end;

        LiveInterval() : vreg(nullptr), start(0), end(0) { }
        LiveInterval(Ptr<VirtualRegister> vreg, int s, int e)
            : vreg(vreg), start(s), end(e) { }
    };

    class LinearScan {
    private:
        int numGeneralRegisters;
        int numFloatRegisters;

    public:
        Ptr<RiscFunction> func;

        LinearScan(Ptr<RiscFunction> func, int numGeneralRegisters, int numFloatRegisters)
            : func(func), numGeneralRegisters(numGeneralRegisters), numFloatRegisters(numFloatRegisters) { }

        std::vector<LiveInterval> computeLiveIntervals();

        void allocateRegistersForType(std::vector<LiveInterval> &);

        int getAvailableRegistersCount(const std::vector<std::shared_ptr<Register>> &, Register::Type);

        void expireOldIntervals(const LiveInterval &interval, std::vector<LiveInterval *> &);

        Ptr<Register> getFreeRegisters(const std::vector<LiveInterval *> &, const std::vector<std::shared_ptr<Register>> &, Register::Type);

        void spillAtInterval(LiveInterval &interval, std::vector<LiveInterval *> &, const std::vector<std::shared_ptr<Register>> &);
    };

    void allocateRegisters(Ptr<RiscFunction>, ir::DFAContext &);

}

#endif
