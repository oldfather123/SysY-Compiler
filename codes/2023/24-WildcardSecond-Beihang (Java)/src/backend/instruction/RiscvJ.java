package backend.instruction;

import backend.component.RiscvBlock;
import backend.component.RiscvInstr;
import backend.operand.RiscvLabel;

import java.util.ArrayList;
import java.util.Arrays;

public class RiscvJ extends RiscvInstr {

    public RiscvJ(RiscvLabel label, RiscvBlock parent) {
        super(null, new ArrayList<>(Arrays.asList(label)));
        assert label instanceof RiscvBlock;
        RiscvBlock block1 = (RiscvBlock) label;
        block1.addPreds(parent);
        parent.addSuccs(block1);
    }

    @Override
    public String toString() {
        return "j" + " " + operands.get(0);
    }
}
