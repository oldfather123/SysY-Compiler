package entities;

import java.util.List;
import java.util.Map;

public record Variable(String name, entities.Variable.Type type, int size, List<Integer> dims, List<Integer> dimSum, Map<Integer, FOI> values) {

    public FOI getValue(List<Integer> dim) {
        int index = 0;
        for (int i = 0; i < dim.size(); i++) {
            index += dimSum.get(i) * dim.get(i);
        }
        return isInteger() ? values.getOrDefault(index, FOI.ZERO).toInt()
                : values.getOrDefault(index, FOI.ZERO).toFloat();
    }

    public boolean isInteger() {
        return type == Type.INT || type == Type.INT_ARRAY;
    }

    public boolean isArray() {
        return type == Type.INT_ARRAY || type == Type.FLOAT_ARRAY;
    }

    public static Type getType(Word bType, int dim) {
        return dim == 0 ? (bType.is(Word.Type.INT) ? Type.INT : Type.FLOAT) :
                (bType.is(Word.Type.INT) ? Type.INT_ARRAY : Type.FLOAT_ARRAY);
    }

    public enum Type {
        INT, FLOAT, INT_ARRAY, FLOAT_ARRAY
    }

}
