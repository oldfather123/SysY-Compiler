package backend.ir.entity;

import frontend.semantic.symbol.IRBasicType;

import java.io.BufferedWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class ArrayValue extends Value{
    public List<Integer> dimList;

    public ArrayList<Integer> dimensionPlaceList = new ArrayList<>();

    public ArrayList<Value> arrayValueUseList = new ArrayList<>();

    public ArrayValue(String name, IRBasicType IRBasicType) {
        super(name, IRBasicType);
    }
    public void flatten(){

    }
    public String genDimStr(){
        StringBuilder sb = new StringBuilder();
        int dimSize = dimList.size();
        for(int i = 0; i < dimSize; i++){
            sb.append("[").append(dimList.get(i)).append(" x ");
        }
        sb.append(getTypeStr());
        for(int i = 0; i < dimSize; i++){
            sb.append("]");
        }
        return sb.toString();
    }
    @Override
    public void printAssembly(BufferedWriter fout) throws IOException {
        fout.write(genDimStr() + " ");
        fout.write("[");
        int arraySize = arrayValueUseList.size();
        for(int i = 0; i < arraySize; i++){
            arrayValueUseList.get(i).printAssembly(fout);
            if(i != arraySize - 1){
                fout.write(",");
            }
        }
        fout.write("]");
    }

    @Override
    public void printName(BufferedWriter fout) throws IOException {

    }

    @Override
    public void printUse(BufferedWriter fout) throws IOException {

    }
}
