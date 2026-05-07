package ir.type;

import tools.ErrOutput;

import java.util.ArrayList;

public class ArrayType extends Type {
    public int getSize() {
        return size;
    }

    public int size;
    public Type t;

    public ArrayType(int i, Type t) {
        super(TypeName.A);
        this.size = i;
        this.t = t;
    }

    public Type getT() {
        return t;
    }

    @Override
    public String toString() {
        return "[" + String.valueOf(size) + " x " + t.toString() + "]";
    }

    @Override
    public int getSpace() {
        return size * this.t.getSpace();
    }

    @Override
    public int getOffset() {
        if (super.isInnerMost()) {
            return t.getOffset();
        } else {
            return size * t.getOffset();
        }
    }

    public Type getElemType() {
        if (t instanceof ArrayType) {
            return ((ArrayType) t).getElemType();
        }
        return t;
    }

    public ArrayList<Object> constructContainer() {
        if (getType() == TypeName.A) {
            ArrayList<Object> ret = new ArrayList<Object>();
            if (t instanceof ArrayType) {
                for (int i = 0; i < size; i++) {
                    ret.add(((ArrayType) t).constructContainer());
                }
            }
            return ret;
        }
        return null;
    }

    public boolean isValidPos(ArrayList<Integer> pos) {
        Type current = this;
        for (var i : pos) {
            if (current instanceof ArrayType) {
                if (i < 0 || i >= ((ArrayType) current).getSize()) {
                    return false;
                }
                current = ((ArrayType) current).t;
            } else {
                return false;
            }
        }
        return true;
    }

    public int getDim() {
        ArrayType current = this;
        int res = 1;
        while (current.t instanceof ArrayType) {
            res++;
            current = (ArrayType) current.t;
        }
        return res;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof ArrayType) {
            Type current = this;
            Type other = (ArrayType) obj;
            while(current instanceof ArrayType && other instanceof ArrayType) {
                if(((ArrayType) current).size != ((ArrayType) other).size) {
                    return false;
                }
                current = ((ArrayType) current).t;
                other = ((ArrayType) other).t;
            }
            return current.getType() == other.getType();
        } else if(obj instanceof Pointer){
            if(((Pointer) obj).pointTo instanceof VoidType) {
                return true;
            } else {
                return t.equals(((Pointer) obj).pointTo);
            }
        } else {
            return false;
        }
    }
}
