package IR.Type;

import java.util.ArrayList;

public class ArrayType extends Type{
    int num;
    Type eleType;

    public ArrayType(int num, Type eleType){
        this.num = num;
        this.eleType = eleType;
    }

    @Override
    public boolean isArrayType() {
        return true;
    }

    //  获取每个维度的num
//    public ArrayList<Integer> getDimList(){
//        ArrayList<Integer> dimList = new ArrayList<>();
//        dimList.add(num);
//        if(eleType.isArrayType()){
//            dimList.addAll(((ArrayType) eleType).getDimList());
//        }
//        return dimList;
//    }

    public int getDim(){
        if(eleType.isArrayType()){
            return ((ArrayType) eleType).getDim() + 1;
        }
        else return 1;
    }

    public ArrayList<Integer> getNumList(){
        ArrayList<Integer> numList = new ArrayList<>();
        numList.add(num);
        if(eleType.isArrayType()){
            numList.addAll(((ArrayType) eleType).getNumList());
        }
        return numList;
    }

    public boolean isIntegerArr(){
        if(eleType.isArrayType()){
            return ((ArrayType) eleType).isIntegerArr();
        }
        return eleType.isIntegerTy();
    }

    public int getNum(){
        return num;
    }

    public Type getEleType(){
        return eleType;
    }

    public int getTotalSize(){
        if(eleType.isArrayType()){
            return ((ArrayType) eleType).getTotalSize() * num;
        }
        else return num;
    }

    @Override
    public String toString(){
        return "[" + num + " x " + eleType + "]";
    }
}
