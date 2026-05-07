#pragma once

#include <unordered_map>
#include <vector>

#include "backend/riscv/rv_basic_block.hpp"
#include "backend/riscv/rv_function.hpp"
#include "backend/riscv/rv_instruction.hpp"
#include "backend/riscv/rv_module.hpp"
#include "backend/riscv/rv_operand.hpp"

namespace backend::opt::memory_opt {

class UnknownBaseLSOpt {
public:
    struct UBRecord {
        backend::riscv::RVReg *reg;
        long offset;
        backend::riscv::RVReg *base;

        UBRecord(backend::riscv::RVReg *reg, long off, backend::riscv::RVReg *base);
        backend::riscv::RVReg *get_reg() const;
        long get_offset() const;
        backend::riscv::RVReg *get_base() const;
        UBRecord my_copy(backend::riscv::RVReg *new_reg) const;
    };

    static void run(backend::riscv::RVModule *module);
    static void remove_by_base(backend::riscv::RVReg *base);
    static void remove_by_reg(backend::riscv::RVReg *reg);
    static std::vector<UBRecord> query_by_off(backend::riscv::RVReg *best_reg,
                                              backend::riscv::RVReg *base,
                                              long offset);

    static std::vector<UBRecord> records;

private:
    static void run_on_block(backend::riscv::RVBasicBlock *block);
    static void remove_by_rewrite(const UBRecord &cord);
};

}  // namespace backend::opt::memory_opt
