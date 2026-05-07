package utils;

public class GlobalCounterForInstr {
    private static int num = 0;
    public static int getNewId() {
        return num++;
    }
}
