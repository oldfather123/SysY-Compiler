package mid.IntermediatePresentation.Instruction;

import mid.IntermediatePresentation.User;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.Optimizer;

import java.util.ArrayList;

public class Store extends Instruction {
    private Value addr;

    public Store(Value val, Value addr) {
        super("STORE");
        use(val);
        this.addr = addr;
        use(addr);
    }

    public Value getSrc() {
        return operandList.get(0);
    }

    public Value getAddr() {
        if (operandList.size() <= 1) {
            return addr;
        } else {
            return operandList.get(1);
        }
    }

    public String toString() {
        String type = operandList.get(1).getRefType().toString();
        return "store " + type + " " + operandList.get(0).getReg() + ", " +
                type + "* " + operandList.get(1).getReg() + "\n";
    }

    public boolean isUseless() {
        return false;
    }

    public boolean isDefInstr() {
        return false;
    }

    public void setAddr(Value addr) {
        this.addr = addr;
    }

    public boolean hasSideEffect() {
        Value addr = getAddr();
        while (addr instanceof GetElementPtr gep) {
            for (User user : gep.getUserList()) {
                if (user instanceof Call call && Optimizer.instance().hasSideEffect(call.getCallingFunction())) {
                    //如果被当作了参数，那一定是有副作用了
                    return true;
                }
            }
            addr = gep.getPtr();
        }

        for (User user : addr.getUserList()) {
            if (user instanceof Call call && Optimizer.instance().hasSideEffect(call.getCallingFunction())) {
                return true;
            }
        }

        //如果是全局的，或者是从参数里传过来，那就一定有副作用
        return addr instanceof GlobalDecl || getBlock().getFunction().getParam().getParams().contains(addr);
    }

    public void setOperandList(ArrayList<Value> operandList) {
        this.operandList = operandList;
        addr = operandList.get(1);
    }
}
