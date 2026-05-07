package mid.IntermediatePresentation.Instruction;

import mid.IntermediatePresentation.Value;

import java.util.ArrayList;

public class Load extends Instruction {
    //只有getAddr且并没有operand时才可以访问这个addr，其余都要用operandList.get(0)代替
    private Value addr;

    public Load(String reg, Value addr) {
        super(reg, addr.getRefType());
        this.addr = addr;
        use(addr);
    }

    public Value getAddr() {
        if (operandList.size() == 0) {
            return addr;
        } else {
            return operandList.get(0);
        }
    }

    public String toString() {
        Value addr = operandList.get(0);
        return reg + " = load " + addr.getRefType() + ", " + addr.getTypeString() + " " + addr.getReg() + "\n";
    }

    public ArrayList<String> GVNHash() {
        return null;
    }

    public void setAddr(Value addr) {
        this.addr = addr;
    }

    public void setOperandList(ArrayList<Value> operandList) {
        this.operandList = operandList;
        addr = operandList.get(0);
    }
}
