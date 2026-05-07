package ir.pass.opt;

import ir.Value;
import ir.instr.BinaryInstr;
import ir.instr.Br;
import ir.instr.Call;
import ir.instr.Fptosi;
import ir.instr.GetElementPtrInst;
import ir.instr.InstType;
import ir.instr.Instr;
import ir.instr.Load;
import ir.instr.Phi;
import ir.instr.Sitofp;
import ir.instr.Store;
import ir.pass.analyze.DomAnalyzer;
import ir.pass.analyze.Loop;
import ir.pass.analyze.LoopAnalyzer;
import ir.pass.analyze.RemoveUselessUser;
import ir.type.IntType;
import ir.value.BasicBlock;
import ir.value.ConstNumber;
import ir.value.Module;
import ir.value.User;
import ir.value.user.Function;
import tools.ErrOutput;
import tools.IrRegDispatcher;

import javax.print.attribute.standard.JobName;
import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.LinkedList;

public class GVN implements Pass {
    private LinkedHashMap<Integer, Value> computed = new LinkedHashMap<>();
    private LinkedHashMap<Value, Integer> savedHashCode = new LinkedHashMap<>();
    //不能简化的array
    private LinkedHashSet<Value> sideEffectArray = new LinkedHashSet<>();
    private LinkedHashMap<Value, LinkedHashSet<Value>> arrayStore = new LinkedHashMap<>();
    private LinkedHashMap<Value, LinkedHashSet<Value>> pointerStore = new LinkedHashMap<>();
    // TODO: 2023/8/5 双哈希，防止盲点
    public boolean changedCFG = false;
    public boolean needRepeat = false;
    private IrRegDispatcher dispatcher = new IrRegDispatcher();

    private Value getOriArr(GetElementPtrInst inst) {
        Value value = inst;
        while (value instanceof GetElementPtrInst) {
            value = ((GetElementPtrInst) value).getOP(0);
        }
        return value;
    }

    private void initBound(Function func) {
        for (var block : func.blocks) {
            for (var inst : block.getInsts()) {
                inst.oriInitBound();
            }
        }
    }

    public void run(Module module) {
        needRepeat = true;
        while (needRepeat) {
            needRepeat = false;
            BrRemoving brRemoving0 = new BrRemoving();
            brRemoving0.run(module);
            DomAnalyzer.calDomForModule(module);
            sideEffectArray = new LinkedHashSet<>();
            arrayStore = new LinkedHashMap<>();
            pointerStore = new LinkedHashMap<>();
            for (var func : module.functions.values()) {
                if (func.blocks.size() == 0) {
                    continue;
                }
                initBound(func);
                computed = new LinkedHashMap<>();
                computed.put(0, new ConstNumber(0));
                savedHashCode = new LinkedHashMap<>();
                gvnForFunction(func, false);
            }
            for (var pointer : pointerStore.keySet()) {
                if (pointerStore.get(pointer).size() > 1) {
                    sideEffectArray.add(getOriArr((GetElementPtrInst) pointer));
                }
            }
            for (var array : arrayStore.keySet()) {
                if (arrayStore.get(array).size() > 1) {
                    Function current = null;
                    for (var tmp : arrayStore.get(array)) {
                        if (current == null) {
                            current = ((Store) tmp).getParent().getFatherFunction();
                            if (current == null) {
                                ErrOutput.outputErr("error: basicblock father is not BB");
                            }
                        } else if (current != ((Store) tmp).getParent().getFatherFunction()) {
                            sideEffectArray.add(array);
                            break;
                        }
                    }
                }
            }
            for (var func : module.functions.values()) {
                if (func.blocks.size() == 0) {
                    continue;
                }
                initBound(func);
                computed = new LinkedHashMap<>();
                computed.put(0, new ConstNumber(0));
                savedHashCode = new LinkedHashMap<>();
                gvnForFunction(func, true);
            }
            BrRemoving brRemoving = new BrRemoving();
            brRemoving.run(module);
            needRepeat = needRepeat || brRemoving.changedCFG;
            DeadCodeEmit deadCodeEmit = new DeadCodeEmit();
            deadCodeEmit.run(module);
            if (!needRepeat) {
                for (var func : module.functions.values()) {
                    addCombineForFunction(func);
                }
            }
            DeadCodeEmit deadCodeEmit2 = new DeadCodeEmit();
            deadCodeEmit2.run(module);
            RemoveUselessUser.run(module);
            DomAnalyzer.calDomForModule(module);
            LoopAnalyzer.analyzeLoop(module);
            if (!needRepeat) {
                needRepeat = LoopUnroll.run(module);
            }
        }
    }


    private void addCombineForFunction(Function function) {
        for (var block : function.blocks) {
            LinkedList<Value> instrs = new LinkedList<>(block.getInsts());
            int inext = 0;
            for (int i = instrs.size() - 1; i >= 0; i = inext) {
                inext = i - 1;
                Instr instr = (Instr) block.getInsts().get(i);
                LinkedHashMap<Value, Value> needReplace = new LinkedHashMap<>();
                for (var ope : instr.getOperands()) {
                    if (needReplace.containsKey(ope)) {
                        continue;
                    }
                    if (ope instanceof BinaryInstr &&
                        ((BinaryInstr) ope).instType == InstType.BinaryType.add) {
                        if (((BinaryInstr) ope).combineAddcount >= 4) {
                            int userCount = 0;
                            var now = ((BinaryInstr) ((BinaryInstr) ope).combineChildren);
                            while (now != null) {
                                if (now.users.size() == 1) {
                                    userCount++;
                                }
                                now = (BinaryInstr) (now).combineChildren;
                            }
                            if (userCount > 6) {
                                BinaryInstr mul = new BinaryInstr(
                                    new LinkedList<>(Arrays.asList(
                                        new ConstNumber(((BinaryInstr) ope).combineAddcount),
                                        ((BinaryInstr) ope).combineOther
                                    )),
                                    ((BinaryInstr) ope).combineAddori.type,
                                    dispatcher.allocId("mulInAddcombine"),
                                    ((BinaryInstr) ope).combineAddori.type instanceof IntType ?
                                        InstType.BinaryType.mul :
                                        InstType.BinaryType.fmul, block
                                );
                                instrs.add(i, mul);
                                BinaryInstr add = new BinaryInstr(
                                    new LinkedList<>(Arrays.asList(
                                        ((BinaryInstr) ope).combineAddori, mul
                                    )),
                                    mul.type, dispatcher.allocId("addInAddcombine"),
                                    mul.type instanceof IntType ?
                                        InstType.BinaryType.add : InstType.BinaryType.fadd, block
                                );
                                instrs.add(i + 1, add);
                                needReplace.put(ope, add);
                            }
                        }
                    }
                }
                if (needReplace.size() > 0) {
                    needRepeat = true;
                }
                for (var keyValue : needReplace.keySet()) {
                    instr.replaceOp(keyValue, needReplace.get(keyValue));
                }
            }
            block.getInsts().clear();
            block.getInsts().addAll(instrs);
//            System.out.println("addCombine once for block");
        }
    }

    private void gvnForFunction(Function function, boolean GVNforArray) {
        //两轮遍历，由于此时符合LLVMIR的SSA形式，故直接按顺序遍历block就是逆后续遍历，
        // 此时只有phi指令的操作数可能没有遍历过
        //1.---第一轮：遍历所有二元指令和br
        //  1.1.1 BinaryInstr情况
        //  ——1.1.1.1 判断两个OP是否是Const，是的话常量折叠。
        //  ——1.1.1.2 否则范围分析，如果范围塌缩到确定值则直接赋值
        //  ——1.1.1.3 否则分析冗余指令合并
        //  ——1.1.1.4 hashcode为两个操作数的对应hashcode, 调用getValueOfSameLinkedHash(),自动找到对应的value，替换
        //  1.1.2 br情况，判断是否是有条件br，如果有条件，检查条件范围，如果可以优化，则简化br
        //2.---第二轮：遍历所有phi
        // 如果phi的来源已经被计算hash，则判断是否相同，相同的话替换并删除phi
        // 否则不管
        //1--------------------------
        for (var block : function.blocks) {
            for (Value inst : block.getInsts()) {
                if ((inst instanceof BinaryInstr instr)) {
                    Value op1 = instr.getOP(0);
                    Value op2 = instr.getOP(1);
                    boolean needRedundantAnalysis = true;
                    //1.1.1
                    if (((BinaryInstr) inst).getOP(0) instanceof ConstNumber &&
                        ((BinaryInstr) inst).getOP(1) instanceof ConstNumber) {
                        //1.1.1.1常量折叠
                        ConstNumber res = ((BinaryInstr) inst).computeConstResult(
                            (ConstNumber) ((BinaryInstr) inst).getOP(0),
                            (ConstNumber) ((BinaryInstr) inst).getOP(1));
                        inst.replaceAllUsers(res);
                        needRepeat = true;
                    } else {
                        //1.1.1.2范围分析

                        if (instr.instType == InstType.BinaryType.fcmp) {
                            instr.setBound(1, 0);
                        } else if (instr.instType == InstType.BinaryType.icmp) {
                            instr.setBound(1, 0);
                            //        eq,
                            if (instr.cmpType == InstType.CmpType.eq) {
                                if ((op1.hashCode() == op2.hashCode())) {
                                    instr.setBound(1, 1);
                                } else if ((op1.isFixBound() && op2.isFixBound())) {
                                    if (op1.getUpperBound() == op2.getUpperBound()) {
                                        instr.setBound(1, 1);
                                    } else {
                                        instr.setBound(0, 0);
                                    }
                                }
                            }
                            //        ne,
                            else if (instr.cmpType == InstType.CmpType.ne) {
                                if ((op1.hashCode() == op2.hashCode())) {
                                    instr.setBound(0, 0);
                                } else if ((op1.isFixBound() && op2.isFixBound())) {
                                    if (op1.getUpperBound() == op2.getUpperBound()) {
                                        instr.setBound(0, 0);
                                    } else {
                                        instr.setBound(1, 1);
                                    }
                                } else if (op1.getUpperBound() < op2.getLowerBound() ||
                                    op1.getLowerBound() > op2.getUpperBound()) {
                                    instr.setBound(1, 1);
                                }
                            }
                            //        gt,
                            else if (instr.cmpType == InstType.CmpType.gt) {
                                if ((op1.hashCode() == op2.hashCode())) {
                                    instr.setBound(0, 0);
                                } else if ((op1.isFixBound() && op2.isFixBound())) {
                                    if (op1.getUpperBound() > op2.getUpperBound()) {
                                        instr.setBound(1, 1);
                                    } else {
                                        instr.setBound(0, 0);
                                    }
                                } else if (op1.getLowerBound() > op2.getUpperBound()) {
                                    instr.setBound(1, 1);
                                }
                            }
                            //        ge,
                            else if (instr.cmpType == InstType.CmpType.ge) {
                                if ((op1.hashCode() == op2.hashCode())) {
                                    instr.setBound(1, 1);
                                } else if ((op1.isFixBound() && op2.isFixBound())) {
                                    if (op1.getUpperBound() >= op2.getUpperBound()) {
                                        instr.setBound(1, 1);
                                    } else {
                                        instr.setBound(0, 0);
                                    }
                                } else if (op1.getLowerBound() >= op2.getUpperBound()) {
                                    instr.setBound(1, 1);
                                }
                            }
                            //        lt,
                            else if (instr.cmpType == InstType.CmpType.lt) {
                                if ((op1.hashCode() == op2.hashCode())) {
                                    instr.setBound(0, 0);
                                } else if ((op1.isFixBound() && op2.isFixBound())) {
                                    if (op1.getUpperBound() < op2.getUpperBound()) {
                                        instr.setBound(1, 1);
                                    } else {
                                        instr.setBound(0, 0);
                                    }
                                } else if (op1.getUpperBound() < op2.getLowerBound()) {
                                    instr.setBound(1, 1);
                                }
                            }
                            //        le,
                            else if (instr.cmpType == InstType.CmpType.le) {
                                if ((op1.hashCode() == op2.hashCode())) {
                                    instr.setBound(1, 1);
                                } else if ((op1.isFixBound() && op2.isFixBound())) {
                                    if (op1.getUpperBound() <= op2.getUpperBound()) {
                                        instr.setBound(1, 1);
                                    } else {
                                        instr.setBound(0, 0);
                                    }
                                } else if (op1.getUpperBound() <= op2.getLowerBound()) {
                                    instr.setBound(1, 1);
                                }
                            }
                        } else if (instr.type instanceof IntType) {
                            long upper1 = op1.getUpperBound();
                            long upper2 = op2.getUpperBound();
                            long lower1 = op1.getLowerBound();
                            long lower2 = op2.getLowerBound();
                            if (instr.instType == InstType.BinaryType.add) {
                                long predictupper = shrink(upper1 + upper2);
                                long predictlower = shrink(lower1 + lower2);
                                instr.setBound((int) predictupper, (int) predictlower);
                            } else if (instr.instType == InstType.BinaryType.sub) {
                                long predictupper = shrink(upper1 - lower2);
                                long predictlower = shrink(lower1 - upper2);
                                instr.setBound((int) predictupper, (int) predictlower);
                            } else if (instr.instType == InstType.BinaryType.mul) {
                                long a1 = upper1 * upper2;
                                long a2 = upper1 * lower2;
                                long a3 = lower1 * upper2;
                                long a4 = lower1 * lower2;
                                long max = Long.max(a1, a2);
                                max = Long.max(max, a3);
                                max = Long.max(max, a4);
                                max = shrink(max);
                                long min = Long.min(a1, a2);
                                min = Long.min(min, a3);
                                min = Long.min(min, a4);
                                min = shrink(min);
                                instr.setBound((int) max, (int) min);
                            } else if (instr.instType == InstType.BinaryType.sdiv) {
                                ArrayList<Integer> newrange = computeDivRange((int) lower1,
                                    (int) upper1, (int) lower2, (int) upper2);
                                instr.setBound((int) newrange.get(1), (int) newrange.get(0));
                            } else if (instr.instType == InstType.BinaryType.srem) {
                                long max1 = Long.max(Math.abs(lower1), Math.abs(upper1));
                                long max2 = Long.max(Math.abs(lower2), Math.abs(upper2));
                                long minmax = Long.min(max1, max2);
                                long upper = shrink(minmax);
                                long lower = shrink(-minmax);
                                instr.setBound((int) upper, (int) lower);
                            }
                        }
                        if ((!(instr.instType == InstType.BinaryType.fcmp)) &&
                            (instr.type instanceof IntType)
                            && instr.isFixBound()) {
                            needRepeat = true;
                            ConstNumber constNumber = new ConstNumber(instr.getUpperBound());
                            instr.replaceAllUsers(constNumber);
                            needRedundantAnalysis = false;
                        }
                    }
                    if (needRedundantAnalysis &&
                        (instr.instType == InstType.BinaryType.add ||
                            instr.instType == InstType.BinaryType.sub ||
                            instr.instType == InstType.BinaryType.mul ||
                            instr.instType == InstType.BinaryType.sdiv)) {
                        //1.1.1.3冗余指令合并
                        if ((instr.getOP(0) instanceof ConstNumber ||
                            instr.getOP(1) instanceof ConstNumber) && (!(
                            !(instr.getOP(1) instanceof ConstNumber) &&
                                instr.instType == InstType.BinaryType.sdiv
                        ))) {
                            ConstNumber constNumber =
                                ((ConstNumber) (instr.getOP(1) instanceof ConstNumber ?
                                    instr.getOP(1) : instr.getOP(0)));
                            Value secondValue = (instr.getOP(1) instanceof ConstNumber ?
                                instr.getOP(0) : instr.getOP(1));
                            if (secondValue instanceof BinaryInstr &&
                                (((BinaryInstr) secondValue).instType == instr.instType ||
                                    (instr.instType == InstType.BinaryType.add &&
                                        ((BinaryInstr) secondValue).instType ==
                                            InstType.BinaryType.sub) ||
                                    (instr.instType == InstType.BinaryType.sub &&
                                        ((BinaryInstr) secondValue).instType ==
                                            InstType.BinaryType.add))) {
                                BinaryInstr secondInstr = (BinaryInstr) secondValue;
                                if ((secondInstr.getOP(0) instanceof ConstNumber ||
                                    secondInstr.getOP(1) instanceof ConstNumber) && (!(
                                    !(secondInstr.getOP(1) instanceof ConstNumber) &&
                                        instr.instType == InstType.BinaryType.sdiv
                                ))) {

                                    ConstNumber secondConstNumber =
                                        (ConstNumber) (secondInstr.getOP(1) instanceof ConstNumber ?
                                            secondInstr.getOP(1) : secondInstr.getOP(0));
                                    Value secondSecondValue =
                                        (secondInstr.getOP(1) instanceof ConstNumber ?
                                            secondInstr.getOP(0) : secondInstr.getOP(1));
                                    instr.replaceOp(secondValue, secondSecondValue);
                                    ConstNumber newConst =
                                        constNumber.computeWithConstNumber(secondConstNumber,
                                            secondInstr.instType, instr.instType,
                                            secondInstr.getOperands().indexOf(secondConstNumber),
                                            instr.getOperands().indexOf(constNumber), instr);
                                    instr.replaceOp(constNumber, newConst);
                                    needRepeat = true;
                                }
                            }
                        }
                    }
                    //对于
                    needRedundantAnalysis = optMul1(instr, needRedundantAnalysis);
                    needRedundantAnalysis = optAdd0(instr, needRedundantAnalysis);
                    needRedundantAnalysis = optSub0(instr, needRedundantAnalysis);
                    needRedundantAnalysis = optDiv1(instr, needRedundantAnalysis);
                    if (needRedundantAnalysis) {
                        //1.1.1.4
                        Value sameHash = getValueOfSaveHash(inst);
                        if (sameHash != inst) {
                            needRepeat = true;
                            inst.replaceAllUsers(sameHash);
                            needRedundantAnalysis = false;
                        }
                    }
                    if (needRedundantAnalysis && inst instanceof BinaryInstr &&
                        ((BinaryInstr) inst).instType == InstType.BinaryType.add &&
                        ((BinaryInstr) inst).combineAddori == null) {
                        boolean combined = false;
                        if (((BinaryInstr) inst).getOP(0) instanceof BinaryInstr &&
                            ((BinaryInstr) ((BinaryInstr) inst).getOP(0)).instType ==
                                InstType.BinaryType.add) {
                            BinaryInstr a = (BinaryInstr) ((BinaryInstr) inst).getOP(0);
                            Value other = ((BinaryInstr) inst).getOP(1);
                            if (a.getOperands().contains(other)) {
                                combined = true;
                                if (a.combineAddori == null || a.combineAddori.equals(other)) {
                                    ((BinaryInstr) inst).combineAddori =
                                        a.combineAddori == null ? a.getOP(0).equals(other) ?
                                            a.getOP(1) : a.getOP(0) : a.combineAddori;
                                    ((BinaryInstr) inst).combineAddcount = a.combineAddcount + 1;
                                    ((BinaryInstr) inst).combineOther = other;
                                    ((BinaryInstr) inst).combineChildren = a;
                                }
                            }
                        }
                        if (!combined && ((BinaryInstr) inst).getOP(1) instanceof BinaryInstr b &&
                            ((BinaryInstr) ((BinaryInstr) inst).getOP(1)).instType ==
                                InstType.BinaryType.add) {
                            Value other = ((BinaryInstr) inst).getOP(0);
                            if (b.getOperands().contains(other)) {
                                if (b.combineAddori == null || b.combineAddori.equals(other)) {
                                    ((BinaryInstr) inst).combineAddori = b.combineAddori == null ?
                                        b.getOP(0).equals(other) ? b.getOP(1) : b.getOP(0) :
                                        b.combineAddori;
                                    ((BinaryInstr) inst).combineAddcount = b.combineAddcount + 1;
                                    ((BinaryInstr) inst).combineOther = other;
                                    ((BinaryInstr) inst).combineChildren = b;
                                }
                            }
                        }
                    }
                } else if (inst instanceof Br) {
                    if (((Br) inst).getOperands().size() > 1) {
                        Value cond = ((Br) inst).getOP(0);
                        if (cond instanceof ConstNumber) {
                            changedCFG = true;
                            needRepeat = true;
                            var ops = ((Br) inst).getOperands();
                            BasicBlock removedTo = null;
                            if ((Integer) ((ConstNumber) cond).value == 1) {
                                var tmp = ops.get(1);
                                removedTo = (BasicBlock) ops.get(2);
                                ops.clear();
                                ops.add(tmp);
                            } else {
                                var tmp = ops.get(2);
                                removedTo = (BasicBlock) ops.get(1);
                                ops.clear();
                                ops.add(tmp);
                            }
                            ArrayList<Phi> needRemovedPhi = new ArrayList<>();
                            for (var subinstr : removedTo.getInsts()) {
                                if (subinstr instanceof Phi) {
                                    ((Phi) subinstr).checkAndRemoveBlock(block);
                                    if (((Phi) subinstr).values.size() == 1) {
                                        needRemovedPhi.add((Phi) subinstr);
                                    }
                                }
                            }
                            for (var subinstr : needRemovedPhi) {
                                Value commingvalue = null;
                                for (Value tmpvalue : ((Phi) subinstr).values.values()) {
                                    commingvalue = tmpvalue;
                                }
                                subinstr.replaceAllUsers(commingvalue);
                                ((Phi) subinstr).delete();
                                removedTo.getInsts().remove(subinstr);
                            }
                        }
                    }
                } else if (inst instanceof GetElementPtrInst) {
                    if (((GetElementPtrInst) inst).getOP(0) instanceof GetElementPtrInst) {
                        GetElementPtrInst getptr = ((GetElementPtrInst) inst);
                        Value subgetptr = getptr.getOP(0);
                        while (subgetptr instanceof GetElementPtrInst) {
                            if(((GetElementPtrInst) subgetptr).getOP(
                                    ((GetElementPtrInst) subgetptr).getOperands().size() -
                                        1) instanceof ConstNumber &&
                                    (int) (((ConstNumber) ((GetElementPtrInst) subgetptr).getOP(
                                        ((GetElementPtrInst) subgetptr).getOperands().size() - 1)).value) ==
                                        0){
                                getptr.replaceOp(subgetptr, ((GetElementPtrInst) subgetptr).getOP(0));
                                for (int i = ((GetElementPtrInst) subgetptr).getOperands().size() - 2;
                                     i >= 1; i--) {
                                    getptr.addOperand(1, ((GetElementPtrInst) subgetptr).getOP(i));
                                }
                                subgetptr = ((GetElementPtrInst) subgetptr).getOP(0);
                            }else if(getptr.getOP(1) instanceof ConstNumber &&
                            ((ConstNumber) getptr.getOP(1)).value.equals(0)){
                                getptr.replaceOp(subgetptr, ((GetElementPtrInst) subgetptr).getOP(0));
                                getptr.getOperands().remove(1);
                                for (int i = ((GetElementPtrInst) subgetptr).getOperands().size() - 1;
                                     i >= 1; i--) {
                                    getptr.addOperand(1, ((GetElementPtrInst) subgetptr).getOP(i));
                                }
                                subgetptr = ((GetElementPtrInst) subgetptr).getOP(0);
                            }
                        }
                    }
                    for (int i = 1; i < ((GetElementPtrInst) inst).getOperands().size(); i++) {
                        if (!(((GetElementPtrInst) inst).getOP(i) instanceof ConstNumber)) {
                            var arr = getOriArr((GetElementPtrInst) inst);
                            sideEffectArray.add(arr);
                        }
                    }
                    Value sameHash = getValueOfSaveHash(inst);
                    if (sameHash != inst) {
                        needRepeat = true;
                        inst.replaceAllUsers(sameHash);
                    }
                } else if (inst instanceof Store) {
                    var pointer = ((Store) inst).getDest();
                    if (!GVNforArray && pointer instanceof GetElementPtrInst) {
                        var arr = getOriArr((GetElementPtrInst) pointer);
                        if (!arrayStore.containsKey(arr)) {
                            arrayStore.put(arr, new LinkedHashSet<>());
                        }
                        arrayStore.get(arr).add(inst);
                        while (((GetElementPtrInst) pointer).getOP(
                            0) instanceof GetElementPtrInst) {
                            pointer = ((GetElementPtrInst) pointer).getOP(0);
                        }
                        if (!pointerStore.containsKey(pointer)) {
                            pointerStore.put(pointer, new LinkedHashSet<>());
                        }
                        pointerStore.get(pointer).add(inst);
                    }
                } else if (GVNforArray && inst instanceof Load) {
                    var pointer = ((Load) inst).getOP(0);
                    if (pointer instanceof GetElementPtrInst &&
                        !sideEffectArray.contains(getOriArr((GetElementPtrInst) pointer))) {
                        if (pointerStore.containsKey(pointer)) {
                            var store = pointerStore.get(pointer).iterator().next();
                            if (((Store) store).getParent().getDominate().contains(
                                ((Load) inst).getParent()
                            )) {
                                if (!(((Store) store).getParent() == ((Load) inst).getParent() &&
                                    ((Store) store).getParent().getInsts().indexOf(store) >
                                        ((Store) store).getParent().getInsts().indexOf(inst))) {
                                    inst.replaceAllUsers(((Store) store).getVal());
                                }
                            }
                        }
                    }
                } else if (!GVNforArray && inst instanceof Call) {
                    for (var ope : ((Call) inst).getOperands()) {
                        var tmp = ope;
                        while (tmp instanceof GetElementPtrInst) {
                            tmp = ((GetElementPtrInst) tmp).getOP(0);
                        }
                        sideEffectArray.add(tmp);
                    }
                } else if (inst instanceof Fptosi) {
                    if (((Fptosi) inst).getOP(0) instanceof ConstNumber) {
                        inst.replaceAllUsers(new ConstNumber(
                            (int) (float) (double) (
                                ((ConstNumber) ((Fptosi) inst).getOP(0)).value
                            )
                        ));
                    }
                } else if (inst instanceof Sitofp) {
                    if (((Sitofp) inst).getOP(0) instanceof ConstNumber) {
                        inst.replaceAllUsers(new ConstNumber(
                            (double) (int) ((ConstNumber) ((Sitofp) inst).getOP(0)).value
                        ));
                    }
                }
            }
        }
        for (var block : function.blocks) {
            for (Value inst : block.getInsts()) {
                if (inst instanceof Phi) {
                    Value tmp = ((Phi) inst).getOP(0);
                    boolean same = true;
                    for (var used : ((Phi) inst).getOperands()) {
                        if (used != tmp) {
                            same = false;
                            break;
                        }
                    }
                    if (same) {
                        inst.replaceAllUsers(tmp);
                    }
                }
            }
        }
    }

    boolean optDiv1(BinaryInstr instr, Boolean needRedundantAnalysis){
        if (needRedundantAnalysis && instr.instType == InstType.BinaryType.sdiv) {
            if (instr.getOP(1) instanceof ConstNumber) {
                if (((int) ((ConstNumber) instr.getOP(1)).value) == 1) {
                    instr.replaceAllUsers(instr.getOP(0));
                    needRedundantAnalysis = false;
                    needRepeat = true;
                } else if (((int) ((ConstNumber) instr.getOP(1)).value) == -1) {
                    instr.instType = InstType.BinaryType.sub;
                    var ops = instr.getOperands();
                    ops.set(1, ops.get(0));
                    ops.set(0, new ConstNumber(0));
                    needRepeat = true;
                }
            }
        }
        return needRedundantAnalysis;
    }
    boolean optSub0(BinaryInstr instr, Boolean needRedundantAnalysis){
        if (needRedundantAnalysis && instr.instType == InstType.BinaryType.sub) {
            if (instr.getOP(1) instanceof ConstNumber &&
                (Integer) ((ConstNumber) instr.getOP(1)).value == 0) {
                instr.replaceAllUsers(instr.getOP(0));
                needRedundantAnalysis = false;
                needRepeat = true;
            }
        }
        return needRedundantAnalysis;
    }

    boolean optAdd0(BinaryInstr instr, Boolean needRedundantAnalysis){
        if (needRedundantAnalysis && instr.instType == InstType.BinaryType.add) {
            if (instr.getOP(0) instanceof ConstNumber) {
                var opes = instr.getOperands();
                var tmp = opes.get(0);
                opes.set(0, opes.get(1));
                opes.set(1, tmp);
            }
            if (instr.getOP(1) instanceof ConstNumber &&
                (Integer) ((ConstNumber) instr.getOP(1)).value == 0) {
                instr.replaceAllUsers(instr.getOP(0));
                needRedundantAnalysis = false;
                needRepeat = true;
            }

            if(!needRepeat){
                if (!(instr.getOP(0) instanceof BinaryInstr && ((BinaryInstr) instr.getOP(0)).instType == InstType.BinaryType.sub)) {
                    var opes = instr.getOperands();
                    var tmp = opes.get(0);
                    opes.set(0, opes.get(1));
                    opes.set(1, tmp);
                }
                if(instr.getOP(0) instanceof BinaryInstr && ((BinaryInstr) instr.getOP(0)).instType == InstType.BinaryType.sub){
                    BinaryInstr sub = (BinaryInstr) instr.getOP(0);
                    if(sub.getOP(0) instanceof ConstNumber && ((ConstNumber) sub.getOP(0)).value.equals(0)){
                        var opes = instr.getOperands();
                        var tmp = opes.get(0);
                        opes.set(0, opes.get(1));
                        opes.set(1, tmp);
                        instr.replaceOp(instr.getOP(1), sub.getOP(1));
                        instr.instType = InstType.BinaryType.sub;
                        needRepeat = true;
                    }
                }
            }
        }
        return needRedundantAnalysis;
    }

    boolean optMul1(BinaryInstr instr, Boolean needRedundantAnalysis){
        if (needRedundantAnalysis && instr.instType == InstType.BinaryType.mul) {
            if (instr.getOP(0) instanceof ConstNumber) {
                var opes = instr.getOperands();
                var tmp = opes.get(0);
                opes.set(0, opes.get(1));
                opes.set(1, tmp);
            }
            if (instr.getOP(1) instanceof ConstNumber) {
                if (((int) ((ConstNumber) instr.getOP(1)).value) == 1) {
                    instr.replaceAllUsers(instr.getOP(0));
                    needRedundantAnalysis = false;
                    needRepeat = true;
                } else if (((int) ((ConstNumber) instr.getOP(1)).value) == -1) {
                    instr.instType = InstType.BinaryType.sub;
                    var ops = instr.getOperands();
                    ops.set(1, ops.get(0));
                    ops.set(0, new ConstNumber(0));
                    needRepeat = true;
                }
            }
        }
        return needRedundantAnalysis;
    }

    ArrayList<Integer> mergeRange(ArrayList<Integer> range1, ArrayList<Integer> range2) {
        if (range1 == null) {
            return range2;
        }
        if (range2 == null) {
            return range1;
        }
        int upper = Integer.max(range1.get(1), range2.get(1));
        int lower = Integer.min(range1.get(0), range2.get(0));
        var res = new ArrayList<Integer>();
        res.add(lower);
        res.add(upper);
        return res;
    }

    ArrayList<Integer> computeDivRange(int al, int au, int bl, int bu) {
        if (bl > bu) {
            return null;
        }
        if (bl <= 0 && bu >= 0) {
            return mergeRange(computeDivRange(al, au, bl, -1), computeDivRange(al, au, 1, bu));
        } else if (bu < 0) {
            return computeDivRange(-au, -al, -bu, -bl);
        }
        ArrayList<Integer> range1 = new ArrayList<>();
        range1.add(al / bl);
        range1.add(au / bl);
        ArrayList<Integer> range2 = new ArrayList<>();
        range2.add(al / bu);
        range2.add(au / bu);
        return mergeRange(range1, range2);
    }


    private Value getValueOfSaveHash(Value value) {
        int hash = getHashCodeOfValue(value);
        if (!computed.containsKey(hash)) {
            computed.put(hash, value);
            return value;
        } else {
            return computed.get(hash);
        }
    }

    private int getHashCodeOfValue(Value value) {
        if (!savedHashCode.containsKey(value)) {
            computeHashCodeOfValue(value);
        }
        return savedHashCode.get(value);
    }

    private void computeHashCodeOfValue(Value value) {
        if (value instanceof BinaryInstr) {
            if (((BinaryInstr) value).instType == InstType.BinaryType.add) {
                savedHashCode.put(value, getHashCodeOfValue(((BinaryInstr) value).getOP(0)) +
                    getHashCodeOfValue(((BinaryInstr) value).getOP(1)));
            } else if (((BinaryInstr) value).instType == InstType.BinaryType.sub) {
                savedHashCode.put(value, getHashCodeOfValue(((BinaryInstr) value).getOP(0)) -
                    getHashCodeOfValue(((BinaryInstr) value).getOP(1)));
            } else if (((BinaryInstr) value).instType == InstType.BinaryType.mul) {
                savedHashCode.put(value,
                    (int) (((long) getHashCodeOfValue(((BinaryInstr) value).getOP(0)))%998244353 *
                        getHashCodeOfValue(((BinaryInstr) value).getOP(1))%998244353) % 998244353);
            } else {
                int hashres = 0;
                for (var ope : ((BinaryInstr) value).getOperands()) {
                    hashres = (131 * hashres + getHashCodeOfValue(ope) + 19260817) % 998244353;
                }
                savedHashCode.put(value, hashres +
                    ((BinaryInstr) value).instType.hashCode() +
                    ((BinaryInstr) value).cmpType.hashCode());
            }
        } else if (value instanceof ConstNumber) {
            savedHashCode.put(value, ((ConstNumber) value).value.hashCode());
        } else if (value instanceof GetElementPtrInst) {
            int hashres = 0;
            for (var ope : ((GetElementPtrInst) value).getOperands()) {
                hashres = (131 * hashres + getHashCodeOfValue(ope) + 19260817) % 998244353;
//                hashres = ()
            }
            savedHashCode.put(value, hashres);
        } else {
            savedHashCode.put(value, value.hashCode());
        }
    }

    private long shrink(long value) {
        if (value > Integer.MAX_VALUE) {
            return Integer.MAX_VALUE;
        } else if (value < Integer.MIN_VALUE) {
            return Integer.MIN_VALUE;
        }
        return value;
    }

}
