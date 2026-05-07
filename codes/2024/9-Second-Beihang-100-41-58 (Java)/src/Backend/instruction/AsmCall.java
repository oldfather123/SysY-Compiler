package Backend.instruction;

import Backend.component.AsmFunction;
import Backend.component.AsmInstr;

public class AsmCall extends AsmInstr {
    private AsmFunction tarFunction;

    public AsmCall(AsmFunction objFunction) {
        this.tarFunction = objFunction;
    }

    @Override
    public String toString() {
        return "jal\t" + tarFunction.getName();
    }

    public AsmFunction getTarFunction() {
        return tarFunction;
    }
}
