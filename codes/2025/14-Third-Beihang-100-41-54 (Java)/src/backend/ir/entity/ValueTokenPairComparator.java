package backend.ir.entity;

import java.util.Comparator;

/**
 * &#064;Classname ValueTokenComparator
 * &#064;Description  TODO
 * &#064;Date 2025/7/26 16:51
 * &#064;Created MuJue
 */
public class ValueTokenPairComparator implements Comparator<ValueTokenPair> {
    @Override
    public int compare(ValueTokenPair p1, ValueTokenPair p2) {
        // 按照 Value 是否为常量进行排序，常量排在前面
        boolean isConst1 = p1.getValue() instanceof LiteralConst;
        boolean isConst2 = p2.getValue() instanceof LiteralConst;

        if (isConst1 && !isConst2) return -1;
        if (!isConst1 && isConst2) return 1;
        return 0;
    }
}
