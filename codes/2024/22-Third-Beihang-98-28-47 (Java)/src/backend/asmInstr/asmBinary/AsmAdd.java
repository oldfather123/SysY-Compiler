package backend.asmInstr.asmBinary;

import backend.itemStructure.AsmImm12;
import backend.itemStructure.AsmOperand;

public class AsmAdd extends AsmBinary {
    public boolean isWord = false;

    public AsmAdd(AsmOperand dst, AsmOperand src1, AsmOperand src2) {
        super(dst, src1, src2);
    }

    public void allocNewSize(int newAllocSize) {
        ((AsmImm12)super.src2).addNewAllocSize(newAllocSize);
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("add");
        if(src2 instanceof AsmImm12){
            sb.append("i");
        }
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
