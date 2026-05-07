package IR.Value.Instructions;

import IR.Type.PointerType;
import IR.Type.Type;
import IR.Value.BasicBlock;
import IR.Value.Value;

import java.util.ArrayList;

public class GepInst extends Instruction{
    private boolean isInitArrayInst = false;
    public GepInst(Value target, ArrayList<Value> indexs, Type type) {
        super("%" + (++Value.valNumber), type, OP.GEP);
        this.addOperand(target);
        for(Value index : indexs){
            this.addOperand(index);
        }
    }

    public Value getTarget() {
        return getOperand(0);
    }

    public ArrayList<Value> getIndexs() {
        ArrayList<Value> indexs = new ArrayList<>();
        for(int i = 1; i < getOperands().size(); i++){
            indexs.add(getOperand(i));
        }
        return indexs;
    }

    //  是否是数组初始化时的存储指令(优化用)
    public boolean isInitArrayInst(){
        return this.isInitArrayInst;
    }

    public void setAsInitArrayInst(){
        this.isInitArrayInst = true;
    }

    public void modifyTarget(Value target){
        this.replaceOperand(0, target);
    }

    public void modifyIndexs(ArrayList<Value> indexs){
        ArrayList<Value> tmpOperands = new ArrayList<>(operands);
        for(int i = 1; i < tmpOperands.size(); i++){
            Value operand = tmpOperands.get(i);
            operand.removeUseByUser(this);
            operands.remove(operand);
        }
        for(Value idxValue : indexs){
            this.addOperand(idxValue);
        }
    }

    @Override
    public String getInstString(){
        StringBuilder sb = new StringBuilder();
        sb.append(getName()).append(" = getelementptr ");
        Value target = getTarget();
        PointerType pointerType = (PointerType) target.getType();
        sb.append(pointerType.getEleType()).append(", ");
        sb.append(pointerType).append(" ");
        sb.append(target.getName());

        ArrayList<Value> indexs = getIndexs();
        for(Value index : indexs){
            sb.append(", i32 ").append(index.getName());
        }
        return sb.toString();
    }

}
