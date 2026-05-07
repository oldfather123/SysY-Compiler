package midend.analysis;

import midend.ConstInt;
import midend.Value;
import midend.instr.GetElementPtrInstr;
import midend.instr.LoadInstr;

import java.util.ArrayList;

public class AliasAnalysis {
    public static ArrayList<Value> getRoot(Value pointer) {
        Value current = pointer;
        ArrayList<Value> resultList = new ArrayList<>();
        resultList.add(pointer);

        while (true) {
            if (current instanceof GetElementPtrInstr) {
                current = ((GetElementPtrInstr) current).getTarget();
                resultList.add(current);
            }else if (current instanceof LoadInstr) {
                current = ((LoadInstr) current).getValue();
                resultList.add(current);
            }
            else {
                break;
            }
        }
        return resultList;
    }

    public static boolean isSameRoot(Value left, Value right) {
        ArrayList<Value> leftRoots = getRoot(left);
        ArrayList<Value> rightRoots = getRoot(right);

        Value lastLeft = leftRoots.get(leftRoots.size() - 1);
        Value lastRight = rightRoots.get(rightRoots.size() - 1);

        return lastLeft.equals(lastRight);
    }

    public static boolean hasReg(Value value) {
        ArrayList<Value> rootValues = getRoot(value);

        for (int i = rootValues.size() - 2; i >= 0; i--) {
            Value currentValue = rootValues.get(i);

            if (currentValue instanceof GetElementPtrInstr gepInstr) {
                ArrayList<Value> indexes = gepInstr.getIndexes();
                for (Value indexValue : indexes) {
                    if (!(indexValue instanceof ConstInt)) {
                        return true;
                    }
                }
            }
        }

        return false;
    }
    public static boolean isSame(Value left, Value right) {
        ArrayList<Value> left0 = getRoot(left);
        ArrayList<Value> right0 = getRoot(right);
//        if (left0.size() == 3 || right0.size() == 3) {
//            System.out.println("");
//        }
        if (left0.get(left0.size() - 1).equals(right0.get(right0.size() - 1))) {
            if (left0.size() == right0.size() && left0.size() == 1) {
                return true;
            } else if (left0.size() == 1 || right0.size() == 1) {
                return false;
            }
            //alloca相等，可以理解为只有数组才会如此,下一个为load或者getelementptr
            if (left0.get(left0.size() - 2) instanceof GetElementPtrInstr getElementPtrInstr1 && right0.get(right0.size() - 2) instanceof GetElementPtrInstr getElementPtrInstr2) {
                if (getElementPtrInstr1.getIndexes().size() != getElementPtrInstr2.getIndexes().size()) {
                    return false;
                } else {
                    //boolean isEqual = true;
                    for (int count = 0; count < getElementPtrInstr1.getIndexes().size(); count++) {
                        Value value1 = getElementPtrInstr1.getIndexes().get(count);
                        Value value2 = getElementPtrInstr2.getIndexes().get(count);
                        if (value1 instanceof ConstInt && value2 instanceof ConstInt) {
                            if (((ConstInt) value1).getValue() != ((ConstInt) value2).getValue()) {
                                return left0.size() != right0.size();
                            }
                        } else {
                            return true; //MaySame but there set same
                        }
                    }
                    return true;
                }
            } else {
                GetElementPtrInstr getElementPtrInstr1 = (GetElementPtrInstr) left0.get(left0.size() - 3);
                GetElementPtrInstr getElementPtrInstr2 = (GetElementPtrInstr) right0.get(right0.size() - 3);
                if (getElementPtrInstr1.getIndexes().size() != getElementPtrInstr2.getIndexes().size()) {
                    return false;
                } else {
                    //boolean isEqual = true;
                    for (int count = 0; count < getElementPtrInstr1.getIndexes().size(); count++) {
                        Value value1 = getElementPtrInstr1.getIndexes().get(count);
                        Value value2 = getElementPtrInstr2.getIndexes().get(count);
                        if (value1 instanceof ConstInt && value2 instanceof ConstInt) {
                            if (((ConstInt) value1).getValue() != ((ConstInt) value2).getValue()) {
                                if (left0.size() != right0.size()) {
                                    return true;
                                }
                                return false;
                            }
                        } else {
                            return true; //MaySame but there set same
                        }
                    }
                    return true;
                }
            }
        }
        return false;
    }
}
