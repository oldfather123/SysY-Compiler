package backend.asmInstr;

import Utils.CustomList;
import backend.asmInstr.asmBinary.AsmBinary;
import backend.asmInstr.asmBinary.AsmFneg;
import backend.asmInstr.asmBr.AsmBnez;
import backend.asmInstr.asmConv.AsmFtoi;
import backend.asmInstr.asmConv.AsmitoF;
import backend.asmInstr.asmLS.AsmL;
import backend.asmInstr.asmLS.AsmS;
import backend.itemStructure.AsmBlock;
import backend.itemStructure.AsmOperand;
import backend.regs.AsmReg;

import java.util.ArrayList;
import java.util.HashSet;

public class AsmInstr extends CustomList.Node {
    /* 用于记录该指令的寄存器的定义和使用情况
    例如对于 add x1, x2, x3, 会有 regDef = {x1}, regUse = {x2, x3}
    引用新寄存器时需要释放旧寄存器的使用权
     */
    public HashSet<AsmReg> LiveIn = new HashSet<>();
    public HashSet<AsmReg> LiveOut = new HashSet<>();
    public ArrayList<AsmReg> regDef = new ArrayList<>();
    public ArrayList<AsmReg> regUse = new ArrayList<>();

    private String type;
    public AsmBlock parent;

    public AsmInstr(String type) {
        this.type = type;
    }

    public void addDefReg(AsmOperand oldReg, AsmOperand newReg) {
        if (oldReg != null && oldReg instanceof AsmReg)
            regDef.remove((AsmReg) oldReg);
        if (newReg instanceof AsmReg)
            regDef.add((AsmReg) newReg);
    }

    public void addUseReg(AsmOperand oldReg, AsmOperand newReg) {
        if (oldReg != null && oldReg instanceof AsmReg)
            regUse.remove((AsmReg) oldReg);
        if (newReg instanceof AsmReg)
            regUse.add((AsmReg) newReg);
    }

    public void changeUseReg(int index ,AsmReg oldReg, AsmReg newReg) {
        if (type == "AsmL") {
            if (this.regUse.get(index) == oldReg) {
                this.regUse.set(index, newReg);
                AsmL asmL = (AsmL) this;
                asmL.ReSetSrc(newReg);
            }
        } else if (type == "AsmS") {
            if (this.regUse.get(index) == oldReg) {
                this.regUse.set(index, newReg);
                AsmS asmS = (AsmS) this;
                asmS.ReSetSrc(index,newReg);
            }
        } else if (type == "AsmBinary") {
            if (this.regUse.get(index) == oldReg) {
                this.regUse.set(index, newReg);
                AsmBinary asmBinary = (AsmBinary) this;
                asmBinary.ReSetSrc(index, newReg);
            }
        } else if (type == "AsmBnez") {
            if (this.regUse.get(index) == oldReg) {
                this.regUse.set(index, newReg);
                AsmBnez asmBnez = (AsmBnez) this;
                asmBnez.ReSetSrc(newReg);
            }
        } else if (type == "AsmFtoi") {
            if (this.regUse.get(index) == oldReg) {
                this.regUse.set(index, newReg);
                AsmFtoi asmFtoi = (AsmFtoi) this;
                asmFtoi.ReSetSrc(index, newReg);
            }
        } else if (type == "AsmitoF") {
            if (this.regUse.get(index) == oldReg) {
                this.regUse.set(index, newReg);
                AsmitoF asmitoF = (AsmitoF) this;
                asmitoF.ReSetSrc(index, newReg);
            }
        } else if (type == "AsmZext") {
            if (this.regUse.get(index) == oldReg) {
                this.regUse.set(index, newReg);
                AsmitoF asmitoF = (AsmitoF) this;
                asmitoF.ReSetSrc(index, newReg);
            }
        } else if (type == "fneg") {
            if (this.regUse.get(index) == oldReg) {
                this.regUse.set(index, newReg);
                AsmFneg asmFneg = (AsmFneg) this;
                asmFneg.ReSetSrc(index, newReg);
            }
        }
    }
    public void changeDstReg(int index ,AsmReg oldReg, AsmReg newReg) {
        if (type == "AsmL") {
            if (this.regDef.get(index) == oldReg) {
                this.regDef.set(index, newReg);
                AsmL asmL = (AsmL) this;
                asmL.ReSetDst(newReg);
            }
        } else if (type == "AsmBinary") {
            if (this.regDef.get(index) == oldReg) {
                this.regDef.set(index, newReg);
                AsmBinary asmBinary = (AsmBinary) this;
                asmBinary.ReSetDst(newReg);
            }
        } else if (type == "AsmFtoi") {
            if (this.regDef.get(index) == oldReg) {
                this.regDef.set(index, newReg);
                AsmFtoi asmFtoi = (AsmFtoi) this;
                asmFtoi.ReSetDst(index, newReg);
            }
        } else if (type == "AsmitoF") {
            if (this.regDef.get(index) == oldReg) {
                this.regDef.set(index, newReg);
                AsmitoF asmitoF = (AsmitoF) this;
                asmitoF.ReSetDst(index, newReg);
            }
        } else if (type == "AsmZext") {
            if (this.regDef.get(index) == oldReg) {
                this.regDef.set(index, newReg);
                AsmitoF asmitoF = (AsmitoF) this;
                asmitoF.ReSetDst(index, newReg);
            }
        } else if (type == "fneg") {
            if (this.regDef.get(index) == oldReg) {
                this.regDef.set(index, newReg);
                AsmFneg asmFneg = (AsmFneg) this;
                asmFneg.ReSetDst(index, newReg);
            }
        }
    }


}
