package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.type.VoidType;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.GlobalVariable;
import cn.edu.bit.newnewcc.ir.value.Instruction;

import java.util.HashMap;
import java.util.Map;

public class IrNamingPass {

    /**
     * 名称分配器
     * <p>
     * 为局步变量和全局变量自动分配名字
     */
    public static class NameAllocator {

        /**
         * 局步变量分配状态
         */
        private final Map<Function, Integer> lvAllocateState = new HashMap<>();

        /**
         * 为局部变量获取一个名字
         *
         * @param function 局部变量所属的函数
         * @return 一个纯数字的名字
         */
        public String getLvName(Function function) {
            int id = lvAllocateState.getOrDefault(function, -1) + 1;
            lvAllocateState.put(function, id);
            return String.valueOf(id);
        }

        /**
         * 全局变量分配状态
         */
        private int gvAllocateState = 0;

        /**
         * 为全局变量获取一个名字
         *
         * @return 一个纯数字的名字
         */
        public String getGvName() {
            return String.valueOf(gvAllocateState++);
        }

    }

    private IrNamingPass() {
    }

    private final NameAllocator nameAllocator = new NameAllocator();

    private void nameLocalVariable(Function function, Value value) {
        if (value.getType() != VoidType.getInstance()) {
            if (isTemporaryName(value.getValueName())) {
                value.setValueName(nameAllocator.getLvName(function));
            }
        }
    }

    private void nameBlock(Function function, BasicBlock block) {
        nameLocalVariable(function, block);
        for (Instruction instruction : block.getInstructions()) {
            nameLocalVariable(function, instruction);
        }
    }

    private void nameFunction(Function function) {
        if (isTemporaryName(function.getValueName())) {
            function.setValueName(nameAllocator.getGvName());
        }
        for (Function.FormalParameter formalParameter : function.getFormalParameters()) {
            nameLocalVariable(function, formalParameter);
        }
        for (BasicBlock block : function.getBasicBlocks()) {
            nameBlock(function, block);
        }
    }

    private void nameModule(Module module) {
        // 命名全局变量
        for (GlobalVariable globalVariable : module.getGlobalVariables()) {
            if (isTemporaryName(globalVariable.getValueName())) {
                globalVariable.setValueName(nameAllocator.getGvName());
            }
        }
        // 命名函数
        for (Function function : module.getFunctions()) {
            nameFunction(function);
        }
    }

    private static boolean isTemporaryName(String name) {
        if (name == null) return true;
        for (char c : name.toCharArray()) {
            if (!('0' <= c && c <= '9')) return false;
        }
        return true;
    }

    public static void runOnModule(Module module) {
        new IrNamingPass().nameModule(module);
    }

}
