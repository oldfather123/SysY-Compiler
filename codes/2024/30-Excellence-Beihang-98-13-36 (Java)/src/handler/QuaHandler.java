package handler;

import entities.*;
import util.*;

import java.util.*;

public class QuaHandler {

    public static final VarTable valTable = new VarTable();
    public static DoubleList<Quaternion> qua;
    private static final List<Variable> globalVar = new ArrayList<>();
    private static int now = 0, space = 0;
    private static final Stack<Val> breakLabel = new Stack<>(), continueLabel = new Stack<>();
    private static boolean willCall = false;
    private static final Map<String, List<Integer>> funcFParamType = new HashMap<>();
    public static final List<Triple<String, String, Long>>
            funcFValUseTimes = new ArrayList<>(), funcRValUseTimes = new ArrayList<>();
    private static final Map<Pair<String, String>, Long>
            funcFValUseTimesMap = new HashMap<>(), funcRValUseTimesMap = new HashMap<>();
    private static long multiTime = 1;
    private static String currentFunc;
    private static final Set<String> globalValNames = new HashSet<>(), funcArgNames = new HashSet<>();
    public static final Map<String, FOI> constGlobalVal = new HashMap<>();

    public static QuaResult handle(TreeNode root) {
        valTable.init();
        globalVar.clear();
        funcFParamType.clear();
        funcFValUseTimes.clear();
        funcRValUseTimes.clear();
        funcFValUseTimesMap.clear();
        funcRValUseTimesMap.clear();
        QuaResult RESULT = new QuaResult(new HashMap<>(), new ArrayList<>(), new HashMap<>(), new DoubleList<>(), new HashMap<>());
        for (TreeNode node = root.initIterator(); node != null; node = root.nextSon()) {
            if (node.typeIs(TreeNode.Type.FUNC_DEF)) {
                node.initIterator();//type
                String ident = node.nextSon().getWord().getValue();//ident
                List<Integer> types = new ArrayList<>();
                for (TreeNode funcFParam = node.nextSon();
                     funcFParam.typeIs(TreeNode.Type.FUNC_F_PARAM); funcFParam = node.nextSon()) {
                    types.add(funcFParam.initIterator().getWord().is(Word.Type.FLOAT) && funcFParam.getSonCount() == 2 ? 0 : 1);
                }
                funcFParamType.put(ident, types);
            }
        }
        specificFunctionArgs();
        for (TreeNode node = root.initIterator(); node != null; node = root.nextSon()) {
            if (node.typeIs(TreeNode.Type.FUNC_DEF)) {
                willCall = false;
                String typeString = node.initIterator().getWord().getValue();
                int type = typeString.equals(Strings.VOID_) ? Integers.VOID :
                        (typeString.equals(Strings.INT_) ? Integers.INT : Integers.FLOAT);
                String name = node.nextSon().getWord().getValue();
                Pair<String, Integer> typeAndName = new Pair<>(name, type);
                currentFunc = name;
                space = 0;
                multiTime = 1;
                funcArgNames.clear();
                RESULT.funcQua.put(typeAndName, funcQuaternion(node, type == Integers.VOID));
                RESULT.funcValSpace.put(name, space * 8);
                RESULT.funcOrder.add(typeAndName);
                RESULT.funcWillCallOtherFunc.put(name, willCall);
            } else if (node.typeIs(TreeNode.Type.CONST_DECL) || node.typeIs(TreeNode.Type.VAR_DECL)) {
                globalVarDefine(node);
            }
        }
        RESULT.globalVar.addAll(globalVar);
        for (Pair<String, String> funcAndVal : funcRValUseTimesMap.keySet()) {
            funcRValUseTimes.add(new Triple<>(funcAndVal.first, funcAndVal.second, funcRValUseTimesMap.get(funcAndVal)));
        }
        for (Pair<String, String> funcAndVal : funcFValUseTimesMap.keySet()) {
            funcFValUseTimes.add(new Triple<>(funcAndVal.first, funcAndVal.second, funcFValUseTimesMap.get(funcAndVal)));
        }
        return RESULT;
    }

    private static DoubleList<Quaternion> funcQuaternion(TreeNode funcDef, boolean isVoid) {
        qua = new DoubleList<>();
        valTable.add();
        funcDef.initIterator();//Type
        funcDef.nextSon();//Name
        TreeNode block = funcDef.nextSon();
        while (!block.typeIs(TreeNode.Type.BLOCK)) {
            funcFParamQuaternion(block);
            block = funcDef.nextSon();
        }
        blockQuaternion(block, false);
        if (isVoid) {
            qua.add(new Quaternion(Quaternion.Operator.RETURN));
        }
        valTable.pop();
        return qua;
    }

    private static void funcFParamQuaternion(TreeNode funcFParam) {
        TreeNode bType = funcFParam.initIterator();
        TreeNode ident = funcFParam.nextSon();
        boolean isInt = bType.getWord().is(Word.Type.INT);
        ArrayList<Val> args = new ArrayList<>();
        List<Integer> dims = new ArrayList<>(), dimSum = new ArrayList<>();
        int dim0 = 1;
        for (TreeNode node = funcFParam.nextSon(); node != null; node = funcFParam.nextSon()) {
            if (node.typeIs(TreeNode.Type.EMPTY)) {
                args.add(Val.intConst(1));
                continue;
            }
            int dim = ConstExp.addExp(node).getInt();
            dim0 *= dim;
            dims.add(dim);
            args.add(Val.intConst(dim));
        }
        for (Integer dim : dims) {
            dim0 /= dim;
            dimSum.add(dim0);
        }
        String name = ident.getWord().getValue();
        Val val = new Val(isInt, false, name + "_" + IntHandler.get()).addArgs(args);
        valTable.defineConst(new Variable(val.getName(), Variable.getType(bType.getWord(), funcFParam.getSonCount() - 2),
                funcFParam.getSonCount() - 2, dims, dimSum, new HashMap<>()), name);
        funcArgNames.add(val.getName());
        qua.add(new Quaternion(Quaternion.Operator.DECLARE_ARG, val));
    }

    private static void blockQuaternion(TreeNode block, boolean willAdd) {
        if (willAdd) {
            valTable.add();
        }
        for (TreeNode node = block.initIterator(); node != null; node = block.nextSon()) {
            if (node.typeIs(TreeNode.Type.STMT)) {
                stmtQuaternion(node);
            } else {
                declQuaternion(node);
            }
        }
        if (willAdd) {
            valTable.pop();
        }
    }

    private static void stmtQuaternion(TreeNode stmt) {
        TreeNode firstSon = stmt.initIterator();
        if (firstSon.typeIs(TreeNode.Type.L_VAL)) {
            qua.add(new Quaternion(Quaternion.Operator.ASSIGN, lValQuaternion(firstSon), addExpQuaternion(stmt.nextSon())));
        } else if (firstSon.typeIs(TreeNode.Type.ADD_EXP)) {
            addExpQuaternion(firstSon);
        } else if (firstSon.typeIs(TreeNode.Type.BLOCK)) {
            blockQuaternion(firstSon, true);
        } else if (firstSon.typeIs(TreeNode.Type.IF)) {
            Val trueLabel = Val.label(Strings.IF_TRUE);
            Val falseLabel = Val.label(Strings.IF_FALSE);
            Val endLabel = Val.label(Strings.IF_END);
            lOrExpQuaternion(stmt.nextSon(), trueLabel, falseLabel);
            qua.add(new Quaternion(Quaternion.Operator.DEFINE_LABEL, trueLabel));
            stmtQuaternion(stmt.nextSon());
            qua.add(new Quaternion(Quaternion.Operator.JUMP, endLabel));
            qua.add(new Quaternion(Quaternion.Operator.DEFINE_LABEL, falseLabel));
            TreeNode elseStmt = stmt.nextSon();
            if (elseStmt != null) {
                stmtQuaternion(elseStmt);
            }
            qua.add(new Quaternion(Quaternion.Operator.DEFINE_LABEL, endLabel));
        } else if (firstSon.typeIs(TreeNode.Type.WHILE)) {
            multiTime++;
            Val condLabel = Val.label(Strings.WHILE_COND);
            Val beginLabel = Val.label(Strings.WHILE_BEGIN);
            Val endLabel = Val.label(Strings.WHILE_END);
            breakLabel.add(endLabel);
            continueLabel.add(condLabel);
            qua.add(new Quaternion(Quaternion.Operator.DEFINE_LABEL, condLabel));
            lOrExpQuaternion(stmt.nextSon(), beginLabel, endLabel);
            qua.add(new Quaternion(Quaternion.Operator.DEFINE_LABEL, beginLabel));
            stmtQuaternion(stmt.nextSon());
            qua.add(new Quaternion(Quaternion.Operator.JUMP, condLabel));
            qua.add(new Quaternion(Quaternion.Operator.DEFINE_LABEL, endLabel));
            breakLabel.pop();
            continueLabel.pop();
            multiTime--;
        } else if (firstSon.typeIs(TreeNode.Type.BREAK)) {
            qua.add(new Quaternion(Quaternion.Operator.JUMP, breakLabel.peek()));
        } else if (firstSon.typeIs(TreeNode.Type.CONTINUE)) {
            qua.add(new Quaternion(Quaternion.Operator.JUMP, continueLabel.peek()));
        } else if (firstSon.typeIs(TreeNode.Type.RETURN)) {
            TreeNode addExp = stmt.nextSon();
            if (addExp != null) {
                qua.add(new Quaternion(Quaternion.Operator.RETURN, addExpQuaternion(addExp)));
            } else {
                qua.add(new Quaternion(Quaternion.Operator.RETURN));
            }
        }
    }

    private static void declQuaternion(TreeNode decl) {
        TreeNode constDef = decl.initIterator();
        Word type = constDef.getWord();
        for (constDef = decl.nextSon(); constDef != null; constDef = decl.nextSon()) {
            String name = constDef.initIterator().getWord().getValue();
            int dim = constDef.getSonCount() - 2, mulSum = 1;
            List<Integer> dims = new ArrayList<>();
            for (int i = 0; i < dim; i++) {
                int res = ConstExp.addExp(constDef.nextSon()).getInt();
                dims.add(res);
                mulSum *= res;
            }
            if (mulSum > 1024) {
                globalDef(constDef, type, true);
                continue;
            }
            space += mulSum;
            List<Integer> dimSum = new ArrayList<>();
            for (int i = 0; i < dim; i++) {
                mulSum /= dims.get(i);
                dimSum.add(mulSum);
            }
            boolean isInt = type.is(Word.Type.INT);
            ArrayList<Val> args = new ArrayList<>();
            for (int i : dims) {
                args.add(Val.intConst(i));
            }
            Val val = new Val(isInt, false, name + "_" + IntHandler.get()).addArgs(args);
            qua.add(new Quaternion(Quaternion.Operator.DECLARE, val));
            if (decl.typeIs(TreeNode.Type.CONST_DECL)) {
                Map<Integer, FOI> value = ConstExp.constInitVal(constDef.nextSon(), dimSum,
                        dims.isEmpty() ? 0 : dims.get(0), 0);
                Variable variable = new Variable(val.getName(), Variable.getType(type, constDef.getSonCount() - 2),
                        constDef.getSonCount() - 2, dims, dimSum, value);
                valTable.defineConst(variable, name);
                constInitValQuaternion(value, val, dimSum);
            } else {
                valTable.defineConst(new Variable(val.getName(), Variable.getType(type, constDef.getSonCount() - 2),
                        constDef.getSonCount() - 2, dims, dimSum, new HashMap<>()), name);
                TreeNode initVal = constDef.nextSon();
                if (initVal != null) {
                    now = 0;
                    initValQuaternion(type, val, initVal, dimSum, dims.isEmpty() ? 0 : dims.get(0), 0);
                }
            }
        }
    }

    private static void constInitValQuaternion(Map<Integer, FOI> value, Val val, List<Integer> dimSum) {
        if (dimSum.isEmpty()) {
            qua.add(new Quaternion(Quaternion.Operator.ASSIGN, Val.fromVal(val),
                    val.isInt() ? Val.intConst(value.get(0).getInt()) : Val.floatConst(value.get(0).getFloat())));
            return;
        }
        for (Integer idx : value.keySet()) {
            FOI foi = value.get(idx);
            qua.add(new Quaternion(Quaternion.Operator.ASSIGN, offsetVal(val, dimSum, idx),
                    val.isInt() ? Val.intConst(foi.getInt()) : Val.floatConst(foi.getFloat())));
        }
    }

    private static void initValQuaternion(Word type, Val val, TreeNode constInitVal, List<Integer> dimSum, int dim0, int index) {
        if (dimSum.isEmpty()) {
            qua.add(new Quaternion(Quaternion.Operator.ASSIGN, Val.fromVal(val), addExpQuaternion(constInitVal)));
            return;
        }
        TreeNode constExp = constInitVal.initIterator();
        for (int i = 0; i < dim0; ++i) {
            if (constExp == null) {
                int maxRead = dimSum.get(index);
                while (maxRead != 0) {
                    maxRead--;
                    qua.add(new Quaternion(Quaternion.Operator.ASSIGN, offsetVal(val, dimSum, now++), Val.constVal(type, FOI.ZERO)));
                }
            } else if (constExp.typeIs(TreeNode.Type.ADD_EXP)) {
                int maxRead = dimSum.get(index) - 1;
                qua.add(new Quaternion(Quaternion.Operator.ASSIGN, offsetVal(val, dimSum, now++), addExpQuaternion(constExp)));
                while (maxRead != 0) {
                    constExp = constInitVal.currentSon();
                    if (constExp == null || !constExp.typeIs(TreeNode.Type.ADD_EXP)) {
                        break;
                    }
                    constExp = constInitVal.nextSon();
                    maxRead--;
                    qua.add(new Quaternion(Quaternion.Operator.ASSIGN, offsetVal(val, dimSum, now++), addExpQuaternion(constExp)));
                }
                while (maxRead != 0) {
                    maxRead--;
                    qua.add(new Quaternion(Quaternion.Operator.ASSIGN, offsetVal(val, dimSum, now++), Val.constVal(type, FOI.ZERO)));
                }
            } else {
                initValQuaternion(type, val, constExp, dimSum, dimSum.get(index) / dimSum.get(index + 1), index + 1);
            }
            constExp = constInitVal.nextSon();
        }
    }

    private static Val offsetVal(Val val, List<Integer> dimSum, int i) {
        List<Integer> offset = new ArrayList<>();
        for (Integer integer : dimSum) {
            offset.add(i / integer);
            i %= integer;
        }
        return Val.withoutArgs(val).offset(offset);
    }

    private static Val addExpQuaternion(TreeNode addExp) {
        Val result = mulExpQuaternion(addExp.initIterator());
        if (addExp.getSonCount() == 1) {
            return result;
        }
        for (TreeNode op = addExp.nextSon(); op != null; op = addExp.nextSon()) {
            Val addVal = Val.label(Strings.ADD_VAL);
            Val mulVal = mulExpQuaternion(addExp.nextSon());
            if (op.getWord().is(Word.Type.PLUS)) {
                qua.add(new Quaternion(Quaternion.Operator.PLUS, addVal, result, mulVal));
            } else {
                qua.add(new Quaternion(Quaternion.Operator.MINU, addVal, result, mulVal));
            }
            result = addVal;
        }
        return result;
    }

    private static Val mulExpQuaternion(TreeNode mulExp) {
        Val result = unaryExpQuaternion(mulExp.initIterator());
        if (mulExp.getSonCount() == 1) {
            return result;
        }
        for (TreeNode op = mulExp.nextSon(); op != null; op = mulExp.nextSon()) {
            Val mulVal = Val.label(Strings.MUL_VAL);
            Val unaryVal = unaryExpQuaternion(mulExp.nextSon());
            if (op.getWord().is(Word.Type.MULT)) {
                qua.add(new Quaternion(Quaternion.Operator.MULT, mulVal, result, unaryVal));
            } else if (op.getWord().is(Word.Type.DIV)) {
                qua.add(new Quaternion(Quaternion.Operator.DIV, mulVal, result, unaryVal));
            } else {
                qua.add(new Quaternion(Quaternion.Operator.MOD, mulVal, result, unaryVal));
            }
            result = mulVal;
        }
        return result;
    }

    private static Val unaryExpQuaternion(TreeNode unaryExp) {
        TreeNode firstSon = unaryExp.initIterator();
        if (firstSon.typeIs(TreeNode.Type.UNARY_OP)) {
            Val unaryVal = Val.label(Strings.UNARY_VAL);
            Val unaryVal2 = unaryExpQuaternion(unaryExp.nextSon());
            if (firstSon.getWord().is(Word.Type.PLUS)) {
                qua.add(new Quaternion(Quaternion.Operator.PLUS, unaryVal, Val.intConst(0), unaryVal2));
            } else if (firstSon.getWord().is(Word.Type.MINU)) {
                qua.add(new Quaternion(Quaternion.Operator.MINU, unaryVal, Val.intConst(0), unaryVal2));
            } else {
                qua.add(new Quaternion(Quaternion.Operator.SEQ, unaryVal, unaryVal2, Val.intConst(0)));
            }
            return unaryVal;
        } else if (firstSon.typeIs(TreeNode.Type.PRIMARY_EXP)) {
            return primaryExpQuaternion(firstSon);
        } else {
            willCall = true;
            int argumentCount = unaryExp.getSonCount() - 1;
            String funcName = firstSon.getWord().getValue();
            boolean isSpecialIOFunction = Strings.PRE_FUNC_NAMES.contains(funcName);
            List<Integer> argType = funcFParamType.get(funcName);
            if (isSpecialIOFunction) {
                switch (funcName) {
                    case Strings.GET_INT_ARRAY -> {
                        funcName = Strings.GET_INT_ARRAY_;
                        Util.hasGetIntArray = true;
                    }
                    case Strings.GET_FLOAT_ARRAY -> {
                        funcName = Strings.GET_FLOAT_ARRAY_;
                        Util.hasGetFloatArray = true;
                    }
                    case Strings.PUT_INT_ARRAY -> {
                        funcName = Strings.PUT_INT_ARRAY_;
                        Util.hasPutIntArray = true;
                    }
                    case Strings.PUT_FLOAT_ARRAY -> {
                        funcName = Strings.PUT_FLOAT_ARRAY_;
                        Util.hasPutFloatArray = true;
                    }
                    case Strings.START_TIME -> {
                        funcName = Strings.SYSY_START_TIME;
                        qua.add(new Quaternion(Quaternion.Operator.SET_ARG, Val.intConst(0), Val.intConst(firstSon.getWord().line), Val.intConst(1)));
                    }
                    case Strings.STOP_TIME -> {
                        funcName = Strings.SYSY_STOP_TIME;
                        qua.add(new Quaternion(Quaternion.Operator.SET_ARG, Val.intConst(0), Val.intConst(firstSon.getWord().line), Val.intConst(1)));
                    }
                }
                for (int i = 0; i < argumentCount; ++i) {
                    qua.add(new Quaternion(Quaternion.Operator.SET_ARG, Val.intConst(i), addExpQuaternion(unaryExp.nextSon()), Val.intConst(argType.get(i))));
                }
            } else {
                qua.add(new Quaternion(Quaternion.Operator.ADD_SP, Val.intConst(-argumentCount * 8)));
                for (int i = 0; i < argumentCount; ++i) {
                    qua.add(new Quaternion(Quaternion.Operator.SAVE_ARG, Val.intConst(i), addExpQuaternion(unaryExp.nextSon()), Val.intConst(argType.get(i))));
                }
            }
            qua.add(new Quaternion(Quaternion.Operator.CALL, Val.string(funcName)));
            Val returnVal = Val.label(Strings.RETURN_VALUE);
            qua.add(new Quaternion(Quaternion.Operator.GET_RETURN_VALUE, returnVal, Val.string(funcName)));
            if (!isSpecialIOFunction) {
                qua.add(new Quaternion(Quaternion.Operator.ADD_SP, Val.intConst(argumentCount * 8)));
            }
            return returnVal;
        }
    }

    private static Val primaryExpQuaternion(TreeNode primaryExp) {
        TreeNode firstSon = primaryExp.initIterator();
        if (firstSon.typeIs(TreeNode.Type.INT_CONST)) {
            return Val.intConst(NumberHandler.getInteger(firstSon.getWord().getValue()));
        } else if (firstSon.typeIs(TreeNode.Type.FLOAT_CONST)) {
            return Val.floatConst(NumberHandler.getFloat(firstSon.getWord().getValue()));
        } else if (firstSon.typeIs(TreeNode.Type.L_VAL)) {
            return lValQuaternion(firstSon);
        } else {
            return addExpQuaternion(firstSon);
        }
    }

    private static Val lValQuaternion(TreeNode lVal) {
        TreeNode ident = lVal.initIterator();
        Variable variable = valTable.getVar(ident.getWord().getValue());
        String name = variable.name();
        if (!funcArgNames.contains(name) && !constGlobalVal.containsKey(name)) {
            Pair<String, String> funcAndVal = new Pair<>(globalValNames.contains(name) ? Strings.GLOBAL : currentFunc, name);
            if (variable.isInteger() || variable.isArray()) {
                funcRValUseTimesMap.put(funcAndVal, Math.min(funcRValUseTimesMap.getOrDefault(funcAndVal, 0L)
                        + (1L << Math.min(multiTime * 3, 60)), (1L << 60)));
            } else {
                funcFValUseTimesMap.put(funcAndVal, Math.min(funcFValUseTimesMap.getOrDefault(funcAndVal, 0L)
                        + (1L << Math.min(multiTime * 3, 60)), (1L << 60)));
            }
        }
        if (!variable.isArray()) {
            return Val.fromVariable(variable);
        }
        ArrayList<Val> args = new ArrayList<>();
        for (TreeNode addExp = lVal.nextSon(); addExp != null; addExp = lVal.nextSon()) {
            args.add(addExpQuaternion(addExp));
        }
        return Val.fromVariable(variable).addArgs(args);
    }

    private static void lOrExpQuaternion(TreeNode lOrExp, Val trueLabel, Val falseLabel) {
        if (lOrExp.getSonCount() == 1) {
            lAndExpQuaternion(lOrExp.initIterator(), trueLabel, falseLabel);
            return;
        }
        for (TreeNode lAndExp = lOrExp.initIterator(); lAndExp != null; lAndExp = lOrExp.nextSon()) {
            Val shortLabel = Val.label(Strings.SHORT_LABEL);
            lAndExpQuaternion(lAndExp, trueLabel, shortLabel);
            qua.add(new Quaternion(Quaternion.Operator.DEFINE_LABEL, shortLabel));
        }
        qua.add(new Quaternion(Quaternion.Operator.JUMP, falseLabel));
    }

    private static void lAndExpQuaternion(TreeNode lAndExp, Val trueLabel, Val falseLabel) {
        for (TreeNode eqExp = lAndExp.initIterator(); eqExp != null; eqExp = lAndExp.nextSon()) {
            Val eqVal = eqExpQuaternion(eqExp);
            qua.add(new Quaternion(Quaternion.Operator.EQZ, eqVal, falseLabel));
        }
        qua.add(new Quaternion(Quaternion.Operator.JUMP, trueLabel));
    }

    private static Val eqExpQuaternion(TreeNode eqExp) {
        Val result = relExpQuaternion(eqExp.initIterator());
        if (eqExp.getSonCount() == 1) {
            return result;
        }
        for (TreeNode op = eqExp.nextSon(); op != null; op = eqExp.nextSon()) {
            Val eqVal = Val.label(Strings.EQ_VAL);
            Val relVal = relExpQuaternion(eqExp.nextSon());
            qua.add(new Quaternion(genOperator(op.getWord().getType()), eqVal, result, relVal));
            result = eqVal;
        }
        return result;
    }

    private static Val relExpQuaternion(TreeNode relExp) {
        Val result = addExpQuaternion(relExp.initIterator());
        if (relExp.getSonCount() == 1) {
            return result;
        }
        for (TreeNode op = relExp.nextSon(); op != null; op = relExp.nextSon()) {
            Val relVal = Val.label(Strings.REL_VAL);
            Val addVal = addExpQuaternion(relExp.nextSon());
            qua.add(new Quaternion(genOperator(op.getWord().getType()), relVal, result, addVal));
            result = relVal;
        }
        return result;
    }

    private static Quaternion.Operator genOperator(Word.Type operator) {
        return switch (operator) {
            case LSS -> Quaternion.Operator.SLT;
            case LEQ -> Quaternion.Operator.SLE;
            case GRE -> Quaternion.Operator.SGT;
            case GEQ -> Quaternion.Operator.SGE;
            case NEQ -> Quaternion.Operator.SNE;
            default -> Quaternion.Operator.SEQ;
        };
    }

    private static void globalVarDefine(TreeNode decl) {
        TreeNode constDef = decl.initIterator();
        Word type = constDef.getWord();
        for (constDef = decl.nextSon(); constDef != null; constDef = decl.nextSon()) {
            globalDef(constDef, type, false);
        }
    }

    private static void globalDef(TreeNode constDef, Word type, boolean isLocalToGlobal) {
        String name = constDef.initIterator().getWord().getValue();
        int dim = constDef.getSonCount() - 2, mulSum = 1;
        List<Integer> dims = new ArrayList<>();
        for (int i = 0; i < dim; i++) {
            int res = ConstExp.addExp(constDef.nextSon()).getInt();
            dims.add(res);
            mulSum *= res;
        }
        List<Integer> dimSum = new ArrayList<>();
        for (int i = 0; i < dim; i++) {
            mulSum /= dims.get(i);
            dimSum.add(mulSum);
        }
        boolean isInt = type.is(Word.Type.INT);
        Val val = new Val(isInt, false, name + "_" + IntHandler.get());
        if (isLocalToGlobal) {
            Util.localArrayToGlobalArray.add(val.getName());
        }
        Map<Integer, FOI> value = ConstExp.globalConstInitVal(constDef.nextSon(), dimSum,
                dims.isEmpty() ? 0 : dims.get(0), 0);
        Variable variable = new Variable(val.getName(), Variable.getType(type, constDef.getSonCount() - 2),
                constDef.getSonCount() - 2, dims, dimSum, value);
        globalValNames.add(val.getName());
        valTable.defineConst(variable, name);
        globalVar.add(variable);
        if (dims.isEmpty() && constDef.typeIs(TreeNode.Type.CONST_DEF)) {
            constGlobalVal.put(val.getName(), value.get(0));
        }
    }

    private static void specificFunctionArgs() {
        funcFParamType.put(Strings.GET_INT_ARRAY_, List.of(1));
        funcFParamType.put(Strings.GET_FLOAT_ARRAY_, List.of(1));
        funcFParamType.put(Strings.GET_INT_ARRAY,  List.of(1));
        funcFParamType.put(Strings.GET_FLOAT_ARRAY, List.of(1));
        funcFParamType.put(Strings.PUT_INT, List.of(1));
        funcFParamType.put(Strings.PUT_CHAR, List.of(1));
        funcFParamType.put(Strings.PUT_FLOAT, List.of(0));
        funcFParamType.put(Strings.PUT_INT_ARRAY_, List.of(1, 1));
        funcFParamType.put(Strings.PUT_FLOAT_ARRAY_, List.of(1, 1));
        funcFParamType.put(Strings.PUT_INT_ARRAY, List.of(1, 1));
        funcFParamType.put(Strings.PUT_FLOAT_ARRAY, List.of(1, 1));
    }

    public static class VarTable {

        private final Map<String, Stack<String>> names = new HashMap<>();
        private final LinkedList<Map<String, Variable>> table = new LinkedList<>();

        private VarTable() {
            table.addLast(new HashMap<>());
        }

        private void init() {
            names.clear();
            table.clear();
            table.addLast(new HashMap<>());
        }

        private void add() {
            table.addLast(new HashMap<>());
        }

        private void pop() {
            table.removeLast();
        }

        private void defineConst(Variable variable, String name) {
            if (!names.containsKey(name)) {
                names.put(name, new Stack<>());
            }
            names.get(name).add(variable.name());
            table.getLast().put(name, variable);
        }

        public FOI getValue(String name, List<Integer> dim) {
            ListIterator<Map<String, Variable>> iterator = table.listIterator(table.size());
            while (iterator.hasPrevious()) {
                Variable variable = iterator.previous().get(name);
                if (variable != null) {
                    return variable.getValue(dim).copy();
                }
            }
            return FOI.ZERO;
        }
        
        public Variable getVar(String name) {
            ListIterator<Map<String, Variable>> iterator = table.listIterator(table.size());
            while (iterator.hasPrevious()) {
                Variable variable = iterator.previous().get(name);
                if (variable != null) {
                    return variable;
                }
            }
            return null;
        }

    }

    public record QuaResult(Map<Pair<String, Integer>, DoubleList<Quaternion>> funcQua, List<Variable> globalVar,
                            Map<String, Integer> funcValSpace, DoubleList<Pair<String, Integer>> funcOrder,
                            Map<String, Boolean> funcWillCallOtherFunc) { }

}
