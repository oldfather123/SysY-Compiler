package Backend.instruction;

import Backend.component.AsmBlock;
import Backend.component.AsmInstr;

public class AsmJ extends AsmInstr {
    private AsmBlock target;

    public AsmJ(AsmBlock target) {
        this.target = target;
    }

    @Override
    public String toString() {
        return "j\t" + target.getName();
    }

    public AsmBlock getTarget() {
        return target;
    }

    public void setTarget(AsmBlock target) {
        this.target = target;
    }
}
