package cn.edu.bit.newnewcc;

public class CompilerOptions {
    String[] inputFileNames;
    String outputFileName;
    int optimizationLevel;
    boolean emitAssembly;
    boolean emitLLVM;

    public CompilerOptions(String[] inputFileNames, String outputFileName, int optimizationLevel, boolean emitAssembly, boolean emitLLVM) {
        this.inputFileNames = inputFileNames;
        this.outputFileName = outputFileName;
        this.optimizationLevel = optimizationLevel;
        this.emitAssembly = emitAssembly;
        this.emitLLVM = emitLLVM;
    }

    public String[] getInputFileNames() {
        return inputFileNames;
    }

    public String getOutputFileName() {
        return outputFileName;
    }

    public int getOptimizationLevel() {
        return optimizationLevel;
    }

    public boolean isEmitAssembly() {
        return emitAssembly;
    }

    public boolean isEmitLLVM() {
        return emitLLVM;
    }

}
