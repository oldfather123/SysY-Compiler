package ir.type;

public abstract class Type {
    private TypeName typeN;
    private boolean innerMost = false;
    public Type(TypeName TypeN){
        this.typeN= TypeN;
    }
    public TypeName getType(){
        return typeN;
    }

    public boolean isInnerMost(){
        return innerMost;
    }

    public void setInnerMost(boolean tmp){
        this.innerMost = tmp;
    }
    public boolean isFloat(){
        return this.typeN == TypeName.F;
    }

    //these 2 method is for backend to allocate space and locate
    public abstract int getSpace();
    public abstract int getOffset();

    @Override
    public boolean equals(Object obj) {
        if(!(obj instanceof Type)){
            return false;
        }
        if(typeN != TypeName.A && typeN != TypeName.P){
            return ((Type) obj).typeN == this.typeN;
        } else {
            if(((Type)obj).typeN!=TypeName.A && ((Type)obj).typeN!=TypeName.P){
                return false;
            }else{
                if(typeN == TypeName.A) {
                    return ((ArrayType)this).equals(obj);
                } else {
                    return ((Pointer)this).equals(obj);
                }
            }
        }
    }

    public Type innerType(){
        Type t = this;
        while(t instanceof ArrayType){
            t = ((ArrayType)t).t;
        }
        return t;
    }

    public Type innerType(int i){
        Type t = this;
        for(int j = 0;j<i;j++){
            t = ((ArrayType)t).t;
        }
        return t;
    }

    public abstract String toString();
}
