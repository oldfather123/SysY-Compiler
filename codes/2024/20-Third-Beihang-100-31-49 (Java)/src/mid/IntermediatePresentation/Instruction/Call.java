package mid.IntermediatePresentation.Instruction;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.Optimizer;

import java.util.ArrayList;

public class Call extends Instruction {

    public Call(Function function, ArrayList<Value> params) {
        super("CALL", function.getType());
        if (function.isVoid()) {
            use(function);
        } else {
            reg = IRManager.getInstance().declareTempVar();
            use(function);
        }

        for (Value v : params) {
            use(v);
        }
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        Function function = (Function) operandList.get(0);
        if (function.isVoid()) {
            sb.append("call void ");
        } else {
            sb.append(reg).append(" = call ").append(function.getTypeString()).append(" ");
        }
        sb.append(function.getReg()).append("(");


        for (Value param : operandList) {
            if (!(param instanceof Function)) {
                sb.append(param.getTypeString()).append(" ");
                sb.append(param.getReg()).append(", ");
            }
        }
        if (operandList.size() != 1) {
            sb = new StringBuilder(sb.substring(0, sb.length() - 2));
        }
        sb.append(")\n");
        return sb.toString();
    }

    public ArrayList<String> GVNHash() {
        // TODO: 函数的gvn (之后会做inline,必要性存疑)
//        if (Optimizer.instance().hasSideEffect(getCallingFunction())) {
//            return super.GVNHash();
//        } else {
            return null;
//        }
    }

    public Function getCallingFunction() {
        return (Function) operandList.get(0);
    }

    public boolean isUseless() {
        return !Optimizer.instance().hasSideEffect(getCallingFunction()) && userList.size() == 0;
    }

    public Number toConst() {
        if (Optimizer.instance().hasSideEffect(getCallingFunction())) {
            //不能有副作用
            return null;
        }
        Function function = (Function) operandList.get(0);
        Number retVal = null;
        if (!function.isVoid()) {
            for (BasicBlock block : function.getBlocks()) {
                for (Instruction instruction : block.getInstructionList()) {
                    if (instruction instanceof Ret ret) {
                        if (ret.getRetValue() instanceof ConstNumber n) {
                            //必须有且仅有一个返回常数值的ret语句(或返回值相同)
                            if (retVal == null || retVal.floatValue() == n.getVal().floatValue()) {
                                retVal = n.getVal();
                            } else {
                                return null;
                            }
                        } else {
                            return null;
                        }
                    }
                }
            }
        }
        return retVal;
    }
}
