package frontend.ir.instr.otherop;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.objecttype.TVoid;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.FunctionInfoCollector;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

public class CallInstr extends Instruction {
    private final Function function;
    private List<Value> rParams;

    public CallInstr(Integer regIdx, Function function, List<Value> rParams, BasicBlock parentBB) {
        super(function.getType(), regIdx, parentBB);
        this.function = function;
        this.rParams = new ArrayList<>(rParams);

        boolean cond1 = this.getType() instanceof TVoid && regIdx == null;
        boolean cond2 = !(this.getType() instanceof TVoid) && regIdx != null;
        if (!cond1 && !cond2) {
            throw new RuntimeException("函数调用要么返回值非 void 要么寄存器编号为空");
        }

        if (parentBB.isNotClosed()) {
            function.addCallCnt();
            parentBB.getParentFunc().addCallee(function);
            for (Value rParam : rParams) {
                this.setUse(rParam);
            }
        }

        function.addCall(this);
    }

    public Function getCallee() {
        return function;
    }

    public void setRParams(List<Value> rParams) {
        this.rParams = rParams;
    }

    public boolean isVoidCall() {
        return this.getType() instanceof TVoid;
    }

    public List<Value> getRParams() {
        return rParams;
    }

    public int getRParamSize() {
        return this.rParams.size();
    }

    public boolean isCallingLibFunc() {
        return function instanceof Function.LibFunc;
    }

    @Override
    protected void modifyValue(Value from, Value to) {
        for (int i = 0; i < rParams.size(); i++) {
            if (rParams.get(i) == from) {
                rParams.set(i, to);
            }
        }
    }

    @Override
    public void removeFromList() {
        super.removeFromList();
        function.decCallCnt();
        function.delCall(this);
        if (this.getParentBB().getParentFunc().getCalleeMap().get(function) == 1) {
            this.getParentBB().getParentFunc().getCalleeMap().remove(function);
        } else {
            this.getParentBB().getParentFunc().getCalleeMap().put(function, this.getParentBB().getParentFunc().getCalleeMap().get(function) - 1);
        }
    }

    @Override
    public void forceRemoveFromList() {
        super.forceRemoveFromList();
        function.delCall(this);
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("call ").append(function.getType().printIRType());
        sb.append(" ").append(function.value2string()).append("(");
        for (int i = 0; i < rParams.size(); i++) {
            Value rParam = rParams.get(i);
            sb.append(rParam.getType().printIRType()).append(" ").append(rParam.value2string());
            if (i < rParams.size() - 1) {
                sb.append(", ");
            }
        }
        sb.append(")");
        if (function.getType() instanceof TVoid) {
            return sb.toString();
        } else {
            return this.value2string() + " = " + sb;
        }
    }

    @Override
    public String myHash() {
        // todo: 没有副作用的函数还是可以 hash 的
        if (FunctionInfoCollector.getFuncInfo(function) != null && FunctionInfoCollector.getFuncInfo(function).canGVN()) {
            StringBuilder sb = new StringBuilder("call ").append(function.getName()).append(" ");
            for (int i = 0; i < this.function.getFParams().size(); i++) {
                if (!this.function.getFParams().get(i).getUserList().isEmpty()) {
                    sb.append(rParams.get(i).value2string()).append(" ");
                }
            }
            //System.out.println("myHash: "+sb.toString());
            return sb.toString();
        } else {
            return Integer.toString(this.hashCode());
        }
    }

    @Override
    public Value simplify() {
        return this;
    }

    public HashSet<Value> getOperands() {
        return new HashSet<>(rParams);
    }
}
