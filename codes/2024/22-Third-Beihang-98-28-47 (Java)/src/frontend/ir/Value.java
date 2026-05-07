package frontend.ir;

import Utils.CustomList;
import frontend.ir.instr.Instruction;
import frontend.ir.structure.BasicBlock;

import java.util.HashSet;

public abstract class Value extends CustomList.Node {
    private static int value_num = 0;
    protected int pointerLevel = 0;
    private static int id = ++value_num;
    /*begin与end是链表的头尾指针（本身不存放真正的use，也就是noUse时仅存在头尾指针），
    * 负责记录这个这个value被哪些use使用*/
    private final CustomList useList;   // 保存 “用了 this” 的 Value
    
    public Value() {
        useList = new CustomList();
    }
    
    public int getPointerLevel() {
        return pointerLevel;
    }
    
    public void insertAtTail(Use use) {
        useList.addToTail(use);
    }
    
    public abstract Number getNumber();
    
    public abstract DataType getDataType();
    
    public String type2string() {
        return this.getDataType().toString();
    }
    
    public abstract String value2string();
    
    public Use getBeginUse() {
        return (Use)useList.getHead();
    }
    
    public Use getEndUse() {
        return (Use) useList.getTail();
    }
    
    public void removeUse(Use use) {
        use.removeFromList();
    }
    
    public HashSet<Value> getUserSet() {
        Use use = this.getBeginUse();
        HashSet<Value> res = new HashSet<>();
        while (use != null) {
            res.add(use.getUser());
            use = (Use) use.getNext();
        }
        return res;
    }

    public void replaceUseTo(Value to) {
        Use use = this.getBeginUse();//使用this的值
        //TODO：将使用this的值，变为to
        while (use != null) {
            Use nextUse = (Use) use.getNext();
            Instruction instrUseThis = use.getUser();
            instrUseThis.modifyUse(this, to);
            use = nextUse;
        }
    }

    public void replaceUseInRange(HashSet<BasicBlock> range, Value to) {
        Use use = this.getBeginUse();
        while (use != null) {
            Instruction user = use.getUser();
            if (range.contains(user.getParentBB())) {
                user.modifyUse(this, to);
            }
            use = (Use) use.getNext();
        }

    }

    @Override
    public String toString() {
        return value2string();
    }
}
