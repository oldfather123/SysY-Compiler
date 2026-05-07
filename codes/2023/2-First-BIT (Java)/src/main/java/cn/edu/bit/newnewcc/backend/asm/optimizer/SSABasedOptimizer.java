package cn.edu.bit.newnewcc.backend.asm.optimizer;

import cn.edu.bit.newnewcc.backend.asm.AsmFunction;
import cn.edu.bit.newnewcc.backend.asm.instruction.*;
import cn.edu.bit.newnewcc.backend.asm.operand.AsmOperand;
import cn.edu.bit.newnewcc.backend.asm.operand.Immediate;
import cn.edu.bit.newnewcc.backend.asm.operand.Register;
import cn.edu.bit.newnewcc.backend.asm.operand.RegisterReplaceable;
import cn.edu.bit.newnewcc.backend.asm.optimizer.ssabased.*;
import cn.edu.bit.newnewcc.backend.asm.util.AsmInstructions;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class SSABasedOptimizer implements Optimizer {

    /**
     * 正在优化的函数
     */
    public AsmFunction functionContext;

    private final Map<Register, Register> registerReplacementMap = new HashMap<>();
    private final Map<Register, AsmInstruction> valueSourceMap = new HashMap<>();

    private Register getReplacedRegister(Register register) {
        var nextRegister = registerReplacementMap.get(register);
        if (nextRegister == null) {
            return register;
        } else {
            nextRegister = getReplacedRegister(nextRegister);
            registerReplacementMap.put(register, nextRegister);
            return nextRegister;
        }
    }

    /**
     * 获取寄存器中值的来源 <br>
     * 若值有多个来源，或是值的来源无法确定，返回null <br>
     *
     * @param register 寄存器
     * @return 如果值有唯一确定的来源指令，返回来源指令；否则返回null。
     */
    public AsmInstruction getValueSource(Register register) {
        return valueSourceMap.get(register);
    }

    public boolean isConstIntOperand(AsmOperand asmOperand) {
        if (asmOperand instanceof Immediate) return true;
        if (asmOperand instanceof Register register) {
            if (getValueSource(register) instanceof AsmLoad asmLoad) {
                return asmLoad.getOpcode() == AsmLoad.Opcode.LI;
            }
            if (register.getIndex() == 0) {
                return true;
            }
        }
        return false;
    }

    public Integer getConstIntValueFromOperand(AsmOperand asmOperand) {
        if (!isConstIntOperand(asmOperand)) return null;
        if (asmOperand instanceof Immediate immediate) return immediate.getValue();
        if (asmOperand instanceof Register register) {
            if (getValueSource(register) instanceof AsmLoad asmLoad) {
                if (asmLoad.getOpcode() == AsmLoad.Opcode.LI) {
                    var immediate = (Immediate) asmLoad.getOperand(2);
                    return immediate.getValue();
                }
            }
            if (register.getIndex() == 0) {
                return 0;
            }
        }
        // 函数开头已经通过了检查，运行到此处说明检查方式和获取方式中至少有一处不对应
        throw new RuntimeException("Unable to get const value from this operand.");
    }

    private AsmInstruction breakThroughMove(AsmMove asmMove) {
        var father = (Register) asmMove.getOperand(2);
        if (valueSourceMap.get(father) instanceof AsmMove asmMove1) {
            var breakThroughResult = breakThroughMove(asmMove1);
            if (breakThroughResult != null) {
                return breakThroughResult;
            } else {
                return asmMove;
            }
        } else {
            return valueSourceMap.get(father);
        }
    }

    private void preprocessValueSourceMap() {
        for (AsmInstruction instruction : functionContext.getInstrList()) {
            if (instruction instanceof AsmLabel || instruction instanceof AsmBlockEnd) continue;
            for (Register definedRegister : instruction.getDef()) {
                if (!definedRegister.isVirtual()) continue;
                if (valueSourceMap.containsKey(definedRegister)) {
                    // 此处必须 put null ，不能 remove
                    // 若是 remove ，则下一次 define 时会进入 else 分支，导致多定义的变量被错误的视作SSA
                    valueSourceMap.put(definedRegister, null);
                } else {
                    valueSourceMap.put(definedRegister, instruction);
                }
            }
        }
        // 试图穿过 Move 指令获取更早的定义指令，以获得更佳的效果
        for (Register register : valueSourceMap.keySet()) {
            var valueSource = valueSourceMap.get(register);
            if (valueSource instanceof AsmMove asmMove) {
                var breakThroughResult = breakThroughMove(asmMove);
                if (breakThroughResult != null) {
                    valueSourceMap.put(register, breakThroughResult);
                }
            }
        }
    }

    private void checkRegisterReplacementMap() {
        for (Register register : registerReplacementMap.keySet()) {
            var sourceRegister = register;
            var targetRegister = registerReplacementMap.get(register);
            if (getValueSource(sourceRegister) == null) {
                throw new RuntimeException("Trying to replace a register that is NOT SSA.");
            }
            if (!sourceRegister.isVirtual()) {
                throw new RuntimeException("Replacing usage of physical register is dangerous, it is NOT ALLOWED.");
            }
            if (!targetRegister.isVirtual()) {
                throw new RuntimeException("Replacing usage of physical register is dangerous, it is NOT ALLOWED.");
            }

        }
    }

    private static List<ISSABasedOptimizer> optimizerList;

    private static List<ISSABasedOptimizer> getOptimizerList() {
        if (optimizerList == null) {
            var list = new ArrayList<ISSABasedOptimizer>();
            list.add(new LI0ToX0Optimizer());
            list.add(new MvX0ToLI0Optimizer());
            list.add(new SLLIAddToShNAddOptimizer());
            list.add(new AddX0ToMvOptimizer());
            list.add(new SrslMergeOptimizer());
            list.add(new StrengthReductionOptimizer());
            list.add(new MergeRecentlyUsedLa());
            list.add(new MergeRecentlyUsedLi());
            list.add(new MergeRecentlySameValue());
            list.add(new LiAddToAddiForSSA());
            list.add(new AddiToOffsetForSSA());
            optimizerList = list;
        }
        return optimizerList;
    }

    private boolean runOn(AsmFunction function, ISSABasedOptimizer optimizer) {
        functionContext = function;

        var optimizerList = getOptimizerList();

        preprocessValueSourceMap();

        List<AsmInstruction> oldInstrList = new ArrayList<>(function.getInstrList());
        List<AsmInstruction> newInstrList = function.getInstrList();
        newInstrList.clear();
        int count = 0;

        optimizer.setFunctionBegins();
        for (AsmInstruction instruction : oldInstrList) {
            if (instruction instanceof AsmLabel) {
                newInstrList.add(instruction);
                optimizer.setBlockBegins();
            } else if (instruction instanceof AsmBlockEnd) {
                optimizer.setBlockEnds();
                newInstrList.add(instruction);
            } else {
                var result = optimizer.getReplacement(this, instruction);
                if (result != null) {
                    newInstrList.addAll(result.getInstructions());
                    registerReplacementMap.putAll(result.getRegisterMap());
                    count++;
                }
                newInstrList.add(instruction);
            }
        }
        optimizer.setFunctionEnds();

        checkRegisterReplacementMap();

        for (AsmInstruction instruction : newInstrList) {
            if (instruction instanceof AsmLabel || instruction instanceof AsmBlockEnd) continue;
            for (int id : AsmInstructions.getReadRegId(instruction)) {
                if (instruction.getOperand(id) instanceof RegisterReplaceable registerReplaceable) {
                    var sourceReg = registerReplaceable.getRegister();
                    if (registerReplacementMap.containsKey(sourceReg)) {
                        var targetReg = getReplacedRegister(sourceReg);
                        instruction.setOperand(id, registerReplaceable.withRegister(targetReg));
                    }
                }
            }
        }

        new DeadInstructionEliminationOptimizer().runOn(function);

        return count > 0;
    }

    public boolean runOn(AsmFunction function) {
        boolean changed = false;
        for (ISSABasedOptimizer optimizer : getOptimizerList()) {
            changed |= new SSABasedOptimizer().runOn(function, optimizer);
        }
        return changed;
    }


    /**
     * 判断一个指令是否使用了物理寄存器
     *
     * @param instruction 指令
     * @return 若使用了物理寄存器，返回true；否则返回false。
     */
    public static boolean usesPhysicalRegister(AsmInstruction instruction) {
        for (int id = 1; id <= 3; id++) {
            if (instruction.getOperand(id) instanceof Register register && !register.isVirtual()) {
                return true;
            }
        }
        return false;
    }

}
