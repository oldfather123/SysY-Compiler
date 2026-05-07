package backend.ir.entity.insts;

import backend.ir.entity.BasicBlock;
import backend.ir.entity.Value;
import frontend.lexer.entity.Token;
import frontend.lexer.entity.TokenType;
import frontend.semantic.symbol.IRBasicType;

import java.io.BufferedWriter;
import java.io.IOException;

/**
 * &#064;Classname CmpInst
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:20
 * &#064;Created MuJue
 */
public class CmpInst extends Inst{
    public TokenType opType;
    private IRBasicType opBasicType;
    public CmpInst(String name, IRBasicType opBasicType, TokenType opType, BasicBlock basicBlock) {
        super(name, IRBasicType.I1, basicBlock);
        this.opType = opType;
        this.opBasicType = opBasicType;
    }

    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        if (usees.size() < 2) return;
        
        Value operand1 = usees.get(0);
        Value operand2 = usees.get(1);
        
        String op = getCmpOperatorString();
        String typeStr = opBasicType.toString();

        if(opBasicType == IRBasicType.FLOAT){
            fout.write(name + " = fcmp " + op + " " + typeStr + " ");
        }
        else if(opBasicType == IRBasicType.I32){
            fout.write(name + " = icmp " + op + " " + typeStr + " ");
        }

        operand1.printName(fout);
        fout.write(", ");
        operand2.printName(fout);
    }
    
    private String getCmpOperatorString() {
        if(opBasicType == IRBasicType.I32){
            return switch (opType) {
                case EQ -> "eq";
                case NE -> "ne";
                case LT -> "slt";
                case GT -> "sgt";
                case LE -> "sle";
                case GE -> "sge";
                default -> "eq";
            };
        }
        else {
            return switch (opType) {
                case EQ -> "oeq";
                case NE -> "one";
                case LT -> "olt";
                case GT -> "ogt";
                case LE -> "ole";
                case GE -> "oge";
                default -> "oeq";
            };
        }
    }

    @Override
    public void printName(BufferedWriter fout) throws IOException {
        try {
            fout.write(name);
        } catch (IOException e) {
            // 处理异常
        }
    }

    @Override
    public void printUse(BufferedWriter fout) throws IOException {
        try {
            if (usees.size() >= 2) {
                usees.get(0).printName(fout);
                fout.write(", ");
                usees.get(1).printName(fout);
            }
        } catch (IOException e) {
            // 处理异常
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    @Override
    public void replaceWithNewValue(Value oldValue,Value newValue){
        super.replaceWithNewValue(oldValue,newValue);
    }
}
