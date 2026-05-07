package frontend.syntactic.entity.ast;

import frontend.lexer.entity.Token;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;

public class StmtNode {

    public StmtType stmtType;
    public LValStmtNode lValStmtNode;
    // [Exp];
    public ExpStmtNode expStmtNode;
    public BlockStmtNode blockStmtNode;
    public IfStmtNode ifStmtNode;
    public WhileStmtNode whileStmtNode;
    public BreakStmtNode breakStmtNode;
    public ContinueStmtNode continueStmtNode;
    public ReturnStmtNode returnStmtNode;
    public PutFStmtNode putfStmtNode;

    public static class LValStmtNode extends StmtNode {
        public LValNode lValNode;
        public ExpNode expNode;

        public void syntacticDebug(BufferedWriter writer) throws IOException{
            lValNode.syntacticDebug(writer);
            expNode.syntacticDebug(writer);
        }
    }

    public static class ExpStmtNode extends StmtNode {
        public ExpNode expNode = null;
        public boolean hasExp(){ return expNode != null;}

        public void syntacticDebug(BufferedWriter writer) throws IOException{
            if(hasExp()){
                expNode.syntacticDebug(writer);
            }
        }
    }

    public static class BlockStmtNode extends StmtNode {
        public BlockNode blockNode;

        public void syntacticDebug(BufferedWriter writer) throws IOException{
            blockNode.syntacticDebug(writer);
        }
    }

    public static class IfStmtNode extends StmtNode {
        public CondNode condNode = null;
        public StmtNode stmtNode;
        public StmtNode elseStmtNode = null;

        public boolean hasCond(){ return condNode != null;}
        public boolean hasElseStmt(){ return elseStmtNode != null;}

        public void syntacticDebug(BufferedWriter writer) throws IOException{
            if(hasCond()){
                condNode.syntacticDebug(writer);
            }
            stmtNode.syntacticDebug(writer);
            if(hasElseStmt()){
                elseStmtNode.syntacticDebug(writer);
            }
        }
    }

    public static class WhileStmtNode extends StmtNode {
        public CondNode condNode = null;
        public StmtNode stmtNode;
        public boolean hasCond(){return condNode != null;}

        public void syntacticDebug(BufferedWriter writer) throws IOException{
            if(hasCond()){
                condNode.syntacticDebug(writer);
            }
            stmtNode.syntacticDebug(writer);
        }
    }

    public static class BreakStmtNode extends StmtNode {

        public void syntacticDebug(BufferedWriter writer) throws IOException{
            ;
        }
    }

    public static class ContinueStmtNode extends StmtNode {

        public void syntacticDebug(BufferedWriter writer) throws IOException{
            ;
        }
    }

    public static class ReturnStmtNode extends StmtNode {
        public ExpNode expNode = null;
        public boolean hasExp(){return expNode != null;}

        public void syntacticDebug(BufferedWriter writer) throws IOException{
            if(hasExp()){
                expNode.syntacticDebug(writer);
            }
        }
    }
    public static class PutFStmtNode extends StmtNode{
        public Token strConstToken;
        public ArrayList<ExpNode> expParamNodeList = null;
        public Boolean hasParam() {return expParamNodeList != null;}
        public void syntacticDebug(BufferedWriter writer) throws IOException{
            writer.write(strConstToken.content + "\n");
            if(hasParam()){
                for(ExpNode expNode: expParamNodeList){
                    expNode.syntacticDebug(writer);
                }
            }
        }
    }

    public enum StmtType{
        LVAL_STMT("<lVal_stmt>"),
        EXP_STMT("<exp_stmt>"),
        BLOCK_STMT("<block_stmt>"),
        IF_STMT("<if_stmt>"),
        WHILE_STMT("<while_stmt>"),

        BREAK_STMT("<break_stmt>"),
        CONTINUE_STMT("<continue_stmt>"),
        RETURN_STMT("<return_stmt>"),
        PUT_F_STMT("<putf_stmt>");
        private final String syntacticName;
        private StmtType(String name){
            syntacticName = name;
        }
        public String getSyntacticName(){
            return syntacticName;
        }
    }

    public void syntacticDebug(BufferedWriter writer) throws IOException {
//        writer.write("<Stmt>\n");
        writer.write(stmtType.getSyntacticName() + "\n");
        if(stmtType == StmtType.LVAL_STMT){
            lValStmtNode.syntacticDebug(writer);
        }
        else if(stmtType == StmtType.EXP_STMT){
            expStmtNode.syntacticDebug(writer);
        }
        else if(stmtType == StmtType.BLOCK_STMT){
            blockStmtNode.syntacticDebug(writer);
        }
        else if(stmtType == StmtType.IF_STMT){
            ifStmtNode.syntacticDebug(writer);
        }
        else if(stmtType == StmtType.WHILE_STMT){
            whileStmtNode.syntacticDebug(writer);
        }
        else if(stmtType == StmtType.BREAK_STMT){
            breakStmtNode.syntacticDebug(writer);
        }
        else if(stmtType == StmtType.CONTINUE_STMT){
            continueStmtNode.syntacticDebug(writer);
        }
        else if(stmtType == StmtType.RETURN_STMT){
            returnStmtNode.syntacticDebug(writer);
        }
        else if(stmtType == StmtType.PUT_F_STMT){
            putfStmtNode.syntacticDebug(writer);
        }
    }
}
