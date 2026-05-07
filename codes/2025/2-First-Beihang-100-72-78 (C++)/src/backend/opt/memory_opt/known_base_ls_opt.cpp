#include "backend/opt/memory_opt/known_base_ls_opt.hpp"

namespace backend::opt::memory_opt {

std::unordered_map<backend::riscv::RVReg *, backend::riscv::RVGlobalVar *> KnownBaseLSOpt::base_records;
std::vector<KnownBaseLSOpt::LSRecord> KnownBaseLSOpt::records;

KnownBaseLSOpt::LSRecord::LSRecord(backend::riscv::RVReg *reg, long off, backend::riscv::RVOperand *base)
    : reg(reg), offset(off), base(base) {}

backend::riscv::RVReg *KnownBaseLSOpt::LSRecord::get_reg() const { return reg; }

long KnownBaseLSOpt::LSRecord::get_offset() const { return offset; }

backend::riscv::RVOperand *KnownBaseLSOpt::LSRecord::get_base() const { return base; }

KnownBaseLSOpt::LSRecord KnownBaseLSOpt::LSRecord::my_copy(backend::riscv::RVReg *new_reg) const {
    return LSRecord(new_reg, offset, base);
}

void KnownBaseLSOpt::run(backend::riscv::RVModule *module) {
    // TODO: 运行已知基址load/store优化
}

void KnownBaseLSOpt::run_on_block(backend::riscv::RVBasicBlock *block) {
    // TODO: 在基本块上运行
}

void KnownBaseLSOpt::cover_by_offset(const LSRecord &record) {
    // TODO: 按偏移覆盖
}

void KnownBaseLSOpt::cover_by_reg(backend::riscv::RVReg *reg) {
    // TODO: 按寄存器覆盖
}

std::vector<KnownBaseLSOpt::LSRecord> KnownBaseLSOpt::query_by_off(backend::riscv::RVReg *best_reg,
                                                                   backend::riscv::RVOperand *base,
                                                                   long offset) {
    // TODO: 按偏移查询
    return std::vector<LSRecord>();
}

std::vector<KnownBaseLSOpt::LSRecord> KnownBaseLSOpt::query_by_reg(backend::riscv::RVReg *reg) {
    // TODO: 按寄存器查询
    return std::vector<LSRecord>();
}

bool KnownBaseLSOpt::has_same(const LSRecord &record) {
    // TODO: 检查是否有相同记录
    return false;
}

}  // namespace backend::opt::memory_opt
