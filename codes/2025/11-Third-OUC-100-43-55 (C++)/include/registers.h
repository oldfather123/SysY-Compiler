#ifndef RISCV_REGISTERS_H
#define RISCV_REGISTERS_H

#include "type.h"
#include "machine_insts.h"
#include "register.h"
#include <string>
#include <vector>

namespace riscv {

class MachineInst;

class MReg : public ir::Reg {
private:
  std::string _name;

public:
  static std::unique_ptr<ir::BasicType> i32_t;
  static std::unique_ptr<ir::BasicType> f32_t;

  static MReg *const zero;
  static MReg *const ra;
  static MReg *const sp;
  static MReg *const gp;
  static MReg *const tp;
  static MReg *const t0;
  static MReg *const t1;
  static MReg *const t2;
  static MReg *const s0;
  static MReg *const s1;
  static MReg *const a0;
  static MReg *const a1;
  static MReg *const a2;
  static MReg *const a3;
  static MReg *const a4;
  static MReg *const a5;
  static MReg *const a6;
  static MReg *const a7;
  static MReg *const s2;
  static MReg *const s3;
  static MReg *const s4;
  static MReg *const s5;
  static MReg *const s6;
  static MReg *const s7;
  static MReg *const s8;
  static MReg *const s9;
  static MReg *const s10;
  static MReg *const s11;
  static MReg *const t3;
  static MReg *const t4;
  static MReg *const t5;
  static MReg *const t6;
  static MReg *const fa0;
  static MReg *const fa1;
  static MReg *const fa2;
  static MReg *const fa3;
  static MReg *const fa4;
  static MReg *const fa5;
  static MReg *const fa6;
  static MReg *const fa7;
  static MReg *const fs0;
  static MReg *const fs1;
  static MReg *const fs2;
  static MReg *const fs3;
  static MReg *const fs4;
  static MReg *const fs5;
  static MReg *const fs6;
  static MReg *const fs7;
  static MReg *const fs8;
  static MReg *const fs9;
  static MReg *const fs10;
  static MReg *const fs11;
  static MReg *const ft0;
  static MReg *const ft1;
  static MReg *const ft2;
  static MReg *const ft3;
  static MReg *const ft4;
  static MReg *const ft5;
  static MReg *const ft6;
  static MReg *const ft7;
  static MReg *const ft8;
  static MReg *const ft9;
  static MReg *const ft10;
  static MReg *const ft11;

  static MachineInst *const zeroInst;
  static MachineInst *const raInst;
  static MachineInst *const spInst;
  static MachineInst *const gpInst;
  static MachineInst *const tpInst;
  static MachineInst *const a0Inst;
  static MachineInst *const fa0Inst;
  static MachineInst *const t0Inst;

  static const std::vector<MReg *> iRegs;
  static const std::vector<MReg *> fRegs;
  static const std::vector<MReg *> iCallerRegs;
  static const std::vector<MReg *> fCallerRegs;
  static const std::vector<MReg *> iCalleeRegs;
  static const std::vector<MReg *> fCalleeRegs;

  MReg(ir::BasicType *type, std::string name)
      : Reg(type), _name(std::move(name)) {}
  std::string str() const override { return _name; }

  static int argsStackOffset(int iSize, int fSize) {
    return (std::max(iSize - static_cast<int>(iCallerRegs.size()), 0) +
            std::max(fSize - static_cast<int>(fCallerRegs.size()), 0)) *
           8;
  }
};

} // namespace riscv

#endif
