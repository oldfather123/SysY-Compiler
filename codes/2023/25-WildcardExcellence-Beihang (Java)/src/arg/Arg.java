package arg;

import java.io.IOException;

public class Arg {
    private String inputFile = null;
    private String outputFile = "a.out";
    private boolean speed = false; // 是否做优化
    private boolean onlyLLVM = false; // 是否只输出 LLVM

    public Arg(String[] args) throws IOException {
        for (int i = 0; i < args.length; i++) {
            if (args[i].startsWith("-")) {
                if (args[i].equals("-S")) {
                    onlyLLVM = false;
                }
                else if (args[i].equals("-llvm")) {
                    onlyLLVM = true;
                }
                else if (args[i].equals("-O1")) {
                    speed = true;
                }
                else if (args[i].equals("-o")) {
                    if (i + 1 >= args.length || args[i + 1].startsWith("-")) {
                        errorFormat("Missing output file for -o");
                    }
                    outputFile = args[i + 1];
                    i++;
                }
                else {
                    errorFormat("Unknown argument " + args[i]);
                    return;
                }
            }
            else {
                inputFile = args[i];
            }
        }

        if (inputFile == null) {
            errorFormat("Missing input file");
            return;
        }
        if (outputFile == null) {
            errorFormat("Missing output file");
            return;
        }
    }

    public boolean isSpeed() {
        return speed;
    }

    public boolean isOnlyLLVM() {
        return onlyLLVM;
    }

    public String getInputFilePath() {
        return inputFile;
    }

    public String getOutputFilePath() {
        return outputFile;
    }

    private void errorFormat(String exStr) throws IOException {
        System.out.println(exStr);
        System.out.println("""
                -S：只产生汇编文件
                -llvm：只产生 LLVM
                -o <output>：指定生成文件的文件名
                -O1：打开优化开关
                """);
        System.out.println("如果你在用 IDEA，可以在 Run - Edit Configurations - Build and Run 中填写编译参数");
        throw new IOException();
    }
}
