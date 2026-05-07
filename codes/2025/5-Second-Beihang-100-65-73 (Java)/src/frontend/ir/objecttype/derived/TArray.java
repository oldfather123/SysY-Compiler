package frontend.ir.objecttype.derived;

import frontend.ir.objecttype.TVoid;
import frontend.ir.objecttype.Type;

import java.util.ArrayList;
import java.util.List;

/**
 * 数组类型
 */
public class TArray extends DerivedType {
    private final Type elementType;
    private final Type baseElementType;
    protected final List<Integer> limList;
    private final String limListString;

    public TArray(Type elementType, int elementNumber) {
        if (elementType == null || elementType instanceof TVoid) {
            throw new RuntimeException("无效的元素类型");
        }
        this.elementType = elementType;

        if (elementType instanceof TArray) {
            baseElementType = ((TArray) elementType).getBaseElementType();
            this.limList = new ArrayList<>(((TArray) elementType).limList);
        } else {
            baseElementType = elementType;
            this.limList = new ArrayList<>();
        }

        this.limListString = this.limList2string();
        this.limList.add(0, elementNumber);
    }

    private String limList2string() {
        StringBuilder sb = new StringBuilder();
        for (Integer integer : limList) {
            sb.append("[").append(integer).append("]");
        }
        return sb.toString();
    }

    /**
     * @return 数组维数，即有多少个 lim
     */
    public int getDim() {
        return this.limList.size();
    }

    public Type getElementType() {
        return elementType;
    }

    public Type getBaseElementType() {
        return baseElementType;
    }

    public int getLim() {
        return this.limList.get(0);
    }

    public List<Integer> getLimList() {
        return limList;
    }

    @Override
    public String printIRType() {
        return "[" + this.getLim() + " x " + this.getElementType().printIRType() + "]";
    }

    @Override
    public boolean equals(Object obj) {
        if (!(obj instanceof TArray)) {
            return false;
        }
        if (!this.baseElementType.equals(((TArray) obj).baseElementType)) {
            return false;
        }

        return this.limListString.equals(((TArray) obj).limListString);
    }

    /**
     * @return 数组共有多少个元素
     */
    public int getFullSize() {
        int size = 1;
        for (int lim : limList) {
            size *= lim;
        }
        return size;
    }

    public List<Integer> getSizeList() {
        int fullSize = this.getFullSize() * 4;
        List<Integer> sizeList = new ArrayList<>();
        for (int lim : limList) {
            sizeList.add(fullSize);
            fullSize /= lim;
        }
        sizeList.add(fullSize);
        return sizeList;
    }

    public int getByteSize() {
        return this.getFullSize() * this.getBaseElementType().getByteSize();
    }
}
