package ir.type;

public interface IrType {
    /**
     * 判断是否是给定的类型
     * @param tyName 类型名，可以是 INT, i32, ARRAY, i32[5] 等标准名或打印名
     * @return 是否是给定的类型
     */
    boolean is(String tyName);

    /**
     * 获取指定类型的大小
     * @return 以字节为单位的占用大小
     */
    int sizeof();

    /**
     * 判断两个类型是否相等
     * @param obj 要比较的对象
     * @return 是否相等
     */
    @Override
    boolean equals(Object obj);

    /**
     * 获取类型的哈希值
     * @return 哈希值
     */
    @Override
    int hashCode();
}
