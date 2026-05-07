package IO;

import java.io.*;

public class Arg {
    public static Arg ARG;

    private final int optLevel;
    private final boolean printIR;
    private final boolean skipBackEnd;
    private final boolean time;
    private final Framework framework;
    private final FileInputStream srcStream;    // 源代码输入流
    private final FileWriter asmWriter;         // 汇编代码输出流
    private final FileWriter irWriter;          // LLVM-IR 代码输出流
    
    private Arg(String src, String asm, String ll, int optimize, boolean skipBackEnd, boolean time, Framework framework)
            throws IOException {
        this.optLevel    = optimize;
        this.printIR     = ll != null;
        this.skipBackEnd = skipBackEnd;
        this.time        = time;
        this.framework    = framework;
        
        this.irWriter    = this.printIR ? new FileWriter(ll) : null;
        this.srcStream   = new FileInputStream(src);
        this.asmWriter   = new FileWriter(asm);
    }
    
    public static void parse(String[] args) throws IOException {
        String src = "in.sy";
        String asm = "output.s";
        String ll  = null;
        Framework framework  = Framework.RV;
        int optLevel = 0;
        boolean skipBackEnd = false;
        boolean time = false;
        
        for (int i = 0; i < args.length; i++) {
            String arg = args[i];
            if (arg.contains(".sy")) {
                src = arg;
                continue;
            }
            if (arg.equals("-o")) {
                asm = args[++i];
                continue;
            }
            if (arg.equals("-S")) {
                continue;
            }
            if (arg.equals("-mid")) {
                skipBackEnd = true;
                continue;
            }
            if (arg.equals("-time")) {
                time = true;
                continue;
            }
            if (arg.matches("-O[0-9]")) {
                optLevel = Integer.parseInt(arg.substring(2));
                continue;
            }
            if (arg.equals("-ll")) {
                ll = args[++i];
            }
            if (arg.equals("-f")) {
                framework = args[++i].equalsIgnoreCase("RV") ? Framework.RV : Framework.ARM;
            }
        }

        ARG = new Arg(src, asm, ll, optLevel, skipBackEnd, time, framework);
    }
    
    public FileInputStream getSrcStream() {
        return srcStream;
    }
    
    public int getOptLevel() {
        return optLevel;
    }
    
    public boolean toPrintIR() {
        return printIR;
    }
    
    public FileWriter getAsmWriter() {
        return asmWriter;
    }
    
    public FileWriter getIrWriter() {
        return irWriter;
    }
    
    public boolean toSkipBackEnd() {
        return skipBackEnd;
    }
    
    public boolean toTime() {
        return time;
    }

    public Framework getFramework() {
        return framework;
    }

    public boolean debug() {
        return false;
    }

    public enum Framework {
        RV,
        ARM,
    }
}
