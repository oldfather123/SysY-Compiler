package backend.ir.entity;

import frontend.semantic.symbol.IRBasicType;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.List;

/**
 * &#064;Classname GlobalVar
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:18
 * &#064;Created MuJue
 */
public class GlobalVar extends User{
    public GlobalVar(IRBasicType IRBasicType, String name, boolean isConst, boolean isArray, List<Integer> dim){
        super(name, IRBasicType, isConst, isArray, dim);
    }
    public boolean isInit(){
        return !usees.isEmpty();
    }
    public boolean isConst() {
        return varInfo.isConst();
    }

    public boolean isArray() {
        return varInfo.isArray();
    }

    public List<Integer> getDimList() {
        return varInfo.getFullDim();
    }

    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        fout.write("@" + name + " = ");
        if(varInfo.isArray() && getBasicType() == IRBasicType.I8){
            fout.write("private unnamed_addr ");
        }
        else{
            fout.write("dso_local ");
        }
        if (varInfo.isConst()) {
            fout.write("constant ");
        } else {
            fout.write("global ");
        }
        if(usees.isEmpty()){
            if(isArray()){
                fout.write(getTypeStr() + " zeroinitializer\n");
                return;
            }
            else{
                usees.add(new LiteralConst(0, getBasicType()));
            }
        }
        usees.get(0).printAssembly(fout);
        fout.write("\n");
    }

    @Override
    public void printName(BufferedWriter fout) throws IOException {
        try {
            fout.write("@" + name);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void printUse(BufferedWriter fout) throws IOException {
        try {
            for (int i = 0; i < usees.size(); i++) {
                if (i > 0) fout.write(", ");
                usees.get(i).printName(fout);
            }
        } catch (IOException e) {
            e.printStackTrace();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}
