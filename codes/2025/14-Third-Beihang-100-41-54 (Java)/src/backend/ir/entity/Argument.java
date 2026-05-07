package backend.ir.entity;

import frontend.semantic.symbol.IRBasicType;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.List;

/**
 * &#064;Classname Argument
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:23
 * &#064;Created MuJue
 */
public class Argument extends Value{
    public List<Integer> getDimList(){
        return varInfo.getFullDim();
    }
    public Argument(String name, IRBasicType IRBasicType, boolean isConst, boolean isArray, List<Integer> dimensions){
        super(name, IRBasicType, isConst, isArray, dimensions);
    }

    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        String type = varInfo.getTypeStr();
        fout.write(type + " " + name);
    }

    @Override
    public void printName(BufferedWriter fout) throws IOException {
        fout.write(name);
    }

    @Override
    public void printUse(BufferedWriter fout) throws IOException {

    }
}
