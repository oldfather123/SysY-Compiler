package debug;

public class DEBUG {
//    public static  boolean debug1 = true;
     public static  boolean debug1 = false;

    public static void dbgPrint1(String string) {
        if (debug1) {
            System.out.println(string);
        }
    }
    public static void dbgPrint2(String string) {
        if (debug1) {
            System.out.println("\t" + string);
        }
    }
    public static void dbgPrint(String string) {
        if (debug1) {
            System.out.print(string);
        }
    }
}
