package backend.asmInstr.asmBinary;

import backend.itemStructure.AsmOperand;

public class AsmDiv extends AsmBinary{
    public boolean isWord = false;
    public AsmDiv(AsmOperand dst, AsmOperand src1, AsmOperand src2) {
        super(dst, src1, src2);
    }
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("div");
        if(isWord){
            sb.append("w");
        }
        sb.append("\t");
        sb.append(dst);
        sb.append(",\t");
        sb.append(src1);
        sb.append(",\t");
        sb.append(src2);
        return sb.toString();
    }
}
