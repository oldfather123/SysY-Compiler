package frontend.semantic.symbol;

import java.util.List;
import java.util.Objects;

public class VarInfo {
    private IRBasicType basicType;
    private final boolean isConst;
    private final boolean isArray;
    private List<Integer> dimensions;

    public VarInfo(IRBasicType basicType, boolean isConst, boolean isArray, List<Integer> dimensions){
        this.basicType = basicType;
        this.isArray = isArray;
        this.isConst = isConst;
        this.dimensions = dimensions;
    }
    public VarInfo(IRBasicType basicType){
        this.basicType = basicType;
        this.isConst = false;
        this.isArray = false;
        this.dimensions = null;
    }
    public Integer getBasicBytes(){
        if(isPointer() || basicType == IRBasicType.I64) return 8;
        return 4;
    }
    public Integer getTotalBytes(){
        if(isPointer()){
            return 8;
        }
        int baseCount = 1;
        if(isArray()){
            for(Integer dim : dimensions){
                if(dim == -1) continue;
                baseCount *= dim;
            }
        }
        return getBasicBytes() * baseCount;
    }
    public Boolean isInt(){
        return basicType == IRBasicType.I32 || basicType == IRBasicType.I64;
    }
    public Boolean isFloat(){
        return basicType == IRBasicType.FLOAT;
    }
    public Boolean isChar() {return basicType == IRBasicType.I8; }
    public Boolean isBoolean() {return basicType == IRBasicType.I1; }
    public Boolean isString() {
        return isChar() && isArray();
    }
    public boolean isArray() {
        return isArray;
    }
    public boolean isConst() {
        return isConst;
    }
    public boolean isVoid(){
        return basicType == IRBasicType.VOID;
    }
    public boolean isPointer() {
        return  dimensions != null && dimensions.getLast() == -1;
    }
    public int getDimSize(){
        assert dimensions != null;
        int space = 1;
        for(Integer dim : dimensions){
            if(dim == -1) continue;
            space *= dim;
        }
        return space;
    }
    public IRBasicType getBasicType() {
        return basicType;
    }
    public void setBasicType(IRBasicType basicType) {this.basicType = basicType;}
    public List<Integer> getFullDim() {
        return dimensions;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;
        VarInfo varInfo = (VarInfo) o;
        return isArray == varInfo.isArray && basicType == varInfo.basicType && Objects.equals(dimensions, varInfo.dimensions);
    }

    @Override
    public int hashCode() {
        return Objects.hash(basicType, isConst, isArray, dimensions);
    }

    public String getTypeStr(){
        StringBuilder sb = new StringBuilder();
        if(isArray){
            int dimSize = dimensions.size();
            if(dimensions.getLast() == -1){
                dimSize--;
            }
            for(int i = 0; i < dimSize; i++){
                sb.append("[").append(dimensions.get(i)).append(" x ");
            }
            sb.append(basicType.toString());
            for(int i = 0; i < dimSize; i++){
                sb.append("]");
            }
            if(dimensions.getLast() == -1){
                sb.append("*");
            }
        }
        else{
            sb.append(basicType.toString());
        }
        return sb.toString();
    }
}
