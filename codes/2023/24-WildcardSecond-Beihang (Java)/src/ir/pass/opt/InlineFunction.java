package ir.pass.opt;
import frontend.MyVisitor;
import ir.Value;
import ir.instr.Alloca;
import ir.instr.Br;
import ir.instr.Call;
import ir.instr.Instr;
import ir.instr.Load;
import ir.instr.Phi;
import ir.instr.Ret;
import ir.instr.Store;
import ir.pass.analyze.DomAnalyzer;
import ir.type.FloatType;
import ir.type.IntType;
import ir.type.Type;
import ir.type.VariLen;
import ir.type.VoidType;
import ir.value.Arg;
import ir.value.BasicBlock;
import ir.value.ConstNumber;
import ir.value.Module;
import ir.value.User;
import ir.value.Variable;
import ir.value.user.Function;
import tools.IRBuilder;
import tools.IrRegDispatcher;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;

public class InlineFunction {
    public  final LinkedHashMap<Function, ArrayList<Function>> callers; // key 调用 values
    public  final LinkedHashMap<Function, ArrayList<Function>> callees; // values 调用 key
    public  boolean changed; //不动点

    public InlineFunction() {
        this.callers = new LinkedHashMap<>();
        this.callees = new LinkedHashMap<>();
        this.changed = true;
    }

    public void run(Module module, MyVisitor visitor) throws CloneNotSupportedException {
        ArrayList<Function> inlineFuncs = new ArrayList<>(); // 内联函数的集合
        while (changed) {
            changed = false;
            newBiCallGraph(module); // 新建一个双向调用关系图

            for (Function func : module.functions.values()) { //遍历调用者
                // 指示当前函数是否可以被内联，评价标准是内联函数不能递归
                if (!func.blocks.isEmpty() && !func.getName().equals("main")) {  // 不是内建函数，且不是 main
                    if (!callers.get(func).contains(func)) { // func没有递归
                        inlineFuncs.add(func);
                    }
                }
            }
            for (Function inlineFunc : inlineFuncs) { // 进行一次内联
                inlineFunction(inlineFunc, module, visitor);
            }

            inlineFuncs.clear();
        }
    }

    public void newBiCallGraph(Module module) {
        callers.clear();
        callees.clear();
        for (Function func : module.functions.values()) {
            if (func.blocks.isEmpty()) {
                continue;
            }
            if (!callers.containsKey(func)) {
                ArrayList<Function> callerFuncs = new ArrayList<>();
                callers.put(func, callerFuncs);
            }
            for (BasicBlock block : func.blocks) {
                for (Value inst : block.getInsts()) {
                    if (inst instanceof Call) {
                        Function callee = ((Call) inst).getFunction();
                        if (callee.blocks.isEmpty()) {
                            continue;
                        }
                        if (!callees.containsKey(callee)) {
                            ArrayList<Function> calleeFuncs = new ArrayList<>();
                            callees.put(callee, calleeFuncs);
                        }
                        if (!callers.get(func).contains(callee))
                        {
                            callers.get(func).add(callee);
                        }
                        if (!callees.get(callee).contains(func))
                        {
                            callees.get(callee).add(func);
                        }
                    }
                }
            }
        }
    }

    private void inlineFunction(Function inlineFunc, Module module, MyVisitor visitor) throws CloneNotSupportedException {
        ArrayList<Call> calls = new ArrayList<>(); // 有调用inlineFunc的函数集合
        if (callees.containsKey(inlineFunc) && !callees.get(inlineFunc).isEmpty()) {
            changed = true; // 这里赋值为 true，已经进行了内联，所以还需要再分析一遍内联后的成果
            for (Function func : callees.get(inlineFunc)) { // 遍历所有调用 inlineFunc 的 call
                for (BasicBlock block : func.blocks) {
                    ArrayList<Call> callsInBlock = new ArrayList<>();
                    for (Value inst : block.getInsts()) {
                        if (inst instanceof Call && ((Call) inst).getFunction().getName().equals(inlineFunc.getName())) {
                            callsInBlock.add((Call) inst);
                        }
                    }
                    // TODO 如果 sort 数据点最后时间还是太长，可以把下面的开关打开
//                    if (callsInBlock.size() > 3) {
//                        continue;
//                    }
                    for (Call inst : callsInBlock) {
                        calls.add(inst);
                    }
                }
            }
            for (Call call : calls) { // 对所有的 call 进行替换
                replaceCall(call, module, visitor);
            }

            for (Function func : callees.get(inlineFunc)) { // 去掉调用关系
                callers.get(func).remove(inlineFunc);
                callers.get(func).addAll(callers.get(inlineFunc));
            }
            callees.get(inlineFunc).clear();
        }
    }

    private void replaceCall(Call call, Module module, MyVisitor visitor) throws CloneNotSupportedException {
        IrRegDispatcher irRegDispatcher = new IrRegDispatcher();
        Function func = (Function) call.getParent().getUsers().get(0);//call所在函数
        Function calleeFunc = call.getFunction();//目标待内联函数
        BasicBlock block = call.getParent();//call所在block
        Type retType = calleeFunc.getRetType();//目标待内联函数的返回值类型

        // ======================== 这里是为了将 call 指令所在的块分割成两半 ==========================
        // 在当前块（也就是 call 在的那个块）之后再建一个块，用于存放 call 之后的指令
        BasicBlock nextBlock = visitor.buildNewBlock(
                new BasicBlock(irRegDispatcher.allocId(), block.getFatherFunction(), false),
                null, true, true, false);
        func.insertAfter(block, nextBlock);
        boolean nextInstsFlag = false;
        for (Value instr : block.getInsts()) { // 遍历当前块
            if (!nextInstsFlag && instr.equals(call)) {
                nextInstsFlag = true;
                continue;
            }
            if (nextInstsFlag) { // 将 call 后的指令插入 nextBlock
                nextBlock.addValue(instr);
                ((Instr) instr).setParent(nextBlock);
            }
        }
         for (int i = block.getInsts().size() - 1; i >= 0; i--) {
             if (block.getInsts().get(i).equals(call)) {
                 break;
             } else {
                 block.deletInstr(block.getInsts().get(i));
             }
         }

        // 修改后继节点 phi 指令
        for (BasicBlock succBlock : block.getSuccs()) {
            for (Value succInstr : succBlock.getInsts()) {
                if (succInstr instanceof Phi && ((Phi) succInstr).values.containsKey(block)) { // 说明是 phi
                    Value value = ((Phi) succInstr).values.get(block);
                    ((Phi) succInstr).addBlock2Value(nextBlock, value);
                    ((Phi) succInstr).values.remove(block);
                }
            }
        }

        // ==================================== 分割结束 =====================================
        Function copyFunction = calleeFunc.clone(module, new LinkedHashMap<>(),
                module.globals, module.constGlobals);
//         // 处理函数参数, 处理所有操作数
//        for (BasicBlock block1 : copyFunction.blocks) {
//            for (Value instr : block1.getInsts()) {
//                assert instr instanceof Instr;
//                HashMap<Value, Value> needReplace = new HashMap<>();
//                for(var op : ((Instr) instr).getOperands()){
//                    if(op == null)continue;
//                    if(op instanceof Arg){
//                        needReplace.put(op, call.getOP(Integer.parseInt(op.name)));
//                    }
//                }
//                for(var oriop : needReplace.keySet()){
//                    ((Instr) instr).replaceOp(oriop, needReplace.get(oriop));
//                }
////                if (instr instanceof Store && ((Store) instr).getVal() instanceof Arg) {// TODO: 2023/8/9 ?不是store的情况也会用到arg，比如getelementptr，比如所有指令
////                    Value curArg = ((Store) instr).getVal();
////                    ((Store) instr).replaceOp(((Store) instr).getVal(),call.getOP(Integer.parseInt(curArg.name)));
////                }
//            }
//        }
        // 处理函数参数
        for (BasicBlock block1 : copyFunction.blocks) {
            for (Value instr : block1.getInsts()) {
                for (Value operand: ((Instr) instr).getOperands()) {
                    if (operand instanceof Arg) {
                        for (Arg arg : copyFunction.getArgv2()) {
                            if (arg.name.equals(operand.name)) {
                                int index = copyFunction.getArgv2().indexOf(arg);
                                ((Instr) instr).replaceOp(operand,call.getOP(index));
                                if(instr instanceof Phi){
                                    ArrayList<BasicBlock> needReplace = new ArrayList<>();
                                    for(var comeblock : ((Phi) instr).values.keySet()){
                                        if(((Phi) instr).values.get(comeblock).equals(operand)){
                                            needReplace.add(comeblock);
                                        }
                                    }
                                    for (BasicBlock basicBlock : needReplace) {
                                        ((Phi) instr).values.put(basicBlock, call.getOP(index));
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // 让 curBlock 跳掉入口块, 用br直接跳到目标函数的入口
        Br br = IRBuilder.buildBr(null, copyFunction.blocks.get(0),null,block);
        block.addValue(br);
        br.setParent(block);

        ArrayList<Ret> rets = new ArrayList<>();
        // 收集所有的 ret 指令
        for (BasicBlock block1 : copyFunction.blocks) {
            for (Value inst : block1.getInsts()) {
                if (inst instanceof Ret) {
                    rets.add((Ret) inst);
                }
            }
        }

        // 改变 copyfunction 里面的 block 的parent 为 func
        for (BasicBlock block1 : copyFunction.blocks) {
            block1.getUsers().clear();
            block1.getUsers().add(0, func);
        }
        // TODO: 2023/7/24 在phi之前进行该优化吧，不用处理phi了，另外返回值还有可能是float，需要补充一下
        // TODO 被内联的函数原来是通过返回值来和调用它的程序互动的，内联后把两边“接上”
        // 创建一个临时的匿名的变量，用于存储一个 call 指令的返回值，
        // 被内联函数的所有 return 转换为对这个临时变量的存，
        // 对内联前的 call 指令的使用转换为对这个临时的匿名变量的使用。
        // 如果是有返回值的，可以用 load 指令
        if (retType instanceof IntType || retType instanceof FloatType) {
            for (Ret ret : rets)
            {
                call.replaceAllUsers(ret.getOP(0));

                Br br1 = IRBuilder.buildBr(null, nextBlock,null,ret.getParent());
                ret.getParent().insertBefore(ret, br1); // 最终 ret 在 br 前面

                ret.clearOperands();
                ret.getParent().deletInstr(ret);
                ret.delete();
            }
        } else if (retType instanceof VoidType) {
            for (Ret ret : rets) {
                Br br1 = IRBuilder.buildBr(null, nextBlock,null,ret.getParent());
                ret.getParent().insertBefore(ret, br1); // 最终 ret 在 br 前面

                ret.clearOperands();
                ret.getParent().deletInstr(ret);
                ret.delete();
            }
        }


        for (BasicBlock block1 : copyFunction.blocks) { // 清除重新插入
            func.deletBlock(block1);
            func.insertBefore(nextBlock, block1); // 最终 block1 在 nextblock 前面
            block1.setParent(call.getParent());
        }

        // 去掉 call
        call.clearOperands();
        block.deletInstr(call);
        call.delete();

        // 找到需要插入的那个指令： br 或 第一个Call
        Value head = func.blocks.get(0).getInsts().get(func.blocks.get(0).getInsts().size() - 1);
        for (Value intr : func.blocks.get(0).getInsts()) {
            if (intr instanceof Call) {
                head = intr;
                break;
            }
        }
        // 把 alloca 提前
        for (BasicBlock block1 : copyFunction.blocks) {
            for (int i = block1.getInsts().size() - 1; i >= 0; i--) {
                Value inst = block1.getInsts().get(i);
                if (inst instanceof Alloca) {
                    func.blocks.get(0).insertBefore(head, inst);
                    if (((Alloca)inst).getAllocType() instanceof IntType
                            || ((Alloca)inst).getAllocType() instanceof FloatType) {
                        Store store = new Store(inst, new ConstNumber(0), block);
                        func.blocks.get(0).insertBefore(head, store);
                    }
                    block1.deletInstr(inst);
                }
            }
        }
        DomAnalyzer.calPredSuccBB(module);
    }
}
