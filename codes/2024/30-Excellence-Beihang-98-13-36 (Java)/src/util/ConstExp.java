package util;

import entities.FOI;
import entities.TreeNode;
import entities.Word;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static handler.QuaHandler.valTable;

public class ConstExp {

    public static FOI addExp(TreeNode addExp) {
        FOI result = mulExp(addExp.initIterator());
        for (TreeNode op = addExp.nextSon(), mulExp = addExp.nextSon();
             op != null; op = addExp.nextSon(), mulExp = addExp.nextSon()) {
            if (op.getWord().is(Word.Type.PLUS)) {
                result.add(mulExp(mulExp));
            } else if (op.getWord().is(Word.Type.MINU)) {
                result.sub(mulExp(mulExp));
            }
        }
        return result;
    }

    public static FOI mulExp(TreeNode mulExp) {
        FOI result = unaryExp(mulExp.initIterator());
        for (TreeNode op = mulExp.nextSon(), unaryExp = mulExp.nextSon();
             op != null; op = mulExp.nextSon(), unaryExp = mulExp.nextSon()) {
            if (op.getWord().is(Word.Type.MULT)) {
                result.mul(unaryExp(unaryExp));
            } else if (op.getWord().is(Word.Type.DIV)) {
                result.div(unaryExp(unaryExp));
            } else if (op.getWord().is(Word.Type.MOD)) {
                result.mod(unaryExp(unaryExp));
            }
        }
        return result;
    }

    public static FOI unaryExp(TreeNode unaryExp) {
        TreeNode firstSon = unaryExp.initIterator();
        if (firstSon.getWord() != null) {
            if (firstSon.getWord().is(Word.Type.PLUS)) {
                return unaryExp(unaryExp.nextSon());
            }
            return unaryExp(unaryExp.nextSon()).neg();
        } else {
            return primaryExp(firstSon);
        }
    }

    public static FOI primaryExp(TreeNode primaryExp) {
        TreeNode firstSon = primaryExp.initIterator();
        if (firstSon.typeIs(TreeNode.Type.ADD_EXP)) {
            return addExp(firstSon);
        } else if (firstSon.getWord() != null && firstSon.getWord().is(Word.Type.INT_CONST)) {
            return new FOI(NumberHandler.getInteger(firstSon.getWord().getValue()));
        } else if (firstSon.getWord() != null && firstSon.getWord().is(Word.Type.FLOAT_CONST)) {
            return new FOI(NumberHandler.getFloat(firstSon.getWord().getValue()));
        } else {
            return lVal(firstSon);
        }
    }

    public static FOI lVal(TreeNode lVal) {
        List<Integer> dims = new ArrayList<>();
        TreeNode ident = lVal.initIterator();
        for (TreeNode addExp = lVal.nextSon(); addExp != null; addExp = lVal.nextSon()) {
            dims.add(addExp(addExp).getInt());
        }
        return valTable.getValue(ident.getWord().getValue(), dims);
    }

    public static Map<Integer, FOI> globalConstInitVal(TreeNode constInitVal, List<Integer> dimSum, int dim0, int index) {
        Map<Integer, FOI> result = new HashMap<>();
        CNT = 0;
        globalConstInitVal(constInitVal, dimSum, dim0, index, result);
        return result;
    }

    private static int CNT = 0;

    private static void globalConstInitVal(TreeNode constInitVal, List<Integer> dimSum, int dim0, int index, Map<Integer, FOI> result) {
        if (dimSum.isEmpty()) {
            result.put(0, addExp(constInitVal));
            return;
        }
        TreeNode constExp = constInitVal.initIterator();
        for (int i = 0; i < dim0; ++i) {
            if (constExp == null) {
                CNT += dimSum.get(index);
            } else if (constExp.typeIs(TreeNode.Type.ADD_EXP)) {
                int maxRead = dimSum.get(index) - 1;
                result.put(CNT++, addExp(constExp));
                while (maxRead != 0) {
                    constExp = constInitVal.currentSon();
                    if (constExp == null || !constExp.typeIs(TreeNode.Type.ADD_EXP)) {
                        break;
                    }
                    constExp = constInitVal.nextSon();
                    maxRead--;
                    result.put(CNT++, addExp(constExp));
                }
                CNT += maxRead;
            } else {
                globalConstInitVal(constExp, dimSum, dimSum.get(index) / dimSum.get(index + 1), index + 1, result);
            }
            constExp = constInitVal.nextSon();
        }
    }

    public static Map<Integer, FOI> constInitVal(TreeNode constInitVal, List<Integer> dimSum, int dim0, int index) {
        Map<Integer, FOI> result = new HashMap<>();
        CNT = 0;
        constInitVal(constInitVal, dimSum, dim0, index, result);
        return result;
    }

    private static void constInitVal(TreeNode constInitVal, List<Integer> dimSum, int dim0, int index, Map<Integer, FOI> result) {
        if (dimSum.isEmpty()) {
            result.put(0, addExp(constInitVal));
            return;
        }
        TreeNode constExp = constInitVal.initIterator();
        for (int i = 0; i < dim0; ++i) {
            if (constExp == null) {
                int maxRead = dimSum.get(index);
                while (maxRead != 0) {
                    maxRead--;
                    result.put(CNT++, FOI.ZERO);
                }
            } else if (constExp.typeIs(TreeNode.Type.ADD_EXP)) {
                int maxRead = dimSum.get(index) - 1;
                result.put(CNT++, addExp(constExp));
                while (maxRead != 0) {
                    constExp = constInitVal.currentSon();
                    if (constExp == null || !constExp.typeIs(TreeNode.Type.ADD_EXP)) {
                        break;
                    }
                    constExp = constInitVal.nextSon();
                    maxRead--;
                    result.put(CNT++, addExp(constExp));
                }
                while (maxRead != 0) {
                    maxRead--;
                    result.put(CNT++, FOI.ZERO);
                }
            } else {
                constInitVal(constExp, dimSum, dimSum.get(index) / dimSum.get(index + 1), index + 1, result);
            }
            constExp = constInitVal.nextSon();
        }
    }

}
