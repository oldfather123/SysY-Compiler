package midpass;

import midend.value.Function;

import java.util.ArrayList;

public class MidPassManager {
    ArrayList<Function> functions;

    public MidPassManager(ArrayList<Function> functions) {
        this.functions = functions;
        new MakeDomTree(functions);
        new Mem2Reg(functions);
        new GVN(functions);
        new ReplacePhi(functions);
        //new MakeDomTree(functions);
        //new MergeBB(functions);
    }

}
