#include "riscv_value.hpp"
#include "register.hpp"
#include "ir_type.hpp"
#include "ir_value.hpp"

// --------------------------------------  构造函数  --------------------------------------
RiscvValue::RiscvValue(RiscvTypeID rtid, const string& name)
    : rtid(rtid), name(name) {}

RiscvConst::RiscvConst(int val)
    : RiscvValue(RiscvTypeID::RIntImmTy), int_val(val) {}

RiscvConst::RiscvConst(float val)
    : RiscvValue(RiscvTypeID::RFloatImmTy), float_val(val) {}

RiscvIntReg::RiscvIntReg(Register* reg)
    : RiscvValue(RiscvTypeID::RIntRegTy), reg(reg) {}

RiscvFloatReg::RiscvFloatReg(Register* reg)
    : RiscvValue(RiscvTypeID::RFloatRegTy), reg(reg) {}

RiscvIntMem::RiscvIntMem(Register* base, int offset, bool is_global)
    : RiscvValue(RiscvTypeID::RIntMemTy), base(base), base_name(base->alias), offset(offset), is_global(is_global) {}

RiscvIntMem::RiscvIntMem(const string& base_name, int offset, bool is_global)
    : RiscvValue(RiscvTypeID::RIntMemTy), base(nullptr), base_name(base_name), offset(offset), is_global(is_global) {
}

RiscvFloatMem::RiscvFloatMem(Register* base, int offset, bool is_global)
    : RiscvValue(RiscvTypeID::RFloatMemTy), base(base), offset(offset), is_global(is_global) {}

RiscvFloatMem::RiscvFloatMem(const string& base_name, int offset, bool is_global)
    : RiscvValue(RiscvTypeID::RFloatMemTy), base(nullptr), base_name(base_name), offset(offset), is_global(is_global) {
}

RiscvGlobalVariable::RiscvGlobalVariable(RiscvTypeID type, const string& name, bool is_const, Const* init_val, int element_num)
    : RiscvValue(type, name), is_const(is_const), element_num(element_num), init_val(init_val) {
}

// --------------------------------------  功能函数  --------------------------------------
bool RiscvValue::is_reg() {
    return rtid == RiscvTypeID::RIntRegTy || rtid == RiscvTypeID::RFloatRegTy;
}