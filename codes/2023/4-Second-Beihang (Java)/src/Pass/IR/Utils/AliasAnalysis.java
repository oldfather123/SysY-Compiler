package Pass.IR.Utils;

import IR.Use;
import IR.Value.Argument;
import IR.Value.GlobalVar;
import IR.Value.Instructions.AllocInst;
import IR.Value.Instructions.GepInst;
import IR.Value.Instructions.LoadInst;
import IR.Value.Instructions.StoreInst;
import IR.Value.Value;

public class AliasAnalysis {

    //  获取某个数组value的根定义，要么是alloc指令(局部变量，局部数组或参数数组)，要么是全局数组
    public static Value getRoot(Value pointer){
        Value iter = pointer;

        //  这里loadInst是因为有可能为局部变量或参数数组(本质上最初定义也是allocInst)
        //  我们一路循环直到iter是AllocInst或者为全局数组
        while (iter instanceof GepInst || iter instanceof LoadInst){
            if (iter instanceof GepInst) {
                iter = ((GepInst) iter).getTarget();
            } else {
                iter = ((LoadInst) iter).getPointer();
            }
        }

        return iter;
    }

    public static boolean isGlobal(Value array){
        return array instanceof GlobalVar;
    }
    public static boolean isParam(Value array){
        if(array instanceof AllocInst allocInst){
            return allocInst.getAllocType().isPointerType();
        }
        return false;
    }

    public static boolean isMaySame(Value pointer1, Value pointer2){
        if(pointer1.equals(pointer2)) return true;
        Value root1 = getRoot(pointer1);
        Value root2 = getRoot(pointer2);
        if(!root1.equals(root2)){
            return false;
        }
        return true;
    }

    public static boolean isLocal(Value array){
        return !isGlobal(array) && !isParam(array) && !(array instanceof Argument);
    }

}