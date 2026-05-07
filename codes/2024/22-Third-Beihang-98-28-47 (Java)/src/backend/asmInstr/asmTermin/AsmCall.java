package backend.asmInstr.asmTermin;

import backend.asmInstr.AsmInstr;
import backend.regs.RegGeter;

public class AsmCall extends AsmInstr {
    public String funcName;
    public int IntArgRegNum;
    public int floatArgRegNum;
    public boolean isLibCall;

    public AsmCall(String funcName, int IntArgRegNum,int floatArgRegNum,boolean isLibCall) {
        super("AsmCall");
        this.funcName = funcName;
        this.IntArgRegNum = IntArgRegNum;
        this.floatArgRegNum = floatArgRegNum;
        for (int i = 0 ; i < IntArgRegNum ; i++) {
            addUseReg(null, RegGeter.AllRegsInt.get(i + 10));
        }
        for (int i = 0 ; i < floatArgRegNum ; i++) {
            addUseReg(null, RegGeter.AllRegsFloat.get(i + 10));
        }
        this.isLibCall = isLibCall;
    }

    public String getFuncName() {
        return funcName;
    }

    @Override
    public String toString() {
        return "call\t" + funcName;
    }
}
