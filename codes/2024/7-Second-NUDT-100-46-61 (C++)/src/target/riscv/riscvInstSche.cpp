#define NDEBUG
#include "../../../include/ir/ir.hpp"

#include "../../../include/mir/mir.hpp"
#include "../../../include/mir/instinfo.hpp"

#include "../../../include/target/riscv/riscv.hpp"
#include "../../../include/autogen/riscv/ScheduleModelDecl.hpp"

#include "../../../include/autogen/riscv/ScheduleModelImpl.hpp"

namespace mir::RISCV {

MicroArchInfo& RISCVScheduleModel_sifive_u74::getMicroArchInfo() {
    static MicroArchInfo info{
        .enablePostRAScheduling = true,
        .hasRegRenaming = false,
        .hasMacroFusion = false,
        .issueWidth = 2,
        .outOfOrder = false,
        .hardwarePrefetch = true,
        .maxDataStreams = 8,
        .maxStrideByBytes = 256,
    };
    return info;
}
bool RISCVScheduleModel_sifive_u74::peepholeOpt(MIRFunction& func,
                                                CodeGenContext& context) {
    return false;
}
bool RISCVScheduleModel_sifive_u74::isExpensiveInst(MIRInst* inst,
                                                    CodeGenContext& context) {
    return false;
}

}  // namespace mir::RISCV

//! Dont Change This Line!"