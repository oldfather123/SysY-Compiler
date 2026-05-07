import cn.edu.bit.newnewcc.Main;

public class Compiler implements Runnable {
    public static void main(String[] args) throws InterruptedException {
        Thread compileThread = new Thread(null, new Compiler(args), "CompileThread", 1L << 30);
        compileThread.start();
        compileThread.join();
    }

    private final String[] args;

    private Compiler(String[] args) {
        this.args = args;
    }

    @Override
    public void run() {
        try {
            Main.main(args);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

}
