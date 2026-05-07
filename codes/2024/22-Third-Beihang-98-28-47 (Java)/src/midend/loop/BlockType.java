package midend.loop;

public enum BlockType {
    /*
    * TODO:
    *  多次循环，重复染色？
    *  每次构建前的清理？
    *  dealwhile latch & exiting 不会相同
    * */
    OUTOFLOOP(0),
    HEADER(1),
    PREHEADER(1 << 1),
    EXIT(1 << 2),
    LATCH(1 << 3),
    INLOOP(1 << 4),
    EXITING(1 << 5),
    LOOPEXIT(1 << 6),
    NOTSIMPLE(Integer.MAX_VALUE);

    private int num;

    BlockType(int num) {
        this.num = num;
    }

    public int getNum() {
        return num;
    }
}
