package backend.asmInstr.asmLS;

import backend.asmInstr.AsmInstr;
import backend.itemStructure.AsmImm12;
import backend.itemStructure.AsmOperand;
public class AsmS extends AsmInstr {
    protected AsmOperand src, addr, offset;

    public AsmS() {
        super("AsmS");
    }

    public void changeSrc(AsmOperand src) {
        addUseReg(this.src, src);
        this.src = src;
    }
    public void changeAddr(AsmOperand addr) {
        addUseReg(this.addr, addr);
        this.addr = addr;
    }
    public void changeOffset(AsmOperand offset) {
        addUseReg(this.offset, offset);
        this.offset = offset;
    }
    public void ReSetSrc(int index,AsmOperand src) {
        if (index == 0) {
            this.src = src;
        }else {
            this.addr = src;
        }
    }

    public AsmOperand getOffset() {
        return offset;
    }

    public AsmOperand getSrc() {
        return src;
    }

    public AsmOperand getAddr() {
        return addr;
    }
    public void allocNewSize(int newAllocSize) {
        ((AsmImm12)offset).addNewAllocSize(newAllocSize);
    }
}
