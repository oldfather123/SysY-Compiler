package cn.edu.nju.software.backendarm.arminstruction;

import cn.edu.nju.software.backendarm.arminstruction.operand.ArmImmediateValue;
import cn.edu.nju.software.backendarm.arminstruction.operand.ArmLabelAddress;
import cn.edu.nju.software.backendarm.arminstruction.operand.ArmOperand;
import cn.edu.nju.software.backendarm.arminstruction.util.ArmOpcode;

public class ArmLdr extends ArmDefaultInstruction {

    public ArmLdr(ArmOperand... armOperands) {
        super(ArmOpcode.LDR, armOperands);
    }

    //ldr needs =
    //LDR R0, =label
}
