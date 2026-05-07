package front.AST.Func;

import front.AST.Exp.ConstExp;
import mid.IntermediatePresentation.Instruction.Alloca;
import mid.IntermediatePresentation.Instruction.Store;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;
import front.AST.Exp.Exp;
import front.AST.Node;
import front.AST.TokenNode;
import front.AST.Var.BType;
import utils.type.SyntaxType;

import java.util.ArrayList;
import java.util.Objects;

// FuncFParam ==> BType Ident [ '[' ']' { '[' Exp ']' } ]
public class FuncFParam extends Node {


    public FuncFParam(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Value toIR() {
        // 数组某一维度的最大长度，用于填充空缺长度
        int MAXLEN = 1000;

        boolean isInt = true;
        String ident = ((TokenNode) children.get(1)).getToken().getValue();
        ArrayList<Node> exps = new ArrayList<>();

        for (Node child : children) {
            if (child instanceof BType bType) {
                isInt = bType.isInt();
            } else if (child instanceof Exp || child instanceof ConstExp) {
                exps.add(child);
            }
        }

        Value fPara, paraAddr = null;
        if (children.size() == 2) {
            fPara = new Value(irManager.declareParam(), isInt ? ValueType.I32 : ValueType.FLT);

            Alloca p = new Alloca(isInt);
            new Store(fPara, p);
            symbolTableManager.varDecl(ident, false, 0, new ArrayList<>());
            paraAddr = p;
        } else {
            fPara = new Value(irManager.declareParam(), isInt ? ValueType.PI32 : ValueType.PFLT);

            ArrayList<Integer> dimLens = new ArrayList<>();
            dimLens.add(MAXLEN);
            for (Node exp : exps) {
                dimLens.add(exp.evaluate().intValue());
            }
            symbolTableManager.varDecl(ident, false, dimLens.size(), dimLens);
        }
        // 如果参数传入的是值，则必须先alloca，使用对应的addr；否则直接用传入的地址
        symbolTableManager.setIRValue(ident, Objects.requireNonNullElse(paraAddr, fPara));
        return fPara;
    }
}
