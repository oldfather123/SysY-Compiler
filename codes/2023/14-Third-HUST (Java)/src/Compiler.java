import execute.Executor;

public class Compiler {
    private Compiler() {
    }

    public static void main(String[] args) {
        Executor executor = new Executor(args);
        executor.execute();
    }
}
