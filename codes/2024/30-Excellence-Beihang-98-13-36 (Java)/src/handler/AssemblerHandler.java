package handler;

import entities.*;
import util.IntHandler;
import util.Util;

import java.util.*;

import static entities.Registers.*;
import static util.Integers.INT;
import static util.Integers.FLOAT;
import static util.Strings.*;

public class AssemblerHandler {

    private static final Map<String, DoubleList<Assembler>> assemblers = new HashMap<>();
    private static final DoubleList<Assembler> global = new DoubleList<>();
    private static DoubleList<Assembler> ass;
    private static final Map<String, Register> regMap = new HashMap<>();
    private static final Map<Register, Integer> regTypeMap = new HashMap<>();
    private static final Map<String, Integer> funcTypeMap = new HashMap<>();
    private static final Map<String, Integer> funcValPos = new HashMap<>();
    private static final Set<String> argArraySet = new HashSet<>();
    private static final Map<String, String> globalVarMap = new HashMap<>();
    private static final List<Pair<String, Integer>> bssList = new ArrayList<>();
    private static final Map<String, List<Integer>> funcValDims = new HashMap<>();
    private static final Map<Float, String> constFloatVal = new HashMap<>();
    private static Integer funcType;
    private static int pos, spCount, argBeginPos, allSp, currentFunctionSpDelta;
    private static boolean willCall;

    public static AssemblerResult handle(QuaHandler.QuaResult quaResult) {
        bssList.clear();
        global.clear();
        global.add(new Statement(Statement.Op._SECTION, Register.label(_DATA)));
        ass = new DoubleList<>();
        assemblers.clear();
        regMap.clear();
        regTypeMap.clear();
        funcValPos.clear();
        funcTypeMap.clear();
        funcTypeMap.put(GET_INT, INT);
        funcTypeMap.put(GET_CHAR, INT);
        funcTypeMap.put(GET_FLOAT, FLOAT);
        funcTypeMap.put(GET_INT_ARRAY, INT);
        funcTypeMap.put(GET_FLOAT_ARRAY, INT);
        funcTypeMap.put(GET_INT_ARRAY_, INT);
        funcTypeMap.put(GET_FLOAT_ARRAY_, INT);
        argArraySet.clear();
        funcValDims.clear();
        globalVarMap.clear();
        constFloatVal.clear();
        quaResult.globalVar().forEach(AssemblerHandler::setGlobal);
        if (!bssList.isEmpty()) {
            global.add(new Statement(Statement.Op._SECTION, Register.label(_BSS)));
            for (Pair<String, Integer> val : bssList) {
                global.add(new Label(val.first));
                global.add(new Statement(Statement.Op._SPACE, Register.num(val.second)));
            }
        }
        ass.add(new Statement(Statement.Op._SECTION, Register.label(_TEXT_STARTUP)));
        ass.add(new Statement(Statement.Op._GLOBL, Register.label(MAIN)));
        assemblers.put(_PRE, ass);
        for (Register reg : RegisterHandler.mem2regR.values()) {
            regTypeMap.put(reg, INT);
        }
        for (Register reg : RegisterHandler.mem2regF.values()) {
            regTypeMap.put(reg, FLOAT);
        }
        quaResult.funcOrder().forEach(func -> {
            ass = new DoubleList<>();
            funcType = func.second;
            String name = func.first;
            funcTypeMap.put(name, funcType);
            ass.add(new Label(name));
            if (name.equals(MAIN)) {
                ass.add(new Statement(Statement.Op.LI, t(1), Register.num(0x20000000)));
                ass.add(new Statement(Statement.Op.CSRRS, t(2), Register.label(FCSR), t(1)));
                for (Variable var : quaResult.globalVar()) {
                    if (RegisterHandler.mem2regR.containsKey(var.name())) {
                        if (var.isArray()) {
                            if (Util.localArrayToGlobalArray.contains(var.name())) {
                                ass.add(new Statement(Statement.Op.LA, RegisterHandler.mem2regR.get(var.name()),
                                        Register.label(var.name() + XX)));
                            } else {
                                ass.add(new Statement(Statement.Op.LA, RegisterHandler.mem2regR.get(var.name()),
                                        Register.label(var.name())));
                            }
                        } else {
                            ass.add(new Statement(Statement.Op.LI, RegisterHandler.mem2regR.get(var.name()),
                                    Register.num(var.values().get(0).getInt())));
                        }
                    } else if (RegisterHandler.mem2regF.containsKey(var.name())) {
                        float value = var.values().get(0).getFloat();
                        Register temp = getTempRegisterR();
                        addStaticFloatValue(value);
                        ass.add(new Statement(Statement.Op.LA, temp, Register.label(constFloatVal.get(value))));
                        ass.add(new Statement(Statement.Op.FLW, RegisterHandler.mem2regF.get(var.name()), temp.addr()));
                    }
                }
            }
            willCall = quaResult.funcWillCallOtherFunc().get(func.first);
            spCount = quaResult.funcValSpace().get(func.first);
            argBeginPos = spCount + (willCall ? 8 : 0);
            pos = willCall ? ALL_SIZE * 8 + 8 : 0;
            allSp = argBeginPos + (willCall ? ALL_SIZE * 8 : 0);
            currentFunctionSpDelta = 0;
            ass.add(new Statement(Statement.Op.ADDI, SP, SP, (willCall ? Register.td(-allSp) : Register.num(-allSp))));
            if (willCall) {
                ass.add(new Statement(Statement.Op.SD, RA, _SP_));
            }
            doAssemble(quaResult.funcQua().get(func));
            assemblers.put(name, ass);
        });
        return new AssemblerResult(assemblers, global);
    }

    private static void setGlobal(Variable var) {
        funcValDims.put(var.name(), var.dimSum());
        if (!var.dims().isEmpty()) {
            int total = var.dims().get(0) * var.dimSum().get(0);
            if (total > 500000) {
                globalVarMap.put(var.name(), var.name());
                bssList.add(new Pair<>(var.name(), total * 8));
                return;
            }
        }
        boolean isIntVar = var.isInteger();
        if (Util.localArrayToGlobalArray.contains(var.name())) {
            global.add(new Label(var.name() + XX));
            globalVarMap.put(var.name(), var.name() + XX);
        } else {
            global.add(new Label(var.name()));
            globalVarMap.put(var.name(), var.name());
        }
        int now = 0;
        for (Integer idx : var.values().keySet().stream().sorted().toList()) {
            int cnt0 = idx - now;
            if (cnt0 != 0) {
                global.add(new Statement(Statement.Op._ZERO, Register.num(cnt0 * 8)));
            }
            now = idx + 1;
            FOI foi = var.values().get(idx);
            global.add(new Statement(Statement.Op._DWORD,
                    isIntVar ? Register.num(foi.getInt()) : Register.num(Float.floatToIntBits(foi.getFloat()))));
        }
        if (!var.dims().isEmpty()) {
            int cnt0 = var.dims().get(0) * var.dimSum().get(0) - now;
            if (cnt0 != 0) {
                global.add(new Statement(Statement.Op._ZERO, Register.num(cnt0 * 8)));
            }
        }
    }

    private static void doAssemble(DoubleList<Quaternion> qua) {
        DoubleList<Quaternion>.Iterator iterator = qua.iterator();
        while (iterator.hasNext()) {
            Quaternion q = iterator.next();
            switch (q.getOp()) {
                case ASSIGN -> {
                    Val lVal = q.getVal(0), exp = q.getVal(1);
                    Register expReg = getMaybeConstValue(exp);
                    if (lVal.isConst()) {
                        boolean isFloat = regTypeMap.get(expReg) == FLOAT;
                        Register temp = isFloat ? getTempRegisterF() : getTempRegisterR();
                        regMap.put(lVal.getName(), temp);
                        regTypeMap.put(temp, regTypeMap.get(expReg));
                        ass.add(new Statement(isFloat ? Statement.Op.FMV_S : Statement.Op.MV, temp, expReg));
                    } else {
                        if (RegisterHandler.mem2regR.containsKey(lVal.getName()) && lVal.getArgs().isEmpty()) {
                            expReg = tryFloatToInt(expReg);
                            ass.add(new Statement(Statement.Op.MV, RegisterHandler.mem2regR.get(lVal.getName()), expReg));
                        } else if (RegisterHandler.mem2regF.containsKey(lVal.getName())) {
                            expReg = tryIntToFloat(expReg);
                            ass.add(new Statement(Statement.Op.FMV_S, RegisterHandler.mem2regF.get(lVal.getName()), expReg));
                        } else {
                            Register temp = getVariableLeft(lVal);
                            if (lVal.isInt()) {
                                expReg = tryFloatToInt(expReg);
                                ass.add(new Statement(Statement.Op.SD, expReg, temp.addr()));
                            } else {
                                expReg = tryIntToFloat(expReg);
                                ass.add(new Statement(Statement.Op.FSW, expReg, temp.addr()));
                            }
                        }
                    }
                }
                case GET_RETURN_VALUE -> {
                    if (willCall) {
                        ass.add(new Mark(Mark.Type.LOAD, currentFunctionSpDelta));
                    }
                    boolean isFloatFunc = funcTypeMap.getOrDefault(q.getName(1), INT) == FLOAT;
                    Register temp = isFloatFunc ? getTempRegisterF() : getTempRegisterR();
                    regMap.put(q.getName(0), temp);
                    regTypeMap.put(temp, funcTypeMap.getOrDefault(q.getName(1), INT));
                    ass.add(new Statement(isFloatFunc ? Statement.Op.FMV_S : Statement.Op.MV,
                            temp, isFloatFunc ? fa(0) : a(0)));
                }
                case CALL -> {
                    if (willCall) {
                        ass.add(new Mark(Mark.Type.SAVE, currentFunctionSpDelta));
                    }
                    ass.add(new Statement(Statement.Op.CALL, Register.label(q.getVal(0).getName())));
                }
                case RETURN -> {
                    if (q.getValCount() == 1) {
                        Val val = q.getVal(0);
                        boolean isFloat = funcType == FLOAT;
                        if (val.isIntConst()) {
                            int value = Integer.parseInt(val.getName());
                            if (!isFloat) {
                                ass.add(new Statement(Statement.Op.LI, a(0), Register.num(value)));
                            } else {
                                float f = (float) value;
                                addStaticFloatValue(f);
                                Register temp = getTempRegisterR();
                                ass.add(new Statement(Statement.Op.LA, temp, Register.label(constFloatVal.get(f))));
                                ass.add(new Statement(Statement.Op.FLW, fa(0), temp.addr()));
                            }
                        } else if (val.isFloatConst()) {
                            float value = Float.parseFloat(val.getName());
                            if (isFloat) {
                                addStaticFloatValue(value);
                                Register temp = getTempRegisterR();
                                ass.add(new Statement(Statement.Op.LA, temp, Register.label(constFloatVal.get(value))));
                                ass.add(new Statement(Statement.Op.FLW, fa(0), temp.addr()));
                            } else {
                                ass.add(new Statement(Statement.Op.LI, a(0), Register.num((int) value)));
                            }
                        } else {
                            Register reg = getVariableRight(val);
                            if (!isFloat && isFloat(reg)) {
                                reg = floatToInt(reg);
                            } else if (isFloat && isInt(reg)) {
                                reg = intToFloat(reg);
                            }
                            ass.add(new Statement(isFloat ? Statement.Op.FMV_S : Statement.Op.MV,
                                    isFloat ? fa(0) : a(0), reg));
                        }
                    }
                    if (willCall) {
                        ass.add(new Statement(Statement.Op.LD, RA, _SP_));
                    }
                    ass.add(new Statement(Statement.Op.ADDI, SP, SP, willCall ? Register.td(allSp) : Register.num(allSp)));
                    ass.add(new Statement(Statement.Op.RET));
                }
                case SAVE_ARG -> {
                    int pos = Integer.parseInt(q.getVal(0).getName());
                    int delta = pos * 8;
                    Register exp = getMaybeConstValue(q.getVal(1));
                    int argIsInt = Integer.parseInt(q.getVal(2).getName());
                    if (argIsInt == 1) {
                        exp = tryFloatToInt(exp);
                    } else {
                        exp = tryIntToFloat(exp);
                    }
                    if (delta < -2048 || delta > 2047) {
                        Register temp = getTempRegisterR(), tempSp = getTempRegisterR();
                        ass.add(new Statement(Statement.Op.LI, temp, Register.num(delta)));
                        ass.add(new Statement(Statement.Op.ADD, tempSp, SP, temp));
                        ass.add(new Statement(isInt(exp) ? Statement.Op.SD : Statement.Op.FSW, exp, tempSp.addr()));
                    } else {
                        ass.add(new Statement(isInt(exp) ? Statement.Op.SD : Statement.Op.FSW, exp, Registers.sp(delta)));
                    }
                }
                case SET_ARG -> {
                    int pos = Integer.parseInt(q.getVal(0).getName());
                    Register exp = getMaybeConstValue(q.getVal(1));
                    int argIsInt = Integer.parseInt(q.getVal(2).getName());
                    if (argIsInt == 1) {
                        exp = tryFloatToInt(exp);
                    } else {
                        exp = tryIntToFloat(exp);
                    }
                    boolean isInt = isInt(exp);
                    ass.add(new Statement(isInt ? Statement.Op.MV : Statement.Op.FMV_S, isInt ? a(pos) : fa(pos),
                            isInt ? tryFloatToInt(exp) : tryIntToFloat(exp)));
                }
                case ADD_SP -> {
                    int delta = Integer.parseInt(q.getVal(0).getName());
                    ass.add(new Statement(Statement.Op.ADDI, SP, SP, Register.num(delta)));
                    currentFunctionSpDelta -= delta;
                }
                case PLUS, MINU, MULT, DIV -> {
                    Val val1 = q.getVal(0), val2 = q.getVal(1), val3 = q.getVal(2);
                    Register reg2 = getMaybeConstValue(val2), reg3 = getMaybeConstValue(val3);
                    if (isFloat(reg2) || isFloat(reg3)) {
                        Register reg1 = getTempRegisterF();
                        regMap.put(val1.getName(), reg1);
                        reg2 = tryIntToFloat(reg2);
                        reg3 = tryIntToFloat(reg3);
                        switch (q.getOp()) {
                            case PLUS -> ass.add(new Statement(Statement.Op.FADD_S, reg1, reg2, reg3));
                            case MINU -> ass.add(new Statement(Statement.Op.FSUB_S, reg1, reg2, reg3));
                            case MULT -> ass.add(new Statement(Statement.Op.FMUL_S, reg1, reg2, reg3));
                            case DIV -> ass.add(new Statement(Statement.Op.FDIV_S, reg1, reg2, reg3));
                        }
                        regTypeMap.put(reg1, FLOAT);
                    } else {
                        Register reg1 = getTempRegisterR();
                        regMap.put(val1.getName(), reg1);
                        switch (q.getOp()) {
                            case PLUS -> ass.add(new Statement(Statement.Op.ADDW, reg1, reg2, reg3));
                            case MINU -> ass.add(new Statement(Statement.Op.SUBW, reg1, reg2, reg3));
                            case MULT -> ass.add(new Statement(Statement.Op.MULW, reg1, reg2, reg3));
                            case DIV -> ass.add(new Statement(Statement.Op.DIVW, reg1, reg2, reg3));
                        }
                        regTypeMap.put(reg1, INT);
                    }
                }
                case MOD -> {
                    Val val1 = q.getVal(0), val2 = q.getVal(1), val3 = q.getVal(2);
                    Register reg1 = getTempRegisterR(), reg2 = getMaybeConstValue(val2), reg3 = getMaybeConstValue(val3);
                    regMap.put(val1.getName(), reg1);
                    regTypeMap.put(reg1, INT);
                    reg2 = tryFloatToInt(reg2);
                    reg3 = tryFloatToInt(reg3);
                    ass.add(new Statement(Statement.Op.REMW, reg1, reg2, reg3));
                }
                case JUMP -> ass.add(new Statement(Statement.Op.J, Register.label(q.getName(0))));
                case DEFINE_LABEL -> ass.add(new Label(q.getName(0)));
                case SLT, SLE, SGT, SGE, SEQ, SNE -> {
                    Val val1 = q.getVal(0), val2 = q.getVal(1), val3 = q.getVal(2);
                    Register reg1 = getTempRegisterR(), reg2 = getMaybeConstValue(val2), reg3 = getMaybeConstValue(val3);
                    regMap.put(val1.getName(), reg1);
                    regTypeMap.put(reg1, INT);
                    boolean isFloat = isFloat(reg2) || isFloat(reg3);
                    if (isFloat) {
                        reg2 = tryIntToFloat(reg2);
                        reg3 = tryIntToFloat(reg3);
                        ass.add(new Statement(getCmpOp(true, q.getOp()), reg1, reg2, reg3));
                    } else {
                        ass.add(new Statement(Statement.Op.SEXT_W, reg2, reg2));
                        ass.add(new Statement(Statement.Op.SEXT_W, reg3, reg3));
                        switch (q.getOp()) {
                            case SLT, SGT, SLE, SGE -> ass.add(new Statement(getCmpOp(false, q.getOp()), reg1, reg2, reg3));
                            case SEQ, SNE -> {
                                ass.add(new Statement(Statement.Op.XOR, reg1, reg2, reg3));
                                ass.add(new Statement(Statement.Op.SLTIU, reg1, reg1, Register.num(1)));
                            }
                        }
                    }
                    switch (q.getOp()) {
                        case SNE, SLE, SGE -> ass.add(new Statement(Statement.Op.XORI, reg1, reg1, Register.num(1)));
                    }
                }
                case EQZ -> {
                    Register exp = getMaybeConstValue(q.getVal(0));
                    if (isInt(exp)) {
                        ass.add(new Statement(Statement.Op.BEQZ, exp, Register.label(q.getName(1))));
                    } else {
                        Register zero = getMaybeConstValue(Val.ZERO_F);
                        Register res = getTempRegisterR();
                        ass.add(new Statement(Statement.Op.FEQ_S, res, exp, zero));
                        ass.add(new Statement(Statement.Op.XORI, res, res, Register.num(1)));
                        ass.add(new Statement(Statement.Op.BEQZ, res, Register.label(q.getVal(1).getName())));
                    }
                }
                case DECLARE -> {
                    Val val = q.getVal(0);
                    int space = addVarDimSum(val);
                    funcValPos.put(val.getName(), pos);
                    if (RegisterHandler.mem2regR.containsKey(val.getName()) && !val.getArgs().isEmpty()) {
                        int delta = pos + currentFunctionSpDelta;
                        ass.add(new Statement(Statement.Op.ADDI, RegisterHandler.mem2regR.get(val.getName()),
                                SP, willCall ? Register.td(delta) : Register.num(delta)));
                    }
                    pos += space;
                }
                case DECLARE_ARG -> {
                    Val val = q.getVal(0);
                    List<Val> args = val.getArgs();
                    funcValPos.put(val.getName(), argBeginPos + (willCall ? ALL_SIZE * 8 : 0));
                    argBeginPos += 8;
                    if (!args.isEmpty()) {
                        argArraySet.add(val.getName());
                    }
                    addVarDimSum(val);
                }
            }
        }
    }

    private static int addVarDimSum(Val val) {
        List<Val> args = val.getArgs();
        List<Integer> dims = new ArrayList<>(), dimSum = new ArrayList<>();
        int space = 8;
        for (Val v : args) {
            int dim = Integer.parseInt(v.getName());
            space *= dim;
            dims.add(dim);
        }
        int allDim = space / 8;
        for (Integer dim : dims) {
            allDim /= dim;
            dimSum.add(allDim);
        }
        funcValDims.put(val.getName(), dimSum);
        return space;
    }

    private static Statement.Op getCmpOp(boolean isFloat, Quaternion.Operator op) {
        return switch (op) {
            case SLT, SGE -> isFloat ? Statement.Op.FLT_S : Statement.Op.SLT;
            case SGT, SLE -> isFloat ? Statement.Op.FGT_S : Statement.Op.SGT;
            case SEQ -> isFloat ? Statement.Op.FEQ_S : Statement.Op.SEQ;
            case SNE -> isFloat ? Statement.Op.FEQ_S : Statement.Op.SNE;
            default -> throw new RuntimeException("unexpected operator");
        };
    }

    private static Register variableShift(Register reg, Val val) {
        List<Val> args = val.getArgs();
        if (!args.isEmpty()) {
            Register result = getTempRegisterR();
            ass.add(new Statement(Statement.Op.MV, result, reg));
            regTypeMap.put(result, INT);
            List<Integer> dims = funcValDims.get(val.getName());
            boolean isConstPos = true;
            int deltaPos = 0;
            for (int i = 0, len = args.size(); i < len; ++i) {
                if (args.get(i).isIntConst()) {
                    deltaPos += dims.get(i) * Integer.parseInt(args.get(i).getName()) * 8;
                } else {
                    isConstPos = false;
                    break;
                }
            }
            if (!isConstPos) {
                isConstPos = true;
                deltaPos = 0;
                for (int i = 0, len = args.size(); i < len; ++i) {
                    if (args.get(i).isIntConst() && isConstPos) {
                        deltaPos += dims.get(i) * 8 * Integer.parseInt(args.get(i).getName());
                    } else {
                        isConstPos = false;
                        Register dimDelta = getMaybeConstValue(args.get(i));
                        Register temp = getTempRegisterR();
                        ass.add(new Statement(Statement.Op.LI, temp, Register.num(dims.get(i) * 8)));
                        ass.add(new Statement(Statement.Op.MUL, temp, temp, dimDelta));
                        ass.add(new Statement(Statement.Op.ADD, result, result, temp));
                    }
                }
            }
            ass.add(new Statement(Statement.Op.ADDI, result, result, Register.num(deltaPos)));
            return result;
        } else {
            return reg;
        }
    }

    private static Register getVariableLeft(Val val) {
        String varName = val.getName();
        if (RegisterHandler.mem2regR.containsKey(varName)) {
            Register reg = RegisterHandler.mem2regR.get(varName);
            return variableShift(reg, val);
        }
        if (globalVarMap.get(varName) != null) {
            Register reg = getTempRegisterR();
            ass.add(new Statement(Statement.Op.LA, reg, Register.label(globalVarMap.get(varName))));
            regTypeMap.put(reg, INT);
            return variableShift(reg, val);
        }
        int pos = funcValPos.get(varName);
        Register reg = getTempRegisterR();
        regTypeMap.put(reg, INT);
        int delta = pos + currentFunctionSpDelta;
        ass.add(new Statement(Statement.Op.ADDI, reg, SP, willCall ? Register.td(delta) : Register.num(delta)));
        if (argArraySet.contains(varName)) {
            ass.add(new Statement(Statement.Op.LD, reg, reg.addr()));
        }
        return variableShift(reg, val);
    }

    private static Register getVariableRight(Val val) {
        String varName = val.getName();
        if (RegisterHandler.mem2regR.containsKey(varName) && val.getArgs().isEmpty()) {
            return RegisterHandler.mem2regR.get(varName);
        }
        if (RegisterHandler.mem2regF.containsKey(varName)) {
            return RegisterHandler.mem2regF.get(varName);
        }
        if (globalVarMap.get(varName) != null) {
            Register addr = getVariableLeft(val);
            boolean isFloat = val.isFloat();
            Register reg = addr.copy();
            if (isFloat) {
                reg = getTempRegisterF();
            }
            ass.add(new Statement(isFloat ? Statement.Op.FLW : Statement.Op.LD, reg, addr.addr()));
            regTypeMap.put(reg, isFloat ? FLOAT : INT);
            return reg;
        }
        if (regMap.get(val.getName()) != null && val.isLabel()) {
            return regMap.get(val.getName());
        }
        boolean isFloat = val.isFloat();
        Register reg = getVariableLeft(val);
        List<Val> args = val.getArgs();
        List<Integer> dims = funcValDims.get(varName);
        int varDim = dims.size();
        int realDim = args.size();
        if (realDim == varDim) {
            if (isFloat) {
                Register fReg = getTempRegisterF();
                ass.add(new Statement(Statement.Op.FLW, fReg, reg.addr()));
                reg = fReg.copy();
                regTypeMap.put(reg, FLOAT);
            } else {
                ass.add(new Statement(Statement.Op.LD, reg, reg.addr()));
                regTypeMap.put(reg, INT);
            }
        } else {
            regTypeMap.put(reg, INT);
        }
        regMap.put(val.getName(), reg);
        return reg;
    }

    private static Register getMaybeConstValue(Val val) {
        Register reg;
        if (val.isFloatConst()) {
            float value = Float.parseFloat(val.getName());
            Register temp = getTempRegisterR();
            reg = getTempRegisterF();
            addStaticFloatValue(value);
            ass.add(new Statement(Statement.Op.LA, temp, Register.label(constFloatVal.get(value))));
            ass.add(new Statement(Statement.Op.FLW, reg, temp.addr()));
            regTypeMap.put(reg, FLOAT);
        } else if (val.isIntConst()) {
            int value = Integer.parseInt(val.getName());
            reg = getTempRegisterR();
            ass.add(new Statement(Statement.Op.LI, reg, Register.num(value)));
            regTypeMap.put(reg, INT);
        } else {
            if (globalVarMap.get(val.getName()) != null) {
                List<Integer> dims = funcValDims.get(val.getName());
                if (val.getArgs().size() != dims.size()) {
                    reg = getVariableLeft(val);
                    return reg;
                }
            }
            reg = getVariableRight(val);
        }
        return reg;
    }

    private static void addStaticFloatValue(float value) {
        if (!constFloatVal.containsKey(value)) {
            constFloatVal.put(value, _LC + IntHandler.get());
            global.add(new Label(constFloatVal.get(value)));
            global.add(new Statement(Statement.Op._DWORD, Register.num(Float.floatToIntBits(value))));
        }
    }

    private static Register floatToInt(Register reg) {
        Register result = getTempRegisterR();
        ass.add(new Statement(Statement.Op.FCVT_W_S, result, reg, Register.label(RTZ)));
        regTypeMap.put(result, INT);
        return result;
    }

    private static Register intToFloat(Register reg) {
        Register result = getTempRegisterF();
        ass.add(new Statement(Statement.Op.FCVT_S_W, result, reg));
        regTypeMap.put(result, FLOAT);
        return result;
    }

    private static Register tryFloatToInt(Register reg) {
        return isInt(reg) ? reg : floatToInt(reg);
    }

    private static Register tryIntToFloat(Register reg) {
        return isFloat(reg) ? reg : intToFloat(reg);
    }

    private static boolean isFloat(Register reg) {
        return regTypeMap.get(reg) == FLOAT;
    }

    private static boolean isInt(Register reg) {
        return regTypeMap.get(reg) == INT;
    }

    private static Register getTempRegisterR() {
        return Register.tr(IntHandler.get());
    }

    private static Register getTempRegisterF() {
        return Register.tf(IntHandler.get());
    }

    public record AssemblerResult(Map<String, DoubleList<Assembler>> assemblers, DoubleList<Assembler> global) {}

}
