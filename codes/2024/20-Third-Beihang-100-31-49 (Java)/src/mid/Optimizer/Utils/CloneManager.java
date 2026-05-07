package mid.Optimizer.Utils;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.ALU;
import mid.IntermediatePresentation.Instruction.Alloca;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Instruction.Call;
import mid.IntermediatePresentation.Instruction.Cmp;
import mid.IntermediatePresentation.Instruction.Fptosi;
import mid.IntermediatePresentation.Instruction.GetElementPtr;
import mid.IntermediatePresentation.Instruction.GlobalDecl;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Load;
import mid.IntermediatePresentation.Instruction.Phi;
import mid.IntermediatePresentation.Instruction.Ret;
import mid.IntermediatePresentation.Instruction.Shift;
import mid.IntermediatePresentation.Instruction.Sitofp;
import mid.IntermediatePresentation.Instruction.Store;
import mid.IntermediatePresentation.Instruction.ZextTo;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;
import mid.Optimizer.Optimizer;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

public class CloneManager {
    private final HashMap<Value, Value> valMap = new HashMap<>();
    private final HashSet<Phi> phiToBeFill = new HashSet<>();

    private final HashSet<Alloca> allocasToBeMove = new HashSet<>();

    private final Function f;

    public CloneManager(Function f) {
        this.f = f;
    }

    public void put(Value u, Value v) {
        valMap.put(u, v);
    }

    public Value get(Value u) {
        return getVal(u);
    }

    public Instruction getNewInstruction(Instruction instruction) {
        Instruction newInstruction = null;
        if (instruction instanceof ALU alu) {
            newInstruction = new ALU(getVal(alu.getOperand1()),
                    alu.getOperator(), getVal(alu.getOperand2()), !alu.isFloat());
        } else if (instruction instanceof Br br) {
            if (br.getDest() == null) {
                newInstruction = new Br(getVal(br.getCond()), (BasicBlock) getVal(br.getIfTrue()),
                        (BasicBlock) getVal(br.getIfFalse()));
            } else {
                newInstruction = new Br((BasicBlock) getVal(br.getDest()));
            }
        } else if (instruction instanceof GetElementPtr gep) {
            newInstruction = new GetElementPtr(getVal(gep.getPtr()),
                    getVal(gep.getElemIndex()));
        } else if (instruction instanceof Cmp cmp) {
            newInstruction = new Cmp(cmp.getOperator(), getVal(cmp.getOperandList().get(0)),
                    getVal(cmp.getOperandList().get(1)));
        } else if (instruction instanceof Load load) {
            newInstruction = new Load(IRManager.getInstance().declareLocalVar(),
                    getVal(load.getAddr()));
        } else if (instruction instanceof Alloca alloca) {
            if (!alloca.getType().equals(ValueType.ARRAY)) {
                newInstruction = new Alloca(!alloca.isFloat());
            } else {
                newInstruction = new Alloca(alloca.getLen(), !alloca.isFloat());
            }
            allocasToBeMove.add((Alloca) newInstruction);
        } else if (instruction instanceof Phi aphi) {
            newInstruction = new Phi(!aphi.isFloat(),
                    IRManager.getInstance().declareLocalVar());
            phiToBeFill.add(aphi);
        } else if (instruction instanceof Ret ret) {
            newInstruction = new Ret(getVal(ret.getRetValue()), !ret.isFloat());
        } else if (instruction instanceof Shift shift) {
            newInstruction = new Shift(shift.isShiftRight(),
                    getVal(shift.getOperandList().get(0)),
                    (ConstNumber) shift.getOperandList().get(1));
            ((Shift) newInstruction).setLogicalShiftRight(shift.isLogicalShiftRight());
        } else if (instruction instanceof Store store) {
            newInstruction = new Store(getVal(store.getSrc()),
                    getVal(store.getAddr()));
        } else if (instruction instanceof ZextTo zextTo) {
            newInstruction = new ZextTo(getVal(zextTo.getOperandList().get(0)), zextTo.getType());
        } else if (instruction instanceof Call call) {
            ArrayList<Value> rParams = new ArrayList<>();
            for (Value para : call.getOperandList()) {
                if (!(para instanceof Function)) {
                    rParams.add(getVal(para));
                }
            }
            newInstruction = new Call(call.getCallingFunction(), rParams);
        } else if (instruction instanceof Sitofp sitofp) {
            newInstruction = new Sitofp(getVal(sitofp.getOperandList().get(0)));
        } else if (instruction instanceof Fptosi fptosi) {
            newInstruction = new Fptosi(getVal(fptosi.getOperandList().get(0)));
        }
        return newInstruction;
    }

    public void cloneEnd() {
        //等到克隆完成了，再去做phi的填充，以防出现未定义的问题
        if (!phiToBeFill.isEmpty()) {
            Function f = ((Phi) get((Value) phiToBeFill.toArray()[0])).getBlock().getFunction();
            Optimizer.instance().cfgAnalyze(f);
            for (Phi originPhi : phiToBeFill) {
                Phi phiToFill = (Phi) get(originPhi);
                HashSet<BasicBlock> parents = Optimizer.instance().getCFG().getParents(phiToFill.getBlock());
                for (int i = 0; i < originPhi.getOperandList().size(); i += 2) {
                    BasicBlock parent = (BasicBlock) get(originPhi.getOperandList().get(i + 1));
                    if (parents != null && parents.contains(parent)) {
                        phiToFill.addCond(get(originPhi.getOperandList().get(i)), parent);
                    }
                }
            }
        }

        // 处理所有克隆的alloca，移动至函数首部
        BasicBlock entry = f.getEntranceBlock();
        for (Alloca alloca : allocasToBeMove) {
            alloca.getBlock().removeInstruction(alloca);
            entry.addInstrAtEntry(alloca);
        }
    }

    private Value getVal(Value key) {
        if (key instanceof ConstNumber || key instanceof GlobalDecl) {
            return key;
        } else return valMap.getOrDefault(key, key);
    }
}
