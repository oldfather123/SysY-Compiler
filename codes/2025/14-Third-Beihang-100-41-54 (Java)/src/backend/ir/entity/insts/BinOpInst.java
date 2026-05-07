package backend.ir.entity.insts;

import backend.ir.entity.BasicBlock;
import backend.ir.entity.Value;
import frontend.lexer.entity.Token;
import frontend.lexer.entity.TokenType;
import frontend.semantic.symbol.IRBasicType;

import java.io.BufferedWriter;
import java.io.IOException;

/**
 * &#064;Classname BinOpInst
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:20
 * &#064;Created MuJue
 */
public class BinOpInst extends Inst{
    public TokenType opType;
    private IRBasicType opIRBasicType;
    // 对于BinOpInst而言，运算数的类型就是其运算结果的类型
    public BinOpInst(String name, IRBasicType opIRBasicType, TokenType opType, BasicBlock basicBlock) {
        super(name, opIRBasicType, basicBlock);
        this.opType = opType;
        this.opIRBasicType = opIRBasicType;
    }

    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        if (usees.size() < 2) return;
        
        Value operand1 = usees.get(0);
        Value operand2 = usees.get(1);
        
        String op = getBinInstOpStr(opType);
        
        fout.write(name + " = " + op + " " + getTypeStr() + " ");
        operand1.printName(fout);
        fout.write(", ");
        operand2.printName(fout);
    }
    private String getBinInstOpStr(TokenType opType){
        if(opIRBasicType == IRBasicType.FLOAT){
            return switch (opType) {
                case ADD -> "fadd";
                case SUB -> "fsub";
                case MUL -> "fmul";
                case DIV -> "fdiv";
                case MOD -> "frem";
                case XOR -> "xor";
                default -> "add"; // 默认情况
            };
        }
        else{
            return switch (opType) {
                case ADD -> "add";
                case SUB -> "sub";
                case MUL -> "mul";
                case DIV -> "sdiv";
                case MOD -> "srem";
                case XOR -> "xor";
                default -> "add"; // 默认情况
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
