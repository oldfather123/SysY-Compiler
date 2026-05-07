package backend.ir.optimize.tool;

import backend.ir.entity.*;
import backend.ir.entity.insts.*;
import backend.ir.handler.IRParser;
import frontend.semantic.symbol.IRBasicType;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

public class MemToReg {
    //生成LLVM IR的顶级模块
    static private Program program;
    //存储了每个函数的第一个基本块
    static private BasicBlock firstBlock;
    //存储了所有使用该alloca指令的load指令，以及phi指令，维护使用链
    static List<Inst> loadInstructions;
    //存储了所有使用该alloca指令的store指令，以及phi指令，维护定义链
    static List<Inst> storeInstructions;
    //存储所有使用该指令的基本块
    static List<BasicBlock> useBlocks;
    //存储所有定义该指令的基本块
    static List<BasicBlock> defineBlocks;
    //存储phi指令的值的
    static List<Value> phiValues;
    //当前的alloca指令
    static Inst nowAllocateInst;

    public static BasicBlock getFirstBlock() {return firstBlock;}
    public static void setFirstBlock(BasicBlock firstBlock) {
        MemToReg.firstBlock = firstBlock;
    }

    /**
     * 启动MemToReg
     */
    public static void Mem2Reg(Program program){
        MemToReg.program = program;
        List<Function> functions = program.getFunctions();
        for(Function function:functions){
            function.insertPhiInst();
        }
    }

    /**
     * 根据指令，重新配置相关参数
     */
    public static void resetByInst(Inst inst){
        //清空上述的数组，进行重置
        loadInstructions = new ArrayList<>();
        storeInstructions = new ArrayList<>();
        useBlocks = new ArrayList<>();
        defineBlocks = new ArrayList<>();
        phiValues = new ArrayList<>();
        nowAllocateInst = inst;
        List<User> users = inst.getUsers();
        for(User user: users){
            if(user instanceof Inst instr){
                //查看哪些指令使用了alloca，分别将它们加入到对应的数组
                BasicBlock basicBlockParent = instr.getBasicBlock();
                if(instr instanceof LoadInst && basicBlockParent.isExist()){
                    //这个基本块没有被消除，并且指令是load
                    loadInstructions.add(instr);
                    if(!defineBlocks.contains(basicBlockParent)){
                        useBlocks.add(basicBlockParent);
                    }
                } else if (instr instanceof StoreInst && basicBlockParent.isExist()) {
                    storeInstructions.add(instr);
                    if(!defineBlocks.contains(basicBlockParent)){
                        defineBlocks.add(basicBlockParent);
                    }
                }
            }
        }
    }

    /**
     * 插入phi指令，要求在每个基本块最前面
     * 根据课件给出的算法
     * for v : 原程序中的变量 do
         F ←{}   ▷已插入ϕ函数的基本块集合
         W←{}    ▷v 的定义（包括新插入的ϕ函数）所在的基本块集合
         for d ∈ v 的定义Defs(v) do
            W←W∪{d所在的基本块}
         while W={} do
            从W中选择并删除一个基本块X
            for Y : DF(X) 中的基本块 do
                if Y / ∈ F then
                 在基本块Y的入口处插入ϕ函数v←ϕ(...)
                  F ←F∪{Y}
                  if Y / ∈ Defs(v) then
                    W←W∪{Y}
     */
    public static void insertPhiInst(){
        List<BasicBlock> F = new ArrayList<>();
        List<BasicBlock> W = new ArrayList<>(defineBlocks);

        while (!W.isEmpty()) {
            BasicBlock x = W.removeLast(); // 从末尾取出
            List<BasicBlock> DFX = x.getDominateEdge();

            for (BasicBlock y : DFX) {
                if (!F.contains(y)) {
                    // 1. 将y加入F集合
                    F.add(y);

                    // 2. 在基本块y的入口处创建空的phi指令
                    PHIInst phiInstruction = new PHIInst(
                            "%var_phi_"+ IRParser.indexInFunction,
                            IRBasicType.I32,
                            y,
                            y.getInBlocks()
                    );
                    IRParser.indexInFunction++;

                    // 3. 将phi指令插入到基本块开头
                    y.addInst(phiInstruction, 0);

                    // 4. 记录phi指令
                    loadInstructions.add(phiInstruction);
                    storeInstructions.add(phiInstruction);

                    // 5. 如果y不在定义块中，加入工作列表
                    if (!defineBlocks.contains(y)) {
                        W.add(y);
                    }
                }
            }
        }
    }

    public static void renameVar(BasicBlock... basicBlocks){
        BasicBlock basicBlock = basicBlocks.length > 0 ? basicBlocks[0] : firstBlock;
        int cnt = removeInst(basicBlock);
        //获取后继的block
        //注意，不能直接获取支配树的后续，因为存在一种情况：
        //1->2->3, 1->3, 这种情况1不是直接支配3，但是3确实是1的后继
        //在基本块3中，很可能有phi指令，所以需要赋值，那么应当获得的是流图的后继
        List<BasicBlock> outBlocks = basicBlock.getOutBlocks();
        for(BasicBlock outBlock : outBlocks){
            List<Inst> outInsts = outBlock.getInstructions();
            for(Inst inst : outInsts){
                if(inst instanceof PHIInst phiInst && loadInstructions.contains(inst)){
                    Value value = null;
                    if(!phiValues.isEmpty()){
                        value = phiValues.getLast();
                    }else{
                        value = new LiteralConst(0, IRBasicType.I32, false);
                    }
                    phiInst.changeValue(value, basicBlock);
                }
            }
        }
        //获取支配树的子节点，也进行遍历
        List<BasicBlock> childDominateBlocks = basicBlock.getChildDominateBlocks();
        if(childDominateBlocks != null){
            for(BasicBlock childDominateBlock : childDominateBlocks){
                renameVar(childDominateBlock);
            }
        }
        //弹出
        for(int i = 0;i<cnt;i++){
            phiValues.removeLast();
        }
    }

    public static int removeInst(BasicBlock basicBlock){
        int cnt = 0;
        List<Inst> insts = basicBlock.getInstructions();
        List<Value> blockUsees = basicBlock.getUsees();
        Iterator<Inst> it = insts.iterator();

        while (it.hasNext()) {
            Inst inst = it.next();

            if (inst == nowAllocateInst) {
                // 是alloca指令，直接删除
                it.remove();
                blockUsees.remove(inst);
                inst.deleteAllUsees();
            }
            else if (inst instanceof StoreInst &&
                    storeInstructions.contains(inst)) {
                // 是store指令，处理并删除
                cnt++;
                Value value = inst.getUsees().get(0);
                phiValues.add(value);
                it.remove();
                blockUsees.remove(inst);
                inst.deleteAllUsees();
            }
            else if (inst instanceof LoadInst &&
                    loadInstructions.contains(inst)) {
                // 是load指令，替换为值并维护定义使用链
                Value value = null;
                if (!phiValues.isEmpty()) {
                    value = phiValues.getLast();
                }else{
                    value = new LiteralConst(0, IRBasicType.I32, false);
                }
                inst.replaceAllUser(value);
                it.remove();
                blockUsees.remove(inst);
            }
            else if (inst instanceof PHIInst &&
                    storeInstructions.contains(inst)) {
                // 是phi指令，记录但不删除
                cnt++;
                phiValues.add(inst);
            }
            // 其他情况不处理，继续迭代
        }
        return cnt;
    }
}
