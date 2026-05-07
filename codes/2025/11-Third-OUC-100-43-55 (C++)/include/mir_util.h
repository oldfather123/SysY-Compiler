#ifndef RISCV_MIR_UTIL_H
#define RISCV_MIR_UTIL_H

#include "machine_func.h"
#include "machine_insts.h"
#include <bitset>
#include <limits>

#define MAKE_I32 std::make_unique<ir::BasicType>(ir::BasicKind::I32)
#define MAKE_F32 std::make_unique<ir::BasicType>(ir::BasicKind::F32)

namespace riscv {

MachineInst *loadImmI(MachineBlock *block, int imm);

MachineInst *loadImmF(MachineBlock *block, int imm);

MachineInst *loadImmI(MachineBlock *block, float imm);

MachineInst *loadImmF(MachineBlock *block, float imm);

int bitcount(int x);

int leadingZeros(int x);

int trailingZeros(int x);

MachineInst *addRegRegI(MachineBlock *block, MachineInst *src0,
                        MachineInst *src1);

MachineInst *addRegRegF(MachineBlock *block, MachineInst *src0,
                        MachineInst *src1);

MachineInst *addRegImmF(MachineBlock *block, MachineInst *src, float imm);

MachineInst *addRegImmI(MachineBlock *block, MachineInst *src, int imm);

MachineInst *divRegRegF(MachineBlock *block, MachineInst *src0,
                        MachineInst *src1);

MachineInst *divRegRegI(MachineBlock *block, MachineInst *src0,
                        MachineInst *src1);

MachineInst *divImmRegF(MachineBlock *block, float imm, MachineInst *src);

MachineInst *divImmRegI(MachineBlock *block, int imm, MachineInst *src);

MachineInst *divRegImmF(MachineBlock *block, MachineInst *src, float imm);

MachineInst *divRegImmI(MachineBlock *block, MachineInst *src, int imm);

MachineInst *modRegReg(MachineBlock *block, MachineInst *src0,
                       MachineInst *src1);

MachineInst *modImmReg(MachineBlock *block, int imm, MachineInst *src);

MachineInst *modRegImm(MachineBlock *block, MachineInst *src, int imm);

MachineInst *mulRegRegF(MachineBlock *block, MachineInst *src0,
                        MachineInst *src1);

MachineInst *mulRegRegI(MachineBlock *block, MachineInst *src0,
                        MachineInst *src1);

MachineInst *mulRegImmF(MachineBlock *block, MachineInst *src, float imm);

MachineInst *mulRegImmI(MachineBlock *block, MachineInst *src, int imm);

MachineInst *subRegRegF(MachineBlock *block, MachineInst *src0,
                        MachineInst *src1);

MachineInst *subRegRegI(MachineBlock *block, MachineInst *src0,
                        MachineInst *src1);

MachineInst *subImmRegF(MachineBlock *block, float imm, MachineInst *src);
MachineInst *subImmRegI(MachineBlock *block, int imm, MachineInst *src);
MachineInst *subRegImmF(MachineBlock *block, MachineInst *src, float imm);
MachineInst *subRegImmI(MachineBlock *block, MachineInst *src, int imm);

MachineInst *binImmReg(MachineBlock *block, ir::BinaryInst *binInst,
                       ir::ConstantNumber *imm, MachineInst *src);
MachineInst *binRegImm(MachineBlock *block, ir::BinaryInst *binInst,
                       MachineInst *src, ir::ConstantNumber *imm);
MachineInst *binRegReg(MachineBlock *block, ir::BinaryInst *binInst,
                       MachineInst *src0, MachineInst *src1);

} // namespace riscv

#endif
