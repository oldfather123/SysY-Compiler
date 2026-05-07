#include "registers.h"
#include "type.h"

#define DECLARE_INT_REG(name)                                                  \
  MReg *const MReg::name = new MReg(MReg::i32_t.get(), #name)
#define DECLARE_FLOAT_REG(name)                                                \
  MReg *const MReg::name = new MReg(MReg::f32_t.get(), #name)

namespace riscv
{

std::unique_ptr<ir::BasicType> MReg::i32_t =
    std::make_unique<ir::BasicType>(ir::BasicKind::I32);
std::unique_ptr<ir::BasicType> MReg::f32_t =
    std::make_unique<ir::BasicType>(ir::BasicKind::F32);

// int
DECLARE_INT_REG(zero);
DECLARE_INT_REG(ra);
DECLARE_INT_REG(sp);
DECLARE_INT_REG(gp);
DECLARE_INT_REG(tp);
DECLARE_INT_REG(t0);
DECLARE_INT_REG(t1);
DECLARE_INT_REG(t2);
DECLARE_INT_REG(t3);
DECLARE_INT_REG(t4);
DECLARE_INT_REG(t5);
DECLARE_INT_REG(t6);
DECLARE_INT_REG(a0);
DECLARE_INT_REG(a1);
DECLARE_INT_REG(a2);
DECLARE_INT_REG(a3);
DECLARE_INT_REG(a4);
DECLARE_INT_REG(a5);
DECLARE_INT_REG(a6);
DECLARE_INT_REG(a7);
DECLARE_INT_REG(s0);
DECLARE_INT_REG(s1);
DECLARE_INT_REG(s2);
DECLARE_INT_REG(s3);
DECLARE_INT_REG(s4);
DECLARE_INT_REG(s5);
DECLARE_INT_REG(s6);
DECLARE_INT_REG(s7);
DECLARE_INT_REG(s8);
DECLARE_INT_REG(s9);
DECLARE_INT_REG(s10);
DECLARE_INT_REG(s11);

// float
DECLARE_FLOAT_REG(fa0);
DECLARE_FLOAT_REG(fa1);
DECLARE_FLOAT_REG(fa2);
DECLARE_FLOAT_REG(fa3);
DECLARE_FLOAT_REG(fa4);
DECLARE_FLOAT_REG(fa5);
DECLARE_FLOAT_REG(fa6);
DECLARE_FLOAT_REG(fa7);
DECLARE_FLOAT_REG(fs0);
DECLARE_FLOAT_REG(fs1);
DECLARE_FLOAT_REG(fs2);
DECLARE_FLOAT_REG(fs3);
DECLARE_FLOAT_REG(fs4);
DECLARE_FLOAT_REG(fs5);
DECLARE_FLOAT_REG(fs6);
DECLARE_FLOAT_REG(fs7);
DECLARE_FLOAT_REG(fs8);
DECLARE_FLOAT_REG(fs9);
DECLARE_FLOAT_REG(fs10);
DECLARE_FLOAT_REG(fs11);
DECLARE_FLOAT_REG(ft0);
DECLARE_FLOAT_REG(ft1);
DECLARE_FLOAT_REG(ft2);
DECLARE_FLOAT_REG(ft3);
DECLARE_FLOAT_REG(ft4);
DECLARE_FLOAT_REG(ft5);
DECLARE_FLOAT_REG(ft6);
DECLARE_FLOAT_REG(ft7);
DECLARE_FLOAT_REG(ft8);
DECLARE_FLOAT_REG(ft9);
DECLARE_FLOAT_REG(ft10);
DECLARE_FLOAT_REG(ft11);

MachineInst *const MReg::zeroInst = new MachineInst(MReg::zero);
MachineInst *const MReg::raInst = new MachineInst(MReg::ra);
MachineInst *const MReg::spInst = new MachineInst(MReg::sp);
MachineInst *const MReg::gpInst = new MachineInst(MReg::gp);
MachineInst *const MReg::tpInst = new MachineInst(MReg::tp);
MachineInst *const MReg::a0Inst = new MachineInst(MReg::a0);
MachineInst *const MReg::fa0Inst = new MachineInst(MReg::fa0);
MachineInst *const MReg::t0Inst = new MachineInst(MReg::t0);

const std::vector<MReg *> MReg::iRegs = {
    MReg::a0, MReg::a1, MReg::a2, MReg::a3, MReg::a4, MReg::a5, MReg::a6,
    MReg::a7, MReg::s0, MReg::s1, MReg::s2, MReg::s3, MReg::s4, MReg::s5,
    MReg::s6, MReg::s7, MReg::s8, MReg::s9, MReg::s10, MReg::s11};

const std::vector<MReg *> MReg::fRegs = {
    MReg::fa0, MReg::fa1, MReg::fa2, MReg::fa3, MReg::fa4,
    MReg::fa5, MReg::fa6, MReg::fa7, MReg::fs0, MReg::fs1,
    MReg::fs2, MReg::fs3, MReg::fs4, MReg::fs5, MReg::fs6,
    MReg::fs7, MReg::fs8, MReg::fs9, MReg::fs10, MReg::fs11};

const std::vector<MReg *> MReg::iCallerRegs = {MReg::a0, MReg::a1, MReg::a2,
                                               MReg::a3, MReg::a4, MReg::a5,
                                               MReg::a6, MReg::a7};

const std::vector<MReg *> MReg::fCallerRegs = {MReg::fa0, MReg::fa1, MReg::fa2,
                                               MReg::fa3, MReg::fa4, MReg::fa5,
                                               MReg::fa6, MReg::fa7};

const std::vector<MReg *> MReg::iCalleeRegs = {
    MReg::s0, MReg::s1, MReg::s2, MReg::s3, MReg::s4, MReg::s5,
    MReg::s6, MReg::s7, MReg::s8, MReg::s9, MReg::s10, MReg::s11};

const std::vector<MReg *> MReg::fCalleeRegs = {
    MReg::fs0, MReg::fs1, MReg::fs2, MReg::fs3, MReg::fs4, MReg::fs5,
    MReg::fs6, MReg::fs7, MReg::fs8, MReg::fs9, MReg::fs10, MReg::fs11};

} // namespace riscv

#undef DECLARE_INT_REG
#undef DECLARE_FLOAT_REG
