#pragma once

#include <unordered_map>
#include <vector>

#include "backend/riscv/rv_basic_block.hpp"
#include "backend/riscv/rv_function.hpp"
#include "backend/riscv/rv_global_var.hpp"
#include "backend/riscv/rv_instruction.hpp"
#include "backend/riscv/rv_module.hpp"
#include "backend/riscv/rv_operand.hpp"

namespace backend::opt::memory_opt {

class KnownBaseLSOpt {
public:
    static void run(backend::riscv::RVModule *module);

private:
    struct LSRecord {
        backend::riscv::RVReg *reg;
        long offset;
        backend::riscv::RVOperand *base;

        LSRecord(backend::riscv::RVReg *reg, long off, backend::riscv::RVOperand *base);
        backend::riscv::RVReg *get_reg() const;
        long get_offset() const;
        backend::riscv::RVOperand *get_base() const;
        LSRecord my_copy(backend::riscv::RVReg *new_reg) const;
    };

    static void run_on_block(backend::riscv::RVBasicBlock *block);
    static void cover_by_offset(const LSRecord &record);
    static void cover_by_reg(backend::riscv::RVReg *reg);
    static std::vector<LSRecord> query_by_off(backend::riscv::RVReg *best_reg,
                                              backend::riscv::RVOperand *base,
                                              long offset);
    static std::vector<LSRecord> query_by_reg(backend::riscv::RVReg *reg);
    static bool has_same(const LSRecord &record);

    static std::unordered_map<backend::riscv::RVReg *, backend::riscv::RVGlobalVar *> base_records;
    static std::vector<LSRecord> records;
};

}  // namespace backend::opt::memory_opt
