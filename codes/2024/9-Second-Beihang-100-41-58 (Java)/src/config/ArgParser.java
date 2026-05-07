package config;

public class ArgParser {
    private String[] args;

    public ArgParser(String[] args) {
        this.args = args;
    }

    public void parseArgs() {
        for (String arg : args) {
            if (arg.contains(".sy")) {
                Config.inputFile = arg;
            }
            if (arg.contains(".s") && !arg.contains(".sy")) {
                Config.outputFile = arg;
            }
            if (arg.equals("-mid")) {
                Config.BackEnd = false;
            }
            if (arg.contains(".ll")) {
                Config.llvmoptFile = arg;
            }
            if (arg.equals("-O1")) {
                Config.isO1 = true;
            }
        }
    }
}
