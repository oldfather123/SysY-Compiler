package backend.risc_v.optimize.tool;

import backend.ir.entity.Function;
import backend.ir.entity.LiteralConst;
import backend.ir.entity.Program;
import backend.ir.entity.Value;
import backend.ir.entity.insts.MoveInst;
import backend.ir.handler.IRParser;
import frontend.semantic.symbol.IRBasicType;

import java.io.BufferedWriter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * 主要用于phi消除，消除后才能转换为mips指令
 * 主要是封装了pc转move的情况
 */
public class DeletePhi {
    private static Program program;

    /**
     * 消除phi的启动类
     */
    public static void deletePhi(Program program){
        DeletePhi.program = program;
        program.deletePhi();
    }

    /**
     * 生成临时变量的temp move
     * 由于可能存在循环和冲突，所以需要引入临时寄存器，保证串行执行后和并行一致
     */
    public static List<MoveInst> generateMoveTemp(Function function, List<MoveInst> moveInsts){
        List<MoveInst> moveCycle = generateMoveCycle(function, moveInsts);
        List<MoveInst> moveSameRegister = generateMoveSameRegister(function, moveInsts);
        moveCycle.addAll(moveSameRegister);
        return moveCycle;
    }

    /**
     * 生成循环的move指令集合
     * 比如： move a, b;  move c, a;，这种串行化后会改变PC的语义，所以需要引入临时寄存器
     * 首先，检查每个move指令之后的指令，如果这个指令的dest是某个move的src，那么存在循环赋值的问题
     * 需要增加中间变量，即将所有source使用dest的move指令全部替换为临时寄存器，
     * 最后在moveList开头添加临时寄存器的指令
     */
    private static List<MoveInst> generateMoveCycle(Function function, List<MoveInst> moveInsts){
        List<MoveInst> moveCycle = new ArrayList<>();
        Map<Value,Boolean> visit =  new HashMap<>();
        for(int i = 0; i < moveInsts.size(); i++){
            Value target = moveInsts.get(i).getTarget();
            if(! (target instanceof LiteralConst) &&
                    (!visit.containsKey(target) || !visit.get(target))){
                visit.put(target, true);
                boolean isCycle = false;
                for(int j = i+1; j < moveInsts.size(); j++){
                    if(moveInsts.get(j).getSource().equals(target)){
                        isCycle = true;
                        break;
                    }
                }
                if(isCycle){
                    //说明存在循环，需要破除, 添加临时寄存器
                    Value temp = new Value(
                            "temp_"+target.getName(),
                            target.getBasicType()) {
                        @Override
                        public void printAssembly(BufferedWriter fout) {}
                        @Override
                        public void printName(BufferedWriter fout) {}
                        @Override
                        public void printUse(BufferedWriter fout) {}
                    };
                    for(MoveInst moveInst : moveInsts){
                        if(moveInst.getSource().equals(target)){
                            moveInst.setSource(temp);
                        }
                    }
                    MoveInst newMoveInst = new MoveInst(
                            "var_move_"+ IRParser.indexInFunction++,
                            IRBasicType.NONE,
                            moveInsts.get(0).getBasicBlock(),
                            target,
                            temp
                    );
                    //将上面这条转移到temp_value的语句移动进cycle_instructions最开始的位置
                    moveCycle.addFirst(newMoveInst);
                }
            }
        }
        return moveCycle;
    }

    /**
     * 用于生成共享寄存器的Move指令集合
     * 1.检查该指令之前的所有指令，
     * 如果source对应的reg同时是某一个move的dst的reg，那么存在寄存器冲突的问题
     * 2. 如果出现了寄存器冲突的情况，我们需要增加中间变量，
     * 将所有使用作为dest的move指令，改为临时寄存器，并在开头添加寄存器
     */
    private static List<MoveInst> generateMoveSameRegister(Function function, List<MoveInst> moveInsts){
        //TODO: 这个需要根据后端的寄存器分配来实现。
        // 它看的是两个虚拟寄存器是否分到了同一个寄存器
        // 如果是同一个，就需要拆
        return new ArrayList<>();
    }
}
