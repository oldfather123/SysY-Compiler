package front.AST.Exp;

import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.Instruction.ALU;
import mid.IntermediatePresentation.Instruction.Call;
import mid.IntermediatePresentation.Instruction.Cmp;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;
import front.AST.Func.FuncRParams;
import front.AST.Node;
import front.AST.TokenNode;
import utils.type.SyntaxType;

import java.util.ArrayList;

// UnaryExp ==> PrimaryExp | Ident '(' [ FuncRParams ] ')' | UnaryOp UnaryExp
public class UnaryExp extends Node {

    public UnaryExp(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Number evaluate() {
        if (children.get(0) instanceof UnaryOp op) {
            if (((TokenNode) op.children.get(0)).getToken().getValue().equals("-")) {
                Number n = children.get(1).evaluate();
                if (n instanceof Integer i) {
                    return -i;
                } else if (n instanceof Float f) {
                    return -f;
                } else {
                    return null;
                }
            } else if (((TokenNode) op.children.get(0)).getToken().getValue().equals("+")) {
                return children.get(1).evaluate();
            } else {
                return null;
            }
        } else if (children.get(0) instanceof PrimaryExp) {
            return children.get(0).evaluate();
        } else {
            //函数调用不管
            return null;
        }
    }

    public Value toIR() {
        String str = getTokenValue();
        if (children.size() == 1) {
            return children.get(0).toIR();
        } else if (str.equals("+")) {
            return children.get(1).toIR();
        } else if (str.equals("-")) {
            Value op1 = new ConstNumber(0);
            Value op2 = children.get(1).toIR();
            boolean isInt = !(op1.isFloat() || op2.isFloat());
            return new ALU(op1.withType(isInt), "-", op2.withType(isInt), isInt);
        } else if (str.equals("!")) {
            Value op2 = children.get(1).toIR();
            boolean isInt = !op2.isFloat();
            return new Cmp("==", (new ConstNumber(0)).withType(isInt),
                    op2.withType(isInt));
        } else {
            //函数调用
            ArrayList<Value> params = new ArrayList<>();
            for (Node child : children) {
                if (child instanceof FuncRParams) {
                    for (Node param : child.getChildren()) {
                        if (param instanceof Exp) {
                            params.add(param.toIR());
                        }
                    }
                }
            }

            if (str.equals("starttime") || str.equals("stoptime")) {
                str = "_sysy_" + str;
            }
            Function function = (Function) symbolTableManager.getIRValue("@" + str);
            ArrayList<Value> fParams = function.getParam().getParams();
            ArrayList<Value> rParams = new ArrayList<>();
            for (int i = 0; i < params.size(); i++) {
                Value v = params.get(i);
                ValueType paramType = fParams.get(i).getType();
                if (v.getType() != paramType) {
                    /*
                        如果参数需要指针，则传入的是数组指针；否则传入的是值
                        传值需要从内存中取值，而且可能需要类型转换
                        传数组，已经在子节点中处理过了，全部改为了指针
                     */
                    if (!paramType.isPointer()) {
                        v = v.withType(paramType == ValueType.I32);
                    }
                }
                rParams.add(v);
            }

            return new Call(function, rParams);
        }
    }

}
