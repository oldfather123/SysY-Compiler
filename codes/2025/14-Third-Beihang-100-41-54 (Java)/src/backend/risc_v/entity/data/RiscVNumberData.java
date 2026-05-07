package backend.risc_v.entity.data;

import backend.risc_v.entity.type.RiscVBasicType;

import java.util.ArrayList;

public class RiscVNumberData extends RiscVData {
    private final ArrayList<Number> dataList;
    private final boolean isEmpty;
    private final int size;

    public RiscVNumberData(String name, RiscVBasicType basicType) {
        super(name, basicType);
        this.dataList = new ArrayList<>();
        isEmpty = false;
        size = 0;
    }
    public RiscVNumberData(String name, boolean isEmpty, int size, RiscVBasicType basicType) {
        super(name, basicType);
        this.dataList = new ArrayList<>();
        this.isEmpty = isEmpty;
        this.size = size;
    }
    public RiscVNumberData(String name, Number data, RiscVBasicType basicType) {
        this(name, basicType);
        this.dataList.add(data);
    }

    public void addData(Number data) {
        this.dataList.add(data);
    }

    @Override
    public String toString() {
        if (isEmpty) {
            return name + ": .zero " + size;
        } else {
            StringBuilder sb = new StringBuilder(super.toString());
            for (int i = 0; i < dataList.size(); i++) {
                sb.append(dataList.get(i));
                if (i != dataList.size() - 1) {
                    sb.append(", ");
                }
            }
            return sb.toString();
        }
    }
}
