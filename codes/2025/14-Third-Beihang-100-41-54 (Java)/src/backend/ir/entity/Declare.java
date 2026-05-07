package backend.ir.entity;

import frontend.semantic.symbol.IRBasicType;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * &#064;Classname Declare
 * &#064;Description  TODO
 * &#064;Date 2025/7/12 15:23
 * &#064;Created MuJue
 */
public class Declare extends Value{
    private List<Argument> arguments =  new ArrayList<>();
    public Declare(String name, IRBasicType IRBasicType) {
        super(name, IRBasicType);
    }
    public void addArgument(Argument argument){
        arguments.add(argument);
    }

    public List<Argument> getArguments() {
        return arguments;
    }

    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        fout.write("declare ");
        fout.write(getTypeStr() + " ");
        fout.write("@" + name);

        int argSize = arguments.size();

        fout.write("(");
        for(int i = 0; i < argSize; i++){
            Argument argument = arguments.get(i);
            argument.printAssembly(fout);
            if (i != argSize - 1) {
                fout.write(",");
            }
        }
        fout.write(")\n");
    }

    @Override
    public void printName(BufferedWriter fout) throws IOException {
        fout.write("@" + name);
    }

    @Override
    public void printUse(BufferedWriter fout) throws IOException {

    }
}
