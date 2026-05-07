#include "AsmPassLiAddToAddi.h"

namespace Backend
{
    std::vector<AsmInstBasic *> AsmPassLiAddToAddi::run(std::vector<AsmInstBasic *> insts)
    {
        std::vector<AsmInstBasic *> newInsts;

        int instsSize = insts.size();
        for (int i = 0; i < instsSize; i ++)
        {
            if (i + 1 < instsSize 
                && insts[i]->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstLoad
                && insts[i + 1]->getOpcodeBasic() == AsmInstBasic::OpcodeBasic::AsmInstAdd
            )
            {
                AsmInstLoad *loadInst = static_cast<AsmInstLoad *>(insts[i]);
                AsmInstAdd *addInst = static_cast<AsmInstAdd *>(insts[i + 1]);

                if (loadInst->getOpcode() == AsmInstLoad::Opcode::LI
                    && addInst->getOpcode() == AsmInstAdd::Opcode::ADD
                )
                {
                    // li reg1, imm
                    // add reg2, reg3, reg4

                    AsmOperandRegister *reg1 = static_cast<AsmOperandRegister *>(loadInst->getOperand(0));
                    AsmOperandImmediate *imm = static_cast<AsmOperandImmediate *>(loadInst->getOperand(1));
                    AsmOperandRegister *reg2 = static_cast<AsmOperandRegister *>(addInst->getOperand(0));
                    AsmOperandRegister *reg3 = static_cast<AsmOperandRegister *>(addInst->getOperand(1));
                    AsmOperandRegister *reg4 = static_cast<AsmOperandRegister *>(addInst->getOperand(2));

                    // li $t0, imm
                    // add $t1, $t0, $t2 / add $t1, $t2, $t0

                    if (ImmediateWithin12Bits(imm->getValue())
                        && (reg1 == reg3 || reg1 == reg4)
                    )
                    {
                        // addi dst, base, imm
                        AsmOperandRegisterInt *dst = static_cast<AsmOperandRegisterInt *>(reg2);
                        AsmOperandRegisterInt *base = static_cast<AsmOperandRegisterInt *>(reg1 == reg3 ? reg4 : reg3);
                        AsmInstAdd *newInst = new AsmInstAdd(dst, base, imm, 64);
                        newInsts.push_back(newInst);

                        i ++;
                        continue;
                    }
                }
            }

            newInsts.push_back(insts[i]);
        }

        return newInsts;
    }
}