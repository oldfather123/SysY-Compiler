import handler.OptHandler;
import main.Main;

public class Compiler {

    public static void main(String[] args) {
        String input = null, output = null;
        for (String arg : args) {
            if (arg.equals("-O1")) {
                OptHandler.shouldOpt = true;
            } else if (arg.endsWith(".s")) {
                output = arg;
            } else if (arg.endsWith(".sy")) {
                input = arg;
            }
        }
        if (input == null || output == null) {
            throw new RuntimeException("Invalid arguments");
        }
        Main.doCompile(input, output);
    }

}
