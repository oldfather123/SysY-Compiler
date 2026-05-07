package frontend.ir.structure;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.instr.otherop.MoveInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.objecttype.TVoid;
import frontend.ir.objecttype.Type;
import frontend.ir.objecttype.arithmetic.TChar;
import frontend.ir.objecttype.arithmetic.TFloat;
import frontend.ir.objecttype.arithmetic.TInt;
import frontend.ir.objecttype.derived.TPointer;
import midend.Analysis.BranchForeseeManager;
import midend.Analysis.LoopInfo;
import midend.Analysis.SCEV.SCEVManager;
import midend.FuncMemorizeV4;
import midend.FunctionInfo;
import midend.FunctionInfoCollector;
import util.CustomList;

import java.util.*;

public class Function extends Value {
    private List<FParam> fParams;
    private CustomList basicBlocks;
    private final String name;
    private int curRegIndex;
    private int curBlkIndex;
    private int callCnt;    // 记录一下这个函数被调用多少次
    private final List<CallInstr> callInstrList; // 调用此函数的Call指令
    private boolean isTailRecursive = false; // 是否是尾递归函数
    private final HashMap<Function, Integer> calleeMap; // 请注意，这个东西现在一定准确
    private boolean isParallelLoopBody = false;
    private Set<LoopInfo> ancientLoopInfo;
    private SCEVManager scevManager;
    private Map<BasicBlock, List<BasicBlock>> blk2domTreeChildren;
    private HashMap<Value, Value> cloneInfo;
    private boolean memorized;
    private BranchForeseeManager branchForeseeManager;

    public Function(Type returnType, String name) {
        super(returnType);
        this.fParams = new ArrayList<>();
        this.basicBlocks = new CustomList(this);
        this.name = name;
        this.curRegIndex = 0;
        this.curBlkIndex = 0;
        this.callCnt = 0;
        this.calleeMap = new HashMap<>();
        this.ancientLoopInfo = new HashSet<>();
        this.callInstrList = new ArrayList<>();
        this.scevManager = null;
        this.blk2domTreeChildren = new HashMap<>();
        this.memorized = false;
    }

    public BranchForeseeManager getBranchForeseeManager() {
        return branchForeseeManager;
    }

    public void setBranchForeseeManager(BranchForeseeManager branchForeseeManager) {
        this.branchForeseeManager = branchForeseeManager;
    }

    public boolean isMemorized() {
        return memorized;
    }

    public boolean canBeMemorized() {
        FunctionInfo functionInfo = FunctionInfoCollector.getFuncInfo(this);
        // 如果没有返回值不能记忆化
        if (this.getType() instanceof TVoid) {
            return false;
        }
        // 没有参数的函数不能记忆化
        if (this.fParams.isEmpty()) {
            return false;
        }
        // 如果传了指针参数则不可以记忆化
        // todo:对数组参数做哈希比较麻烦，暂时没加
        for (FParam param : this.fParams) {
            if (param.getType() instanceof TPointer) {
                return false;
            }
        }
        if (fParams.size() > FuncMemorizeV4.MAX_PARAMS && this.getInstrNum() < FuncMemorizeV4.EXEMPT_INSTR_NUM) {
            return false;
        }
        // 只有调用自身多次（考虑新增：或者被外界重复调用）做记忆化才有意义
        if (this.calleeMap.getOrDefault(this, 0) <= 1/* && this.calledCnt - this.recursiveCallCnt <= 2*/) {
            return false;
        }
        if(!functionInfo.canBeMemorizedBase()) {
            return false;
        }
        if (!functionInfo.isExternRead()) {
            return true;
        } else {
            return this.isCalledOnlyOnceExceptSelf();
        }
    }

    public boolean isCalledOnlyOnceExceptSelf() {
        if (!this.isRecursive()) {
            // todo ：限制了只有递归能做强化记忆
            return false;
        }
        CallInstr mainCalled = null;
        for (CallInstr callInstr : callInstrList) {
            if (callInstr.getParentBB().getParentFunc() == this) {
                continue;
            } else if (callInstr.getParentBB().getParentFunc().getName().equals("main")) {
                if (mainCalled == null) {
                    mainCalled = callInstr;
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }
        if (mainCalled != null) {
            if (mainCalled.getParentBB().getLoopDepth() == 0) {
                return true;
            }
        }
        return false;
    }

    public void setMemorized(boolean memorized) {
        this.memorized = memorized;
    }

    public int getInstrNum() {
        int sumInstr = 0;
        for (CustomList.Node basicBlock : basicBlocks) {
            sumInstr += ((BasicBlock) basicBlock).getInstrNum();
        }
        return sumInstr;
    }

    public void computeDomTreeChildren() {
        blk2domTreeChildren.clear();
        for (BasicBlock block : this.getBasicBlockList()) {
            BasicBlock idom = block.getIDom();
            if (idom != null) {
                blk2domTreeChildren.computeIfAbsent(idom, k -> new ArrayList<>()).add(block);
            }
        }
    }

    public List<BasicBlock> getDomTreeChildren(BasicBlock block) {
        return blk2domTreeChildren.getOrDefault(block, new ArrayList<>());
    }


    public void setCloneInfo(HashMap<Value, Value> cloneInfo) {
        this.cloneInfo = cloneInfo;
    }

    public HashMap<Value, Value> getCloneInfo() {
        return cloneInfo;
    }

    public void setSCEVManager(SCEVManager scevManager) {
        this.scevManager = scevManager;
    }

    public SCEVManager getSCEVManager() {
        return scevManager;
    }

    public void setFParams(List<FParam> fParams) {
        this.fParams = fParams;
        this.ancientLoopInfo = new HashSet<>();
    }

    public Set<LoopInfo> getAncientLoopInfo() {
        return ancientLoopInfo;
    }

    public void setAncientLoopInfo(Set<LoopInfo> ancientLoopInfo) {
        this.ancientLoopInfo = ancientLoopInfo;
    }

    public void appendFParam(FParam fParam) {
        this.fParams.add(fParam);
    }

    public List<FParam> getFParams() {
        return fParams;
    }

    public BasicBlock getFirstBlk() {
        return (BasicBlock) this.basicBlocks.getHead();
    }

    public List<BasicBlock> getBasicBlockList() {
        List<BasicBlock> bbs = new ArrayList<>();
        for (CustomList.Node node : basicBlocks) {
            if (!(node instanceof BasicBlock basicBlock)) {
                throw new RuntimeException("基本块列表里只能有基本块");
            }
            bbs.add(basicBlock);
        }
        return bbs;
    }

    public int getAndAddRegIdx() {
        return this.curRegIndex++;
    }

    public int getAndAddBlkIdx() {
        return this.curBlkIndex++;
    }

    public void appendBasicBlock(BasicBlock newBB) {
        this.basicBlocks.addToTail(newBB);
        newBB.setParentFunc(this);
    }

    public String getName() {
        return name;
    }

    public void rearrangeAlloca() {
        List<Instruction> allocaList = new ArrayList<>();
        for (CustomList.Node node : basicBlocks) {
            allocaList.addAll(((BasicBlock) node).popAllAlloca());
        }
        ((BasicBlock) basicBlocks.getHead()).insertInsListToHead(allocaList);
    }

    public void addCallCnt() {
        this.callCnt++;
    }

    public void decCallCnt() {
        this.callCnt--;
    }

    public int getCallCnt() {
        return this.callCnt;
    }

    public HashMap<Function, Integer> getCalleeMap() {
        return this.calleeMap;
    }

    public void addCallee(Function callee) {
        if (!this.calleeMap.containsKey(callee)) {
            this.calleeMap.put(callee, 1);
        } else {
            this.calleeMap.put(callee, this.calleeMap.get(callee) + 1);
        }
    }

    public void addCall(CallInstr call) {
        this.callInstrList.add(call);
    }

    public void delCall(CallInstr call) {
        this.callInstrList.remove(call);
    }

    public List<CallInstr> getCallInstrList() {
        return callInstrList;
    }

    public boolean isRecursive() {
        return this.calleeMap.containsKey(this);
    }

    public boolean isParallelLoopBody() {
        return isParallelLoopBody;
    }

    public void setIsParallelLoopBody(boolean isParallelLoopBody) {
        this.isParallelLoopBody = isParallelLoopBody;
    }

    @Override
    public String value2string() {
        return "@" + name;
    }

    @Override
    public void removeFromList() {
        for (BasicBlock bb : this.getBasicBlockList()) {
            bb.removeFromListClear();
        }
        super.removeFromList();
    }

    public boolean isTailRecursive() {
        return isTailRecursive;
    }

    public void setIsTailRecursive() {
        HashSet<Instruction> selfCallSet = new HashSet<>();
        for (CustomList.Node BasicBlockNode : this.basicBlocks) {
            BasicBlock bb = (BasicBlock) BasicBlockNode;
            for (CustomList.Node insNode : bb.getInstrList()) {
                if (insNode instanceof CallInstr callInstr && callInstr.getCallee() == this) {
                    selfCallSet.add((Instruction) insNode);
                }
            }
        }
        if (selfCallSet.isEmpty()) {
            this.isTailRecursive = false;
            return;
        }
        for (Instruction callInstr : selfCallSet) {
            /*
             * 允许两种情况：call -> move -> jump, 对应有返回值的尾调用
             * 或者 call -> jump, 对应无返回值的尾调用
             * 其他情况均视为非尾递归
             */
            Instruction nextInstr = (Instruction) callInstr.getNext();
            if (!(nextInstr instanceof MoveInstr || nextInstr instanceof JumpInstr)) {
                this.isTailRecursive = false;
                return;
            }
            if (nextInstr instanceof MoveInstr moveInstr) {
                if (!(moveInstr.getNext() instanceof JumpInstr jumpInstr)) {
                    this.isTailRecursive = false;
                    return;
                }
                if (!jumpInstr.getTarget().equals(this.basicBlocks.getTail())) {
                    this.isTailRecursive = false;
                    return;
                }
            }
            if (nextInstr instanceof JumpInstr jumpInstr) {
                if (!jumpInstr.getTarget().equals(this.basicBlocks.getTail())) {
                    this.isTailRecursive = false;
                    return;
                }
            }

        }
        this.isTailRecursive = true;
    }

    public List<BasicBlock> getTopoSortExludingLatch() {
        List<BasicBlock> bbList = getBasicBlockList();

        // 用身份语义更稳
        IdentityHashMap<BasicBlock, Integer> indeg = new IdentityHashMap<>();
        IdentityHashMap<BasicBlock, List<BasicBlock>> adj = new IdentityHashMap<>();

        // 初始化
        for (BasicBlock b : bbList) {
            indeg.put(b, 0);
            adj.put(b, new ArrayList<>());
        }

        // 建图：所有 CFG 边都加入，但对 (latch -> header) 的边跳过
        for (BasicBlock u : bbList) {
            for (BasicBlock v : u.getSucs()) {
                LoopInfo loop = v.getLoopBelonged();
                boolean isHeader = loop != null && loop.getLoopHeader() == v; // 统一用 ==
                if (isHeader && loop.getLatchBlocks().contains(u)) {
                    // 跳过回边
                    continue;
                }
                adj.get(u).add(v);
                indeg.put(v, indeg.get(v) + 1);
            }
        }

        // Kahn：所有入度为 0 的块都入队（不只 entry）
        ArrayDeque<BasicBlock> q = new ArrayDeque<>();
        for (BasicBlock b : bbList) {
            if (indeg.get(b) == 0) {
                /*System.out.println("0: "+b.value2string());*/
                q.addLast(b);
            }
        }

        ArrayList<BasicBlock> order = new ArrayList<>(bbList.size());
        IdentityHashMap<BasicBlock, Boolean> seen = new IdentityHashMap<>();

        while (!q.isEmpty()) {
            BasicBlock u = q.pollFirst();
            if (seen.put(u, Boolean.TRUE) != null) continue;
            order.add(u);
            for (BasicBlock v : adj.get(u)) {
                int d = indeg.get(v) - 1;
                indeg.put(v, d);
                if (d == 0) q.addLast(v);
                else if (d < 0) {
                    throw new IllegalStateException("negative in-degree for " + v.value2string());
                }
            }
        }

        // 附加未输出的块（不可达/存在非latch环，例如不可约图）
        if (order.size() != bbList.size()) {
            for (BasicBlock b : bbList) {
                if (!seen.containsKey(b)) {
                    System.err.println("Unreached or in irreducible SCC (excluding latch): " + b.value2string());
                    order.add(b);
                }
            }
            // 按需：如果你希望严格报错，可改回 throw
            // throw new RuntimeException("CFG contains unreachable or cyclic blocks (excluding latch)");
        }

        return order;
    }



    public List<BasicBlock> oldgetTopoSortExludingLatch() {
        ArrayList<BasicBlock> sortedBlocks = new ArrayList<>();
        Map<BasicBlock, Integer> inDegreeMap = new HashMap<>();

        List<BasicBlock> bbList = getBasicBlockList();

        // Step 1: 初始化所有块的入度（不考虑 latch）
        for (BasicBlock bb : bbList) {
            LoopInfo loop = bb.getLoopBelonged();
            if (!(loop != null && loop.getLoopHeader().equals(bb))) {
                inDegreeMap.put(bb, bb.getPres().size());
                continue;
            }

            int inDegree = 0;
            for (BasicBlock pred : bb.getPres()) {
                // 如果是 loop header，且 pred 是 latch，则忽略该边
                if (loop.getLatchBlocks().contains(pred)) continue;
                inDegree++;
            }

            inDegreeMap.put(bb, inDegree);
        }

        // Step 2: 拓扑排序（跳过 latch edge）
        Queue<BasicBlock> queue = new LinkedList<>();
        queue.add(getFirstBlk());

        while (!queue.isEmpty()) {
            BasicBlock current = queue.poll();
            sortedBlocks.add(current);

            for (BasicBlock suc : current.getSucs()) {
                LoopInfo loop = suc.getLoopBelonged();
                boolean isHeader = loop != null && loop.getLoopHeader() == suc;

                // 如果当前是 latch 边（回到 header），跳过
                if (isHeader && loop.getLatchBlocks().contains(current)) continue;

                int newDeg = inDegreeMap.get(suc) - 1;
                inDegreeMap.put(suc, newDeg);

                if (newDeg == 0) {
                    queue.offer(suc);
                } else if (newDeg < 0) {
                    throw new IllegalStateException("negative in-degree for " + suc.value2string());
                }
            }
        }

        // Step 3: 校验是否所有节点都已排序
        if (sortedBlocks.size() != bbList.size()) {
            for (BasicBlock block : bbList) {
                if (!sortedBlocks.contains(block)) {
                    System.err.println("Unreachable block: " + block.value2string());
                }
            }
            throw new RuntimeException("CFG contains unreachable or cyclic blocks (excluding latch)");
        }

        return sortedBlocks;
    }

    public CustomList getBasicBlocks() {
        return this.basicBlocks;
    }

    public static class FParam extends Value {
        private final int regIdx;
        private final boolean isFromGlobal;

        public FParam(Type type, int regIdx) {
            super(type);
            this.regIdx = regIdx;
            this.isFromGlobal = false;
        }

        public FParam(Type type, int regIdx, boolean isFromGlobal) {
            super(type);
            this.regIdx = regIdx;
            this.isFromGlobal = isFromGlobal;
        }
        
        public int getRegIdx() {
            return regIdx;
        }
        
        @Override
        public String toString() {
            return this.getType().printIRType() + " " + value2string();
        }

        @Override
        public String value2string() {
            return "%reg_" + regIdx;
        }
    }

    public static class LibFunc extends Function {
        public static final LibFunc GETINT = new LibFunc(
                new TInt(), "getint"
        );
        public static final LibFunc GETCH = new LibFunc(
                new TInt(), "getch"
        );
        public static final LibFunc GETFLOAT = new LibFunc(
                new TFloat(), "getfloat"
        );
        public static final LibFunc GETARRAY = new LibFunc(
                new TInt(), "getarray", new TPointer(new TInt())
        );
        public static final LibFunc GETFARRAY = new LibFunc(
                new TInt(), "getfarray", new TPointer(new TFloat())
        );
        public static final LibFunc PUTINT = new LibFunc(
                new TVoid(), "putint", new TInt()
        );
        public static final LibFunc PUTCH = new LibFunc(
                new TVoid(), "putch", new TInt()
        );
        public static final LibFunc PUTFLOAT = new LibFunc(
                new TVoid(), "putfloat", new TFloat()
        );
        public static final LibFunc PUTARRAY = new LibFunc(
                new TVoid(), "putarray", new TInt(), new TPointer(new TInt())
        );
        public static final LibFunc PUTFARRAY = new LibFunc(
                new TVoid(), "putfarray", new TInt(), new TPointer(new TFloat())
        );
        public static final LibFunc PUTF = new LibFunc(
                new TVoid(), "putf" // declare void @putf(i8*, ...) todo: 如果要实现需要进行特殊处理
        );
        public static final LibFunc STARTTIME = new LibFunc(
                // 调用时需要特殊处理（源文件中是 void starttime() 但是运行时需要替换函数名并提供行号）
                new TVoid(), "_sysy_starttime", new TInt()
        );
        public static final LibFunc STOPTIME = new LibFunc(
                // 调用时需要特殊处理（源文件中是 void stoptime() 但是运行时需要替换函数名并提供行号）
                new TVoid(), "_sysy_stoptime", new TInt()
        );
        public static final LibFunc MEMSET = new LibFunc(
                new TVoid(), "memset", new TPointer(new TChar()), new TInt(), new TInt()
        );
        public static final List<LibFunc> ALL_LIB_FUNC = Arrays.asList(
                GETINT, GETCH, GETFLOAT, GETARRAY, GETFARRAY,
                PUTINT, PUTCH, PUTFLOAT, PUTARRAY, PUTFARRAY, // PUTF,
                STARTTIME, STOPTIME,
                MEMSET
        );
        private static final Map<String, LibFunc> NAME_TO_LIB_FUNC = new HashMap<>();

        static {
            for (LibFunc libFunc : ALL_LIB_FUNC) {
                if (libFunc == STARTTIME) {
                    NAME_TO_LIB_FUNC.put("starttime", libFunc);
                } else if (libFunc == STOPTIME) {
                    NAME_TO_LIB_FUNC.put("stoptime", libFunc);
                } else {
                    NAME_TO_LIB_FUNC.put(libFunc.getName(), libFunc);
                }
            }
        }

        public static LibFunc getLibFunc(String name) {
            return NAME_TO_LIB_FUNC.getOrDefault(name, null);
        }

        public LibFunc(Type retType, String name, Type... fParamTypes) {
            super(retType, name);

            int regIdx = 0;
            for (Type type : fParamTypes) {
                this.appendFParam(new FParam(type, regIdx++));
            }
        }

        public String getDecl() {
            // if (this == PUTF) { return "declare void @putf(i8*, ...)"; }

            StringBuilder fParamStringBuilder = new StringBuilder();
            for (FParam fParam : this.getFParams()) {
                fParamStringBuilder.append(fParam.getType()).append(", ");
            }
            String fParamStr = fParamStringBuilder.toString();
            if (fParamStr.length() >= 2) {
                fParamStr = fParamStr.substring(0, fParamStr.length() - 2);
            }
            return "declare " + this.getType().printIRType() + " @" + this.getName() + "(" + fParamStr + ")";
        }
    }

    public void ReallocateBlocks(List<BasicBlock> chain) {
        CustomList newBasicBlocks = new CustomList(this);
        for (BasicBlock block : chain) {
            newBasicBlocks.addToTail(block);
        }
        this.basicBlocks = newBasicBlocks;
    }

    public void printDataFlow() {
        for (BasicBlock bb : this.getBasicBlockList()) {
            System.out.println(bb);
            System.out.println(bb.getLastInstr());
        }
    }

    public Instruction instructionSpliter(int regIndex) {
        for (BasicBlock bb : this.getBasicBlockList()) {
            for (Instruction instr : bb.getInstrList()) {
                if (instr.getRegIndex() != null && instr.getRegIndex() == regIndex) {
                    return instr;
                }
            }
        }
        return null;
    }
}
