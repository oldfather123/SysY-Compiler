package cn.edu.bit.newnewcc.backend.asm.controller;

import cn.edu.bit.newnewcc.backend.asm.AsmFunction;
import cn.edu.bit.newnewcc.backend.asm.allocator.StackAllocator;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmCall;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmInstruction;
import cn.edu.bit.newnewcc.backend.asm.operand.*;
import cn.edu.bit.newnewcc.backend.asm.util.AsmInstructions;
import cn.edu.bit.newnewcc.backend.asm.util.Registers;
import org.antlr.v4.runtime.misc.Pair;

import java.util.*;

/**
 * 线性扫描寄存器分配器
 */
public class LinearScanRegisterControlR1 extends RegisterControl{
    protected final IntRegister addressReg = IntRegister.getPhysical(5);
    protected final IntRegister t1 = IntRegister.getPhysical(6), t2 = IntRegister.getPhysical(7);
    protected final FloatRegister ft1 = FloatRegister.getPhysical(1), ft2 = FloatRegister.getPhysical(2);
    //每个虚拟寄存器的值当前存储的位置
    private final Map<Integer, AsmOperand> vRegLocation = new HashMap<>();
    //每个寄存器当前存储着哪个虚拟寄存器的内容
    private final Map<Register, Integer> registerPool = new HashMap<>();

    public LinearScanRegisterControlR1(AsmFunction function, StackAllocator allocator) {
        super(function, allocator);
        //加入目前可使用的寄存器
        for (var reg : Registers.USABLE_REGISTERS) {
            registerPool.put(reg, 0);
        }
        registerPool.remove(addressReg);
        registerPool.remove(t1);
        registerPool.remove(t2);
        registerPool.remove(ft1);
        registerPool.remove(ft2);
    }

    private void allocateVReg(Register vReg) {
        var index = vReg.getAbsoluteIndex();
        for (var reg : registerPool.keySet()) {
            if (reg.getClass() == vReg.getClass() && registerPool.get(reg) == 0) {
                registerPool.put(reg, vReg.getAbsoluteIndex());
                vRegLocation.put(vReg.getAbsoluteIndex(), reg);
                updateRegisterPreserve(reg);
                return;
            }
        }
        Register spillReg = null;
        int maxR = function.getLifeTimeController().getLifeTimeRange(index).b.getInstID();
        for (var reg : registerPool.keySet()) {
            if (reg.getClass() == vReg.getClass()) {
                int original = registerPool.get(reg);
                int regR = function.getLifeTimeController().getLifeTimeRange(original).b.getInstID();
                if (regR > maxR) {
                    maxR = regR;
                    spillReg = reg;
                }
            }
        }
        if (spillReg != null) {
            int original = registerPool.get(spillReg);
            vRegLocation.remove(original);
            registerPool.put(spillReg, index);
            vRegLocation.put(index, spillReg);
        }
    }

    /**
     * 进行两次线性扫描，第一次分配寄存器并进行spill操作，第二次为spill操作后的寄存器分配栈空间
     */
    public void virtualRegAllocateToPhysics() {
        List<Register> vRegList = new ArrayList<>();
        List<Pair<Integer, Integer>> recycleList = new ArrayList<>();
        for (var index : function.getLifeTimeController().getVRegKeySet()) {
            vRegList.add(function.getRegisterAllocator().get(index));
            recycleList.add(new Pair<>(index, function.getLifeTimeController().getLifeTimeRange(index).b.getInstID()));
        }
        vRegList.sort((a, b) -> {
            var lifeTimeA = function.getLifeTimeController().getLifeTimeRange(a.getAbsoluteIndex());
            var lifeTimeB = function.getLifeTimeController().getLifeTimeRange(b.getAbsoluteIndex());
            return lifeTimeA.compareTo(lifeTimeB);
        });
        recycleList.sort(Comparator.comparingInt(a -> a.b));

        //第一次扫描，分配寄存器本身
        int recycleHead = 0;
        for (var vReg : vRegList) {
            var lifeTime = function.getLifeTimeController().getLifeTimeRange(vReg.getAbsoluteIndex());
            while (recycleHead < recycleList.size() && recycleList.get(recycleHead).b <= lifeTime.a.getInstID()) {
                recycle(recycleList.get(recycleHead).a);
                recycleHead += 1;
            }
            allocateVReg(vReg);
        }

        //第二次扫描，分配被spill的寄存器的栈空间
        recycleHead = 0;
        for (var vReg : vRegList) {
            if (vRegLocation.containsKey(vReg.getAbsoluteIndex())) {
                continue;
            }
            var lifeTime = function.getLifeTimeController().getLifeTimeRange(vReg.getAbsoluteIndex());
            while (recycleHead < recycleList.size() && recycleList.get(recycleHead).b <= lifeTime.a.getInstID()) {
                recycle(recycleList.get(recycleHead).a);
                recycleHead += 1;
            }
            vRegLocation.put(vReg.getAbsoluteIndex(), stackPool.pop());
        }
        stackPool.clear();
    }

    List<AsmInstruction> getLoad(Map<Register, Register> load, Register vReg, StackVar stackVar) {
        if (vReg instanceof IntRegister) {
            if (load.containsValue(t1)) {
                load.put(vReg, t2);
                return loadFromStackVar(t2, stackVar, addressReg);
            } else {
                load.put(vReg, t1);
                return loadFromStackVar(t1, stackVar, addressReg);
            }
        } else {
            if (load.containsValue(ft1)) {
                load.put(vReg, ft2);
                return loadFromStackVar(ft2, stackVar, addressReg);
            } else {
                load.put(vReg, ft1);
                return loadFromStackVar(ft1, stackVar, addressReg);
            }
        }
    }

    public List<AsmInstruction> spillRegisters(List<AsmInstruction> instructionList) {
        registerPool.replaceAll((r, v) -> 0);
        List<AsmInstruction> newInstList = new ArrayList<>();
        List<List<Pair<Integer, Integer>>> callSavedRegisters = new ArrayList<>();
        for (int i = 0; i <= instructionList.size() + 1; i++) {
            callSavedRegisters.add(new ArrayList<>());
        }
        for (var index : function.getLifeTimeController().getVRegKeySet()) {
            if (vRegLocation.get(index) instanceof Register) {
                var lifeTime = function.getLifeTimeController().getLifeTimeRange(index);
                callSavedRegisters.get(lifeTime.a.getInstID()).add(new Pair<>(index, 1));
                callSavedRegisters.get(lifeTime.b.getInstID() + 1).add(new Pair<>(index, -1));
            }
        }
        for (int i = 0; i < instructionList.size(); i++) {
            for (var p : callSavedRegisters.get(i)) {
                Register reg = (Register) vRegLocation.get(p.a);
                if (p.b == 1) {
                    registerPool.put(reg, p.a);
                } else {
                    if (Objects.equals(registerPool.get(reg), p.a)) {
                        registerPool.put(reg, 0);
                    }
                }
            }
            var inst = instructionList.get(i);
            Map<Register, Register> load = new HashMap<>();

            var readId = AsmInstructions.getReadVRegId(inst);
            var writeId = AsmInstructions.getWriteVRegId(inst);
            for (int j : readId) {
                RegisterReplaceable registerReplaceable = (RegisterReplaceable) inst.getOperand(j);
                var vReg = registerReplaceable.getRegister();
                if (vRegLocation.get(vReg.getAbsoluteIndex()) instanceof StackVar stackVar) {
                    if (!load.containsKey(vReg)) {
                        newInstList.addAll(getLoad(load, vReg, stackVar));
                    }
                    inst.setOperand(j, registerReplaceable.withRegister(load.get(vReg)));
                } else {
                    inst.setOperand(j, registerReplaceable.withRegister((Register) vRegLocation.get(vReg.getAbsoluteIndex())));
                }
            }

            Register writeReg = null;
            StackVar writeStack = null;
            for (int j : writeId) {
                RegisterReplaceable registerReplaceable = (RegisterReplaceable) inst.getOperand(j);
                var vReg = registerReplaceable.getRegister();
                if (vRegLocation.get(vReg.getAbsoluteIndex()) instanceof StackVar stackVar) {
                    writeReg = (vReg instanceof IntRegister) ? t1 : ft1;
                    writeStack = stackVar;
                } else {
                    writeReg = (Register) vRegLocation.get(vReg.getAbsoluteIndex());
                }
                inst.setOperand(j, registerReplaceable.withRegister(writeReg));
            }

            if (inst instanceof AsmCall) {
                Map<Register, StackVar> callSaved = new HashMap<>();
                for (var reg : registerPool.keySet()) {
                    if (registerPool.get(reg) != 0 && !Registers.isPreservedAcrossCalls(reg)) {
                        var tmp = stackPool.pop();
                        callSaved.put(reg, tmp);
                        var tmpl = saveToStackVar(reg, tmp, addressReg);
                        newInstList.addAll(tmpl);
                    }
                }
                newInstList.add(inst);
                for (var reg : callSaved.keySet()) {
                    var tmp = callSaved.get(reg);
                    var tmpl = loadFromStackVar(reg, tmp, addressReg);
                    newInstList.addAll(tmpl);
                    stackPool.push(tmp);
                }
            } else {
                newInstList.add(inst);
            }
            if (writeStack != null) {
                var tmpl = saveToStackVar(writeReg, writeStack, addressReg);
                newInstList.addAll(tmpl);
            }
        }
        return newInstList;
    }

    private void recycle(Integer index) {
        if (!vRegLocation.containsKey(index)) {
            return;
        }
        var container = vRegLocation.get(index);
        if (container instanceof Register register) {
            registerPool.put(register, 0);
        } else if (container instanceof StackVar stackVar) {
            stackPool.push(stackVar);
        }
    }

    @Override
    public List<AsmInstruction> work(List<AsmInstruction> instructionList) {
        virtualRegAllocateToPhysics();
        return spillRegisters(instructionList);
    }
}
