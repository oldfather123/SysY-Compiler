package backend.RISCVCode;

import backend.RISCVCode.RISCVInstruction.RISCVInstruction;

import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
/**
 * RISCVCode 类用于生成储存 RISCV 汇编代码。
 * 它包含了一个函数列表和一个全局变量映射。
 */
public class RISCVCode {
    private ArrayList<RISCVFunction> functions;

    private Map<String, Integer> globalIntVar;

    private Map<String, Float>globalFloatVar;

    private Map<String,List<Integer>>globalIntArrayVar;

    private Map<String,List<Float>>globalFloatArrayVar;

    private Map<String,Integer> globalAllZeroArrayVar;

    public RISCVCode() {
        functions = new ArrayList<>();
        globalIntVar = new HashMap<>();
        globalFloatVar = new HashMap<>();
        globalIntArrayVar = new HashMap<>();
        globalFloatArrayVar = new HashMap<>();
        globalAllZeroArrayVar = new HashMap<>();
    }

    /**
     * 添加一个 RISCV 函数到函数列表中。
     *
     * @param function 要添加的 RISCV 函数
     */
    public void addFunction(RISCVFunction function) {
        functions.add(function);
    }

    /**
     * 添加一个全局变量及其初始值到全局变量映射中。
     *
     * @param varName 全局变量的名称
     * @param value   全局变量的初始值
     */
    public void addGlobalVar(String varName, Integer value) {
        globalIntVar.put(varName, value);
    }

    public void addGlobalVar(String varName, Float value) {
        globalFloatVar.put(varName, value);
    }

    public void addGlobalIntArrayVar(String varName, List<Integer> value) {
        globalIntArrayVar.put(varName, value);
    }
    public void addGlobalFloatArrayVar(String varName, List<Float> value) {
        globalFloatArrayVar.put(varName, value);
    }
    public void addGlobalAllZeroArray(String varName, int size) {
        globalAllZeroArrayVar.put(varName, size);
    }
    public ArrayList<RISCVFunction> getFunctions(){return functions;}

    /**
     * 生成 RISCV 汇编代码的字符串表示。
     *
     * @return 生成的 RISCV 汇编代码字符串
     */
    public void generateRISCVCode(String outputPath)  {
        StringBuilder outputStringBuilder = new StringBuilder();
        try (FileWriter writer = new FileWriter(outputPath)) {
            writer.write(outputStringBuilder.toString());
            writer.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        outputStringBuilder.append("  .data\n");
        if (!globalIntVar.isEmpty()) {
            for (String name : globalIntVar.keySet()) {
                outputStringBuilder.append(name).append(":\n");
                outputStringBuilder.append("  .dword ").append(globalIntVar.get(name)).append("\n\n");
                if (outputStringBuilder.length() > 10000) {
                    try (FileWriter writer = new FileWriter(outputPath, true)) {
                        writer.write(outputStringBuilder.toString());
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    outputStringBuilder = new StringBuilder();
                }
            }
        }
        if (!globalFloatVar.isEmpty()) {
            for (String name : globalFloatVar.keySet()) {
                outputStringBuilder.append(name).append(":\n");
                outputStringBuilder.append("  .dword 0X").append(Integer.toHexString(Float.floatToIntBits(globalFloatVar.get(name)))).append("\n\n");
                if (outputStringBuilder.length() > 10000) {
                    try (FileWriter writer = new FileWriter(outputPath, true)) {
                        writer.write(outputStringBuilder.toString());
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    outputStringBuilder = new StringBuilder();
                }
            }
        }
        if (!globalIntArrayVar.isEmpty()) {
            for (String name : globalIntArrayVar.keySet()) {
                outputStringBuilder.append(name).append(":\n");
                outputStringBuilder.append("  .word ");
                List<Integer> value = globalIntArrayVar.get(name);
                for(int i = 0; i < value.size(); i++) {
                    outputStringBuilder.append(value.get(i));
                    if (i != value.size() - 1) {
                        outputStringBuilder.append(", ");
                    }
                    if (outputStringBuilder.length() > 10000) {
                        try (FileWriter writer = new FileWriter(outputPath, true)) {
                            writer.write(outputStringBuilder.toString());
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                        outputStringBuilder = new StringBuilder();
                    }
                }
                outputStringBuilder.append("\n\n");
            }
        }
        if (!globalFloatArrayVar.isEmpty()) {
            for (String name : globalFloatArrayVar.keySet()) {
                outputStringBuilder.append(name).append(":\n");
                outputStringBuilder.append("  .word ");
                List<Float> value = globalFloatArrayVar.get(name);
                for(int i = 0; i < value.size(); i++) {
                    outputStringBuilder.append("0X").append(Integer.toHexString(Float.floatToIntBits(value.get(i))));
                    if (i != value.size() - 1) {
                        outputStringBuilder.append(", ");
                    }
                    if (outputStringBuilder.length() > 10000) {
                        try (FileWriter writer = new FileWriter(outputPath, true)) {
                            writer.write(outputStringBuilder.toString());
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                        outputStringBuilder = new StringBuilder();
                    }
                }
                outputStringBuilder.append("\n\n");
            }
        }
        if(!globalAllZeroArrayVar.isEmpty()) {
            outputStringBuilder.append("  .bss \n");
            for (String name : globalAllZeroArrayVar.keySet()) {
                outputStringBuilder.append(name).append(":\n");
                outputStringBuilder.append("  .space ").append(globalAllZeroArrayVar.get(name) * 4).append("\n");
                outputStringBuilder.append("\n");
                if (outputStringBuilder.length() > 10000) {
                    try (FileWriter writer = new FileWriter(outputPath, true)) {
                        writer.write(outputStringBuilder.toString());
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    outputStringBuilder = new StringBuilder();
                }
            }
        }
        outputStringBuilder.append("  .text\n");
        outputStringBuilder.append("  .globl main\n");
        for(RISCVFunction function : functions) {
            outputStringBuilder.append(function.getFunctionLabel().getName()).append(":");
            if (function.getStackSize() < 2048) {
                outputStringBuilder.append("  addi sp, sp, ").append(-function.getStackSize()).append("\n");
                if (outputStringBuilder.length() > 10000) {
                    try (FileWriter writer = new FileWriter(outputPath, true)) {
                        writer.write(outputStringBuilder.toString());
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    outputStringBuilder = new StringBuilder();
                }
            }
            else {
                outputStringBuilder.append("  li s0, ").append(-function.getStackSize()).append("\n");
                if (outputStringBuilder.length() > 10000) {
                    try (FileWriter writer = new FileWriter(outputPath, true)) {
                        writer.write(outputStringBuilder.toString());
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    outputStringBuilder = new StringBuilder();
                }
                outputStringBuilder.append("  add sp, sp, s0").append("\n");
                if (outputStringBuilder.length() > 10000) {
                    try (FileWriter writer = new FileWriter(outputPath, true)) {
                        writer.write(outputStringBuilder.toString());
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    outputStringBuilder = new StringBuilder();
                }

            }
            for(RISCVBlock block : function.getBlocks()) {
                outputStringBuilder.append(block.getBlockLabel().getName()).append(":\n");
                for (RISCVInstruction code : block.getInstructions()) {
                    outputStringBuilder.append(code.getString());
                    if (outputStringBuilder.length() > 10000) {
                        try (FileWriter writer = new FileWriter(outputPath, true)) {
                            writer.write(outputStringBuilder.toString());
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                        outputStringBuilder = new StringBuilder();
                    }
                }
            }

        }
        try (FileWriter writer = new FileWriter(outputPath, true)) {
            writer.write(outputStringBuilder.toString());
            writer.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
