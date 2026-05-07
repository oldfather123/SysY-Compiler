package backend;

import Utils.CustomList;
import backend.asmInstr.AsmInstr;
import backend.asmInstr.asmBinary.AsmAdd;
import backend.asmInstr.asmLS.*;
import backend.asmInstr.asmTermin.AsmCall;
import backend.itemStructure.*;
import backend.regs.*;
import frontend.ir.Value;

import java.util.*;

public class RegAllocLinear {
    private RegAllocLinear(HashMap<AsmOperand, Value> downOperandMap) {
        // 初始化逻辑
        this.downOperandMap = downOperandMap;
    }
    private static int newAllocSize = 0;
    private HashMap<AsmOperand, Value> downOperandMap;
    private static RegAllocLinear instance = null;
    private HashSet<AsmOperand> all = new HashSet<>();
    private List<Interval> sortedIntervals = new ArrayList<>();
    private Stack<Integer> freeRegisters = new Stack<>();
    // 提供一个公共的静态方法，用于获取单例对象
    public static synchronized RegAllocLinear getInstance(HashMap<AsmOperand, Value> downOperandMap) {
        if (instance == null) {
            instance = new RegAllocLinear(downOperandMap);
        }
        return instance;
    }


    // 你需要实现的Interval类
    private class Interval {
        public int type; // 0非固定区间， 1固定区间，不可split，不可spill
        private AsmOperand register;
        private int assignedReg = -1;
        public int assignedSpill = 0;
        public ArrayList<Range> RangeList = new ArrayList<>();
        public ArrayList<UsePostion> UsePostionList = new ArrayList<>();
        public Interval splitParent = null;
        public ArrayList<Interval> splitChildren = new ArrayList<>();
        public Interval(AsmOperand register) {
            this.register = register;
            if (register instanceof AsmVirReg || register instanceof AsmFVirReg) {
                type = 0;
            } else if (register instanceof AsmPhyReg || register instanceof AsmFPhyReg) {
                type = 1;
                if (register instanceof AsmPhyReg) {
                    assignedReg = RegGeter.nameToIndexInt.get(register.toString());
                } else {
                    assignedReg = RegGeter.nameToIndexFloat.get(register.toString()) + 32;
                }
            }
        }
        public Interval(AsmOperand register, Interval splitParent) {
            this.register = register;
            this.splitParent = splitParent;
            if (register instanceof AsmVirReg || register instanceof AsmFVirReg) {
                type = 0;
            } else if (register instanceof AsmPhyReg || register instanceof AsmFPhyReg) {
                type = 1;
                if (register instanceof AsmPhyReg) {
                    assignedReg = RegGeter.nameToIndexInt.get(register.toString());
                } else {
                    assignedReg = RegGeter.nameToIndexFloat.get(register.toString()) + 32;
                }
            }
        }
        public void addRange(int from, int to) {
            Range newRange = new Range(from, to);
            int insertPos = 0;
            // 查找插入位置，保持有序
            for (int i = 0; i < RangeList.size(); i++) {
                Range range = RangeList.get(i);
                if (range.from > from) {
                    insertPos = i;
                    break;
                } else if (i == RangeList.size() - 1) {
                    insertPos = RangeList.size();
                }
            }
            // 插入新的区间
            RangeList.add(insertPos, newRange);
            // 合并区间
            mergeRanges();
        }
        private void mergeRanges() {
            ArrayList<Range> mergedRanges = new ArrayList<>();
            Range current = null;

            for (Range range : RangeList) {
                if (current == null) {
                    current = range;
                } else {
                    if (current.to >= range.from) {
                        current.to = Math.max(current.to, range.to);
                    } else {
                        mergedRanges.add(current);
                        current = range;
                    }
                }
            }

            if (current != null) {
                mergedRanges.add(current);
            }

            RangeList = mergedRanges;
        }
        private boolean cover(int position) {
            for (Range range : RangeList) {
                if (range.from <= position && range.to >= position) {
                    return true;
                }
            }
            return false;
        }
        public Interval child_at(int position) {
            if (cover(position)) {
                return this;// todo
            }
            for (Interval child: splitChildren) {
                if (child.cover(position)) {
                    return child;
                }
            }
            return null;
        }
        public void add_use_pos(int pos, int kind) {
            UsePostionList.add(new UsePostion(pos,kind));
        }
        public void printRanges() {
            // Find the maximum 'to' value to determine the size of the output
            int maxTo = 0;
            for (Range range : RangeList) {
                maxTo = Math.max(maxTo, range.to);
            }

            // Create an array to represent the output line
            char[] line = new char[maxTo + 1];
            java.util.Arrays.fill(line, ' ');

            // Mark ranges with '*'
            for (Range range : RangeList) {
                for (int i = range.from; i <= range.to; i++) {
                    line[i] = '*';
                }
            }
            for (UsePostion usePostion : UsePostionList) {
                line[usePostion.position] = '&';
            }

            // Convert the line to a string and print it
            int totalLength = 25;
            // 获取注册器字符串，并在末尾加上冒号
            String registerStr = register.toString() + ":";
            // 使用 String.format 生成指定长度的字符串，并用空格填充
            String format = String.format("%-" + totalLength + "s", registerStr);
            System.out.print(format);
            System.out.println(new String(line));
        }

    }
    //from包含，to不包含（todo 注意检查这个逻辑
    private class Range {
        public int from;
        public int to;
        public Range(int from, int to) {
            this.from = from;
            this.to = to;
        }
    }

    private class UsePostion {
        public int position;
        public int kind;
        public UsePostion(int position, int use_kind) {
            this.position = position;
            this.kind = use_kind;
        }
    }
    private class Move extends AsmInstr {
        public Interval src;
        public Interval dst;
        public Move(Interval src, Interval dst) {
            super("DefineMove");
            this.src = src;
            this.dst = dst;
        }
    }



    ///////
//    // 将方法的第一个基本块添加到工作列表
//    append first block of method to work_list
//
//// 当工作列表不为空时，进行以下操作
//while work_list is not empty do
//    // 从工作列表中选取并移除第一个基本块
//    BlockBegin b = pick and remove first block from work_list
//    // 将选取的基本块添加到blocks集合
//    append b to blocks
//    // 遍历基本块b的所有后继基本块
//    for each successor sux of b do
//    // 减少后继基本块的前向分支计数
//    decrement sux.incoming_forward_branches
//    // 如果后继基本块的前向分支计数为0
//        if sux.incoming_forward_branches = 0 then
//    // 将后继基本块按顺序插入工作列表
//    sort sux into work_list
//    end if
//    end for
//    end while
    ArrayList<AsmBlock> blockWorkList = new ArrayList<>();
    ArrayList<AsmBlock> blocks = new ArrayList<>();
    HashMap<AsmBlock, Integer> blockPreNum = new HashMap<>();
    private void initial(AsmFunction function) {
        AsmBlock blockHead = (AsmBlock) function.getBlocks().getHead();
        while (blockHead != null) {
            blockPreNum.put(blockHead, blockHead.pres.size());
            blockHead = (AsmBlock) blockHead.getNext();
        }
        newAllocSize = 0;
    }
    private void COMPUTE_BLOCK_ORDER(AsmFunction function) {
        this.blockWorkList.clear();
        this.blocks.clear();
        initial(function);
        blockWorkList.add((AsmBlock) function.getBlocks().getHead());
        while (blockWorkList.size() > 0) {
            AsmBlock block = (AsmBlock) blockWorkList.remove(0);
            blocks.add(block);
            for (AsmBlock successor: block.sucs) {
                blockPreNum.put(successor, blockPreNum.get(successor) - 1);
                if (blockPreNum.get(successor) == 0) {
                    blockWorkList.add(successor);
                }
            }
        }
    }

//    NUMBER_OPERATIONS
//    // 初始化下一个ID为0
//    int next_id = 0
//    // 遍历所有基本块
//    for each block b in blocks do
//            // 遍历基本块b中的所有操作
//            for each operation op in b.operations do
//             // 为操作op分配当前的ID
//              op.id = next_id
//            // 更新下一个ID，增加2
//              next_id = next_id + 2
//            end for
//    end for
    HashMap<AsmInstr, Integer> instrsId = new HashMap<>();
    HashMap<Integer, AsmInstr> idInstrs = new HashMap<>();
    private void NUMBER_OPERATIONS() {
        idInstrs.clear();
        instrsId.clear();
        int next_id = 0;
        for (AsmBlock block: blocks) {
            AsmInstr instrHead = (AsmInstr) block.getInstrs().getHead();
            while (instrHead != null) {
                instrHead.parent = block;
                instrsId.put(instrHead,next_id);
                idInstrs.put(next_id, instrHead);
                next_id+=2;
                instrHead = (AsmInstr) instrHead.getNext();
            }
        }
    }
//    {
//        // 声明一个LIR_OpVisitState类型的访问器，用于收集操作的所有操作数
//        LIR_OpVisitState visitor;
//// 遍历所有基本块
//        for each block b in blocks do
//        // 初始化当前基本块的局部生成集和局部消除集为空集合
//        b.live_gen = { }
//        b.live_kill = { }
//        // 遍历基本块b中的所有操作
//        for each operation op in b.operations do
//        // 使用访问器访问当前操作
//        visitor.visit(op)
//        // 遍历当前操作的输入操作数
//        for each virtual register opr in visitor.input_oprs do
//        // 如果输入操作数不在当前基本块的消除集中，则将其添加到生成集中
//        if opr ∉ b.live_kill then
//        b.live_gen = b.live_gen ∪ { opr }
//        end if
//        end for
//        // 遍历当前操作的临时操作数
//        for each virtual register opr in visitor.temp_oprs do //我们没有临时操作数
//        // 将临时操作数添加到当前基本块的消除集中
//        b.live_kill = b.live_kill ∪ { opr }
//        end for
//        // 遍历当前操作的输出操作数
//        for each virtual register opr in visitor.output_oprs do
//        // 将输出操作数添加到当前基本块的消除集中
//        b.live_kill = b.live_kill ∪ { opr }
//        end for
//        end for
//        end for
//    }
    private void COMPUTE_LOCAL_LIVE_SETS() {
        for (AsmBlock block: blocks) {
            AsmInstr InsHead = (AsmInstr) block.getInstrs().getHead();
            while (InsHead != null) {
                for (AsmReg one : InsHead.regUse) {
                        if (!block.getDef().contains(one)) {
                            block.getUse().add(one);
                             all.add(one);
                        }
                }
                for (AsmReg one : InsHead.regDef) {
                        if (!block.getDef().contains(one)) {
                            block.getDef().add(one);
                             all.add(one);
                        }
                }
                InsHead = (AsmInstr) InsHead.getNext();
            }
        }
    }
//    {
//        COMPUTE_GLOBAL_LIVE_SETS
//        do
//            for each block b in blocks in reverse order do // 反向遍历所有基本块
//        b.live_out = { } // 初始化当前基本块的 live_out 集合为空
//        for each successor sux of b do // 遍历当前基本块的每个后继块
//        b.live_out = b.live_out ∪ sux.live_in // 将后继块的 live_in 集合并入当前块的 live_out 集合
//        end for
//        b.live_in = (b.live_out – b.live_kill) ∪ b.live_gen // 计算当前块的 live_in 集合
//        end for
//        while change occurred in any live set // 只要任何 live 集合发生变化，就继续循环
//    }
    private void COMPUTE_GLOBAL_LIVE_SETS(AsmFunction function) {
        {
            AsmBlock blockHead = (AsmBlock) function.getBlocks().getHead();
            while (blockHead != null) {
                blockHead.LiveIn.clear();
                blockHead.LiveOut.clear();
                AsmInstr instrHead = (AsmInstr) blockHead.getInstrs().getHead();
                while (instrHead != null) {
                    instrHead.LiveIn.clear();
                    instrHead.LiveOut.clear();
                    instrHead = (AsmInstr) instrHead.getNext();
                }
                blockHead = (AsmBlock) blockHead.getNext();
            }
        }
        boolean changed = true;
        while (changed) {
            changed = false;
            for (int i = blocks.size() - 1; i >= 0; i--) {
                AsmBlock blockTail = blocks.get(i);
                //LiveOut[B_i] <- Union (LiveIn[s]) where s belongs to succ(B_i) ;
                for (AsmBlock succBlock : blockTail.sucs) {
                    for (AsmReg one : succBlock.LiveIn) {
                        blockTail.LiveOut.add(one);
                    }
                }
                //NewLiveIn <- Union (LiveUse[B_i], (LiveOut[B_i] – Def[B_i]));
                HashSet<AsmReg> NewLiveIn = new HashSet<>();
                NewLiveIn.addAll(blockTail.LiveOut);
                NewLiveIn.removeAll(blockTail.getDef());
                NewLiveIn.addAll(blockTail.getUse());
                if (!NewLiveIn.equals(blockTail.LiveIn)) {
                    changed = true;
                    blockTail.LiveIn = NewLiveIn;
                }
            }
        }
    }
    {
//        BUILD_INTERVALS
//        LIR_OpVisitState visitor; // 访问状态，用于收集操作的所有操作数
//        for each block b in blocks in reverse order do // 反向遍历所有基本块
//        int block_from = b.first_op.id // 获取当前块的起始操作ID
//        int block_to = b.last_op.id + 2 // 获取当前块的结束操作ID并加2
//        for each operand opr in b.live_out do // 遍历当前块的所有活跃操作数
//        intervals[opr].add_range(block_from, block_to) // 为操作数添加区间
//        end for
//        for each operation op in b.operations in reverse order do // 反向遍历当前块的所有操作
//        visitor.visit(op) // 访问操作，收集操作数
//        if visitor.has_call then // 如果操作是一个调用
//        for each physical register reg do // 遍历所有物理寄存器
//        intervals[reg].add_range(op.id, op.id + 1) // 为物理寄存器添加一个长度为1的短区间
//        end for
//        end if
//        for each virtual or physical register opr in visitor.output_oprs do // 遍历所有输出操作数
//        intervals[opr].first_range.from = op.id // 设置区间的起始位置
//        intervals[opr].add_use_pos(op.id, use_kind_for(op, opr)) // 添加使用位置
//        end for
//        for each virtual or physical register opr in visitor.temp_oprs do // 遍历所有临时操作数
//        intervals[opr].add_range(op.id, op.id + 1) // 为临时操作数添加一个长度为1的短区间
//        intervals[opr].add_use_pos(op.id, use_kind_for(op, opr)) // 添加使用位置
//        end for
//        for each virtual or physical register opr in visitor.input_oprs do // 遍历所有输入操作数
//        intervals[opr].add_range(block_from, op.id) // 为输入操作数添加区间
//        intervals[opr].add_use_pos(op.id, use_kind_for(op, opr)) // 添加使用位置 /todo use_kind_for啥意思
//        end for
//        end for
//        end for
    }
    private HashMap<AsmReg, Interval> IntIntervals = new HashMap<>();
    private HashMap<AsmReg, Interval> FloatIntervals = new HashMap<>();
    private boolean PhyCanBeAdd(AsmOperand opr){
        if (opr == RegGeter.AllRegsInt.get(10) || opr != RegGeter.AllRegsInt.get(5) || opr != RegGeter.AllRegsInt.get(0) || opr != RegGeter.AllRegsInt.get(1) || opr != RegGeter.AllRegsInt.get(2) || opr != RegGeter.AllRegsInt.get(3) || opr != RegGeter.AllRegsInt.get(4)) {
            return false;
        } else {
            return true;
        }
    }
    private  void  BUILD_INTERVALS() {
        IntIntervals.clear();
        FloatIntervals.clear();
        for (int b = blocks.size() - 1; b >= 0; b--) {
            AsmBlock blockTail = blocks.get(b);
            int block_from = instrsId.get((AsmInstr) blockTail.getInstrs().getHead());
            int block_to = instrsId.get((AsmInstr) blockTail.getInstrTail()) + 2;
            for (AsmOperand opr: blockTail.LiveOut) {
                if ((opr instanceof AsmPhyReg && PhyCanBeAdd(opr)) || opr instanceof AsmVirReg) {
                    IntIntervals.putIfAbsent((AsmReg) opr, new Interval(opr));
                    IntIntervals.get(opr).addRange(block_from, block_to);
                }
                if (opr instanceof AsmFPhyReg && opr != RegGeter.AllRegsFloat.get(10) || opr instanceof AsmFVirReg) {
                    FloatIntervals.putIfAbsent((AsmReg) opr, new Interval(opr));
                    FloatIntervals.get(opr).addRange(block_from, block_to);
                }
            }
            AsmInstr instrTail = (AsmInstr) blockTail.getInstrTail();
            while (instrTail != null) {
                if (instrTail instanceof AsmCall) {
                    for (int i = 0; i < 32; i++) {
                        if ((i >= 6 && i <= 7) || (i >= 11 && i <= 11) || (i >= 12 && i <= 17) || (i >= 28 && i <= 31)) {
                            IntIntervals.putIfAbsent(RegGeter.AllRegsInt.get(i), new Interval(RegGeter.AllRegsInt.get(i)));
                            IntIntervals.get(RegGeter.AllRegsInt.get(i)).addRange(instrsId.get(instrTail), instrsId.get(instrTail) + 1);
                        }
                    }
                    for (int i = 32; i < 64; i++) {
                        if ((i <= 39 && i >= 32) || (i >= 43 && i <= 49) || (i >= 60 && i <= 63)) {
                            FloatIntervals.putIfAbsent(RegGeter.AllRegsFloat.get(i - 32), new Interval(RegGeter.AllRegsFloat.get(i - 32)));
                            FloatIntervals.get(RegGeter.AllRegsFloat.get(i-32)).addRange(instrsId.get(instrTail), instrsId.get(instrTail) + 1);
                        }
                    }
                }
                for (AsmOperand opr: instrTail.regDef) {
                    if ((opr instanceof AsmPhyReg && PhyCanBeAdd(opr)) || opr instanceof AsmVirReg) {
                        IntIntervals.putIfAbsent((AsmReg) opr, new Interval(opr));
                        if (IntIntervals.get(opr).RangeList.isEmpty()) {
                            IntIntervals.get(opr).addRange(instrsId.get(instrTail), instrsId.get(instrTail) + 1); //todo 不确定
                        }
                        IntIntervals.get(opr).RangeList.get(0).from = instrsId.get(instrTail);
                        IntIntervals.get(opr).add_use_pos(instrsId.get(instrTail), 1);
                    }
                    if ((opr instanceof AsmFPhyReg && opr != RegGeter.AllRegsFloat.get(10)) || opr instanceof AsmFVirReg) {
                        FloatIntervals.putIfAbsent((AsmReg) opr, new Interval(opr));
                        FloatIntervals.get(opr).RangeList.get(0).from = instrsId.get(instrTail);
                        FloatIntervals.get(opr).add_use_pos(instrsId.get(instrTail), 1);
                    }
                }
                for (AsmOperand opr: instrTail.regUse) {
                    if ((opr instanceof AsmPhyReg && PhyCanBeAdd(opr)) || opr instanceof AsmVirReg) {
                        IntIntervals.putIfAbsent((AsmReg) opr, new Interval(opr));
                        IntIntervals.get(opr).addRange(block_from, instrsId.get(instrTail));
                        IntIntervals.get(opr).add_use_pos(instrsId.get(instrTail), 1);
                    }
                    if ((opr instanceof AsmFPhyReg && opr != RegGeter.AllRegsFloat.get(10)) || opr instanceof AsmFVirReg) {
                        FloatIntervals.putIfAbsent((AsmReg) opr, new Interval(opr));
                        FloatIntervals.get(opr).RangeList.get(0).from = instrsId.get(instrTail);
                        FloatIntervals.get(opr).add_use_pos(instrsId.get(instrTail), 1);
                    }
                }
                instrTail = (AsmInstr) instrTail.getPrev();
            }
        }
    }
//    {
//        WALK_INTERVALS
//        unhandled = list of intervals sorted by increasing start point // 未处理区间列表，按起始点递增排序
//        active = { } // 活动区间集合
//        inactive = { } // 非活动区间集合
//
//// 注意：在分配期间，当区间被分割时，新的区间可能会被排序到unhandled列表中
//        while unhandled ≠ { } do // 当未处理区间不为空时，继续执行
//        current = pick and remove first interval from unhandled // 选取并移除unhandled列表中的第一个区间
//        position = current.first_range.from // 获取当前区间的起始位置
//
//        // 检查活动区间中的区间是否过期或变为非活动
//        for each interval it in active do
//        if it.last_range.to < position then // 如果区间的结束位置小于当前起始位置
//        move it from active to handled // 将区间从active移动到handled（已处理区间集合）
//        else if not it.covers(position) then // 否则，如果区间不覆盖当前起始位置
//        move it from active to inactive // 将区间从active移动到inactive
//        end if
//        end for
//
//        // 检查非活动区间中的区间是否过期或变为活动
//        for each interval it in inactive do
//        if it.last_range.to < position then // 如果区间的结束位置小于当前起始位置
//        move it from inactive to handled // 将区间从inactive移动到handled
//        else if it.covers(position) then // 否则，如果区间覆盖当前起始位置
//        move it from inactive to active // 将区间从inactive移动到active
//        end if
//        end for
//
//        // 为当前区间分配寄存器
//        TRY_ALLOCATE_FREE_REG // 尝试分配空闲寄存器
//        if allocation failed then // 如果分配失败
//            ALLOCATE_BLOCKED_REG // 分配被阻塞的寄存器
//        end if
//
//        if current has a register assigned then // 如果当前区间被分配了寄存器
//        add current to active // 将当前区间添加到active
//        end if
//        end while
//    }
    ArrayList<Interval> unhandled = new ArrayList<>();
    ArrayList<Interval> active = new ArrayList<>();
    ArrayList<Interval> inactive = new ArrayList<>();
    private void initial_( ArrayList<Interval> args) {
        unhandled.clear();
        active.clear();
        inactive.clear();
        unhandled.addAll(args);
        Collections.sort(unhandled, new Comparator<Interval>() {
            @Override
            public int compare(Interval o1, Interval o2) {
                return Integer.compare(o1.RangeList.get(0).from, o2.RangeList.get(0).from);
            }
        });

    }
    private void WALK_INTERVALS(ArrayList<Interval> args, AsmFunction function,int type) {
        initial_(args);
        while (!unhandled.isEmpty()) {
            Interval current = unhandled.remove(0);
            int position = current.RangeList.get(0).from;
            // 检查active中的区间
            Iterator<Interval> activeIterator = active.iterator();
            while (activeIterator.hasNext()) {
                Interval it = activeIterator.next();
                if (it.RangeList.get(it.RangeList.size() - 1).to < position) {
                    activeIterator.remove();
                    // move it to handled (not implemented, assumed to be a separate collection)
                } else if (!it.cover(position)) {
                    activeIterator.remove();
                    inactive.add(it);
                }
            }
            // 检查inactive中的区间
            for (int i = 0; i < inactive.size(); i++) {
                Interval it = inactive.get(i);
                if (it.RangeList.get(it.RangeList.size() - 1).to < position) {
                    inactive.remove(i);
                } else if (it.cover(position)) {
                    inactive.remove(i);
                    active.add(it);
                }
                i--;
            }

            // 为current分配寄存器
            boolean allocation = TRY_ALLOCATE_FREE_REG(current, type);
            if (!allocation) {
                ALLOCATE_BLOCKED_REG(current, function, type);
            }

            if (current.assignedReg != -1) {
                active.add(current);
            }
        }
    }

//    {
//        TRY_ALLOCATE_FREE_REG
//        // 将所有物理寄存器的 free_pos 设置为最大整数，表示所有寄存器初始状态均可用
//        set free_pos of all physical registers to max_int
//
//        // 遍历 active 集合中的每个区间
//        for each interval it in active do
//        // 将这些区间对应的寄存器的 free_pos 设置为 0，表示这些寄存器当前正在使用，不能分配给 current 区间
//        set_free_pos(it, 0)
//        end for
//
//        // 遍历与 current 相交的 inactive 集合中的每个区间
//        for each interval it in inactive intersecting with current do
//        // 将这些区间的下一个与 current 区间相交的位置设置为 free_pos
//        // 表示这些寄存器在相交前是可用的，但在相交之后就不能再用了
//        set_free_pos(it, next intersection of it with current)
//        end for
//
//        // 找到 free_pos 最高的寄存器
//        reg = register with highest free_pos
//
//        // 检查 free_pos[reg] 是否为 0
//        if free_pos[reg] = 0 then
//        // 分配失败，没有可用的寄存器而不发生溢出
//        return false
// else if free_pos[reg] > current.last_range.to then
//        // 寄存器在整个 current 区间生命周期内可用
//        // 将寄存器分配给 current 区间
//        assign register reg to interval current
// else
//        // 寄存器在 current 区间的前一部分可用
//        // 将寄存器分配给 current 区间的前一部分
//        assign register reg to interval current
//        // 在 free_pos[reg] 之前的最优位置将 current 区间分割开
//        split current at optimal position before free_pos[reg]
//        end if
//    }
    private HashMap<Integer, Integer> free_pos; // <寄存器编号，free_pos>
    private boolean TRY_ALLOCATE_FREE_REG(Interval current, int type) {
        free_pos.clear();
        // 寄存器分配的逻辑实现
        if (type == 0) { // 整数型
            for (int i = 0 ; i < 32; i++) {
                if (PhyCanBeAdd(RegGeter.AllRegsInt.get(i))) {
                    free_pos.put(i, 2147483647);
                }
            }
        } else {
            for (int i = 32 ; i < 64; i++) {
                if (i != 42) {
                    free_pos.put(i, 2147483647);
                }
            }
        }
        for (Interval it: active){
            free_pos.put(it.assignedReg, 0);
        }
        for (Interval it: inactive){
            if (intersects(it, current)) {
               int pos = findNextIntersection(it, current);
               free_pos.put(it.assignedReg, pos);
            }
        }
        ///自己加的
        if (current.type == 1) {
            int freePos = free_pos.get(current.assignedReg);
            if (freePos == 0) {
                return false;
            } else if (freePos > current.RangeList.get(current.RangeList.size()).to) {
                return true;
            } else {
                return false;
            }
        }
        ///
        int alloc = RegisterWithMaxFreePos();
        if (free_pos.get(alloc) == 0) {
            return false;
        } else if (free_pos.get(alloc) > current.RangeList.get(current.RangeList.size()).to) {
            current.assignedReg = alloc;
        } else {
            current.assignedReg = alloc;
            Interval newInterval = splitInterval(current, free_pos.get(alloc));
            GenerateMove(current, newInterval, newInterval.RangeList.get(0).from,0);
        }
        return true;
    }

    private boolean intersects(Interval it, Interval current) {
        // 检查两个区间是否相交
        for (Range r1 : it.RangeList) {
            for (Range r2 : current.RangeList) {
                if (r1.from < r2.to && r2.from < r1.to) {
                    return true;
                }
            }
        }
        return false;
    }

    private int findNextIntersection(Interval it, Interval current) {
        // 找到 it 区间和 current 区间的下一个交点位置
        int nextIntersection = Integer.MAX_VALUE;
        for (Range r1 : it.RangeList) {
            for (Range r2 : current.RangeList) {
                if (r1.from < r2.to && r2.from < r1.to) {
                    int intersectionPoint = Math.max(r1.from, r2.from);
                    if (intersectionPoint > current.RangeList.get(0).from) {
                        nextIntersection = Math.min(nextIntersection, intersectionPoint);
                    }
                }
            }
        }
        return nextIntersection;
    }
    public Integer RegisterWithMaxFreePos() {
        Integer maxRegister = null;
        int maxFreePos = Integer.MIN_VALUE;

        for (Map.Entry<Integer, Integer> entry : free_pos.entrySet()) {
            if (entry.getValue() > maxFreePos) {
                maxFreePos = entry.getValue();
                maxRegister = entry.getKey();
            }
        }

        return maxRegister;
    }
    public Interval splitInterval(Interval current, int splitPoint) {
        // 检查是否需要分割，如果分割点在当前区间的有效范围之外，则无需分割。
        if (splitPoint <= current.RangeList.get(0).from || splitPoint >= current.RangeList.get(current.RangeList.size() - 1).to) {
            return null; // 如果分割点在当前区间的范围之外，则不进行分割
        }
        Interval newInterval;
        // 创建新的子区间，从分割点开始
        if (current.splitParent == null) {
            newInterval = new Interval(current.register, current);
        } else {
            newInterval = new Interval(current.register, current.splitParent);
        }
        // 分割当前区间的范围和使用位置
        ArrayList<Range> updatedRanges = new ArrayList<>();
        ArrayList<UsePostion> updatedUsePositions = new ArrayList<>();
        for (Range range : current.RangeList) {
            if (range.to <= splitPoint) {
                // 当前范围在分割点之前
                updatedRanges.add(range);
            } else if (range.from < splitPoint) {
                // 当前范围在分割点中断
                updatedRanges.add(new Range(range.from, splitPoint));
                newInterval.addRange(splitPoint, range.to);
            } else {
                // 当前范围在分割点之后
                newInterval.addRange(range.from, range.to);
            }
        }
        // 处理使用位置
        for (UsePostion usePos : current.UsePostionList) {
            if (usePos.position < splitPoint) {
                updatedUsePositions.add(usePos);
            } else {
                newInterval.add_use_pos(usePos.position, usePos.kind);
            }
        }
        current.RangeList = updatedRanges;
        current.UsePostionList = updatedUsePositions;
        if (current.splitParent == null) {
            int insertPos = Collections.binarySearch(current.splitChildren, newInterval, Comparator.comparingInt(o -> o.RangeList.get(0).from));
            if (insertPos < 0) {
                insertPos = -insertPos - 1;
            }
            current.splitChildren.add(insertPos,newInterval);
        } else {
            int insertPos = Collections.binarySearch(current.splitParent.splitChildren, newInterval, Comparator.comparingInt(o -> o.RangeList.get(0).from));
            if (insertPos < 0) {
                insertPos = -insertPos - 1;
            }
            current.splitParent.splitChildren.add(insertPos,newInterval);
        }
        int insertPos = Collections.binarySearch(unhandled, newInterval, Comparator.comparingInt(o -> o.RangeList.get(0).from));
        if (insertPos < 0) {
            insertPos = -insertPos - 1;
        }
        unhandled.add(insertPos, newInterval);
        return newInterval;
    }
    public void GenerateMove(Interval src, Interval dst, int insertPlace, int foreOrback) {
        Move newMove = new Move(src,dst);
        if (foreOrback == 0) {
            newMove.insertBefore(idInstrs.get(insertPlace));
        } else {
            newMove.insertAfter(idInstrs.get(insertPlace));
        }
    }

//    {
//        // 初始化所有物理寄存器的 use_pos 和 block_pos 为最大整数值
//        set use_pos and block_pos of all physical registers to max_int
//
//// 对于每个非固定的活动区间
//        for each non-fixed interval it in active do
//        // 设置它们在 current.first_range.from 之后的下一个使用位置
//        set_use_pos(it, next usage of it after current.first_range.from)
//        end for
//
//// 对于每个与 current 相交的非固定的非活动区间
//        for each non-fixed interval it in inactive intersecting with current do
//        // 设置它们在 current.first_range.from 之后的下一个使用位置
//        set_use_pos(it, next usage of it after current.first_range.from)
//        end for
//
//// 对于每个固定的活动区间
//        for each fixed interval it in active do
//        // 将它们的阻塞位置设置为 0
//        set_block_pos(it, 0)
//        end for
//
//// 对于每个与 current 相交的固定的非活动区间
//        for each fixed interval it in inactive intersecting with current do
//        // 设置它们与 current 相交的下一个位置为阻塞位置
//        set_block_pos(it, next intersection of it with current)
//        end for
//
//// 选择 use_pos 最大的寄存器
//        reg = register with highest use_pos
//
//        if use_pos[reg] < first usage of current then
//        // 所有活动和非活动区间都在 current 之前使用，所以最好溢出 current 自身
//        assign spill slot to current
//        // 在第一个需要寄存器的位置之前的最优位置将 current 分割开
//        split current at optimal position before first use position that requires a register
//else if block_pos[reg] > current.last_range.to then
//        // 溢出使寄存器在整个 current 期间都是空闲的
//        assign register reg to interval current
//        // 分割并溢出与 reg 相交的活动和非活动区间
//        split and spill intersecting active and inactive intervals for reg
//else
//        // 溢出使寄存器在 current 区间的前一部分是空闲的
//        assign register reg to interval current
//        // 在 block_pos[reg] 之前的最优位置将 current 区间分割开
//        split current at optimal position before block_pos[reg]
//        // 分割并溢出与 reg 相交的活动和非活动区间
//        split and spill intersecting active and inactive intervals for reg
//        end if
//    }
    HashMap<Integer, Integer> use_pos = new HashMap<>();
    HashMap<Integer, Integer> block_pos = new HashMap<>();

    private void ALLOCATE_BLOCKED_REG(Interval current, AsmFunction function,int type) {//这边type指整数与浮点

        use_pos.clear();
        block_pos.clear();
        if (type == 0) { // 整数型
            for (int i = 0 ; i < 32; i++) {
                if (PhyCanBeAdd(RegGeter.AllRegsInt.get(i))) {
                    use_pos.put(i, 2147483647);
                    block_pos.put(i, 2147483647);
                }
            }
        } else {
            for (int i = 32 ; i < 64; i++) {
                if (i != 42) {
                    use_pos.put(i, 2147483647);
                    block_pos.put(i, 2147483647);
                }
            }
        }
        if (current.type == 1) {
            Interval needSpill = null;
            for (Interval one: inactive) {
                if (one.assignedReg == current.assignedReg) {
                    needSpill = one;
                }
            }
            for (Interval one: active) {
                if (one.assignedReg == current.assignedReg) {
                    needSpill = one;
                }
            }
            Interval newInterval = splitInterval(needSpill, current.RangeList.get(0).from);
            GenerateMove(current,newInterval, newInterval.RangeList.get(0).from, 0);
            int spillPlace = assignSpillSlot(function);
            newInterval.assignedSpill = spillPlace;
            newInterval.assignedReg = -1;
            Interval newnewInterval = splitInterval(newInterval, newInterval.UsePostionList.get(0).position);
            GenerateMove(newInterval, newnewInterval, newnewInterval.RangeList.get(0).from, 0);
            //扔进unhandled
            int insertPos = Collections.binarySearch(unhandled, newnewInterval, Comparator.comparingInt(o -> o.RangeList.get(0).from));
            if (insertPos < 0) {
                insertPos = -insertPos - 1;
            }
            unhandled.add(insertPos, newnewInterval);
        }
        for (Interval non_fixed: active) {
            if (non_fixed.type == 0) {
                use_pos.put(non_fixed.assignedReg, findNextUsageAfter(non_fixed, current.RangeList.get(0).from));
            }
        }
        for (Interval non_fixed: inactive){
            if (intersects(non_fixed, current) && non_fixed.type == 0) {
                use_pos.put(non_fixed.assignedReg, findNextUsageAfter(non_fixed, current.RangeList.get(0).from));
            }
        }
        for (Interval fixed: active) {
            if (fixed.type == 1) {
                block_pos.put(fixed.assignedReg, 0);
                use_pos.put(fixed.assignedReg, 0);
            }
        }
        for (Interval fixed: inactive){
            if (intersects(fixed, current) && fixed.type == 1) {
                block_pos.put(fixed.assignedReg, findNextIntersection(fixed, current));
                if (findNextIntersection(fixed, current) < use_pos.get(fixed.assignedReg)) {
                    use_pos.put(fixed.assignedReg, findNextIntersection(fixed, current));
                }
            }
        }
        int alloc = findRegisterWithHighestUsePos();
        if (use_pos.get(alloc) < current.UsePostionList.get(0).position) {
            int spillPlace = assignSpillSlot(function);
            current.assignedSpill = spillPlace;
            current.assignedReg = -1;
            Interval newInterval = splitInterval(current, current.UsePostionList.get(0).position);
            GenerateMove(current, newInterval, newInterval.RangeList.get(0).from, 0);
            //扔进unhandled
            int insertPos = Collections.binarySearch(unhandled, newInterval, Comparator.comparingInt(o -> o.RangeList.get(0).from));
            if (insertPos < 0) {
                insertPos = -insertPos - 1;
            }
            unhandled.add(insertPos, newInterval);
        } else if (block_pos.get(alloc) > current.RangeList.get(current.RangeList.size()).to){
            current.assignedReg = alloc;
            Interval needSpill = null;
            for (Interval one: inactive) {
                if (one.assignedReg == alloc) {
                    needSpill = one;
                }
            }
            for (Interval one: active) {
                if (one.assignedReg == alloc) {
                    needSpill = one;
                }
            }
            Interval newInterval = splitInterval(needSpill, current.RangeList.get(0).from);
            GenerateMove(current,newInterval, newInterval.RangeList.get(0).from, 0);
            int spillPlace = assignSpillSlot(function);
            newInterval.assignedSpill = spillPlace;
            newInterval.assignedReg = -1;
            Interval newnewInterval = splitInterval(newInterval, newInterval.UsePostionList.get(0).position);
            GenerateMove(newInterval, newnewInterval, newnewInterval.RangeList.get(0).from, 0);
            //扔进unhandled
            int insertPos = Collections.binarySearch(unhandled, newnewInterval, Comparator.comparingInt(o -> o.RangeList.get(0).from));
            if (insertPos < 0) {
                insertPos = -insertPos - 1;
            }
            unhandled.add(insertPos, newnewInterval);
        } else {
            current.assignedReg = alloc;
            Interval newCurrent = splitInterval(current, block_pos.get(alloc));
            GenerateMove(current, newCurrent, newCurrent.RangeList.get(0).from, 0);

            Interval needSpill = null;
            for (Interval one: inactive) {
                if (one.assignedReg == alloc) {
                    needSpill = one;
                }
            }
            for (Interval one: active) {
                if (one.assignedReg == alloc) {
                    needSpill = one;
                }
            }
            Interval newInterval = splitInterval(needSpill, current.RangeList.get(0).from);
            GenerateMove(needSpill,newInterval, newInterval.RangeList.get(0).from, 0);
            Interval newnewInterval = splitInterval(newInterval, newInterval.UsePostionList.get(0).position);
            GenerateMove(newInterval, newnewInterval, newnewInterval.RangeList.get(0).from, 0);
            int spillPlace = assignSpillSlot(function);
            newInterval.assignedSpill = spillPlace;
            newInterval.assignedReg = -1;

            //扔进unhandled
            int insertPos = Collections.binarySearch(unhandled, newnewInterval, Comparator.comparingInt(o -> o.RangeList.get(0).from));
            if (insertPos < 0) {
                insertPos = -insertPos - 1;
            }
            unhandled.add(insertPos, newnewInterval);
            insertPos = Collections.binarySearch(unhandled, newCurrent, Comparator.comparingInt(o -> o.RangeList.get(0).from));
            if (insertPos < 0) {
                insertPos = -insertPos - 1;
            }
            unhandled.add(insertPos, newCurrent);
        }
    }
    private int findNextUsageAfter(Interval interval, int position) {
        // 遍历 UsePostionList 找到第一个大于指定位置的使用位置
        for (UsePostion usePos : interval.UsePostionList) {
            if (usePos.position > position) {
                return usePos.position;
            }
        }
        // 如果没有找到，返回最大整数表示没有下一个使用位置
        return Integer.MAX_VALUE;
    }
    private int findRegisterWithHighestUsePos() {
        int maxUsePos = Integer.MIN_VALUE;
        int registerWithMaxUsePos = -1;

        for (Map.Entry<Integer, Integer> entry : use_pos.entrySet()) {
            int reg = entry.getKey();
            int pos = entry.getValue();

            if (pos > maxUsePos) {
                maxUsePos = pos;
                registerWithMaxUsePos = reg;
            }
        }

        return registerWithMaxUsePos;
    }
    private int assignSpillSlot(AsmFunction function) {
        // Assign a spill slot (memory location) to the current interval
        int spillSlot = function.getAllocaSize() + function.getArgsSize();
        function.addAllocaSize(8);
        newAllocSize += 8;
        return spillSlot;
    }

    private void storeOrLoadInt(int spillPlace, AsmReg reg, AsmInstr nowInstr, String type, int foreOrBack) {
        if (spillPlace >= -2048 && spillPlace <= 2047) {
            AsmImm12 place = new AsmImm12(spillPlace);
            if (type.equals("store")) {
                if (foreOrBack == 0) {
                    AsmSd store = new AsmSd(reg, RegGeter.SP, place);
                    store.insertBefore(nowInstr);
                } else {
                    AsmSd store = new AsmSd(reg, RegGeter.SP, place);
                    store.insertAfter(nowInstr);
                }
            } else if (type.equals("load")) {
                if (foreOrBack == 0) {
                        AsmLd load = new AsmLd(reg, RegGeter.SP, place);
                        load.insertBefore(nowInstr);
                } else {
                        AsmLd load = new AsmLd(reg, RegGeter.SP, place);
                        instrsId.put(load, instrsId.get(nowInstr) - 1);
                        idInstrs.put(instrsId.get(nowInstr) - 1, load);
                        load.insertAfter(nowInstr);
                }
            }
        } else {
            AsmReg tmpMove = RegGeter.AllRegsInt.get(5);
            AsmReg tmpAdd = RegGeter.AllRegsInt.get(5);
            AsmMove asmMove = new AsmMove(tmpMove, new AsmImm32(spillPlace));
            AsmAdd asmAdd = new AsmAdd(tmpAdd, RegGeter.SP, tmpMove);
            if (foreOrBack == 0) {
                asmMove.insertBefore(nowInstr);
                asmAdd.insertBefore(nowInstr);
                if (type.equals("load")) {
                    AsmLd load = new AsmLd(reg, tmpAdd, new AsmImm12(0));
                    load.insertBefore(nowInstr);
                } else if (type.equals("store")) {
                    AsmSd store = new AsmSd(reg, tmpAdd, new AsmImm12(0));
                    store.insertBefore(nowInstr);
                }
            } else {
                if (type.equals("load")) {
                    AsmLd load = new AsmLd(reg, tmpAdd, new AsmImm12(0));
                    load.insertAfter(nowInstr);
                } else if (type.equals("store")) {
                    AsmSd store = new AsmSd(reg, tmpAdd, new AsmImm12(0));
                    store.insertAfter(nowInstr);
                }
                asmAdd.insertAfter(nowInstr);
                asmMove.insertAfter(nowInstr);
            }
        }

    }

    private void storeOrLoadFloat(int spillPlace, AsmReg reg, AsmInstr nowInstr, String type, int foreOrBack) {
        if (spillPlace >= -2048 && spillPlace <= 2047) {
            AsmImm12 place = new AsmImm12(spillPlace);
            if (type.equals("store")) {
                if (foreOrBack == 0) {
                    AsmFsd store = new AsmFsd(reg, RegGeter.SP, place);
                    store.insertBefore(nowInstr);
                } else {
                    AsmFsd store = new AsmFsd(reg, RegGeter.SP, place);
                    store.insertAfter(nowInstr);
                }
            } else if (type.equals("load")) {
                if (foreOrBack == 0) {
                    AsmFld load = new AsmFld(reg, RegGeter.SP, place);
                    load.insertBefore(nowInstr);
                } else {
                    AsmFld load = new AsmFld(reg, RegGeter.SP, place);
                    load.insertAfter(nowInstr);
                }
            }
        } else {
            AsmReg tmpMove = RegGeter.AllRegsInt.get(5);
            AsmReg tmpAdd = RegGeter.AllRegsInt.get(5);
            AsmMove asmMove = new AsmMove(tmpMove, new AsmImm32(spillPlace));
            AsmAdd asmAdd = new AsmAdd(tmpAdd, RegGeter.SP, tmpMove);
            if (foreOrBack == 0) {
                asmMove.insertBefore(nowInstr);
                asmAdd.insertBefore(nowInstr);
                if (type.equals("load")) {
                    AsmFld load = new AsmFld(reg, tmpAdd, new AsmImm12(0));
                    load.insertBefore(nowInstr);
                } else if (type.equals("store")) {
                    AsmFsd store = new AsmFsd(reg, tmpAdd, new AsmImm12(0));
                    store.insertBefore(nowInstr);
                }
            } else {
                if (type.equals("load")) {
                    AsmFld load = new AsmFld(reg, tmpAdd, new AsmImm12(0));
                    load.insertAfter(nowInstr);
                } else if (type.equals("store")) {
                    AsmFsd store = new AsmFsd(reg, tmpAdd, new AsmImm12(0));
                    store.insertAfter(nowInstr);
                }
                asmAdd.insertAfter(nowInstr);
                asmMove.insertAfter(nowInstr);
            }
        }

    }

//    {
//        // 解析数据流
//        MoveResolver resolver // 用于在低级中间表示 (LIR) 中排序和插入移动指令
//        for each block from in blocks do
//        for each successor to of from do
//        // 收集在块 from 和 to 之间必要的所有解决移动指令
//        for each operand opr in to.live_in do
//        Interval parent_interval = intervals[opr]
//        Interval from_interval = parent_interval.child_at(from.last_op.id)
//        Interval to_interval = parent_interval.child_at(to.first_op.id)
//        if from_interval ≠ to_interval then
//        // 区间在块 from 和块 to 之间的边界被分割
//        resolver.add_mapping(from_interval, to_interval)
//        end if
//        end for
//        // 移动指令要么插入在块 from 的结尾，要么插入在块 to 的开头，取决于控制流
//        resolver.find_insert_position(from, to)
//        // 按正确顺序插入所有移动指令（不覆盖稍后使用的寄存器）
//        resolver.resolve_mappings()
//        end for
//        end for
//    }

    private void RESOLVE_DATA_FLOW(int type) {
        for (AsmBlock from: blocks) {
            for (AsmBlock to: from.sucs) {
                for (AsmReg opr: to.LiveIn) {
                    if (IntIntervals.containsKey(opr) && type == 0) {
                        Interval parent_interval = IntIntervals.get(opr);
                        Interval from_interval = parent_interval.child_at(instrsId.get(from.getInstrTail()));
                        Interval to_interval = parent_interval.child_at(instrsId.get(to.getInstrTail()));
                        if (from_interval != to_interval) {
                            AsmInstr instr = null;
                            if (to_interval != null) {
                                instr = idInstrs.get(to_interval.RangeList.get(0).from);
                            }
                            AsmBlock b = null;
                            if (instr != null) {
                                b = instr.parent;
                            }
                            if (from.sucs.contains(b)) {
                                int insertPlace = instrsId.get(from.getInstrTail());
                                GenerateMove(from_interval, to_interval, insertPlace, 1);
                            } else {
                                int insertPlace = instrsId.get(to.getInstrs().getHead());
                                GenerateMove(from_interval, to_interval, insertPlace, 0);
                            }
                        }
                    }
                    if (FloatIntervals.containsKey(opr) && type == 1) {
                        Interval parent_interval = IntIntervals.get(opr);
                        Interval from_interval = parent_interval.child_at(instrsId.get(from.getInstrTail()));
                        Interval to_interval = parent_interval.child_at(instrsId.get(to.getInstrTail()));
                        if (from_interval != to_interval) {
                            AsmInstr instr = idInstrs.get(to_interval.RangeList.get(0).from);
                            AsmBlock b = instr.parent;
                            if (from.sucs.contains(b)) {
                                int insertPlace = instrsId.get(from.getInstrTail());
                                GenerateMove(from_interval, to_interval, insertPlace, 1);
                            } else {
                                int insertPlace = instrsId.get(to.getInstrs().getHead());
                                GenerateMove(from_interval, to_interval, insertPlace, 0);
                            }
                        }
                    }
                }


            }
        }
    }
    // 分配寄存器编号
//    void ASSIGN_REG_NUM() {
//        LIR_OpVisitState visitor // 用于收集一个操作的所有操作数的访客
//        for each block b in blocks do
//            for each operation op in b.operations do
//            visitor.visit(op)
//        // 处理输入、临时和输出操作数
//        for each virtual register v_opr in visitor.oprs do
//            // 根据分配给区间的寄存器计算新的操作数
//            r_opr = intervals[v_opr].child_at(op.id).assigned_opr
//        // 将新的操作数存回操作中
//        visitor.set_opr(r_opr)
//        end for
//        if op is move with equal source and target then
//        // 从 LIR 操作列表中删除无用的 move 操作
//        remove op from b.operations
//        end if
//        end for
//        end for
//    }
    private void ASSIGN_REG_NUM() {
        for (AsmBlock block: blocks) {
            CustomList.Node instrHead = block.getInstrs().getHead();
            while (instrHead != null) {
                if (instrHead instanceof Move) {
                    Move move = (Move) instrHead;
                    if (move.dst.assignedReg != -1 && move.src.assignedReg != -1) {
                        int dstReg = move.dst.assignedReg;
                        int srcReg = move.src.assignedReg;
                        if (dstReg < 32 && srcReg < 32) {
                            AsmMove move1 = new AsmMove(RegGeter.AllRegsInt.get(dstReg), RegGeter.AllRegsInt.get(srcReg));
                            move1.insertBefore(move);
                            move.removeFromList();
                        } else if (dstReg >= 32 && srcReg >= 32) {
                            AsmMove move1 = new AsmMove(RegGeter.AllRegsFloat.get(dstReg - 32), RegGeter.AllRegsFloat.get(srcReg - 32));
                            move1.insertBefore(move);
                            move.removeFromList();
                        } else {
                            throw new RuntimeException("Invalid Move reg number: dstReg=" + dstReg + ", srcReg=" + srcReg);
                        }
                    } else if (move.dst.assignedReg != -1 && move.src.assignedReg == -1) {
                        int dstReg = move.dst.assignedReg;
                        int spill = move.src.assignedSpill;
                        if (dstReg < 32 && dstReg >= 0) {
                            storeOrLoadInt(spill, RegGeter.AllRegsInt.get(dstReg), move,  "load", 0);
                        } else if (dstReg >= 32) {
                            storeOrLoadFloat(spill, RegGeter.AllRegsFloat.get(dstReg - 32), move,  "load", 0);
                        } else {
                            throw new RuntimeException("Invalid Move reg number: dstReg=" + dstReg + ", spill=" + spill);
                        }
                        move.removeFromList();
                    } else if (move.dst.assignedReg == -1 && move.src.assignedReg != -1) {
                        int spill = move.src.assignedSpill;
                        int srcReg = move.dst.assignedReg;
                        if (srcReg < 32 && srcReg >= 0) {
                            storeOrLoadInt(spill, RegGeter.AllRegsInt.get(srcReg), move,  "load", 0);
                        } else if (srcReg >= 32) {
                            storeOrLoadFloat(spill, RegGeter.AllRegsFloat.get(srcReg - 32), move,  "load", 0);
                        }
                        move.removeFromList();
                    } else {
                        throw new RuntimeException("Invalid Move : 两个都是地址，连续两个Interval都是栈空间：" + move.dst.register.toString());
                    }
                } else {
                    for (AsmOperand opr: ((AsmInstr)instrHead).regDef) {
                        if (opr instanceof AsmVirReg || opr instanceof AsmFVirReg) {
                            if (opr instanceof AsmVirReg) {
                                ((AsmVirReg) opr).color = IntIntervals.get(opr).child_at(instrsId.get((AsmInstr)instrHead)).assignedReg;
                            }
                            if (opr instanceof AsmFVirReg) {
                                ((AsmVirReg) opr).color = FloatIntervals.get(opr).child_at(instrsId.get((AsmInstr)instrHead)).assignedReg;
                            }
                        }
                    }
                }
                instrHead = instrHead.getNext();
            }
        }
    }

    public void debug(AsmModule module) {
        for (AsmFunction function : module.getFunctions()) {
            COMPUTE_BLOCK_ORDER(function);
            NUMBER_OPERATIONS();
            COMPUTE_LOCAL_LIVE_SETS();
            COMPUTE_GLOBAL_LIVE_SETS(function);
            BUILD_INTERVALS();
            initial_(new ArrayList<>(IntIntervals.values()));
            for (Interval interval : unhandled) {
                interval.printRanges();
            }
            System.out.println("###############################################################");
        }
    }

}

