#include "rv_operand.hpp"

#include <utility>

#include "ir/value.hpp"
#include "rv_function.hpp"
#include "rv_instruction.hpp"

namespace backend::riscv {

// A basic mapping for ABI names. A more robust solution might be needed later.
const char *const REGISTER_NAMES[] = {"zero", "ra", "sp", "gp", "tp",  "t0",  "t1", "t2", "s0", "s1", "a0",
                                      "a1",   "a2", "a3", "a4", "a5",  "a6",  "a7", "s2", "s3", "s4", "s5",
                                      "s6",   "s7", "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

/* -------- Base RVOperand Def-Use Management -------- */
void RVOperand::add_graph_use(RVInstruction *inst) {
    // assert(inst != nullptr);
    graph_uses.insert(inst);
}

void RVOperand::remove_graph_use(RVInstruction *inst) {
    // assert(inst != nullptr);
    graph_uses.erase(inst);
}

const std::set<RVInstruction *> &RVOperand::get_graph_uses() const { return graph_uses; }

RVOperand::~RVOperand() { graph_uses.clear(); }

/* -------- Register -------- */
RVPhyReg *RVPhyReg::create(int phys_id, const std::string &name) { return new RVPhyReg(phys_id, name); }

RVPhyReg *RVPhyReg::create(int phys_id, const std::string &name, RVPhyReg::Type type) {
    return new RVPhyReg(phys_id, name, type);
}

std::string RVPhyReg::to_string() const {
    if (!_name.empty()) {
        return _name;  // Use alias like "fa0" if it exists
    }
    if (_phys_id >= 0 && _phys_id < 32) {
        return REGISTER_NAMES[_phys_id];
    }
    return "x" + std::to_string(_phys_id);
}

void RVPhyReg::add_graph_use(RVInstruction *inst) { RVOperand::add_graph_use(inst); }

void RVPhyReg::remove_graph_use(RVInstruction *inst) { RVOperand::remove_graph_use(inst); }

RVPhyReg::~RVPhyReg() {}

/* -------- Immediate -------- */
RVImmediate *RVImmediate::create(long long val) { return new RVImmediate(val); }

RVImmediate::RVImmediate(long long val) : val(val) {}

std::string RVImmediate::to_string() const { return std::to_string(val); }

RVImmediate::~RVImmediate() {}

/* -------- Label / Symbol -------- */
RVLabel *RVLabel::create(std::string name) { return new RVLabel(std::move(name)); }

RVLabel::~RVLabel() {}

/* -------- Stack Location -------- */
RVStk *RVStk::create(int offset) { return new RVStk(offset); }

std::string RVStk::to_string() const { return std::to_string(_offset) + "(sp)"; }

RVStk::~RVStk() {}

/* -------- Stack Fixer -------- */
RVStackFixer *RVStackFixer::create(RVFunction *function, int extra_offset) {
    return new RVStackFixer(function, extra_offset);
}

RVStackFixer::RVStackFixer(RVFunction *function, int extra_offset) : _function(function), _extra_offset(extra_offset) {}

int RVStackFixer::get_aligned_offset() const {
    auto function = _function;
    if (!function) {
        // Fallback if function is no longer available
        return _extra_offset;
    }

    int stack_position = function->get_stack_size();

    if (stack_position == 0) {
        return _extra_offset;
    }
    return stack_position - 1 - (stack_position - 1) % 16 + 16 + _extra_offset;
}

std::string RVStackFixer::to_string() const { return std::to_string(get_aligned_offset()); }

RVStackFixer::~RVStackFixer() {}

/* -------- Symbol Address Parts -------- */
RVHi *RVHi::create(RVLabel *label) { return new RVHi(label); }

RVHi::RVHi(RVLabel *label) : _label(label) {}

std::string RVHi::to_string() const { return "%hi(" + _label->name() + ")"; }

RVLabel *RVHi::get_label() const { return _label; }

RVHi::~RVHi() {}

RVLo *RVLo::create(RVLabel *label) { return new RVLo(label); }

RVLo::RVLo(RVLabel *label) : _label(label) {}

std::string RVLo::to_string() const { return "%lo(" + _label->name() + ")"; }

RVLabel *RVLo::get_label() const { return _label; }

RVLo::~RVLo() {}

/* -------- Virtual Register -------- */
RVVirReg *RVVirReg::create(int index, RVVirReg::RegType reg_type, RVFunction *function) {
    auto *const vir_reg = new RVVirReg(index, reg_type, function);

    // Register the virtual register with the function after construction
    if (function) {
        function->add_virtual_register(vir_reg);
    }

    return vir_reg;
}

RVVirReg::RVVirReg(int index, RegType reg_type, RVFunction *function)
    : _index(index), _reg_type(reg_type), _function(function) {
    // Generate name based on type and index, mimicking Java logic
    if (reg_type == RegType::INT_TYPE) {
        _name = "%int" + std::to_string(index);
    } else {
        _name = "%float" + std::to_string(index);
    }

    // Register this virtual register with the function
    if (function) {
        // Note: We can't call function->add_virtual_register(shared_from_this()) here
        // because the object is not fully constructed yet. This needs to be done
        // from the create() method after construction.
    }
}

RVVirReg::~RVVirReg() {}

}  // namespace backend::riscv
