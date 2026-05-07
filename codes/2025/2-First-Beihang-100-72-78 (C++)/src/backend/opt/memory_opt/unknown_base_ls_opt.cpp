#include "backend/opt/memory_opt/unknown_base_ls_opt.hpp"

namespace backend::opt::memory_opt {

std::vector<UnknownBaseLSOpt::UBRecord> UnknownBaseLSOpt::records;

UnknownBaseLSOpt::UBRecord::UBRecord(backend::riscv::RVReg *reg, long off, backend::riscv::RVReg *base)
    : reg(reg), offset(off), base(base) {}

backend::riscv::RVReg *UnknownBaseLSOpt::UBRecord::get_reg() const { return reg; }

long UnknownBaseLSOpt::UBRecord::get_offset() const { return offset; }

backend::riscv::RVReg *UnknownBaseLSOpt::UBRecord::get_base() const { return base; }

UnknownBaseLSOpt::UBRecord UnknownBaseLSOpt::UBRecord::my_copy(backend::riscv::RVReg *new_reg) const {
    return UBRecord(new_reg, offset, base);
}

void UnknownBaseLSOpt::run(backend::riscv::RVModule *module) {
    // TODO: 运行未知基址load/store优化
}

void UnknownBaseLSOpt::remove_by_base(backend::riscv::RVReg *base) {
    // TODO: 按基址删除
}

void UnknownBaseLSOpt::remove_by_reg(backend::riscv::RVReg *reg) {
    // TODO: 按寄存器删除
}

std::vector<UnknownBaseLSOpt::UBRecord> UnknownBaseLSOpt::query_by_off(backend::riscv::RVReg *best_reg,
                                                                       backend::riscv::RVReg *base,
                                                                       long offset) {
    // TODO: 按偏移查询
    return std::vector<UBRecord>();
}

void UnknownBaseLSOpt::run_on_block(backend::riscv::RVBasicBlock *block) {
    // TODO: 在基本块上运行
}

void UnknownBaseLSOpt::remove_by_rewrite(const UBRecord &cord) {
    // TODO: 按重写删除
}

}  // namespace backend::opt::memory_opt
