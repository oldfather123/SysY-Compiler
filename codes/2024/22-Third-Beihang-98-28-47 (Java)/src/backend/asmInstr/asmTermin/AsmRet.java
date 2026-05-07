package backend.asmInstr.asmTermin;

import backend.asmInstr.AsmInstr;

public class AsmRet extends AsmInstr {
    public AsmRet() {
        super("ret");
    }
    public String toString() {
        return "ret";
    }
}
