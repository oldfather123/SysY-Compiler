package Backend.instruction;

import Backend.component.AsmInstr;

public class AsmRet extends AsmInstr {
    @Override
    public String toString() {
        return "jr\t" + "ra";
    }
}