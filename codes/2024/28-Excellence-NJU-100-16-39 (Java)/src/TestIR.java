import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Objects;


public class TestIR {
    public static void main(String[] args) throws IOException, InterruptedException {
        String testPath = "/home/ang/compilers/compiler2023/公开样例与运行时库/functional";
        File directory = new File(testPath);
        List<File> fileList;

        // 获取文件列表并过滤
        fileList = new ArrayList<>();
        for (File file : Objects.requireNonNull(directory.listFiles())) {
            if (file.isFile() && file.getName().endsWith(".sy")) {
                fileList.add(file);
            }
        }

        // 按文件名排序
        fileList.sort(Comparator.comparing(File::getName));

        for (File f : fileList) {
            // 生成.ll文件
            Compiler.main(new String[]{"-S", "-o", "./tests/test1.s", f.getAbsolutePath()});

            // 使用llc工具将.ll文件转换为.o文件
            ProcessBuilder pb1 = new ProcessBuilder("llc", "-filetype=obj", "-o", "./tests/output.o", "./tests/test1.ll", "--relocation-model=pic");
            Process process = pb1.start();
            int processExitCode = process.waitFor();
            if (processExitCode != 0) {
                System.err.println("llc failed for " + f.getName());
                continue;
            }

            // 使用gcc工具将.o文件和库文件链接为可执行文件
            ProcessBuilder pb2 = new ProcessBuilder("gcc", "./tests/output.o", "/home/ang/compilers/compiler2024-x/res/libsylib.a", "-o", "./tests/output");
            process = pb2.start();
            processExitCode = process.waitFor();
            if (processExitCode != 0) {
                System.err.println("gcc failed for " + f.getName());
                continue;
            }

            // 查找对应的输入文件
            String name = f.getName();
            String inName = name.substring(0, name.length() - 3) + ".in";
            File inFile = new File(testPath + "/" + inName);

            // 程序的return值存在processExitCode中，PRINT的输出存在output文件中
            // 使用生成的可执行文件进行测试
            ProcessBuilder pb3;
            if (inFile.exists()) {
                pb3 = new ProcessBuilder("./tests/output");
                pb3.redirectInput(inFile);
            } else {
                pb3 = new ProcessBuilder("./tests/output");
            }
            File outputFile = new File("./tests/test1.out");
            pb3.redirectOutput(outputFile);
            process = pb3.start();
            processExitCode = process.waitFor();

            // 读取测试输出PRINT的文件,可能为空
            FileReader inReader = new FileReader(outputFile);
            StringBuilder inBuilder = new StringBuilder();
            int c;
            while ((c = inReader.read()) != -1) {
                inBuilder.append((char) c);
            }
            inReader.close();

            // 查找对应的.out文件
            String outName = name.substring(0, name.length() - 3) + ".out";
            File outFile = new File(testPath + "/" + outName);
            if (!outFile.exists()) {
                System.out.println("Reference output file not found: " + outName);
                continue;
            }

            // 读取参考输出文件
            FileReader outReader = new FileReader(outFile);
            StringBuilder outBuilder = new StringBuilder();
            while ((c = outReader.read()) != -1) {
                outBuilder.append((char) c);
            }
            outReader.close();

            //outreader的最后一行是return值，前面是print的输出
            String out = outBuilder.toString();
            String in = inBuilder.toString();
            //截取out的最后一行
            int index = out.lastIndexOf("\n") - 1;
            while(out.charAt(index) != '\n'&& index != 0) {
                index--;
            }
            if(index == 0){
                index = -1;
            }
            String outReturn = out.substring(index + 1, out.length()- 1);
            out = out.substring(0,index + 1);
            int return_value = Integer.parseInt(outReturn);

            in = in.trim();
            out = out.trim();
            if(processExitCode == return_value && in.equals(out)) {
                System.out.println(f.getName() + " passed");
            } else {
                System.err.println(f.getName() + " failed");
            }
        }
    }
}