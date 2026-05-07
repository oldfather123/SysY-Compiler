package frontend.ir.cloner;

import frontend.ir.Value;
import frontend.ir.constant.IRConst;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.*;
import frontend.ir.instr.convop.*;
import frontend.ir.instr.memop.*;
import frontend.ir.instr.otherop.*;
import frontend.ir.instr.terminator.BranchInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.instr.terminator.Terminator;
import frontend.ir.instr.unaryop.FNegInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.symbol.GlbObjPtr;

import java.util.*;

public class Cloner {
    private static Cloner instance = null;

    protected static final HashMap<Value, Value> valueProjection = new HashMap<>();
    protected static final HashMap<EmptyInstr, PhiInstr> needPhi = new HashMap<>();

    protected Cloner() {
    }

    public static Cloner getInstance() {
        if (instance == null) {
            instance = new Cloner();
        }
        return instance;
    }

    /**
     * 得值器，取值依次为
     * 1. 常量：直接克隆
     * 2. 全局变量：直接返回
     * 3. 其他：从valueProjection中获取，拿不到就返回原值
     */
    protected static Value newValueGetter(Value val) {
        if (val instanceof IRConst con) {
            return con.clone();
        } else if (val instanceof GlbObjPtr glbObjPtr) {
            return glbObjPtr;
        } else return valueProjection.getOrDefault(val, val);
    }

    protected void cloneInstr(Instruction instruction, BasicBlock targetBB, Function targetFunc) {
        switch (instruction) {
            case BinaryOperation bin -> {
                Value op1, op2;
                Instruction newInstr;
                op1 = newValueGetter(bin.getOperand1());
                op2 = newValueGetter(bin.getOperand2());
                switch (instruction) {
                    case AddInstr _ -> newInstr = new AddInstr(targetFunc.getAndAddRegIdx(), op1, op2, targetBB);
                    case FAddInstr _ -> newInstr = new FAddInstr(targetFunc.getAndAddRegIdx(), op1, op2, targetBB);
                    case SubInstr _ -> newInstr = new SubInstr(targetFunc.getAndAddRegIdx(), op1, op2, targetBB);
                    case FSubInstr _ -> newInstr = new FSubInstr(targetFunc.getAndAddRegIdx(), op1, op2, targetBB);
                    case MulInstr _ -> newInstr = new MulInstr(targetFunc.getAndAddRegIdx(), op1, op2, targetBB);
                    case FMulInstr _ -> newInstr = new FMulInstr(targetFunc.getAndAddRegIdx(), op1, op2, targetBB);
                    case SDivInstr _ -> newInstr = new SDivInstr(targetFunc.getAndAddRegIdx(), op1, op2, targetBB);
                    case FDivInstr _ -> newInstr = new FDivInstr(targetFunc.getAndAddRegIdx(), op1, op2, targetBB);
                    case SRemInstr _ -> newInstr = new SRemInstr(targetFunc.getAndAddRegIdx(), op1, op2, targetBB);
                    case FRemInstr _ -> newInstr = new FRemInstr(targetFunc.getAndAddRegIdx(), op1, op2, targetBB);
                    default -> throw new IllegalStateException("Unexpected value: " + instruction);
                }
                valueProjection.put(bin, newInstr);
            }
            case ConversionOperation conv -> {
                Value op;
                Instruction newInstr;
                op = newValueGetter(conv.getValue());
                switch (instruction) {
                    case Bitcast _ -> newInstr = new Bitcast(targetFunc.getAndAddRegIdx(), op, targetBB);
                    case Fp2Si _ -> newInstr = new Fp2Si(targetFunc.getAndAddRegIdx(), op, targetBB);
                    case Si2Fp _ -> newInstr = new Si2Fp(targetFunc.getAndAddRegIdx(), op, targetBB);
                    case TruncInstr _ -> newInstr = new TruncInstr(targetFunc.getAndAddRegIdx(), op, targetBB);
                    case ZextInstr _ -> newInstr = new ZextInstr(targetFunc.getAndAddRegIdx(), op, targetBB);
                    default -> throw new IllegalStateException("Unexpected value: " + instruction);
                }
                valueProjection.put(conv, newInstr);
            }
            case MemoryOperation mem -> {
                Instruction newInstr;
                switch (mem) {
                    case AllocaInstr alloca -> {
                        newInstr = new AllocaInstr(alloca.getReferencedType(), targetFunc.getAndAddRegIdx(), targetBB);
                        valueProjection.put(alloca, newInstr);
                    }
                    case GEPInstr gep -> {
                        Value ptrVal = newValueGetter(gep.getPtrVal());
                        List<Value> indexList = new ArrayList<>();
                        for (Value index : gep.getIndexList()) {
                            indexList.add(newValueGetter(index));
                        }
                        newInstr = new GEPInstr(targetFunc.getAndAddRegIdx(), indexList, ptrVal, targetBB);
                        valueProjection.put(gep, newInstr);
                    }
                    case LoadInstr load -> {
                        Value pointer = newValueGetter(load.getPointer());
                        newInstr = new LoadInstr(targetFunc.getAndAddRegIdx(), load.getType(), pointer, targetBB);
                        valueProjection.put(load, newInstr);
                    }
                    case StoreInstr store -> {
                        Value valueToStore = newValueGetter(store.getValueToStore());
                        Value pointer = newValueGetter(store.getPointer());
                        newInstr = new StoreInstr(valueToStore, pointer, targetBB);
                        valueProjection.put(store, newInstr);
                    }
                    default -> throw new IllegalStateException("Unexpected value: " + mem);
                }
            }
            case CallInstr call -> {
                List<Value> args = new ArrayList<>();
                for (Value arg : call.getRParams()) {
                    args.add(newValueGetter(arg));
                }
                Instruction newInstr = new CallInstr(call.isVoidCall() ? null : targetFunc.getAndAddRegIdx(),
                        call.getCallee(), args, targetBB);
                if (!call.isVoidCall()) {
                    valueProjection.put(call, newInstr);
                }
            }
            case CmpInstr cmp -> {
                Value op1 = newValueGetter(cmp.getOperand1());
                Value op2 = newValueGetter(cmp.getOperand2());
                Instruction newInstr = new CmpInstr(targetFunc.getAndAddRegIdx(),
                        cmp.getCond(), op1, op2, targetBB);
                valueProjection.put(cmp, newInstr);
            }
            case EmptyInstr empty -> new EmptyInstr(empty.getType(), targetBB);
            case PhiInstr phi -> {
                EmptyInstr empty = new EmptyInstr(phi.getType(), targetBB);
                valueProjection.put(phi, empty);
                needPhi.put(empty, phi);
            }
            case Terminator terminator -> {
                switch (terminator) {
                    case BranchInstr branch -> {
                        Value cond = newValueGetter(branch.getCondition());
                        BasicBlock trueBB = (BasicBlock) newValueGetter(branch.getThenBlk());
                        BasicBlock falseBB = (BasicBlock) newValueGetter(branch.getElseBlk());
                        new BranchInstr(cond, trueBB, falseBB, targetBB);
                    }
                    case JumpInstr jump -> {
                        BasicBlock target = (BasicBlock) newValueGetter(jump.getTarget());
                        new JumpInstr(target, targetBB);
                    }
                    default -> throw new IllegalStateException("Unexpected value: " + terminator);
                }
            }
            case FNegInstr fNeg -> {
                Value op = newValueGetter(fNeg.getValue());
                Instruction newInstr = new FNegInstr(targetFunc.getAndAddRegIdx(), op, targetBB);
                valueProjection.put(fNeg, newInstr);
            }
            case AtomaticAddInstr atomaticAdd -> {
                Value ptr = newValueGetter(atomaticAdd.getPtr());
                Value inc = newValueGetter(atomaticAdd.getInc());
                Instruction newInstr = new AtomaticAddInstr(ptr, inc, targetBB);
                valueProjection.put(atomaticAdd, newInstr);
            }

            default -> throw new IllegalStateException("Unexpected value: " + instruction);
        }
    }

    protected void cloneBlockDfs(BasicBlock srcBB, BasicBlock targetBB, Function targetFunc) {
        if (!targetBB.isEmpty()) {
            return;
        }
        for (Instruction instr : srcBB.getInstrList()) {
            cloneInstr(instr, targetBB, targetFunc);
        }
        for (BasicBlock sucBB : srcBB.getSucs()) {
            cloneBlockDfs(sucBB, (BasicBlock) valueProjection.get(sucBB), targetFunc);
        }
    }

    protected PhiInstr phiInstrCreator(
            PhiInstr oriPhi, EmptyInstr empty, Function targetFunc
    ) {
        PhiInstr newPhi = new PhiInstr(oriPhi.getType(), targetFunc.getAndAddRegIdx(), empty.getParentBB(), oriPhi.isLCSSA());
        for (Map.Entry<BasicBlock, Value> opBB : oriPhi.getOperandMap().entrySet()) {
            Value bb = newValueGetter(opBB.getKey());
            Value op = newValueGetter(opBB.getValue());
            newPhi.addOperand((BasicBlock) bb, op);
        }
        empty.replaceUseWith(newPhi);
        newPhi.insertAfter(empty);
        empty.removeFromList();
        return newPhi;
    }

    /**
     * 标准phi回填器，利用已经构建好的deedPhi，使用newValueGetter得到的值行回填
     */
    public void makePhi(Function targetFunc) {
        for (Map.Entry<EmptyInstr, PhiInstr> entry : needPhi.entrySet()) {
            EmptyInstr empty = entry.getKey();
            PhiInstr phi = entry.getValue();
//            PhiInstr newPhi = new PhiInstr(phi.getType(), targetFunc.getAndAddRegIdx(), empty.getParentBB(), phi.isLCSSA());
//            for (Map.Entry<BasicBlock, Value> opBB : phi.getOperandMap().entrySet()) {
//                Value bb = newValueGetter(opBB.getKey());
//                Value op = newValueGetter(opBB.getValue());
//                newPhi.addOperand((BasicBlock) bb, op);
//            }
//            empty.replaceUseWith(newPhi);
//            newPhi.insertAfter(empty);
//            empty.removeFromList();
            PhiInstr newPhi = phiInstrCreator(phi, empty, targetFunc);
            valueProjection.put(phi, newPhi);
        }
    }

}
