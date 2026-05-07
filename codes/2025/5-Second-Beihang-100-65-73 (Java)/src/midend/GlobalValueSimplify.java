package midend;

import frontend.ir.Value;
import frontend.ir.constant.IRConst;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.memop.AllocaInstr;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.symbol.SymTab;
import frontend.ir.symbol.Symbol;

import java.util.ArrayList;
import java.util.List;

/**
 * 全局值简化:
 *  1. 删除对未使用过的全局变量的定义
 *  2. 将只定义了一次（只有初始化那次）的全局变量直接用初值替代
 *  3. 将只在主函数中被引用的全局变量变成局部变量（因为只有主函数可以确定只被调用一次）
 */
public class GlobalValueSimplify {
    public static void execute(SymTab globalSymTab) {
        if (globalSymTab == null) {
            throw new NullPointerException();
        }
        
        for (Symbol glbObj : globalSymTab.getObjectList()) {
            Value initVal = glbObj.getInitVal();
            if (!(initVal instanceof IRConst)) {
                continue;   // 数组和字符串是处理不了的
            }
            
            if (deleteIfNoUsage(glbObj)) {
                continue;
            }
            if (replaceIfOnlyDefOnce(glbObj, (IRConst) initVal)) {
                continue;
            }
            tryToLocalize(glbObj, (IRConst) initVal);
        }
    }
    
    /**
     * 尝试进行局部化
     */
    private static void tryToLocalize(Symbol glbObj, IRConst initVal) {
        if (glbObj == null) {
            throw new NullPointerException();
        }
        
        Function parentFunc = null;
        for (Value user : glbObj.getPointer().getUserList()) {
            if (user instanceof Instruction userIns) {
                Function curFunc = userIns.getParentBB().getParentFunc();
                if (!curFunc.getName().equals("main")) {
                    return; // 只在主函数中用到的变量才可以局部化
                }
                parentFunc = curFunc;
            } else {
                throw new RuntimeException("只有指令能使用指针");
            }
        }
        
        if (parentFunc == null) {
            throw new RuntimeException("这变量要是没用过不应该走到这一步");
        }
        BasicBlock headBB = parentFunc.getFirstBlk();
        
        AllocaInstr newAlloca = new AllocaInstr(glbObj.getType(), parentFunc.getAndAddRegIdx(), headBB);
        headBB.insertInsToHead(newAlloca);
        
        AllocaInstr lastAlloca = newAlloca;
        while (lastAlloca.getNext() instanceof AllocaInstr nextAlloca) {
            lastAlloca = nextAlloca;
        }
        StoreInstr storeInitVal = new StoreInstr(initVal, newAlloca, headBB);
        storeInitVal.insertAfter(lastAlloca);
        
        glbObj.getPointer().replaceUseWith(newAlloca);
        glbObj.abandon();
    }
    
    /**
     * 只定义一次（只有初始化那次）就替换，成功返回 true 否则返回 false
     */
    private static boolean replaceIfOnlyDefOnce(Symbol glbObj, IRConst initVal) {
        if (glbObj == null) {
            throw new NullPointerException();
        }
        
        for (Value user : glbObj.getPointer().getUserList()) {
            if (user instanceof StoreInstr) {
                return false;
            }
        }
        // 到这说明没有 store 过
        List<Value> userList = new ArrayList<>(glbObj.getPointer().getUserList());
        for (Value user : userList) {
            if (user instanceof LoadInstr load) {
                load.replaceUseWith(initVal);
                load.removeFromList();
            } else {
                throw new RuntimeException("我觉得这里只应该有 load");
            }
        }
        glbObj.abandon();
        return true;
    }
    
    /**
     * （可能定义过）没用过就删，删了返回 true 否则返回 false
     */
    private static boolean deleteIfNoUsage(Symbol glbObj) {
        if (glbObj == null) {
            throw new NullPointerException();
        }
        
        for (Value user : glbObj.getPointer().getUserList()) {
            if (user instanceof LoadInstr) {
                return false;
            }
        }
        // 到这说明没有 load 过
        List<Value> userList = new ArrayList<>(glbObj.getPointer().getUserList());
        for (Value user : userList) {
            user.removeFromList();
        }
        glbObj.abandon();
        return true;
    }
}
