package tools;

public class ErrOutput {
    public static boolean DEBUG = false;
    public static void outputErr(String message){
        System.out.println(message);
        assert false;
        System.exit(20);
    }
}
